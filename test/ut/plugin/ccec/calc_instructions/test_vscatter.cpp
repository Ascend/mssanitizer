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
#include "plugin/ccec/calc_instructions/vscatter.cpp"

using namespace Sanitizer;

TEST(VscatterCalcInstructions, dump_u32_records_and_parse_each_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    auto record = CreateRandomUnaryOpRecord();

    uint64_t config = ExtractConfigFromUnaryOpRecordWithOffset(record, 0);
    __sanitizer_report_vscatter(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                reinterpret_cast<__ubuf__ uint32_t*>(record.dst),
                                reinterpret_cast<__ubuf__ uint16_t *>(record.src), config);
    __sanitizer_report_vscatter(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                reinterpret_cast<__ubuf__ uint32_t*>(record.dst),
                                reinterpret_cast<__ubuf__ uint16_t *>(record.src), config);

    UnaryOpRecord const *unaryOp = reinterpret_cast<UnaryOpRecord const*>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType) * 2 + sizeof(UnaryOpRecord) * 1);
    EXPECT_EQ(unaryOp->location.fileNo, record.location.fileNo);
    EXPECT_EQ(unaryOp->location.lineNo, record.location.lineNo);
    EXPECT_EQ(unaryOp->location.pc, record.location.pc);
    EXPECT_EQ(unaryOp->dst, record.dst);
    EXPECT_EQ(unaryOp->src, record.src);
    EXPECT_EQ(unaryOp->srcRepeatStride, record.srcRepeatStride);
    EXPECT_EQ(unaryOp->dstRepeatStride, 1);
    EXPECT_EQ(unaryOp->srcBlockNum, 8);
    EXPECT_EQ(unaryOp->dstBlockNum, 8);
    EXPECT_EQ(unaryOp->srcBlockSize, 32);
    EXPECT_EQ(unaryOp->dstBlockSize, 32);
}

TEST(VscatterCalcInstructions, dump_explicit_params_overloads_expect_success) {
    auto record = CreateRandomUnaryOpRecord();
    auto run = [&](auto srcType) {
        using Src = decltype(srcType);
        std::vector<uint8_t> memInfo = CreateMemInfo();
        __sanitizer_report_vscatter(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
            reinterpret_cast<__ubuf__ uint32_t *>(record.dst), reinterpret_cast<__ubuf__ Src *>(record.src),
            0U, false, false, 1U, 1U);
        auto unaryOp = reinterpret_cast<UnaryOpRecord const *>(memInfo.data() + sizeof(RecordGlobalHead) +
            sizeof(RecordBlockHead) + sizeof(RecordType));
        EXPECT_EQ(unaryOp->dst, record.dst);
        EXPECT_EQ(unaryOp->src, record.src);
    };
    run(uint16_t{});
    run(uint32_t{});
}

TEST(VscatterCalcInstructions, dump_u16_records_and_parse_each_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    auto record = CreateRandomUnaryOpRecord();

    uint64_t config = ExtractConfigFromUnaryOpRecordWithOffset(record, 0);
    __sanitizer_report_vscatter(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                reinterpret_cast<__ubuf__ uint32_t*>(record.dst),
                                reinterpret_cast<__ubuf__ uint32_t *>(record.src), config);
    __sanitizer_report_vscatter(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                reinterpret_cast<__ubuf__ uint32_t*>(record.dst),
                                reinterpret_cast<__ubuf__ uint32_t *>(record.src), config);

    UnaryOpRecord const *unaryOp = reinterpret_cast<UnaryOpRecord const*>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType) * 2 + sizeof(UnaryOpRecord) * 1);
    EXPECT_EQ(unaryOp->location.fileNo, record.location.fileNo);
    EXPECT_EQ(unaryOp->location.lineNo, record.location.lineNo);
    EXPECT_EQ(unaryOp->location.pc, record.location.pc);
    EXPECT_EQ(unaryOp->dst, record.dst);
    EXPECT_EQ(unaryOp->srcRepeatStride, record.srcRepeatStride);
    EXPECT_EQ(unaryOp->dstRepeatStride, 1);
    EXPECT_EQ(unaryOp->srcBlockNum, 8);
    EXPECT_EQ(unaryOp->dstBlockNum, 8);
    EXPECT_EQ(unaryOp->srcBlockSize, 32);
    EXPECT_EQ(unaryOp->dstBlockSize, 32);
}
