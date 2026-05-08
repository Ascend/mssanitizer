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
#include <gtest/internal/gtest-port.h>
#include <any>
#include <mutex>
#include <string>
#include <vector>

#include "temp_file_guard.h"
#include "core/framework/utility/path.h"

/// 暴露私有成员用于单元测试
#define private public
#include "core/cli_parser.h"
#undef private

using namespace Sanitizer;

namespace SanitizerTest {

void InvalidParamCheckHelpInfo(const char* invalidInput)
{
    std::vector<const char*> argv = {
        "asan",
        invalidInput
    };

    optind = 1;
    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Interpretor(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    ASSERT_NE(capture.find("Usage"), std::string::npos);
}

TEST(CliParser, pass_help_parameter_expect_get_print_help_info)
{
    std::vector<const char*> argv = {
        "asan",
        "--help"
    };

    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.printHelpInfo);
}

TEST(CliParser, pass_help_parameter_expect_show_help_info)
{
    std::vector<const char*> argv = {
        "asan",
        "--help"
    };

    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Interpretor(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    ASSERT_NE(capture.find("Usage"), std::string::npos);
}

TEST(CliParser, pass_version_parameter_expect_get_print_version_info)
{
    std::vector<const char*> argv = {
        "asan",
        "--version"
    };

    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.printVersionInfo);
}

TEST(CliParser, pass_version_parameter_expect_show_version_info)
{
    std::vector<const char*> argv = {
        "asan",
        "--version"
    };

    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    cliParser.Interpretor(argv.size(), const_cast<char**>(argv.data()));
}

TEST(CliParser, set_loglv_warn_parameter_expect_get_loglv_warn)
{
    std::vector<const char*> argv = {
        "asan",
        "--log-level=warn"
    };

    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_EQ(cmd.logLv, LogLv::WARN);
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_TRUE(cmd.config.memCheck);
}

TEST(CliParser, invalid_log_level_parameter_expect_show_help_info)
{
    InvalidParamCheckHelpInfo("-log-level=1");
    InvalidParamCheckHelpInfo("-log-level=INFO");
    InvalidParamCheckHelpInfo("-log-level=\n");
}

TEST(CliParser, set_log_file_path_expect_get_same_log_file_path)
{
    std::string logfile = "./test.log";
    std::string logArg = std::string("--log-file=") + logfile;
    std::string parentPath = Path(logfile).Absolute().Parent().ToString();
    chmod(parentPath.c_str(), 0750);
    remove(logfile.c_str());

    std::vector<const char*> argv = {
        "asan",
        logArg.c_str()
    };

    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    char resolvedPath[MAX_FILE_PATH_LEN]{0};
    realpath(logfile.c_str(), resolvedPath);
    ASSERT_EQ(cmd.logFile, resolvedPath);
}

TEST(CliParser, pass_empty_prog_name_expect_get_empty_bin_cmd)
{
    std::vector<const char*> argv = {
        "asan"
    };

    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.cmd.empty());
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_TRUE(cmd.config.memCheck);
}

TEST(CliParser, enable_mem_check_expect_get_mem_check_mode_and_no_leak_check)
{
    std::vector<const char*> argv = {
        "asan",
        "--tool=memcheck"
    };

    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_TRUE(cmd.config.memCheck);
    ASSERT_FALSE(cmd.config.leakCheck);
}

TEST(cliParser, pass_tool_initcheck_parameter_expect_init_check_mode_enabled)
{
    std::vector<const char*> argv = {
        "asan",
        "--tool=initcheck"
    };
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.config.initCheck);
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_FALSE(cmd.config.memCheck);
}

TEST(CliParser, enable_race_check_expect_get_race_check_mode_and_no_leak_check)
{
    std::vector<const char*> argv = {
        "asan",
        "--tool=racecheck"
    };

    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.config.raceCheck);
    ASSERT_FALSE(cmd.config.defaultCheck);
    ASSERT_FALSE(cmd.config.memCheck);
    ASSERT_FALSE(cmd.config.leakCheck);
}

