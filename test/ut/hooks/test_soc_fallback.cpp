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

#include "hooks/soc_fallback.h"
#include "core/SocResolutionCases.h"

namespace {

class SocFallbackTest : public testing::Test {};

TEST_F(SocFallbackTest, resolves_exact_and_prefix_soc_with_valid_specs) {
    for (const auto &testCase : SocResolutionTestData::RESOLVED_SOC_CASES) {
        SCOPED_TRACE(testCase.soc);
        const auto device = Sanitizer::ResolveSocVersion(testCase.soc);
        EXPECT_EQ(static_cast<Sanitizer::DeviceType>(testCase.device), device);
        if (device != Sanitizer::DeviceType::INVALID) {
            EXPECT_EQ(1U, Sanitizer::CHIP_INFO_MAP.count(device));
        }
    }
}

TEST_F(SocFallbackTest, rejects_unknown_or_incomplete_prefixes) {
    for (const auto *soc : SocResolutionTestData::INVALID_SOC_NAMES) {
        SCOPED_TRACE(soc);
        EXPECT_EQ(Sanitizer::DeviceType::INVALID, Sanitizer::ResolveSocVersion(soc));
    }
}

// 拒绝名单的优先级由 common UT 覆盖，hook 不再复制名单或通过宏替换名单。
TEST_F(SocFallbackTest, default_devices_have_valid_specs) {
    for (const auto device : {Sanitizer::DeviceType::ASCEND_910B_DEFAULT, Sanitizer::DeviceType::ASCEND_950_DEFAULT}) {
        EXPECT_EQ(1U, Sanitizer::CHIP_INFO_MAP.count(device));
        EXPECT_TRUE(Sanitizer::HasSubBlocks(device));
    }
    EXPECT_TRUE(Sanitizer::IsAscend95(Sanitizer::DeviceType::ASCEND_950_DEFAULT));
    EXPECT_FALSE(Sanitizer::IsAscend95(Sanitizer::DeviceType::ASCEND_910B_DEFAULT));
}

} // namespace
