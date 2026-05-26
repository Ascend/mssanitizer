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

#include "core/framework/sync_event_data_base.h"

using namespace Sanitizer;

TEST(SyncEventDataBase, set_and_wait_same_event_expect_success)
{
    SyncEventDataBase db;

    VectorTime vt1;
    SyncEvent e1;
    e1.info.srcPipe = 1;
    e1.info.dstPipe = 2;
    e1.info.eventId = 3;
    e1.info.memType = 4;
    db.Set(e1, vt1);

    ASSERT_TRUE(db.Get(e1, vt1));
}

TEST(SyncEventDataBase, set_and_wait_different_event_expect_fail)
{
    SyncEventDataBase db;

    VectorTime vt1;
    SyncEvent e1;
    e1.info.srcPipe = 1;
    e1.info.dstPipe = 2;
    e1.info.eventId = 3;
    e1.info.memType = 4;
    db.Set(e1, vt1);

    SyncEvent e2;
    e2.info.srcPipe = 2;
    e2.info.dstPipe = 2;
    e2.info.eventId = 3;
    e2.info.memType = 4;
    ASSERT_FALSE(db.Get(e2, vt1));
}

TEST(SyncEventDataBase, set_and_wait_expect_get_the_correct_vector_time)
{
    SyncEventDataBase db;

    SyncEvent e1;
    VectorTime vt1;
    vt1.resize(static_cast<uint8_t>(PipeType::SIZE), 2);
    VectorClock::UpdateLogicTime(vt1, static_cast<uint8_t>(PipeType::PIPE_V));
    VectorClock::UpdateLogicTime(vt1, static_cast<uint8_t>(PipeType::PIPE_V));

    e1.info.srcPipe = 1;
    e1.info.dstPipe = 2;
    e1.info.eventId = 3;
    e1.info.memType = 4;

    VectorTime vt2;
    db.Set(e1, vt1);
    ASSERT_TRUE(db.Get(e1, vt2));
    ASSERT_TRUE(VectorClock::IsEqual(vt1, vt2));
}

TEST(SyncEventDataBase, set_many_times_then_wait_expect_get_the_correct_vector_time)
{
    SyncEventDataBase db;

    SyncEvent e1;
    e1.info.srcPipe = 1;
    e1.info.dstPipe = 2;
    e1.info.eventId = 3;
    e1.info.memType = 4;

    VectorTime vt1;
    vt1.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    db.Set(e1, vt1);

    VectorClock::UpdateLogicTime(vt1, static_cast<uint8_t>(PipeType::PIPE_V));
    db.Set(e1, vt1);

    VectorClock::UpdateLogicTime(vt1, static_cast<uint8_t>(PipeType::PIPE_V));
    db.Set(e1, vt1);

    VectorTime vt2;
    vt2.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    db.Get(e1, vt1);
    ASSERT_TRUE(VectorClock::IsEqual(vt1, vt2));

    VectorClock::UpdateLogicTime(vt2, static_cast<uint8_t>(PipeType::PIPE_V));
    db.Get(e1, vt1);
    ASSERT_TRUE(VectorClock::IsEqual(vt1, vt2));

    VectorClock::UpdateLogicTime(vt2, static_cast<uint8_t>(PipeType::PIPE_V));
    db.Get(e1, vt1);
    ASSERT_TRUE(VectorClock::IsEqual(vt1, vt2));

    ASSERT_FALSE(db.Get(e1, vt1));
}
