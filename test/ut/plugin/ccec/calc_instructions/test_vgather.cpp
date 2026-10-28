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
#include "plugin/ccec/calc_instructions/vgather.cpp"

using namespace Sanitizer;

TEST(VgatherCalcInstructions, dump_u32_type_records_and_parse_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    auto record1 = CreateRandomVgatherRecord();
    uint32_t offsetAddr = 0x0UL;
    uint16_t dstRepeatStride = 8U;
    uint8_t repeat = 1U;
    __sanitizer_report_vgather(memInfo.data(), record1.location.fileNo, record1.location.lineNo, record1.location.pc,
                reinterpret_cast<__ubuf__ uint32_t*>(record1.dst), reinterpret_cast<__ubuf__ uint32_t *>(record1.src),
                offsetAddr, dstRepeatStride, repeat);

    auto record = reinterpret_cast<VgatherRecord *>(memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) +
                                                    sizeof(RecordType));
    ASSERT_EQ(record->location, record1.location);
    ASSERT_EQ(record->dst, record1.dst);
    ASSERT_EQ(record->src, record1.src);
    ASSERT_EQ(record->dstBlockSize, 32);
    ASSERT_EQ(record->srcBlockSize, 32);
    ASSERT_EQ(record->offsetAddr, 0);
    ASSERT_EQ(record->dstRepeatStride, 8);
    ASSERT_EQ(record->dstBlockNum, 8);
    ASSERT_EQ(record->srcBlockNum, 8);
    ASSERT_EQ(record->dstAlignSize, 32);
    ASSERT_EQ(record->dstBlockStride, 1);
    ASSERT_EQ(record->dstRepeat, 1);
    ASSERT_EQ(record->dstDataBits, 32);
}

TEST(VgatherCalcInstructions, dump_u32_type_input_repeat_records_and_parse_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    auto record1 = CreateRandomVgatherRecord();
    uint16_t dstRepeatStride = 8U;
    uint8_t repeat = 2U;
    uint32_t offsetAddr = 0x0UL;
    __sanitizer_report_vgather(memInfo.data(), record1.location.fileNo, record1.location.lineNo, record1.location.pc,
                reinterpret_cast<__ubuf__ uint32_t*>(record1.dst), reinterpret_cast<__ubuf__ uint32_t *>(record1.src),
                offsetAddr, dstRepeatStride, repeat);

    auto record = reinterpret_cast<VgatherRecord *>(memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) +
                                                    sizeof(RecordType));
    ASSERT_EQ(record->location, record1.location);
    ASSERT_EQ(record->dst, record1.dst);
    ASSERT_EQ(record->src, record1.src);
    ASSERT_EQ(record->dstBlockSize, 32);
    ASSERT_EQ(record->srcBlockSize, 32);
    ASSERT_EQ(record->offsetAddr, 0);
    ASSERT_EQ(record->dstRepeatStride, 8);
    ASSERT_EQ(record->dstBlockNum, 8);
    ASSERT_EQ(record->srcBlockNum, 16);
    ASSERT_EQ(record->dstAlignSize, 32);
    ASSERT_EQ(record->dstBlockStride, 1);
    ASSERT_EQ(record->dstRepeat, 2);
    ASSERT_EQ(record->dstDataBits, 32);
}

TEST(VgatherCalcInstructions, dump_u16_type_records_and_parse_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    auto record1 = CreateRandomVgatherRecord();
    uint32_t offsetAddr = 0x0UL;
    uint16_t dstRepeatStride = 8U;
    uint8_t repeat = 1U;
    __sanitizer_report_vgather(memInfo.data(), record1.location.fileNo, record1.location.lineNo, record1.location.pc,
                reinterpret_cast<__ubuf__ uint16_t*>(record1.dst), reinterpret_cast<__ubuf__ uint32_t *>(record1.src),
                offsetAddr, dstRepeatStride, repeat);

    auto record = reinterpret_cast<VgatherRecord *>(memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) +
                                                    sizeof(RecordType));
    ASSERT_EQ(record->location, record1.location);
    ASSERT_EQ(record->dst, record1.dst);
    ASSERT_EQ(record->src, record1.src);
    ASSERT_EQ(record->dstBlockSize, 32);
    ASSERT_EQ(record->srcBlockSize, 32);
    ASSERT_EQ(record->offsetAddr, 0);
    ASSERT_EQ(record->dstRepeatStride, 8);
    ASSERT_EQ(record->dstBlockNum, 8);
    ASSERT_EQ(record->srcBlockNum, 16);
    ASSERT_EQ(record->dstAlignSize, 32);
    ASSERT_EQ(record->dstBlockStride, 1);
    ASSERT_EQ(record->dstRepeat, 1);
    ASSERT_EQ(record->dstDataBits, 16);
}

