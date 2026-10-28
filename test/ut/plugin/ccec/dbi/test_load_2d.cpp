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
#include "plugin/ccec/dbi/probes/load_2d.cpp"

TEST(Load2DInstructions, load_gm_to_l0_and_l1_expect_record) {
    auto callAndCheck = [&](auto &&func) {
        std::vector<uint8_t> memInfo = CreateMemInfo();
        func(memInfo.data());
        uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
        ASSERT_EQ(*reinterpret_cast<RecordType *>(ptr), RecordType::LOAD_2D);
    };

    callAndCheck([&](uint8_t *m) {
        __sanitizer_report_load_gm_to_ca(m, 0, 0,
            reinterpret_cast<__ca__ void *>(0x100), reinterpret_cast<__gm__ void *>(0x200), 0, static_cast<addr_cal_mode_t>(0));
    });
    callAndCheck([&](uint8_t *m) {
        __sanitizer_report_load_gm_to_cb(m, 0, 0,
            reinterpret_cast<__cb__ void *>(0x100), reinterpret_cast<__gm__ void *>(0x200), 0, static_cast<addr_cal_mode_t>(0));
    });
    callAndCheck([&](uint8_t *m) {
        __sanitizer_report_load_gm_to_cbuf(m, 0, 0,
            reinterpret_cast<__cbuf__ void *>(0x100), reinterpret_cast<__gm__ void *>(0x200), 0, static_cast<addr_cal_mode_t>(0));
    });
    callAndCheck([&](uint8_t *m) {
        __sanitizer_report_load_gm_to_cbuf_2dv2(m, 0, 0,
            reinterpret_cast<__cbuf__ void *>(0x100), reinterpret_cast<__gm__ void *>(0x200), 0, 0);
    });
}

