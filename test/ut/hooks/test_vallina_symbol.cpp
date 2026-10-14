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
#include <dlfcn.h>

#include "vallina_symbol.h"

namespace {
// 可稳定解析到的库，用于验证符号解析契约
struct LibcLibLoader {
    static void *Load(void) { return dlopen("libc.so.6", RTLD_NOW | RTLD_GLOBAL); }
};

// 模拟加载失败（如环境缺失），用于验证降级路径
struct NullLibLoader {
    static void *Load(void) { return nullptr; }
};

using LibcSymbol = VallinaSymbol<LibcLibLoader>;
using NullLibSymbol = VallinaSymbol<NullLibLoader>;
} // namespace

// 已存在的符号应解析成功
TEST(VallinaSymbol, Get_existing_symbol_expect_non_null) { ASSERT_NE(LibcSymbol::Instance().Get("malloc"), nullptr); }

// 不存在的符号应返回空，调用方据此降级，避免调用空指针
TEST(VallinaSymbol, Get_missing_symbol_expect_null) {
    ASSERT_EQ(LibcSymbol::Instance().Get("mssanitizer_ut_no_such_symbol"), nullptr);
}

// 库句柄为空（加载失败）时应安全返回空
TEST(VallinaSymbol, Get_when_handle_null_expect_null) { ASSERT_EQ(NullLibSymbol::Instance().Get("malloc"), nullptr); }
