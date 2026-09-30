// -------------------------------------------------------------------------
// This file is part of the MindStudio project.
// Copyright (c) 2026 Huawei Technologies Co.,Ltd.
//
// MindStudio is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
//
//          http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
// See the Mulan PSL v2 for more details.
// -------------------------------------------------------------------------

#include <utility>
#include <gtest/gtest.h>

#include "../data_process.h"
#define BUILD_DYNAMIC_PROBE
#include "plugin/ccec/dbi/probes/load_3d.cpp"
#include "plugin/ccec/dbi/probes/cube_mte_registers.cpp"

TEST(Load3DInstructions, load_cbuf_to_ca_3dv2_b32)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    Load3DV2Record record{};
    record.dst = 0x1000;
    record.src = 0x1000;

    record.matrixKStep = 32;
    record.matrixMStep = 32;
    record.matrixKPos = 0;
    record.matrixMPos = 0;

    record.filterWStride = 1;
    record.filterHStride = 1;
    record.filterW = 64;
    record.filterH = 64;
    record.filterWDilation = 1;
    record.filterHDilation = 1;
    record.transposeMode = 0;
    record.matrixMode = 0;
    record.fMapC = 32;

    record.fMapW = 512;
    record.fMapH = 512;
    record.fMapLeftPad = 0;
    record.fMapRightPad = 0;
    record.fMapTopPad = 0;
    record.fMapBottomPad = 0;

    record.matrixRptStride = 64;
    record.matrixRptTimes = 64;
    record.matrixRptMode = 0;
    record.dstStride = 32;
    record.outputMPos = 0;

    record.dataType = DataType::DATA_B32;
    record.dstMemType = MemType::L0A;
    record.srcMemType = MemType::L1;

    uint64_t xm = 0;
    uint64_t xt = 0;
    SetConfByUint<15, 0>(xm, 32);//kstep
    SetConfByUint<31, 16>(xm, 32);//mstep
    SetConfByUint<47, 32>(xm, 0);//kstart
    SetConfByUint<63, 48>(xm, 0);//mstart
    SetConfByUint<5, 0>(xt, 1);//wstride
    SetConfByUint<11, 6>(xt, 1);//hstride
    SetConfByUint<19, 12>(xt, 64);//low bit of wsize
    SetConfByUint<44, 44>(xt, 0);//high bit of wsize
    SetConfByUint<27, 20>(xt, 64);//low bit of hsize
    SetConfByUint<45, 45>(xt, 0);//high bit of hsize
    SetConfByUint<35, 28>(xt, 1);//wDilation
    SetConfByUint<43, 36>(xt, 1);//hDilation
    SetConfByUint<46, 46>(xt, 0);//Transpose Control
    SetConfByUint<47, 47>(xt, 0);//SPR Control
    SetConfByUint<63, 48>(xt, 32);//channel size N

    uint64_t sprMatrix = 0;
    uint64_t sprL3dRpt = 0;
    SetConfByUint<15, 0>(sprMatrix, 512);//W
    SetConfByUint<31, 16>(sprMatrix, 512);//H
    SetConfByUint<39, 32>(sprMatrix, 0);//left padding
    SetConfByUint<47, 40>(sprMatrix, 0);//right padding
    SetConfByUint<55, 48>(sprMatrix, 0);//top padding
    SetConfByUint<63, 56>(sprMatrix, 0);//bottom padding

    SetConfByUint<15, 0>(sprL3dRpt, 64);//repeatStride
    SetConfByUint<23, 16>(sprL3dRpt, 64);//repeatTimes
    SetConfByUint<24, 24>(sprL3dRpt, 0);//repeatMode
    SetConfByUint<47, 32>(sprL3dRpt, 32);//dstStride
    SetConfByUint<63, 48>(sprL3dRpt, 0);//mStartPosition output

    __sanitizer_report_set_fmatrix(memInfo.data(), record.location.pc, 0, sprMatrix);
    __sanitizer_report_set_l3d_rpt(memInfo.data(), record.location.pc, 0, sprL3dRpt);

    auto dst = (__ca__ uint32_t *)(0x1000);
    auto src = (__cbuf__ uint32_t *)(0x1000);

    __sanitizer_report_img2colv2_cbuf_to_ca_b32(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        xm,
        xt);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::Load3DV2Record *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(x->dstStride == record.dstStride);
}

