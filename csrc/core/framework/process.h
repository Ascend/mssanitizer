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


#ifndef CORE_FRAMEWORK_PROCESS_H
#define CORE_FRAMEWORK_PROCESS_H

#include <functional>
#include <memory>
#include "config.h"
#include "communication.h"

namespace Sanitizer {
struct ExecCmd {
    explicit ExecCmd(std::vector<std::string> const &args);
    std::string const &ExecPath(void) const;
    char *const *ExecArgv(void) const;

private:
    std::string path_;
    int argc_;
    std::vector<char*> argv_;
    std::vector<std::string> args_;
};

// 被检测程序的退出状态
struct ProgramStatus {
    bool launchError{false}; // 拉起失败（fork 失败等）
    bool exitedNormally{false}; // WIFEXITED；false 表示被信号终止
    int32_t exitCode{0}; // WEXITSTATUS，0~255
    int32_t termSignal{0}; // WTERMSIG
};

// 等待子进程结束，EINTR 时重试；返回 false 表示拿不到有效状态
bool WaitChild(pid_t child, int32_t &status);

// Process类将整个流程伪装成一个普通的进程拉起过程
// 其中：
// Launch函数将被检测程序拉起
// RegisterMsgTrap函数会将收集到的信息返回给关注的目标
class Process {
public:
    using MsgRspFunc = std::function<void(const std::string&)>;
    using ANALYSIS_FUNC = std::function<void(const std::string&, MsgRspFunc&)>;
    using ClientId = std::size_t;
    explicit Process(Config const &config) : config_{config}, mainPid_(getpid()) {}
    void Launch(const ExecCmd& cmd);
    // 当被测程序处有信息返回时，调用此处的注册函数
    // 说明：此处的信息可能是非完整的数据，处理方需要记录历史信息
    void RegisterMsgTrap(const ANALYSIS_FUNC& analysisFunc, const std::string &socketPath);
    std::string CreateSockPath() const;
    ProgramStatus const &GetProgramStatus() const { return programStatus_; }

private:
    void PreProcess(Config const &config);
    void PostProcess(pid_t child, ExecCmd const &cmd);
    void DoLaunch(const ExecCmd& cmd) const;

private:
    Config config_;
    pid_t mainPid_;
    std::shared_ptr<CommunicationServer> server_;
    ProgramStatus programStatus_{};
}; // class Process

} // namespace Framwork

#endif // CORE_FRAMEWORK_PROCESS_H
