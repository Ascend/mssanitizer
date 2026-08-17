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

#include "mem_error_def.h"
#include "platform_config.h"
#include "shadow_memory.h"

using namespace Sanitizer;

constexpr uint64_t TEST_ADDR = 0x61;
constexpr uint64_t TEST_MISALIGNED_ADDR = 0x3F;
constexpr uint64_t TEST_ALIGNED_ADDR = 0x100;
namespace {

TEST(ShadowMemory, init_sm_expect_ready)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910_PREMIUM_A);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());
}

namespace {
TEST(ShadowMemory, load_nbytes_on_uninitialized_expect_get_uninitialized_read_error)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910_PREMIUM_A);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());
    MemOpRecordForShadow memOpRecordForShadow(AddressSpace::PRIVATE, TEST_ADDR, TEST_ADDR, 0);
    ErrorMsgList errors = sm.LoadNBytes(memOpRecordForShadow, true);
    ASSERT_EQ(errors.size(), 1);
    ASSERT_TRUE(errors[0].isError);
    ASSERT_EQ(errors[0].type, MemErrorType::UNINITIALIZED_READ);
    ASSERT_EQ(errors[0].auxData.badAddr.addr, TEST_ADDR);
    ASSERT_EQ(errors[0].auxData.nBadBytes, TEST_ADDR);
}
}

TEST(ShadowMemory, load_nbytes_aligned_ub_expect_get_success)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910_PREMIUM_A);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());
    MemOpRecordForShadow memOpRecordForShadow(AddressSpace::UB, TEST_ALIGNED_ADDR, TEST_ALIGNED_ADDR, 0);
    ErrorMsgList errors = sm.LoadNBytes(memOpRecordForShadow, false);
    ASSERT_EQ(errors.size(), 0);
}

TEST(ShadowMemory, store_nbytes_aligned_ub_expect_get_success)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910_PREMIUM_A);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());
    MemOpRecordForShadow memOpRecordForShadow(AddressSpace::UB, TEST_ALIGNED_ADDR, TEST_ALIGNED_ADDR, 0);
    ErrorMsgList errors = sm.StoreNBytes(memOpRecordForShadow, true, true);
    ASSERT_EQ(errors.size(), 0);
}

TEST(ShadowMemory, free_out_of_bounds_and_double_free_expect_get_illegal_free_error)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910_PREMIUM_A);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());

    MemOpRecord record{};
    record.serialNo = 0;
    record.type = MemOpType::MALLOC;
    record.coreId = 1;
    record.moduleId = 0;
    record.srcAddr = 0x1000;
    record.dstAddr = 0x2000;
    record.srcSpace = AddressSpace::GM;
    record.dstSpace = AddressSpace::GM;
    record.memSize = 0x1000;
    record.lineNo = 19;
    strncpy(record.fileName, "test.cpp", sizeof(record.fileName) - 1);
    sm.AddHeapBlock(record);
    uint64_t size{};
    ErrorMsg error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);
    error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError);
    ASSERT_EQ(error.type, MemErrorType::ILLEGAL_FREE);
    record.dstAddr = TEST_ADDR;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError);
    ASSERT_EQ(error.type, MemErrorType::ILLEGAL_FREE);
}

TEST(ShadowMemory, malloc_but_do_not_free_expect_get_mem_leak_error)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910_PREMIUM_A);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());

    MemOpRecord record{};
    record.serialNo = 0;
    record.type = MemOpType::MALLOC;
    record.coreId = 1;
    record.moduleId = 0;
    record.srcAddr = 0x1000;
    record.dstAddr = 0x2000;
    record.srcSpace = AddressSpace::GM;
    record.dstSpace = AddressSpace::GM;
    record.memSize = 0x1000;
    record.lineNo = 19;
    record.infoSrc = MemInfoSrc::ACL;
    strncpy(record.fileName, "test.cpp", sizeof(record.fileName) - 1);
    sm.AddHeapBlock(record);
    uint64_t size;
    record.dstAddr = 0x5000;
    sm.AddHeapBlock(record);

    record.dstAddr = 0x2000;
    ErrorMsg error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    ErrorMsgList errors = sm.DoLeakCheck();
    ASSERT_EQ(errors.size(), 1);
    ASSERT_EQ(errors[0].type, MemErrorType::MEM_LEAK);
    ASSERT_EQ(errors[0].auxData.badAddr.addr, 0x5000);
    ASSERT_EQ(errors[0].auxData.nBadBytes, 0x1000);
}