TEST(Load3DInstructions, load_cbuf_to_ca_3dv2_b16)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    Load3DV2Record record{};
    record.dst = 0x1000;
    record.src = 0x1000;

    record.matrixKStep = 32;
    record.matrixMStep = 32;
    record.matrixKPos = 0;
    record.matrixMPos = 0;

    record.filterWStride = 1;
    record.filterHStride = 1;
    record.filterW = 64;
    record.filterH = 64;
    record.filterWDilation = 1;
    record.filterHDilation = 1;
    record.transposeMode = 0;
    record.matrixMode = 0;
    record.fMapC = 32;

    record.fMapW = 512;
    record.fMapH = 512;
    record.fMapLeftPad = 0;
    record.fMapRightPad = 0;
    record.fMapTopPad = 0;
    record.fMapBottomPad = 0;

    record.matrixRptStride = 64;
    record.matrixRptTimes = 64;
    record.matrixRptMode = 0;
    record.dstStride = 32;
    record.outputMPos = 0;

    record.dataType = DataType::DATA_B16;
    record.dstMemType = MemType::L0A;
    record.srcMemType = MemType::L1;

    uint64_t xm = 0;
    uint64_t xt = 0;
    SetConfByUint<15, 0>(xm, 32);//kstep
    SetConfByUint<31, 16>(xm, 32);//mstep
    SetConfByUint<47, 32>(xm, 0);//kstart
    SetConfByUint<63, 48>(xm, 0);//mstart
    SetConfByUint<5, 0>(xt, 1);//wstride
    SetConfByUint<11, 6>(xt, 1);//hstride
    SetConfByUint<19, 12>(xt, 64);//low bit of wsize
    SetConfByUint<44, 44>(xt, 0);//high bit of wsize
    SetConfByUint<27, 20>(xt, 64);//low bit of hsize
    SetConfByUint<45, 45>(xt, 0);//high bit of hsize
    SetConfByUint<35, 28>(xt, 1);//wDilation
    SetConfByUint<43, 36>(xt, 1);//hDilation
    SetConfByUint<46, 46>(xt, 0);//Transpose Control
    SetConfByUint<47, 47>(xt, 0);//SPR Control
    SetConfByUint<63, 48>(xt, 32);//channel size N

    uint64_t sprMatrix = 0;
    uint64_t sprL3dRpt = 0;
    SetConfByUint<15, 0>(sprMatrix, 512);//W
    SetConfByUint<31, 16>(sprMatrix, 512);//H
    SetConfByUint<39, 32>(sprMatrix, 0);//left padding
    SetConfByUint<47, 40>(sprMatrix, 0);//right padding
    SetConfByUint<55, 48>(sprMatrix, 0);//top padding
    SetConfByUint<63, 56>(sprMatrix, 0);//bottom padding

    SetConfByUint<15, 0>(sprL3dRpt, 64);//repeatStride
    SetConfByUint<23, 16>(sprL3dRpt, 64);//repeatTimes
    SetConfByUint<24, 24>(sprL3dRpt, 0);//repeatMode
    SetConfByUint<47, 32>(sprL3dRpt, 32);//dstStride
    SetConfByUint<63, 48>(sprL3dRpt, 0);//mStartPosition output

    __sanitizer_report_set_fmatrix(memInfo.data(), record.location.pc, 0, sprMatrix);
    __sanitizer_report_set_l3d_rpt(memInfo.data(), record.location.pc, 0, sprL3dRpt);

    auto dst = (__ca__ uint16_t *)(0x1000);
    auto src = (__cbuf__ uint16_t *)(0x1000);

    __sanitizer_report_img2colv2_cbuf_to_ca_b16(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        xm,
        xt);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::Load3DV2Record *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}


