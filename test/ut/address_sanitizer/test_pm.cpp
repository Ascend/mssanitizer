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
#include <any>
#include <mutex>

#define private public
#include "pm.h"
#undef private

using namespace Sanitizer;

constexpr uint64_t ONE_SM_STAND_FOR_BYTE = 65536U; // 64KB
constexpr uint8_t MEM_DEFAULT_VALUE = 0b11111100;
constexpr uint8_t CHIPMEM_DEFAULT_VALUE = 0b11111110;

TEST(PM, construct_pm_expect_construct_success)
{
    uint64_t byteNum = 1 << 20;
    PM pm(byteNum, MEM_DEFAULT_VALUE);
}

TEST(PM, pm_get_range1d_expect_success_and_has_correct_size)
{
    uint64_t byteNum = 1 << 20;
    PM pm(byteNum, MEM_DEFAULT_VALUE);
    Range1D range = pm.GetRange(0, 10);
    ASSERT_EQ(range.Size(), 10);
}

TEST(PM, empty_range_expect_begin_iterator_equal_to_end_iterator)
{
    uint64_t byteNum = 1 << 20;
    PM pm(byteNum, MEM_DEFAULT_VALUE);
    Range1D range = pm.GetRange(0, 0);
    ASSERT_EQ(range.Begin(), range.End());
}

TEST(PM, nonempty_range_expect_begin_iterator_equal_not_to_end_iterator)
{
    uint64_t byteNum = 1 << 20;
    PM pm(byteNum, MEM_DEFAULT_VALUE);
    Range1D range = pm.GetRange(0, 10);
    ASSERT_NE(range.Begin(), range.End());
}

TEST(PM, range_get_iterator_at_valid_addr_expect_return_valid_iterator)
{
    uint64_t byteNum = 1 << 20;
    PM pm(byteNum, MEM_DEFAULT_VALUE);
    Range1D range = pm.GetRange(0, 10);
    Range1D::Iterator it = range.At(5);
    ASSERT_NE(it, range.End());
    ASSERT_EQ(it.addr_, 5);
}

TEST(PM, range_get_iterator_at_invalid_addr_expect_return_iterator_equal_to_range_end)
{
    uint64_t byteNum = 1 << 20;
    PM pm(byteNum, MEM_DEFAULT_VALUE);
    Range1D range = pm.GetRange(0, 10);
    Range1D::Iterator it = range.At(15);
    ASSERT_EQ(it, range.End());
}

TEST(PM, range_set_range_bits_expect_pm_get_correct_bits)
{
    uint64_t byteNum = 10;
    PM pm(byteNum, MEM_DEFAULT_VALUE);

    Range1D range = pm.GetRange(1, 7);
    range.Set(0x77);

    Range1D fullRange = pm.GetRange(0, 10);
    uint64_t index = 0UL;
    for (auto it = fullRange.Begin(); it != fullRange.End(); ++it, ++index) {
        if (index >= 1 && index < 8) {
            ASSERT_EQ(it.GetBits(), 0x77);
        } else {
            ASSERT_EQ(it.GetBits(), 0xFC);
        }
    }
}

TEST(PM, range_get_unified_range_size_on_unified_pm_expect_correct_remain_range_size)
{
    uint64_t byteNum = 10;
    PM pm(byteNum, MEM_DEFAULT_VALUE);
    Range1D range = pm.GetRange(0, 10);
    for (uint64_t addr = 0UL; addr < byteNum; ++addr) {
        Range1D::Iterator it = range.At(addr);
        ASSERT_EQ(range.UnifiedRangeAfter(it).Size(), byteNum - addr);
    }
}

TEST(PM, iterator_for_range_increase_once_expect_move_to_next_addr)
{
    uint64_t byteNum = 10;
    PM pm(byteNum, MEM_DEFAULT_VALUE);
    Range1D range = pm.GetRange(0, 10);
    Range1D::Iterator it = range.At(5);
    ++it;
    ASSERT_EQ(it, range.At(6));
    ASSERT_NE(it, range.End());
}

TEST(PM, iterator_for_range_increase_once_at_all_repeat_end_expect_move_to_range_end)
{
    uint64_t byteNum = 10;
    PM pm(byteNum, MEM_DEFAULT_VALUE);
    Range1D range = pm.GetRange(0, 10);
    Range1D::Iterator it = range.At(9);
    ++it;
    ASSERT_EQ(it, range.End());
}

TEST(PM, iterator_for_range_increase_n_expect_move_to_next_n_addr)
{
    uint64_t byteNum = 10;
    PM pm(byteNum, MEM_DEFAULT_VALUE);
    Range1D range = pm.GetRange(0, 10);
    Range1D::Iterator it = range.At(5);
    it += 3;
    ASSERT_EQ(it, range.At(8));
    ASSERT_NE(it, range.End());
}

TEST(PM, reset_should_clear_previous_writes)
{
    auto byteNum = uint64_t{10};
    auto pm = PM{byteNum, CHIPMEM_DEFAULT_VALUE};
    auto op_range = pm.GetRange(2, 8);
    op_range.Set(0b0);
    auto range0 = pm.GetRange(0, 10);
    auto iter0 = range0.At(0);
    ASSERT_EQ(iter0.GetBits(), CHIPMEM_DEFAULT_VALUE);
    iter0 += 2;
    ASSERT_EQ(iter0.GetBits(), 0b0);
    pm.Reset(CHIPMEM_DEFAULT_VALUE);
    auto range1 = pm.GetRange(0, 10);
    auto iter1 = range1.At(0);
    ASSERT_EQ(iter1.GetBits(), CHIPMEM_DEFAULT_VALUE);
    iter1 += 2;
    // bit should be CHIPMEM_DEFAULT_VALUE after reset
    ASSERT_EQ(iter1.GetBits(), CHIPMEM_DEFAULT_VALUE);
}

