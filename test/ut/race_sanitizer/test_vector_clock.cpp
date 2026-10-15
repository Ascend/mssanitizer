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

TEST(VectorClock, update_vector_time_expect_unsigned_semantics)
{
    // 逐元素无符号 max：高位为 1 的值必须按无符号序比较（SIMD 路径的符号处理）
    VectorTime in = {0xFFFFFFFFu, 1u, 0x80000000u, 0x7FFFFFFFu};
    VectorTime out = {1u, 0xFFFFFFFFu, 0x7FFFFFFFu, 0x80000000u};
    VectorClock::UpdateVectorTime(in, out);
    ASSERT_EQ(out[0], 0xFFFFFFFFu);
    ASSERT_EQ(out[1], 0xFFFFFFFFu);
    ASSERT_EQ(out[2], 0x80000000u);
    ASSERT_EQ(out[3], 0x80000000u);
}

TEST(VectorClock, update_vector_time_expect_min_size_and_tail)
{
    // 长度不一致时只更新前 min(in, out) 个元素
    VectorTime inLong = {9u, 9u, 9u, 9u, 9u, 9u, 9u};
    VectorTime outShort = {1u, 2u, 3u, 4u, 5u};
    VectorClock::UpdateVectorTime(inLong, outShort);
    VectorTime expect = {9u, 9u, 9u, 9u, 9u};
    ASSERT_TRUE(VectorClock::IsEqual(outShort, expect));

    // 长度非向量宽度整数倍，覆盖 SIMD 尾部标量处理
    VectorTime in = VectorTime(10, 7u);
    VectorTime out = VectorTime(10, 0u);
    out[9] = 100u;
    VectorClock::UpdateVectorTime(in, out);
    for (std::size_t i = 0; i < 9; ++i) {
        ASSERT_EQ(out[i], 7u);
    }
    ASSERT_EQ(out[9], 100u);
}

TEST(VectorClock, update_vector_time_with_empty_expect_noop)
{
    VectorTime empty;
    VectorTime out = {1u, 2u, 3u};
    VectorClock::UpdateVectorTime(empty, out);
    VectorTime expect = {1u, 2u, 3u};
    ASSERT_TRUE(VectorClock::IsEqual(out, expect));

    VectorTime in = {1u, 2u, 3u};
    VectorTime outEmpty;
    VectorClock::UpdateVectorTime(in, outEmpty);
    ASSERT_TRUE(outEmpty.empty());
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
