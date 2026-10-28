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

static std::vector<uint8_t> CreateRegisterCheckMemInfo()
{
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.registerCheck = true;
    std::copy_n(reinterpret_cast<uint8_t const *>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    return memInfo;
}

TEST(CtrlInstructions, set_other_register_instructions_with_initcheck_expect_record) {
    uint64_t cfg = 0x123456789ABCDEF0UL;
    auto recPtr = [](std::vector<uint8_t> &m) {
        return m.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    };
    struct Case { RecordType type; std::function<void(uint8_t *)> call; };
    std::vector<Case> cases = {
        {RecordType::SET_FFTS_BASE_ADDR, [&](uint8_t *m) { __sanitizer_report_set_ffts_base_addr(m, 1, 2, 3, cfg); }},
        {RecordType::SET_FPC, [&](uint8_t *m) { __sanitizer_report_set_fpc(m, 1, 2, 3, cfg); }},
        {RecordType::SET_QUANT_PRE, [&](uint8_t *m) { __sanitizer_report_set_quant_pre(m, 1, 2, 3, cfg); }},
        {RecordType::SET_QUANT_POST, [&](uint8_t *m) { __sanitizer_report_set_quant_post(m, 1, 2, 3, cfg); }},
    };
    for (auto const &c : cases) {
        auto m = CreateRegisterCheckMemInfo();
        c.call(m.data());
        uint8_t *ptr = recPtr(m);
        ASSERT_EQ(*reinterpret_cast<RecordType const *>(ptr), c.type);
        auto const &rec = *reinterpret_cast<RegisterSetRecord const *>(ptr + sizeof(RecordType));
        ASSERT_EQ(rec.regPayLoad.regVal, cfg);
        ASSERT_EQ(rec.regPayLoad.regValType, RegisterValueType::VAL_UINT64);
    }
    auto checkLrelu = [&](auto alpha, RegisterValueType vt) {
        auto m = CreateRegisterCheckMemInfo();
        __sanitizer_report_set_lrelu_alpha(m.data(), 1, 2, 3, alpha);
        uint8_t *ptr = recPtr(m);
        ASSERT_EQ(*reinterpret_cast<RecordType const *>(ptr), RecordType::SET_LRELU_ALPHA);
        auto const &rec = *reinterpret_cast<RegisterSetRecord const *>(ptr + sizeof(RecordType));
        ASSERT_EQ(rec.regPayLoad.regValType, vt);
    };
    half h{};
    checkLrelu(h, RegisterValueType::VAL_HALF);
    checkLrelu(1.0f, RegisterValueType::VAL_FLOAT);

    auto m = CreateRegisterCheckMemInfo();
    __sanitizer_report_set_nd_para(m.data(), 1, 2, 3, cfg);
    __sanitizer_report_MOVEVA(m.data(), 1, 2, 3, static_cast<ub_addr8_t>(VaAddr8::VA0), 0, 0, 0);
    auto blockHead = *reinterpret_cast<RecordBlockHead const *>(m.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);
}

}
