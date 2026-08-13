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
    msg.errorMsg.auxData.side = MemOpSide::KERNEL;
    std::stringstream oss;
    oss << FormatBlockInfo{msg, true};
    ASSERT_EQ(oss.str(), "======    in block aiv(1,3-4) ");
}

TEST(MemErrorDef, format_block_info_with_aicube_block_idxes_expect_return_only_aicube_blocks)
{
    ReducedErrorMsg msg{ErrorMsg{}, {}, {2, 4, 5}, {}};
    msg.errorMsg.auxData.side = MemOpSide::KERNEL;
    std::stringstream oss;
    oss << FormatBlockInfo{msg, true};
    ASSERT_EQ(oss.str(), "======    in block aic(2,4-5) ");
}

TEST(MemErrorDef, format_block_info_with_both_aivec_and_aicube_block_idxes_expect_return_both_blocks)
{
    ReducedErrorMsg msg{ErrorMsg{}, {1, 3, 4}, {2, 4, 5}, {}};
    msg.errorMsg.auxData.side = MemOpSide::KERNEL;
    std::stringstream oss;
    oss << FormatBlockInfo{msg, true};
    ASSERT_EQ(oss.str(), "======    in block aiv(1,3-4),aic(2,4-5) ");
}

TEST(MemErrorDef, format_simt_block_info_expect_return_thread_info_blocks)
{
    ErrorMsg errorMsg{};
    errorMsg.auxData.side = MemOpSide::KERNEL;
    errorMsg.auxData.isSimt = true;
    errorMsg.auxData.displayThread = true;
    errorMsg.auxData.threadLoc = {30, 20, 5};
    ReducedErrorMsg msg{errorMsg, {1, 3, 4}, {2, 4, 5}, {}};
    std::stringstream oss;
    oss << FormatBlockInfo{msg, true};
    ASSERT_EQ(oss.str(), "======    by thread (30,20,5) in block aiv(1,3-4),aic(2,4-5) ");
}

TEST(MemErrorDef, format_simt_block_info_expect_return_no_thread_info_blocks)
{
    ErrorMsg errorMsg{};
    errorMsg.auxData.side = MemOpSide::KERNEL;
    errorMsg.auxData.isSimt = false;
    errorMsg.auxData.threadLoc = {30, 20, 5};
    ReducedErrorMsg msg{errorMsg, {1, 3, 4}, {2, 4, 5}, {}};
    std::stringstream oss;
    oss << FormatBlockInfo{msg, true};
    ASSERT_EQ(oss.str(), "======    in block aiv(1,3-4),aic(2,4-5) ");
}

TEST(MemErrorDef, format_block_info_with_aicore_block_idxes_expect_return_aicore_blocks)
{
    ReducedErrorMsg msg{ErrorMsg{}, {}, {}, {2, 4, 5}};
    msg.errorMsg.auxData.side = MemOpSide::KERNEL;
    std::stringstream oss;
    oss << FormatBlockInfo{msg, false};
    ASSERT_EQ(oss.str(), "======    in block aicore(2,4-5) ");
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
    msg.SetType(static_cast<MemErrorType>(100), AddressSpace::GM, 0x61);
    std::stringstream oss;
    oss << ReducedErrorMsg{msg, {0}, {}, {}};
    ASSERT_TRUE(oss.str().empty());
}

TEST(MemErrorDef, format_non_default_reg_error_msg_expect_success)
{
    ErrorMsg msg;
    msg.type = MemErrorType::NON_DEFAULT_REG;
    msg.isError = true;
    msg.auxData.instrName = InstrName::VADD;
    msg.auxData.maskMode = MaskMode::MASK_NORM;
    msg.auxData.vectorMask.mask0 = 0x0000FFFFULL;
    msg.auxData.vectorMask.mask1 = 0ULL;
    std::stringstream oss;
    oss << ReducedErrorMsg{msg, {0}, {}, {}};
    ASSERT_TRUE(oss.str().find("non-default mask") != std::string::npos);
    ASSERT_TRUE(oss.str().find("VADD") != std::string::npos);
    ASSERT_TRUE(oss.str().find("MaskMode") != std::string::npos);
}

