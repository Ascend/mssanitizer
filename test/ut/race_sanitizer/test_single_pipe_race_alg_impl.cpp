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

// GET_BUF(BLOCK_MODE, rlsCount>0)起同步屏障作用，前后MEM_EVENT不应检测到竞争
TEST(SinglePipeRaceAlgImpl, get_buf_block_mode_expect_no_race) {
    SinglePipeRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent event;

    event.loc.coreId = 0;
    event.pipe = PipeType::PIPE_V;
    event.loc.blockType = BlockType::AIVEC;

    // 第一条mem事件：写地址0x50
    event.type = EventType::MEM_EVENT;
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

    // GET_BUF(BLOCK_MODE)，rlsCount>0表示非首个get_buf，起屏障作用
    event.type = EventType::BUF_SYNC_EVENT;
    event.eventInfo.bufSyncInfo.opType = SyncType::GET_BUF;
    event.eventInfo.bufSyncInfo.mode = BufMode::BLOCK_MODE;
    event.eventInfo.bufSyncInfo.bufId = 0;
    event.eventInfo.bufSyncInfo.rlsCount = 1U;
    event.serialNo = 2U;
    alg.Do(event);

    // 第二条mem事件：读地址0x50，union已被bufSyncInfo覆盖，需重新设置所有memInfo字段
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo.opType = AccessType::READ;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.serialNo = 3U;
    alg.Do(event);

    ASSERT_EQ(alg.IsFinished(), false);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    ASSERT_EQ(alg.GetRaceCount(), 0U);
    ASSERT_EQ(alg.IsFinished(), true);
}

// 首个GET_BUF(rlsCount==0)不具备阻塞作用，前后MEM_EVENT应检测到竞争
TEST(SinglePipeRaceAlgImpl, first_get_buf_expect_race) {
    SinglePipeRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent event;

    event.loc.coreId = 0;
    event.pipe = PipeType::PIPE_V;
    event.loc.blockType = BlockType::AIVEC;

    // 第一条mem事件：写地址0x50
    event.type = EventType::MEM_EVENT;
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

    // 首个GET_BUF(BLOCK_MODE)，rlsCount==0，不起屏障作用
    event.type = EventType::BUF_SYNC_EVENT;
    event.eventInfo.bufSyncInfo.opType = SyncType::GET_BUF;
    event.eventInfo.bufSyncInfo.mode = BufMode::BLOCK_MODE;
    event.eventInfo.bufSyncInfo.bufId = 0;
    event.eventInfo.bufSyncInfo.rlsCount = 0U;
    event.serialNo = 2U;
    alg.Do(event);

    // 第二条mem事件：读地址0x50，union已被bufSyncInfo覆盖，需重新设置所有memInfo字段
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo.opType = AccessType::READ;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.serialNo = 3U;
    alg.Do(event);

    ASSERT_EQ(alg.IsFinished(), false);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    ASSERT_EQ(alg.GetRaceCount(), 1U);
    ASSERT_EQ(alg.IsFinished(), true);
}

// RLS_BUF不阻塞流水线，前后MEM_EVENT应检测到竞争
TEST(SinglePipeRaceAlgImpl, rls_buf_expect_race) {
    SinglePipeRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent event;

    event.loc.coreId = 0;
    event.pipe = PipeType::PIPE_V;
    event.loc.blockType = BlockType::AIVEC;

    // 第一条mem事件：写地址0x50
    event.type = EventType::MEM_EVENT;
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

    // RLS_BUF不阻塞流水线，不递增barrierNo
    event.type = EventType::BUF_SYNC_EVENT;
    event.eventInfo.bufSyncInfo.opType = SyncType::RLS_BUF;
    event.eventInfo.bufSyncInfo.bufId = 0;
    event.serialNo = 2U;
    alg.Do(event);

    // 第二条mem事件：读地址0x50，union已被bufSyncInfo覆盖，需重新设置所有memInfo字段
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo.opType = AccessType::READ;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.serialNo = 3U;
    alg.Do(event);

    ASSERT_EQ(alg.IsFinished(), false);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    ASSERT_EQ(alg.GetRaceCount(), 1U);
    ASSERT_EQ(alg.IsFinished(), true);
}
}
