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

#include "core/framework/event_container.h"

using namespace Sanitizer;

TEST(EventContainer, push_some_events_and_expect_the_size_is_right)
{
    EventContainer container;
    container.Init(1);
    SanEvent e;
    e.pipe = PipeType::PIPE_M;

    for (uint32_t i = 0; i < 100U; i++) {
        container.Push(e, e.pipe, 0);
    }

    container.SetQueIndex(PipeType::PIPE_M);
    ASSERT_EQ(container.GetCurQueSize(), 100U);
    ASSERT_EQ(container.GetAllQueSize(), 100U);
}

TEST(EventContainer, push_event_with_invalid_blockIdx_and_expect_no_dump)
{
    EventContainer container;
    container.Init(1);
    SanEvent e;
    e.pipe = PipeType::PIPE_S;
    e.loc.coreId = 1001;
    e.serialNo = 1000;

    ASSERT_NO_THROW(container.Push(e, e.pipe, 8001));
    ASSERT_EQ(container.IsEmpty(), true);
}

TEST(EventContainer, pop_some_events_and_expect_the_size_is_right)
{
    EventContainer container;
    container.Init(1);

    SanEvent e;
    e.pipe = PipeType::PIPE_M;
    for (uint32_t i = 0; i < 100U; i++) {
        container.Push(e, e.pipe, 0);
    }

    e.pipe = PipeType::PIPE_V;
    for (uint32_t i = 0; i < 100U; i++) {
        container.Push(e, e.pipe, 0);
    }

    e.pipe = PipeType::PIPE_MTE1;
    for (uint32_t i = 0; i < 100U; i++) {
        container.Push(e, e.pipe, 0);
    }

    ASSERT_EQ(container.GetAllQueSize(), 300U);

    container.SetQueIndex(PipeType::PIPE_M);
    for (uint32_t i = 0; i < 100U; i++) {
        if (container.IsCurQueEmpty()) {
            break;
        }

        e = container.Front();
        ASSERT_EQ(e.pipe, PipeType::PIPE_M);
        container.Pop();
    }

    container.SetQueIndex(PipeType::PIPE_V);
    for (uint32_t i = 0; i < 100U; i++) {
        if (container.IsCurQueEmpty()) {
            break;
        }

        e = container.Front();
        ASSERT_EQ(e.pipe, PipeType::PIPE_V);
        container.Pop();
    }

    container.SetQueIndex(PipeType::PIPE_MTE1);
    for (uint32_t i = 0; i < 100U; i++) {
        if (container.IsCurQueEmpty()) {
            break;
        }

        e = container.Front();
        ASSERT_EQ(e.pipe, PipeType::PIPE_MTE1);
        container.Pop();
    }

    ASSERT_TRUE(container.IsEmpty());
}