TEST(VgatherCalcInstructions, dump_u16_type_input_repeat_records_and_parse_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    auto record1 = CreateRandomVgatherRecord();
    uint16_t dstRepeatStride = 8;
    uint8_t repeat = 2;
    uint32_t offsetAddr = 0x0UL;
    __sanitizer_report_vgather(memInfo.data(), record1.location.fileNo, record1.location.lineNo, record1.location.pc,
                reinterpret_cast<__ubuf__ uint16_t*>(record1.dst), reinterpret_cast<__ubuf__ uint32_t *>(record1.src),
                offsetAddr, dstRepeatStride, repeat);

    auto record = reinterpret_cast<VgatherRecord *>(memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) +
                                                    sizeof(RecordType));
    ASSERT_EQ(record->location, record1.location);
    ASSERT_EQ(record->dst, record1.dst);
    ASSERT_EQ(record->src, record1.src);
    ASSERT_EQ(record->dstBlockSize, 32);
    ASSERT_EQ(record->srcBlockSize, 32);
    ASSERT_EQ(record->offsetAddr, 0);
    ASSERT_EQ(record->dstRepeatStride, 8);
    ASSERT_EQ(record->dstBlockNum, 8);
    ASSERT_EQ(record->srcBlockNum, 32);
    ASSERT_EQ(record->dstAlignSize, 32);
    ASSERT_EQ(record->dstBlockStride, 1);
    ASSERT_EQ(record->dstRepeat, 2);
    ASSERT_EQ(record->dstDataBits, 16);
}

TEST(VgatherCalcInstructions, dump_u16_type_input_config_records_and_parse_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    auto record1 = CreateRandomVgatherRecord();
    uint64_t dstRepeatStride = 8;
    uint64_t repeat = 2;
    uint64_t offsetAddr = 28888;
    uint64_t config = ((offsetAddr & 0xffffffff) | (dstRepeatStride & 0xff) << 32 |
        (dstRepeatStride & 0xf00) << 44 | (repeat & 0xff) << 56);
    __sanitizer_report_vgather(memInfo.data(), record1.location.fileNo, record1.location.lineNo, record1.location.pc,
                reinterpret_cast<__ubuf__ uint16_t*>(record1.dst), reinterpret_cast<__ubuf__ uint32_t *>(record1.src),
                config);

    auto record = reinterpret_cast<VgatherRecord *>(memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) +
                                                    sizeof(RecordType));
    ASSERT_EQ(record->location, record1.location);
    ASSERT_EQ(record->dst, record1.dst);
    ASSERT_EQ(record->src, record1.src);
    ASSERT_EQ(record->dstBlockSize, 32);
    ASSERT_EQ(record->srcBlockSize, 32);
    ASSERT_EQ(record->offsetAddr, 28888);
    ASSERT_EQ(record->dstRepeatStride, 8);
    ASSERT_EQ(record->dstBlockNum, 8);
    ASSERT_EQ(record->srcBlockNum, 32);
    ASSERT_EQ(record->dstAlignSize, 32);
    ASSERT_EQ(record->dstBlockStride, 1);
    ASSERT_EQ(record->dstRepeat, 2);
    ASSERT_EQ(record->dstDataBits, 16);
}

TEST(VgatherCalcInstructions, dump_u32_type_input_config_records_and_parse_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    auto record1 = CreateRandomVgatherRecord();
    uint64_t dstRepeatStride = 8;
    uint64_t repeat = 2;
    uint64_t offsetAddr = 28888;
    uint64_t config = ((offsetAddr & 0xffffffff) | ((dstRepeatStride & 0xff) << 32) |
        ((dstRepeatStride & 0xf00) << 44) | ((repeat & 0xff) << 56));
    __sanitizer_report_vgather(memInfo.data(), record1.location.fileNo, record1.location.lineNo, record1.location.pc,
                reinterpret_cast<__ubuf__ uint32_t*>(record1.dst), reinterpret_cast<__ubuf__ uint32_t *>(record1.src),
                config);

    auto record = reinterpret_cast<VgatherRecord *>(memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) +
                                                    sizeof(RecordType));
    ASSERT_EQ(record->location, record1.location);
    ASSERT_EQ(record->dst, record1.dst);
    ASSERT_EQ(record->src, record1.src);
    ASSERT_EQ(record->dstBlockSize, 32);
    ASSERT_EQ(record->srcBlockSize, 32);
    ASSERT_EQ(record->offsetAddr, 28888);
    ASSERT_EQ(record->dstRepeatStride, 8);
    ASSERT_EQ(record->dstBlockNum, 8);
    ASSERT_EQ(record->srcBlockNum, 16);
    ASSERT_EQ(record->dstAlignSize, 32);
    ASSERT_EQ(record->dstBlockStride, 1);
    ASSERT_EQ(record->dstRepeat, 2);
    ASSERT_EQ(record->dstDataBits, 32);
}

TEST(VgatherCalcInstructions, dump_repeat_stride_mode_overloads_expect_success) {
    auto record1 = CreateRandomVgatherRecord();
    auto run = [&](auto dstType) {
        using Dst = decltype(dstType);
        std::vector<uint8_t> memInfo = CreateMemInfo();
        __sanitizer_report_vgather(memInfo.data(), record1.location.fileNo, record1.location.lineNo, record1.location.pc,
            reinterpret_cast<__ubuf__ Dst *>(record1.dst), reinterpret_cast<__ubuf__ uint32_t *>(record1.src),
            0U, true);
        auto record = reinterpret_cast<VgatherRecord *>(memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) +
                                                        sizeof(RecordType));
        EXPECT_EQ(record->dst, record1.dst);
        EXPECT_EQ(record->src, record1.src);
    };
    run(uint16_t{});
    run(uint32_t{});
}
