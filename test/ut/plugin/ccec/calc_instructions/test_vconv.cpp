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
#include "plugin/ccec/calc_instructions/vconv.cpp"

using namespace Sanitizer;

TEST(VconvCalcInstructions, dump_bf16_to_other_type_records_and_parse_each_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint8_t recordNum = 12;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckUnaryOpBlockInfo> blockInfoVec(12);
    CheckUnaryOpBlockInfo  initialValue= {8, 4, 32, 32};
    std::fill_n(blockInfoVec.begin(), blockInfoVec.size(), initialValue);

    CallUnaryU16Func<float, __bf16>(memInfo.data(), records, idx++, __sanitizer_report_vconv_bf162f32);
    CallUnaryConfigFunc<float, __bf16>(memInfo.data(), records, idx++, __sanitizer_report_vconv_bf162f32);
    CallUnaryU16Func<int32_t, __bf16>(memInfo.data(), records, idx++, __sanitizer_report_vconv_bf162s32a);
    CallUnaryConfigFunc<int32_t, __bf16>(memInfo.data(), records, idx++, __sanitizer_report_vconv_bf162s32a);
    CallUnaryU16Func<int32_t, __bf16>(memInfo.data(), records, idx++, __sanitizer_report_vconv_bf162s32c);
    CallUnaryConfigFunc<int32_t, __bf16>(memInfo.data(), records, idx++, __sanitizer_report_vconv_bf162s32c);
    CallUnaryU16Func<int32_t, __bf16>(memInfo.data(), records, idx++, __sanitizer_report_vconv_bf162s32f);
    CallUnaryConfigFunc<int32_t, __bf16>(memInfo.data(), records, idx++, __sanitizer_report_vconv_bf162s32f);
    CallUnaryU16Func<int32_t, __bf16>(memInfo.data(), records, idx++, __sanitizer_report_vconv_bf162s32r);
    CallUnaryConfigFunc<int32_t, __bf16>(memInfo.data(), records, idx++, __sanitizer_report_vconv_bf162s32r);
    CallUnaryU16Func<int32_t, __bf16>(memInfo.data(), records, idx++, __sanitizer_report_vconv_bf162s32z);
    CallUnaryConfigFunc<int32_t, __bf16>(memInfo.data(), records, idx++, __sanitizer_report_vconv_bf162s32z);

    bool isEqual = MultiIsEqual<UnaryOpRecord, CheckUnaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VconvCalcInstructions, dump_deq_records_and_parse_each_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint8_t recordNum = 13;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckUnaryOpBlockInfo> blockInfoVec(13);
    CheckUnaryOpBlockInfo  initialValue= {4, 8, 32, 32};
    std::fill_n(blockInfoVec.begin(), blockInfoVec.size(), initialValue);

    CallUnaryU8Func<half, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_deq);
    CallUnaryConfigFunc<half, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_deq);
    CallUnaryU8Func<half, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_deq, bool{}, bool{});

    CallUnaryConfigFunc<int8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_deqs162b8, bool{});
    CallUnaryU8Func<int8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_deqs162b8,
        bool{}, bool{}, bool{});
    CallUnaryConfigFunc<uint8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_deqs162b8, bool{});
    CallUnaryU8Func<uint8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_deqs162b8,
        bool{}, bool{}, bool{});

    CallUnaryConfigFunc<int8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_deqs162b8h);
    CallUnaryU8Func<int8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_deqs162b8h);
    CallUnaryConfigFunc<uint8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_deqs162b8h);

    CallUnaryConfigFunc<int8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_deqs162b8l);
    CallUnaryU8Func<int8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_deqs162b8l);
    CallUnaryConfigFunc<uint8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_deqs162b8l);

    bool isEqual = MultiIsEqual<UnaryOpRecord, CheckUnaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VconvCalcInstructions, dump_half_to_float_records_and_parse_each_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint8_t recordNum = 3;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckUnaryOpBlockInfo> blockInfoVec(3);
    CheckUnaryOpBlockInfo  initialValue= {8, 4, 32, 32};
    std::fill_n(blockInfoVec.begin(), blockInfoVec.size(), initialValue);

    CallUnaryU16Func<float, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162f32);
    CallUnaryConfigFunc<float, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162f32);
    CallUnaryU8Func<float, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162f32, bool{}, bool{});

    bool isEqual = MultiIsEqual<UnaryOpRecord, CheckUnaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VconvCalcInstructions, dump_half_to_int_type_records_and_parse_each_expect_success_first)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint8_t recordNum = 11;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckUnaryOpBlockInfo> blockInfoVec(11);
    CheckUnaryOpBlockInfo  initialValue= {8, 8, 32, 32};
    std::fill_n(blockInfoVec.begin(), blockInfoVec.size(), initialValue);

    CallUnaryU16Func<int16_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s16a);
    CallUnaryConfigFunc<int16_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s16a);
    CallUnaryU16Func<int16_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s16c);
    CallUnaryConfigFunc<int16_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s16c);
    CallUnaryU16Func<int16_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s16f);
    CallUnaryConfigFunc<int16_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s16f);
    CallUnaryConfigFunc<int16_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s16r);
    CallUnaryU8Func<int16_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s16r, bool{}, bool{});
    CallUnaryU16Func<int16_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s16r);
    CallUnaryU16Func<int16_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s16z);
    CallUnaryConfigFunc<int16_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s16z);

    bool isEqual = MultiIsEqual<UnaryOpRecord, CheckUnaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VconvCalcInstructions, dump_half_to_int_type_records_and_parse_each_expect_success_second)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint8_t recordNum = 12;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckUnaryOpBlockInfo> blockInfoVec(12);
    CheckUnaryOpBlockInfo  initialValue= {8, 4, 32, 32};
    std::fill_n(blockInfoVec.begin(), blockInfoVec.size(), initialValue);

    CallUnaryConfigFunc<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32a);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32a);
