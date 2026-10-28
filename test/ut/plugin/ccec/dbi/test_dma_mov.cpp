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
#define BUILD_DYNAMIC_PROBE
#include "plugin/ccec/dbi/probes/dma_mov.cpp"
#include "plugin/ccec/dbi/probes/dma_mov_registers.cpp"

template<MemType srcMemType, MemType dstMemType, PadMode padMode = PadMode::PAD_NONE,
         ByteMode byteMode = ByteMode::BM_DISABLE>
DmaMovRecord CreateRandomDmaMovRecord()
{
    DmaMovRecord record{};
    record.dst = RandInt(0, 0x1000000);
    record.src = RandInt(0, 0x1000000);
    record.nBurst = RandInt(0, 0xFFF);
    record.lenBurst = RandInt(0, 0xFFFF);
    record.srcStride = RandInt(0, 0xFFF);
    record.dstStride = RandInt(0, 0xFFF);
    record.location.fileNo = RandInt(0, 0x100000);
    record.location.lineNo = RandInt(0, 0x1000);

    record.padMode = padMode;
    record.byteMode = byteMode;
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    return record;
}

template<MemType srcMemType, MemType dstMemType>
MovL1UBRecord CreateRandomMovL1UBRecord()
{
    MovL1UBRecord record{};
    record.dst = RandInt(0, 0x1000000);
    record.src = RandInt(0, 0x1000000);
    record.nBurst = RandInt(0, 0xFFF);
    record.lenBurst = RandInt(0, 0xFFFF);
    record.srcGap= RandInt(0, 0xFFF);
    record.dstGap = RandInt(0, 0xFFF);
    record.location.fileNo = RandInt(0, 0x100000);
    record.location.lineNo = RandInt(0, 0x1000);

    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    return record;
}

void ExtractConfigFromDmaMovRecord(const Sanitizer::DmaMovRecord *record, uint64_t &config0, uint64_t &config1)
{
    SetConfByUint<20, 4>(config0, record->nBurst);
    SetConfByUint<41, 25>(config0, record->lenBurst);
    SetConfByUint<59, 56>(config0, static_cast<uint8_t>(record->padMode));
    SetConfByUint<35, 0>(config1, record->srcStride);
    SetConfByUint<56, 40>(config1, record->dstStride);
}

TEST(DbiDmaMovInstructions, dump_copy_gm_to_cbuf_v2_expect_get_correct_records)
{
    auto record = CreateRandomDmaMovRecord<MemType::GM, MemType::L1>();

    std::vector<uint8_t> memInfo = CreateMemInfo();

    uint64_t config0{};
    uint64_t config1{};
    ExtractConfigFromDmaMovRecord(&record, config0, config1);
    __sanitizer_report_copy_gm_to_cbuf_v2(memInfo.data(),
        record.location.pc,
        0,
        reinterpret_cast<void *>(record.dst),
        reinterpret_cast<void *>(record.src),
        config0,
        config1);


    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    record.location.fileNo = reinterpret_cast<Sanitizer::DmaMovRecord *>(ptr)->location.fileNo;
    record.location.lineNo = reinterpret_cast<Sanitizer::DmaMovRecord *>(ptr)->location.lineNo;
    record.location.pc = reinterpret_cast<Sanitizer::DmaMovRecord *>(ptr)->location.pc;
    ASSERT_TRUE(CheckRecordEqual<RecordType::DMA_MOV>(ptr, record));

    record = CreateRandomDmaMovRecord<MemType::GM, MemType::L1>();
    ExtractConfigFromDmaMovRecord(&record, config0, config1);
    __sanitizer_report_copy_gm_to_cbuf_v2(memInfo.data(),
        record.location.pc,
        0,
        reinterpret_cast<void *>(record.dst),
        reinterpret_cast<void *>(record.src),
        config0,
        config1);

    ptr += sizeof(RecordType) + sizeof(DmaMovRecord);
    record.location.fileNo = reinterpret_cast<Sanitizer::DmaMovRecord *>(ptr)->location.fileNo;
    record.location.lineNo = reinterpret_cast<Sanitizer::DmaMovRecord *>(ptr)->location.lineNo;
    record.location.pc = reinterpret_cast<Sanitizer::DmaMovRecord *>(ptr)->location.pc;
    ASSERT_TRUE(CheckRecordEqual<RecordType::DMA_MOV>(ptr, record));
}

