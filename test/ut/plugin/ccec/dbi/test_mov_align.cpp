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
#include <iostream>
#include <gtest/gtest.h>
#include <cstdint>

#include "../data_process.h"
#define BUILD_DYNAMIC_PROBE
#include "plugin/ccec/dbi/probes/mov_align.cpp"
#include "plugin/ccec/dbi/probes/mov_align_registers.cpp"

template<MemType srcMemType, MemType dstMemType>
MovAlignRecordV2 CreateRandomMovAlignV2Record()
{
    MovAlignRecordV2 record{};
    record.dst = RandInt(0, 0x1000000);
    record.src = RandInt(0, 0x1000000);
    record.nBurst = RandInt(0, 0xFFF);
    record.lenBurst = RandInt(0, 0xFFFF);
    record.srcStride = RandInt(0, 0xFFF);
    record.dstStride = RandInt(0, 0xFFF);
    record.location.fileNo = RandInt(0, 0x100000);
    record.location.lineNo = RandInt(0, 0x1000);
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    record.loop1DstStride = RandInt(0, 0xFFFFF);
    record.loop1SrcStride = RandInt(0, 0xFFFFF);
    record.loop2DstStride = RandInt(0, 0xFFFFF);
    record.loop2SrcStride = RandInt(0, 0xFFFFF);
    record.loop1Size = RandInt(0, 0xFFFFF);
    record.loop2Size = RandInt(0, 0xFFFFF);
    record.dataType = Sanitizer::DataType::DATA_B32;
    // not gen left/right padding
    return record;
}

void ExtractConfigFromMovAlignV2Record(const Sanitizer::MovAlignRecordV2 *record, uint64_t &config0, uint64_t &config1)
{
    SetConfByUint<24, 4>(config0, record->nBurst);
    SetConfByUint<45, 25>(config0, record->lenBurst);
    if (record->srcMemType == MemType::UB && record->dstMemType == MemType::GM) {
        SetConfByUint<39, 0>(config1, record->dstStride);
        SetConfByUint<60, 40>(config1, record->srcStride);
    } else {
        SetConfByUint<39, 0>(config1, record->srcStride);
        SetConfByUint<60, 40>(config1, record->dstStride);
    }
}

TEST(DbiMovAlignInstructions, dump_mov_align_v2_ub2gm_expect_get_correct_records)
{
    auto record = CreateRandomMovAlignV2Record<MemType::UB, MemType::GM>();
    std::vector<uint8_t> memInfo = CreateMemInfo();

    uint64_t config0{};
    uint64_t config1{};
    ExtractConfigFromMovAlignV2Record(&record, config0, config1);

    uint8_t *ptr = memInfo.data();

    uint64_t loopSizeUbToOut = 0;
    SetConfByUint<20, 0>(loopSizeUbToOut, record.loop1Size);
    SetConfByUint<42, 21>(loopSizeUbToOut, record.loop2Size);
    uint64_t loop1StrideUbToOut = 0;
    SetConfByUint<39, 0>(loop1StrideUbToOut, record.loop1DstStride);
    SetConfByUint<60, 40>(loop1StrideUbToOut, record.loop1SrcStride);
    uint64_t loop2StrideUbToOut = 0;
    SetConfByUint<39, 0>(loop2StrideUbToOut, record.loop2DstStride);
    SetConfByUint<60, 40>(loop2StrideUbToOut, record.loop2SrcStride);

    __sanitizer_report_set_loop_size_ubtoout(memInfo.data(),
        record.location.pc, 0, loopSizeUbToOut);
    Sanitizer::RecordGlobalHead &recordGlobalHead = *reinterpret_cast<Sanitizer::RecordGlobalHead*>(ptr);
    int64_t regIdx = GetRegisterIdx();
    uint64_t regVal = GetUintFromConf<20, 0>(recordGlobalHead.registers[regIdx].sprLoopSizeUb2Out);
    ASSERT_TRUE(regVal == record.loop1Size);
    regVal = GetUintFromConf<42, 21>(recordGlobalHead.registers[regIdx].sprLoopSizeUb2Out);
    ASSERT_TRUE(regVal == record.loop2Size);

    __sanitizer_report_set_loop1_stride_ubtoout(memInfo.data(),
        record.location.pc, 0, loop1StrideUbToOut);
    regVal = GetUintFromConf<39, 0>(recordGlobalHead.registers[regIdx].sprLoop1StrideUb2Out);
    ASSERT_TRUE(regVal == record.loop1DstStride);
    regVal = GetUintFromConf<60, 40>(recordGlobalHead.registers[regIdx].sprLoop1StrideUb2Out);
    ASSERT_TRUE(regVal == record.loop1SrcStride);

    __sanitizer_report_set_loop2_stride_ubtoout(memInfo.data(),
        record.location.pc, 0, loop2StrideUbToOut);
    regVal = GetUintFromConf<39, 0>(recordGlobalHead.registers[regIdx].sprLoop2StrideUb2Out);
    ASSERT_TRUE(regVal == record.loop2DstStride);
    regVal = GetUintFromConf<60, 40>(recordGlobalHead.registers[regIdx].sprLoop2StrideUb2Out);
    ASSERT_TRUE(regVal == record.loop2SrcStride);

    __sanitizer_report_copy_ubuf_to_gm_align_v2(memInfo.data(),
        record.location.pc,
        0,
        reinterpret_cast<void *>(record.dst),
        reinterpret_cast<void *>(record.src),
        config0,
        config1);

    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::MovAlignRecordV2 *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_ALIGN_V2>(ptr, record));
}

