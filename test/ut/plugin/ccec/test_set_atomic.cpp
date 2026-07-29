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
#include "core/framework/record_defs.h"

extern void __sanitizer_report_set_atomic_none(uint8_t *memInfo, uint64_t fileNo, uint64_t lineNo, int64_t pc);
extern void __sanitizer_report_set_atomic_f32(uint8_t *memInfo, uint64_t fileNo, uint64_t lineNo, int64_t pc);
extern void __sanitizer_report_set_atomic_add(uint8_t *memInfo, uint64_t fileNo, uint64_t lineNo, int64_t pc);
extern void __sanitizer_report_set_atomic_max(uint8_t *memInfo, uint64_t fileNo, uint64_t lineNo, int64_t pc);
extern void __sanitizer_report_set_atomic_min(uint8_t *memInfo, uint64_t fileNo, uint64_t lineNo, int64_t pc);

using namespace Sanitizer;

namespace SanitizerTest {

constexpr uint64_t MEM_INFO_SIZE = 1024 * 1024;

static bool CheckAtomicRecordEqual(uint8_t const *memInfo, AtomicModeRecord const &expected) {
    RecordType actualType = *reinterpret_cast<RecordType const *>(memInfo);
    if (actualType != RecordType::SET_ATOMIC) {
        return false;
    }
    AtomicModeRecord const &actualRecord = *reinterpret_cast<AtomicModeRecord const *>(memInfo + sizeof(RecordType));
    return actualRecord.location == expected.location && actualRecord.mode == expected.mode;
}

TEST(SetAtomic, set_atomic_none_with_racecheck_expect_record) {
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const *>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    __sanitizer_report_set_atomic_none(memInfo.data(), 10, 20, 0x1000);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    AtomicModeRecord expected{};
    expected.location = {10, 20, 0x1000, 0};
    expected.mode = AtomicMode::NONE;
    ASSERT_TRUE(CheckAtomicRecordEqual(ptr, expected));
}

TEST(SetAtomic, set_atomic_f32_with_racecheck_expect_record) {
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const *>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    __sanitizer_report_set_atomic_f32(memInfo.data(), 10, 20, 0x1000);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    AtomicModeRecord expected{};
    expected.location = {10, 20, 0x1000, 0};
    expected.mode = AtomicMode::F32;
    ASSERT_TRUE(CheckAtomicRecordEqual(ptr, expected));
}

TEST(SetAtomic, set_atomic_add_with_racecheck_expect_record) {
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const *>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    __sanitizer_report_set_atomic_add(memInfo.data(), 10, 20, 0x1000);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    AtomicModeRecord expected{};
    expected.location = {10, 20, 0x1000, 0};
    expected.mode = AtomicMode::SUM;
    ASSERT_TRUE(CheckAtomicRecordEqual(ptr, expected));
}

TEST(SetAtomic, set_atomic_max_with_racecheck_expect_record) {
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const *>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    __sanitizer_report_set_atomic_max(memInfo.data(), 10, 20, 0x1000);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    AtomicModeRecord expected{};
    expected.location = {10, 20, 0x1000, 0};
    expected.mode = AtomicMode::MAX;
    ASSERT_TRUE(CheckAtomicRecordEqual(ptr, expected));
}

TEST(SetAtomic, set_atomic_min_with_racecheck_expect_record) {
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const *>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    __sanitizer_report_set_atomic_min(memInfo.data(), 10, 20, 0x1000);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    AtomicModeRecord expected{};
    expected.location = {10, 20, 0x1000, 0};
    expected.mode = AtomicMode::MIN;
    ASSERT_TRUE(CheckAtomicRecordEqual(ptr, expected));
}
}
