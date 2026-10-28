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
#include "plugin/ccec/calc_instructions/vsel.cpp"

TEST(VselCalcInstructions, dump_records_and_parse_each_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint8_t recordNum = 18;
    auto records = CreateRandomBinaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckBinaryOpBlockInfo> blockInfoVec(18);
    CheckBinaryOpBlockInfo initialValue = {8, 8, 8, 32, 32, 32};
    std::fill_n(blockInfoVec.begin(), blockInfoVec.size(), initialValue);

    CallBinaryConfigFunc<half>(memInfo.data(), records, idx++, __sanitizer_report_vsel);
    CallBinaryConfigFunc<half, half, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel);
    CallBinaryFunc<half, half, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel);
    CallBinaryFunc<half, half, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel,
        uint8_t{}, bool{}, bool{});
    CallBinaryFunc<half, half, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel, VSEL_MODE0);
    CallBinaryFunc<half, half, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel, VSEL_MODE2);

    CallBinaryFunc<half>(memInfo.data(), records, idx++, __sanitizer_report_vsel);
    CallBinaryFunc<half>(memInfo.data(), records, idx++, __sanitizer_report_vsel, uint8_t{}, bool{}, bool{});
    CallBinaryFunc<half>(memInfo.data(), records, idx++, __sanitizer_report_vsel, uint8_t{});

    CallBinaryConfigFunc<float>(memInfo.data(), records, idx++, __sanitizer_report_vsel);
    CallBinaryConfigFunc<float, float, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel);
    CallBinaryFunc<float, float, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel);
    CallBinaryFunc<float, float, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel,
        uint8_t{}, bool{}, bool{});
    CallBinaryFunc<float, float, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel, uint8_t{});

    CallBinaryFunc<float>(memInfo.data(), records, idx++, __sanitizer_report_vsel);
    CallBinaryFunc<float>(memInfo.data(), records, idx++, __sanitizer_report_vsel, uint8_t{}, bool{}, bool{});
    CallBinaryFunc<float>(memInfo.data(), records, idx++, __sanitizer_report_vsel, uint8_t{});

    bool isEqual = MultiIsEqual<BinaryOpRecord, CheckBinaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VselCalcInstructions, dump_records_with_half_mode1_and_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint8_t recordNum = 1;
    auto records = CreateRandomBinaryOpRecords(recordNum);
    uint8_t idx = 0;
    CallBinaryFunc<half, half, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel, VSEL_MODE1);
    BinaryOpRecord const &actualRecord = *reinterpret_cast<BinaryOpRecord const *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType));
    ASSERT_EQ(actualRecord.src1BlockSize, 16);
    ASSERT_EQ(actualRecord.src1BlockNum, 1);
    ASSERT_EQ(actualRecord.src1BlockStride, 0);
    ASSERT_EQ(actualRecord.src1RepeatStride, 1);
    ASSERT_EQ(actualRecord.src0DataBits, sizeof(half) * BITS_EACH_BYTE);
    ASSERT_EQ(actualRecord.src1DataBits, sizeof(half) * BITS_EACH_BYTE);
    ASSERT_EQ(actualRecord.dstDataBits, sizeof(half) * BITS_EACH_BYTE);
}

TEST(VselCalcInstructions, dump_records_with_float_mode1_and_parse_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint8_t recordNum = 1;
    auto records = CreateRandomBinaryOpRecords(recordNum);
    uint8_t idx = 0;
    CallBinaryFunc<float, float, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel, VSEL_MODE1);
    BinaryOpRecord const actualRecord = *reinterpret_cast<BinaryOpRecord const *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType));
    ASSERT_EQ(actualRecord.src1BlockSize, 8);
    ASSERT_EQ(actualRecord.src1BlockNum, 1);
    ASSERT_EQ(actualRecord.src1BlockStride, 0);
    ASSERT_EQ(actualRecord.src1RepeatStride, 1);
    ASSERT_EQ(actualRecord.src0DataBits, sizeof(float) * BITS_EACH_BYTE);
    ASSERT_EQ(actualRecord.src1DataBits, sizeof(float) * BITS_EACH_BYTE);
    ASSERT_EQ(actualRecord.dstDataBits, sizeof(float) * BITS_EACH_BYTE);
}

TEST(VselCalcInstructions, config_overloads_with_all_select_modes_expect_success) {
    auto record = CreateRandomBinaryOpRecord();
    uint64_t ubufAddr = 0x1000;
    auto setup = [&](std::vector<uint8_t> &memInfo) {
        __gm__ RecordGlobalHead *globalHead = reinterpret_cast<__gm__ RecordGlobalHead *>(memInfo.data());
        globalHead->registers[GetRegisterIdx()].cmpMaskAddr = ubufAddr;
    };

    auto callConfig = [&](uint8_t selectMode) {
        std::vector<uint8_t> memInfo = CreateMemInfo();
        setup(memInfo);
        uint64_t config = (static_cast<uint64_t>(selectMode) & 0x3) << VSEL_MODESHIFT;
        __sanitizer_report_vsel(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
            reinterpret_cast<__ubuf__ half *>(record.dst), reinterpret_cast<__ubuf__ half *>(record.src0),
            reinterpret_cast<__ubuf__ half *>(record.src1), config);
    };

    callConfig(VSEL_MODE0);
    callConfig(VSEL_MODE1);
    callConfig(VSEL_MODE2);
}
