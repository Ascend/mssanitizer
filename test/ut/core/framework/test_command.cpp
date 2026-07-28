/* -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * ------------------------------------------------------------------------- */


#include <gtest/gtest.h>
#include <fstream>
#include <climits>
#include <experimental/filesystem>

#include "command.h"
#include "record_defs.h"
#include "protocol.h"
#include "thread_manager.h"
#include "utility/serializer.h"
#include "utility/path.h"
#include "utility/umask_guard.h"

using namespace Sanitizer;
using namespace std;
namespace fs = std::experimental::filesystem;
constexpr mode_t DIR_DEFAULT_MOD = 0750;

class TestCommand : public testing::Test {
public:
    static void SetUpTestCase()
    {
        char buf[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        ASSERT_TRUE(len > 0);
        buf[len] = 0;
        Path exePath{buf};
        TestCommand::launcherPath_ = exePath.Parent() / Path("kernel-launcher");
        ofstream outfile;
        outfile.open(TestCommand::launcherPath_.ToString().c_str(), ios::out);
    }

    static void TearDownTestCase()
    {
        fs::remove_all(TestCommand::launcherPath_.ToString().c_str());
    }

    void SetUp() override
    {
        dumpPath_ = "./_temp_dumpDir";
        ASSERT_EQ(mkdir(dumpPath_.c_str(), DIR_DEFAULT_MOD), 0);
    }

    void TearDown() override
    {
        fs::remove_all(dumpPath_.c_str());
        fs::remove_all("/tmp/test.log");
    }

public:
    static Path launcherPath_;
    string dumpPath_;
};

Path TestCommand::launcherPath_{""};

TEST_F(TestCommand, run_ls_command_expect_success)
{
    std::vector<std::string> paramList = {"/bin/ls"};
    Config config {
        .defaultCheck = true,
        .memCheck = true,
    };
    Command command(config, false, LogLv::INFO, "/tmp/test.log");
    command.Exec(paramList);
}

void ParseIPCResponse(const std::string &msg, IPCResponse &response)
{
    PacketType type;
    ASSERT_TRUE(Deserialize(msg, type));
    ASSERT_EQ(type, PacketType::IPC_RESPONSE);
    ASSERT_TRUE(Deserialize(msg.substr(sizeof(type)), response));
}

void ResetIPCMap()
{
    Command::sharedMemInfoMp.clear();
    Command::shareeMemInfoMp.clear();
}

using ForkJob = std::function<void()>;
using ForkJobList = std::vector<ForkJob>;
using ChildPidContainer = std::vector<int>;

class ResponseHandler {
public:
    static void Set(const std::string &resp)
    {
        response = resp;
    }
    static std::string response;
};
std::string ResponseHandler::response = {};

auto RespFunc = std::function<void(const std::string &)>(ResponseHandler::Set);

int DispatchForkJobs(ChildPidContainer &container, const ForkJobList &jobList)
{
    if (jobList.empty()) {
        return -1;
    }
    int pid = -1;
    for (auto job : jobList) {
        if (job == nullptr) {
            std::cout << "error happened when calling job: (nullptr)." << std::endl;
            return -1;
        }
        pid = fork();
        if (pid == 0) {
            job();
            _exit(0);
        } else if (pid > 0) {
            container.push_back(pid);
        } else {
            std::cout << "error happened when calling fork." << std::endl;
        }
    }
    return pid;
}

TEST_F(TestCommand, IPC_functions_basic)
{
    const char *ipcMemName = "SHARED_MEMORY";
    Config config{
        .defaultCheck = true,
        .memCheck = true,
    };
    ThreadManager threadManager(config, false, LogLv::INFO, "/tmp/test.log");
    Checker checker(config, false);
    IPCMemRecord record1;
    record1.type = IPCOperationType::SET_INFO;
    constexpr uint64_t randAddr = 123;
    constexpr uint64_t randSize = 1024;
    record1.setInfo.addr = randAddr;
    record1.setInfo.size = randSize;
    strcpy_s(record1.setInfo.name, sizeof(record1.setInfo.name), ipcMemName);
    HandleIpcMemRecord(checker, record1, threadManager, RespFunc);
    auto responseStr = ResponseHandler::response;
    IPCResponse response;
    ParseIPCResponse(responseStr, response);
    ASSERT_EQ(response.type, record1.type);
    ASSERT_EQ(response.status, ResponseStatus::SUCCESS);

    ChildPidContainer childPidVec{};
    auto workerJob = [&config, &checker, &ipcMemName]() {
        ThreadManager threadManager(config, false, LogLv::INFO, "/tmp/test.log");
        IPCResponse response;
        IPCMemRecord record3;
        record3.type = IPCOperationType::MAP_INFO;
        record3.mapInfo.addr = randAddr;
        strcpy_s(record3.mapInfo.name, sizeof(record3.mapInfo.name), ipcMemName);
        HandleIpcMemRecord(checker, record3, threadManager, RespFunc);
        auto responseStr = ResponseHandler::response;
        ParseIPCResponse(responseStr, response);
        ASSERT_EQ(response.type, record3.type);
        ASSERT_EQ(response.status, ResponseStatus::SUCCESS);

        IPCMemRecord record4;
        record4.type = IPCOperationType::UNMAP_INFO;
        record4.unmapInfo.addr = 123;
        HandleIpcMemRecord(checker, record4, threadManager, RespFunc);
        responseStr = ResponseHandler::response;
        ParseIPCResponse(responseStr, response);
        ASSERT_EQ(response.type, record4.type);
        ASSERT_EQ(response.status, ResponseStatus::SUCCESS);
    };
    ForkJobList jobList{workerJob, workerJob, workerJob};
    int pid = DispatchForkJobs(childPidVec, jobList);
    if (pid != 0) {
        int status;
        for (auto pid : childPidVec) {
            waitpid(pid, &status, 0);
        }

        IPCMemRecord record2;
        record2.type = IPCOperationType::DESTROY_INFO;
        strcpy_s(record2.destroyInfo.name, sizeof(record2.destroyInfo.name), ipcMemName);
        HandleIpcMemRecord(checker, record2, threadManager, RespFunc);
        responseStr = ResponseHandler::response;
        ParseIPCResponse(responseStr, response);
        ASSERT_EQ(response.type, record2.type);
        ASSERT_EQ(response.status, ResponseStatus::SUCCESS);
        ResetIPCMap();
    }
}

TEST_F(TestCommand, IPC_function_repeat_operation_expect_fail)
{
    const char *ipcMemName = "SHARED_MEMORY";
    Config config{
        .defaultCheck = true,
        .memCheck = true,
    };
    ThreadManager threadManager(config, false, LogLv::INFO, "/tmp/test.log");
    Checker checker(config, false);
    IPCMemRecord record1;
    record1.type = IPCOperationType::SET_INFO;
    constexpr uint64_t randAddr = 123;
    constexpr uint64_t randSize = 1024;
    record1.setInfo.addr = randAddr;
    record1.setInfo.size = randSize;
    strcpy_s(record1.setInfo.name, sizeof(record1.setInfo.name), ipcMemName);
    HandleIpcMemRecord(checker, record1, threadManager, RespFunc);
    auto responseStr = ResponseHandler::response;
    IPCResponse response;
    ParseIPCResponse(responseStr, response);
    ASSERT_EQ(response.type, record1.type);
    ASSERT_EQ(response.status, ResponseStatus::SUCCESS);

    HandleIpcMemRecord(checker, record1, threadManager, RespFunc);
    responseStr = ResponseHandler::response;
    ParseIPCResponse(responseStr, response);
    ASSERT_EQ(response.type, record1.type);
    ASSERT_EQ(response.status, ResponseStatus::FAIL);

    ChildPidContainer childPidVec{};
    auto workerJob = [&config, &checker, &ipcMemName]() {
        ThreadManager threadManager(config, false, LogLv::INFO, "/tmp/test.log");
        IPCResponse response;
        IPCMemRecord record3;
        record3.type = IPCOperationType::MAP_INFO;
        record3.mapInfo.addr = randAddr;
        strcpy_s(record3.mapInfo.name, sizeof(record3.mapInfo.name), ipcMemName);
        HandleIpcMemRecord(checker, record3, threadManager, RespFunc);
        auto responseStr = ResponseHandler::response;
        ParseIPCResponse(responseStr, response);
        ASSERT_EQ(response.type, record3.type);
        ASSERT_EQ(response.status, ResponseStatus::SUCCESS);

        HandleIpcMemRecord(checker, record3, threadManager, RespFunc);
        responseStr = ResponseHandler::response;
        ParseIPCResponse(responseStr, response);
        ASSERT_EQ(response.type, record3.type);
        ASSERT_EQ(response.status, ResponseStatus::FAIL);

        IPCMemRecord record4;
        record4.type = IPCOperationType::UNMAP_INFO;
        record4.unmapInfo.addr = randAddr;
        HandleIpcMemRecord(checker, record4, threadManager, RespFunc);
        responseStr = ResponseHandler::response;
        ParseIPCResponse(responseStr, response);
        ASSERT_EQ(response.type, record4.type);
        ASSERT_EQ(response.status, ResponseStatus::SUCCESS);

        HandleIpcMemRecord(checker, record4, threadManager, RespFunc);
        responseStr = ResponseHandler::response;
        ParseIPCResponse(responseStr, response);
        ASSERT_EQ(response.type, record4.type);
        ASSERT_EQ(response.status, ResponseStatus::FAIL);
    };
    ForkJobList jobList{workerJob};
    int pid = DispatchForkJobs(childPidVec, jobList);
    if (pid != 0) {
        int status;
        for (auto pid : childPidVec) {
            waitpid(pid, &status, 0);
        }

        IPCMemRecord record2;
        record2.type = IPCOperationType::DESTROY_INFO;
        strcpy_s(record2.destroyInfo.name, sizeof(record2.destroyInfo.name), ipcMemName);
        HandleIpcMemRecord(checker, record2, threadManager, RespFunc);
        responseStr = ResponseHandler::response;
        ParseIPCResponse(responseStr, response);
        ASSERT_EQ(response.type, record2.type);
        ASSERT_EQ(response.status, ResponseStatus::SUCCESS);

        HandleIpcMemRecord(checker, record2, threadManager, RespFunc);
        responseStr = ResponseHandler::response;
        ParseIPCResponse(responseStr, response);
        ASSERT_EQ(response.type, record2.type);
        ASSERT_EQ(response.status, ResponseStatus::FAIL);
        ResetIPCMap();
    }
}

TEST(Command, IPC_function_open_without_set_expect_fail)
{
    const char *ipcMemName = "SHARED_MEMORY";
    Config config{
        .defaultCheck = true,
        .memCheck = true,
    };
    ThreadManager threadManager(config, false, LogLv::INFO, "/tmp/test.log");
    Checker checker(config, false);
    IPCMemRecord record1;
    record1.type = IPCOperationType::MAP_INFO;
    constexpr uint64_t randAddr = 123;
    record1.mapInfo.addr = 123;
    strcpy_s(record1.mapInfo.name, sizeof(record1.mapInfo.name), ipcMemName);
    HandleIpcMemRecord(checker, record1, threadManager, RespFunc);
    auto responseStr = ResponseHandler::response;
    IPCResponse response;
    ParseIPCResponse(responseStr, response);
    ASSERT_EQ(response.type, record1.type);
    ASSERT_EQ(response.status, ResponseStatus::FAIL);
    ResetIPCMap();
}

TEST(Command, IPC_function_destroy_without_set_expect_fail)
{
    const char *ipcMemName = "SHARED_MEMORY";
    Config config{
        .defaultCheck = true,
        .memCheck = true,
    };
    ThreadManager threadManager(config, false, LogLv::INFO, "/tmp/test.log");
    Checker checker(config, false);
    IPCMemRecord record1;
    record1.type = IPCOperationType::DESTROY_INFO;
    strcpy_s(record1.destroyInfo.name, sizeof(record1.destroyInfo.name), ipcMemName);
    HandleIpcMemRecord(checker, record1, threadManager, RespFunc);
    auto responseStr = ResponseHandler::response;
    IPCResponse response;
    ParseIPCResponse(responseStr, response);
    ASSERT_EQ(response.type, record1.type);
    ASSERT_EQ(response.status, ResponseStatus::FAIL);
    ResetIPCMap();
}

TEST(Command, IPC_function_close_without_open_expect_fail)
{
    const char *ipcMemName = "SHARED_MEMORY";
    Config config{
        .defaultCheck = true,
        .memCheck = true,
    };
    ThreadManager threadManager(config, false, LogLv::INFO, "/tmp/test.log");
    Checker checker(config, false);
    IPCMemRecord record1;
    constexpr uint64_t randAddr = 123;
    record1.type = IPCOperationType::UNMAP_INFO;
    record1.unmapInfo.addr = randAddr;
    strcpy_s(record1.setInfo.name, sizeof(record1.setInfo.name), ipcMemName);
    HandleIpcMemRecord(checker, record1, threadManager, RespFunc);
    auto responseStr = ResponseHandler::response;
    IPCResponse response;
    ParseIPCResponse(responseStr, response);
    ASSERT_EQ(response.type, record1.type);
    ASSERT_EQ(response.status, ResponseStatus::FAIL);
    ResetIPCMap();
}

TEST(Command, IPC_function_unknown_type_expect_no_response)
{
    const char *ipcMemName = "SHARED_MEMORY";
    Config config{
        .defaultCheck = true,
        .memCheck = true,
    };
    ThreadManager threadManager(config, false, LogLv::INFO, "/tmp/test.log");
    Checker checker(config, false);
    IPCMemRecord record1;
    record1.type = static_cast<IPCOperationType>(static_cast<uint32_t>(IPCOperationType::UNMAP_INFO) + 1);
    record1.unmapInfo.addr = 123;
    strcpy_s(record1.setInfo.name, sizeof(record1.setInfo.name), ipcMemName);
    HandleIpcMemRecord(checker, record1, threadManager, RespFunc);
    auto responseStr = ResponseHandler::response;
    ASSERT_TRUE(responseStr.empty());
}

TEST(Command, IPC_operate_the_wrong_name_expect_fail)
{
    const char *ipcMemName = "SHARED_MEMORY";
    const char *ipcMemNameWrong = "SHARED_MEMORY_WRONG";
    Config config{
        .defaultCheck = true,
        .memCheck = true,
    };
    ThreadManager threadManager(config, false, LogLv::INFO, "/tmp/test.log");
    Checker checker(config, false);
    IPCMemRecord record1;
    record1.type = IPCOperationType::SET_INFO;
    constexpr uint64_t randAddr = 123;
    constexpr uint64_t randSize = 1024;
    record1.setInfo.addr = randAddr;
    record1.setInfo.size = randSize;
    strcpy_s(record1.setInfo.name, sizeof(record1.setInfo.name), ipcMemName);
    HandleIpcMemRecord(checker, record1, threadManager, RespFunc);
    auto responseStr = ResponseHandler::response;
    IPCResponse response;
    ParseIPCResponse(responseStr, response);
    ASSERT_EQ(response.type, record1.type);
    ASSERT_EQ(response.status, ResponseStatus::SUCCESS);

    ChildPidContainer childPidVec{};
    auto workerJob = [&config, &checker, &ipcMemNameWrong]() {
        ThreadManager threadManager(config, false, LogLv::INFO, "/tmp/test.log");
        IPCResponse response;
        IPCMemRecord record3;
        record3.type = IPCOperationType::MAP_INFO;
        record3.mapInfo.addr = randAddr;
        strcpy_s(record3.mapInfo.name, sizeof(record3.mapInfo.name), ipcMemNameWrong);
        HandleIpcMemRecord(checker, record3, threadManager, RespFunc);
        auto responseStr = ResponseHandler::response;
        ParseIPCResponse(responseStr, response);
        ASSERT_EQ(response.type, record3.type);
        ASSERT_EQ(response.status, ResponseStatus::FAIL);
    };
    ForkJobList jobList{workerJob};
    int pid = DispatchForkJobs(childPidVec, jobList);
    if (pid != 0) {
        int status;
        for (auto pid : childPidVec) {
            waitpid(pid, &status, 0);
        }

        IPCMemRecord record2;
        record2.type = IPCOperationType::DESTROY_INFO;
        strcpy_s(record2.destroyInfo.name, sizeof(record2.destroyInfo.name), ipcMemNameWrong);
HandleIpcMemRecord(checker, record2, threadManager, RespFunc);
responseStr = ResponseHandler::response;
        ParseIPCResponse(responseStr, response);
        ASSERT_EQ(response.type, record2.type);
        ASSERT_EQ(response.status, ResponseStatus::FAIL);

        strcpy_s(record2.destroyInfo.name, sizeof(record2.destroyInfo.name), ipcMemName);
        HandleIpcMemRecord(checker, record2, threadManager, RespFunc);
        responseStr = ResponseHandler::response;
        ParseIPCResponse(responseStr, response);
        ASSERT_EQ(response.type, record2.type);
        ASSERT_EQ(response.status, ResponseStatus::SUCCESS);
        ResetIPCMap();
    }
}

static void FakeConfigFile(const Path &projectPath)
{
    Path configPath = projectPath / Path("kernel_config.bin");
    ofstream outfile;
    outfile.open(configPath.ToString().c_str(), ios::out);
}

TEST_F(TestCommand, fake_valid_dump_path_expect_detect_finish_success)
{
    Config config {};
    Command command(config, false, LogLv::INFO, "/tmp/test.log");
    constexpr int loopNum = 3;
    for (size_t i = 0; i < loopNum; i++) {
        Path subDir{dumpPath_ + "/dumpData_0_" + to_string(i)};
        ASSERT_EQ(mkdir(subDir.ToString().c_str(), DIR_DEFAULT_MOD), 0);
        FakeConfigFile(subDir);
    }
    ASSERT_TRUE(DetectDumpProject(command, dumpPath_));
    ASSERT_FALSE(Path(dumpPath_).Exists());
}

TEST_F(TestCommand, fake_empty_config_dump_project_expect_detect_fail)
{
    Config config {};
    Command command(config, false, LogLv::INFO, "/tmp/test.log");
    Path subDir{dumpPath_ + "/dumpData_0_0"};
    ASSERT_EQ(mkdir(subDir.ToString().c_str(), DIR_DEFAULT_MOD), 0);
    ASSERT_FALSE(DetectDumpProject(command, dumpPath_));
    ASSERT_FALSE(Path(dumpPath_).Exists());
}

TEST_F(TestCommand, fake_invalid_number_in_dump_project_expect_detect_fail)
{
    Config config {};
    Command command(config, false, LogLv::INFO, "/tmp/test.log");
    Path subDir{dumpPath_ + "/dumpData_a_0"};
    ASSERT_EQ(mkdir(subDir.ToString().c_str(), DIR_DEFAULT_MOD), 0);
    FakeConfigFile(subDir);
    ASSERT_FALSE(DetectDumpProject(command, dumpPath_));
    ASSERT_FALSE(Path(dumpPath_).Exists());
}

TEST_F(TestCommand, fake_invalid_lower_line_in_dump_project_expect_detect_fail)
{
    Config config {};
    Command command(config, false, LogLv::INFO, "/tmp/test.log");
    Path subDir{dumpPath_ + "/dump_Data_1_0"};
    ASSERT_EQ(mkdir(subDir.ToString().c_str(), DIR_DEFAULT_MOD), 0);
    FakeConfigFile(subDir);
    ASSERT_FALSE(DetectDumpProject(command, dumpPath_));
    ASSERT_FALSE(Path(dumpPath_).Exists());
}

TEST_F(TestCommand, test_empty_dump_project_generated_expect_finish_ok)
{
    Config config {};
    Command command(config, false, LogLv::INFO, "/tmp/test.log");
    ASSERT_FALSE(DetectDumpProject(command, dumpPath_));
    ASSERT_FALSE(Path(dumpPath_).Exists());
}