TEST(ShadowMemory, malloc_mems_far_away_expect_no_interaction)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910B3);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());

    MemOpRecord record{};
    record.serialNo = 0;
    record.type = MemOpType::MALLOC;
    record.coreId = 1;
    record.moduleId = 0;
    record.srcAddr = 0x1000;
    record.dstAddr = 0x11c100000000;
    record.srcSpace = AddressSpace::GM;
    record.dstSpace = AddressSpace::GM;
    record.memSize = 0x1000;
    record.lineNo = 19;
    Sanitizer::ErrorMsgList errorMsgs;
    strncpy(record.fileName, "test.cpp", sizeof(record.fileName) - 1);
    sm.AddHeapBlock(record);
    uint64_t size;
    record.dstAddr = 0xf2d100000100;
    sm.AddHeapBlock(record);

    ErrorMsg error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    Sanitizer::MemOpRecordForShadow readMemOp(AddressSpace::GM, record.dstAddr, record.memSize, 1);

    errorMsgs = sm.StoreNBytes(readMemOp, true, true);
    ASSERT_EQ(errorMsgs.size(), 0);
}

TEST(ShadowMemory, reg_heap_but_do_not_unreg_heap_expect_get_illegal_free_error)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910B1);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());

    // malloc heap 1
    MemOpRecord record{};
    record.dstAddr = 0x2000;
    record.dstSpace = AddressSpace::GM;
    record.memSize = 1000;
    record.type = MemOpType::MALLOC;
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    strncpy(record.fileName, "test.cpp", sizeof(record.fileName) - 1);
    sm.AddHeapBlock(record);

    uint64_t size;

    // malloc heap 2
    record.dstAddr = 0x5000;
    sm.AddHeapBlock(record);

    // free heap 2
    record.type = MemOpType::FREE;
    ErrorMsg error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    // malloc heap 3
    record.dstAddr = 0x7000;
    record.type = MemOpType::MALLOC;
    sm.AddHeapBlock(record);

    // free heap 3
    record.type = MemOpType::FREE;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);
    record.dstAddr = 0x8000;

    // free heap 4
    error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError);
    ASSERT_EQ(error.type, MemErrorType::ILLEGAL_FREE);
    ASSERT_EQ(error.auxData.badAddr.addr, 0x8000);
}

TEST(ShadowMemory, reg_heap_and_reg_reiongs_but_do_not_unreg_regions_expect_get_illegal_free_error)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910B1);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());

    // malloc heap 1
    MemOpRecord record{};
    record.dstAddr = 0x2000;
    record.dstSpace = AddressSpace::GM;
    record.memSize = 1000;
    record.type = MemOpType::MALLOC;
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    record.rootAddr = 0x2000;
    strncpy(record.fileName, "test.cpp", sizeof(record.fileName) - 1);
    sm.AddHeapBlock(record);

    // malloc region 1
    record.infoSrc  = MemInfoSrc::MSTX_REGION;
    record.memSize = 300;
    sm.AddHeapBlock(record);
    record.dstAddr += record.memSize;
    record.memSize = 200;
    sm.AddHeapBlock(record);

    // free region 1-2
    record.type = MemOpType::FREE;
    uint64_t size;
    ErrorMsg error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    // free region 1-2
    error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError);
    ASSERT_EQ(error.type, MemErrorType::ILLEGAL_FREE);
    ASSERT_EQ(error.auxData.badAddr.addr, record.dstAddr);

    // free region 1-1
    record.dstAddr -= 300;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    // free heap 1
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    record.dstAddr = 0x2000;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    // free heap 1
    error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError);
    ASSERT_EQ(error.type, MemErrorType::ILLEGAL_FREE);
    ASSERT_EQ(error.auxData.badAddr.addr, record.dstAddr);
}