NdDMAOut2UbRecord CreateRandomNdDMAOut2UbRecord()
{
    NdDMAOut2UbRecord record{};
    record.dst = RandInt(0, 0x1000000);
    record.src = RandInt(0, 0x1000000);
    record.location.fileNo = RandInt(0, 0x100000);
    record.location.lineNo = RandInt(0, 0x1000);
    record.dataType = Sanitizer::DataType::DATA_B32;
    for (size_t i = 0; i < NdDMAOut2UbRecord::LOOP; ++i) {
        record.loop[i].loopSrcStride = RandInt(0, 0xFFFFF);
        record.loop[i].loopDstStride = RandInt(0, 0xFFFFF);
        record.loop[i].loopSize = RandInt(0, 0xFFFFF);
        record.loop[i].loopLpSize = RandInt(0, 0xFF);
        record.loop[i].loopRpSize = RandInt(0, 0xFF);
    }
    return record;
}

void SetNdDMAOut2UbReg(NdDMAOut2UbRecord &record, uint64_t &config0, uint64_t &config1, uint64_t &sprPadCntNdDma,
                       std::vector<uint64_t> &sprLoopStrideNdDma)
{
    SetConfByUint<23, 4>(config0, record.loop[0].loopSize);
    SetConfByUint<43, 24>(config0, record.loop[1].loopSize);
    SetConfByUint<63, 44>(config0, record.loop[2].loopSize);
    SetConfByUint<19, 0>(config1, record.loop[3].loopSize);
    SetConfByUint<39, 20>(config1, record.loop[4].loopSize);
    SetConfByUint<47, 40>(config1, record.loop[0].loopLpSize);
    SetConfByUint<55, 48>(config1, record.loop[0].loopRpSize);
    SetConfByUint<7, 0>(sprPadCntNdDma, record.loop[1].loopLpSize);
    SetConfByUint<15, 8>(sprPadCntNdDma, record.loop[1].loopRpSize);
    SetConfByUint<23, 16>(sprPadCntNdDma, record.loop[2].loopLpSize);
    SetConfByUint<31, 24>(sprPadCntNdDma, record.loop[2].loopRpSize);
    SetConfByUint<39, 32>(sprPadCntNdDma, record.loop[3].loopLpSize);
    SetConfByUint<47, 40>(sprPadCntNdDma, record.loop[3].loopRpSize);
    SetConfByUint<55, 48>(sprPadCntNdDma, record.loop[4].loopLpSize);
    SetConfByUint<63, 56>(sprPadCntNdDma, record.loop[4].loopRpSize);
    for (size_t i = 0; i < NdDMAOut2UbRecord::LOOP; ++i) {
        SetConfByUint<19, 0>(sprLoopStrideNdDma[i], record.loop[i].loopDstStride);
        SetConfByUint<59, 20>(sprLoopStrideNdDma[i], record.loop[i].loopSrcStride);
    }
}

TEST(DbiDmaMovInstructions, nd_copy_gm_to_ubuf_expect_get_correct_records)
{
    auto record = CreateRandomNdDMAOut2UbRecord();
    std::vector<uint8_t> memInfo = CreateMemInfo();

    uint64_t config0{};
    uint64_t config1{};
    uint64_t sprPadCntNdDma{};
    std::vector<uint64_t> sprLoopStrideNdDma(NdDMAOut2UbRecord::LOOP, 0);
    SetNdDMAOut2UbReg(record, config0, config1, sprPadCntNdDma, sprLoopStrideNdDma);
    __sanitizer_report_set_pad_cnt_nddma(memInfo.data(), record.location.pc, 0, sprPadCntNdDma);
    __sanitizer_report_set_loop0_stride_nddma(memInfo.data(), record.location.pc, 0, sprLoopStrideNdDma[0]);
    __sanitizer_report_set_loop1_stride_nddma(memInfo.data(), record.location.pc, 0, sprLoopStrideNdDma[1]);
    __sanitizer_report_set_loop2_stride_nddma(memInfo.data(), record.location.pc, 0, sprLoopStrideNdDma[2]);
    __sanitizer_report_set_loop3_stride_nddma(memInfo.data(), record.location.pc, 0, sprLoopStrideNdDma[3]);
    __sanitizer_report_set_loop4_stride_nddma(memInfo.data(), record.location.pc, 0, sprLoopStrideNdDma[4]);
    __sanitizer_report_nd_copy_gm_to_ubuf_b32(memInfo.data(),
        record.location.pc,
        0,
        reinterpret_cast<void *>(record.dst),
        reinterpret_cast<void *>(record.src),
        config0,
        config1);
    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::MovAlignRecordV2 *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::ND_DMA_OUT_TO_UB>(ptr, record));
}

