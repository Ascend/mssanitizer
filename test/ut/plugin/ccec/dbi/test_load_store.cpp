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

#include "../data_process.h"
#define BUILD_DYNAMIC_PROBE
#include "plugin/ccec/dbi/probes/load_store.cpp"

using namespace Sanitizer;

TEST(DbiLoadStore, st_b64_imm_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t addr = 0;
    SetConfByUint<48, 25>(addr, 1);
    int64_t offset = 8;
    uint64_t post = 0;

    __sanitizer_report_st_b64_imm(memInfo.data(), 0, 0, addr, offset, post);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::ST_IO);

    auto record = reinterpret_cast<LoadStoreRecord*>(ptr + sizeof(RecordType));

    ASSERT_TRUE(record->addr == addr + offset);
    ASSERT_TRUE(record->size == sizeof(uint64_t));
    ASSERT_TRUE(record->dataType == DetailedDataType::B64);
    ASSERT_TRUE(record->space == AddressSpace::GM);
}

TEST(DbiLoadStore, st_b64_reg_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t addr = 0;
    SetConfByUint<19, 19>(addr, 1);
    int64_t offset = 2;
    uint64_t post = 0;

    __sanitizer_report_st_b64_reg(memInfo.data(), 0, 0, addr, offset, post);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::ST);

    auto record = reinterpret_cast<LoadStoreRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(record->addr == ((addr + offset * sizeof(uint64_t)) & 0x7FFFF));
    ASSERT_TRUE(record->size == sizeof(uint64_t));
    ASSERT_TRUE(record->dataType == DetailedDataType::B64);
    ASSERT_TRUE(record->space == AddressSpace::UB);
}

TEST(DbiLoadStore, ld_b64_imm_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t addr = 0x9000;
    int64_t offset = 16;
    uint64_t post = 0;

    __sanitizer_report_ld_b64_imm(memInfo.data(), 0, 0, addr, offset, post);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::LD_IO);

    auto record = reinterpret_cast<LoadStoreRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(record->addr == addr + offset);
    ASSERT_TRUE(record->size == sizeof(uint64_t));
    ASSERT_TRUE(record->dataType == DetailedDataType::B64);
}

TEST(DbiLoadStore, ld_b64_reg_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t addr = 0xA000;
    int64_t offset = 1;
    uint64_t post = 0;

    __sanitizer_report_ld_b64_reg(memInfo.data(), 0, 0, addr, offset, post);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::LD);

    auto record = reinterpret_cast<LoadStoreRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(record->addr == addr + offset * sizeof(uint64_t));
    ASSERT_TRUE(record->size == sizeof(uint64_t));
    ASSERT_TRUE(record->dataType == DetailedDataType::B64);
}

TEST(DbiLoadStore, stp_b64_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t addr = 0xD000;
    int64_t offset = 16;

    __sanitizer_report_stp_b64(memInfo.data(), 0, 0, addr, offset);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::STP);

    auto record = reinterpret_cast<LoadStoreRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(record->addr == addr + offset);
    ASSERT_TRUE(record->size == sizeof(uint64_t) * 2);
    ASSERT_TRUE(record->dataType == DetailedDataType::B64);
}

TEST(DbiLoadStore, ldp_b64_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t addr = 0xF000;
    int64_t offset = 32;

    __sanitizer_report_ldp_b64(memInfo.data(), 0, 0, addr, offset);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::LDP);

    auto record = reinterpret_cast<LoadStoreRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(record->addr == addr + offset);
    ASSERT_TRUE(record->size == sizeof(uint64_t) * 2);
    ASSERT_TRUE(record->dataType == DetailedDataType::B64);
}

TEST(DbiLoadStore, sti_b64_imm_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t addr = 0x19000;
    int64_t offset = 8;
    uint64_t post = 0;

    __sanitizer_report_sti_b64_imm(memInfo.data(), 0, 0, addr, offset, post);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::STI_IO);

    auto record = reinterpret_cast<LoadStoreRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(record->addr == addr + offset);
    ASSERT_TRUE(record->size == sizeof(uint64_t));
    ASSERT_TRUE(record->dataType == DetailedDataType::B64);
}

TEST(DbiLoadStore, sti_b64_reg_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t addr = 0x1D000;
    int64_t offset = 2;
    uint64_t post = 0;

    __sanitizer_report_sti_b64_reg(memInfo.data(), 0, 0, addr, offset, post);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::STI);

    auto record = reinterpret_cast<LoadStoreRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(record->addr == addr + offset * sizeof(uint64_t));
    ASSERT_TRUE(record->size == sizeof(uint64_t));
    ASSERT_TRUE(record->dataType == DetailedDataType::B64);
}

TEST(DbiLoadStore, st_atomic_b32_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t addr = 0x11000;
    int64_t offset = 2;
    uint64_t post = 0;

    __sanitizer_report_st_atomic_b32(memInfo.data(), 0, 0, addr, offset, post);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::ST_ATOMIC);

    auto record = reinterpret_cast<LoadStoreRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(record->addr == addr + offset * sizeof(uint32_t));
    ASSERT_TRUE(record->size == sizeof(uint32_t));
    ASSERT_TRUE(record->dataType == DetailedDataType::B32);
}