#else
    CallUnaryU8Func<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32a);
#endif
    CallUnaryU8Func<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32a, bool{}, bool{});
    CallUnaryConfigFunc<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32c);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32c);
#else
    CallUnaryU8Func<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32c);
#endif
    CallUnaryU8Func<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32c, bool{}, bool{});
    CallUnaryConfigFunc<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32f);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32f);
#else
    CallUnaryU8Func<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32f);
#endif
    CallUnaryU8Func<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32f, bool{}, bool{});
    CallUnaryConfigFunc<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32r);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32r);
#else
    CallUnaryU8Func<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32r);
#endif
    CallUnaryU8Func<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32r, bool{}, bool{});

    bool isEqual = MultiIsEqual<UnaryOpRecord, CheckUnaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VconvCalcInstructions, dump_half_to_int_and_void_type_records_and_parse_each_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint8_t recordNum = 19;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckUnaryOpBlockInfo> blockInfoVec(19);
    CheckUnaryOpBlockInfo  initialValue= {8, 4, 32, 32};
    CheckUnaryOpBlockInfo  secondValue= {2, 8, 32, 32};
    CheckUnaryOpBlockInfo  thirdValue= {4, 8, 32, 32};
    std::fill_n(blockInfoVec.begin(), 3, initialValue);
    std::fill_n(blockInfoVec.begin() + 3, 13, secondValue);
    std::fill_n(blockInfoVec.begin() + 16, 3, thirdValue);

    CallUnaryConfigFunc<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32z);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32z);
#else
    CallUnaryU8Func<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32z);
#endif
    CallUnaryU8Func<int32_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s32z, bool{}, bool{});
    CallUnaryConfigFunc<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4);
    CallUnaryU8Func<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4, bool{}, bool{});
    CallUnaryU16Func<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4);
    CallUnaryU16Func<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4a);
    CallUnaryConfigFunc<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4a);
    CallUnaryU16Func<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4c);
    CallUnaryConfigFunc<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4c);
    CallUnaryU16Func<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4f);
    CallUnaryConfigFunc<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4f);
    CallUnaryU16Func<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4r);
    CallUnaryConfigFunc<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4r);
    CallUnaryU16Func<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4z);
    CallUnaryConfigFunc<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4z);

    CallUnaryConfigFunc<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8);
