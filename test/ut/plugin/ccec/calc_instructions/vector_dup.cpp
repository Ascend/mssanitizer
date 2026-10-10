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


#include <cstdint>
#include <utility>
#include <gtest/gtest.h>

#include "../data_process.h"
#include "../../ccec_defs.h"
#include "plugin/ccec/calc_instructions/vector_dup.cpp"

using namespace Sanitizer;

uint64_t ExtractConfigFromVecDupRecord(const VecDupRecord &record)
{
    constexpr uint64_t repeatMask = 0xFFUL;
    constexpr uint64_t blockStrideMask = 0xFFFFUL;
    constexpr uint64_t dstRepeatStrideMask0 = 0xFFUL;
    constexpr uint64_t dstRepeatStrideMask1 = 0xF00UL;

    constexpr uint64_t repeatShift = 56;
    constexpr uint64_t dstRepeatStrideShift0 = 32;
    constexpr uint64_t dstRepeatStrideShift1 = 44;
    constexpr uint64_t dstBlockStrideShift = 0;

    return ((record.dstBlockStride & blockStrideMask) << dstBlockStrideShift) |
           ((record.dstRepeatStride & dstRepeatStrideMask0) << dstRepeatStrideShift0) |
           ((record.dstRepeatStride & dstRepeatStrideMask1) << dstRepeatStrideShift1) |
           ((record.repeat & repeatMask) << repeatShift);
}

VecDupRecord ConvertToLowerRecord(const VecDupRecord &rhs)
{
    VecDupRecord record{};
    record.dst = rhs.dst;
    record.dstBlockStride = rhs.dstBlockStride;
    record.dstRepeatStride = rhs.dstRepeatStride & 0xFFUL;
    record.repeat = rhs.repeat;
    record.location = rhs.location;
    return record;
}

template<typename T>
void TestVectorDupForType(T src, uint8_t bitsSize)
{
    auto record = CreateRandomVecDupRecord();
    auto record_shorten = ConvertToLowerRecord(record);

    std::vector<uint8_t> memInfo = CreateMemInfo();

    uint64_t config = ExtractConfigFromVecDupRecord(record);
    // 四种重载，如果不考虑硬件差距则他们的结果是一样的
    __sanitizer_report_vector_dup(memInfo.data(), static_cast<uint64_t>(record.location.fileNo),
                                  static_cast<uint64_t>(record.location.lineNo), record.location.pc,
                                  reinterpret_cast<T *>(record.dst), src, config);

    __sanitizer_report_vector_dup(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                  reinterpret_cast<T *>(record.dst), src, record.repeat, record.dstBlockStride,
                                  1U, static_cast<uint8_t>(record.dstRepeatStride & 0xFFUL),
                                  8U, false, false);

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    __sanitizer_report_vector_dup(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                  reinterpret_cast<T *>(record.dst), src, record.repeat, record.dstBlockStride,
                                  1U, record.dstRepeatStride, 8U);
#else
    __sanitizer_report_vector_dup(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                  reinterpret_cast<T *>(record.dst), src, record.repeat, record.dstBlockStride,
                                  1U, static_cast<uint8_t>(record.dstRepeatStride & 0xFFUL),
                                  8U);
#endif

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual(ptr, record, bitsSize));
    ptr += sizeof(RecordType) + sizeof(VecDupRecord);
    ASSERT_TRUE(CheckRecordEqual(ptr, record_shorten, bitsSize));
    ptr += sizeof(RecordType) + sizeof(VecDupRecord);

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    ASSERT_TRUE(CheckRecordEqual(ptr, record, bitsSize));
#else
    ASSERT_TRUE(CheckRecordEqual(ptr, record_shorten, bitsSize));
#endif
}

TEST(VecDupInstructions, dump_vector_dup_f16_expect_get_correct_records)
{
    half src{};
    TestVectorDupForType<half>(src, 16);
}

TEST(VecDupInstructions, dump_vector_dup_f32_expect_get_correct_records)
{
    TestVectorDupForType<float>(0.0F, 32);
}

TEST(VecDupInstructions, dump_vector_dup_i16_expect_get_correct_records)
{
    TestVectorDupForType<int16_t>(0, 16);
}

TEST(VecDupInstructions, dump_vector_dup_i32_expect_get_correct_records)
{
    TestVectorDupForType<int32_t>(0, 32);
}

TEST(VecDupInstructions, dump_vector_dup_u16_expect_get_correct_records)
{
    TestVectorDupForType<uint16_t>(0, 16);
}

TEST(VecDupInstructions, dump_vector_dup_u32_expect_get_correct_records)
{
    TestVectorDupForType<uint32_t>(0, 32);
}

TEST(VecDupInstructions, dump_vector_dup_bf16_expect_get_correct_records)
{
    TestVectorDupForType<__bf16>(Bf16{}, 16);
}

template <typename T>
void TestVectorDupHighAddrPreserved(uint8_t bitsSize)
{
    constexpr uint64_t highAddr = 0xABCD000012345678ULL;

    auto record = CreateRandomVecDupRecord();
    record.dst = highAddr;

    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t config = ExtractConfigFromVecDupRecord(record);

    __sanitizer_report_vector_dup(memInfo.data(), static_cast<uint64_t>(record.location.fileNo),
                                  static_cast<uint64_t>(record.location.lineNo), record.location.pc,
                                  reinterpret_cast<T *>(record.dst), T{}, config);

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    VecDupRecord const &actual = *reinterpret_cast<VecDupRecord const *>(ptr + sizeof(RecordType));

    ASSERT_EQ(actual.dst, highAddr);
    ASSERT_EQ(actual.dataBits, bitsSize);
}

TEST(VecDupInstructions, dump_vector_dup_high_addr_b16_expect_full_addr_preserved)
{
    TestVectorDupHighAddrPreserved<half>(16);
}

TEST(VecDupInstructions, dump_vector_dup_high_addr_b32_expect_full_addr_preserved)
{
    TestVectorDupHighAddrPreserved<float>(32);
}
