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

#include "alg_framework/single_pipe_race_alg_impl.h"

namespace {
using namespace Sanitizer;

TEST(SinglePipeRaceAlgImpl, race_alg_can_detect_race_events_expect_success)
{
    SinglePipeRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent event;

    event.loc.coreId = 0;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    event.loc.blockType = BlockType::AIVEC;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.serialNo = 1U;
    alg.Do(event);
    event.eventInfo.memInfo.opType = AccessType::READ;
    event.serialNo = 2U;
    alg.Do(event);
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.serialNo = 3U;
    alg.Do(event);
    ASSERT_EQ(alg.IsFinished(), false);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    ASSERT_EQ(alg.GetRaceCount(), 2U);
    ASSERT_EQ(alg.IsFinished(), true);
}

TEST(SinglePipeRaceAlgImpl, race_alg_detect_pipe_barrier_expect_no_race)
{
    SinglePipeRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent event;

    event.loc.coreId = 0;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    event.loc.blockType = BlockType::AIVEC;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.serialNo = 1U;
    alg.Do(event);
    event.type = EventType::SYNC_EVENT;
    event.eventInfo.syncInfo.opType = SyncType::PIPE_BARRIER;
    event.serialNo = 2U;
    alg.Do(event);
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo.opType = AccessType::READ;
    event.serialNo = 3U;
    alg.Do(event);
    ASSERT_EQ(alg.IsFinished(), false);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    ASSERT_EQ(alg.GetRaceCount(), 0U);
    ASSERT_EQ(alg.IsFinished(), true);
}

}