#else
    CallUnaryU8Func<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8);
#endif
    CallUnaryU8Func<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8, bool{}, bool{});

    bool isEqual = MultiIsEqual<UnaryOpRecord, CheckUnaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VconvCalcInstructions, dump_half_to_int8_type_records_and_parse_each_expect_success)
{
    std::vector <uint8_t> memInfo(UT_RECORD_BUF_SIZE, 0);
    uint8_t recordNum = 17;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckUnaryOpBlockInfo> blockInfoVec(17);
    CheckUnaryOpBlockInfo  initialValue= {4, 8, 32, 32};
    std::fill_n(blockInfoVec.begin(), blockInfoVec.size(), initialValue);

    CallUnaryConfigFunc<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8a);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8a);
#else
    CallUnaryU8Func<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8a);
#endif
    CallUnaryU8Func<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8a, bool{}, bool{});
    CallUnaryConfigFunc<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8c);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8c);
#else
    CallUnaryU8Func<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8c);
#endif
    CallUnaryU8Func<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8c, bool{}, bool{});
    CallUnaryConfigFunc<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8f);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8f);
#else
    CallUnaryU8Func<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8f);
#endif
    CallUnaryU8Func<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8f, bool{}, bool{});
    CallUnaryU16Func<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8r);
    CallUnaryConfigFunc<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8r);
    CallUnaryConfigFunc<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8z);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8z);
#else
    CallUnaryU8Func<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8z);
#endif
    CallUnaryU8Func<int8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s8z, bool{}, bool{});
    CallUnaryConfigFunc<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8);
#else
    CallUnaryU8Func<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8);
#endif
    CallUnaryU8Func<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8, bool{}, bool{});

    bool isEqual = MultiIsEqual<UnaryOpRecord, CheckUnaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VconvCalcInstructions, dump_half_to_uint8_type_records_and_parse_each_expect_success)
{
    std::vector <uint8_t> memInfo(UT_RECORD_BUF_SIZE, 0);
    uint8_t recordNum = 20;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckUnaryOpBlockInfo> blockInfoVec(20);
    CheckUnaryOpBlockInfo  initialValue= {4, 8, 32, 32};
    std::fill_n(blockInfoVec.begin(), blockInfoVec.size(), initialValue);

    CallUnaryConfigFunc<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8a);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8a);
#else
    CallUnaryU8Func<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8a);
#endif
    CallUnaryU8Func<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8a, bool{}, bool{});
    CallUnaryConfigFunc<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8c);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8c);
#else
    CallUnaryU8Func<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8c);
#endif
    CallUnaryU8Func<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8c, bool{}, bool{});
    CallUnaryConfigFunc<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8f);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8f);
#else
    CallUnaryU8Func<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8f);
#endif
    CallUnaryU8Func<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8f, bool{}, bool{});
    CallUnaryU16Func<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8r);
    CallUnaryConfigFunc<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8r);
    CallUnaryConfigFunc<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8z);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8z);
#else
    CallUnaryU8Func<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8z);
#endif
    CallUnaryU8Func<uint8_t, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162u8z, bool{}, bool{});
    CallUnaryU16Func<__bf16, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322bf16a);
    CallUnaryConfigFunc<__bf16, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322bf16a);
    CallUnaryU16Func<__bf16, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322bf16c);
    CallUnaryConfigFunc<__bf16, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322bf16c);
    CallUnaryU16Func<__bf16, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322bf16f);
    CallUnaryConfigFunc<__bf16, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322bf16f);

    bool isEqual = MultiIsEqual<UnaryOpRecord, CheckUnaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VconvCalcInstructions, dump_float_to_half_type_records_and_parse_each_expect_success)
{
    std::vector <uint8_t> memInfo(UT_RECORD_BUF_SIZE, 0);
    uint8_t recordNum = 26;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckUnaryOpBlockInfo> blockInfoVec(26);
    CheckUnaryOpBlockInfo  initialValue= {4, 8, 32, 32};
    CheckUnaryOpBlockInfo  secondValue= {8, 8, 32, 32};
    std::fill_n(blockInfoVec.begin(), 20, initialValue);
    std::fill_n(blockInfoVec.begin() + 20, 6, secondValue);

    CallUnaryU16Func<__bf16, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322bf16r);
    CallUnaryConfigFunc<__bf16, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322bf16r);
    CallUnaryU16Func<__bf16, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322bf16z);
    CallUnaryConfigFunc<__bf16, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322bf16z);
    CallUnaryConfigFunc<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16);
