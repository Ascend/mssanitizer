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


#include <random>
#include <utility>
#include <algorithm>
#include <gtest/gtest.h>

#include "../../ccec_defs.h"
#include "../data_process.h"
#include "plugin/ccec/move_instructions/load_store.cpp"

using namespace Sanitizer;

namespace SanitizerTest {

template<AddressSpace space>
LoadStoreRecord CreateRandomLoadStoreRecord()
{
    LoadStoreRecord record{};
    record.addr = RandInt(0, 0x1000000);
    record.size = RandInt(0, 0x1000000);
    record.location.fileNo = RandInt(0, 0x100000);
    record.location.lineNo = RandInt(0, 0x1000);
    record.space = space;
    record.alignSize = GetUintFromConf<7, 0>(record.size);
    return record;
}

bool CheckRecordEqual(uint8_t const *memInfo, RecordType recordType, LoadStoreRecord &record)
{
    /// 检查 RecordType 是否匹配
    RecordType actualType = *reinterpret_cast<RecordType const *>(memInfo);
    if (actualType != recordType) {
        return false;
    }

    /// 检查 Record 结构体是否一致
    LoadStoreRecord const &actualRecord = *reinterpret_cast<LoadStoreRecord const *>(memInfo + sizeof(RecordType));
    if (record.space == AddressSpace::UB) {
        record.addr = GetUintFromConf<18, 0>(record.addr);
    }
    return IsEqual(actualRecord, record);
}

TEST(MoveInstructions, dump_ub_load_expect_get_correct_record)
{
    auto record = CreateRandomLoadStoreRecord<AddressSpace::UB>();
    std::vector<uint8_t> memInfo = CreateMemInfo();

    __sanitizer_report_load(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                       record.space, record.addr, record.size);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual(ptr, RecordType::LOAD, record));
}

TEST(MoveInstructions, dump_gm_load_expect_get_correct_record)
{
    auto record = CreateRandomLoadStoreRecord<AddressSpace::GM>();
    std::vector<uint8_t> memInfo = CreateMemInfo();

    __sanitizer_report_load(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                       record.space, record.addr, record.size);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual(ptr, RecordType::LOAD, record));
}

TEST(MoveInstructions, dump_ub_store_expect_get_correct_record)
{
    auto record = CreateRandomLoadStoreRecord<AddressSpace::UB>();
    std::vector<uint8_t> memInfo = CreateMemInfo();

    __sanitizer_report_store(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                       record.space, record.addr, record.size);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual(ptr, RecordType::STORE, record));
}

TEST(MoveInstructions, dump_gm_store_expect_get_correct_record)
{
    auto record = CreateRandomLoadStoreRecord<AddressSpace::GM>();
    std::vector<uint8_t> memInfo = CreateMemInfo();

    __sanitizer_report_store(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                       record.space, record.addr, record.size);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual(ptr, RecordType::STORE, record));
}

TEST(MoveInstructions, dump_private_load_store_with_initcheck_expect_get_success)
{
    auto record = CreateRandomLoadStoreRecord<AddressSpace::GM>();
    std::vector<uint8_t> memInfo = CreateMemInfo();
    RecordGlobalHead globalHead{};
    globalHead.checkParms.initcheck = true,
    std::copy_n(reinterpret_cast<uint8_t const*>(&globalHead), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_store(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                       AddressSpace::PRIVATE, record.addr, record.size);
    __sanitizer_report_load(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                       AddressSpace::PRIVATE, record.addr, record.size);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 2);
    ASSERT_EQ(blockHead.recordCount, 2);
}

TEST(MoveInstructions, dump_private_load_store_with_no_initcheck_expect_get_none)
{
    auto record = CreateRandomLoadStoreRecord<AddressSpace::GM>();
    // 地址须落在合法栈窗内（UT 环境下 sysVaBase=0，0x101000 的 bits[23:20]!=0），
    // 避免随机地址落进保留区被谓词改标 UNKNOWN 绕过 initcheck 过滤
    record.addr = 0x101000;
    std::vector<uint8_t> memInfo = CreateMemInfo();
    __sanitizer_report_store(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                       AddressSpace::PRIVATE, record.addr, record.size);
    __sanitizer_report_load(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                       AddressSpace::PRIVATE, record.addr, record.size);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);
    ASSERT_EQ(blockHead.recordCount, 0);
}

TEST(MoveInstructions, dump_private_load_store_with_no_initcheck_expect_get_gm_records)
{
    auto record = CreateRandomLoadStoreRecord<AddressSpace::GM>();
    // 同上：固化非保留区地址，保证 PRIVATE 记录按原过滤逻辑被丢弃
    record.addr = 0x101000;
    std::vector<uint8_t> memInfo = CreateMemInfo();
    __sanitizer_report_store(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                       AddressSpace::PRIVATE, record.addr, record.size);
    __sanitizer_report_store(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                       AddressSpace::GM, record.addr, record.size);
    __sanitizer_report_load(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                       AddressSpace::PRIVATE, record.addr, record.size);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);
    ASSERT_EQ(blockHead.recordCount, 1);
}