TEST(DbiMovAlignInstructions, dump_mov_align_v2_gm2ub__expect_get_correct_records)
{
    auto record = CreateRandomMovAlignV2Record<MemType::GM, MemType::UB>();
    std::vector<uint8_t> memInfo = CreateMemInfo();

    uint64_t config0{};
    uint64_t config1{};
    ExtractConfigFromMovAlignV2Record(&record, config0, config1);

    uint8_t *ptr = memInfo.data();

    uint64_t loopSizeOut2Ub = 0;
    SetConfByUint<20, 0>(loopSizeOut2Ub, record.loop1Size);
    SetConfByUint<42, 21>(loopSizeOut2Ub, record.loop2Size);
    uint64_t loop1StrideOut2Ub = 0;
    SetConfByUint<39, 0>(loop1StrideOut2Ub, record.loop1SrcStride);
    SetConfByUint<60, 40>(loop1StrideOut2Ub, record.loop1DstStride);
    uint64_t loop2StrideOut2Ub = 0;
    SetConfByUint<39, 0>(loop2StrideOut2Ub, record.loop2SrcStride);
    SetConfByUint<60, 40>(loop2StrideOut2Ub, record.loop2DstStride);

    __sanitizer_report_set_loop_size_outtoub(memInfo.data(),
        record.location.pc, 0, loopSizeOut2Ub);
    Sanitizer::RecordGlobalHead &recordGlobalHead = *reinterpret_cast<Sanitizer::RecordGlobalHead*>(ptr);
    int64_t regIdx = GetRegisterIdx();
    uint64_t regVal = GetUintFromConf<20, 0>(recordGlobalHead.registers[regIdx].sprLoopSizeOut2Ub);
    ASSERT_TRUE(regVal == record.loop1Size);
    regVal = GetUintFromConf<42, 21>(recordGlobalHead.registers[regIdx].sprLoopSizeOut2Ub);
    ASSERT_TRUE(regVal == record.loop2Size);

    __sanitizer_report_set_loop1_stride_outtoub(memInfo.data(),
        record.location.pc, 0, loop1StrideOut2Ub);
    regVal = GetUintFromConf<39, 0>(recordGlobalHead.registers[regIdx].sprLoop1StrideOut2Ub);
    ASSERT_TRUE(regVal == record.loop1SrcStride);
    regVal = GetUintFromConf<60, 40>(recordGlobalHead.registers[regIdx].sprLoop1StrideOut2Ub);
    ASSERT_TRUE(regVal == record.loop1DstStride);

    __sanitizer_report_set_loop2_stride_outtoub(memInfo.data(),
        record.location.pc, 0, loop2StrideOut2Ub);
    regVal = GetUintFromConf<39, 0>(recordGlobalHead.registers[regIdx].sprLoop2StrideOut2Ub);
    ASSERT_TRUE(regVal == record.loop2SrcStride);
    regVal = GetUintFromConf<60, 40>(recordGlobalHead.registers[regIdx].sprLoop2StrideOut2Ub);
    ASSERT_TRUE(regVal == record.loop2DstStride);

    __sanitizer_report_copy_gm_to_ubuf_align_v2_b32(memInfo.data(),
        record.location.pc,
        0,
        reinterpret_cast<void *>(record.dst),
        reinterpret_cast<void *>(record.src),
        config0,
        config1);

    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::MovAlignRecordV2 *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_ALIGN_V2>(ptr, record));
}