TEST(DbiLoadStore, st_dev_b64_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t addr = 0x12000;
    int64_t offset = 8;

    __sanitizer_report_st_dev_b64(memInfo.data(), 0, 0, addr, offset);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::ST_DEV);

    auto record = reinterpret_cast<LoadStoreRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(record->addr == addr + offset);
    ASSERT_TRUE(record->size == sizeof(uint64_t));
    ASSERT_TRUE(record->dataType == DetailedDataType::B64);
    ASSERT_TRUE(record->space == AddressSpace::GM);
}

TEST(DbiLoadStore, ld_dev_b64_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t addr = 0x29000;
    int64_t offset = 8;

    __sanitizer_report_ld_dev_b64(memInfo.data(), 0, 0, addr, offset);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::LD_DEV);

    auto record = reinterpret_cast<LoadStoreRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(record->addr == addr + offset);
    ASSERT_TRUE(record->size == sizeof(uint64_t));
    ASSERT_TRUE(record->dataType == DetailedDataType::B64);
    ASSERT_TRUE(record->space == AddressSpace::GM);
}

TEST(DbiLoadStore, post_mode_expect_original_addr)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t addr = 0x2C000;
    int64_t offset = 8;
    uint64_t post = 1;

    __sanitizer_report_st_b64_imm(memInfo.data(), 0, 0, addr, offset, post);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::ST_IO);

    auto record = reinterpret_cast<LoadStoreRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(record->addr == addr);
    ASSERT_TRUE(record->size == sizeof(uint64_t));
    ASSERT_TRUE(record->dataType == DetailedDataType::B64);
}

TEST(DbiLoadStore, multiple_load_store_expect_correct_sequence)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();

    __sanitizer_report_st_b32_imm(memInfo.data(), 0, 0, 0x1000, 4, 0);
    __sanitizer_report_ld_b32_reg(memInfo.data(), 0, 0, 0x2000, 2, 0);
    __sanitizer_report_stp_b64(memInfo.data(), 0, 0, 0x3000, 16);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);

    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::ST_IO);
    auto record1 = reinterpret_cast<LoadStoreRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(record1->addr == 0x1004);
    ASSERT_TRUE(record1->dataType == DetailedDataType::B32);

    ptr += sizeof(RecordType) + sizeof(LoadStoreRecord);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::LD);
    auto record2 = reinterpret_cast<LoadStoreRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(record2->addr == 0x2008);
    ASSERT_TRUE(record2->dataType == DetailedDataType::B32);

    ptr += sizeof(RecordType) + sizeof(LoadStoreRecord);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::STP);
    auto record3 = reinterpret_cast<LoadStoreRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(record3->addr == 0x3010);
    ASSERT_TRUE(record3->dataType == DetailedDataType::B64);
}