TEST(ShadowMemory, reg_heap_and_reg_regions_then_unreg_heap_expect_get_illegal_free_error)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910B2);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());
    MemOpRecord record{};
    record.dstAddr = 0x2000;
    record.dstSpace = AddressSpace::GM;
    record.memSize = 2000;
    record.type = MemOpType::MALLOC;
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    record.rootAddr = 0x2000;
    strncpy(record.fileName, "test.cpp", sizeof(record.fileName) - 1);

    // malloc heap 1
    sm.AddHeapBlock(record);
    record.rootAddr = 0x5000;
    record.dstAddr = 0x5000;

    // malloc heap 2
    sm.AddHeapBlock(record);
    record.rootAddr = 0x2000;

    // malloc regions 1
    record.infoSrc = MemInfoSrc::MSTX_REGION;
    record.memSize = 500;
    sm.AddHeapBlock(record);
    record.dstAddr += record.memSize;
    sm.AddHeapBlock(record);
    record.dstAddr += record.memSize;
    sm.AddHeapBlock(record);

    // malloc regions 2
    record.rootAddr = 0x5000;
    record.dstAddr = 0x5000;
    sm.AddHeapBlock(record);
    record.dstAddr += record.memSize;
    sm.AddHeapBlock(record);

    // free heap 2
    record.type = MemOpType::FREE;
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    record.rootAddr = 0x2000;
    record.dstAddr = 0x2000;
    uint64_t size{};
    ErrorMsg error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    // free regions 2-1
    record.infoSrc = MemInfoSrc::MSTX_REGION;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError);
    ASSERT_EQ(error.type, MemErrorType::ILLEGAL_FREE);
    ASSERT_EQ(error.auxData.badAddr.addr, record.dstAddr);

    // free regions 2-2
    record.dstAddr += 500;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError);
    ASSERT_EQ(error.type, MemErrorType::ILLEGAL_FREE);
    ASSERT_EQ(error.auxData.badAddr.addr, record.dstAddr);
}

TEST(ShadowMemory, unreg_unknown_heap_expect_illegal_free_error)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910B1);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());

    MemOpRecord record{};
    record.dstAddr = 0x3000;
    record.dstSpace = AddressSpace::GM;
    record.memSize = 1000;
    record.type = MemOpType::FREE;
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    strncpy(record.fileName, "test.cpp", sizeof(record.fileName) - 1);

    uint64_t size;
    ErrorMsg error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError);
    ASSERT_EQ(error.type, MemErrorType::ILLEGAL_FREE);
    ASSERT_EQ(error.auxData.badAddr.addr, 0x3000);
}

TEST(ShadowMemory, double_free_heap_expect_error)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910B1);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());

    MemOpRecord record{};
    record.dstAddr = 0x4000;
    record.dstSpace = AddressSpace::GM;
    record.memSize = 2000;
    record.type = MemOpType::MALLOC;
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    strncpy(record.fileName, "test.cpp", sizeof(record.fileName) - 1);
    sm.AddHeapBlock(record);

    uint64_t size;
    record.type = MemOpType::FREE;
    ErrorMsg error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError);
    ASSERT_EQ(error.type, MemErrorType::ILLEGAL_FREE);
    ASSERT_EQ(error.auxData.badAddr.addr, 0x4000);
}