TEST(GMPM, set_max_addr_then_get_expect_correct)
{
    GmPM pm(MEM_DEFAULT_VALUE);
    pm.Set(0xffffffffffff, 10, 0x0);
    uint64_t size = 10;
    Range1D range = pm.GetRange(0xffffffffffff - size, size);
    Range1D::Iterator it = range.At(0xffffffffffff - 1);
    ASSERT_NE(it, range.End());
}

TEST(GMPM, set_max_addr_then_get_expect_error)
{
    GmPM pm(MEM_DEFAULT_VALUE);
    pm.Set(0xffffffffffff, 1024, 0x0);
    uint64_t size = 10;
    Range1D range = pm.GetRange(0xffffffffffff - size, size);
    Range1D::Iterator it = range.At(0xffffffffffff);
    ASSERT_EQ(it, range.End());
}

TEST(GMPM, set_large_size_then_get_expect_error)
{
    GmPM pm(MEM_DEFAULT_VALUE);
    pm.Set(0xffffffffffff, 1024, 0x0);
    uint64_t size = 1024 * 1024 * 1024UL;
    Range1D range = pm.GetRange(0, size);
    Range1D::Iterator it = range.At(0x15);
    ASSERT_NE(it, range.End());
}

TEST(GMPM, set_large_size_then_get_expect_correct)
{
    GmPM pm(MEM_DEFAULT_VALUE);
    pm.Set(0xffffffffffff, 1024, 0x0);
    uint64_t size = 1024 * 1024 * 1024UL;
    Range1D range = pm.GetRange(0, size);
    Range1D::Iterator it = range.At(size);
    ASSERT_EQ(it, range.End());
}

TEST(PM, status_scan_matches_scalar_oracle_with_owner_bits_and_tails)
{
    PM pm(Sanitizer::ONE_SM_STAND_FOR_BYTE * 2, 0xFE);
    const uint64_t base = Sanitizer::ONE_SM_STAND_FOR_BYTE - 37;
    std::vector<uint8_t> values(128);
    uint32_t seed = 17;
    for (size_t i = 0; i < values.size(); ++i) {
        seed = seed * 1664525U + 1013904223U;
        values[i] = static_cast<uint8_t>(seed >> 24);
        pm.Set(base + i, 1, values[i]);
    }
    for (size_t start = 0; start < 48; ++start) {
        for (size_t length = 0; length < 64; ++length) {
            for (uint8_t status = 0; status < 4; ++status) {
                bool expected = false;
                for (size_t i = start; i < start + length; ++i) {
                    expected = expected || (values[i] & 3U) == status;
                }
                EXPECT_EQ(pm.RangeHasStatus(base + start, length, status), expected);
            }
        }
    }
    EXPECT_FALSE(pm.RangeHasStatus(0, 0, 2));
    EXPECT_TRUE(pm.RangeHasStatus(0, 16, 2));
    EXPECT_FALSE(pm.RangeHasStatus(0, 16, 1));
}

TEST(PM, unified_scan_preserves_owner_bits_and_exact_tail_boundaries)
{
    PM pm(Sanitizer::ONE_SM_STAND_FOR_BYTE * 2, 0xFE);
    // 同时覆盖 SM 边界截断和未对齐地址上的整字扫描、逐字节尾部。
    for (uint64_t base : {Sanitizer::ONE_SM_STAND_FOR_BYTE - 7, Sanitizer::ONE_SM_STAND_FOR_BYTE + 3}) {
        for (uint64_t length = 1; length < 40; ++length) {
            pm.Set(base, 48, 0xFE);
            pm.Set(base + length, 1, 0x02); // 低 2 位相同，归属位不同，仍必须终止同值段。
            const uint64_t firstBlock = std::min(length,
                Sanitizer::ONE_SM_STAND_FOR_BYTE - base % Sanitizer::ONE_SM_STAND_FOR_BYTE);
            EXPECT_EQ(pm.UnifiedSizeAfter(base, 48, base), firstBlock);
            EXPECT_EQ(pm.UnifiedSizeAfter(base, length, base), firstBlock);
        }
    }
}

TEST(GMPM, status_scan_handles_uniform_materialized_and_cross_pm_ranges)
{
    GmPM pm(0xFE);
    const uint64_t boundary = LOCAL_MEM_MASK;
    EXPECT_FALSE(pm.RangeHasStatus(boundary - 8, 0, 2));
    EXPECT_TRUE(pm.RangeHasStatus(boundary - 8, 16, 2));
    EXPECT_FALSE(pm.RangeHasStatus(boundary - 8, 16, 1));
    pm.Set(boundary - 1, 2, 0xFD);
    EXPECT_TRUE(pm.RangeHasStatus(boundary - 8, 16, 1));
    EXPECT_TRUE(pm.RangeHasStatus(boundary, 1, 1));
    EXPECT_FALSE(pm.RangeHasStatus(boundary + 1, 7, 1));
}
