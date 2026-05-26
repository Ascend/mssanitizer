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

#include "core/framework/vector_clock.h"

using namespace Sanitizer;

TEST(VectorClock, update_logic_time_expect_success)
{
    VectorTime vt1;
    vt1.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    for (uint8_t i = 0; i < static_cast<uint8_t>(PipeType::SIZE); i++) {
        VectorClock::UpdateLogicTime(vt1, i);
    }

    VectorTime vt2;
    vt2.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    for (uint8_t i = 0; i < static_cast<uint8_t>(PipeType::SIZE); i++) {
        vt2[i]++;
    }

    ASSERT_TRUE(VectorClock::IsEqual(vt1, vt2));
}

TEST(VectorClock, update_vector_time_expect_success)
{
    VectorTime vt1;
    vt1.resize(static_cast<uint8_t>(PipeType::SIZE), 1);

    VectorTime vt2;
    vt2.resize(static_cast<uint8_t>(PipeType::SIZE), 3);

    VectorClock::UpdateVectorTime(vt2, vt1);

    ASSERT_TRUE(VectorClock::IsEqual(vt1, vt2));
}

TEST(VectorClock, check_is_happens_before_expect_success)
{
    VectorTime vt1;
    vt1.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    VectorClock::UpdateLogicTime(vt1, static_cast<uint8_t>(PipeType::PIPE_M));

    VectorTime vt2;
    vt2.resize(static_cast<uint8_t>(PipeType::SIZE), 2);
    ASSERT_FALSE(VectorClock::IsNotHappensBefore(vt1, vt2, static_cast<uint8_t>(PipeType::PIPE_M),
        static_cast<uint8_t>(PipeType::PIPE_V)));
    VectorClock::UpdateLogicTime(vt1, static_cast<uint8_t>(PipeType::PIPE_M));
    ASSERT_TRUE(VectorClock::IsNotHappensBefore(vt1, vt2, static_cast<uint8_t>(PipeType::PIPE_M),
        static_cast<uint8_t>(PipeType::PIPE_V)));
}
