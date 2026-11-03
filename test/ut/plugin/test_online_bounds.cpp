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
#include <array>
#include <cstring>
#include <limits>
#include <random>
#include <vector>

#include "ccec_defs.h"
#define private public
#include "plugin/online_check.h"
#undef private

using namespace Sanitizer;

namespace {
constexpr uint64_t MAX_ADDRESS = std::numeric_limits<uint64_t>::max();
// Existing physical UB model; these tests do not validate individual UB object sizes.
constexpr uint64_t UB_LIMIT = 253952;
constexpr uint32_t READ_WRITE = MSTX_MEM_PERMISSIONS_REGION_FLAGS_DEFAULT;

uint64_t IntersectionOracle(uint64_t addr, uint64_t size, uint64_t base, uint64_t length)
{
    // Independent endpoint arithmetic: neither end can wrap in this host-only 128-bit oracle.
    using WideAddress = unsigned __int128;
    const WideAddress begin = std::max<WideAddress>(addr, base);
    const WideAddress end = std::min(WideAddress{addr} + size, WideAddress{base} + length);
    return end > begin ? static_cast<uint64_t>(end - begin) : 0;
}

uint64_t InvalidBytesOracle(uint64_t addr, uint64_t size, const std::vector<HostMemoryInfo> &ranges,
    AccessType access)
{
    const uint32_t required = access == AccessType::READ ? MSTX_MEM_PERMISSIONS_REGION_FLAGS_READ :
        access == AccessType::WRITE ? MSTX_MEM_PERMISSIONS_REGION_FLAGS_WRITE : READ_WRITE;
    uint64_t valid = 0;
    // GmReadWriteCheck consumes disjoint ranges after registration/merging; do not double-count allocations.
    for (const auto &range : ranges) {
        if ((range.permission & required) == required) {
            valid += IntersectionOracle(addr, size, range.addr, range.size);
        }
    }
    return size - valid;
}

struct IntersectionCase {
    const char *name;
    uint64_t addr;
    uint64_t size;
    uint64_t base;
    uint64_t length;
    uint64_t overlap;
};

class OnlineIntersection : public testing::TestWithParam<IntersectionCase> {};

TEST_P(OnlineIntersection, counts_only_bytes_inside_both_half_open_ranges)
{
    const auto &input = GetParam();
    OnlineCheck check;
    EXPECT_EQ(check.CalIntersectionSize(input.addr, input.size, input.base, input.length), input.overlap);
    EXPECT_EQ(check.CalIntersectionSize(input.base, input.length, input.addr, input.size), input.overlap);
}

INSTANTIATE_TEST_SUITE_P(OnlineBounds, OnlineIntersection, testing::Values(
    IntersectionCase{"empty_access_before", 0xFFF, 0, 0x1000, 0x20, 0},
    IntersectionCase{"empty_access_inside", 0x1010, 0, 0x1000, 0x20, 0},
    IntersectionCase{"empty_allocation", 0x1000, 8, 0x1004, 0, 0},
    IntersectionCase{"disjoint_left", 0xFF0, 8, 0x1000, 0x20, 0},
    IntersectionCase{"touch_left", 0xFF8, 8, 0x1000, 0x20, 0},
    IntersectionCase{"partial_left", 0xFFC, 8, 0x1000, 0x20, 4},
    IntersectionCase{"same_start", 0x1000, 8, 0x1000, 0x20, 8},
    IntersectionCase{"contained", 0x1008, 8, 0x1000, 0x20, 8},
    IntersectionCase{"same_end", 0x1018, 8, 0x1000, 0x20, 8},
    IntersectionCase{"equal_ranges", 0x1000, 0x20, 0x1000, 0x20, 0x20},
    IntersectionCase{"contains_allocation", 0xFF0, 0x40, 0x1000, 0x20, 0x20},
    IntersectionCase{"partial_right", 0x101C, 8, 0x1000, 0x20, 4},
    IntersectionCase{"touch_right", 0x1020, 8, 0x1000, 0x20, 0},
    IntersectionCase{"disjoint_right", 0x1028, 8, 0x1000, 0x20, 0},
    IntersectionCase{"wrapping_width", 0x1010, MAX_ADDRESS, 0x1000, 0x20, 16},
    IntersectionCase{"no_wrap_to_zero", MAX_ADDRESS - 1, 4, 0, 16, 0},
    IntersectionCase{"last_address_byte", MAX_ADDRESS, 1, MAX_ADDRESS, 1, 1}),
    [](const testing::TestParamInfo<IntersectionCase> &info) { return info.param.name; });

TEST(OnlineBounds, intersection_matches_exhaustive_byte_membership)
{
    OnlineCheck check;
    // Every interval relationship in a small domain, including empty intervals and touching ends.
    for (uint64_t base = 0; base <= 16; ++base) {
        for (uint64_t length = 0; length <= 16; ++length) {
            for (uint64_t addr = 0; addr <= 16; ++addr) {
                for (uint64_t size = 0; size <= 16; ++size) {
                    uint64_t expected = 0;
                    for (uint64_t byte = addr; byte < addr + size; ++byte) {
                        expected += byte >= base && byte < base + length;
                    }
                    ASSERT_EQ(check.CalIntersectionSize(addr, size, base, length), expected)
                        << "access=" << addr << "," << size << " range=" << base << "," << length;
                }
            }
        }
    }
}

TEST(OnlineBounds, intersection_matches_wide_oracle_at_uint64_boundaries)
{
    OnlineCheck check;
    const uint64_t values[] = {0, 1, 2, 7, 8, 15, 16, 31, 32,
        MAX_ADDRESS - 32, MAX_ADDRESS - 16, MAX_ADDRESS - 8, MAX_ADDRESS - 1, MAX_ADDRESS};
    for (uint64_t base : values) {
        for (uint64_t length : values) {
            for (uint64_t addr : values) {
                for (uint64_t size : values) {
                    ASSERT_EQ(check.CalIntersectionSize(addr, size, base, length),
                        IntersectionOracle(addr, size, base, length))
                        << "access=" << addr << "," << size << " range=" << base << "," << length;
                }
            }
        }
    }
}

TEST(OnlineBounds, intersection_matches_wide_oracle_for_reproducible_full_width_inputs)
{
    OnlineCheck check;
    std::mt19937_64 random(0x20261102);
    for (uint32_t sample = 0; sample < 10000; ++sample) {
        const uint64_t addr = random();
        const uint64_t size = random();
        const uint64_t base = random();
        const uint64_t length = random();
        const auto actual = check.CalIntersectionSize(addr, size, base, length);
        ASSERT_EQ(actual, IntersectionOracle(addr, size, base, length)) << "sample=" << sample;
        ASSERT_EQ(actual, check.CalIntersectionSize(base, length, addr, size)) << "sample=" << sample;
        ASSERT_LE(actual, std::min(size, length)) << "sample=" << sample;
    }
}

TEST(OnlineBounds, gm_disjoint_ranges_gaps_and_permissions_count_invalid_bytes)
{
    OnlineCheck check;
    RecordBlockHead block{};
    std::vector<HostMemoryInfo> ranges = {
        {0x1000, 16, MSTX_MEM_PERMISSIONS_REGION_FLAGS_READ},
        {0x1010, 16, MSTX_MEM_PERMISSIONS_REGION_FLAGS_WRITE},
        {0x1030, 16, READ_WRITE}, {0x1040, 8, MSTX_MEM_PERMISSIONS_REGION_FLAGS_NONE},
        {0x1050, 0, READ_WRITE}};
    block.hostMemoryInfoPtr = ranges.data();
    block.hostMemoryNum = ranges.size();
    check.simdBlockHead_ = &block;
    for (auto access : {AccessType::READ, AccessType::WRITE, AccessType::MEMCPY_BLOCKS}) {
        for (uint64_t addr = 0xFF0; addr <= 0x1058; ++addr) {
            for (uint64_t size : {0ULL, 1ULL, 2ULL, 4ULL, 8ULL, 16ULL, 32ULL, 64ULL, 128ULL}) {
                AddrInfo info{};
                info.space = AddressSpace::GM;
                info.opType = access;
                info.addr = addr;
                info.size = size;
                uint64_t illegal = 0;
                const uint64_t expected = InvalidBytesOracle(addr, size, ranges, access);
                ASSERT_EQ(check.GmReadWriteCheck(info, illegal), expected != 0);
                ASSERT_EQ(illegal, expected) << "access=" << static_cast<int>(access)
                    << " addr=" << addr << " size=" << size;
            }
        }
    }
}

TEST(OnlineBounds, gm_empty_allocations_and_overflow_do_not_wrap_into_low_memory)
{
    OnlineCheck check;
    RecordBlockHead block{};
    check.simdBlockHead_ = &block;
    for (auto ranges : {std::vector<HostMemoryInfo>{},
        std::vector<HostMemoryInfo>{{0, 16, READ_WRITE}, {MAX_ADDRESS - 15, 15, READ_WRITE}}}) {
        block.hostMemoryInfoPtr = ranges.data();
        block.hostMemoryNum = ranges.size();
        for (uint64_t addr : {uint64_t{0}, uint64_t{8}, MAX_ADDRESS - 16, MAX_ADDRESS - 7, MAX_ADDRESS}) {
            for (uint64_t size : {uint64_t{0}, uint64_t{1}, uint64_t{16}, MAX_ADDRESS}) {
                for (auto access : {AccessType::READ, AccessType::WRITE}) {
                    AddrInfo info{};
                    info.space = AddressSpace::GM;
                    info.opType = access;
                    info.addr = addr;
                    info.size = size;
                    uint64_t illegal = 0;
                    const uint64_t expected = InvalidBytesOracle(addr, size, ranges, access);
                    ASSERT_EQ(check.GmReadWriteCheck(info, illegal), expected != 0);
                    ASSERT_EQ(illegal, expected) << "addr=" << addr << " size=" << size;
                }
            }
        }
    }
}

TEST(OnlineBounds, ub_endpoints_and_extreme_widths_match_wide_oracle)
{
    OnlineCheck check;
    const uint64_t addresses[] = {0, 1, 7, 8, UB_LIMIT - 16, UB_LIMIT - 8, UB_LIMIT - 1,
        UB_LIMIT, UB_LIMIT + 1, MAX_ADDRESS - 8, MAX_ADDRESS};
    const uint64_t widths[] = {0, 1, 2, 4, 8, 16, UB_LIMIT - 1, UB_LIMIT, UB_LIMIT + 1,
        MAX_ADDRESS - 8, MAX_ADDRESS - 1, MAX_ADDRESS};
    for (auto access : {AccessType::READ, AccessType::WRITE}) {
        for (uint64_t addr : addresses) {
            for (uint64_t size : widths) {
                AddrInfo info{};
                info.space = AddressSpace::UB;
                info.opType = access;
                info.addr = addr;
                info.size = size;
                uint64_t illegal = 0;
                const uint64_t expected = size - IntersectionOracle(addr, size, 0, UB_LIMIT);
                ASSERT_EQ(check.UbReadWriteCheck(info, illegal), expected != 0)
                    << "addr=" << addr << " size=" << size;
                ASSERT_EQ(illegal, expected) << "addr=" << addr << " size=" << size;
            }
        }
    }
}

TEST(OnlineBounds, ub_small_widths_match_byte_membership_near_both_boundaries)
{
    OnlineCheck check;
    for (uint64_t origin : {uint64_t{0}, UB_LIMIT - 16}) {
        for (uint64_t addr = origin; addr <= origin + 32; ++addr) {
            for (uint64_t size = 0; size <= 32; ++size) {
                uint64_t expected = 0;
                for (uint64_t byte = addr; byte < addr + size; ++byte) {
                    expected += byte >= UB_LIMIT;
                }
                AddrInfo info{};
                info.space = AddressSpace::UB;
                info.addr = addr;
                info.size = size;
                uint64_t illegal = 0;
                ASSERT_EQ(check.UbReadWriteCheck(info, illegal), expected != 0);
                ASSERT_EQ(illegal, expected) << "addr=" << addr << " size=" << size;
            }
        }
    }
}

TEST(OnlineBounds, gm_and_ub_checks_do_not_claim_other_address_spaces)
{
    OnlineCheck check;
    for (auto space : {AddressSpace::GM, AddressSpace::UB, AddressSpace::PRIVATE, AddressSpace::UNKNOWN}) {
        AddrInfo info{};
        info.space = space;
        info.size = 8;
        uint64_t illegal = 7;
        if (space != AddressSpace::GM) {
            EXPECT_FALSE(check.GmReadWriteCheck(info, illegal));
            EXPECT_EQ(illegal, 7);
        }
        if (space != AddressSpace::UB) {
            EXPECT_FALSE(check.UbReadWriteCheck(info, illegal));
            EXPECT_EQ(illegal, 7);
        }
    }
}

void ExpectOnlineRecord(AddressSpace space, uint64_t addr, uint64_t width, DetailedDataType type,
    std::vector<HostMemoryInfo> ranges, bool write, bool enabled)
{
    SCOPED_TRACE(testing::Message() << "space=" << static_cast<int>(space) << " addr=" << addr
        << " width=" << width << " write=" << write << " enabled=" << enabled);
    const size_t simtOffset = sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    std::vector<uint8_t> buffer(simtOffset + sizeof(SimtRecordBlockHead) + 4096, 0);
    auto head = reinterpret_cast<RecordGlobalHead *>(buffer.data());
    *head = RecordGlobalHead{};
    head->checkParms.memcheck = enabled;
    head->offsetInfo.simtErrorInfo.size = sizeof(SimtRecordBlockHead) + 4096;
    auto block = reinterpret_cast<RecordBlockHead *>(buffer.data() + sizeof(RecordGlobalHead));
    *block = RecordBlockHead{};
    block->hostMemoryInfoPtr = ranges.data();
    block->hostMemoryNum = ranges.size();
    auto simt = reinterpret_cast<SimtRecordBlockHead *>(buffer.data() + simtOffset);
    OnlineCheck check;
    check.Init(buffer.data(), reinterpret_cast<uint8_t *>(simt), reinterpret_cast<uint8_t *>(block), 0);
    SimtLoadStoreRecord record{};
    record.addr = addr;
    record.size = width;
    record.space = space;
    record.detailedDataType = type;
    record.location.pc = 0x80;
    record.location.blockId = 2;
    record.threadLoc.idX = 3;
    if (space == AddressSpace::GM) {
        if (write) {
            check.Process<RecordType::SIMT_STG>(record);
        } else {
            check.Process<RecordType::SIMT_LDG>(record);
        }
    } else if (write) {
        check.Process<RecordType::SIMT_STS>(record);
    } else {
        check.Process<RecordType::SIMT_LDS>(record);
    }
    const auto access = write ? AccessType::WRITE : AccessType::READ;
    const uint64_t bad = space == AddressSpace::GM ? InvalidBytesOracle(addr, width, ranges, access) :
        width - IntersectionOracle(addr, width, 0, UB_LIMIT);
    // All calls below use actual 1/2/4/8/16-byte types, whose alignment equals their width.
    const bool misaligned = addr % width != 0;
    const uint32_t errors = enabled ? static_cast<uint32_t>(bad != 0) + misaligned : 0;
    ASSERT_EQ(simt->recordCount, errors != 0 ? 1 : 0);
    ASSERT_EQ(simt->recordWriteCount, simt->recordCount);
    if (errors == 0) {
        EXPECT_EQ(simt->writeOffset, 0);
        return;
    }
    const uint8_t *cursor = reinterpret_cast<const uint8_t *>(simt + 1);
    RecordType marker{};
    std::memcpy(&marker, cursor, sizeof(marker));
    ASSERT_EQ(marker, RecordType::ONLINE_ERROR);
    cursor += sizeof(marker);
    KernelErrorRecord header{};
    std::memcpy(&header, cursor, sizeof(header));
    ASSERT_EQ(header.errorNum, errors);
    ASSERT_EQ(header.recordSize, sizeof(record));
    EXPECT_EQ(header.recordType, space == AddressSpace::GM ?
        (write ? RecordType::SIMT_STG : RecordType::SIMT_LDG) :
        (write ? RecordType::SIMT_STS : RecordType::SIMT_LDS));
    cursor += sizeof(header);
    SimtLoadStoreRecord saved{};
    std::memcpy(&saved, cursor, sizeof(saved));
    EXPECT_EQ(saved.addr, addr);
    EXPECT_EQ(saved.size, width);
    EXPECT_EQ(saved.space, space);
    EXPECT_EQ(saved.detailedDataType, type);
    cursor += sizeof(saved);
    uint32_t illegalErrors = 0;
    uint32_t alignmentErrors = 0;
    for (uint32_t i = 0; i < errors; ++i) {
        KernelErrorDesc error{};
        std::memcpy(&error, cursor, sizeof(error));
        cursor += sizeof(error);
        EXPECT_EQ(error.space, space);
        EXPECT_EQ(error.location.pc, 0x80);
        EXPECT_EQ(error.location.blockId, 2);
        EXPECT_EQ(error.threadLoc.idX, 3);
        if (error.errorType == KernelErrorType::MISALIGNED_ACCESS) {
            ++alignmentErrors;
        } else {
            ++illegalErrors;
            EXPECT_EQ(error.errorType,
                write ? KernelErrorType::ILLEGAL_ADDR_WRITE : KernelErrorType::ILLEGAL_ADDR_READ);
            EXPECT_EQ(error.payload.illegalDesc.addr, addr);
            EXPECT_EQ(error.payload.illegalDesc.illegalSize, bad);
        }
    }
    EXPECT_EQ(illegalErrors, bad != 0 ? 1 : 0);
    EXPECT_EQ(alignmentErrors, misaligned ? 1 : 0);
}

TEST(OnlineBounds, load_store_error_records_preserve_invalid_bytes_and_location)
{
    const DetailedDataType types[] = {DetailedDataType::U8, DetailedDataType::U16, DetailedDataType::B32,
        DetailedDataType::B64, DetailedDataType::B128};
    for (size_t i = 0; i < 5; ++i) {
        const uint64_t width = uint64_t{1} << i;
        for (bool write : {false, true}) {
            for (auto space : {AddressSpace::GM, AddressSpace::UB}) {
                const uint64_t base = space == AddressSpace::GM ? 0x2001000 : 0;
                const uint64_t end = space == AddressSpace::GM ? base + 32 : UB_LIMIT;
                for (uint64_t addr : {base, end - width, end, end + 1, end - width / 2}) {
                    ExpectOnlineRecord(space, addr, width, types[i], {{base, end - base, READ_WRITE}}, write, true);
                }
            }
            ExpectOnlineRecord(AddressSpace::GM, 0x2001000, width, types[i],
                {{0x2001000 + (width + 1) / 2, 32, READ_WRITE}}, write, true);
        }
    }
    for (bool write : {false, true}) {
        ExpectOnlineRecord(AddressSpace::GM, 0x2001000, 16, DetailedDataType::B128,
            {{0x2001004, 8, READ_WRITE}}, write, true);
    }
}

TEST(OnlineBounds, disabled_memcheck_emits_no_bounds_or_alignment_error)
{
    for (bool write : {false, true}) {
        ExpectOnlineRecord(AddressSpace::GM, 0x2001001, 8, DetailedDataType::B64, {}, write, false);
        ExpectOnlineRecord(AddressSpace::UB, UB_LIMIT - 1, 8, DetailedDataType::B64, {}, write, false);
    }
}
} // namespace
