/* -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
#include <cstring>
#include <string>
#include <thread>
#include <chrono>

#include "communication.h"
#include "config.h"
#include "protocol.h"
#include "utility/serializer.h"

#define private public
#include "hook_report.h"
#undef private
#include "record_defs.h"

namespace Sanitizer {
MemOpRecord CreateRecord(MemOpType type, const ReportAddrInfo &addrInfo);
}

using namespace Sanitizer;

namespace {
// 启动 CommunicationServer，客户端连接时发送 Config；收集客户端写入的消息供断言。
class HookReportServer {
public:
    explicit HookReportServer(const std::string &socketPath) : server_(socketPath) {}

    void Start()
    {
        server_.RegisterMsgHandler([this](std::string msg, CommunicationServer::MsgResponseFunc &rsp) {
            std::lock_guard<std::mutex> lock(mtx_);
            receivedMsgs_.push_back(msg);
            cv_.notify_all();
        });
        server_.SetClientConnectHook([this](CommunicationServer::ClientId id) {
            Config config{};
            config.cacheSize = 100;
            config.checkBlockId = -1;
            config.defaultCheck = true;
            config.memCheck = true;
            std::string configMsg(reinterpret_cast<char const *>(&config), sizeof(Config));
            server_.Write(id, configMsg);
        });
        server_.StartListen();
    }

    void Stop() { server_.Close(); }

    bool WaitForMessage(std::string &outMsg)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (!cv_.wait_for(lock, std::chrono::seconds(5), [this] { return !receivedMsgs_.empty(); })) {
            return false;
        }
        outMsg = receivedMsgs_.front();
        receivedMsgs_.erase(receivedMsgs_.begin());
        return true;
    }

private:
    CommunicationServer server_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::vector<std::string> receivedMsgs_;
};

class HookReportTest : public testing::Test {
protected:
    void SetUp() override
    {
        socketPath_ = "/tmp/hook_report_ut_" + std::to_string(getpid()) + ".sock";
        unlink(socketPath_.c_str());
        server_ = std::make_unique<HookReportServer>(socketPath_);
        server_->Start();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        setenv("MSOP_SOCKET_PATH", socketPath_.c_str(), 1);
        auto &hook = HookReport::Instance();
        hook.client_.reset();
        hook.determined_ = false;
        hook.memInfoSrc_ = MemInfoSrc::RT;
        hook.deviceType_ = DeviceType::INVALID;
        hook.blockDim_ = 0U;
        hook.recordBuffer_.clear();
        hook.kernelInfo_ = KernelInfo{};
        hook.checkParms_ = CheckParmsInfo{};
        hook.serialNo_ = 0;
        ASSERT_TRUE(hook.InitClient());
        ASSERT_NE(hook.client_, nullptr);
    }

    void TearDown() override
    {
        // 先关闭 client 连接，避免 server 的 Listen 线程阻塞在 recvmsg 上导致 Close 死锁
        auto &hook = HookReport::Instance();
        hook.client_.reset();
        server_->Stop();
        unsetenv("MSOP_SOCKET_PATH");
    }

    std::string socketPath_;
    std::unique_ptr<HookReportServer> server_;
};
} // namespace

// 覆盖 CreateRecord：null/valid/long/invalid_chars 文件名 + type/addr 字段
TEST(HookReport, CreateRecord_AllPaths_expect_correct) {
    {
        ReportAddrInfo addrInfo(0x1000, 0x100, 10, nullptr, MemInfoSrc::RT);
        MemOpRecord record = CreateRecord(MemOpType::LOAD, addrInfo);
        std::string fileName(record.fileName, sizeof(record.fileName));
        ASSERT_NE(fileName.find("<unknown>"), std::string::npos);
    }
    {
        ReportAddrInfo addrInfo(0x1000, 0x100, 10, "test_file.cpp", MemInfoSrc::RT);
        MemOpRecord record = CreateRecord(MemOpType::LOAD, addrInfo);
        std::string fileName(record.fileName, sizeof(record.fileName));
        ASSERT_NE(fileName.find("test_file.cpp"), std::string::npos);
    }
    {
        std::string longName(1024, 'a');
        ReportAddrInfo addrInfo(0x1000, 0x100, 5, longName.c_str(), MemInfoSrc::BYPASS);
        MemOpRecord record = CreateRecord(MemOpType::STORE, addrInfo);
        std::string fileName(record.fileName, sizeof(record.fileName));
        ASSERT_EQ(fileName.find_first_of('\0'), sizeof(record.fileName) - 1);
    }
    {
        ReportAddrInfo addrInfo(0x1000, 0x100, 20, "test\nfile\rname", MemInfoSrc::RT);
        MemOpRecord record = CreateRecord(MemOpType::MALLOC, addrInfo);
        std::string fileName(record.fileName, sizeof(record.fileName));
        ASSERT_EQ(fileName.find('\n'), std::string::npos);
        ASSERT_EQ(fileName.find('\r'), std::string::npos);
    }
    {
        ReportAddrInfo addrInfo(0x2000, 0x200, 30, "demo.cpp", MemInfoSrc::BYPASS);
        MemOpRecord record = CreateRecord(MemOpType::FREE, addrInfo);
        ASSERT_EQ(record.type, MemOpType::FREE);
        ASSERT_EQ(record.dstAddr, 0x2000);
        ASSERT_EQ(record.memSize, 0x200);
        ASSERT_EQ(record.infoSrc, MemInfoSrc::BYPASS);
    }
}

TEST_F(HookReportTest, InitClient_expect_success_and_check_parms_correct)
{
    auto &hook = HookReport::Instance();
    ASSERT_NE(hook.client_, nullptr);
    auto parms = hook.GetCheckParms();
    EXPECT_EQ(parms.cacheSize, 100U);
    EXPECT_TRUE(parms.defaultcheck);
    EXPECT_TRUE(parms.memcheck);
}

// 三个 Report* 接口走同一套成功上报流程，循环覆盖
TEST_F(HookReportTest, ReportDeviceTypeKernelInfoMemOp_expect_success)
{
    auto &hook = HookReport::Instance();
    ReportAddrInfo addrInfo(0x1000, 0x100, 10, "test.cpp", MemInfoSrc::RT);
    MemOpRecord record = CreateRecord(MemOpType::MALLOC, addrInfo);

    hook.SetBlockDim(4);
    hook.SetKernelType(KernelType::AIVEC);

    EXPECT_TRUE(hook.ReportDeviceType(DeviceType::ASCEND_910B1));
    EXPECT_EQ(hook.GetDeviceType(), DeviceType::ASCEND_910B1);
    EXPECT_TRUE(hook.ReportKernelInfo());
    EXPECT_EQ(hook.GetKernelInfo().kernelType, KernelType::AIVEC);
    EXPECT_TRUE(hook.ReportMemOp(record));

    for (int i = 0; i < 3; ++i) {
        std::string msg;
        ASSERT_TRUE(server_->WaitForMessage(msg));
        EXPECT_FALSE(msg.empty());
    }
}

TEST(HookReport, ReportWithoutClient_expect_fail)
{
    unsetenv("MSOP_SOCKET_PATH");
    auto &hook = HookReport::Instance();
    ReportAddrInfo addrInfo(0x1000, 0x100, 10, "test.cpp", MemInfoSrc::RT);
    MemOpRecord record = CreateRecord(MemOpType::MALLOC, addrInfo);
    SanitizerRecord sr{};
    sr.version = RecordVersion::KERNEL_RECORD;
    // 循环覆盖 InitClient / ReportDeviceType / ReportMemOp / Report 四条失败路径
    for (int i = 0; i < 4; ++i) {
        hook.client_.reset();
        switch (i) {
            case 0: EXPECT_FALSE(hook.InitClient()); break;
            case 1: EXPECT_FALSE(hook.ReportDeviceType(DeviceType::ASCEND_910B1)); break;
            case 2: EXPECT_FALSE(hook.ReportMemOp(record)); break;
            default: EXPECT_FALSE(hook.Report(sr)); break;
        }
    }
}

TEST_F(HookReportTest, Report_kernel_and_memory_record_expect_success)
{
    auto &hook = HookReport::Instance();
    // kernel record
    SanitizerRecord kr{};
    kr.version = RecordVersion::KERNEL_RECORD;
    kr.payload.kernelRecord.recordType = RecordType::SET_FLAG;
    EXPECT_TRUE(hook.Report(kr));
    std::string msg;
    ASSERT_TRUE(server_->WaitForMessage(msg));
    // memory record
    SanitizerRecord mr{};
    mr.version = RecordVersion::MEMORY_RECORD;
    mr.payload.memoryRecord.type = MemOpType::MALLOC;
    EXPECT_TRUE(hook.Report(mr));
    ASSERT_TRUE(server_->WaitForMessage(msg));
}

// Report* 接口走同一套上报流程，循环覆盖
TEST_F(HookReportTest, ReportMemOps_with_addrInfo_expect_success)
{
    auto &hook = HookReport::Instance();
    ReportAddrInfo addrInfo(0x1000, 0x100, 10, "test.cpp", MemInfoSrc::RT);
    ReportAddrInfo addrInfoFree(0x1000, MemInfoSrc::RT);
    for (int i = 0; i < 3; ++i) {
        std::string msg;
        switch (i) {
            case 0: EXPECT_TRUE(hook.ReportMalloc(addrInfo, 1)); break;
            case 1: EXPECT_TRUE(hook.ReportFree(addrInfoFree)); break;
            default: EXPECT_TRUE(hook.ReportMemset(addrInfo)); break;
        }
        ASSERT_TRUE(server_->WaitForMessage(msg));
    }
}

// 覆盖未确定来源时的缓存+上报，以及已确定时的 same_src 直报 / diff_src 跳过
TEST_F(HookReportTest, ReportOps_determine_src_expect_correct)
{
    auto &hook = HookReport::Instance();
    for (int i = 0; i < 2; ++i) {
        hook.determined_ = false;
        hook.memInfoSrc_ = MemInfoSrc::RT;
        hook.recordBuffer_.clear();
        if (i == 0) {
            EXPECT_TRUE(hook.ReportMalloc(0x1000, 0x100, MemInfoSrc::RT));
        } else {
            EXPECT_TRUE(hook.ReportFree(0x1000, MemInfoSrc::RT));
        }
        EXPECT_EQ(hook.recordBuffer_.size(), 1U);
        hook.DetermineMemInfoSrc();
        EXPECT_TRUE(hook.recordBuffer_.empty());
        std::string msg;
        ASSERT_TRUE(server_->WaitForMessage(msg));
    }
    hook.determined_ = true;
    hook.memInfoSrc_ = MemInfoSrc::RT;
    EXPECT_TRUE(hook.ReportMalloc(0x1000, 0x100, MemInfoSrc::RT));
    std::string msg;
    ASSERT_TRUE(server_->WaitForMessage(msg));
    EXPECT_TRUE(hook.ReportMalloc(0x1000, 0x100, MemInfoSrc::BYPASS));
}