TEST(MemErrorDef, format_mask_mode_norm_expect_normal_string)
{
    std::stringstream oss;
    oss << MaskMode::MASK_NORM;
    ASSERT_EQ(oss.str(), "NORM");
}

TEST(MemErrorDef, format_mask_mode_count_expect_counter_string)
{
    std::stringstream oss;
    oss << MaskMode::MASK_COUNT;
    ASSERT_EQ(oss.str(), "COUNT");
}

TEST(MemErrorDef, format_instr_name_vadd_expect_vadd_string)
{
    std::stringstream oss;
    oss << InstrName::VADD;
    ASSERT_EQ(oss.str(), "VADD");
}

TEST(MemErrorDef, format_instr_name_none_expect_default_string)
{
    std::stringstream oss;
    oss << InstrName::NONE;
    ASSERT_TRUE(oss.str().find("InstrName") != std::string::npos);
}

TEST(MemErrorDef, error_msg_with_vector_mask_expect_equal_when_same_mask)
{
    ErrorMsg msg1;
    msg1.SetType(MemErrorType::NON_DEFAULT_REG, AddressSpace::GM, 0x0);
    msg1.isError = true;
    msg1.auxData.vectorMask.mask0 = 0x0000FFFFULL;
    msg1.auxData.vectorMask.mask1 = 0ULL;
    msg1.auxData.maskMode = MaskMode::MASK_NORM;

    ErrorMsg msg2;
    msg2.SetType(MemErrorType::NON_DEFAULT_REG, AddressSpace::GM, 0x0);
    msg2.isError = true;
    msg2.auxData.vectorMask.mask0 = 0x0000FFFFULL;
    msg2.auxData.vectorMask.mask1 = 0ULL;
    msg2.auxData.maskMode = MaskMode::MASK_NORM;

    ASSERT_TRUE(msg1 == msg2);
}

TEST(MemErrorDef, error_msg_with_vector_mask_expect_equal_when_different_mask) {
    ErrorMsg msg1;
    msg1.SetType(MemErrorType::NON_DEFAULT_REG, AddressSpace::GM, 0x0);
    msg1.isError = true;
    msg1.auxData.vectorMask.mask0 = 0x0000FFFFULL;
    msg1.auxData.vectorMask.mask1 = 0ULL;

    ErrorMsg msg2;
    msg2.SetType(MemErrorType::NON_DEFAULT_REG, AddressSpace::GM, 0x0);
    msg2.isError = true;
    msg2.auxData.vectorMask.mask0 = ~0ULL;
    msg2.auxData.vectorMask.mask1 = ~0ULL;

    ASSERT_TRUE(msg1 == msg2);
}

TEST(MemErrorDef, error_msg_with_mask_mode_expect_equal_when_different_mode) {
    ErrorMsg msg1;
    msg1.SetType(MemErrorType::NON_DEFAULT_REG, AddressSpace::GM, 0x0);
    msg1.isError = true;
    msg1.auxData.maskMode = MaskMode::MASK_NORM;

    ErrorMsg msg2;
    msg2.SetType(MemErrorType::NON_DEFAULT_REG, AddressSpace::GM, 0x0);
    msg2.isError = true;
    msg2.auxData.maskMode = MaskMode::MASK_COUNT;

    ASSERT_TRUE(msg1 == msg2);
}

