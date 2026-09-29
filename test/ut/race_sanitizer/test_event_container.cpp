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

// 场景：同一对象跨 kernel 复用，kernel1 队列里还留着未处理事件。
// 预期：第二次 Init 后队列整体清空（IsEmpty()==true、GetAllQueSize()==0）。
TEST(EventContainer, reinit_shall_clear_leftover_events_across_kernels)
{
    EventContainer container;
    container.Init(1);
    SanEvent e;
    e.pipe = PipeType::PIPE_S;
    container.Push(e, e.pipe, 0);
    ASSERT_FALSE(container.IsEmpty());

    // 同一个对象进入下一个 kernel
    container.Init(1);
    ASSERT_TRUE(container.IsEmpty());
    ASSERT_EQ(container.GetAllQueSize(), 0U);
}

// 场景：kernel1 以“所有 device 卡死”收尾后，同一对象再 Init 进入 kernel2。
// 预期：重走一遍卡死判定仍能到达 IsAllDeviceStuck()（未复位时 stuckDeviceNum_ 会越过 deviceNum_，Run() 死循环）。
TEST(EventContainer, reinit_shall_reset_stuck_state_across_kernels)
{
    EventContainer container;
    container.Init(1);
    SanEvent e;
    e.pipe = PipeType::PIPE_S;
    container.Push(e, e.pipe, 0);

    // kernel1 以“所有 device 卡死”收尾：一轮 block 零出队
    container.SwitchToNextBlock();
    container.CheckCurDeviceStuck();
    ASSERT_TRUE(container.IsAllDeviceStuck());

    // kernel2 复用同一对象后，重新走一遍卡死判定必须仍能到达 IsAllDeviceStuck()
    container.Init(1);
    container.SwitchToNextBlock();
    container.CheckCurDeviceStuck();
    ASSERT_TRUE(container.IsAllDeviceStuck());
}
