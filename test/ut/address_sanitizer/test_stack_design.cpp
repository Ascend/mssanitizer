/* Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 * Licensed under Mulan PSL v2. See LICENSE in the repository root.
 */
#include <gtest/gtest.h>
#include <limits>
#include <sstream>
#include <tuple>

#include "address_sanitizer/asan_action.h"
#include "address_sanitizer/bounds_check.h"
#include "address_sanitizer/mem_error_def.h"
#include "core/framework/platform_config.h"

using namespace Sanitizer;

namespace {
// Synthetic bounds test the algorithm, not a physical-core/stack allocation formula.
class StackRangeBoundary : public testing::TestWithParam<std::tuple<uint64_t, uint64_t, uint64_t>> {};

TEST_P(StackRangeBoundary, checks_entire_access_and_counts_only_invalid_bytes)
{
    const auto [addr, width, badBytes] = GetParam();
    UnionBounds bounds(0x1000, 0x20);
    for (AccessType access : {AccessType::READ, AccessType::WRITE}) {
        const auto error = bounds.Check(addr, width, access);
        EXPECT_EQ(error.isError, badBytes != 0);
        if (error.isError) {
            EXPECT_EQ(error.auxData.badAddr.addr, addr);
            EXPECT_EQ(error.auxData.nBadBytes, badBytes);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(FullWidth, StackRangeBoundary, testing::Values(
    std::make_tuple(0xFFFULL, 1ULL, 1ULL),
    std::make_tuple(0xFFFULL, 4ULL, 1ULL),
    std::make_tuple(0xFF0ULL, 0x40ULL, 0x20ULL),
    std::make_tuple(0x1000ULL, 1ULL, 0ULL),
    std::make_tuple(0x1000ULL, 8ULL, 0ULL),
    std::make_tuple(0x1018ULL, 8ULL, 0ULL),
    std::make_tuple(0x101CULL, 4ULL, 0ULL),
    std::make_tuple(0x101EULL, 2ULL, 0ULL),
    std::make_tuple(0x101FULL, 1ULL, 0ULL),
    std::make_tuple(0x101FULL, 2ULL, 1ULL),
    std::make_tuple(0x101EULL, 4ULL, 2ULL),
    std::make_tuple(0x101CULL, 8ULL, 4ULL),
    std::make_tuple(0x1020ULL, 4ULL, 4ULL),
    std::make_tuple(0x1021ULL, 4ULL, 4ULL)));

TEST(StackDesign, wrapping_access_cannot_hide_valid_overlap_or_wrap_to_another_allocation)
{
    constexpr uint64_t max = std::numeric_limits<uint64_t>::max();
    UnionBounds bounds(0x1000, 0x20);
    auto error = bounds.Check(0x1010, max, AccessType::READ);
    ASSERT_TRUE(error.isError);
    EXPECT_EQ(error.auxData.nBadBytes, max - 16);
    error = bounds.Check(max - 1, 4, AccessType::WRITE);
    EXPECT_EQ(error.auxData.nBadBytes, 4);

    DiscreteBounds discrete;
    ASSERT_FALSE(discrete.Add(0x1000, 0x20, Bounds::DEFAULT_PERMISSION).isError);
    error = discrete.Check(0x1010, max, AccessType::READ);
    ASSERT_TRUE(error.isError);
    EXPECT_EQ(error.auxData.nBadBytes, max - 16);
}

TEST(StackDesign, range_results_match_independent_byte_membership_oracle)
{
    UnionBounds bounds(17, 31);
    for (uint64_t addr = 0; addr < 64; ++addr) {
        for (uint64_t width : {1ULL, 2ULL, 4ULL, 8ULL, 16ULL, 64ULL}) {
            uint64_t bad = 0;
            for (uint64_t byte = 0; byte < width; ++byte) {
                bad += addr + byte < 17 || addr + byte >= 48;
            }
            const auto error = bounds.Check(addr, width, AccessType::READ);
            ASSERT_EQ(error.isError, bad != 0) << addr << ":" << width;
            if (bad != 0) {
                ASSERT_EQ(error.auxData.nBadBytes, bad) << addr << ":" << width;
            }
        }
    }
}

TEST(StackDesign, zero_offset_is_legal_private_data_and_not_a_null_pointer)
{
    BoundsCheck bounds;
    bounds.Init(CHIP_INFO_MAP.at(DeviceType::ASCEND_910B1));
    for (uint64_t width : {1ULL, 2ULL, 4ULL, 8ULL}) {
        EXPECT_FALSE(bounds.Check(AddressSpace::PRIVATE, 0, width, AccessType::READ).isError);
        EXPECT_FALSE(bounds.Check(AddressSpace::PRIVATE, 0, width, AccessType::WRITE).isError);
        EXPECT_TRUE(bounds.Check(AddressSpace::UNKNOWN, 0, width, AccessType::READ).isError);
        EXPECT_TRUE(bounds.Check(AddressSpace::UNKNOWN, 0, width, AccessType::WRITE).isError);
    }
}

TEST(StackDesign, private_tail_report_preserves_original_invalid_byte_format)
{
    const auto chip = CHIP_INFO_MAP.at(DeviceType::ASCEND_910B1);
    BoundsCheck bounds;
    bounds.Init(chip);
    ShadowMemory shadow;
    Config config{};
    config.memCheck = true;
    for (MemOpType op : {MemOpType::LOAD, MemOpType::STORE}) {
        MemOpRecord record{};
        record.type = op;
        record.dstSpace = AddressSpace::PRIVATE;
        record.dstAddr = chip.privateSize - 2;
        record.memSize = 4;
        record.side = MemOpSide::KERNEL;
        auto action = AsanActionFactory::CreateAsanAction(record);
        auto errors = action.doAction(shadow, bounds, config, true);
        ASSERT_EQ(errors.size(), 1);
        const auto &error = errors.front();
        EXPECT_EQ(error.auxData.nBadBytes, 2);
        std::ostringstream out;
        out << ReducedErrorMsg{error, {}, {}, {}};
        const std::string expected = op == MemOpType::LOAD ?
            "====== ERROR: illegal read of size 2\n" : "====== ERROR: illegal write of size 2\n";
        EXPECT_EQ(out.str().substr(0, expected.size()), expected);
        EXPECT_EQ(out.str().find("access_width:"), std::string::npos);
        EXPECT_EQ(out.str().find("invalid_bytes:"), std::string::npos);
        EXPECT_EQ(out.str().find("stack_range_scope:"), std::string::npos);
        EXPECT_EQ(out.str().find("pool_base:"), std::string::npos);
        EXPECT_EQ(out.str().find("pool_length:"), std::string::npos);
        EXPECT_EQ(out.str().find("range_reason:"), std::string::npos);
    }
}

} // namespace