TEST(cliParser, pass_tool_initcheck_and_memcheck_expect_only_two_mode_enabled)
{
    std::vector<const char*> argv = {
        "asan",
        "--tool=initcheck",
        "--tool=memcheck"
    };
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.config.initCheck);
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_TRUE(cmd.config.memCheck);
}

TEST(cliParser, pass_tool_racecheck_and_memcheck_expect_only_two_mode_enabled)
{
    std::vector<const char*> argv = {
        "asan",
        "--tool=racecheck",
        "--tool=memcheck"
    };
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_FALSE(cmd.config.initCheck);
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_TRUE(cmd.config.memCheck);
    ASSERT_TRUE(cmd.config.raceCheck);
}

TEST(cliParser, pass_tool_racecheck_and_initcheck_expect_only_two_mode_enabled)
{
    std::vector<const char*> argv = {
        "asan",
        "--tool=racecheck",
        "--tool=initcheck"
    };
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.config.initCheck);
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_FALSE(cmd.config.memCheck);
    ASSERT_TRUE(cmd.config.raceCheck);
}

TEST(cliParser, pass_three_tools_expect_all_modes_enabled)
{
    std::vector<const char*> argv = {
        "asan",
        "--tool=racecheck",
        "--tool=initcheck",
        "--tool=memcheck"
    };
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.config.initCheck);
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_TRUE(cmd.config.memCheck);
    ASSERT_TRUE(cmd.config.raceCheck);
}

TEST(CliParser, invalid_tool_expect_show_help_info)
{
    InvalidParamCheckHelpInfo("--tool=a");
    InvalidParamCheckHelpInfo("--tool=1");
    InvalidParamCheckHelpInfo("--tool=\x1b[D");
}

TEST(CliParser, enable_mem_check_and_leak_check_expect_get_mem_check_and_leak_check_mode)
{
    std::vector<const char*> argv = {
        "asan",
        "--tool=memcheck",
        "--leak-check=yes"
    };

    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_TRUE(cmd.config.memCheck);
    ASSERT_TRUE(cmd.config.leakCheck);
    ASSERT_FALSE(cmd.config.initCheck);
    ASSERT_FALSE(cmd.config.raceCheck);
}

TEST(CliParser, disable_leak_check_expect_get_leak_check_mode_dsiabled)
{
    std::vector<const char*> argv = {
        "asan",
        "--leak-check=no"
    };

    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_TRUE(cmd.config.memCheck);
    ASSERT_FALSE(cmd.config.leakCheck);
    ASSERT_FALSE(cmd.config.initCheck);
    ASSERT_FALSE(cmd.config.raceCheck);
}

TEST(CliParser, invalid_leak_check_expect_show_help_info)
{
    InvalidParamCheckHelpInfo("--leak-check=n");
    InvalidParamCheckHelpInfo("--leak-check=1");
    InvalidParamCheckHelpInfo("--leak-check=\u007F");
}

TEST(cliParser, pass_invalid_tool_parameter_expect_print_help_info)
{
    std::vector<const char*> argv = {
        "asan",
        "--tool=unknown-tool"
    };
    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.printHelpInfo);
}

TEST(cliParser, pass_check_device_heap_parameter_expect_check_device_heap_mode_enabled)
{
    std::vector<const char*> argv = {
        "asan",
        "--check-device-heap=yes"
    };
    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_TRUE(cmd.config.memCheck);
    ASSERT_TRUE(cmd.config.checkDeviceHeap);
    ASSERT_FALSE(cmd.config.initCheck);
    ASSERT_FALSE(cmd.config.raceCheck);
}

TEST(cliParser, disable_check_device_heap_expect_check_device_heap_mode_disabled)
{
    std::vector<const char*> argv = {
        "asan",
        "--check-device-heap=no"
    };
    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_TRUE(cmd.config.memCheck);
    ASSERT_FALSE(cmd.config.checkDeviceHeap);
    ASSERT_FALSE(cmd.config.initCheck);
    ASSERT_FALSE(cmd.config.raceCheck);
}