TEST(DbiDmaMovInstructions, copy_gm_to_cbuf_multi_nd2nz_d_expect_get_correct_records)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    DmaMovNd2nzDavRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L1;
    record.dataType = DataType::DATA_B8;
    record.dst = 0x100;
    record.src = 0x200;
    record.loop1SrcStride = 8;
    record.nValue = 16;
    record.dValue = 8;
    record.loop4SrcStride = 2048;
    record.smallC0 = 0;
    record.ndNum = 2;
    record.loop2DstStride = 128;
    record.loop3DstStride = 512;
    record.loop4DstStride = 1024;

    uint64_t config0 = 0;
    SetConfByUint<43, 4>(config0, 8); // loop 1 src stride
    SetConfByUint<63, 48>(config0, 16); // n

    uint64_t config1 = 0;
    SetConfByUint<20, 0>(config1, 8); // d
    SetConfByUint<60, 21>(config1, 2048); // loop 4 src stride
    SetConfByUint<61, 61>(config1, 0); // small c0

    uint64_t nzPara = 0;
    SetConfByUint<15, 0>(nzPara, 2); // nd num
    SetConfByUint<31, 16>(nzPara, 128); // loop 2 dst stride dst_nz_n_stride
    SetConfByUint<47, 32>(nzPara, 512); // loop 3 dst stride dst_nz_c0_stride
    SetConfByUint<63, 48>(nzPara, 1024); // loop 4 dst stride dst_nz_matrix_stride * sizeof_dst_type / C0_size
    __sanitizer_report_set_mte2_nz_para(memInfo.data(),
        record.location.pc,
        0,
        nzPara);

    __sanitizer_report_copy_gm_to_cbuf_multi_nd2nz_d_b8(memInfo.data(),
        record.location.pc,
        0,
        reinterpret_cast<void *>(record.dst),
        reinterpret_cast<void *>(record.src),
        config0,
        config1);
    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::DmaMovNd2nzDavRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::DMA_MOV_ND2NZ_D>(ptr, record));
}

TEST(DbiDmaMovInstructions, copy_gm_to_cbuf_multi_dn2nz_d_expect_get_correct_records)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    DmaMovNd2nzDavRecord record{};
    record.srcMemType = MemType::GM;
    record.dstMemType = MemType::L1;
    record.dataType = DataType::DATA_B32;
    record.dst = 0x100;
    record.src = 0x200;
    record.loop1SrcStride = 8;
    record.nValue = 16;
    record.dValue = 8;
    record.loop4SrcStride = 2048;
    record.smallC0 = 0;
    record.ndNum = 2;
    record.loop2DstStride = 128;
    record.loop3DstStride = 512;
    record.loop4DstStride = 2048;

    uint64_t config0 = 0;
    SetConfByUint<43, 4>(config0, 8); // loop 1 src stride
    SetConfByUint<63, 48>(config0, 16); // n

    uint64_t config1 = 0;
    SetConfByUint<20, 0>(config1, 8); // d
    SetConfByUint<60, 21>(config1, 2048); // loop 4 src stride
    SetConfByUint<61, 61>(config1, 0); // small c0

    uint64_t nzPara = 0;
    SetConfByUint<15, 0>(nzPara, 2); // nd num
    SetConfByUint<31, 16>(nzPara, 128); // loop 2 dst stride dst_nz_n_stride
    SetConfByUint<47, 32>(nzPara, 512); // loop 3 dst stride dst_nz_c0_stride
    SetConfByUint<63, 48>(nzPara, 2048); // loop 4 dst stride dst_nz_matrix_stride * sizeof_dst_type / C0_size
    __sanitizer_report_set_mte2_nz_para(memInfo.data(),
        record.location.pc,
        0,
        nzPara);

    __sanitizer_report_copy_gm_to_cbuf_multi_dn2nz_d_b32(memInfo.data(),
        record.location.pc,
        0,
        reinterpret_cast<void *>(record.dst),
        reinterpret_cast<void *>(record.src),
        config0,
        config1);
    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::DmaMovNd2nzDavRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::DMA_MOV_DN2NZ_D>(ptr, record));
}

