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
#include <vector>

#define private public
#include "core/framework/record_parse_move.h"
#include "address_sanitizer/align_checker.h"
#undef private

namespace Sanitizer {
inline bool operator==(MemOpInfo const &lhs, MemOpInfo const &rhs)
{
    return lhs.memType == rhs.memType &&
        lhs.opType == rhs.opType &&
        lhs.maskMode == rhs.maskMode &&
        lhs.instrName == rhs.instrName &&
        lhs.dataBits == rhs.dataBits &&
        lhs.addr == rhs.addr &&
        lhs.blockNum == rhs.blockNum &&
        lhs.blockSize == rhs.blockSize &&
        lhs.blockStride == rhs.blockStride &&
        lhs.repeatTimes == rhs.repeatTimes &&
        lhs.repeatStride == rhs.repeatStride &&
        lhs.alignSize == rhs.alignSize;
}
}  // namespace Sanitizer

namespace {
using namespace Sanitizer;

static constexpr uint64_t CORE_ID = 7;

TEST(CeilByAlignSizeTest, template_version_returns_aligned_result)
{
    EXPECT_EQ(CeilByAlignSize<32>(0), 0);
    EXPECT_EQ(CeilByAlignSize<32>(1), 32);
    EXPECT_EQ(CeilByAlignSize<32>(31), 32);
    EXPECT_EQ(CeilByAlignSize<32>(32), 32);
    EXPECT_EQ(CeilByAlignSize<32>(33), 64);
    EXPECT_EQ(CeilByAlignSize<32>(100), 128);

    EXPECT_EQ(CeilByAlignSize<16>(0), 0);
    EXPECT_EQ(CeilByAlignSize<16>(1), 16);
    EXPECT_EQ(CeilByAlignSize<16>(15), 16);
    EXPECT_EQ(CeilByAlignSize<16>(16), 16);
    EXPECT_EQ(CeilByAlignSize<16>(17), 32);

    EXPECT_EQ(CeilByAlignSize<8>(7), 8);
    EXPECT_EQ(CeilByAlignSize<8>(8), 8);
    EXPECT_EQ(CeilByAlignSize<8>(9), 16);
}

TEST(CeilByAlignSizeTest, runtime_version_returns_aligned_result)
{
    EXPECT_EQ(CeilByAlignSize(0, 32), 0);
    EXPECT_EQ(CeilByAlignSize(1, 32), 32);
    EXPECT_EQ(CeilByAlignSize(31, 32), 32);
    EXPECT_EQ(CeilByAlignSize(32, 32), 32);
    EXPECT_EQ(CeilByAlignSize(33, 32), 64);
    EXPECT_EQ(CeilByAlignSize(100, 32), 128);

    EXPECT_EQ(CeilByAlignSize(0, 16), 0);
    EXPECT_EQ(CeilByAlignSize(1, 16), 16);
    EXPECT_EQ(CeilByAlignSize(15, 16), 16);
    EXPECT_EQ(CeilByAlignSize(16, 16), 16);
    EXPECT_EQ(CeilByAlignSize(17, 16), 32);

    EXPECT_EQ(CeilByAlignSize(7, 8), 8);
    EXPECT_EQ(CeilByAlignSize(8, 8), 8);
    EXPECT_EQ(CeilByAlignSize(9, 8), 16);
}

TEST(CeilByAlignSizeTest, zero_align_size_returns_zero)
{
    EXPECT_EQ(CeilByAlignSize(100, 0), 0);
    EXPECT_EQ(CeilByAlignSize(0, 0), 0);
}

TEST(ParseDmaMovWithByteModeTest, given_record_and_expect_correct_events)
{
    std::vector<SanEvent> events;
    DmaMovRecord dmaMovRecord{};
    dmaMovRecord.dst = 0xaa;
    dmaMovRecord.src = 0x55;
    dmaMovRecord.nBurst = 10;
    dmaMovRecord.lenBurst = 32;
    dmaMovRecord.srcStride = 8;
    dmaMovRecord.dstStride = 4;
    dmaMovRecord.location.blockId = CORE_ID;
    dmaMovRecord.srcMemType = MemType::UB;
    dmaMovRecord.dstMemType = MemType::GM;
    dmaMovRecord.padMode = PadMode::PAD_NONE;
    dmaMovRecord.byteMode = ByteMode::BM_ENABLE;

    SanEvent event;
    event.loc.coreId = CORE_ID;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE3;

    ParseDmaMovWithByteMode(dmaMovRecord, events, event);

    ASSERT_EQ(events.size(), 2);

    ASSERT_EQ(events[0].loc.coreId, CORE_ID);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE3);
    ASSERT_EQ(events[0].eventInfo.memInfo.memType, MemType::UB);
    ASSERT_EQ(events[0].eventInfo.memInfo.opType, AccessType::READ);
    ASSERT_EQ(events[0].eventInfo.memInfo.addr, 0x55);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockNum, 32U);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockSize, 1U);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockStride, 1U);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatTimes, 10U);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatStride, 32U + 8U * MOV_LOCAL_BLOCK_SIZE);

    ASSERT_EQ(events[1].loc.coreId, CORE_ID);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_MTE3);
    ASSERT_EQ(events[1].eventInfo.memInfo.memType, MemType::GM);
    ASSERT_EQ(events[1].eventInfo.memInfo.opType, AccessType::WRITE);
    ASSERT_EQ(events[1].eventInfo.memInfo.addr, 0xaa);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockNum, 32U);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockSize, 1U);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockStride, 1U);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatTimes, 10U);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatStride, 32U + 4U * MOV_LOCAL_BLOCK_SIZE);
}