TEST(DbiMovAlignInstructions, dump_loop_out_to_l1_registers_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    auto *globalHead = reinterpret_cast<Sanitizer::RecordGlobalHead *>(memInfo.data());
    int64_t regIdx = GetRegisterIdx();

    uint64_t loopSize = 0;
    SetConfByUint<20, 0>(loopSize, 16U);
    SetConfByUint<42, 21>(loopSize, 32U);
    __sanitizer_report_set_loop_size_outtol1(memInfo.data(), 0, 0, loopSize);
    auto &loopSizeReg = globalHead->registers[regIdx].sprLoopSizeOut2L1;
    EXPECT_EQ((GetUintFromConf<20, 0>(loopSizeReg)), 16U);
    EXPECT_EQ((GetUintFromConf<42, 21>(loopSizeReg)), 32U);

    uint64_t loop1Stride = 0;
    SetConfByUint<39, 0>(loop1Stride, 64U);
    SetConfByUint<60, 40>(loop1Stride, 128U);
    __sanitizer_report_set_loop1_stride_outtol1(memInfo.data(), 0, 0, loop1Stride);
    auto &loop1Reg = globalHead->registers[regIdx].sprLoop1StrideOut2L1;
    EXPECT_EQ((GetUintFromConf<39, 0>(loop1Reg)), 64U);
    EXPECT_EQ((GetUintFromConf<60, 40>(loop1Reg)), 128U);

    uint64_t loop2Stride = 0;
    SetConfByUint<39, 0>(loop2Stride, 256U);
    SetConfByUint<60, 40>(loop2Stride, 512U);
    __sanitizer_report_set_loop2_stride_outtol1(memInfo.data(), 0, 0, loop2Stride);
    auto &loop2Reg = globalHead->registers[regIdx].sprLoop2StrideOut2L1;
    EXPECT_EQ((GetUintFromConf<39, 0>(loop2Reg)), 256U);
    EXPECT_EQ((GetUintFromConf<60, 40>(loop2Reg)), 512U);
}

TEST(DbiMovAlignInstructions, align_basic_and_v2_remaining_overloads_expect_record)
{
    auto call = [&](auto &&func, RecordType expectType) {
        std::vector<uint8_t> memInfo = CreateMemInfo();
        func(memInfo.data());
        uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
        EXPECT_EQ(*reinterpret_cast<RecordType *>(ptr), expectType);
    };
    uint64_t config = 0, gapConfig = 0, config1 = 0;
    auto *ub = reinterpret_cast<__ubuf__ void *>(0x100);
    auto *gm = reinterpret_cast<__gm__ void *>(0x200);
    auto *cb = reinterpret_cast<__cbuf__ void *>(0x100);
    call([&](uint8_t *m) { __sanitizer_report_copy_gm_to_ubuf_align_b8(m, 0, 0, ub, gm, config, gapConfig); }, RecordType::MOV_ALIGN);
    call([&](uint8_t *m) { __sanitizer_report_copy_gm_to_ubuf_align_b16(m, 0, 0, ub, gm, config, gapConfig); }, RecordType::MOV_ALIGN);
    call([&](uint8_t *m) { __sanitizer_report_copy_gm_to_ubuf_align_b32(m, 0, 0, ub, gm, config, gapConfig); }, RecordType::MOV_ALIGN);
    call([&](uint8_t *m) { __sanitizer_report_copy_ubuf_to_gm_align_b8(m, 0, 0, gm, ub, config, gapConfig); }, RecordType::MOV_ALIGN);
    call([&](uint8_t *m) { __sanitizer_report_copy_ubuf_to_gm_align_b16(m, 0, 0, gm, ub, config, gapConfig); }, RecordType::MOV_ALIGN);
    call([&](uint8_t *m) { __sanitizer_report_copy_ubuf_to_gm_align_b32(m, 0, 0, gm, ub, config, gapConfig); }, RecordType::MOV_ALIGN);
    call([&](uint8_t *m) { __sanitizer_report_copy_gm_to_ubuf_align_v2_b8(m, 0, 0, ub, gm, config, config1); }, RecordType::MOV_ALIGN_V2);
    call([&](uint8_t *m) { __sanitizer_report_copy_gm_to_ubuf_align_v2_b16(m, 0, 0, ub, gm, config, config1); }, RecordType::MOV_ALIGN_V2);
    call([&](uint8_t *m) { __sanitizer_report_copy_gm_to_cbuf_align_v2_b8(m, 0, 0, cb, gm, config, config1); }, RecordType::MOV_ALIGN_V2);
    call([&](uint8_t *m) { __sanitizer_report_copy_gm_to_cbuf_align_v2_b16(m, 0, 0, cb, gm, config, config1); }, RecordType::MOV_ALIGN_V2);
    call([&](uint8_t *m) { __sanitizer_report_copy_gm_to_cbuf_align_v2_b32(m, 0, 0, cb, gm, config, config1); }, RecordType::MOV_ALIGN_V2);
}
