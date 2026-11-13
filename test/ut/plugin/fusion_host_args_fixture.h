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

#pragma once
#include <cstdint>
#include <gtest/gtest.h>

// 跨 Host/plugin 头文件边界传递真实适配器生成的参数；可选择原参数头长度及是否包含内联 tiling。
testing::AssertionResult MakeFusionAicArgs(uint8_t *record, uint8_t *output, uint32_t capacity, uint32_t &recordOffset,
    uint32_t headSize = 24, bool inlineTiling = true);
