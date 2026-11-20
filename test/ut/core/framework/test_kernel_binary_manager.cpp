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
#include <memory>
#include <vector>

#include "core/framework/kernel_binary_manager.h"

using namespace Sanitizer;

TEST(KernelBinaryManager, get_not_exist_kernel_name_expect_get_failed)
{
    KernelBinaryManager::BinaryPtr binary;
    ASSERT_FALSE(KernelBinaryManager::Instance().Get("unknown", binary));
}

TEST(KernelBinaryManager, get_exist_kernel_name_expect_get_success_and_get_correct_binary)
{
    std::vector<char> data{'1', '2', '3'};
    auto binary = BinaryBuffer::FromVector(std::make_shared<const std::vector<char>>(data));
    KernelBinaryManager::Instance().Set("kernel", binary);

    KernelBinaryManager::BinaryPtr binaryGet;
    ASSERT_TRUE(KernelBinaryManager::Instance().Get("kernel", binaryGet));
    ASSERT_NE(binaryGet, nullptr);
    ASSERT_EQ(std::vector<char>(binaryGet->data(), binaryGet->data() + binaryGet->size()), data);

    KernelBinaryManager::Instance().Clear();
}
