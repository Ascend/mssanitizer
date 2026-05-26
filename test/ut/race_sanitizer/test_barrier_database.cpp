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

#include "core/framework/barrier_database.hpp"

using namespace Sanitizer;

TEST(BarrierDatabase, barrier_event_wait_not_enough_worker_expect_return_false)
{
    std::size_t rank = 4;
    VectorTime vtGlobal;
    BarrierEvent<uint32_t> barrierEvent;
    ASSERT_FALSE(barrierEvent.Wait(rank, 0, {}, vtGlobal));
    ASSERT_FALSE(barrierEvent.Wait(rank, 1, {}, vtGlobal));
}

TEST(SoftSyncBarrierDatabase, barrier_event_wait_enough_worker_expect_return_true)
{
    std::size_t rank = 4;
    VectorTime vtGlobal;
    BarrierEvent<uint32_t> barrierEvent;
    ASSERT_FALSE(barrierEvent.Wait(rank, 0, {}, vtGlobal));
    ASSERT_FALSE(barrierEvent.Wait(rank, 1, {}, vtGlobal));
    ASSERT_FALSE(barrierEvent.Wait(rank, 2, {}, vtGlobal));
    // consume stage
    ASSERT_TRUE(barrierEvent.Wait(rank, 3, {}, vtGlobal));
    ASSERT_TRUE(barrierEvent.Wait(rank, 0, {}, vtGlobal));
    ASSERT_TRUE(barrierEvent.Wait(rank, 1, {}, vtGlobal));
    ASSERT_TRUE(barrierEvent.Wait(rank, 2, {}, vtGlobal));
}

TEST(SoftSyncBarrierDatabase, barrier_event_wait_not_exist_worker_expect_return_false)
{
    std::size_t rank = 2;
    VectorTime vtGlobal;
    BarrierEvent<uint32_t> barrierEvent;
    ASSERT_FALSE(barrierEvent.Wait(rank, 0, {}, vtGlobal));
    // consume stage
    ASSERT_TRUE(barrierEvent.Wait(rank, 1, {}, vtGlobal));
    // pipe not exist in barrier event
    ASSERT_FALSE(barrierEvent.Wait(rank, 2, {}, vtGlobal));
}

TEST(SoftSyncBarrierDatabase, barrier_event_wait_after_consumed_all_worker_expect_return_false)
{
    std::size_t rank = 2;
    VectorTime vtGlobal;
    BarrierEvent<uint32_t> barrierEvent;
    ASSERT_FALSE(barrierEvent.Wait(rank, 0, {}, vtGlobal));
    // consume stage
    ASSERT_TRUE(barrierEvent.Wait(rank, 1, {}, vtGlobal));
    ASSERT_TRUE(barrierEvent.Wait(rank, 0, {}, vtGlobal));
    // wait after consume stage
    ASSERT_FALSE(barrierEvent.Wait(rank, 0, {}, vtGlobal));
}

TEST(SoftSyncBarrierDatabase, barrier_event_wait_expect_get_correct_global_vector_time)
{
    std::size_t rank = 2;
    VectorTime vtGlobal;
    VectorTime vtExpect = {4, 3, 3, 4};
    BarrierEvent<uint32_t> barrierEvent;
    ASSERT_FALSE(barrierEvent.Wait(rank, 0, {1, 2, 3, 4}, vtGlobal));
    // consume stage
    ASSERT_TRUE(barrierEvent.Wait(rank, 1, {4, 3, 2, 1}, vtGlobal));
    ASSERT_EQ(vtGlobal, vtExpect);
    ASSERT_TRUE(barrierEvent.Wait(rank, 0, {1, 2, 3, 4}, vtGlobal));
    ASSERT_EQ(vtGlobal, vtExpect);
}

TEST(SoftSyncBarrierDatabase, barrier_event_wait_custom_worker_type_expect_correct_behavior)
{
    using Worker = std::pair<uint32_t, uint32_t>;

    std::size_t rank = 4;
    VectorTime vtGlobal;
    BarrierEvent<Worker> barrierEvent;
    ASSERT_FALSE(barrierEvent.Wait(rank, {0, 0}, {}, vtGlobal));
    ASSERT_FALSE(barrierEvent.Wait(rank, {0, 1}, {}, vtGlobal));
    ASSERT_FALSE(barrierEvent.Wait(rank, {1, 0}, {}, vtGlobal));
    // consume stage
    ASSERT_TRUE(barrierEvent.Wait(rank, {1, 1}, {}, vtGlobal));
    ASSERT_TRUE(barrierEvent.Wait(rank, {0, 0}, {}, vtGlobal));
    ASSERT_TRUE(barrierEvent.Wait(rank, {0, 1}, {}, vtGlobal));
    ASSERT_TRUE(barrierEvent.Wait(rank, {1, 0}, {}, vtGlobal));
}