TEST(Load3DInstructions, load_cbuf_to_ca_3dv2_b8)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    Load3DV2Record record{};
    record.dst = 0x1000;
    record.src = 0x1000;

    record.matrixKStep = 32;
    record.matrixMStep = 32;
    record.matrixKPos = 0;
    record.matrixMPos = 0;

    record.filterWStride = 1;
    record.filterHStride = 1;
    record.filterW = 64;
    record.filterH = 64;
    record.filterWDilation = 1;
    record.filterHDilation = 1;
    record.transposeMode = 0;
    record.matrixMode = 0;
    record.fMapC = 32;

    record.fMapW = 512;
    record.fMapH = 512;
    record.fMapLeftPad = 0;
    record.fMapRightPad = 0;
    record.fMapTopPad = 0;
    record.fMapBottomPad = 0;

    record.matrixRptStride = 64;
    record.matrixRptTimes = 64;
    record.matrixRptMode = 0;
    record.dstStride = 32;
    record.outputMPos = 0;

    record.dataType = DataType::DATA_B8;
    record.dstMemType = MemType::L0A;
    record.srcMemType = MemType::L1;

    uint64_t xm = 0;
    uint64_t xt = 0;
    SetConfByUint<15, 0>(xm, 32);//kstep
    SetConfByUint<31, 16>(xm, 32);//mstep
    SetConfByUint<47, 32>(xm, 0);//kstart
    SetConfByUint<63, 48>(xm, 0);//mstart
    SetConfByUint<5, 0>(xt, 1);//wstride
    SetConfByUint<11, 6>(xt, 1);//hstride
    SetConfByUint<19, 12>(xt, 64);//low bit of wsize
    SetConfByUint<44, 44>(xt, 0);//high bit of wsize
    SetConfByUint<27, 20>(xt, 64);//low bit of hsize
    SetConfByUint<45, 45>(xt, 0);//high bit of hsize
    SetConfByUint<35, 28>(xt, 1);//wDilation
    SetConfByUint<43, 36>(xt, 1);//hDilation
    SetConfByUint<46, 46>(xt, 0);//Transpose Control
    SetConfByUint<47, 47>(xt, 0);//SPR Control
    SetConfByUint<63, 48>(xt, 32);//channel size N

    uint64_t sprMatrix = 0;
    uint64_t sprL3dRpt = 0;
    SetConfByUint<15, 0>(sprMatrix, 512);//W
    SetConfByUint<31, 16>(sprMatrix, 512);//H
    SetConfByUint<39, 32>(sprMatrix, 0);//left padding
    SetConfByUint<47, 40>(sprMatrix, 0);//right padding
    SetConfByUint<55, 48>(sprMatrix, 0);//top padding
    SetConfByUint<63, 56>(sprMatrix, 0);//bottom padding

    SetConfByUint<15, 0>(sprL3dRpt, 64);//repeatStride
    SetConfByUint<23, 16>(sprL3dRpt, 64);//repeatTimes
    SetConfByUint<24, 24>(sprL3dRpt, 0);//repeatMode
    SetConfByUint<47, 32>(sprL3dRpt, 32);//dstStride
    SetConfByUint<63, 48>(sprL3dRpt, 0);//mStartPosition output

    __sanitizer_report_set_fmatrix(memInfo.data(), record.location.pc, 0, sprMatrix);
    __sanitizer_report_set_l3d_rpt(memInfo.data(), record.location.pc, 0, sprL3dRpt);

    auto dst = (__ca__ uint8_t *)(0x1000);
    auto src = (__cbuf__ uint8_t *)(0x1000);

    __sanitizer_report_img2colv2_cbuf_to_ca_b8(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        xm,
        xt);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::Load3DV2Record *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(Load3DInstructions, load_cbuf_to_cb_3dv2_b32)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    Load3DV2Record record{};
    record.dst = 0x1000;
    record.src = 0x1000;

    record.matrixKStep = 32;
    record.matrixMStep = 32;
    record.matrixKPos = 0;
    record.matrixMPos = 0;

    record.filterWStride = 1;
    record.filterHStride = 1;
    record.filterW = 64;
    record.filterH = 64;
    record.filterWDilation = 1;
    record.filterHDilation = 1;
    record.transposeMode = 0;
    record.matrixMode = 0;
    record.fMapC = 32;

    record.fMapW = 512;
    record.fMapH = 512;
    record.fMapLeftPad = 0;
    record.fMapRightPad = 0;
    record.fMapTopPad = 0;
    record.fMapBottomPad = 0;

    record.matrixRptStride = 64;
    record.matrixRptTimes = 64;
    record.matrixRptMode = 0;
    record.dstStride = 32;
    record.outputMPos = 0;

    record.dataType = DataType::DATA_B32;
    record.dstMemType = MemType::L0B;
    record.srcMemType = MemType::L1;

    uint64_t xm = 0;
    uint64_t xt = 0;
    SetConfByUint<15, 0>(xm, 32);//kstep
    SetConfByUint<31, 16>(xm, 32);//mstep
    SetConfByUint<47, 32>(xm, 0);//kstart
    SetConfByUint<63, 48>(xm, 0);//mstart
    SetConfByUint<5, 0>(xt, 1);//wstride
    SetConfByUint<11, 6>(xt, 1);//hstride
    SetConfByUint<19, 12>(xt, 64);//low bit of wsize
    SetConfByUint<44, 44>(xt, 0);//high bit of wsize
    SetConfByUint<27, 20>(xt, 64);//low bit of hsize
    SetConfByUint<45, 45>(xt, 0);//high bit of hsize
    SetConfByUint<35, 28>(xt, 1);//wDilation
    SetConfByUint<43, 36>(xt, 1);//hDilation
    SetConfByUint<46, 46>(xt, 0);//Transpose Control
    SetConfByUint<47, 47>(xt, 0);//SPR Control
    SetConfByUint<63, 48>(xt, 32);//channel size N

    uint64_t sprMatrix = 0;
    uint64_t sprL3dRpt = 0;
    SetConfByUint<15, 0>(sprMatrix, 512);//W
    SetConfByUint<31, 16>(sprMatrix, 512);//H
    SetConfByUint<39, 32>(sprMatrix, 0);//left padding
    SetConfByUint<47, 40>(sprMatrix, 0);//right padding
    SetConfByUint<55, 48>(sprMatrix, 0);//top padding
    SetConfByUint<63, 56>(sprMatrix, 0);//bottom padding

    SetConfByUint<15, 0>(sprL3dRpt, 64);//repeatStride
    SetConfByUint<23, 16>(sprL3dRpt, 64);//repeatTimes
    SetConfByUint<24, 24>(sprL3dRpt, 0);//repeatMode
    SetConfByUint<47, 32>(sprL3dRpt, 32);//dstStride
    SetConfByUint<63, 48>(sprL3dRpt, 0);//mStartPosition output

    __sanitizer_report_set_fmatrix(memInfo.data(), record.location.pc, 0, sprMatrix);
    __sanitizer_report_set_l3d_rpt(memInfo.data(), record.location.pc, 0, sprL3dRpt);

    auto dst = (__cb__ uint32_t *)(0x1000);
    auto src = (__cbuf__ uint32_t *)(0x1000);

    __sanitizer_report_img2colv2_cbuf_to_cb_b32(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        xm,
        xt);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::Load3DV2Record *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(Load3DInstructions, load_cbuf_to_cb_3dv2_b16)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    Load3DV2Record record{};
    record.dst = 0x1000;
    record.src = 0x1000;

    record.matrixKStep = 32;
    record.matrixMStep = 32;
    record.matrixKPos = 0;
    record.matrixMPos = 0;

    record.filterWStride = 1;
    record.filterHStride = 1;
    record.filterW = 64;
    record.filterH = 64;
    record.filterWDilation = 1;
    record.filterHDilation = 1;
    record.transposeMode = 0;
    record.matrixMode = 0;
    record.fMapC = 32;

    record.fMapW = 512;
    record.fMapH = 512;
    record.fMapLeftPad = 0;
    record.fMapRightPad = 0;
    record.fMapTopPad = 0;
    record.fMapBottomPad = 0;

    record.matrixRptStride = 64;
    record.matrixRptTimes = 64;
    record.matrixRptMode = 0;
    record.dstStride = 32;
    record.outputMPos = 0;

    record.dataType = DataType::DATA_B16;
    record.dstMemType = MemType::L0B;
    record.srcMemType = MemType::L1;

    uint64_t xm = 0;
    uint64_t xt = 0;
    SetConfByUint<15, 0>(xm, 32);//kstep
    SetConfByUint<31, 16>(xm, 32);//mstep
    SetConfByUint<47, 32>(xm, 0);//kstart
    SetConfByUint<63, 48>(xm, 0);//mstart
    SetConfByUint<5, 0>(xt, 1);//wstride
    SetConfByUint<11, 6>(xt, 1);//hstride
    SetConfByUint<19, 12>(xt, 64);//low bit of wsize
    SetConfByUint<44, 44>(xt, 0);//high bit of wsize
    SetConfByUint<27, 20>(xt, 64);//low bit of hsize
    SetConfByUint<45, 45>(xt, 0);//high bit of hsize
    SetConfByUint<35, 28>(xt, 1);//wDilation
    SetConfByUint<43, 36>(xt, 1);//hDilation
    SetConfByUint<46, 46>(xt, 0);//Transpose Control
    SetConfByUint<47, 47>(xt, 0);//SPR Control
    SetConfByUint<63, 48>(xt, 32);//channel size N

    uint64_t sprMatrix = 0;
    uint64_t sprL3dRpt = 0;
    SetConfByUint<15, 0>(sprMatrix, 512);//W
    SetConfByUint<31, 16>(sprMatrix, 512);//H
    SetConfByUint<39, 32>(sprMatrix, 0);//left padding
    SetConfByUint<47, 40>(sprMatrix, 0);//right padding
    SetConfByUint<55, 48>(sprMatrix, 0);//top padding
    SetConfByUint<63, 56>(sprMatrix, 0);//bottom padding

    SetConfByUint<15, 0>(sprL3dRpt, 64);//repeatStride
    SetConfByUint<23, 16>(sprL3dRpt, 64);//repeatTimes
    SetConfByUint<24, 24>(sprL3dRpt, 0);//repeatMode
    SetConfByUint<47, 32>(sprL3dRpt, 32);//dstStride
    SetConfByUint<63, 48>(sprL3dRpt, 0);//mStartPosition output

    __sanitizer_report_set_fmatrix(memInfo.data(), record.location.pc, 0, sprMatrix);
    __sanitizer_report_set_l3d_rpt(memInfo.data(), record.location.pc, 0, sprL3dRpt);

    auto dst = (__cb__ uint16_t *)(0x1000);
    auto src = (__cbuf__ uint16_t *)(0x1000);

    __sanitizer_report_img2colv2_cbuf_to_cb_b16(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        xm,
        xt);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::Load3DV2Record *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(Load3DInstructions, load_cbuf_to_cb_3dv2_b8)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    Load3DV2Record record{};
    record.dst = 0x1000;
    record.src = 0x1000;

    record.matrixKStep = 32;
    record.matrixMStep = 32;
    record.matrixKPos = 0;
    record.matrixMPos = 0;

    record.filterWStride = 1;
    record.filterHStride = 1;
    record.filterW = 64;
    record.filterH = 64;
    record.filterWDilation = 1;
    record.filterHDilation = 1;
    record.transposeMode = 0;
    record.matrixMode = 0;
    record.fMapC = 32;

    record.fMapW = 512;
    record.fMapH = 512;
    record.fMapLeftPad = 0;
    record.fMapRightPad = 0;
    record.fMapTopPad = 0;
    record.fMapBottomPad = 0;

    record.matrixRptStride = 64;
    record.matrixRptTimes = 64;
    record.matrixRptMode = 0;
    record.dstStride = 32;
    record.outputMPos = 0;

    record.dataType = DataType::DATA_B8;
    record.dstMemType = MemType::L0B;
    record.srcMemType = MemType::L1;

    uint64_t xm = 0;
    uint64_t xt = 0;
    SetConfByUint<15, 0>(xm, 32);//kstep
    SetConfByUint<31, 16>(xm, 32);//mstep
    SetConfByUint<47, 32>(xm, 0);//kstart
    SetConfByUint<63, 48>(xm, 0);//mstart
    SetConfByUint<5, 0>(xt, 1);//wstride
    SetConfByUint<11, 6>(xt, 1);//hstride
    SetConfByUint<19, 12>(xt, 64);//low bit of wsize
    SetConfByUint<44, 44>(xt, 0);//high bit of wsize
    SetConfByUint<27, 20>(xt, 64);//low bit of hsize
    SetConfByUint<45, 45>(xt, 0);//high bit of hsize
    SetConfByUint<35, 28>(xt, 1);//wDilation
    SetConfByUint<43, 36>(xt, 1);//hDilation
    SetConfByUint<46, 46>(xt, 0);//Transpose Control
    SetConfByUint<47, 47>(xt, 0);//SPR Control
    SetConfByUint<63, 48>(xt, 32);//channel size N

    uint64_t sprMatrix = 0;
    uint64_t sprL3dRpt = 0;
    SetConfByUint<15, 0>(sprMatrix, 512);//W
    SetConfByUint<31, 16>(sprMatrix, 512);//H
    SetConfByUint<39, 32>(sprMatrix, 0);//left padding
    SetConfByUint<47, 40>(sprMatrix, 0);//right padding
    SetConfByUint<55, 48>(sprMatrix, 0);//top padding
    SetConfByUint<63, 56>(sprMatrix, 0);//bottom padding

    SetConfByUint<15, 0>(sprL3dRpt, 64);//repeatStride
    SetConfByUint<23, 16>(sprL3dRpt, 64);//repeatTimes
    SetConfByUint<24, 24>(sprL3dRpt, 0);//repeatMode
    SetConfByUint<47, 32>(sprL3dRpt, 32);//dstStride
    SetConfByUint<63, 48>(sprL3dRpt, 0);//mStartPosition output

    __sanitizer_report_set_fmatrix(memInfo.data(), record.location.pc, 0, sprMatrix);
    __sanitizer_report_set_l3d_rpt(memInfo.data(), record.location.pc, 0, sprL3dRpt);

    auto dst = (__cb__ uint8_t *)(0x1000);
    auto src = (__cbuf__ uint8_t *)(0x1000);

    __sanitizer_report_img2colv2_cbuf_to_cb_b8(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        xm,
        xt);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::Load3DV2Record *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