TEST(Load2DInstructions, load_cbuf_to_ca_2dv2_b32)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    LoadL12DRecord record{};
    record.dst = 0x1000;
    record.src = 0x1000;
    record.mStartPosition = 2;
    record.kStartPosition = 2;
    record.mStep = 2;
    record.kStep = 3;
    record.srcStride = 5;
    record.dstStride = 3;
    record.dstMemType = MemType::L0A;
    record.transposeMode = TransposeMode::DISABLE;
    record.detailedDataType = DetailedDataType::B32;

    __ca__ int32_t *dst = (__ca__ int32_t *)0x1000;
    __cbuf__ int32_t *src = (__cbuf__ int32_t *)0x1000;
    uint64_t config0 = 0;
    uint64_t config1 = 0;

    SetConfByUint<15, 0>(config0, 2); // M_Start_Position
    SetConfByUint<31, 16>(config0, 2); // K_Start_Position
    SetConfByUint<39, 32>(config0, 2); // M_step
    SetConfByUint<47, 40>(config0, 3); // K_step
    SetConfByUint<63, 48>(config0, 0); // reserved

    SetConfByUint<15, 0>(config1, 5); // src_stride
    SetConfByUint<31, 16>(config1, 3); // dst_stride
    SetConfByUint<63, 32>(config1, 0); // reserved

    __sanitizer_report_load_cbuf_to_ca_2dv2_b32(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        config0,
        config1,
        false);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::LoadL12DRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(Load2DInstructions, load_cbuf_to_ca_2dv2_b16)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    LoadL12DRecord record{};
    record.dst = 0x1000;
    record.src = 0x1000;
    record.mStartPosition = 2;
    record.kStartPosition = 2;
    record.mStep = 2;
    record.kStep = 3;
    record.srcStride = 5;
    record.dstStride = 3;
    record.dstMemType = MemType::L0A;
    record.transposeMode = TransposeMode::DISABLE;
    record.detailedDataType = DetailedDataType::B16;

    __ca__ int16_t *dst = (__ca__ int16_t *)0x1000;
    __cbuf__ int16_t *src = (__cbuf__ int16_t *)0x1000;
    uint64_t config0 = 0;
    uint64_t config1 = 0;

    SetConfByUint<15, 0>(config0, 2); // M_Start_Position
    SetConfByUint<31, 16>(config0, 2); // K_Start_Position
    SetConfByUint<39, 32>(config0, 2); // M_step
    SetConfByUint<47, 40>(config0, 3); // K_step
    SetConfByUint<63, 48>(config0, 0); // reserved

    SetConfByUint<15, 0>(config1, 5); // src_stride
    SetConfByUint<31, 16>(config1, 3); // dst_stride
    SetConfByUint<63, 32>(config1, 0); // reserved

    __sanitizer_report_load_cbuf_to_ca_2dv2_b16(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        config0,
        config1,
        false);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::LoadL12DRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(Load2DInstructions, load_cbuf_to_ca_2dv2_b8)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    LoadL12DRecord record{};
    record.dst = 0x1000;
    record.src = 0x1000;
    record.mStartPosition = 2;
    record.kStartPosition = 2;
    record.mStep = 2;
    record.kStep = 3;
    record.srcStride = 5;
    record.dstStride = 3;
    record.dstMemType = MemType::L0A;
    record.transposeMode = TransposeMode::DISABLE;
    record.detailedDataType = DetailedDataType::B8;

    __ca__ int8_t *dst = (__ca__ int8_t *)0x1000;
    __cbuf__ int8_t *src = (__cbuf__ int8_t *)0x1000;
    uint64_t config0 = 0;
    uint64_t config1 = 0;

    SetConfByUint<15, 0>(config0, 2); // M_Start_Position
    SetConfByUint<31, 16>(config0, 2); // K_Start_Position
    SetConfByUint<39, 32>(config0, 2); // M_step
    SetConfByUint<47, 40>(config0, 3); // K_step
    SetConfByUint<63, 48>(config0, 0); // reserved

    SetConfByUint<15, 0>(config1, 5); // src_stride
    SetConfByUint<31, 16>(config1, 3); // dst_stride
    SetConfByUint<63, 32>(config1, 0); // reserved

    __sanitizer_report_load_cbuf_to_ca_2dv2_b8(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        config0,
        config1,
        false);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::LoadL12DRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(Load2DInstructions, load_cbuf_to_ca_2dv2_b4)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    LoadL12DRecord record{};
    record.dst = 0x1000;
    record.src = 0x1000;
    record.mStartPosition = 2;
    record.kStartPosition = 2;
    record.mStep = 2;
    record.kStep = 3;
    record.srcStride = 5;
    record.dstStride = 3;
    record.dstMemType = MemType::L0A;
    record.transposeMode = TransposeMode::DISABLE;
    record.detailedDataType = DetailedDataType::B4;

    __ca__ half *dst = (__ca__ half *)0x1000;
    __cbuf__ half *src = (__cbuf__ half *)0x1000;
    uint64_t config0 = 0;
    uint64_t config1 = 0;

    SetConfByUint<15, 0>(config0, 2); // M_Start_Position
    SetConfByUint<31, 16>(config0, 2); // K_Start_Position
    SetConfByUint<39, 32>(config0, 2); // M_step
    SetConfByUint<47, 40>(config0, 3); // K_step
    SetConfByUint<63, 48>(config0, 0); // reserved

    SetConfByUint<15, 0>(config1, 5); // src_stride
    SetConfByUint<31, 16>(config1, 3); // dst_stride
    SetConfByUint<63, 32>(config1, 0); // reserved

    __sanitizer_report_load_cbuf_to_ca_2dv2_b4(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        config0,
        config1,
        false);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::LoadL12DRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(Load2DInstructions, load_cbuf_to_cb_2dv2_b32)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    LoadL12DRecord record{};
    record.dst = 0x1000;
    record.src = 0x1000;
    record.mStartPosition = 2;
    record.kStartPosition = 2;
    record.mStep = 2;
    record.kStep = 3;
    record.srcStride = 5;
    record.dstStride = 3;
    record.dstMemType = MemType::L0B;
    record.transposeMode = TransposeMode::DISABLE;
    record.detailedDataType = DetailedDataType::B32;

    __cb__ int32_t *dst = (__cb__ int32_t *)0x1000;
    __cbuf__ int32_t *src = (__cbuf__ int32_t *)0x1000;
    uint64_t config0 = 0;
    uint64_t config1 = 0;

    SetConfByUint<15, 0>(config0, 2); // M_Start_Position
    SetConfByUint<31, 16>(config0, 2); // K_Start_Position
    SetConfByUint<39, 32>(config0, 2); // M_step
    SetConfByUint<47, 40>(config0, 3); // K_step
    SetConfByUint<63, 48>(config0, 0); // reserved

    SetConfByUint<15, 0>(config1, 5); // src_stride
    SetConfByUint<31, 16>(config1, 3); // dst_stride
    SetConfByUint<63, 32>(config1, 0); // reserved

    __sanitizer_report_load_cbuf_to_cb_2dv2_b32(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        config0,
        config1,
        false);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::LoadL12DRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(Load2DInstructions, load_cbuf_to_cb_2dv2_b16)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    LoadL12DRecord record{};
    record.dst = 0x1000;
    record.src = 0x1000;
    record.mStartPosition = 2;
    record.kStartPosition = 2;
    record.mStep = 2;
    record.kStep = 3;
    record.srcStride = 5;
    record.dstStride = 3;
    record.dstMemType = MemType::L0B;
    record.transposeMode = TransposeMode::DISABLE;
    record.detailedDataType = DetailedDataType::B16;

    __cb__ int16_t *dst = (__cb__ int16_t *)0x1000;
    __cbuf__ int16_t *src = (__cbuf__ int16_t *)0x1000;
    uint64_t config0 = 0;
    uint64_t config1 = 0;

    SetConfByUint<15, 0>(config0, 2); // M_Start_Position
    SetConfByUint<31, 16>(config0, 2); // K_Start_Position
    SetConfByUint<39, 32>(config0, 2); // M_step
    SetConfByUint<47, 40>(config0, 3); // K_step
    SetConfByUint<63, 48>(config0, 0); // reserved

    SetConfByUint<15, 0>(config1, 5); // src_stride
    SetConfByUint<31, 16>(config1, 3); // dst_stride
    SetConfByUint<63, 32>(config1, 0); // reserved

    __sanitizer_report_load_cbuf_to_cb_2dv2_b16(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        config0,
        config1,
        false);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::LoadL12DRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(Load2DInstructions, load_cbuf_to_cb_2dv2_b8)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    LoadL12DRecord record{};
    record.dst = 0x1000;
    record.src = 0x1000;
    record.mStartPosition = 2;
    record.kStartPosition = 2;
    record.mStep = 2;
    record.kStep = 3;
    record.srcStride = 5;
    record.dstStride = 3;
    record.dstMemType = MemType::L0B;
    record.transposeMode = TransposeMode::DISABLE;
    record.detailedDataType = DetailedDataType::B8;

    __cb__ int8_t *dst = (__cb__ int8_t *)0x1000;
    __cbuf__ int8_t *src = (__cbuf__ int8_t *)0x1000;
    uint64_t config0 = 0;
    uint64_t config1 = 0;

    SetConfByUint<15, 0>(config0, 2); // M_Start_Position
    SetConfByUint<31, 16>(config0, 2); // K_Start_Position
    SetConfByUint<39, 32>(config0, 2); // M_step
    SetConfByUint<47, 40>(config0, 3); // K_step
    SetConfByUint<63, 48>(config0, 0); // reserved

    SetConfByUint<15, 0>(config1, 5); // src_stride
    SetConfByUint<31, 16>(config1, 3); // dst_stride
    SetConfByUint<63, 32>(config1, 0); // reserved

    __sanitizer_report_load_cbuf_to_cb_2dv2_b8(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        config0,
        config1,
        false);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::LoadL12DRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(Load2DInstructions, load_cbuf_to_cb_2dv2_b4)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    LoadL12DRecord record{};
    record.dst = 0x1000;
    record.src = 0x1000;
    record.mStartPosition = 2;
    record.kStartPosition = 2;
    record.mStep = 2;
    record.kStep = 3;
    record.srcStride = 5;
    record.dstStride = 3;
    record.dstMemType = MemType::L0B;
    record.transposeMode = TransposeMode::DISABLE;
    record.detailedDataType = DetailedDataType::B4;

    __cb__ half *dst = (__cb__ half *)0x1000;
    __cbuf__ half *src = (__cbuf__ half *)0x1000;
    uint64_t config0 = 0;
    uint64_t config1 = 0;

    SetConfByUint<15, 0>(config0, 2); // M_Start_Position
    SetConfByUint<31, 16>(config0, 2); // K_Start_Position
    SetConfByUint<39, 32>(config0, 2); // M_step
    SetConfByUint<47, 40>(config0, 3); // K_step
    SetConfByUint<63, 48>(config0, 0); // reserved

    SetConfByUint<15, 0>(config1, 5); // src_stride
    SetConfByUint<31, 16>(config1, 3); // dst_stride
    SetConfByUint<63, 32>(config1, 0); // reserved

    __sanitizer_report_load_cbuf_to_cb_2dv2_b4(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        config0,
        config1,
        false);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::LoadL12DRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(Load2DInstructions, load_cbuf_to_cb_2d_transpose_b4)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    LoadL12DTransposeRecord record{};
    record.dst = 0x1000;
    record.src = 0x1000;
    record.repeat = 2;
    record.srcStride = 4;
    record.dstStride = 8;
    record.srcFracStride = 1;
    record.dstFracStride = 1;
    record.detailedDataType = DetailedDataType::B4;

    __cb__ uint64_t *dst = (__cb__ uint64_t *)0x1000;
    __cbuf__ uint64_t *src = (__cbuf__ uint64_t *)0x1000;
    uint64_t config0 = 0;
    uint64_t config1 = 0;

    SetConfByUint<15, 0>(config0, 0); // indexID
    SetConfByUint<23, 16>(config0, 2); // repeat
    SetConfByUint<39, 24>(config0, 4); // srcStride
    SetConfByUint<43, 40>(config0, 0); // reserved
    SetConfByUint<59, 44>(config0, 7); // dstStride - 1
    SetConfByUint<62, 60>(config0, 0); // reserved
    SetConfByUint<63, 63>(config0, 0); // 地址更新模式控制位，不影响分形步幅计算

    SetConfByUint<15, 0>(config1, 0); // dstFracStride - 1
    SetConfByUint<31, 16>(config1, 0); // srcFracStride - 1
    SetConfByUint<63, 32>(config1, 0); // reserved

    __sanitizer_report_load_cbuf_to_cb_2d_transpose_b4(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        config0,
        config1);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::LoadL12DTransposeRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(Load2DInstructions, load_cbuf_to_cb_2d_transpose_b8)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    LoadL12DTransposeRecord record{};
    record.dst = 0x1000;
    record.src = 0x1000;
    record.repeat = 3;
    record.srcStride = 2;
    record.dstStride = 6;
    record.srcFracStride = 1;
    record.dstFracStride = 1;
    record.detailedDataType = DetailedDataType::B8;

    __cb__ uint64_t *dst = (__cb__ uint64_t *)0x1000;
    __cbuf__ uint64_t *src = (__cbuf__ uint64_t *)0x1000;
    uint64_t config0 = 0;
    uint64_t config1 = 0;

    SetConfByUint<15, 0>(config0, 0); // indexID
    SetConfByUint<23, 16>(config0, 3); // repeat
    SetConfByUint<39, 24>(config0, 2); // srcStride
    SetConfByUint<43, 40>(config0, 0); // reserved
    SetConfByUint<59, 44>(config0, 5); // dstStride - 1
    SetConfByUint<62, 60>(config0, 0); // reserved
    SetConfByUint<63, 63>(config0, 0); // 地址更新模式控制位，不影响分形步幅计算

    SetConfByUint<15, 0>(config1, 0); // dstFracStride - 1
    SetConfByUint<31, 16>(config1, 0); // srcFracStride - 1
    SetConfByUint<63, 32>(config1, 0); // reserved

    __sanitizer_report_load_cbuf_to_cb_2d_transpose_b8(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        config0,
        config1);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::LoadL12DTransposeRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(Load2DInstructions, load_cbuf_to_cb_2d_transpose_b16)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    LoadL12DTransposeRecord record{};
    record.dst = 0x1000;
    record.src = 0x1000;
    record.repeat = 3;
    record.srcStride = 1;
    record.dstStride = 3;
    record.srcFracStride = 1;
    record.dstFracStride = 1;
    record.detailedDataType = DetailedDataType::B16;

    __cb__ uint64_t *dst = (__cb__ uint64_t *)0x1000;
    __cbuf__ uint64_t *src = (__cbuf__ uint64_t *)0x1000;
    uint64_t config0 = 0;
    uint64_t config1 = 0;

    SetConfByUint<15, 0>(config0, 0); // indexID
    SetConfByUint<23, 16>(config0, 3); // repeat
    SetConfByUint<39, 24>(config0, 1); // srcStride
    SetConfByUint<43, 40>(config0, 0); // reserved
    SetConfByUint<59, 44>(config0, 2); // dstStride - 1
    SetConfByUint<62, 60>(config0, 0); // reserved
    SetConfByUint<63, 63>(config0, 0); // 地址更新模式控制位，不影响分形步幅计算

    SetConfByUint<15, 0>(config1, 0); // dstFracStride - 1
    SetConfByUint<31, 16>(config1, 0); // srcFracStride - 1
    SetConfByUint<63, 32>(config1, 0); // reserved

    __sanitizer_report_load_cbuf_to_cb_2d_transpose_b16(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        config0,
        config1);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::LoadL12DTransposeRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(Load2DInstructions, load_cbuf_to_cb_2d_transpose_b32)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    LoadL12DTransposeRecord record{};
    record.dst = 0x1000;
    record.src = 0x1000;
    record.repeat = 3;
    record.srcStride = 4;
    record.dstStride = 1;
    record.srcFracStride = 2;
    record.dstFracStride = 4;
    record.detailedDataType = DetailedDataType::B32;

    __cb__ uint64_t *dst = (__cb__ uint64_t *)0x1000;
    __cbuf__ uint64_t *src = (__cbuf__ uint64_t *)0x1000;
    uint64_t config0 = 0;
    uint64_t config1 = 0;

    SetConfByUint<15, 0>(config0, 0); // indexID
    SetConfByUint<23, 16>(config0, 3); // repeat
    SetConfByUint<39, 24>(config0, 4); // srcStride
    SetConfByUint<43, 40>(config0, 0); // reserved
    SetConfByUint<59, 44>(config0, 0); // dstStride - 1
    SetConfByUint<62, 60>(config0, 0); // reserved
    SetConfByUint<63, 63>(config0, 0); // 地址更新模式控制位，不影响分形步幅计算

    SetConfByUint<15, 0>(config1, 3); // dstFracStride - 1
    SetConfByUint<31, 16>(config1, 1); // srcFracStride - 1
    SetConfByUint<63, 32>(config1, 0); // reserved

    __sanitizer_report_load_cbuf_to_cb_2d_transpose_b32(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        config0,
        config1);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::LoadL12DTransposeRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(Load2DInstructions, load_cbuf_to_ca_mx_2dv2)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    LoadL1Mx2DRecord record{};
    record.dst = 0x10;
    record.src = 0x1000;
    record.xStartPosition = 2;
    record.yStartPosition = 2;
    record.xStep = 2;
    record.yStep = 3;
    record.srcStride = 5;
    record.dstStride = 3;
    record.dstMemType = MemType::L0A;

    uint64_t dst = (uint64_t)0x10;
    __cbuf__ uint8_t *src = (__cbuf__ uint8_t *)0x1000;
    uint64_t config0 = 0;
    uint64_t config1 = 0;

    SetConfByUint<15, 0>(config0, 2); // X_Start_Position
    SetConfByUint<31, 16>(config0, 2); // Y_Start_Position
    SetConfByUint<39, 32>(config0, 2); // X_step
    SetConfByUint<47, 40>(config0, 3); // Y_step
    SetConfByUint<63, 48>(config0, 0); // reserved

    SetConfByUint<15, 0>(config1, 5); // src_stride
    SetConfByUint<31, 16>(config1, 3); // dst_stride
    SetConfByUint<63, 32>(config1, 0); // reserved

    __sanitizer_report_load_cbuf_to_ca_mx_2dv2(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        config0,
        config1);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::LoadL1Mx2DRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(Load2DInstructions, load_cbuf_to_cb_mx_2dv2)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    LoadL1Mx2DRecord record{};
    record.dst = 0x10;
    record.src = 0x1000;
    record.xStartPosition = 2;
    record.yStartPosition = 2;
    record.xStep = 2;
    record.yStep = 3;
    record.srcStride = 5;
    record.dstStride = 3;
    record.dstMemType = MemType::L0B;

    uint64_t dst = (uint64_t)0x10;
    __cbuf__ uint8_t *src = (__cbuf__ uint8_t *)0x1000;
    uint64_t config0 = 0;
    uint64_t config1 = 0;

    SetConfByUint<15, 0>(config0, 2); // X_Start_Position
    SetConfByUint<31, 16>(config0, 2); // Y_Start_Position
    SetConfByUint<39, 32>(config0, 2); // X_step
    SetConfByUint<47, 40>(config0, 3); // Y_step
    SetConfByUint<63, 48>(config0, 0); // reserved

    SetConfByUint<15, 0>(config1, 5); // src_stride
    SetConfByUint<31, 16>(config1, 3); // dst_stride
    SetConfByUint<63, 32>(config1, 0); // reserved

    __sanitizer_report_load_cbuf_to_cb_mx_2dv2(memInfo.data(),
        record.location.pc,
        0,
        dst,
        src,
        config0,
        config1);

    uint8_t *ptr = memInfo.data();
    ptr += sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::LoadL1Mx2DRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}
