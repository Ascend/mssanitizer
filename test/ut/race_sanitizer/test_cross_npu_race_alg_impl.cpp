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

#include "alg_framework/cross_npu_race_alg_impl.h"
#include "core/framework/device_manager.h"
#include "core/framework/event_def.h"
#include "core/framework/kernel_manager.h"
#include "core/framework/record_defs.h"

using namespace Sanitizer;

namespace SanitizerTest {

class TestCrossNpuRaceAlgImpl : public testing::Test {
public:
    void SetUp() override
    {
        DeviceManager::Instance().Clear();
        KernelManager::Instance().Clear();

        DeviceInfoSummary deviceInfo{};
        deviceInfo.deviceId = 0;
        deviceInfo.device = DeviceType::ASCEND_910B1;
        DeviceManager::Instance().Set(deviceInfo.deviceId, deviceInfo);
        deviceInfo.deviceId = 1;
        DeviceManager::Instance().Set(deviceInfo.deviceId, deviceInfo);

        KernelSummary kernelSummary{};
        kernelSummary.blockDim = 8;
        kernelSummary.kernelType = KernelType::MIX;
        KernelManager::Instance().Add(0, kernelSummary);
        KernelManager::Instance().Add(1, kernelSummary);
    }
    void TearDown() override
    {
        DeviceManager::Instance().Clear();
        KernelManager::Instance().Clear();
    }
};

TEST_F(TestCrossNpuRaceAlgImpl, handle_mem_event_not_on_gm_expect_return_no_race)
{
    CrossNpuRaceAlgImpl alg;
    alg.Init();

    SanEvent event;
    event.loc.deviceId = 0;
    event.loc.deviceIdx = 0;
    event.loc.coreId = 0;
    event.loc.blockType = BlockType::AIVEC;
    event.loc.kernelIdx = 0;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE2;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    alg.Do(event);

    event.loc.deviceId = 1;
    event.loc.deviceIdx = 1;
    alg.Do(event);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::FINISH;
    alg.Do(event);

    ASSERT_EQ(alg.GetResult()->size(), 0U);
}

TEST_F(TestCrossNpuRaceAlgImpl, handle_mem_event_on_gm_expect_return_race)
{
    CrossNpuRaceAlgImpl alg;
    alg.Init();

    SanEvent event;
    event.loc.deviceId = 0;
    event.loc.deviceIdx = 0;
    event.loc.coreId = 0;
    event.loc.blockType = BlockType::AIVEC;
    event.loc.kernelIdx = 0;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE2;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    alg.Do(event);

    event.loc.deviceId = 1;
    event.loc.deviceIdx = 1;
    alg.Do(event);

    DeviceManager::Instance().GetSharedMemorySpans(0).Union({0x50, 0x51});
    DeviceManager::Instance().GetSharedMemorySpans(1).Union({0x50, 0x51});

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::FINISH;
    alg.Do(event);

    ASSERT_EQ(alg.GetResult()->size(), 1U);
}

} // namespace SanitizerTest
