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
#include <sstream>

#include "mem_error_def.h"

using namespace Sanitizer;

TEST(MemErrorDef, error_msg_without_error_expect_equal_to_self)
{
    ErrorMsg msg;
    msg.isError = false;
    ASSERT_EQ(msg, msg);
}

TEST(MemErrorDef, error_msg_with_error_expect_equal_to_self)
{
    ErrorMsg msg;
    msg.SetType(MemErrorType::ILLEGAL_ADDR_READ, AddressSpace::GM, 0x1234);
    msg.SetLocInfo("test.cpp", 0, 0x1234, 1, BlockType::AIVEC);
    ASSERT_EQ(msg, msg);
}

TEST(MemErrorDef, error_msg_with_error_expect_not_equal_to_error_msg_without_error)
{
    ErrorMsg msg1;
    msg1.SetType(MemErrorType::ILLEGAL_ADDR_READ, AddressSpace::GM, 0x1234);
    msg1.SetLocInfo("test.cpp", 0, 0x1234, 1, BlockType::AIVEC);
    ErrorMsg msg2;
    msg2.isError = false;
    ASSERT_FALSE(msg1 == msg2);
}

TEST(MemErrorDef, get_hash_of_error_msg_twice_expect_equal)
{
    ErrorMsg msg;
    msg.SetType(MemErrorType::ILLEGAL_ADDR_READ, AddressSpace::GM, 0x1234);
    msg.SetLocInfo("test.cpp", 0, 0x1234, 1, BlockType::AIVEC);
    ASSERT_EQ(msg(msg), msg(msg));
}

TEST(MemErrorDef, format_block_idx_list_of_size_0_expect_return_nothing)
{
    BlockIdxList blockIdxList = {{}};
    std::stringstream oss;
    oss << blockIdxList;
    ASSERT_EQ(oss.str(), "");
}

TEST(MemErrorDef, format_block_idx_list_of_size_1_expect_return_idx)
{
    BlockIdxList blockIdxList = {{1}};
    std::stringstream oss;
    oss << blockIdxList;
    ASSERT_EQ(oss.str(), "1");
}

TEST(MemErrorDef, format_block_idx_list_of_size_2_expect_return_idxes_split_by_comma)
{
    BlockIdxList blockIdxList = {{1, 2}};
    std::stringstream oss;
    oss << blockIdxList;
    ASSERT_EQ(oss.str(), "1-2");
}

TEST(MemErrorDef, format_discrete_block_idx_list_of_size_3_expect_return_idxes_split_by_comma)
{
    BlockIdxList blockIdxList = {{1, 3, 4}};
    std::stringstream oss;
    oss << blockIdxList;
    ASSERT_EQ(oss.str(), "1,3-4");
}

TEST(MemErrorDef, format_block_info_with_aivec_block_idxes_expect_return_only_aivec_blocks)
{
    ReducedErrorMsg msg{ErrorMsg{}, {1, 3, 4}, {}, {}};
    std::stringstream oss;
    oss << FormatBlockInfo{msg, true};
    ASSERT_EQ(oss.str(), "======    in block aiv(1,3-4)");
}

TEST(MemErrorDef, format_block_info_with_aicube_block_idxes_expect_return_only_aicube_blocks)
{
    ReducedErrorMsg msg{ErrorMsg{}, {}, {2, 4, 5}, {}};
    std::stringstream oss;
    oss << FormatBlockInfo{msg, true};
    ASSERT_EQ(oss.str(), "======    in block aic(2,4-5)");
}

TEST(MemErrorDef, format_block_info_with_both_aivec_and_aicube_block_idxes_expect_return_both_blocks)
{
    ReducedErrorMsg msg{ErrorMsg{}, {1, 3, 4}, {2, 4, 5}, {}};
    std::stringstream oss;
    oss << FormatBlockInfo{msg, true};
    ASSERT_EQ(oss.str(), "======    in block aiv(1,3-4),aic(2,4-5)");
}

TEST(MemErrorDef, format_simt_block_info_expect_return_thread_info_blocks)
{
    ErrorMsg errorMsg{};
    errorMsg.auxData.isSimt = true;
    errorMsg.auxData.displayThread = true;
    errorMsg.auxData.threadLoc = {30, 20, 5};
    ReducedErrorMsg msg{errorMsg, {1, 3, 4}, {2, 4, 5}, {}};
    std::stringstream oss;
    oss << FormatBlockInfo{msg, true};
    ASSERT_EQ(oss.str(), "======    by thread (30,20,5) in block aiv(1,3-4),aic(2,4-5)");
}