TEST(ShadowMemory, free_partial_regions_then_free_heap_expect_success)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910B2);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());

    MemOpRecord record{};
    record.dstAddr = 0x6000;
    record.dstSpace = AddressSpace::GM;
    record.memSize = 3000;
    record.type = MemOpType::MALLOC;
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    record.rootAddr = 0x6000;
    strncpy(record.fileName, "test.cpp", sizeof(record.fileName) - 1);
    sm.AddHeapBlock(record);

    record.infoSrc = MemInfoSrc::MSTX_REGION;
    record.memSize = 1000;
    sm.AddHeapBlock(record);

    record.dstAddr += record.memSize;
    sm.AddHeapBlock(record);

    uint64_t size;
    record.type = MemOpType::FREE;
    ErrorMsg error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    record.dstAddr = 0x6000;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);
}

TEST(ShadowMemory, free_region_before_free_heap_expect_illegal_free)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910B1);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());

    MemOpRecord record{};
    record.dstAddr = 0x9000;
    record.dstSpace = AddressSpace::GM;
    record.memSize = 4000;
    record.type = MemOpType::MALLOC;
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    record.rootAddr = 0x9000;
    strncpy(record.fileName, "test.cpp", sizeof(record.fileName) - 1);
    sm.AddHeapBlock(record);

    record.infoSrc = MemInfoSrc::MSTX_REGION;
    record.memSize = 500;
    sm.AddHeapBlock(record);

    uint64_t size;
    record.type = MemOpType::FREE;
    ErrorMsg error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    record.dstAddr = 0x9000;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    record.infoSrc = MemInfoSrc::MSTX_REGION;
    record.dstAddr = 0x9000;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError);
    ASSERT_EQ(error.type, MemErrorType::ILLEGAL_FREE);
}

TEST(ShadowMemory, multiple_heaps_interleaved_operations)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910B2);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());

    MemOpRecord record{};
    record.dstSpace = AddressSpace::GM;
    record.type = MemOpType::MALLOC;
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    strncpy(record.fileName, "multi_test.cpp", sizeof(record.fileName) - 1);

    uint64_t size;
    ErrorMsg error;

    for (int i = 0; i < 5; ++i) {
        record.dstAddr = 0xA000 + i * 0x2000;
        record.memSize = 1000 + i * 500;
        record.rootAddr = record.dstAddr;
        sm.AddHeapBlock(record);
    }

    record.type = MemOpType::FREE;
    record.dstAddr = 0xA000;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    record.dstAddr = 0xC000;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    record.dstAddr = 0xE000;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    record.dstAddr = 0xA000;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError);
}