namespace CubeMteRegistersTest {

constexpr uint64_t MEM_INFO_SIZE = 1024 * 1024 * 10;

TEST(DbiCubeMteRegisters, set_mte2_src_para_expect_get_correct_register_value)
{
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    uint64_t value = 0x12345678ABCDEF00ULL;
    __sanitizer_report_set_mte2_src_para(memInfo.data(), 0, 0, value);

    RecordGlobalHead const *globalHead = reinterpret_cast<RecordGlobalHead const *>(memInfo.data());
    int64_t regIdx = GetRegisterIdx();
    ASSERT_TRUE(globalHead->registers[regIdx].sprMTE2SrcPara == value);
}

TEST(DbiCubeMteRegisters, set_loop3_para_expect_get_correct_register_value)
{
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    uint64_t value = 0x87654321FEDCBA98ULL;
    __sanitizer_report_set_loop3_para(memInfo.data(), 0, 0, value);

    RecordGlobalHead const *globalHead = reinterpret_cast<RecordGlobalHead const *>(memInfo.data());
    int64_t regIdx = GetRegisterIdx();
    ASSERT_TRUE(globalHead->registers[regIdx].sprLoop3Para == value);
}

TEST(DbiCubeMteRegisters, set_channel_para_expect_get_correct_register_value)
{
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    uint64_t value = 0x1111222233334444ULL;
    __sanitizer_report_set_channel_para(memInfo.data(), 0, 0, value);

    RecordGlobalHead const *globalHead = reinterpret_cast<RecordGlobalHead const *>(memInfo.data());
    int64_t regIdx = GetRegisterIdx();
    ASSERT_TRUE(globalHead->registers[regIdx].sprChannelPara == value);
}

TEST(DbiCubeMteRegisters, set_fmatrix_expect_get_correct_register_value)
{
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    uint64_t value = 0xAAAABBBBCCCCDDDDULL;
    __sanitizer_report_set_fmatrix(memInfo.data(), 0, 0, value);

    RecordGlobalHead const *globalHead = reinterpret_cast<RecordGlobalHead const *>(memInfo.data());
    int64_t regIdx = GetRegisterIdx();
    ASSERT_TRUE(globalHead->registers[regIdx].sprFmatrix == value);
}

TEST(DbiCubeMteRegisters, set_fmatrix_b_expect_get_correct_register_value)
{
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    uint64_t value = 0x5555666677778888ULL;
    __sanitizer_report_set_fmatrix_b(memInfo.data(), 0, 0, value);

    RecordGlobalHead const *globalHead = reinterpret_cast<RecordGlobalHead const *>(memInfo.data());
    int64_t regIdx = GetRegisterIdx();
    ASSERT_TRUE(globalHead->registers[regIdx].sprFmatrixB == value);
}

TEST(DbiCubeMteRegisters, set_fmatrix_dual_0_expect_get_correct_register_value)
{
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    uint64_t value = 0x9999000011112222ULL;
    __sanitizer_report_set_fmatrix_dual_0(memInfo.data(), 0, 0, value);

    RecordGlobalHead const *globalHead = reinterpret_cast<RecordGlobalHead const *>(memInfo.data());
    int64_t regIdx = GetRegisterIdx();
    ASSERT_TRUE(globalHead->registers[regIdx].sprFmatrixDual0 == value);
}

TEST(DbiCubeMteRegisters, set_fmatrix_dual_1_expect_get_correct_register_value)
{
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    uint64_t value = 0x3333444455556666ULL;
    __sanitizer_report_set_fmatrix_dual_1(memInfo.data(), 0, 0, value);

    RecordGlobalHead const *globalHead = reinterpret_cast<RecordGlobalHead const *>(memInfo.data());
    int64_t regIdx = GetRegisterIdx();
    ASSERT_TRUE(globalHead->registers[regIdx].sprFmatrixDual1 == value);
}

TEST(DbiCubeMteRegisters, set_l3d_rpt_expect_get_correct_register_value)
{
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    uint64_t value = 0x7777888899990000ULL;
    __sanitizer_report_set_l3d_rpt(memInfo.data(), 0, 0, value);

    RecordGlobalHead const *globalHead = reinterpret_cast<RecordGlobalHead const *>(memInfo.data());
    int64_t regIdx = GetRegisterIdx();
    ASSERT_TRUE(globalHead->registers[regIdx].sprL3dRpt == value);
}

TEST(DbiCubeMteRegisters, set_l3d_rpt_b_expect_get_correct_register_value)
{
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    uint64_t value = 0xABCDEF0123456789ULL;
    __sanitizer_report_set_l3d_rpt_b(memInfo.data(), 0, 0, value);

    RecordGlobalHead const *globalHead = reinterpret_cast<RecordGlobalHead const *>(memInfo.data());
    int64_t regIdx = GetRegisterIdx();
    ASSERT_TRUE(globalHead->registers[regIdx].sprL3dRptB == value);
}

TEST(DbiCubeMteRegisters, set_padding_expect_get_correct_register_value)
{
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    uint64_t value = 0xFEDCBA9876543210ULL;
    __sanitizer_report_set_padding(memInfo.data(), 0, 0, value);

    RecordGlobalHead const *globalHead = reinterpret_cast<RecordGlobalHead const *>(memInfo.data());
    int64_t regIdx = GetRegisterIdx();
    ASSERT_TRUE(globalHead->registers[regIdx].sprPadding == value);
}

TEST(DbiCubeMteRegisters, set_padding_b_expect_get_correct_register_value)
{
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    uint64_t value = 0x0F0F0F0F0F0F0F0FULL;
    __sanitizer_report_set_padding_b(memInfo.data(), 0, 0, value);

    RecordGlobalHead const *globalHead = reinterpret_cast<RecordGlobalHead const *>(memInfo.data());
    int64_t regIdx = GetRegisterIdx();
    ASSERT_TRUE(globalHead->registers[regIdx].sprPaddingB == value);
}

TEST(DbiCubeMteRegisters, multiple_register_sets_expect_correct_values)
{
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    __sanitizer_report_set_mte2_src_para(memInfo.data(), 0, 0, 0x1111ULL);
    __sanitizer_report_set_loop3_para(memInfo.data(), 0, 0, 0x2222ULL);
    __sanitizer_report_set_channel_para(memInfo.data(), 0, 0, 0x3333ULL);
    __sanitizer_report_set_fmatrix(memInfo.data(), 0, 0, 0x4444ULL);
    __sanitizer_report_set_fmatrix_b(memInfo.data(), 0, 0, 0x5555ULL);
    __sanitizer_report_set_fmatrix_dual_0(memInfo.data(), 0, 0, 0x6666ULL);
    __sanitizer_report_set_fmatrix_dual_1(memInfo.data(), 0, 0, 0x7777ULL);
    __sanitizer_report_set_l3d_rpt(memInfo.data(), 0, 0, 0x8888ULL);
    __sanitizer_report_set_l3d_rpt_b(memInfo.data(), 0, 0, 0x9999ULL);
    __sanitizer_report_set_padding(memInfo.data(), 0, 0, 0xAAAAULL);
    __sanitizer_report_set_padding_b(memInfo.data(), 0, 0, 0xBBBBULL);

    RecordGlobalHead const *globalHead = reinterpret_cast<RecordGlobalHead const *>(memInfo.data());
    int64_t regIdx = GetRegisterIdx();
    ASSERT_TRUE(globalHead->registers[regIdx].sprMTE2SrcPara == 0x1111ULL);
    ASSERT_TRUE(globalHead->registers[regIdx].sprLoop3Para == 0x2222ULL);
    ASSERT_TRUE(globalHead->registers[regIdx].sprChannelPara == 0x3333ULL);
    ASSERT_TRUE(globalHead->registers[regIdx].sprFmatrix == 0x4444ULL);
    ASSERT_TRUE(globalHead->registers[regIdx].sprFmatrixB == 0x5555ULL);
    ASSERT_TRUE(globalHead->registers[regIdx].sprFmatrixDual0 == 0x6666ULL);
    ASSERT_TRUE(globalHead->registers[regIdx].sprFmatrixDual1 == 0x7777ULL);
    ASSERT_TRUE(globalHead->registers[regIdx].sprL3dRpt == 0x8888ULL);
    ASSERT_TRUE(globalHead->registers[regIdx].sprL3dRptB == 0x9999ULL);
    ASSERT_TRUE(globalHead->registers[regIdx].sprPadding == 0xAAAAULL);
    ASSERT_TRUE(globalHead->registers[regIdx].sprPaddingB == 0xBBBBULL);
}

}