TEST(DbiLoadStore, remaining_load_store_variants_expect_record)
{
    auto call = [&](auto &&func, RecordType expectType) {
        std::vector<uint8_t> memInfo = CreateMemInfo();
        func(memInfo.data());
        uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
        EXPECT_EQ(*reinterpret_cast<RecordType *>(ptr), expectType);
    };
    uint64_t addr = 0x1000;
    int64_t offset = 4;
    uint64_t post = 0;
    call([&](uint8_t *m) { __sanitizer_report_st_b32_reg(m, 0, 0, addr, offset, post); }, RecordType::ST);
    call([&](uint8_t *m) { __sanitizer_report_st_b16_imm(m, 0, 0, addr, offset, post); }, RecordType::ST_IO);
    call([&](uint8_t *m) { __sanitizer_report_st_b16_reg(m, 0, 0, addr, offset, post); }, RecordType::ST);
    call([&](uint8_t *m) { __sanitizer_report_st_b8_imm(m, 0, 0, addr, offset, post); }, RecordType::ST_IO);
    call([&](uint8_t *m) { __sanitizer_report_st_b8_reg(m, 0, 0, addr, offset, post); }, RecordType::ST);
    call([&](uint8_t *m) { __sanitizer_report_stp_b32(m, 0, 0, addr, offset); }, RecordType::STP);
    call([&](uint8_t *m) { __sanitizer_report_stp_b16(m, 0, 0, addr, offset); }, RecordType::STP);
    call([&](uint8_t *m) { __sanitizer_report_stp_b8(m, 0, 0, addr, offset); }, RecordType::STP);
    call([&](uint8_t *m) { __sanitizer_report_sti_b32_imm(m, 0, 0, addr, offset, post); }, RecordType::STI_IO);
    call([&](uint8_t *m) { __sanitizer_report_sti_b16_imm(m, 0, 0, addr, offset, post); }, RecordType::STI_IO);
    call([&](uint8_t *m) { __sanitizer_report_sti_b8_imm(m, 0, 0, addr, offset, post); }, RecordType::STI_IO);
    call([&](uint8_t *m) { __sanitizer_report_sti_b32_reg(m, 0, 0, addr, offset, post); }, RecordType::STI);
    call([&](uint8_t *m) { __sanitizer_report_sti_b16_reg(m, 0, 0, addr, offset, post); }, RecordType::STI);
    call([&](uint8_t *m) { __sanitizer_report_sti_b8_reg(m, 0, 0, addr, offset, post); }, RecordType::STI);
    call([&](uint8_t *m) { __sanitizer_report_ld_b32_imm(m, 0, 0, addr, offset, post); }, RecordType::LD_IO);
    call([&](uint8_t *m) { __sanitizer_report_ld_b16_imm(m, 0, 0, addr, offset, post); }, RecordType::LD_IO);
    call([&](uint8_t *m) { __sanitizer_report_ld_b8_imm(m, 0, 0, addr, offset, post); }, RecordType::LD_IO);
    call([&](uint8_t *m) { __sanitizer_report_ld_b16_reg(m, 0, 0, addr, offset, post); }, RecordType::LD);
    call([&](uint8_t *m) { __sanitizer_report_ld_b8_reg(m, 0, 0, addr, offset, post); }, RecordType::LD);
    call([&](uint8_t *m) { __sanitizer_report_ldp_b32(m, 0, 0, addr, offset); }, RecordType::LDP);
    call([&](uint8_t *m) { __sanitizer_report_ldp_b16(m, 0, 0, addr, offset); }, RecordType::LDP);
    call([&](uint8_t *m) { __sanitizer_report_ldp_b8(m, 0, 0, addr, offset); }, RecordType::LDP);
    call([&](uint8_t *m) { __sanitizer_report_st_atomic_b16(m, 0, 0, addr, offset, post); }, RecordType::ST_ATOMIC);
    call([&](uint8_t *m) { __sanitizer_report_st_atomic_b8(m, 0, 0, addr, offset, post); }, RecordType::ST_ATOMIC);
    call([&](uint8_t *m) { __sanitizer_report_sti_atomic_b32(m, 0, 0, addr, offset, post); }, RecordType::STI_ATOMIC);
    call([&](uint8_t *m) { __sanitizer_report_sti_atomic_b16(m, 0, 0, addr, offset, post); }, RecordType::STI_ATOMIC);
    call([&](uint8_t *m) { __sanitizer_report_sti_atomic_b8(m, 0, 0, addr, offset, post); }, RecordType::STI_ATOMIC);
    call([&](uint8_t *m) { __sanitizer_report_st_dev_b32(m, 0, 0, addr, offset); }, RecordType::ST_DEV);
    call([&](uint8_t *m) { __sanitizer_report_st_dev_b16(m, 0, 0, addr, offset); }, RecordType::ST_DEV);
    call([&](uint8_t *m) { __sanitizer_report_st_dev_b8(m, 0, 0, addr, offset); }, RecordType::ST_DEV);
    call([&](uint8_t *m) { __sanitizer_report_ld_dev_b32(m, 0, 0, addr, offset); }, RecordType::LD_DEV);
    call([&](uint8_t *m) { __sanitizer_report_ld_dev_b16(m, 0, 0, addr, offset); }, RecordType::LD_DEV);
    call([&](uint8_t *m) { __sanitizer_report_ld_dev_b8(m, 0, 0, addr, offset); }, RecordType::LD_DEV);
    call([&](uint8_t *m) { __sanitizer_report_scalar_atom_u32(m, 0, 0, addr); }, RecordType::SCALAR_ATOM);
    call([&](uint8_t *m) { __sanitizer_report_scalar_atom_s32(m, 0, 0, addr); }, RecordType::SCALAR_ATOM);
    call([&](uint8_t *m) { __sanitizer_report_scalar_atom_u64(m, 0, 0, addr); }, RecordType::SCALAR_ATOM);
    call([&](uint8_t *m) { __sanitizer_report_scalar_atom_s64(m, 0, 0, addr); }, RecordType::SCALAR_ATOM);
    call([&](uint8_t *m) { __sanitizer_report_scalar_red_u32(m, 0, 0, addr); }, RecordType::SCALAR_RED);
    call([&](uint8_t *m) { __sanitizer_report_scalar_red_s32(m, 0, 0, addr); }, RecordType::SCALAR_RED);
    call([&](uint8_t *m) { __sanitizer_report_scalar_red_f16(m, 0, 0, addr); }, RecordType::SCALAR_RED);
    call([&](uint8_t *m) { __sanitizer_report_scalar_red_bf16(m, 0, 0, addr); }, RecordType::SCALAR_RED);
    call([&](uint8_t *m) { __sanitizer_report_scalar_red_f32(m, 0, 0, addr); }, RecordType::SCALAR_RED);
    {
        std::vector<uint8_t> memInfo = CreateMemInfo();
        uint8_t buf[8] = {0};
        __sanitizer_report___mstx_dfx_report_stub_calli(memInfo.data(), 0, 0, 0, sizeof(buf), buf);
        __sanitizer_report___mstx_dfx_report_stub_call(memInfo.data(), 0, 0, 0, sizeof(buf), buf);
        SUCCEED();
    }
}