TEST(MoveInstructions, dump_private_load_store_reserved_region_expect_unknown_record) {
    // 保留区谓词：静态路径 PRIVATE 标签 + 保留区落点（空指针 addr=0）→ 改标 UNKNOWN、地址保真，
    // 且不受 initcheck 过滤管辖（memcheck 模式可检出）
    std::vector<uint8_t> memInfo = CreateMemInfo();
    __sanitizer_report_load(memInfo.data(), 2, 47, 0x104, AddressSpace::PRIVATE, 0x0, 4);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType const *>(ptr) == RecordType::LOAD);
    auto actualRecord = reinterpret_cast<LoadStoreRecord const *>(ptr + sizeof(RecordType));
    ASSERT_EQ(actualRecord->space, AddressSpace::UNKNOWN);
    ASSERT_EQ(actualRecord->addr, 0x0);
}

TEST(MoveInstructions, dump_private_load_store_reserved_region_with_initcheck_expect_unknown_record) {
    // 保留区落点在 initcheck 开启时同样改标 UNKNOWN（记录本来就会记录，谓词仅改语义）
    std::vector<uint8_t> memInfo = CreateMemInfo();
    RecordGlobalHead globalHead{};
    globalHead.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const *>(&globalHead), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_load(memInfo.data(), 2, 47, 0x104, AddressSpace::PRIVATE, 0x1234, 4);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType const *>(ptr) == RecordType::LOAD);
    auto actualRecord = reinterpret_cast<LoadStoreRecord const *>(ptr + sizeof(RecordType));
    ASSERT_EQ(actualRecord->space, AddressSpace::UNKNOWN);
    ASSERT_EQ(actualRecord->addr, 0x1234);
}

}

// Explicit bases exercise the classifier independently of host SPR mocks.
TEST(MoveInstructions, classify_original_scalar_address_boundaries) {
    for (uint64_t base : {0ULL, 0x2000000ULL, 0x100000000ULL}) {
        EXPECT_EQ(ClassifyScalarAddress(base, base), AddressSpace::UNKNOWN);
        EXPECT_EQ(ClassifyScalarAddress(base + 0x7ffff, base), AddressSpace::UNKNOWN);
        EXPECT_EQ(ClassifyScalarAddress(base + 0x80000, base), AddressSpace::UB);
        EXPECT_EQ(ClassifyScalarAddress(base + 0xfffff, base), AddressSpace::UB);
        EXPECT_EQ(ClassifyScalarAddress(base + 0x100000, base), AddressSpace::PRIVATE);
        EXPECT_EQ(ClassifyScalarAddress(base + 0xffffff, base), AddressSpace::PRIVATE);
        EXPECT_EQ(ClassifyScalarAddress(base + 0x1000000, base), AddressSpace::GM);
        EXPECT_EQ(ClassifyScalarAddress(base + 0x2000000, base), AddressSpace::GM);
    }
}

TEST(MoveInstructions, static_private_runtime_ub_keeps_ub_offset_for_both_modes) {
    for (bool initcheck : {false, true}) {
        for (uint64_t addr : {0x80000ULL, 0xf0000ULL, 0xfffffULL}) {
            auto memInfo = CreateMemInfo();
            auto head = reinterpret_cast<RecordGlobalHead *>(memInfo.data());
            head->checkParms.memcheck = true;
            head->checkParms.initcheck = initcheck;
            __sanitizer_report_load(memInfo.data(), 2, 47, 0x104, AddressSpace::PRIVATE, addr, 4);
            auto ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
            ASSERT_EQ(*reinterpret_cast<RecordType *>(ptr), RecordType::LOAD);
            auto record = reinterpret_cast<LoadStoreRecord *>(ptr + sizeof(RecordType));
            EXPECT_EQ(record->space, AddressSpace::UB);
            EXPECT_EQ(record->addr, addr & 0x7ffff);
            EXPECT_EQ(record->size, 4);
            EXPECT_EQ(record->location.lineNo, 47);
        }
    }
}

TEST(MoveInstructions, static_private_memcheck_records_pool_coordinates) {
    for (uint64_t addr : {0x101000ULL, 0x500000ULL}) {
        auto memInfo = CreateMemInfo();
        reinterpret_cast<RecordGlobalHead *>(memInfo.data())->checkParms.memcheck = true;
        __sanitizer_report_store(memInfo.data(), 2, 47, 0x104, AddressSpace::PRIVATE, addr, 4);
        auto ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
        ASSERT_EQ(*reinterpret_cast<RecordType *>(ptr), RecordType::STORE);
        auto record = reinterpret_cast<LoadStoreRecord *>(ptr + sizeof(RecordType));
        EXPECT_EQ(record->space, AddressSpace::PRIVATE);
        EXPECT_EQ(record->addr, addr - 0x100000);
        EXPECT_EQ(record->size, 4);
    }
}
