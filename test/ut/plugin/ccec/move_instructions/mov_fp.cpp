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


#include <utility>
#include <gtest/gtest.h>

#include "../data_process.h"
#include "../../ccec_defs.h"
#include "plugin/ccec/move_instructions/mov_fp.cpp"

namespace {
using namespace Sanitizer;

MovFpRecord CreateRandomMovFpRecord()
{
    MovFpRecord record{};
    record.dst = RandInt(0, 0x1000000);
    record.src = RandInt(0, 0x1000000);
    record.location.fileNo = RandInt(0, 0x100000);
    record.location.lineNo = RandInt(0, 0x1000);
    record.dstStride = RandInt(0, 0xFFFFFFF);
    record.srcStride = RandInt(0, 0xFFFF);
    record.nSize = RandInt(0, 0xFFF);
    record.mSize = RandInt(0, 0xFFF);
    record.ndNum = 0;
    record.dstNdStride = 0;
    record.srcNdStride = 0;
    record.srcNzC0Stride = 0;
    record.quantPreBits = 32; // f32/s32都是32bits
    record.enUnitFlag = false;
    record.int8ChannelMerge = false;
    record.int4ChannelMerge = false;
    record.channelSplit = false;
    record.enNZ2ND = false;
    record.enNZ2DN = false;
    return record;
}

void ExtractConfigFromMovFpRecord(const MovFpRecord &record, uint64_t &xm, uint64_t &xt)
{
    constexpr uint64_t mask12Bit = 0xfffUL;
    constexpr uint64_t mask16Bit = 0xffffUL;
    constexpr uint64_t mask32Bit = 0xffffffffUL;

    constexpr uint64_t nSizeShift = 4;
    constexpr uint64_t mSizeShift = 16;
    constexpr uint64_t dstStrideShift = 32;
    constexpr uint64_t channelSplitShift = 42;
    constexpr uint64_t enNZ2NDShift = 43;

    xm = (((record.nSize & mask12Bit) << nSizeShift) | ((record.mSize & mask16Bit) << mSizeShift) |
          ((record.dstStride & mask32Bit) << dstStrideShift));
    xt = (((record.srcStride & mask16Bit) << 0) | ((record.enUnitFlag & 0x3) << dstStrideShift) |
          ((record.channelSplit & 0x1) << channelSplitShift) | ((record.enNZ2ND & 0x1) << enNZ2NDShift));
}

TEST(MovFpInstructions, mov_fp_expect_get_correct_records)
{
    auto record = CreateRandomMovFpRecord();
    uint64_t xm;
    uint64_t xt;
    ExtractConfigFromMovFpRecord(record, xm, xt);
    uint8_t sid = 0;
    uint64_t quantPRE = 0;
    uint8_t reLUPRE = 0;
    std::vector<uint8_t> memInfo = CreateMemInfo();

    // #601, FIX_L0C_TO_OUT.f32
    __sanitizer_report_copy_matrix_cc_to_gm(memInfo.data(), record.location.fileNo, record.location.lineNo,
                                            record.location.pc, reinterpret_cast<__gm__ half *>(record.dst),
                                            reinterpret_cast<__cc__ float *>(record.src), xm, xt);
    // #608 -> /* #601 */
    __sanitizer_report_copy_matrix_cc_to_gm(memInfo.data(), record.location.fileNo, record.location.lineNo,
                                            record.location.pc, reinterpret_cast<__gm__ half *>(record.dst),
                                            reinterpret_cast<__cc__ float *>(record.src), sid, record.nSize,
                                            record.mSize, record.dstStride, record.srcStride, record.enUnitFlag,
                                            quantPRE, reLUPRE, record.channelSplit, record.enNZ2ND);
    // #651, FIX_L0C_TO_OUT.s32
    __sanitizer_report_copy_matrix_cc_to_gm(memInfo.data(), record.location.fileNo, record.location.lineNo,
                                            record.location.pc, reinterpret_cast<__gm__ int32_t *>(record.dst),
                                            reinterpret_cast<__cc__ int32_t *>(record.src), xm, xt);
    // #702 -> /* #651 */
    __sanitizer_report_copy_matrix_cc_to_gm(memInfo.data(), record.location.fileNo, record.location.lineNo,
                                            record.location.pc, reinterpret_cast<__gm__ int32_t *>(record.dst),
                                            reinterpret_cast<__cc__ int32_t *>(record.src), sid, record.nSize,
                                            record.mSize, record.dstStride, record.srcStride, record.enUnitFlag,
                                            quantPRE, reLUPRE, record.channelSplit, record.enNZ2ND);
    // #703, FIX_L0C_TO_OUT.f32
    __sanitizer_report_copy_matrix_cc_to_gm_b4(memInfo.data(), record.location.fileNo, record.location.lineNo,
                                               record.location.pc, reinterpret_cast<__gm__ void *>(record.dst),
                                               reinterpret_cast<__cc__ float *>(record.src), xm, xt);
    // #704 -> /* #703 */
    __sanitizer_report_copy_matrix_cc_to_gm_b4(memInfo.data(), record.location.fileNo, record.location.lineNo,
                                               record.location.pc, reinterpret_cast<__gm__ void *>(record.dst),
                                               reinterpret_cast<__cc__ float *>(record.src), sid, record.nSize,
                                               record.mSize, record.dstStride, record.srcStride, record.enUnitFlag,
                                               quantPRE, reLUPRE, record.channelSplit, record.enNZ2ND);

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_FP>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(MovFpRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_FP>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(MovFpRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_FP>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(MovFpRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_FP>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(MovFpRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_FP>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(MovFpRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_FP>(ptr, record));
}

TEST(MovFpInstructions, mov_fp_remaining_overloads_expect_record)
{
    auto record = CreateRandomMovFpRecord();
    uint64_t xm, xt;
    ExtractConfigFromMovFpRecord(record, xm, xt);
    uint8_t sid = 0, reLUPRE = 0;
    uint64_t quantPRE = 0;
    auto &loc = record.location;
    auto call = [&](auto &&func) {
        std::vector<uint8_t> memInfo = CreateMemInfo();
        func(memInfo.data());
        uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
        EXPECT_EQ(*reinterpret_cast<RecordType *>(ptr), RecordType::MOV_FP);
    };
    auto s = [&](auto *d, auto *src) { call([&](uint8_t *m) { __sanitizer_report_copy_matrix_cc_to_gm(m, loc.fileNo, loc.lineNo, loc.pc, d, src, xm, xt); }); };
    auto f = [&](auto *d, auto *src) { call([&](uint8_t *m) { __sanitizer_report_copy_matrix_cc_to_gm(m, loc.fileNo, loc.lineNo, loc.pc, d, src, sid, record.nSize, record.mSize, record.dstStride, record.srcStride, record.enUnitFlag, quantPRE, reLUPRE, record.channelSplit, record.enNZ2ND); }); };
    auto *sf = reinterpret_cast<__cc__ float *>(record.src);
    auto *si = reinterpret_cast<__cc__ int32_t *>(record.src);
    s(reinterpret_cast<__gm__ float *>(record.dst), sf); f(reinterpret_cast<__gm__ float *>(record.dst), sf);
    s(reinterpret_cast<__gm__ __bf16 *>(record.dst), sf); f(reinterpret_cast<__gm__ __bf16 *>(record.dst), sf);
    s(reinterpret_cast<__gm__ int8_t *>(record.dst), sf); f(reinterpret_cast<__gm__ int8_t *>(record.dst), sf);
    s(reinterpret_cast<__gm__ uint8_t *>(record.dst), sf); f(reinterpret_cast<__gm__ uint8_t *>(record.dst), sf);
    s(reinterpret_cast<__gm__ half *>(record.dst), si); f(reinterpret_cast<__gm__ half *>(record.dst), si);
    s(reinterpret_cast<__gm__ int16_t *>(record.dst), si); f(reinterpret_cast<__gm__ int16_t *>(record.dst), si);
    s(reinterpret_cast<__gm__ int8_t *>(record.dst), si); f(reinterpret_cast<__gm__ int8_t *>(record.dst), si);
    s(reinterpret_cast<__gm__ uint8_t *>(record.dst), si); f(reinterpret_cast<__gm__ uint8_t *>(record.dst), si);
    auto b4 = [&](auto *d) { call([&](uint8_t *m) { __sanitizer_report_copy_matrix_cc_to_gm_b4(m, loc.fileNo, loc.lineNo, loc.pc, d, si, xm, xt); }); };
    auto b4f = [&](auto *d) { call([&](uint8_t *m) { __sanitizer_report_copy_matrix_cc_to_gm_b4(m, loc.fileNo, loc.lineNo, loc.pc, d, si, sid, record.nSize, record.mSize, record.dstStride, record.srcStride, record.enUnitFlag, quantPRE, reLUPRE, record.channelSplit, record.enNZ2ND); }); };
    auto *dv = reinterpret_cast<__gm__ void *>(record.dst);
    b4(dv); b4f(dv);
}
}
