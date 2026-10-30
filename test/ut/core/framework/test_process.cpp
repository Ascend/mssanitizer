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
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "process.h"

using namespace Sanitizer;

TEST(Process, process_launch_ls_expect_success)
{
    std::vector<std::string> argv = {"/bin/ls"};
    ExecCmd cmd(argv);
    Config config {
        .defaultCheck = true,
        .memCheck = true,
    };
    Process process(config);
    process.Launch(cmd);
}

TEST(Process, process_launch_true_expect_normal_exit_with_zero) {
    std::vector<std::string> argv = {"/bin/true"};
    ExecCmd cmd(argv);
    Config config{
        .defaultCheck = true,
        .memCheck = true,
    };
    Process process(config);
    process.Launch(cmd);

    ProgramStatus const &status = process.GetProgramStatus();
    ASSERT_FALSE(status.launchError);
    ASSERT_TRUE(status.exitedNormally);
    ASSERT_EQ(status.exitCode, 0);
}

TEST(Process, process_launch_false_expect_normal_exit_with_nonzero) {
    std::vector<std::string> argv = {"/bin/false"};
    ExecCmd cmd(argv);
    Config config{
        .defaultCheck = true,
        .memCheck = true,
    };
    Process process(config);
    process.Launch(cmd);

    ProgramStatus const &status = process.GetProgramStatus();
    ASSERT_FALSE(status.launchError);
    ASSERT_TRUE(status.exitedNormally);
    ASSERT_EQ(status.exitCode, 1);
}

TEST(Process, process_launch_killed_by_signal_expect_term_signal) {
    std::vector<std::string> argv = {"/bin/sh", "-c", "kill -KILL $$"};
    ExecCmd cmd(argv);
    Config config{
        .defaultCheck = true,
        .memCheck = true,
    };
    Process process(config);
    process.Launch(cmd);

    ProgramStatus const &status = process.GetProgramStatus();
    ASSERT_FALSE(status.launchError);
    ASSERT_FALSE(status.exitedNormally);
    ASSERT_EQ(status.termSignal, 9);
}

TEST(Process, wait_child_valid_pid_expect_status) {
    pid_t child = ::fork();
    ASSERT_NE(child, -1);
    if (child == 0) {
        ::_exit(7);
    }

    int32_t status = 0;
    ASSERT_TRUE(WaitChild(child, status));
    uint32_t ustatus = static_cast<uint32_t>(status);
    ASSERT_TRUE(WIFEXITED(ustatus));
    ASSERT_EQ(WEXITSTATUS(ustatus), 7);
}

TEST(Process, wait_child_invalid_pid_expect_false) {
    int32_t status = 0;
    ASSERT_FALSE(WaitChild(static_cast<pid_t>(0x7FFFFFFF), status));
}