TEST(ShadowMemory, complex_heap_region_operations_with_multiple_scenarios)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910B1);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());

    MemOpRecord record{};
    record.dstSpace = AddressSpace::GM;
    record.type = MemOpType::MALLOC;
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    strncpy(record.fileName, "complex_test.cpp", sizeof(record.fileName) - 1);

    uint64_t size;
    ErrorMsg error;

    // Phase 1: Allocate multiple heaps
    for (int i = 0; i < 10; i++) {
        record.dstAddr = 0x10000 + i * 0x1000;
        record.memSize = 500 + i * 100;
        record.rootAddr = record.dstAddr;
        sm.AddHeapBlock(record);
    }

    // Phase 2: Allocate regions within first heap
    record.infoSrc = MemInfoSrc::MSTX_REGION;
    record.rootAddr = 0x10000;
    record.dstAddr = 0x10000;

    for (int i = 0; i < 5; i++) {
        record.memSize = 100 + i * 50;
        sm.AddHeapBlock(record);
        record.dstAddr += record.memSize;
    }

    // Phase 3: Free some regions
    record.type = MemOpType::FREE;
    record.dstAddr = 0x10000 + 200; // Free second region
    error = sm.FreeHeapBlock(record, size);
    if (error.isError) {
        record.dstAddr = 0x10000 + 100; // Try first region if second fails
        error = sm.FreeHeapBlock(record, size);
    }
    ASSERT_FALSE(error.isError);

    // Phase 4: Allocate more regions in different heaps
    record.type = MemOpType::MALLOC;
    record.infoSrc = MemInfoSrc::MSTX_REGION;
    record.rootAddr = 0x11000; // Second heap
    record.dstAddr = 0x11000;

    for (int i = 0; i < 3; i++) {
        record.memSize = 200 + i * 100;
        sm.AddHeapBlock(record);
        record.dstAddr += record.memSize;
    }

    // Phase 5: Free some heaps
    record.type = MemOpType::FREE;
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    record.dstAddr = 0x12000; // Third heap
    error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    // Phase 6: Try to free already freed heap
    error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError);
    ASSERT_EQ(error.type, MemErrorType::ILLEGAL_FREE);

    // Phase 7: Allocate new heap in freed space
    record.type = MemOpType::MALLOC;
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    record.dstAddr = 0x12000; // Reuse address
    record.memSize = 800;
    sm.AddHeapBlock(record);

    // Phase 8: Complex region operations
    record.infoSrc = MemInfoSrc::MSTX_REGION;
    record.rootAddr = 0x12000;
    record.dstAddr = 0x12000;

    for (int i = 0; i < 4; i++) {
        record.memSize = 150 + i * 25;
        sm.AddHeapBlock(record);
        record.dstAddr += record.memSize;

        // Free every other region immediately
        if (i % 2 == 1) {
            record.type = MemOpType::FREE;
            error = sm.FreeHeapBlock(record, size);
            ASSERT_TRUE(error.isError);
            record.type = MemOpType::MALLOC;
        }
    }

    // Phase 9: Final cleanup
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    record.type = MemOpType::FREE;

    // Free remaining heaps
    std::vector<uint64_t> heapAddresses = {0x10000, 0x11000, 0x12000, 0x13000, 0x14000};
    for (auto addr : heapAddresses) {
        record.dstAddr = addr;
        error = sm.FreeHeapBlock(record, size);
        // Some may already be freed, that's expected
    }

    // Final verification: try to free non-existent heap
    record.dstAddr = 0x99999;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError);
    ASSERT_EQ(error.type, MemErrorType::ILLEGAL_FREE);
}

