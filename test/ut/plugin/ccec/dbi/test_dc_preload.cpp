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
#include "plugin/ccec/dbi/probes/dc_preload.cpp"

using namespace Sanitizer;

TEST(DbiDcPreload, dc_preload_gm_addr_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t addr = 0;
    SetConfByUint<48, 25>(addr, 1);
    int64_t offset = 64;

    __sanitizer_report_dc_preload(memInfo.data(), 0, 0, reinterpret_cast<__gm__ uint64_t *>(addr), offset);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::DC_PRELOAD);

    auto preloadRecord = reinterpret_cast<DcPreloadRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(preloadRecord->addr == addr);
    ASSERT_TRUE(preloadRecord->offset == offset);
    ASSERT_TRUE(preloadRecord->space == AddressSpace::GM);
}

TEST(DbiDcPreload, dc_preload_ub_addr_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t addr = 0;
    SetConfByUint<48, 25>(addr, 0);
    SetConfByUint<24, 20>(addr, 0x00);
    SetConfByUint<19, 19>(addr, 0x01);
    int64_t offset = 128;

    __sanitizer_report_dc_preload(memInfo.data(), 0, 0, reinterpret_cast<__gm__ uint64_t *>(addr), offset);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::DC_PRELOAD);

    auto preloadRecord = reinterpret_cast<DcPreloadRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(preloadRecord->addr == addr);
    ASSERT_TRUE(preloadRecord->offset == offset);
    ASSERT_TRUE(preloadRecord->space == AddressSpace::UB);
}

TEST(DbiDcPreload, dc_preload_private_addr_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();

    auto head = reinterpret_cast<RecordGlobalHead*>(memInfo.data());
    head->checkParms.initcheck = true;

    uint64_t addr = 0;
    SetConfByUint<60, 60>(addr, 0x1);
    SetConfByUint<48, 25>(addr, 0);
    SetConfByUint<24, 24>(addr, 0);
    SetConfByUint<23, 20>(addr, 0x1);
    SetConfByUint<18, 18>(addr, 0x1);
    int64_t offset = 32;

    __sanitizer_report_dc_preload(memInfo.data(), 0, 0, reinterpret_cast<__gm__ uint64_t *>(addr), offset);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::DC_PRELOAD);

    auto preloadRecord = reinterpret_cast<DcPreloadRecord*>(ptr + sizeof(RecordType));
    uint64_t expectedAddr = GetUintFromConf<18, 0>(addr) - 0x100000; // GetUintFromConf<18, 0>(addr) 是小于 0x100000的，但是分支确实进入了；
    ASSERT_TRUE(preloadRecord->addr == expectedAddr);
    ASSERT_TRUE(preloadRecord->offset == offset);
    ASSERT_TRUE(preloadRecord->space == AddressSpace::PRIVATE);
}

TEST(DbiDcPreload, dc_preloadi_gm_addr_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t addr = 0;
    SetConfByUint<48, 25>(addr, 1);
    int16_t offset = 256;

    __sanitizer_report_dc_preloadi(memInfo.data(), 0, 0, reinterpret_cast<__gm__ uint64_t *>(addr), offset);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::DC_PRELOAD);

    auto preloadRecord = reinterpret_cast<DcPreloadRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(preloadRecord->addr == addr);
    ASSERT_TRUE(preloadRecord->offset == static_cast<int64_t>(offset));
    ASSERT_TRUE(preloadRecord->space == AddressSpace::GM);
}

TEST(DbiDcPreload, dc_preloadi_ub_addr_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint64_t addr = 0;
    SetConfByUint<48, 25>(addr, 0);
    SetConfByUint<24, 20>(addr, 0x00);
    SetConfByUint<19, 19>(addr, 0x01);
    int16_t offset = 512;

    __sanitizer_report_dc_preloadi(memInfo.data(), 0, 0, reinterpret_cast<__gm__ uint64_t *>(addr), offset);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::DC_PRELOAD);

    auto preloadRecord = reinterpret_cast<DcPreloadRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(preloadRecord->addr == addr);
    ASSERT_TRUE(preloadRecord->offset == static_cast<int64_t>(offset));
    ASSERT_TRUE(preloadRecord->space == AddressSpace::UB);
}

TEST(DbiDcPreload, dc_preloadi_private_addr_expect_correct_record)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();

    auto head = reinterpret_cast<RecordGlobalHead*>(memInfo.data());
    head->checkParms.initcheck = true;

    uint64_t addr = 0;
    SetConfByUint<60, 60>(addr, 0x1);
    SetConfByUint<48, 25>(addr, 0);
    SetConfByUint<24, 24>(addr, 0);
    SetConfByUint<23, 20>(addr, 0x2);
    int16_t offset = -128;

    __sanitizer_report_dc_preloadi(memInfo.data(), 0, 0, reinterpret_cast<__gm__ uint64_t *>(addr), offset);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::DC_PRELOAD);

    auto preloadRecord = reinterpret_cast<DcPreloadRecord*>(ptr + sizeof(RecordType));
    uint64_t expectedAddr = GetUintFromConf<18, 0>(addr) - 0x100000; // GetUintFromConf<18, 0>(addr) 是小于 0x100000的，但是分支确实进入了；
    ASSERT_TRUE(preloadRecord->addr == expectedAddr);
    ASSERT_TRUE(preloadRecord->offset == static_cast<int64_t>(offset));
    ASSERT_TRUE(preloadRecord->space == AddressSpace::PRIVATE);
}

TEST(DbiDcPreload, dc_preload_gm_addr_in_offset_range_expect_subtracted_addr)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();

    auto head = reinterpret_cast<RecordGlobalHead*>(memInfo.data());
    constexpr uint64_t l2CacheOffset = 0x100000;
    head->kernelInfo.l2CacheOffset = l2CacheOffset;

    constexpr uint64_t TB_TO_B_MULTIPLIER = 1ULL << 40;
    constexpr uint64_t GM_OFFSET_RANGE_MIN = 24 * TB_TO_B_MULTIPLIER;

    uint64_t addr = GM_OFFSET_RANGE_MIN + 0x1000;
    int64_t offset = 64;

    __sanitizer_report_dc_preload(memInfo.data(), 0, 0, reinterpret_cast<__gm__ uint64_t *>(addr), offset);

    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::DC_PRELOAD);

    auto preloadRecord = reinterpret_cast<DcPreloadRecord*>(ptr + sizeof(RecordType));
    ASSERT_TRUE(preloadRecord->addr == addr - l2CacheOffset);
    ASSERT_TRUE(preloadRecord->offset == offset);
    ASSERT_TRUE(preloadRecord->space == AddressSpace::GM);
}