#else
    CallUnaryU8Func<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16);
#endif
    CallUnaryU8Func<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16, bool{}, bool{});
    CallUnaryU16Func<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16a);
    CallUnaryConfigFunc<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16a);
    CallUnaryU16Func<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16c);
    CallUnaryConfigFunc<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16c);
    CallUnaryU16Func<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16f);
    CallUnaryConfigFunc<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16f);
    CallUnaryConfigFunc<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16o);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16o);
#else
    CallUnaryU8Func<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16o);
#endif
    CallUnaryU8Func<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16o, bool{}, bool{});
    CallUnaryU16Func<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16r);
    CallUnaryConfigFunc<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16r);
    CallUnaryU16Func<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16z);
    CallUnaryConfigFunc<half, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f16z);
    CallUnaryU16Func<float, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f32a);
    CallUnaryConfigFunc<float, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f32a);
    CallUnaryU16Func<float, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f32c);
    CallUnaryConfigFunc<float, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f32c);
    CallUnaryU16Func<float, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f32f);
    CallUnaryConfigFunc<float, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f32f);

    bool isEqual = MultiIsEqual<UnaryOpRecord, CheckUnaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VconvCalcInstructions, dump_float_to_float_type_records_and_parse_each_expect_success)
{
    std::vector <uint8_t> memInfo(UT_RECORD_BUF_SIZE, 0);
    uint8_t recordNum = 19;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckUnaryOpBlockInfo> blockInfoVec(19);
    CheckUnaryOpBlockInfo  initialValue= {8, 8, 32, 32};
    CheckUnaryOpBlockInfo  secondValue= {4, 8, 32, 32};
    std::fill_n(blockInfoVec.begin(), 4, initialValue);
    std::fill_n(blockInfoVec.begin() + 4, 12, secondValue);
    std::fill_n(blockInfoVec.begin() + 16, 3, initialValue);

    CallUnaryU16Func<float, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f32r);
    CallUnaryConfigFunc<float, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f32r);
    CallUnaryU16Func<float, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f32z);
    CallUnaryConfigFunc<float, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322f32z);
    CallUnaryU16Func<int16_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s16a);
    CallUnaryConfigFunc<int16_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s16a);
    CallUnaryU16Func<int16_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s16c);
    CallUnaryConfigFunc<int16_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s16c);
    CallUnaryU16Func<int16_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s16f);
    CallUnaryConfigFunc<int16_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s16f);
    CallUnaryConfigFunc<int16_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s16r);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int16_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s16r);
#else
    CallUnaryU8Func<int16_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s16r);
#endif
    CallUnaryU8Func<int16_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s16r, bool{}, bool{});
    CallUnaryConfigFunc<int16_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s16z);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int16_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s16z);
#else
    CallUnaryU8Func<int16_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s16z);
#endif
    CallUnaryU8Func<int16_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s16z, bool{}, bool{});
    CallUnaryConfigFunc<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32a);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32a);
#else
    CallUnaryU8Func<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32a);
#endif
    CallUnaryU8Func<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32a, bool{}, bool{});

    bool isEqual = MultiIsEqual<UnaryOpRecord, CheckUnaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VconvCalcInstructions, dump_float_to_int32_type_records_and_parse_each_expect_success)
{
    std::vector <uint8_t> memInfo(UT_RECORD_BUF_SIZE, 0);
    uint8_t recordNum = 12;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckUnaryOpBlockInfo> blockInfoVec(12);
    CheckUnaryOpBlockInfo  initialValue= {8, 8, 32, 32};
    std::fill_n(blockInfoVec.begin(), blockInfoVec.size(), initialValue);

    CallUnaryConfigFunc<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32c);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32c);
