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


#include "cmd.h"

#include <cstdio>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#include "log.h"

namespace {

std::vector<char *> ToRawCArgv(std::vector<std::string> const &argv)
{
    std::vector<char *> rawArgv;
    for (auto const &arg: argv) {
        rawArgv.emplace_back(const_cast<char *>(arg.data())); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }
    rawArgv.emplace_back(nullptr);
    return rawArgv;
}

bool ProcessSubprocessRet(int status)
{
    unsigned int ustatus = static_cast<unsigned int>(status);  // clean code: 只能对无符号整数进行位运算
    if (WIFSIGNALED(ustatus)) {
        // 子进程收到信号被异常终止，返回信号类型是否为实时信号
        int sig = static_cast<int>(WTERMSIG(ustatus));
        if (sig >= SIGRTMIN && sig <= SIGRTMAX) {
            return true;
        } else {
            SAN_ERROR_LOG("Subprocess is terminated abnormally, WTERMSIG = %d", WTERMSIG(ustatus));
            return false;
        }
    } else if (WIFEXITED(ustatus)) {
        // 进程正常结束并且进程结束代码为 0 时认为命令执行成功
        if (WEXITSTATUS(ustatus) == 0) {
            return true;
        } else {
            SAN_ERROR_LOG("Subprocess exits abnormally, WEXITSTATUS = %d", WEXITSTATUS(ustatus));
            return false;
        }
    }
    SAN_ERROR_LOG("PipeCall failed, subprocess status = %d", status);
    return false;
}

} // namspace [Dummy]

namespace Sanitizer {


bool PipeCall(std::vector<std::string> const &cmd, std::string &output)
{
    int pipeStdout[2];
    if (pipe(pipeStdout) != 0) {
        SAN_ERROR_LOG("Create pipe FAILED");
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        SAN_ERROR_LOG("Fork subprocess FAILED");
        return false;
    } else if (pid == 0) {
        dup2(pipeStdout[1], STDOUT_FILENO);
        close(pipeStdout[0]);
        close(pipeStdout[1]);

        execvp(cmd[0].c_str(), ToRawCArgv(cmd).data());
        _exit(EXIT_FAILURE);
    } else {
        close(pipeStdout[1]);

        static constexpr std::size_t bufLen = 256UL;
        char buf[bufLen] = {'\0'};
        ssize_t nBytes = 0L;
        for (; (nBytes = read(pipeStdout[0], buf, bufLen)) > 0L;) {
            output.append(buf, static_cast<std::size_t>(nBytes));
            if (output.size() >= MAX_PIPE_OUTPUT_SIZE) {
                SAN_ERROR_LOG("Return of pipe call exceeded the limit (%lu).", MAX_PIPE_OUTPUT_SIZE);
                break;
            }
        }
        close(pipeStdout[0]);

        int status;
        waitpid(pid, &status, 0);
        return ProcessSubprocessRet(status);
    }
    return true;
}

} // namespace Sanitizer
