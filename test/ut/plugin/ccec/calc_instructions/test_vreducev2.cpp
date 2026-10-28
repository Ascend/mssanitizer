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
#include "ccec_defs.h"
#include "ccec/data_process.h"
#include "plugin/ccec/calc_instructions/vreducev2.cpp"

using namespace Sanitizer;

namespace {
uint64_t BuildVreducev2Config(ReduceV2Record const &record)
{
    return ((static_cast<uint64_t>(record.repeat >> 8) & 0xFF) << 56) |
        (static_cast<uint64_t>(record.src1RepeatStride & 0xFFFF) << 40) |
        (static_cast<uint64_t>(record.src0RepeatStride & 0xFFFF) << 24) |
        (static_cast<uint64_t>(record.patternMode & 0xFF) << 16) |
        (static_cast<uint64_t>(record.src0BlockStride & 0xFF) << 8) | (record.repeat & 0xFF);
}

ReduceV2Record MakeVreducev2Record()
{
    auto record = CreateRandomReduceV2Record();
    static uint64_t ubuf[2] = {0x5555555555555555UL, 0xAAAAAAAAAAAAAAAAUL};
    record.patternMode = 0;
    record.compareMask.mask1 = ubuf[0];
    record.compareMask.mask0 = ubuf[1];
    record.src1 = reinterpret_cast<uint64_t>(ubuf);
    return record;
}
}  // namespace

TEST(Vreducev2CalcInstructions, vreducev2_dump_records_and_parse_last_expect_success_1)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    __gm__ RecordBlockHead *recordBlockHead = reinterpret_cast<__gm__ RecordBlockHead *>(memInfo.data() + sizeof(RecordGlobalHead));

    auto record = MakeVreducev2Record();

    __sanitizer_report_vreducev2(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
        reinterpret_cast<__ubuf__ uint16_t *>(record.dst), reinterpret_cast<__ubuf__ uint16_t *>(record.src0),
        reinterpret_cast<__ubuf__ uint16_t *>(record.src1), record.repeat, record.src0BlockStride, record.patternMode,
        record.src0RepeatStride, record.src1RepeatStride);
    __sanitizer_report_vreducev2(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
        reinterpret_cast<__ubuf__ uint16_t *>(record.dst), reinterpret_cast<__ubuf__ uint16_t *>(record.src0),
        reinterpret_cast<__ubuf__ uint16_t *>(record.src1), record.repeat, record.src0BlockStride, record.patternMode,
        record.src0RepeatStride, record.src1RepeatStride);

    ReduceV2Record const &got = *reinterpret_cast<ReduceV2Record const *>
        (memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) + sizeof(RecordType) * 2 + sizeof(ReduceV2Record));

    ASSERT_EQ(recordBlockHead->recordCount, 2);
    ASSERT_EQ(record.dst, got.dst);
    ASSERT_EQ(record.src0, got.src0);
    ASSERT_EQ(record.src1, got.src1);
    ASSERT_EQ(record.repeat, got.repeat);
    ASSERT_EQ(record.src0BlockStride, got.src0BlockStride);
    ASSERT_EQ(record.src0RepeatStride, got.src0RepeatStride);
    ASSERT_EQ(record.src1RepeatStride, got.src1RepeatStride);
    ASSERT_EQ(record.patternMode, got.patternMode);
    ASSERT_EQ(record.compareMask.mask1, got.compareMask.mask1);
    ASSERT_EQ(record.compareMask.mask0, got.compareMask.mask0);
}

TEST(Vreducev2CalcInstructions, vreducev2_all_overloads_expect_success)
{
    auto record = MakeVreducev2Record();
    uint64_t config = BuildVreducev2Config(record);

    auto run = [&](auto dstType, auto srcType) {
        using Dst = decltype(dstType);
        using Src = decltype(srcType);
        std::vector<uint8_t> memInfo = CreateMemInfo();
        __sanitizer_report_vreducev2(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
            reinterpret_cast<__ubuf__ Dst *>(record.dst), reinterpret_cast<__ubuf__ Src *>(record.src0),
            reinterpret_cast<__ubuf__ Src *>(record.src1), config);
        __sanitizer_report_vreducev2(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
            reinterpret_cast<__ubuf__ Dst *>(record.dst), reinterpret_cast<__ubuf__ Src *>(record.src0),
            reinterpret_cast<__ubuf__ Src *>(record.src1), record.repeat, record.src0BlockStride, record.patternMode,
            record.src0RepeatStride, record.src1RepeatStride);
        __gm__ RecordBlockHead *head = reinterpret_cast<__gm__ RecordBlockHead *>(memInfo.data() + sizeof(RecordGlobalHead));
        EXPECT_EQ(head->recordCount, 2);
    };

    run(uint16_t{}, uint16_t{});
    run(uint32_t{}, uint32_t{});
}