TEST(MemErrorDef, format_non_default_reg_with_mask_count_expect_counter_mode)
{
    ErrorMsg msg;
    msg.type = MemErrorType::NON_DEFAULT_REG;
    msg.isError = true;
    msg.auxData.instrName = InstrName::VMUL;
    msg.auxData.maskMode = MaskMode::MASK_COUNT;
    msg.auxData.vectorMask.mask0 = 0ULL;
    msg.auxData.vectorMask.mask1 = 0ULL;
    std::stringstream oss;
    oss << ReducedErrorMsg{msg, {0}, {}, {}};
    ASSERT_TRUE(oss.str().find("VMUL") != std::string::npos);
    ASSERT_TRUE(oss.str().find("Counter") != std::string::npos);
}

TEST(MemErrorDef, aux_data_default_init_expect_vector_mask_zero) {
    ErrorMsg::AuxData auxData;
    ASSERT_EQ(auxData.vectorMask.mask0, 0ULL);
    ASSERT_EQ(auxData.vectorMask.mask1, 0ULL);
    ASSERT_EQ(auxData.maskMode, MaskMode::MASK_NORM);
    ASSERT_EQ(auxData.instrName, InstrName::NONE);
}

TEST(MemErrorDef, format_instr_name_vconv_expect_vconv_string) {
    std::stringstream oss;
    oss << InstrName::VCONV;
    ASSERT_EQ(oss.str(), "VCONV");
}

TEST(MemErrorDef, format_instr_name_vsub_expect_vsub_string) {
    std::stringstream oss;
    oss << InstrName::VSUB;
    ASSERT_EQ(oss.str(), "VSUB");
}

TEST(MemErrorDef, format_instr_name_vector_dup_expect_vector_dup_string) {
    std::stringstream oss;
    oss << InstrName::VECTOR_DUP;
    ASSERT_EQ(oss.str(), "VECTOR_DUP");
}

TEST(MemErrorDef, error_msg_with_instr_name_expect_equal_when_different_instr) {
    ErrorMsg msg1;
    msg1.SetType(MemErrorType::NON_DEFAULT_REG, AddressSpace::GM, 0x0);
    msg1.isError = true;
    msg1.auxData.instrName = InstrName::VADD;

    ErrorMsg msg2;
    msg2.SetType(MemErrorType::NON_DEFAULT_REG, AddressSpace::GM, 0x0);
    msg2.isError = true;
    msg2.auxData.instrName = InstrName::VSUB;

    ASSERT_TRUE(msg1 == msg2);
}

TEST(MemErrorDef, format_non_default_reg_with_hex_mask_expect_hex_output) {
    ErrorMsg msg;
    msg.type = MemErrorType::NON_DEFAULT_REG;
    msg.isError = true;
    msg.auxData.instrName = InstrName::VADD;
    msg.auxData.maskMode = MaskMode::MASK_NORM;
    msg.auxData.vectorMask.mask0 = 0x0000FFFFULL;
    msg.auxData.vectorMask.mask1 = 0xFF000000ULL;
    std::stringstream oss;
    oss << ReducedErrorMsg{msg, {0}, {}, {}};
    std::string output = oss.str();
    ASSERT_TRUE(output.find("0xff000000") != std::string::npos);
    ASSERT_TRUE(output.find("0xffff") != std::string::npos);
}

TEST(MemErrorDef, format_non_default_reg_default_mask_expect_no_report) {
    ErrorMsg msg;
    msg.type = MemErrorType::NON_DEFAULT_REG;
    msg.isError = true;
    msg.auxData.instrName = InstrName::VADD;
    msg.auxData.maskMode = MaskMode::MASK_NORM;
    msg.auxData.vectorMask.mask0 = ~0ULL;
    msg.auxData.vectorMask.mask1 = ~0ULL;
    std::stringstream oss;
    oss << ReducedErrorMsg{msg, {0}, {}, {}};
    ASSERT_TRUE(oss.str().find("VADD") != std::string::npos);
    ASSERT_TRUE(oss.str().find("non-default mask") != std::string::npos);
}

TEST(MemErrorDef, format_instr_name_vgather_expect_vgather_string) {
    std::stringstream oss;
    oss << InstrName::VGATHER;
    ASSERT_EQ(oss.str(), "VGATHER");
}