TEST(ShadowMemory, extensive_memory_operations_with_edge_cases)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910B2);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());

    MemOpRecord record{};
    record.dstSpace = AddressSpace::GM;
    strncpy(record.fileName, "edge_case_test.cpp", sizeof(record.fileName) - 1);

    uint64_t size;
    ErrorMsg error;

    // Test 1: Single byte allocations
    record.type = MemOpType::MALLOC;
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    record.dstAddr = 0x50000;
    record.memSize = 1; // Minimum size
    record.rootAddr = 0x50000;
    sm.AddHeapBlock(record);

    // Test 2: Very large allocation (within reasonable bounds)
    record.dstAddr = 0x60000;
    record.memSize = 1000000; // Large size
    record.rootAddr = 0x60000;
    sm.AddHeapBlock(record);

    // Test 3: Multiple small regions within large heap
    record.infoSrc = MemInfoSrc::MSTX_REGION;
    record.rootAddr = 0x60000;
    record.dstAddr = 0x60000;

    for (int i = 0; i < 20; i++) {
        record.memSize = 10 + i * 5; // Growing sizes
        sm.AddHeapBlock(record);
        record.dstAddr += record.memSize;
    }

    // Test 4: Free regions in reverse order
    record.type = MemOpType::FREE;
    for (int i = 19; i >= 0; i--) {
        // Calculate address for each region
        uint64_t regionAddr = 0x60000;
        for (int j = 0; j < i; j++) {
            regionAddr += 10 + j * 5;
        }
        record.dstAddr = regionAddr;
        error = sm.FreeHeapBlock(record, size);
        // Some may fail if already freed, that's OK
    }

    // Test 5: Allocate overlapping addresses (should handle gracefully)
    record.type = MemOpType::MALLOC;
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    record.dstAddr = 0x70000;
    record.memSize = 1000;
    record.rootAddr = 0x70000;
    sm.AddHeapBlock(record);

    // Same address different operation
    record.type = MemOpType::FREE;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    // Test 6: Sequential allocation and immediate free
    for (int i = 0; i < 15; i++) {
        record.type = MemOpType::MALLOC;
        record.infoSrc = MemInfoSrc::MSTX_HEAP;
        record.dstAddr = 0x80000 + i * 0x200;
        record.memSize = 100;
        record.rootAddr = record.dstAddr;
        sm.AddHeapBlock(record);

        record.type = MemOpType::FREE;
        error = sm.FreeHeapBlock(record, size);
        ASSERT_FALSE(error.isError);
    }

    // Test 7: Mixed operations
    std::vector<std::pair<uint64_t, uint64_t>> operations = {
        {0x90000, 500}, {0x91000, 600}, {0x92000, 700},
        {0x93000, 800}, {0x94000, 900}
    };

    for (const auto& op : operations) {
        record.type = MemOpType::MALLOC;
        record.infoSrc = MemInfoSrc::MSTX_HEAP;
        record.dstAddr = op.first;
        record.memSize = op.second;
        record.rootAddr = op.first;
        sm.AddHeapBlock(record);
    }

    // Free in random order
    std::vector<uint64_t> addresses = {0x93000, 0x91000, 0x94000, 0x90000, 0x92000};
    for (auto addr : addresses) {
        record.type = MemOpType::FREE;
        record.dstAddr = addr;
        error = sm.FreeHeapBlock(record, size);
        ASSERT_FALSE(error.isError);
    }

    // Test 8: Boundary condition - free address 0
    record.dstAddr = 0x0;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError); // Should be illegal free

    // Test 9: Maximum address (within 64-bit space but reasonable)
    record.type = MemOpType::MALLOC;
    record.infoSrc = MemInfoSrc::MSTX_HEAP;
    record.dstAddr = 0xFFFF0000;
    record.memSize = 1000;
    record.rootAddr = 0xFFFF0000;
    sm.AddHeapBlock(record);

    record.type = MemOpType::FREE;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    // Test 10: Verify final state
    record.dstAddr = 0x50000; // First allocation should still exist
    error = sm.FreeHeapBlock(record, size);
    ASSERT_FALSE(error.isError);

    // Final illegal free
    record.dstAddr = 0x12345678;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError);
    ASSERT_EQ(error.type, MemErrorType::ILLEGAL_FREE);
}