TEST(CliParser, invalid_check_device_heap_expect_show_help_info)
{
    InvalidParamCheckHelpInfo("--check-device-heap=y");
    InvalidParamCheckHelpInfo("--check-device-heap=1");
    InvalidParamCheckHelpInfo("--check-device-heap=\u0008");
}

TEST(cliParser, pass_check_cann_heap_parameter_expect_check_cann_heap_mode_enabled)
{
    std::vector<const char*> argv = {
        "asan",
        "--check-cann-heap=yes"
    };
    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_TRUE(cmd.config.memCheck);
    ASSERT_TRUE(cmd.config.checkCannHeap);
    ASSERT_FALSE(cmd.config.initCheck);
    ASSERT_FALSE(cmd.config.raceCheck);
}

TEST(cliParser, disable_check_cann_heap_expect_check_cann_heap_mode_disabled)
{
    std::vector<const char*> argv = {
        "asan",
        "--check-cann-heap=no"
    };
    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_TRUE(cmd.config.memCheck);
    ASSERT_FALSE(cmd.config.checkCannHeap);
    ASSERT_FALSE(cmd.config.initCheck);
    ASSERT_FALSE(cmd.config.raceCheck);
}

TEST(CliParser, invalid_check_cann_heap_expect_show_help_info)
{
    InvalidParamCheckHelpInfo("--check_cann_heap=Y");
    InvalidParamCheckHelpInfo("--check_cann_heap=0");
    InvalidParamCheckHelpInfo("--check_cann_heap=\x08");
}

TEST(cliParser, pass_check_unused_memory_parameter_expect_check_unused_memory_mode_enabled)
{
    std::vector<const char*> argv = {
        "asan",
        "--check-unused-memory=yes"
    };
    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_TRUE(cmd.config.memCheck);
    ASSERT_TRUE(cmd.config.checkUnusedMemory);
    ASSERT_FALSE(cmd.config.initCheck);
    ASSERT_FALSE(cmd.config.raceCheck);
}

TEST(cliParser, disable_check_unused_memory_parameter_expect_check_unused_memory_mode_disabled)
{
    std::vector<const char*> argv = {
        "asan",
        "--check-unused-memory=no"
    };
    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_TRUE(cmd.config.memCheck);
    ASSERT_FALSE(cmd.config.checkUnusedMemory);
    ASSERT_FALSE(cmd.config.initCheck);
    ASSERT_FALSE(cmd.config.raceCheck);
}

TEST(CliParser, invalid_check_unused_memory_expect_show_help_info)
{
    InvalidParamCheckHelpInfo("--check-unused-memory=NO");
    InvalidParamCheckHelpInfo("--check-unused-memory=1");
    InvalidParamCheckHelpInfo("--check-unused-memory=\r");
}

TEST(CliParser, pass_test_as_prog_expect_get_bin_cmd_test)
{
    std::vector<const char*> argv = {
        "asan",
        "test"
    };

    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_FALSE(cmd.cmd.empty());
    ASSERT_EQ(cmd.cmd[0], "test");
    ASSERT_TRUE(cmd.config.defaultCheck);
    ASSERT_TRUE(cmd.config.memCheck);
    ASSERT_FALSE(cmd.config.initCheck);
    ASSERT_FALSE(cmd.config.raceCheck);
    ASSERT_FALSE(cmd.config.leakCheck);
    ASSERT_FALSE(cmd.config.checkDeviceHeap);
    ASSERT_FALSE(cmd.config.checkCannHeap);
}