TEST(DbiDmaMovInstructions, set_l1_2d_b16)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    SetL12DRecord record{};
    record.dst = 0x1000;
    record.repeat = 4;
    record.dstBlockNum = 32;
    record.repeatGap = 32;
    record.detailedDataType = DetailedDataType::B16;

    uint32_t *dst_l1 = (uint32_t *)0x1000;

    uint64_t src_l1 = 0;
    SetConfByUint<14, 0>(src_l1, 4); // repeat_time
    SetConfByUint<15, 15>(src_l1, 0); // reserved
    SetConfByUint<30, 16>(src_l1, 32); // block_num
    SetConfByUint<31, 31>(src_l1, 0); // reserved
    SetConfByUint<46, 32>(src_l1, 32); // repeat gap
    SetConfByUint<63, 47>(src_l1, 0); // reserved

    __sanitizer_report_set_l1_2d_b16(memInfo.data(),
        record.location.pc,
        0,
        dst_l1,
        src_l1);
    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::SetL12DRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(DbiDmaMovInstructions, set_l1_2d_b32)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    SetL12DRecord record{};
    record.dst = 0x1000;
    record.repeat = 4;
    record.dstBlockNum = 32;
    record.repeatGap = 32;
    record.detailedDataType = DetailedDataType::B32;

    uint32_t *dst_l1 = (uint32_t *)0x1000;

    uint64_t src_l1 = 0;
    SetConfByUint<14, 0>(src_l1, 4); // repeat_time
    SetConfByUint<15, 15>(src_l1, 0); // reserved
    SetConfByUint<30, 16>(src_l1, 32); // block_num
    SetConfByUint<31, 31>(src_l1, 0); // reserved
    SetConfByUint<46, 32>(src_l1, 32); // repeat gap
    SetConfByUint<63, 47>(src_l1, 0); // reserved

    __sanitizer_report_set_l1_2d_b32(memInfo.data(),
        record.location.pc,
        0,
        dst_l1,
        src_l1);
    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::SetL12DRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

void SetL1toUbMovL1UBRecord(const Sanitizer::MovL1UBRecord *record, uint64_t &config0)
{
    SetConfByUint<15, 4>(config0, record->nBurst);
    SetConfByUint<31, 16>(config0, record->lenBurst);
    SetConfByUint<47, 32>(config0, record->srcGap);
    SetConfByUint<63, 48>(config0, record->dstGap);
}

TEST(DbiDmaMovInstructions, copy_l1_to_ub_expect_get_correct_records)
{
    auto record = CreateRandomMovL1UBRecord<MemType::L1, MemType::UB>();

    std::vector<uint8_t> memInfo = CreateMemInfo();

    uint64_t config0{};
    SetL1toUbMovL1UBRecord(&record, config0);
    __sanitizer_report_copy_cbuf_to_ubuf(memInfo.data(),
        record.location.pc,
        0,
        reinterpret_cast<void *>(record.dst),
        reinterpret_cast<void *>(record.src),
        config0);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    record.location.fileNo = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.fileNo;
    record.location.lineNo = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.lineNo;
    record.location.pc = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.pc;
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_L1_TO_UB>(ptr, record));

    record = CreateRandomMovL1UBRecord<MemType::L1, MemType::UB>();
    SetL1toUbMovL1UBRecord(&record, config0);
    __sanitizer_report_copy_cbuf_to_ubuf(memInfo.data(),
        record.location.pc,
        0,
        reinterpret_cast<void *>(record.dst),
        reinterpret_cast<void *>(record.src),
        config0);
    ptr += sizeof(RecordType) + sizeof(MovL1UBRecord);
    record.location.fileNo = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.fileNo;
    record.location.lineNo = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.lineNo;
    record.location.pc = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.pc;
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_L1_TO_UB>(ptr, record));
}