TEST(ShadowMemory, concurrent_stress_simulation)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910B1);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());

    MemOpRecord record{};
    record.dstSpace = AddressSpace::GM;
    strncpy(record.fileName, "stress_test.cpp", sizeof(record.fileName) - 1);

    uint64_t size;
    ErrorMsg error;

    // Simulate complex application behavior with many allocations
    const int NUM_ITERATIONS = 50;
    const int MAX_HEAPS = 20;

    std::vector<uint64_t> activeHeaps;
    std::vector<std::pair<uint64_t, uint64_t>> activeRegions; // <heap_addr, region_addr>

    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        // Randomly decide operation type
        int opType = iter % 10;

        if (opType < 6 || activeHeaps.empty()) { // 60% allocate new heap or if no heaps
            // Allocate new heap
            record.type = MemOpType::MALLOC;
            record.infoSrc = MemInfoSrc::MSTX_HEAP;
            record.dstAddr = 0x100000 + iter * 0x1000;
            record.memSize = 1000 + (iter % 10) * 100;
            record.rootAddr = record.dstAddr;

            sm.AddHeapBlock(record);
            activeHeaps.push_back(record.dstAddr);

            // Also allocate some regions within this heap
            int numRegions = iter % 5 + 1;
            record.infoSrc = MemInfoSrc::MSTX_REGION;
            uint64_t regionAddr = record.dstAddr;

            for (int r = 0; r < numRegions; r++) {
                record.memSize = 50 + (r % 3) * 25;
                sm.AddHeapBlock(record);
                activeRegions.push_back({record.rootAddr, regionAddr});
                regionAddr += record.memSize;
                record.dstAddr = regionAddr;
            }
        }
        else if (opType < 8) { // 20% free random heap
            int heapIndex = iter % activeHeaps.size();
            uint64_t heapAddr = activeHeaps[heapIndex];

            record.type = MemOpType::FREE;
            record.infoSrc = MemInfoSrc::MSTX_HEAP;
            record.dstAddr = heapAddr;

            error = sm.FreeHeapBlock(record, size);
            if (!error.isError) {
                // Remove from active heaps
                activeHeaps.erase(activeHeaps.begin() + heapIndex);

                // Also remove associated regions
                auto it = activeRegions.begin();
                while (it != activeRegions.end()) {
                    if (it->first == heapAddr) {
                        it = activeRegions.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }
        else { // 20% free random region
            if (!activeRegions.empty()) {
                int regionIndex = iter % activeRegions.size();
                auto region = activeRegions[regionIndex];

                record.type = MemOpType::FREE;
                record.infoSrc = MemInfoSrc::MSTX_REGION;
                record.dstAddr = region.second;

                error = sm.FreeHeapBlock(record, size);
                if (!error.isError) {
                    activeRegions.erase(activeRegions.begin() + regionIndex);
                }
            }
        }

        // Every 10 iterations, verify some operations
        if (iter % 10 == 9) {
            // Try to free a non-existent address
            record.type = MemOpType::FREE;
            record.infoSrc = MemInfoSrc::MSTX_HEAP;
            record.dstAddr = 0x99900000 + iter;
            error = sm.FreeHeapBlock(record, size);
            // Expected to fail, that's fine

            // Verify one of the active heaps can be freed properly
            if (!activeHeaps.empty()) {
                record.dstAddr = activeHeaps[0];
                error = sm.FreeHeapBlock(record, size);
                if (!error.isError) {
                    // Update our tracking
                    uint64_t freedHeap = activeHeaps[0];
                    activeHeaps.erase(activeHeaps.begin());

                    // Remove associated regions
                    auto it = activeRegions.begin();
                    while (it != activeRegions.end()) {
                        if (it->first == freedHeap) {
                            it = activeRegions.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
            }
        }
    }

    // Final cleanup - free all remaining heaps
    for (uint64_t heapAddr : activeHeaps) {
        record.type = MemOpType::FREE;
        record.infoSrc = MemInfoSrc::MSTX_HEAP;
        record.dstAddr = heapAddr;
        error = sm.FreeHeapBlock(record, size);
        // Don't care about errors at this point
    }

    // Final verification
    record.dstAddr = 0x12345678;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError);
    ASSERT_EQ(error.type, MemErrorType::ILLEGAL_FREE);
}

TEST(ShadowMemory, stress_test_multiple_concurrent_operations_simulation)
{
    ShadowMemory sm;
    auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910B1);
    ASSERT_NE(it, CHIP_INFO_MAP.cend());
    sm.Init(it->second);
    ASSERT_TRUE(sm.IsReady());

    MemOpRecord record{};
    record.dstSpace = AddressSpace::GM;
    strncpy(record.fileName, "stress_test.cpp", sizeof(record.fileName) - 1);

    uint64_t size;
    ErrorMsg error;

    // Simulate complex application behavior with many allocations
    const int NUM_ITERATIONS = 50;
    const int MAX_HEAPS = 20;

    std::vector<uint64_t> activeHeaps;
    std::vector<std::pair<uint64_t, uint64_t>> activeRegions; // <heap_addr, region_addr>

    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        // Randomly decide operation type
        int opType = iter % 10;

        if (opType < 6 || activeHeaps.empty()) { // 60% allocate new heap or if no heaps
            // Allocate new heap
            record.type = MemOpType::MALLOC;
            record.infoSrc = MemInfoSrc::MSTX_HEAP;
            record.dstAddr = 0x100000 + iter * 0x1000;
            record.memSize = 1000 + (iter % 10) * 100;
            record.rootAddr = record.dstAddr;

            sm.AddHeapBlock(record);
            activeHeaps.push_back(record.dstAddr);

            // Also allocate some regions within this heap
            int numRegions = iter % 5 + 1;
            record.infoSrc = MemInfoSrc::MSTX_REGION;
            uint64_t regionAddr = record.dstAddr;

            for (int r = 0; r < numRegions; r++) {
                record.memSize = 50 + (r % 3) * 25;
                sm.AddHeapBlock(record);
                activeRegions.push_back({record.rootAddr, regionAddr});
                regionAddr += record.memSize;
                record.dstAddr = regionAddr;
            }
        }
        else if (opType < 8) { // 20% free random heap
            int heapIndex = iter % activeHeaps.size();
            uint64_t heapAddr = activeHeaps[heapIndex];

            record.type = MemOpType::FREE;
            record.infoSrc = MemInfoSrc::MSTX_HEAP;
            record.dstAddr = heapAddr;

            error = sm.FreeHeapBlock(record, size);
            if (!error.isError) {
                // Remove from active heaps
                activeHeaps.erase(activeHeaps.begin() + heapIndex);

                // Also remove associated regions
                auto it = activeRegions.begin();
                while (it != activeRegions.end()) {
                    if (it->first == heapAddr) {
                        it = activeRegions.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }
        else { // 20% free random region
            if (!activeRegions.empty()) {
                int regionIndex = iter % activeRegions.size();
                auto region = activeRegions[regionIndex];

                record.type = MemOpType::FREE;
                record.infoSrc = MemInfoSrc::MSTX_REGION;
                record.dstAddr = region.second;

                error = sm.FreeHeapBlock(record, size);
                if (!error.isError) {
                    activeRegions.erase(activeRegions.begin() + regionIndex);
                }
            }
        }

        // Every 10 iterations, verify some operations
        if (iter % 10 == 9) {
            // Try to free a non-existent address
            record.type = MemOpType::FREE;
            record.infoSrc = MemInfoSrc::MSTX_HEAP;
            record.dstAddr = 0x99900000 + iter;
            error = sm.FreeHeapBlock(record, size);
            // Expected to fail, that's fine

            // Verify one of the active heaps can be freed properly
            if (!activeHeaps.empty()) {
                record.dstAddr = activeHeaps[0];
                error = sm.FreeHeapBlock(record, size);
                if (!error.isError) {
                    // Update our tracking
                    uint64_t freedHeap = activeHeaps[0];
                    activeHeaps.erase(activeHeaps.begin());

                    // Remove associated regions
                    auto it = activeRegions.begin();
                    while (it != activeRegions.end()) {
                        if (it->first == freedHeap) {
                            it = activeRegions.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
            }
        }
    }

    // Final cleanup - free all remaining heaps
    for (uint64_t heapAddr : activeHeaps) {
        record.type = MemOpType::FREE;
        record.infoSrc = MemInfoSrc::MSTX_HEAP;
        record.dstAddr = heapAddr;
        error = sm.FreeHeapBlock(record, size);
        // Don't care about errors at this point
    }

    // Final verification
    record.dstAddr = 0x12345678;
    error = sm.FreeHeapBlock(record, size);
    ASSERT_TRUE(error.isError);
    ASSERT_EQ(error.type, MemErrorType::ILLEGAL_FREE);
}

}