TEST(MemErrorDef, format_simt_block_info_expect_return_no_thread_info_blocks)
{
    ErrorMsg errorMsg{};
    errorMsg.auxData.isSimt = false;
    errorMsg.auxData.threadLoc = {30, 20, 5};
    ReducedErrorMsg msg{errorMsg, {1, 3, 4}, {2, 4, 5}, {}};
    std::stringstream oss;
    oss << FormatBlockInfo{msg, true};
    ASSERT_EQ(oss.str(), "======    in block aiv(1,3-4),aic(2,4-5)");
}

TEST(MemErrorDef, format_block_info_with_aicore_block_idxes_expect_return_aicore_blocks)
{
    ReducedErrorMsg msg{ErrorMsg{}, {}, {}, {2, 4, 5}};
    std::stringstream oss;
    oss << FormatBlockInfo{msg, false};
    ASSERT_EQ(oss.str(), "======    in block aicore(2,4-5)");
}

TEST(MemErrorDef, set_type_expect_success_and_equal_to_origin)
{
    ErrorMsg msg;
    msg.SetType(MemErrorType::MEM_LEAK, AddressSpace::GM, 0x00);
    ASSERT_EQ(msg.type, MemErrorType::MEM_LEAK);
    ASSERT_EQ(msg.auxData.space, AddressSpace::GM);
    ASSERT_EQ(msg.auxData.badAddr.addr, 0x00);
}

TEST(MemErrorDef, format_out_of_bounds_error_msg_expect_success)
{
    ErrorMsg msg;
    msg.SetType(MemErrorType::OUT_OF_BOUNDS, AddressSpace::GM, 0x61);
    std::stringstream oss;
    oss << ReducedErrorMsg{msg, {0}, {}, {}};
    ASSERT_TRUE(oss.str().find("out of bounds") != std::string::npos);
}

TEST(MemErrorDef, format_illegal_addr_write_error_msg_expect_success)
{
    ErrorMsg msg;
    msg.SetType(MemErrorType::ILLEGAL_ADDR_WRITE, AddressSpace::GM, 0x61);
    std::stringstream oss;
    oss << ReducedErrorMsg{msg, {0}, {}, {}};
    ASSERT_TRUE(oss.str().find("illegal write") != std::string::npos);
}

TEST(MemErrorDef, format_illegal_addr_read_error_msg_expect_success)
{
    ErrorMsg msg;
    msg.SetType(MemErrorType::ILLEGAL_ADDR_READ, AddressSpace::GM, 0x61);
    std::stringstream oss;
    oss << ReducedErrorMsg{msg, {0}, {}, {}};
    ASSERT_TRUE(oss.str().find("illegal read") != std::string::npos);
}

TEST(MemErrorDef, format_misaligned_access_error_msg_expect_success)
{
    ErrorMsg msg;
    msg.SetType(MemErrorType::MISALIGNED_ACCESS, AddressSpace::GM, 0x61);
    std::stringstream oss;
    oss << ReducedErrorMsg{msg, {0}, {}, {}};
    ASSERT_TRUE(oss.str().find("misaligned access") != std::string::npos);
}

TEST(MemErrorDef, format_illegal_free_error_msg_expect_success)
{
    ErrorMsg msg;
    msg.SetType(MemErrorType::ILLEGAL_FREE, AddressSpace::GM, 0x61);
    std::stringstream oss;
    oss << ReducedErrorMsg{msg, {0}, {}, {}};
    ASSERT_TRUE(oss.str().find("illegal free") != std::string::npos);
}

TEST(MemErrorDef, format_mem_leak_error_msg_expect_success)
{
    ErrorMsg msg;
    msg.SetType(MemErrorType::MEM_LEAK, AddressSpace::GM, 0x61);
    std::stringstream oss;
    oss << ReducedErrorMsg{msg, {0}, {}, {}};
    ASSERT_TRUE(oss.str().find("Direct leak") != std::string::npos);
}

TEST(MemErrorDef, format_internal_error_msg_expect_sucesss)
{
    ErrorMsg msg;
    msg.SetType(MemErrorType::INTERNAL_ERROR, AddressSpace::GM, 0x61);
    std::stringstream oss;
    oss << ReducedErrorMsg{msg, {0}, {}, {}};
    ASSERT_TRUE(oss.str().find("internal errors") != std::string::npos);
}

TEST(MemErrorDef, format_invalid_error_msg_expect_success)
{
    ErrorMsg msg;
    msg.SetType(static_cast<MemErrorType>(10), AddressSpace::GM, 0x61);
    std::stringstream oss;
    oss << ReducedErrorMsg{msg, {0}, {}, {}};
    ASSERT_TRUE(oss.str().empty());
}
