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
#include <sstream>
#include <cstdlib>

#define private public
#include "core/framework/call_stack.h"
#include "core/framework/constant.h"
#undef private

using namespace Sanitizer;

class TestCallStackParse : public testing::Test {
protected:
    void SetUp() override
    {
        auto &cs = CallStack::Instance();
        std::lock_guard<std::mutex> lock(cs.mtx_);
        cs.pcStackMap_.clear();
        cs.isPrintFullStack_ = false;
    }
};

// 覆盖 ParseEachStack 各分支：含 Error / 缺 Symbol / Symbol 非数组 / 缺 Address → 空；正常解析保留；首帧 line/column 为 0 时被擦除。
TEST_F(TestCallStackParse, ParseEachStack_AllPaths_expect_correct)
{
    auto &cs = CallStack::Instance();

    std::vector<nlohmann::json> emptyInputs = {
        {{"Error", "some error"}},
        {{"Address", "0x100"}},
        {{"Symbol", "not_array"}, {"Address", "0x100"}},
        {{"Symbol", nlohmann::json::array()}},
    };
    for (auto &j : emptyInputs) {
        CallStack::StackEachKernel map;
        cs.ParseEachStack(j, map);
        EXPECT_TRUE(map.empty());
    }

    nlohmann::json jNormal = {
        {"Address", "0x100"},
        {"Symbol", nlohmann::json::array({
            {{"FileName", "foo.cpp"}, {"FunctionName", "bar"}, {"Line", 10}, {"Column", 5}},
            {{"FileName", "baz.cpp"}, {"FunctionName", "qux"}, {"Line", 20}, {"Column", 8}},
        })},
    };
    CallStack::StackEachKernel map;
    cs.ParseEachStack(jNormal, map);
    ASSERT_EQ(map.size(), 1U);
    auto it = map.find(0x100);
    ASSERT_NE(it, map.end());
    ASSERT_EQ(it->second.size(), 2U);
    EXPECT_EQ(it->second[0].fileName, "foo.cpp");

    nlohmann::json jZero = {
        {"Address", "0x200"},
        {"Symbol", nlohmann::json::array({
            {{"FileName", "foo.cpp"}, {"FunctionName", "bar"}, {"Line", 0}, {"Column", 0}},
            {{"FileName", "baz.cpp"}, {"FunctionName", "qux"}, {"Line", 20}, {"Column", 8}},
        })},
    };
    CallStack::StackEachKernel map2;
    cs.ParseEachStack(jZero, map2);
    ASSERT_EQ(map2.size(), 1U);
    auto it2 = map2.find(0x200);
    ASSERT_NE(it2, map2.end());
    ASSERT_EQ(it2->second.size(), 1U);
    EXPECT_EQ(it2->second[0].fileName, "baz.cpp");
}

// 覆盖 ParseStacks：空输入 / 非法 JSON / 非对象非数组 → 空；单对象 / 数组 / 多行 → 解析
TEST_F(TestCallStackParse, ParseStacks_AllPaths_expect_correct)
{
    auto &cs = CallStack::Instance();
    const std::string single = R"({"Address":"0x100","Symbol":[{"FileName":"f.cpp","FunctionName":"fn","Line":1,"Column":2}]})";

    for (auto &input : std::vector<std::string>{"", "not a json", "42"}) {
        EXPECT_TRUE(cs.ParseStacks(input).empty());
    }

    EXPECT_EQ(cs.ParseStacks(single).size(), 1U);
    EXPECT_EQ(cs.ParseStacks("[" + single + "]").size(), 1U);

    std::string multi = single + "\n" + R"({"Address":"0x200","Symbol":[{"FileName":"g.cpp","FunctionName":"gn","Line":3,"Column":4}]})";
    auto map = cs.ParseStacks(multi);
    EXPECT_EQ(map.size(), 2U);
    EXPECT_NE(map.find(0x100), map.end());
    EXPECT_NE(map.find(0x200), map.end());
}

// 覆盖 Query：INVALID_PC_OFFSET 返回空栈；缓存命中返回缓存数据
// 覆盖 CachePcOffsets：空 offsets / 全部已缓存 / 含 INVALID_PC_OFFSET 过滤
TEST_F(TestCallStackParse, Query_AndCachePcOffsets_expect_correct)
{
    auto &cs = CallStack::Instance();
    EXPECT_TRUE(cs.Query("test_kernel", INVALID_PC_OFFSET).empty());
    {
        std::lock_guard<std::mutex> lock(cs.mtx_);
        cs.pcStackMap_["test_kernel"][0x100] = {CallStack::Location{"func", "file.cpp", 10, 5}};
    }
    auto stack = cs.Query("test_kernel", 0x100);
    ASSERT_EQ(stack.size(), 1U);
    EXPECT_EQ(stack[0].fileName, "file.cpp");

    cs.CachePcOffsets("test_kernel", {});
    cs.CachePcOffsets("test_kernel", {0x100});
    cs.CachePcOffsets("test_kernel", {0x100, INVALID_PC_OFFSET});
}

// 覆盖 FormatCallStack：全量打印 / 无 ASCEND_HOME_PATH 全量打印 / 有 ASCEND_HOME_PATH 过滤
TEST_F(TestCallStackParse, FormatCallStack_AllPaths_expect_correct)
{
    auto &cs = CallStack::Instance();
    CallStack::Stack stack = {
        {"func1", "/ascend/home/file1.cpp", 10, 5},
        {"func2", "/other/file2.cpp", 20, 8},
    };

    cs.isPrintFullStack_ = true;
    std::ostringstream oss;
    cs.FormatCallStack(oss, stack);
    EXPECT_NE(oss.str().find("file2.cpp"), std::string::npos);

    cs.isPrintFullStack_ = false;
    unsetenv("ASCEND_HOME_PATH");
    std::ostringstream oss2;
    cs.FormatCallStack(oss2, stack);
    EXPECT_NE(oss2.str().find("file1.cpp"), std::string::npos);

    setenv("ASCEND_HOME_PATH", "/ascend/home", 1);
    std::ostringstream oss3;
    cs.FormatCallStack(oss3, stack);
    auto out = oss3.str();
    EXPECT_EQ(out.find("file1.cpp"), std::string::npos);
    EXPECT_NE(out.find("file2.cpp"), std::string::npos);
    unsetenv("ASCEND_HOME_PATH");
}
