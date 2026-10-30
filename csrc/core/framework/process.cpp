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


#include "process.h"

#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <linux/limits.h>

#include "record_defs.h"
#include "utility/log.h"
#include "utility/path.h"
#include "utility/serializer.h"
#include "utility/ustring.h"

namespace {
using namespace Sanitizer;
using std::string;

void SetPreloadEnv(Config const &config)
{
    string hookLibDir = "../lib64/";
    char const *preloadPath = getenv("LD_PRELOAD_PATH");
    if (preloadPath != nullptr && !string(preloadPath).empty()) {
        hookLibDir = preloadPath;
    }

    std::vector<string> hookLibNames;
    if (config.checkDeviceHeap) {
        hookLibNames = {"libascend_hal_hook.so"};
    } else if (config.checkCannHeap) {
        hookLibNames = {"libascend_acl_hook.so"};
    } else {
        hookLibNames = {"libmssanitizer_injection.so"};
    }

    for (string &hookLib : hookLibNames) {
        Path hookLibPath = (Path(hookLibDir) / Path(hookLib)).Resolved();
        if (hookLibPath.Exists()) {
            hookLib = hookLibPath.ToString();
            SAN_INFO_LOG("Use preload lib [%s]", Utility::ReplaceInvalidChar(hookLib).c_str());
        } else {
            SAN_WARN_LOG("No such preload lib [%s]", Utility::ReplaceInvalidChar(hookLibPath.ToString()).c_str());
        }
    }

    // 按照桩使能模式配置LD_PRELOAD环境变量
    string preloadEnv = Utility::Join(hookLibNames.cbegin(), hookLibNames.cend(), ":");
    const string envName = "LD_PRELOAD";
    auto prevLdPreEnv = getenv(envName.c_str());
    if (prevLdPreEnv && !string(prevLdPreEnv).empty()) {
        preloadEnv += ":" + string(prevLdPreEnv);
    }
    setenv(envName.c_str(), preloadEnv.c_str(), 1);
}
}

namespace Sanitizer {

ExecCmd::ExecCmd(std::vector<string> const &args)
    : path_{}, argc_{0}, args_{args}
{
    if (args_.empty()) {
        return;
    }

    /// filename to absolute path
    char *absPath = realpath(args[0].c_str(), nullptr);
    if (absPath) {
        path_ = string(absPath);
        free(absPath); // NOLINT(cppcoreguidelines-owning-memory)
    }

    argc_ = static_cast<int>(args.size());
    for (auto &arg : args_) {
        argv_.push_back(const_cast<char *>(arg.data())); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }
    argv_.push_back(nullptr);
}

string const &ExecCmd::ExecPath(void) const
{
    return path_;
}

char *const *ExecCmd::ExecArgv(void) const
{
    return argv_.data();
}

void Process::Launch(const ExecCmd &cmd)
{
    PreProcess(config_);

    ::pid_t pid = ::fork();
    if (pid == -1) {
        SAN_ERROR_LOG("Fork failed.");
        // 拉起失败，上层据此返回错误码
        programStatus_.launchError = true;
        return;
    } else if (pid == 0) {
        // child process
        SetPreloadEnv(config_);
        DoLaunch(cmd);
    } else {
        PostProcess(pid, cmd);
    }
}

void Process::RegisterMsgTrap(const ANALYSIS_FUNC& analysisFunc, const std::string &socketPath)
{
    // 此类仅需要完成进程间通信(建议调用其他类完成)，然后完成单次数据搬运
    if (server_ == nullptr) {
        server_ = std::make_shared<CommunicationServer>(socketPath);
        server_->RegisterMsgHandler(analysisFunc);
        server_->StartListen();
    }
}

void Process::PreProcess(Config const &config)
{
    // 依据不同的mode，加载不同的桩函数 （建议另起一个类专门处理）
    // 依据不同的mode，使能内部不同的检测功能 (建议另起一个类专门处理)
    if (server_) {
        server_->SetClientConnectHook([this, config](ClientId clientId) {
            this->server_->Write(clientId, Serialize<Config>(config));
        });
    }
}

void Process::PostProcess(pid_t child, ExecCmd const &cmd)
{
    // 可能存在后处理，比如，完成最后一次数据搬运的确认等
    int32_t status = 0;
    if (WaitChild(child, status)) {
        uint32_t ustatus = static_cast<uint32_t>(status);
        if (WIFEXITED(ustatus)) {
            programStatus_.exitedNormally = true;
            programStatus_.exitCode = static_cast<int32_t>(WEXITSTATUS(ustatus));
            if (programStatus_.exitCode != EXIT_SUCCESS) {
                std::cout << "[mssanitizer] user program " << Utility::ReplaceInvalidChar(cmd.ExecPath())
                          << " exited abnormally" << std::endl;
            } else {
                SAN_INFO_LOG("User program exited normally.");
            }
        } else if (WIFSIGNALED(ustatus)) {
            programStatus_.termSignal = static_cast<int32_t>(WTERMSIG(ustatus));
            std::cout << "[mssanitizer] user program " << Utility::ReplaceInvalidChar(cmd.ExecPath())
                      << " exited by signal(" << programStatus_.termSignal << ")" << std::endl;
        }
    } else {
        // 拿不到子进程状态，按拉起失败处理
        SAN_ERROR_LOG("waitpid failed: %s", std::strerror(errno));
        programStatus_.launchError = true;
    }

    // 子进程退出后，认为数据搬运已结束
    if (server_) {
        server_->Close();
        SAN_INFO_LOG("Server is closed.");
    }
}

bool WaitChild(pid_t child, int32_t &status) {
    pid_t ret = 0;
    do {
        ret = ::waitpid(child, &status, 0);
    } while (ret == -1 && errno == EINTR);
    return ret == child;
}

void Process::DoLaunch(const ExecCmd &cmd) const
{
    // pass all env-vars from global variable "environ"
    execve(cmd.ExecPath().c_str(), cmd.ExecArgv(), environ);
    _exit(EXIT_FAILURE);
}

std::string Process::CreateSockPath() const
{
    char buf[MAX_TIMESTAMP_LENGTH];
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    size_t len = std::strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", std::localtime(&time));
    // 通过环境变量向用户进程传递主进程 PID，用于绑定到正确的 socket 文件
    std::string socketPath = "/tmp/msop_connect." + std::string{buf, len} + "." + std::to_string(mainPid_) + ".sock";
    setenv("MSOP_SOCKET_PATH", socketPath.c_str(), 1);
    return socketPath;
}


} // namespace Sanitizer
