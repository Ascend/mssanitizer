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
#define __NPU_ARCH__ 3101
#include "plugin/ccec/dbi/probes/vector_instructions.cpp"

TEST(DbiVecInstructions, ldva_expect_get_correct_records)
{
    LoadStoreRecord record{};
    record.addr = RandInt(0, 0x2000);
    record.size = 32U; // 固定读32B
    record.space = AddressSpace::UB;
    record.alignSize = 32U;
    std::vector<uint8_t> memInfo = CreateMemInfo();

    __sanitizer_report_ldva(memInfo.data(), record.location.pc, 0,
        0,
        record.addr,
        0
    );

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::LDVA);
    auto x = reinterpret_cast<Sanitizer::LoadStoreRecord*>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

ScatterVnchwconvRecord CreateScatterVnchwconvRecord()
{
    ScatterVnchwconvRecord record{};
    record.repeat = 2;
    record.dstStride = 32;
    record.srcStride = 64;
    record.dstHighHalf = 0;
    record.srcHighHalf = 0;
    record.dataType = DataType::DATA_B8;
    return record;
}

TEST(DbiVecInstructions, vnchwconva5_b8_expect_get_correct_records)
{
    ScatterVnchwconvRecord record = CreateScatterVnchwconvRecord();
    record.dstHighHalf = 1;
    uint64_t config = 0;
    SetConfByUint<63, 56>(config, record.repeat);
    SetConfByUint<15, 0>(config, record.dstStride);
    SetConfByUint<31, 16>(config, record.srcStride);
    std::vector<uint8_t> memInfo = CreateMemInfo();
    __sanitizer_report_scatter_vnchwconv_b8(memInfo.data(), record.location.pc, 0,
        0,
        0,
        config,
        true,
        false
    );

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::SCATTERVNCHWCONV_A5);
    auto x = reinterpret_cast<ScatterVnchwconvRecord*>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(DbiVecInstructions, vnchwconva5_b16_expect_get_correct_records)
{
    ScatterVnchwconvRecord record = CreateScatterVnchwconvRecord();
    record.dataType = DataType::DATA_B16;
    uint64_t config = 0;
    SetConfByUint<63, 56>(config, record.repeat);
    SetConfByUint<15, 0>(config, record.dstStride);
    SetConfByUint<31, 16>(config, record.srcStride);
    std::vector<uint8_t> memInfo = CreateMemInfo();
    __sanitizer_report_scatter_vnchwconv_b16(memInfo.data(), record.location.pc, 0,
        0,
        0,
        config
    );

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::SCATTERVNCHWCONV_A5);
    auto x = reinterpret_cast<ScatterVnchwconvRecord*>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(DbiVecInstructions, vnchwconva5_b32_expect_get_correct_records)
{
    ScatterVnchwconvRecord record = CreateScatterVnchwconvRecord();
    record.dataType = DataType::DATA_B32;
    uint64_t config = 0;
    SetConfByUint<63, 56>(config, record.repeat);
    SetConfByUint<15, 0>(config, record.dstStride);
    SetConfByUint<31, 16>(config, record.srcStride);
    std::vector<uint8_t> memInfo = CreateMemInfo();
    __sanitizer_report_scatter_vnchwconv_b32(memInfo.data(), record.location.pc, 0,
        0,
        0,
        config
    );

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::SCATTERVNCHWCONV_A5);
    auto x = reinterpret_cast<ScatterVnchwconvRecord*>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(DbiVecInstructions, vbs32_expect_get_correct_records)
{
    Vbs32Record record{};
    record.dataType = DataType::DATA_B16;
    record.dst = 0x100;
    record.src0 = 0x200;
    record.src1 = 0x300;
    record.repeat = 10;

    uint64_t config = 0;
    SetConfByUint<63, 56>(config, record.repeat);

    std::vector<uint8_t> memInfo = CreateMemInfo();
    __sanitizer_report_vbs32_f16(memInfo.data(), record.location.pc, 0,
        reinterpret_cast<void*>(record.dst),
        reinterpret_cast<void*>(record.src0),
        reinterpret_cast<void*>(record.src1),
        config
    );

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::VBS32_A5);
    ptr += sizeof(RecordType);
    auto x = reinterpret_cast<Vbs32Record*>(ptr);
    record.location = x->location;
    ASSERT_TRUE(*x == record);

    record.dataType = DataType::DATA_B32;
    __sanitizer_report_vbs32_f32(memInfo.data(), record.location.pc, 0,
        reinterpret_cast<void*>(record.dst),
        reinterpret_cast<void*>(record.src0),
        reinterpret_cast<void*>(record.src1),
        config
    );
    ptr += sizeof(Vbs32Record);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::VBS32_A5);
    ptr += sizeof(RecordType);
    x = reinterpret_cast<Vbs32Record*>(ptr);
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(DbiVecInstructions, vms4v2_expect_get_correct_records)
{
    Vms4v2RecordA5 record{};
    record.detailedDataType = DetailedDataType::F16;
    record.dst = 0x2000;
    record.src[0] = 0x80;
    record.src[1] = 0x100;
    record.src[2] = 0x180;
    record.src[3] = 0x200;
    record.elementNum[0] = 0;
    record.elementNum[1] = 1;
    record.elementNum[2] = 2;
    record.elementNum[3] = 3;
    record.repeat = 10;
    record.validMask = 0b1111;
    record.isAllStored = 1;

    uint64_t src = 0x40003000200010;
    uint64_t xm = 0x3000200010000;
    uint64_t xt = 0;
    SetConfByUint<7, 0>(xt, 10);
    SetConfByUint<11, 8>(xt, 0b1111);
    SetConfByUint<12, 12>(xt, 1);

    std::vector<uint8_t> memInfo = CreateMemInfo();
    __sanitizer_report_vmrgsort4_f16(memInfo.data(), record.location.pc, 0,
        reinterpret_cast<void*>(record.dst),
        reinterpret_cast<void*>(src),
        xm,
        xt
    );

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::VMRGSORT4_OP_C310);
    ptr += sizeof(RecordType);
    auto x = reinterpret_cast<Vms4v2RecordA5*>(ptr);
    record.location = x->location;
    ASSERT_TRUE(*x == record);

    record.detailedDataType = DetailedDataType::FLOAT;
    __sanitizer_report_vmrgsort4_f32(memInfo.data(), record.location.pc, 0,
        reinterpret_cast<void*>(record.dst),
        reinterpret_cast<void*>(src),
        xm,
        xt
    );
    ptr += sizeof(Vbs32Record);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::VMRGSORT4_OP_C310);
    x = reinterpret_cast<Vms4v2RecordA5*>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(DbiVecInstructions, moveva_and_vtranspose_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    __sanitizer_report_moveva(memInfo.data(), 0, 0,
        static_cast<uint64_t>(VaAddr8::VA0), 0, 0, 0);

    __sanitizer_report_vtranspose(memInfo.data(), 0, 0,
        reinterpret_cast<__ubuf__ void *>(0x100), reinterpret_cast<__ubuf__ void *>(0x200));
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::UNARY_OP);
}
