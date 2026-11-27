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

#include "plugin/record_move_instructions.h"

using namespace Sanitizer;

namespace {
// Ascend950 / 910b 各 quantPRE 对应的输出位宽，未识别模式回退 32bit
constexpr uint16_t ASCEND950_QUANT_BITS[] = {
    32, // 0  NoQuant
    16, // 1  F322F16
    8,  // 2  VQF322HIF8_PRE
    8,  // 3  QF322HIF8_PRE
    8,  // 4  VQF322HIF8_PRE_HYBRID
    8,  // 5  QF322HIF8_PRE_HYBRID
    32, // 6  VDEQS32_INT
    32, // 7  DEQS32_INT
    8,  // 8  VREQ8
    8,  // 9  REQ8
    16, // 10 VDEQF16
    16, // 11 DEQF16
    8,  // 12 VQF322FP8_PRE
    8,  // 13 QF322FP8_PRE
    32, // 14 VQF322F32_PRE
    32, // 15 QF322F32_PRE
    16, // 16 F322BF16
    32, // 17 VQF162B8_PRE
    32, // 18 QF162B8_PRE
    32, // 19 VQF162S4_PRE
    32, // 20 QF162S4_PRE
    4,  // 21 VREQ4
    4,  // 22 REQ4
    8,  // 23 VQF322B8_PRE
    8,  // 24 QF322B8_PRE
    4,  // 25 VQF322S4_PRE
    4,  // 26 QF322S4_PRE
    32, // 27 VDEQS16
    32, // 28 DEQS16
    32, // 29 VQF162S16_PRE
    32, // 30 QF162S16_PRE
    16, // 31 VQF322F16_PRE
    16, // 32 QF322F16_PRE
    16, // 33 VQF322BF16_PRE
    16, // 34 QF322BF16_PRE
    16, // 35 VQS322BF16_PRE
    16, // 36 QS322BF16_PRE
};

constexpr uint16_t A2A3_QUANT_BITS[] = {
    32, // 0
    16, // 1  F322F16
    32, // 2
    32, // 3
    32, // 4
    32, // 5
    16, // 6  VDEQS32_INT
    32, // 7
    8,  // 8  VREQ8
    8,  // 9  REQ8
    16, // 10 VDEQF16
    16, // 11 DEQF16
    16, // 12 VSHIFTS322S16
    16, // 13 SHIFTS322S16
    32, // 14
    32, // 15
    16, // 16 F322BF16
    32, // 17
    32, // 18
    32, // 19
    32, // 20
    4,  // 21 VREQ4
    4,  // 22 REQ4
    8,  // 23 VQF322B8_PRE
    8,  // 24 QF322B8_PRE
    4,  // 25 VQF322S4_PRE
    4,  // 26 QF322S4_PRE
    32, // 27
    32, // 28
    32, // 29
    32, // 30
    32, // 31
    32, // 32
    32, // 33
    32, // 34
    32, // 35
    32, // 36
};

template <size_t N>
void ExpectQuantBits(const uint16_t (&expect)[N], bool isAscend950)
{
    for (uint64_t quantPRE = 0; quantPRE < N; ++quantPRE) {
        MovFpRecord record {};
        ParseMovFpQuantBits(quantPRE, true, isAscend950, record);
        EXPECT_EQ(record.quantPreBits, expect[quantPRE]) << "quantPRE=" << quantPRE << " isAscend950=" << isAscend950;
    }
}
} // namespace

TEST(FixpipeQuantBits, Ascend950QuantModeEnum)
{
    ExpectQuantBits(ASCEND950_QUANT_BITS, true);
}

TEST(FixpipeQuantBits, A2A3QuantModeEnum)
{
    ExpectQuantBits(A2A3_QUANT_BITS, false);
}

TEST(FixpipeQuantBits, UnknownQuantModeFallsBackTo32)
{
    for (bool isAscend950 : {true, false}) {
        for (uint64_t quantPRE = 37; quantPRE <= 63; ++quantPRE) {
            MovFpRecord record {};
            ParseMovFpQuantBits(quantPRE, true, isAscend950, record);
            EXPECT_EQ(record.quantPreBits, 32) << "quantPRE=" << quantPRE << " isAscend950=" << isAscend950;
        }
    }
}

TEST(FixpipeQuantBits, ChannelMergeFollowsEnNDorDN)
{
    for (uint64_t quantPRE : {uint64_t(8), uint64_t(23)}) {
        MovFpRecord record {};
        ParseMovFpQuantBits(quantPRE, true, true, record);
        EXPECT_FALSE(record.int8ChannelMerge) << "quantPRE=" << quantPRE;
        ParseMovFpQuantBits(quantPRE, false, true, record);
        EXPECT_TRUE(record.int8ChannelMerge) << "quantPRE=" << quantPRE;
    }
    for (uint64_t quantPRE : {uint64_t(21), uint64_t(25)}) {
        MovFpRecord record {};
        ParseMovFpQuantBits(quantPRE, true, true, record);
        EXPECT_FALSE(record.int4ChannelMerge) << "quantPRE=" << quantPRE;
        ParseMovFpQuantBits(quantPRE, false, true, record);
        EXPECT_TRUE(record.int4ChannelMerge) << "quantPRE=" << quantPRE;
    }
}