#else
    CallUnaryU8Func<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32c);
#endif
    CallUnaryU8Func<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32c, bool{}, bool{});
    CallUnaryConfigFunc<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32f);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32f);
#else
    CallUnaryU8Func<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32f);
#endif
    CallUnaryU8Func<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32f, bool{}, bool{});
    CallUnaryConfigFunc<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32r);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32r);
#else
    CallUnaryU8Func<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32r);
#endif
    CallUnaryU8Func<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32r, bool{}, bool{});
    CallUnaryConfigFunc<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32z);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32z);
#else
    CallUnaryU8Func<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32z);
#endif
    CallUnaryU8Func<int32_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s32z, bool{}, bool{});

    bool isEqual = MultiIsEqual<UnaryOpRecord, CheckUnaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VconvCalcInstructions, dump_int16_to_half_type_records_and_parse_each_expect_success)
{
    std::vector <uint8_t> memInfo(UT_RECORD_BUF_SIZE, 0);
    uint8_t recordNum = 26;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckUnaryOpBlockInfo> blockInfoVec(26);
    CheckUnaryOpBlockInfo  initialValue= {8, 4, 32, 32};
    CheckUnaryOpBlockInfo  secondValue= {8, 8, 32, 32};
    std::fill_n(blockInfoVec.begin(), 10, initialValue);
    std::fill_n(blockInfoVec.begin() + 10, 13, secondValue);
    std::fill_n(blockInfoVec.begin() + 23, 3, initialValue);

    CallUnaryU16Func<int64_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s64a);
    CallUnaryConfigFunc<int64_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s64a);
    CallUnaryU16Func<int64_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s64c);
    CallUnaryConfigFunc<int64_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s64c);
    CallUnaryU16Func<int64_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s64f);
    CallUnaryConfigFunc<int64_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s64f);
    CallUnaryU16Func<int64_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s64r);
    CallUnaryConfigFunc<int64_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s64r);
    CallUnaryU16Func<int64_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s64z);
    CallUnaryConfigFunc<int64_t, float>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f322s64z);
    CallUnaryConfigFunc<half, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s162f16);
    CallUnaryU8Func<half, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s162f16, bool{}, bool{});
    CallUnaryU16Func<half, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s162f16);
    CallUnaryU16Func<half, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s162f16a);
    CallUnaryConfigFunc<half, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s162f16a);
    CallUnaryU16Func<half, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s162f16c);
    CallUnaryConfigFunc<half, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s162f16c);
    CallUnaryU16Func<half, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s162f16f);
    CallUnaryConfigFunc<half, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s162f16f);
    CallUnaryU16Func<half, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s162f16r);
    CallUnaryConfigFunc<half, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s162f16r);
    CallUnaryU16Func<half, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s162f16z);
    CallUnaryConfigFunc<half, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s162f16z);
    CallUnaryConfigFunc<float, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s162f32);
    CallUnaryU8Func<float, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s162f32, bool{}, bool{});
    CallUnaryU16Func<float, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s162f32);

    bool isEqual = MultiIsEqual<UnaryOpRecord, CheckUnaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VconvCalcInstructions, dump_int32_to_float_type_records_and_parse_each_expect_success_first)
{
    std::vector <uint8_t> memInfo(UT_RECORD_BUF_SIZE, 0);
    uint8_t recordNum = 15;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckUnaryOpBlockInfo> blockInfoVec(15);
    CheckUnaryOpBlockInfo  initialValue= {8, 8, 32, 32};
    CheckUnaryOpBlockInfo  secondValue= {4, 8, 32, 32};
    std::fill_n(blockInfoVec.begin(), 13, initialValue);
    std::fill_n(blockInfoVec.begin() + 13, 2, secondValue);

    CallUnaryConfigFunc<float, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322f32);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<float, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322f32);
#else
    CallUnaryU8Func<float, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322f32);
