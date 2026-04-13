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

#include "core/framework/event_def.h"
#include "race_sanitizer/alg_framework/soft_sync_barrier_database.h"
#include "sanitizer_report.h"

using namespace Sanitizer;

TEST(SoftSyncBarrierDatabase, barrier_event_wait_not_enough_device_expect_return_false)
{
    MstxCrossNpuBarrier barrier{};
    barrier.isAIVOnly = true;
    barrier.usedDeviceNum = 4;
    barrier.usedCoreNum = 1;

    VectorTime vtGlobal;
    SoftSyncBarrierDatabase::BarrierEvent barrierEvent;
    ASSERT_FALSE(barrierEvent.Wait(0, 0, barrier, {}, vtGlobal));
    ASSERT_FALSE(barrierEvent.Wait(1, 0, barrier, {}, vtGlobal));
}

TEST(SoftSyncBarrierDatabase, barrier_event_wait_not_enough_block_expect_return_false)
{
    MstxCrossNpuBarrier barrier{};
    barrier.isAIVOnly = true;
    barrier.usedDeviceNum = 1;
    barrier.usedCoreNum = 4;

    VectorTime vtGlobal;
    SoftSyncBarrierDatabase::BarrierEvent barrierEvent;
    ASSERT_FALSE(barrierEvent.Wait(0, 0, barrier, {}, vtGlobal));
    ASSERT_FALSE(barrierEvent.Wait(0, 1, barrier, {}, vtGlobal));
}

TEST(SoftSyncBarrierDatabase, barrier_event_wait_enough_block_expect_return_true)
{
    MstxCrossNpuBarrier barrier{};
    barrier.isAIVOnly = true;
    barrier.usedDeviceNum = 2;
    barrier.usedCoreNum = 2;

    VectorTime vtGlobal;
    SoftSyncBarrierDatabase::BarrierEvent barrierEvent;
    ASSERT_FALSE(barrierEvent.Wait(0, 0, barrier, {}, vtGlobal));
    ASSERT_FALSE(barrierEvent.Wait(0, 1, barrier, {}, vtGlobal));
    ASSERT_FALSE(barrierEvent.Wait(1, 0, barrier, {}, vtGlobal));
    // consume stage
    ASSERT_TRUE(barrierEvent.Wait(1, 1, barrier, {}, vtGlobal));
    ASSERT_TRUE(barrierEvent.Wait(0, 0, barrier, {}, vtGlobal));
    ASSERT_TRUE(barrierEvent.Wait(0, 1, barrier, {}, vtGlobal));
    ASSERT_TRUE(barrierEvent.Wait(1, 0, barrier, {}, vtGlobal));
}

TEST(SoftSyncBarrierDatabase, barrier_event_wait_not_exist_pipe_expect_return_false)
{
    MstxCrossNpuBarrier barrier{};
    barrier.isAIVOnly = true;
    barrier.usedDeviceNum = 2;
    barrier.usedCoreNum = 1;

    VectorTime vtGlobal;
    SoftSyncBarrierDatabase::BarrierEvent barrierEvent;
    ASSERT_FALSE(barrierEvent.Wait(0, 0, barrier, {}, vtGlobal));
    // consume stage
    ASSERT_TRUE(barrierEvent.Wait(1, 0, barrier, {}, vtGlobal));
    // pipe not exist in barrier event
    ASSERT_FALSE(barrierEvent.Wait(2, 2, barrier, {}, vtGlobal));
}

TEST(SoftSyncBarrierDatabase, barrier_event_wait_after_consumed_all_pipe_expect_return_false)
{
    MstxCrossNpuBarrier barrier{};
    barrier.isAIVOnly = true;
    barrier.usedDeviceNum = 2;
    barrier.usedCoreNum = 1;

    VectorTime vtGlobal;
    SoftSyncBarrierDatabase::BarrierEvent barrierEvent;
    ASSERT_FALSE(barrierEvent.Wait(0, 0, barrier, {}, vtGlobal));
    // consume stage
    ASSERT_TRUE(barrierEvent.Wait(1, 0, barrier, {}, vtGlobal));
    ASSERT_TRUE(barrierEvent.Wait(0, 0, barrier, {}, vtGlobal));
    // wait after consume stage
    ASSERT_FALSE(barrierEvent.Wait(0, 0, barrier, {}, vtGlobal));
}

TEST(SoftSyncBarrierDatabase, barrier_event_wait_expect_get_correct_global_vector_time)
{
    MstxCrossNpuBarrier barrier{};
    barrier.isAIVOnly = true;
    barrier.usedDeviceNum = 2;
    barrier.usedCoreNum = 1;

    VectorTime vtGlobal;
    VectorTime vtExpect = {4, 3, 3, 4};
    SoftSyncBarrierDatabase::BarrierEvent barrierEvent;
    ASSERT_FALSE(barrierEvent.Wait(0, 0, barrier, {1, 2, 3, 4}, vtGlobal));
    // consume stage
    ASSERT_TRUE(barrierEvent.Wait(1, 0, barrier, {4, 3, 2, 1}, vtGlobal));
    ASSERT_EQ(vtGlobal, vtExpect);
    ASSERT_TRUE(barrierEvent.Wait(0, 0, barrier, {1, 2, 3, 4}, vtGlobal));
    ASSERT_EQ(vtGlobal, vtExpect);
}