/* -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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
#include <algorithm>

#include "../ccec_defs.h"
#include "data_process.h"
#include "plugin/ccec/ctrl_instructions.cpp"

using namespace Sanitizer;

namespace SanitizerTest {

constexpr uint64_t MEM_INFO_SIZE = 1024 * 1024;

TEST(CtrlInstructions, set_mask_count_with_initcheck_expect_record) {
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.registerCheck = true;
    std::copy_n(reinterpret_cast<uint8_t const *>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    __sanitizer_report_set_mask_count(memInfo.data(), 10, 20, 0x1000);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    RecordType actualType = *reinterpret_cast<RecordType const *>(ptr);
    ASSERT_EQ(actualType, RecordType::SET_CTRL);
}

TEST(CtrlInstructions, set_mask_norm_with_initcheck_expect_record) {
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.registerCheck = true;
    std::copy_n(reinterpret_cast<uint8_t const *>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    __sanitizer_report_set_mask_norm(memInfo.data(), 10, 20, 0x1000);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    RecordType actualType = *reinterpret_cast<RecordType const *>(ptr);
    ASSERT_EQ(actualType, RecordType::SET_CTRL);
}

TEST(CtrlInstructions, set_ctrl_with_initcheck_expect_record) {
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.registerCheck = true;
    std::copy_n(reinterpret_cast<uint8_t const *>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    uint64_t configVal = 0xFF;
    __sanitizer_report_set_ctrl(memInfo.data(), 10, 20, 0x1000, configVal);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    RecordType actualType = *reinterpret_cast<RecordType const *>(ptr);
    ASSERT_EQ(actualType, RecordType::SET_CTRL);

    RegisterSetRecord const &actualRecord = *reinterpret_cast<RegisterSetRecord const *>(ptr + sizeof(RecordType));
    ASSERT_EQ(actualRecord.regPayLoad.regValType, RegisterValueType::VAL_UINT64);
    ASSERT_EQ(actualRecord.regPayLoad.regVal, configVal);
}

TEST(CtrlInstructions, set_mask_count_with_memcheck_expect_no_record) {
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.memcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const *>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    __sanitizer_report_set_mask_count(memInfo.data(), 10, 20, 0x1000);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);
}

TEST(CtrlInstructions, set_vector_mask_with_initcheck_expect_record) {
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.registerCheck = true;
    std::copy_n(reinterpret_cast<uint8_t const *>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    __sanitizer_report_set_vector_mask(memInfo.data(), 10, 20, 0x1000, 0, 0xFFFF);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    RecordType actualType = *reinterpret_cast<RecordType const *>(ptr);
    ASSERT_EQ(actualType, RecordType::SET_VECTOR_MASK_0);
}

}
