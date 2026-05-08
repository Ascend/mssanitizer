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
#include <any>
#include <mutex>

#include "constant.h"
#define private public
#include "call_stack.h"
#include "utility/file_system.h"
#undef private

using namespace Sanitizer;

class TestCallStack : public testing::Test {
public:
    static void SetUpTestCase()
    {
        char dirBuff[1024]{0};
        constexpr mode_t testDirMode = 0755;
        getcwd(dirBuff, sizeof(dirBuff));
        initDir = dirBuff;
        testDir = "callstack_test_dir";
        remove(testDir.c_str());
        mkdir(testDir.c_str(), testDirMode);
        chdir(testDir.c_str());
    }
    static void TearDownTestCase()
    {
        chdir(initDir.c_str());
        rmdir(testDir.c_str());
    }
    void SetUp() override
    {
        ASSERT_TRUE(IsPathExists(initDir.c_str()));
        CallStack::Instance().pcStackMap_.clear();
    }
    void TearDown() override
    {
        CallStack::Instance().pcStackMap_.clear();
    }
    static std::string initDir;
    static std::string testDir;
};

std::string TestCallStack::initDir;
std::string TestCallStack::testDir;

TEST_F(TestCallStack, load_from_buffer_stream_expect_get_empty_map)
{
    CallStack &callStack = CallStack::Instance();
    std::vector<char> buffer;
    callStack.Load(buffer);
    ASSERT_TRUE(callStack.pcStackMap_.empty());
}

TEST_F(TestCallStack, query_invalid_pc_expect_get_none)
{
    CallStack &callStack = CallStack::Instance();
    CallStack::Stack stack = callStack.Query("kernel", INVALID_PC_OFFSET);
    ASSERT_EQ(stack.size(), 0);
}

TEST_F(TestCallStack, not_load_query_valid_pc_expect_get_none)
{
    CallStack &callStack = CallStack::Instance();
    CallStack::Stack stack = callStack.Query("kernel", 3);
    ASSERT_EQ(stack.size(), 0);
}

TEST_F(TestCallStack, parse_invalid_json_data_expect_left_stack_cache_map_unchanged)
{
    CallStack &callstack = CallStack::Instance();
    std::string msg = "invalid json data";
    auto stackCacheMap = callstack.ParseStacks(msg);
    ASSERT_TRUE(stackCacheMap.empty());
}

TEST_F(TestCallStack, parse_json_data_of_unexpected_type_expect_left_stack_cache_map_unchanged)
{
    CallStack &callstack = CallStack::Instance();
    std::string msg = "123";
    auto stackCacheMap = callstack.ParseStacks(msg);
    ASSERT_TRUE(stackCacheMap.empty());
}

TEST_F(TestCallStack, parse_json_data_of_symbolizer_error_expect_left_stack_cache_map_unchanged)
{
    CallStack &callstack = CallStack::Instance();
    std::string msg = "{\"Error\": \"some error\"}";
    auto stackCacheMap = callstack.ParseStacks(msg);
    ASSERT_TRUE(stackCacheMap.empty());
}

TEST_F(TestCallStack, parse_json_data_of_symbol_of_not_array_expect_left_stack_cache_map_unchanged)
{
    CallStack &callstack = CallStack::Instance();
    std::string msg = "{\"Symbol\": \"symbol of string type\"}";
    auto stackCacheMap = callstack.ParseStacks(msg);
    ASSERT_TRUE(stackCacheMap.empty());
}

TEST_F(TestCallStack, parse_json_data_of_valid_symbol_expect_get_correct_stack)
{
    CallStack &callstack = CallStack::Instance();
    std::string msg = "{\"Address\": \"0x100\", \"Symbol\": "
        "[{\"FileName\": \"test.cpp\", \"FunctionName\": \"func\", \"Line\": 100, \"Column\": 200}]}";
    auto stackCacheMap = callstack.ParseStacks(msg);
    ASSERT_EQ(stackCacheMap.size(), 1UL);
    typename CallStack::Stack &stack = stackCacheMap[0x100];
    ASSERT_EQ(stack.size(), 1UL);
    ASSERT_EQ(stack[0].fileName, "test.cpp");
    ASSERT_EQ(stack[0].funcName, "func");
    constexpr uint64_t line = 100;
    constexpr uint64_t column = 200;
    ASSERT_EQ(stack[0].line, line);
    ASSERT_EQ(stack[0].column, column);
}

TEST_F(TestCallStack, parse_json_data_with_missing_line_number_expect_strip_frame)
{
    CallStack &callstack = CallStack::Instance();
    std::string msg = "{\"Address\": \"0x100\", \"Symbol\": "
        "[{\"FileName\": \"test.cpp\", \"FunctionName\": \"func\", \"Line\": 0, \"Column\": 0},"
        "{\"FileName\": \"test.cpp\", \"FunctionName\": \"func\", \"Line\": 100, \"Column\": 200}]}";
    auto stackCacheMap = callstack.ParseStacks(msg);
    ASSERT_EQ(stackCacheMap.size(), 1UL);
    typename CallStack::Stack &stack = stackCacheMap[0x100];
    ASSERT_EQ(stack.size(), 1UL);
    ASSERT_EQ(stack[0].fileName, "test.cpp");
    ASSERT_EQ(stack[0].funcName, "func");
    constexpr uint64_t line = 100;
    constexpr uint64_t column = 200;
    ASSERT_EQ(stack[0].line, line);
    ASSERT_EQ(stack[0].column, column);
}

TEST_F(TestCallStack, query_valid_pc_with_unset_env_expect_none)
{
    CallStack &callStack = CallStack::Instance();
    constexpr size_t binaryBuffSize = 10;
    std::vector<char> buffer(binaryBuffSize);
    callStack.Load(buffer);
    unsetenv("ASCEND_HOME_PATH");
    CallStack::Stack stack = callStack.Query("kernel", 3);
    ASSERT_EQ(stack.size(), 0);
}

TEST_F(TestCallStack, query_valid_pc_with_invalid_env_expect_none)
{
    CallStack &callStack = CallStack::Instance();
    constexpr size_t binaryBuffSize = 10;
    std::vector<char> buffer(binaryBuffSize);
    callStack.Load(buffer);
    setenv("ASCEND_HOME_PATH", "/a/", 1);
    CallStack::Stack stack = callStack.Query("kernel", 3);
    ASSERT_EQ(stack.size(), 0);
}