#endif
    CallUnaryU8Func<float, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322f32, bool{}, bool{});
    CallUnaryU16Func<float, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322f32a);
    CallUnaryConfigFunc<float, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322f32a);
    CallUnaryU16Func<float, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322f32c);
    CallUnaryConfigFunc<float, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322f32c);
    CallUnaryU16Func<float, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322f32f);
    CallUnaryConfigFunc<float, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322f32f);
    CallUnaryU16Func<float, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322f32r);
    CallUnaryConfigFunc<float, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322f32r);
    CallUnaryU16Func<float, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322f32z);
    CallUnaryConfigFunc<float, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322f32z);
    CallUnaryU16Func<int16_t, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322s16);

    bool isEqual = MultiIsEqual<UnaryOpRecord, CheckUnaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VconvCalcInstructions, dump_int32_to_float_type_records_and_parse_each_expect_success_second)
{
    std::vector <uint8_t> memInfo(UT_RECORD_BUF_SIZE, 0);
    uint8_t recordNum = 16;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckUnaryOpBlockInfo> blockInfoVec(16);
    CheckUnaryOpBlockInfo  initialValue= {8, 4, 32, 32};
    CheckUnaryOpBlockInfo  secondValue= {8, 2, 32, 32};
    CheckUnaryOpBlockInfo  thirdValue= {4, 8, 32, 32};
    std::fill_n(blockInfoVec.begin(), 2, initialValue);
    std::fill_n(blockInfoVec.begin() + 2, 2, secondValue);
    std::fill_n(blockInfoVec.begin() + 4, 12, thirdValue);

    CallUnaryU16Func<int64_t, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322s64);
    CallUnaryConfigFunc<int64_t, int32_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s322s64);
    CallUnaryU8Func<half, void>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s42f16);
    CallUnaryConfigFunc<half, void>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s42f16);
    CallUnaryU16Func<float, int64_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s642f32a);
    CallUnaryConfigFunc<float, int64_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s642f32a);
    CallUnaryU16Func<float, int64_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s642f32c);
    CallUnaryConfigFunc<float, int64_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s642f32c);
    CallUnaryU16Func<float, int64_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s642f32f);
    CallUnaryConfigFunc<float, int64_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s642f32f);
    CallUnaryU16Func<float, int64_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s642f32r);
    CallUnaryConfigFunc<float, int64_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s642f32r);
    CallUnaryU16Func<float, int64_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s642f32z);
    CallUnaryConfigFunc<float, int64_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s642f32z);
    CallUnaryU16Func<int32_t, int64_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s642s32);
    CallUnaryConfigFunc<int32_t, int64_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s642s32);

    bool isEqual = MultiIsEqual<UnaryOpRecord, CheckUnaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VconvCalcInstructions, dump_int8_to_half_type_records_and_parse_each_expect_success)
{
    std::vector <uint8_t> memInfo(UT_RECORD_BUF_SIZE, 0);
    uint8_t recordNum = 18;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckUnaryOpBlockInfo> blockInfoVec(18);
    CheckUnaryOpBlockInfo  initialValue= {8, 4, 32, 32};
    CheckUnaryOpBlockInfo  secondValue= {4, 8, 32, 32};
    std::fill_n(blockInfoVec.begin(), 6, initialValue);
    std::fill_n(blockInfoVec.begin() + 6, 12, secondValue);

    CallUnaryConfigFunc<half, int8_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s82f16);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<half, int8_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s82f16);
#else
    CallUnaryU8Func<half, int8_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s82f16);
#endif
    CallUnaryU8Func<half, int8_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s82f16, bool{}, bool{});
    CallUnaryConfigFunc<half, uint8_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_u82f16);
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    CallUnaryU16Func<half, uint8_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_u82f16);
#else
    CallUnaryU8Func<half, uint8_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_u82f16);