void SetUbtoL1MovL1UBRecord(const Sanitizer::MovL1UBRecord *record, uint64_t &config0)
{
    SetConfByUint<15, 4>(config0, record->nBurst);
    SetConfByUint<31, 16>(config0, record->lenBurst);
    SetConfByUint<47, 32>(config0, record->srcGap);
    SetConfByUint<63, 48>(config0, record->dstGap);
}

TEST(DbiDmaMovInstructions, copy_ub_to_l1_expect_get_correct_records)
{
    auto record = CreateRandomMovL1UBRecord<MemType::UB, MemType::L1>();

    std::vector<uint8_t> memInfo = CreateMemInfo();

    uint64_t config0{};
    SetUbtoL1MovL1UBRecord(&record, config0);
    __sanitizer_report_copy_ubuf_to_cbuf(memInfo.data(),
        record.location.pc,
        0,
        reinterpret_cast<void *>(record.dst),
        reinterpret_cast<void *>(record.src),
        config0);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    record.location.fileNo = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.fileNo;
    record.location.lineNo = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.lineNo;
    record.location.pc = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.pc;
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_UB_TO_L1>(ptr, record));

    record = CreateRandomMovL1UBRecord<MemType::UB, MemType::L1>();
    SetUbtoL1MovL1UBRecord(&record, config0);
    __sanitizer_report_copy_ubuf_to_cbuf(memInfo.data(),
        record.location.pc,
        0,
        reinterpret_cast<void *>(record.dst),
        reinterpret_cast<void *>(record.src),
        config0);
    ptr += sizeof(RecordType) + sizeof(MovL1UBRecord);
    record.location.fileNo = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.fileNo;
    record.location.lineNo = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.lineNo;
    record.location.pc = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.pc;
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_UB_TO_L1>(ptr, record));
}

void SetUbtoUbMovL1UBRecord(const Sanitizer::MovL1UBRecord *record, uint64_t &config0)
{
    SetConfByUint<15, 0>(config0, record->nBurst);
    SetConfByUint<31, 16>(config0, record->lenBurst);
    SetConfByUint<47, 32>(config0, record->srcGap);
    SetConfByUint<63, 48>(config0, record->dstGap);
}

TEST(DbiDmaMovInstructions, copy_ub_to_ub_expect_get_correct_records)
{
    auto record = CreateRandomMovL1UBRecord<MemType::UB, MemType::UB>();

    std::vector<uint8_t> memInfo = CreateMemInfo();

    uint64_t config0{};
    SetUbtoUbMovL1UBRecord(&record, config0);
    __sanitizer_report_copy_ubuf_to_ubuf(memInfo.data(),
        record.location.pc,
        0,
        reinterpret_cast<void *>(record.dst),
        reinterpret_cast<void *>(record.src),
        config0);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    record.location.fileNo = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.fileNo;
    record.location.lineNo = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.lineNo;
    record.location.pc = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.pc;
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_UB_TO_UB>(ptr, record));

    record = CreateRandomMovL1UBRecord<MemType::UB, MemType::UB>();
    SetUbtoUbMovL1UBRecord(&record, config0);
    __sanitizer_report_copy_ubuf_to_ubuf(memInfo.data(),
        record.location.pc,
        0,
        reinterpret_cast<void *>(record.dst),
        reinterpret_cast<void *>(record.src),
        config0);
    ptr += sizeof(RecordType) + sizeof(MovL1UBRecord);
    record.location.fileNo = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.fileNo;
    record.location.lineNo = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.lineNo;
    record.location.pc = reinterpret_cast<Sanitizer::MovL1UBRecord *>(ptr)->location.pc;
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_UB_TO_UB>(ptr, record));
}

