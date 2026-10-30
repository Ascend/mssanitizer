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
#include <cstdint>
#include <thread>
#include <vector>

#include "error_exitcode.h"

using namespace Sanitizer;

namespace {
constexpr int32_t TEST_ERROR_EXIT_CODE = 10;

ProgramStatus MakeStatus(bool launchError, bool exitedNormally, int32_t exitCode, int32_t termSignal) {
    ProgramStatus status{};
    status.launchError = launchError;
    status.exitedNormally = exitedNormally;
    status.exitCode = exitCode;
    status.termSignal = termSignal;
    return status;
}
}

class TestErrorExitCode : public testing::Test {
protected:
    void SetUp() override { ErrorDetectionSummary::Instance().Reset(); }

    void TearDown() override { ErrorDetectionSummary::Instance().Reset(); }
};

TEST_F(TestErrorExitCode, initial_state_expect_no_detection) {
    ASSERT_FALSE(ErrorDetectionSummary::Instance().HasErrorDetection());
}

TEST_F(TestErrorExitCode, mark_detection_expect_detected) {
    ErrorDetectionSummary::Instance().MarkErrorDetected();
    ErrorDetectionSummary::Instance().MarkErrorDetected();
    ErrorDetectionSummary::Instance().MarkErrorDetected();

    ASSERT_TRUE(ErrorDetectionSummary::Instance().HasErrorDetection());
}

TEST_F(TestErrorExitCode, concurrent_mark_expect_detected) {
    constexpr uint32_t threadNum = 8U;
    constexpr uint32_t loopNum = 10000U;
    std::vector<std::thread> workers;
    workers.reserve(threadNum);
    for (uint32_t i = 0; i < threadNum; ++i) {
        workers.emplace_back([]() {
            for (uint32_t j = 0; j < loopNum; ++j) {
                ErrorDetectionSummary::Instance().MarkErrorDetected();
            }
        });
    }
    for (auto &worker : workers) {
        worker.join();
    }

    ASSERT_TRUE(ErrorDetectionSummary::Instance().HasErrorDetection());
}

TEST_F(TestErrorExitCode, reset_expect_no_detection) {
    ErrorDetectionSummary::Instance().MarkErrorDetected();
    ErrorDetectionSummary::Instance().Reset();

    ASSERT_FALSE(ErrorDetectionSummary::Instance().HasErrorDetection());
}

TEST_F(TestErrorExitCode, error_detection_level_expect_only_error_level) {
    ASSERT_FALSE(IsErrorDetectionLevel(LogLv::DEBUG));
    ASSERT_FALSE(IsErrorDetectionLevel(LogLv::INFO));
    ASSERT_FALSE(IsErrorDetectionLevel(LogLv::WARN));
    ASSERT_TRUE(IsErrorDetectionLevel(LogLv::ERROR));
}

TEST_F(TestErrorExitCode, resolve_exit_code_expect_zero_when_option_not_set) {
    // 配置为 0：即使检出了 error 类告警也不改变退出码
    ErrorDetectionSummary::Instance().MarkErrorDetected();
    ASSERT_EQ(ResolveExitCode(0, MakeStatus(false, true, 0, 0)), 0);
}

TEST_F(TestErrorExitCode, resolve_exit_code_expect_option_value_when_error_detected) {
    ErrorDetectionSummary::Instance().MarkErrorDetected();
    ASSERT_EQ(ResolveExitCode(TEST_ERROR_EXIT_CODE, MakeStatus(false, true, 0, 0)), TEST_ERROR_EXIT_CODE);
}

TEST_F(TestErrorExitCode, resolve_exit_code_expect_zero_when_no_error_detected) {
    ASSERT_EQ(ResolveExitCode(TEST_ERROR_EXIT_CODE, MakeStatus(false, true, 0, 0)), 0);
}

TEST_F(TestErrorExitCode, resolve_exit_code_expect_program_code_when_program_returns_nonzero) {
    ErrorDetectionSummary::Instance().MarkErrorDetected();
    ASSERT_EQ(ResolveExitCode(TEST_ERROR_EXIT_CODE, MakeStatus(false, true, 1, 0)), 1);
    ASSERT_EQ(ResolveExitCode(0, MakeStatus(false, true, 3, 0)), 3);
}

TEST_F(TestErrorExitCode, resolve_exit_code_expect_signal_code_when_program_killed) {
    ErrorDetectionSummary::Instance().MarkErrorDetected();
    ASSERT_EQ(ResolveExitCode(TEST_ERROR_EXIT_CODE, MakeStatus(false, false, 0, 9)), 137);
    ASSERT_EQ(ResolveExitCode(0, MakeStatus(false, false, 0, 11)), 139);
}

TEST_F(TestErrorExitCode, resolve_exit_code_expect_launch_error_when_launch_failed) {
    ErrorDetectionSummary::Instance().MarkErrorDetected();
    ASSERT_EQ(ResolveExitCode(TEST_ERROR_EXIT_CODE, MakeStatus(true, false, 0, 0)), SANITIZER_LAUNCH_ERROR_EXIT_CODE);
    ASSERT_EQ(ResolveExitCode(0, MakeStatus(true, true, 0, 0)), SANITIZER_LAUNCH_ERROR_EXIT_CODE);
}

TEST_F(TestErrorExitCode, reserved_exit_codes_expect_expected_values) {
    ASSERT_EQ(SANITIZER_INVALID_ARGUMENT_EXIT_CODE, 255);
    ASSERT_EQ(SANITIZER_LAUNCH_ERROR_EXIT_CODE, 1);
    ASSERT_EQ(SIGNAL_EXIT_BASE, 128);
}