TEST(ParseDmaMovWithPadModeTest, pad_mode1_returns_correct_events)
{
    std::vector<SanEvent> events;
    DmaMovRecord dmaMovRecord{};
    dmaMovRecord.dst = 0xaa;
    dmaMovRecord.src = 0x55;
    dmaMovRecord.nBurst = 32;
    dmaMovRecord.lenBurst = 1;
    dmaMovRecord.srcStride = 0;
    dmaMovRecord.dstStride = 8;
    dmaMovRecord.location.blockId = CORE_ID;
    dmaMovRecord.srcMemType = MemType::UB;
    dmaMovRecord.dstMemType = MemType::GM;
    dmaMovRecord.padMode = PadMode::PAD_MODE1;
    dmaMovRecord.byteMode = ByteMode::BM_DISABLE;

    SanEvent event;
    event.loc.coreId = CORE_ID;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE3;

    ParseDmaMovWithPadMode(dmaMovRecord, events, event);

    ASSERT_EQ(events.size(), 2);

    ASSERT_EQ(events[0].loc.coreId, CORE_ID);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].eventInfo.memInfo.memType, MemType::UB);
    ASSERT_EQ(events[0].eventInfo.memInfo.opType, AccessType::READ);
    ASSERT_EQ(events[0].eventInfo.memInfo.addr, 0x55);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockNum, 1U);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockSize, 32U);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatTimes, 1U);

    ASSERT_EQ(events[1].loc.coreId, CORE_ID);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].eventInfo.memInfo.memType, MemType::GM);
    ASSERT_EQ(events[1].eventInfo.memInfo.opType, AccessType::WRITE);
    ASSERT_EQ(events[1].eventInfo.memInfo.addr, 0xaa);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockSize, MOV_LOCAL_BLOCK_SIZE);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatTimes, 32U);
}

TEST(ParseDmaMovWithPadModeTest, pad_mode6_returns_correct_events)
{
    std::vector<SanEvent> events;
    DmaMovRecord dmaMovRecord{};
    dmaMovRecord.dst = 0xaa;
    dmaMovRecord.src = 0x55;
    dmaMovRecord.nBurst = 32;
    dmaMovRecord.lenBurst = 1;
    dmaMovRecord.srcStride = 8;
    dmaMovRecord.dstStride = 0;
    dmaMovRecord.location.blockId = CORE_ID;
    dmaMovRecord.srcMemType = MemType::GM;
    dmaMovRecord.dstMemType = MemType::UB;
    dmaMovRecord.padMode = PadMode::PAD_MODE6;
    dmaMovRecord.byteMode = ByteMode::BM_DISABLE;

    SanEvent event;
    event.loc.coreId = CORE_ID;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE2;

    ParseDmaMovWithPadMode(dmaMovRecord, events, event);

    ASSERT_EQ(events.size(), 2);

    ASSERT_EQ(events[0].loc.coreId, CORE_ID);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].eventInfo.memInfo.memType, MemType::GM);
    ASSERT_EQ(events[0].eventInfo.memInfo.opType, AccessType::READ);
    ASSERT_EQ(events[0].eventInfo.memInfo.addr, 0x55);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockNum, 1U);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockSize, MOV_LOCAL_BLOCK_SIZE);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatTimes, 32U);

    ASSERT_EQ(events[1].loc.coreId, CORE_ID);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].eventInfo.memInfo.memType, MemType::UB);
    ASSERT_EQ(events[1].eventInfo.memInfo.opType, AccessType::WRITE);
    ASSERT_EQ(events[1].eventInfo.memInfo.addr, 0xaa);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockNum, 1U);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatTimes, 1U);
}

TEST(ParseDmaMovWithPadModeTest, pad_none_returns_no_events)
{
    std::vector<SanEvent> events;
    DmaMovRecord dmaMovRecord{};
    dmaMovRecord.dst = 0xaa;
    dmaMovRecord.src = 0x55;
    dmaMovRecord.nBurst = 10;
    dmaMovRecord.lenBurst = 8;
    dmaMovRecord.srcStride = 8;
    dmaMovRecord.dstStride = 8;
    dmaMovRecord.location.blockId = CORE_ID;
    dmaMovRecord.srcMemType = MemType::UB;
    dmaMovRecord.dstMemType = MemType::GM;
    dmaMovRecord.padMode = PadMode::PAD_NONE;
    dmaMovRecord.byteMode = ByteMode::BM_DISABLE;

    SanEvent event;
    event.loc.coreId = CORE_ID;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE3;

    ParseDmaMovWithPadMode(dmaMovRecord, events, event);

    ASSERT_EQ(events.size(), 0);
}
}  // namespace