TEST(CliParser, pass_empty_prog_expect_get_no_program_error)
{
    std::vector<const char*> argv = {
        "asan"
    };

    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Interpretor(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    ASSERT_NE(capture.find("ERROR: no program specified"), std::string::npos);
}

TEST(CliParser, pass_not_exist_prog_expect_not_found_error)
{
    std::vector<const char*> argv = {
        "asan",
        "not_exist"
    };

    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Interpretor(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    ASSERT_NE(capture.find("from PATH environment FAILED"), std::string::npos);
}

namespace {
std::string GetProgParseRslt(mode_t mode) {
    std::string tempFile = "./sanitizer_test";
    TempFileGuard guard(tempFile);
    ChangeMode(tempFile, mode);

    std::vector<const char*> argv = {
        "asan",
        tempFile.c_str()
    };

    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Interpretor(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    return capture;
}
}

TEST(CliParser, pass_prog_with_good_perms_expect_no_error)
{
    mode_t modeToTest = 0;
    std::string && capture = GetProgParseRslt(modeToTest);
    ASSERT_EQ(capture.find("write permission"), std::string::npos);
    modeToTest = 0104;
    capture = GetProgParseRslt(modeToTest);
    ASSERT_EQ(capture.find("write permission"), std::string::npos);
    modeToTest = 0755;
    capture = GetProgParseRslt(modeToTest);
    ASSERT_EQ(capture.find("write permission"), std::string::npos);
}

TEST(CliParser, pass_prog_of_soft_link_expect_no_error)
{
    std::string tempFile = "./sanitizer_test";
    TempFileGuard guard(tempFile);
    std::string tempLinkFile = "./san_soft_link";
    symlink(tempFile.c_str(), tempLinkFile.c_str());

    std::vector<const char*> argv = {
        "asan",
        tempLinkFile.c_str()
    };

    /// Reset getopt states
    optind = 1;
    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Interpretor(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    ASSERT_EQ(capture.find("soft link"), std::string::npos);
    remove(tempLinkFile.c_str());
}

TEST(CliParser, set_max_debuglog_size_expect_correct_parse)
{
    std::vector<const char*> argv = {
        "asan",
        "--max-debuglog-size=10"
    };

    optind = 1;
    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    ASSERT_EQ(capture.find("is invalid, range: 1-10240 (MB), default:1024 (MB)"), std::string::npos);
}

TEST(CliParser, set_max_debuglog_size_out_of_range_expect_alarm)
{
    std::vector<const char*> argv = {
        "asan",
        "--max-debuglog-size=10241"
    };

    optind = 1;
    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    ASSERT_NE(capture.find("is invalid. range: 1-10240 (MB), default:1024 (MB)"), std::string::npos);
}

TEST(CliParser, invalid_max_debuglog_size_expect_show_help_info)
{
    InvalidParamCheckHelpInfo("--max-debuglog-size=10MB");
    InvalidParamCheckHelpInfo("--max-debuglog-size=10mb");
    InvalidParamCheckHelpInfo("--max-debuglog-size=\t");
}

TEST(CliParser, invalid_single_dash_option_expect_one_error)
{
    std::vector<const char*> argv = {
        "asan",
        "-log-file=log.txt"
    };

    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    const char *errorStr = "unrecognized command";
    size_t pos = capture.find(errorStr);
    ASSERT_NE(pos, std::string::npos) << capture;
    pos = capture.find(errorStr, pos + 1);
    ASSERT_EQ(pos, std::string::npos);
}

TEST(CliParser, invalid_small_block_id_expect_found_error)
{
    std::vector<const char*> argv = {
        "asan",
        "--block-id=-1"
    };

    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    ASSERT_NE(capture.find("param 'block-id' contains invalid characters and is ignored"), std::string::npos);
}

TEST(CliParser, invalid_large_block_id_expect_found_error)
{
    std::vector<const char*> argv = {
        "asan",
        "--block-id=201"
    };

    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    ASSERT_NE(capture.find("--block-id param is invalid. range: [0, 200]"), std::string::npos);
}

TEST(CliParser, valid_block_id_expect_not_found_error)
{
    std::vector<const char*> argv = {
        "asan",
        "--block-id=0"
    };

    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    ASSERT_EQ(capture.find("--block-id param is invalid. range: [0, 200]"), std::string::npos);
}

TEST(CliParser, invalid_small_cache_size_expect_found_error)
{
    std::vector<const char*> argv = {
        "asan",
        "--cache-size=0"
    };

    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    ASSERT_NE(capture.find("--cache-size param is invalid. range: [1, 8192]"), std::string::npos);
}

TEST(CliParser, invalid_large_cache_size_expect_found_error)
{
    std::vector<const char*> argv = {
        "asan",
        "--cache-size=8193"
    };

    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    ASSERT_NE(capture.find("--cache-size param is invalid. range: [1, 8192]"), std::string::npos);
}

TEST(CliParser, valid_cache_size_expect_not_found_error)
{
    std::vector<const char*> argv = {
        "asan",
        "--cache-size=1"
    };

    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    ASSERT_EQ(capture.find("--cache-size param is invalid. range: [1, 8192]"), std::string::npos);
}

TEST(CliParser, pass_valid_kernel_name_expect_ok)
{
    std::vector<const char*> argv = {
        "asan",
        "--kernel-name=Add"
    };

    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    ASSERT_EQ(capture.find("--kernel-name param is invalid."), std::string::npos);
}

TEST(CliParser, pass_long_kernel_name_expect_error)
{
    std::string longName;
    for (int i = 0; i < KERNEL_NAME_MAX; i ++) {
        longName.append("a");
    }
    std::string param = "--kernel-name=" + longName;
    std::vector<const char*> argv = {
        "asan",
        param.c_str(),
    };

    CliParser cliParser;
    testing::internal::CaptureStdout();
    cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    std::string capture = testing::internal::GetCapturedStdout();
    ASSERT_NE(capture.find("--kernel-name param is invalid."), std::string::npos);
}

TEST(CliParser, no_set_full_backtrace_parameter_expect_get_full_backtrace_false)
{
    std::vector<const char*> argv = {
        "asan"
    };
 
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_FALSE(cmd.config.isPrintFullStack);
}
 
TEST(CliParser, set_full_backtrace_parameter_expect_get_full_backtrace_true)
{
    std::vector<const char*> argv = {
        "asan",
        "--full-backtrace=yes"
    };
 
    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.config.isPrintFullStack);
}

TEST(CliParser, do_not_set_demangle_mode_expect_get_default_demangle_mode_is_full_demangled_name)
{
    std::vector<const char*> argv = {
        "mssanitizer",
    };

    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_EQ(cmd.config.demangleMode, DemangleMode::FULL_DEMANGLED_NAME);
}
 
TEST(CliParser, set_demangle_mode_parameter_expect_get_correct_demangle_mode)
{
    std::vector<const char*> argv = {
        "mssanitizer",
        "--demangle",
        "full"
    };

    CliParser cliParser;

    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_EQ(cmd.config.demangleMode, DemangleMode::FULL_DEMANGLED_NAME);

    argv[2] = "simple";
    cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_EQ(cmd.config.demangleMode, DemangleMode::SIMPLE_DEMANGLED_NAME);

    argv[2] = "no";
    cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_EQ(cmd.config.demangleMode, DemangleMode::MANGLED_NAME);
}

TEST(CliParser, set_invalid_demangle_mode_parameter_expect_print_help)
{
    std::vector<const char*> argv = {
        "mssanitizer",
        "--demangle",
        "invalid"
    };

    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.printHelpInfo);
}

TEST(CliParser, set_check_cross_npu_races_yes_expect_get_check_cross_npu_races_true)
{
    std::vector<const char*> argv = {
        "mssanitizer",
        "--check-cross-npu-races=yes"
    };

    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_TRUE(cmd.config.checkCrossNpuRaces);
}

TEST(CliParser, set_check_cross_npu_races_no_expect_get_check_cross_npu_races_false)
{
    std::vector<const char*> argv = {
        "mssanitizer",
        "--check-cross-npu-races=no"
    };

    CliParser cliParser;
    UserCommand cmd = cliParser.Parse(argv.size(), const_cast<char**>(argv.data()));
    ASSERT_FALSE(cmd.config.checkCrossNpuRaces);
}

}