TEST(DbiDmaMovInstructions, copy_cbuf_to_bt_expect_get_correct_records)
{
    MovL1BtRecord record{};
    record.location = {0, 0, 22};
    record.dst = 0x100;
    record.src = 0x1000;
    record.nBurst = 20;
    record.lenBurst = 30;
    record.srcGap = 2;
    record.dstGap = 3;
    record.cvtEnable = 1;
    std::vector<uint8_t> memInfo = CreateMemInfo();

    uint64_t config{};
    SetConfByUint<15, 4>(config, record.nBurst);
    SetConfByUint<31, 16>(config, record.lenBurst);
    SetConfByUint<47, 32>(config, record.srcGap);
    SetConfByUint<63, 48>(config, record.dstGap);
    SetConfByUint<3, 3>(config, record.cvtEnable);

    // f32
    __sanitizer_report_copy_cbuf_to_bt_f32(memInfo.data(), record.location.pc, 0, record.dst,
        reinterpret_cast<void*>(record.src), config);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_CBUF_TO_BT>(ptr, record));
    const MovL1BtRecord *l1BtRecord = reinterpret_cast<const MovL1BtRecord *>(ptr + sizeof(RecordType));
    ASSERT_EQ(l1BtRecord->dataType, DetailedDataType::FLOAT);

    // s32
    __sanitizer_report_copy_cbuf_to_bt_s32(memInfo.data(), record.location.pc, 0, record.dst,
        reinterpret_cast<void*>(record.src), config);
    ptr += sizeof(RecordType) + sizeof(MovL1BtRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_CBUF_TO_BT>(ptr, record));
    l1BtRecord = reinterpret_cast<const MovL1BtRecord *>(ptr + sizeof(RecordType));
    ASSERT_EQ(l1BtRecord->dataType, DetailedDataType::S32);

    // f16
    __sanitizer_report_copy_cbuf_to_bt_f16(memInfo.data(), record.location.pc, 0, record.dst,
    reinterpret_cast<void*>(record.src), config);
    ptr += sizeof(RecordType) + sizeof(MovL1BtRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_CBUF_TO_BT>(ptr, record));
    l1BtRecord = reinterpret_cast<const MovL1BtRecord *>(ptr + sizeof(RecordType));
    ASSERT_EQ(l1BtRecord->dataType, DetailedDataType::F16);

    // B16
    __sanitizer_report_copy_cbuf_to_bt_b16(memInfo.data(), record.location.pc, 0, record.dst,
    reinterpret_cast<void*>(record.src), config);
    ptr += sizeof(RecordType) + sizeof(MovL1BtRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_CBUF_TO_BT>(ptr, record));
    l1BtRecord = reinterpret_cast<const MovL1BtRecord *>(ptr + sizeof(RecordType));
    ASSERT_EQ(l1BtRecord->dataType, DetailedDataType::B16);
}

TEST(DbiDmaMovInstructions, copy_cbuf_to_fbuf_expect_get_correct_records)
{
    MovL1FbRecord record{};
    uint64_t dst = 0x12aa9;
    record.location = {0, 0, 22};
    record.dst = static_cast<uint16_t>(dst & 0xFFFF);
    record.src = 0x1000;
    record.nBurst = 20;
    record.lenBurst = 30;
    record.srcStride = 2;
    record.dstStride = 3;
    record.dstMemBlock = static_cast<uint8_t>((dst >> 16) & 0x0F);
    std::vector<uint8_t> memInfo = CreateMemInfo();

    uint64_t config{};
    SetConfByUint<15, 4>(config, record.nBurst);
    SetConfByUint<31, 16>(config, record.lenBurst);
    SetConfByUint<47, 32>(config, record.srcStride);
    SetConfByUint<63, 48>(config, record.dstStride);

    __sanitizer_report_copy_cbuf_to_fbuf(memInfo.data(), record.location.pc, 0, reinterpret_cast<void*>(dst),
        reinterpret_cast<void*>(record.src), config);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::MOV_CBUF_TO_FB>(ptr, record));
}

