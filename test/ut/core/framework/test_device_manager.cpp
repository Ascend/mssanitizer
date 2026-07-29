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

#include "core/framework/arch_def.h"
#include "core/framework/device_manager.h"
#include "core/framework/record_defs.h"
#include "core/framework/utility/spans.h"

using namespace Sanitizer;

TEST(DeviceManager, get_not_exist_device_id_expect_get_failed)
{
    DeviceInfoSummary deviceInfo{};
    ASSERT_FALSE(DeviceManager::Instance().Get(0, deviceInfo));
}

TEST(DeviceManager, get_exist_device_id_expect_get_success_and_get_correct_device_info)
{
    DeviceInfoSummary deviceInfo{};
    deviceInfo.device = DeviceType::ASCEND_910B1;
    deviceInfo.deviceId = 1;
    DeviceManager::Instance().Set(1, deviceInfo);

    DeviceInfoSummary deviceInfoGet{};
    ASSERT_TRUE(DeviceManager::Instance().Get(1, deviceInfoGet));
    ASSERT_EQ(deviceInfoGet.device, deviceInfo.device);
    ASSERT_EQ(deviceInfoGet.deviceId, deviceInfo.deviceId);

    DeviceManager::Instance().Clear();
}

TEST(DeviceManager, get_device_count_expect_get_correct_device_count)
{
    DeviceInfoSummary deviceInfo{};
    deviceInfo.device = DeviceType::ASCEND_910B1;
    deviceInfo.deviceId = 1;
    DeviceManager::Instance().Set(0, deviceInfo);
    DeviceManager::Instance().Set(1, deviceInfo);
    DeviceManager::Instance().Set(2, deviceInfo);

    ASSERT_EQ(DeviceManager::Instance().GetDeviceCount(), 3);

    DeviceManager::Instance().Clear();
}

TEST(DeviceManager, get_device_list_expect_get_correct_device_list)
{
    DeviceInfoSummary deviceInfo{};
    deviceInfo.device = DeviceType::ASCEND_910B1;
    deviceInfo.deviceId = 1;
    DeviceManager::Instance().Set(7, deviceInfo);
    DeviceManager::Instance().Set(5, deviceInfo);
    DeviceManager::Instance().Set(3, deviceInfo);

    auto const &deviceList = DeviceManager::Instance().GetDeviceList();
    ASSERT_EQ(deviceList.size(), 3);
    ASSERT_EQ(deviceList[0], 3);
    ASSERT_EQ(deviceList[1], 5);
    ASSERT_EQ(deviceList[2], 7);

    DeviceManager::Instance().Clear();
}

TEST(DeviceManager, get_shared_memory_spans_expect_created_and_accessible) {
    auto &spans0 = DeviceManager::Instance().GetSharedMemorySpans(0);
    spans0.Union(Span<uint64_t>{0x1000, 0x1100});
    spans0.Union(Span<uint64_t>{0x2000, 0x2200});

    auto &spans1 = DeviceManager::Instance().GetSharedMemorySpans(1);
    spans1.Union(Span<uint64_t>{0x3000, 0x3300});

    ASSERT_TRUE(spans0.HasIntersection(Span<uint64_t>{0x1050, 0x1060}));
    ASSERT_TRUE(spans0.HasIntersection(Span<uint64_t>{0x2100, 0x2200}));
    ASSERT_FALSE(spans0.HasIntersection(Span<uint64_t>{0x3000, 0x3010}));
    ASSERT_TRUE(spans1.HasIntersection(Span<uint64_t>{0x3050, 0x3060}));
    ASSERT_FALSE(spans1.HasIntersection(Span<uint64_t>{0x1000, 0x1010}));

    DeviceManager::Instance().Clear();
}

TEST(DeviceManager, set_overwrite_expect_updated_device_info) {
    DeviceInfoSummary deviceInfo{};
    deviceInfo.device = DeviceType::ASCEND_910B1;
    deviceInfo.deviceId = 1;
    DeviceManager::Instance().Set(0, deviceInfo);

    DeviceInfoSummary updatedInfo{};
    updatedInfo.device = DeviceType::ASCEND_910_PREMIUM_A;
    updatedInfo.deviceId = 2;
    DeviceManager::Instance().Set(0, updatedInfo);

    DeviceInfoSummary ret{};
    ASSERT_TRUE(DeviceManager::Instance().Get(0, ret));
    ASSERT_EQ(ret.device, DeviceType::ASCEND_910_PREMIUM_A);
    ASSERT_EQ(ret.deviceId, 2);

    DeviceManager::Instance().Clear();
}