#endif
    CallUnaryU8Func<half, uint8_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_u82f16, bool{}, bool{});
    CallUnaryConfigFunc<int8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_vdeqs162b8, bool{});
    CallUnaryU8Func<int8_t, int16_t>(memInfo.data(), records, idx++,
        __sanitizer_report_vconv_vdeqs162b8, bool{}, bool{}, bool{});
    CallUnaryConfigFunc<uint8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_vdeqs162b8, bool{});
    CallUnaryU8Func<uint8_t, int16_t>(memInfo.data(), records, idx++,
        __sanitizer_report_vconv_vdeqs162b8, bool{}, bool{}, bool{});
    CallUnaryU8Func<int8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_vdeqs162b8h);
    CallUnaryConfigFunc<int8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_vdeqs162b8h);
    CallUnaryU8Func<uint8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_vdeqs162b8h);
    CallUnaryConfigFunc<uint8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_vdeqs162b8h);
    CallUnaryU8Func<int8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_vdeqs162b8l);
    CallUnaryConfigFunc<int8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_vdeqs162b8l);
    CallUnaryU8Func<uint8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_vdeqs162b8l);
    CallUnaryConfigFunc<uint8_t, int16_t>(memInfo.data(), records, idx++, __sanitizer_report_vconv_vdeqs162b8l);

    bool isEqual = MultiIsEqual<UnaryOpRecord, CheckUnaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VconvCalcInstructions, dump_records_with_dst_void_and_parse_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint8_t recordNum = 13;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    CallUnaryConfigFunc<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4);
    CallUnaryU8Func<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4, bool{}, bool{});
    CallUnaryU16Func<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4);
    CallUnaryU16Func<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4a);
    CallUnaryConfigFunc<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4a);
    CallUnaryU16Func<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4c);
    CallUnaryConfigFunc<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4c);
    CallUnaryU16Func<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4f);
    CallUnaryConfigFunc<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4f);
    CallUnaryU16Func<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4r);
    CallUnaryConfigFunc<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4r);
    CallUnaryU16Func<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4z);
    CallUnaryConfigFunc<void, half>(memInfo.data(), records, idx++, __sanitizer_report_vconv_f162s4z);
    auto ptr = reinterpret_cast<uint8_t *>(memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) + sizeof(RecordType));
    for (uint8_t i = 0; i < idx; ++i) {
        UnaryOpRecord const actualRecord = *reinterpret_cast<UnaryOpRecord const *>(ptr);
        ASSERT_EQ(actualRecord.srcBlockSize, 32);
        ASSERT_EQ(actualRecord.srcBlockNum, 8);
        ASSERT_EQ(actualRecord.dstBlockSize, 32);
        ASSERT_EQ(actualRecord.dstBlockNum, 2);
        ASSERT_EQ(actualRecord.srcDataBits, sizeof(half) * BITS_EACH_BYTE);
        ASSERT_EQ(actualRecord.dstDataBits, 4);
        ptr += sizeof(RecordType) + sizeof(UnaryOpRecord);
    }
}

TEST(VconvCalcInstructions, dump_records_with_src_void_and_parse_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint8_t recordNum = 2;
    auto records = CreateRandomUnaryOpRecords(recordNum);
    uint8_t idx = 0;
    CallUnaryU8Func<half, void>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s42f16);
    CallUnaryConfigFunc<half, void>(memInfo.data(), records, idx++, __sanitizer_report_vconv_s42f16);
    auto ptr = reinterpret_cast<uint8_t *>(memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) + sizeof(RecordType));
    for (uint8_t i = 0; i < idx; ++i) {
        UnaryOpRecord const actualRecord = *reinterpret_cast<UnaryOpRecord const *>(ptr);
        ASSERT_EQ(actualRecord.srcBlockSize, 32);
        ASSERT_EQ(actualRecord.srcBlockNum, 2);
        ASSERT_EQ(actualRecord.dstBlockSize, 32);
        ASSERT_EQ(actualRecord.dstBlockNum, 8);
        ASSERT_EQ(actualRecord.srcDataBits, 4);
        ASSERT_EQ(actualRecord.dstDataBits, sizeof(half) * BITS_EACH_BYTE);
        ptr += sizeof(RecordType) + sizeof(UnaryOpRecord);
    }
}
