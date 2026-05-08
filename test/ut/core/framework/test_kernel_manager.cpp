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

#include <cstring>
#include <gtest/gtest.h>

#include "core/framework/config.h"
#include "core/framework/kernel_manager.h"
#include "core/framework/record_defs.h"

using namespace Sanitizer;

class TestKernelManager : public testing::Test {
public:
    TestKernelManager() {}

    void TearDown() override
    {
        KernelManager::Instance().Clear();
    }
};

TEST_F(TestKernelManager, get_not_exist_device_id_expect_get_failed)
{
    KernelSummary kernelSummary{};
    ASSERT_FALSE(KernelManager::Instance().Get(0, 0, kernelSummary));
}

TEST_F(TestKernelManager, get_not_exist_kernel_idx_expect_get_failed)
{
    KernelSummary kernelSummary{};
    KernelManager::Instance().Add(0, kernelSummary);
    ASSERT_FALSE(KernelManager::Instance().Get(0, 1, kernelSummary));
}

TEST_F(TestKernelManager, add_some_kernels_expect_get_correct_kernel_index)
{
    KernelSummary kernelSummary{};
    ASSERT_EQ(KernelManager::Instance().Add(0, kernelSummary), 0);
    ASSERT_EQ(KernelManager::Instance().Add(0, kernelSummary), 1);
}

TEST_F(TestKernelManager, get_valid_kernel_expect_get_correct_kernel_summary)
{
    KernelSummary kernelSummary{};
    std::strncpy(kernelSummary.kernelName, "kernel", sizeof(kernelSummary.kernelName) - 1);
    std::size_t kernelIdx = KernelManager::Instance().Add(0, kernelSummary);

    KernelSummary kernelSummaryGet{};
    ASSERT_TRUE(KernelManager::Instance().Get(0, kernelIdx, kernelSummaryGet));
    ASSERT_EQ(std::string(kernelSummaryGet.kernelName), std::string(kernelSummary.kernelName));
}

TEST_F(TestKernelManager, get_kernel_count_expect_get_correct_kernel_count)
{
    KernelSummary kernelSummary{};
    KernelManager::Instance().Add(0, kernelSummary);
    KernelManager::Instance().Add(0, kernelSummary);

    std::size_t kernelCount{};
    ASSERT_TRUE(KernelManager::Instance().GetKernelCount(0, kernelCount));
    ASSERT_EQ(kernelCount, 2);
}

TEST_F(TestKernelManager, get_display_kernel_name_with_option_mangled_expect_get_mangled_name)
{
    KernelManager::Instance().SetDemangleMode(DemangleMode::MANGLED_NAME);

    std::string kernelName = "_Z29illegal_read_and_write_kernelIiEvPhS0_";
    std::string kernelNameDisplay = KernelManager::Instance().GetDisplayKernelName(kernelName);
    ASSERT_EQ(kernelNameDisplay, kernelName);
}

TEST_F(TestKernelManager, get_display_kernel_name_with_option_full_demangled_expect_get_full_demangled_name)
{
    KernelManager::Instance().SetDemangleMode(DemangleMode::FULL_DEMANGLED_NAME);

    std::string kernelName = "_Z29illegal_read_and_write_kernelIiEvPhS0_";
    std::string kernelNameDisplay = KernelManager::Instance().GetDisplayKernelName(kernelName);
    ASSERT_EQ(kernelNameDisplay, "\"void illegal_read_and_write_kernel<int>(unsigned char*, unsigned char*)\"");
}

TEST_F(TestKernelManager, get_display_kernel_name_with_option_simple_demangled_expect_get_simple_demangled_name)
{
    KernelManager::Instance().SetDemangleMode(DemangleMode::SIMPLE_DEMANGLED_NAME);

    std::string kernelName = "_Z29illegal_read_and_write_kernelIiEvPhS0_";
    std::string kernelNameDisplay = KernelManager::Instance().GetDisplayKernelName(kernelName);
    ASSERT_EQ(kernelNameDisplay, "illegal_read_and_write_kernel");
}