TEST(DbiDmaMovInstructions, basic_dma_mov_and_nd2nz_overloads_expect_record)
{
    auto call = [&](auto &&func, RecordType expectType) {
        std::vector<uint8_t> memInfo = CreateMemInfo();
        func(memInfo.data());
        uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
        EXPECT_EQ(*reinterpret_cast<RecordType *>(ptr), expectType);
    };
    auto pc = 0ULL;
    uint64_t config = 0, xm = 0, xt = 0;
    auto *ub = reinterpret_cast<__ubuf__ void *>(0x100);
    auto *gm = reinterpret_cast<__gm__ void *>(0x200);
    auto *cb = reinterpret_cast<__cbuf__ void *>(0x100);
    call([&](uint8_t *m) { __sanitizer_report_copy_gm_to_ubuf(m, pc, 0, ub, gm, config); }, RecordType::DMA_MOV);
    call([&](uint8_t *m) { __sanitizer_report_copy_ubuf_to_gm(m, pc, 0, gm, ub, config); }, RecordType::DMA_MOV);
    call([&](uint8_t *m) { __sanitizer_report_copy_ubuf_to_gm_byte(m, pc, 0, gm, ub, config, static_cast<bm_t>(0)); }, RecordType::DMA_MOV);
    call([&](uint8_t *m) { __sanitizer_report_copy_cbuf_to_gm(m, pc, 0, gm, cb, config); }, RecordType::DMA_MOV);
    call([&](uint8_t *m) { __sanitizer_report_copy_gm_to_cbuf(m, pc, 0, cb, gm, config, static_cast<pad_t>(0)); }, RecordType::DMA_MOV);
    call([&](uint8_t *m) { __sanitizer_report_copy_gm_to_cbuf_multi_nd2nz_b8(m, pc, 0, cb, gm, xm, xt); }, RecordType::DMA_MOV_ND2NZ);
    call([&](uint8_t *m) { __sanitizer_report_copy_gm_to_cbuf_multi_nd2nz_b16(m, pc, 0, cb, gm, xm, xt); }, RecordType::DMA_MOV_ND2NZ);
    call([&](uint8_t *m) { __sanitizer_report_copy_gm_to_cbuf_multi_nd2nz_b32s(m, pc, 0, cb, gm, xm, xt); }, RecordType::DMA_MOV_ND2NZ);
    call([&](uint8_t *m) { __sanitizer_report_nd_copy_gm_to_ubuf_b8(m, pc, 0, ub, gm, xm, xt); }, RecordType::ND_DMA_OUT_TO_UB);
    call([&](uint8_t *m) { __sanitizer_report_nd_copy_gm_to_ubuf_b16(m, pc, 0, ub, gm, xm, xt); }, RecordType::ND_DMA_OUT_TO_UB);
    auto callDav = [&](auto &&func, RecordType expectType) {
        std::vector<uint8_t> memInfo = CreateMemInfo();
        __sanitizer_report_set_mte2_nz_para(memInfo.data(), pc, 0, 0);
        func(memInfo.data());
        uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
        EXPECT_EQ(*reinterpret_cast<RecordType *>(ptr), expectType);
    };
    callDav([&](uint8_t *m) { __sanitizer_report_copy_gm_to_cbuf_multi_nd2nz_d_b16(m, pc, 0, cb, gm, xm, xt); }, RecordType::DMA_MOV_ND2NZ_D);
    callDav([&](uint8_t *m) { __sanitizer_report_copy_gm_to_cbuf_multi_nd2nz_d_b32(m, pc, 0, cb, gm, xm, xt); }, RecordType::DMA_MOV_ND2NZ_D);
    callDav([&](uint8_t *m) { __sanitizer_report_copy_gm_to_cbuf_multi_dn2nz_d_b8(m, pc, 0, cb, gm, xm, xt); }, RecordType::DMA_MOV_DN2NZ_D);
    callDav([&](uint8_t *m) { __sanitizer_report_copy_gm_to_cbuf_multi_dn2nz_d_b16(m, pc, 0, cb, gm, xm, xt); }, RecordType::DMA_MOV_DN2NZ_D);
}
