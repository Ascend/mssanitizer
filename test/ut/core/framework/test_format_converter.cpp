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
#include "format_converter.h"
#include "event_def.h"
#include "platform_config.h"
#include "constant.h"

using namespace Sanitizer;

class QueryPipeTypeTest : public testing::TestWithParam<std::tuple<MemType, MemType, PipeType>> {};
TEST_P(QueryPipeTypeTest, query_pipe_type_given_src_and_dst_mem_types_and_expect_correct_pipe_type)
{
    auto [srcMem, dstMem, expected] = GetParam();
    ASSERT_EQ(FormatConverter::QueryPipeType(srcMem, dstMem), expected);
}
INSTANTIATE_TEST_CASE_P(FormatConverter, QueryPipeTypeTest, testing::Values(
    std::make_tuple(MemType::L1, MemType::L0A, PipeType::PIPE_MTE1),
    std::make_tuple(MemType::L1, MemType::L0B, PipeType::PIPE_MTE1),
    std::make_tuple(MemType::L1, MemType::L0C, PipeType::PIPE_MTE1),
    std::make_tuple(MemType::L1, MemType::UB, PipeType::PIPE_MTE1),
    std::make_tuple(MemType::L1, MemType::BT, PipeType::PIPE_MTE1),
    std::make_tuple(MemType::L1, MemType::GM, PipeType::PIPE_MTE3),
    std::make_tuple(MemType::GM, MemType::L0A, PipeType::PIPE_MTE2),
    std::make_tuple(MemType::GM, MemType::L0B, PipeType::PIPE_MTE2),
    std::make_tuple(MemType::GM, MemType::L1, PipeType::PIPE_MTE2),
    std::make_tuple(MemType::GM, MemType::UB, PipeType::PIPE_MTE2),
    std::make_tuple(MemType::UB, MemType::GM, PipeType::PIPE_MTE3),
    std::make_tuple(MemType::UB, MemType::L1, PipeType::PIPE_MTE3),
    std::make_tuple(MemType::UB, MemType::L0C, PipeType::PIPE_MTE3),
    std::make_tuple(MemType::L0C, MemType::UB, PipeType::PIPE_V),
    std::make_tuple(MemType::UB, MemType::UB, PipeType::PIPE_V),
    std::make_tuple(MemType::L0C, MemType::L1, PipeType::PIPE_FIX),
    std::make_tuple(MemType::L1, MemType::FB, PipeType::PIPE_FIX),
    std::make_tuple(MemType::INVALID, MemType::INVALID, PipeType::SIZE)
));

class MemTypeToAddrSpaceTest : public testing::TestWithParam<std::tuple<MemType, AddressSpace>> {};
TEST_P(MemTypeToAddrSpaceTest, mem_type_to_addr_space_given_mem_type_and_expect_correct_addr_space)
{
    auto [memType, expected] = GetParam();
    ASSERT_EQ(FormatConverter::MemTypeToAddrSpace(memType), expected);
}
INSTANTIATE_TEST_CASE_P(FormatConverter, MemTypeToAddrSpaceTest, testing::Values(
    std::make_tuple(MemType::L1, AddressSpace::L1),
    std::make_tuple(MemType::L0A, AddressSpace::L0A),
    std::make_tuple(MemType::L0B, AddressSpace::L0B),
    std::make_tuple(MemType::L0C, AddressSpace::L0C),
    std::make_tuple(MemType::UB, AddressSpace::UB),
    std::make_tuple(MemType::BT, AddressSpace::BT),
    std::make_tuple(MemType::FB, AddressSpace::FB),
    std::make_tuple(MemType::GM, AddressSpace::GM),
    std::make_tuple(MemType::INVALID, AddressSpace::INVALID),
    std::make_tuple(MemType::PRIVATE, AddressSpace::PRIVATE)
));

class AddrSpaceToMemTypeTest : public testing::TestWithParam<std::tuple<AddressSpace, MemType>> {};
TEST_P(AddrSpaceToMemTypeTest, addr_space_to_mem_type_given_addr_space_and_expect_correct_mem_type)
{
    auto [addrSpace, expected] = GetParam();
    ASSERT_EQ(AddrSpaceToMemType(addrSpace), expected);
}
INSTANTIATE_TEST_CASE_P(FormatConverter, AddrSpaceToMemTypeTest, testing::Values(
    std::make_tuple(AddressSpace::L1, MemType::L1),
    std::make_tuple(AddressSpace::L0A, MemType::L0A),
    std::make_tuple(AddressSpace::L0B, MemType::L0B),
    std::make_tuple(AddressSpace::L0C, MemType::L0C),
    std::make_tuple(AddressSpace::UB, MemType::UB),
    std::make_tuple(AddressSpace::GM, MemType::GM),
    std::make_tuple(AddressSpace::BT, MemType::BT),
    std::make_tuple(AddressSpace::FB, MemType::FB),
    std::make_tuple(AddressSpace::INVALID, MemType::INVALID),
    std::make_tuple(AddressSpace::PRIVATE, MemType::PRIVATE)
));

TEST(FormatConverter, access_type_to_mem_op_type_given_access_type_and_expect_correct_mem_op_type)
{
    ASSERT_EQ(FormatConverter::AccessTypeToMemOpType(AccessType::READ), MemOpType::LOAD);
    ASSERT_EQ(FormatConverter::AccessTypeToMemOpType(AccessType::WRITE), MemOpType::STORE);
    ASSERT_EQ(FormatConverter::AccessTypeToMemOpType(AccessType::MEMCPY_BLOCKS), MemOpType::MEMCPY_BLOCKS);
    ASSERT_EQ(FormatConverter::AccessTypeToMemOpType(static_cast<AccessType>(100)), MemOpType::INVALID);
}

class GetDataBitSizeByTypeTest : public testing::TestWithParam<std::tuple<DataType, uint16_t, bool>> {};
TEST_P(GetDataBitSizeByTypeTest, get_data_bit_size_by_type_given_data_type_and_expect_correct_bit_size)
{
    auto [dataType, expectedSize, expectedResult] = GetParam();
    uint16_t dataBitSize;
    bool result = FormatConverter::GetDataBitSizeByType(dataType, dataBitSize);
    ASSERT_EQ(result, expectedResult);
    if (expectedResult) {
        ASSERT_EQ(dataBitSize, expectedSize);
    }
}
INSTANTIATE_TEST_CASE_P(FormatConverter, GetDataBitSizeByTypeTest, testing::Values(
    std::make_tuple(DataType::DATA_B4, 4U, true),
    std::make_tuple(DataType::DATA_B8, 8U, true),
    std::make_tuple(DataType::DATA_B16, 16U, true),
    std::make_tuple(DataType::DATA_B32, 32U, true),
    std::make_tuple(static_cast<DataType>(100), 0U, false)
));

class GetDataTypeByDataBitsTest : public testing::TestWithParam<std::tuple<uint8_t, DataType, bool>> {};
TEST_P(GetDataTypeByDataBitsTest, get_data_type_by_data_bits_given_data_bits_and_expect_correct_data_type)
{
    auto [dataBits, expectedType, expectedResult] = GetParam();
    DataType dataType;
    bool result = FormatConverter::GetDataTypeByDataBits(dataBits, dataType);
    ASSERT_EQ(result, expectedResult);
    if (expectedResult) {
        ASSERT_EQ(dataType, expectedType);
    }
}
INSTANTIATE_TEST_CASE_P(FormatConverter, GetDataTypeByDataBitsTest, testing::Values(
    std::make_tuple(4, DataType::DATA_B4, true),
    std::make_tuple(8, DataType::DATA_B8, true),
    std::make_tuple(16, DataType::DATA_B16, true),
    std::make_tuple(32, DataType::DATA_B32, true),
    std::make_tuple(64, DataType::DATA_B4, false)
));

class GetPadModeScaleValueTest : public testing::TestWithParam<std::tuple<PadMode, uint64_t>> {};
TEST_P(GetPadModeScaleValueTest, get_pad_mode_scale_value_given_pad_mode_and_expect_correct_scale_value)
{
    auto [padMode, expected] = GetParam();
    ASSERT_EQ(FormatConverter::GetPadModeScaleValue(padMode), expected);
}
INSTANTIATE_TEST_CASE_P(FormatConverter, GetPadModeScaleValueTest, testing::Values(
    std::make_tuple(PadMode::PAD_MODE1, 32U),
    std::make_tuple(PadMode::PAD_MODE2, 16U),
    std::make_tuple(PadMode::PAD_MODE3, 8U),
    std::make_tuple(PadMode::PAD_MODE4, 4U),
    std::make_tuple(PadMode::PAD_MODE5, 2U),
    std::make_tuple(PadMode::PAD_MODE6, 8U),
    std::make_tuple(PadMode::PAD_MODE7, 4U),
    std::make_tuple(PadMode::PAD_MODE8, 2U),
    std::make_tuple(static_cast<PadMode>(99), 1U)
));

class CheckChannelSizeTest : public testing::TestWithParam<std::tuple<DataType, uint32_t, bool>> {};
TEST_P(CheckChannelSizeTest, check_channel_size_given_data_type_and_channel_and_expect_correct_result)
{
    auto [dataType, channel, expected] = GetParam();
    ASSERT_EQ(FormatConverter::CheckChannelSize(dataType, channel), expected);
}
INSTANTIATE_TEST_CASE_P(FormatConverter, CheckChannelSizeTest, testing::Values(
    std::make_tuple(DataType::DATA_B4, 0U, true),
    std::make_tuple(DataType::DATA_B4, 8U, true),
    std::make_tuple(DataType::DATA_B4, 16U, true),
    std::make_tuple(DataType::DATA_B4, 32U, true),
    std::make_tuple(DataType::DATA_B4, 1U, false),
    std::make_tuple(DataType::DATA_B8, 0U, true),
    std::make_tuple(DataType::DATA_B8, 4U, true),
    std::make_tuple(DataType::DATA_B8, 8U, true),
    std::make_tuple(DataType::DATA_B8, 16U, true),
    std::make_tuple(DataType::DATA_B8, 1U, false),
    std::make_tuple(DataType::DATA_B16, 0U, true),
    std::make_tuple(DataType::DATA_B16, 4U, true),
    std::make_tuple(DataType::DATA_B16, 8U, true),
    std::make_tuple(DataType::DATA_B16, 1U, false),
    std::make_tuple(DataType::DATA_B32, 0U, true),
    std::make_tuple(DataType::DATA_B32, 4U, true),
    std::make_tuple(DataType::DATA_B32, 1U, false),
    std::make_tuple(static_cast<DataType>(100), 0U, false)
));

class GetAtomicFlagTest : public testing::TestWithParam<std::tuple<RecordType, AtomicMode, bool, bool>> {};
TEST_P(GetAtomicFlagTest, get_atomic_flag_given_record_type_and_atomic_mode_and_expect_correct_result)
{
    auto [recordType, atomicMode, expectedResult, expectedEnabled] = GetParam();
    KernelRecord record;
    record.recordType = recordType;
    if (recordType == RecordType::SET_ATOMIC) {
        record.payload.atomicModeRecord.mode = atomicMode;
    }
    bool enabled;
    bool result = FormatConverter::GetAtomicFlag(record, enabled);
    ASSERT_EQ(result, expectedResult);
    if (expectedResult) {
        ASSERT_EQ(enabled, expectedEnabled);
    }
}
INSTANTIATE_TEST_CASE_P(FormatConverter, GetAtomicFlagTest, testing::Values(
    std::make_tuple(RecordType::LOAD, AtomicMode::NONE, false, false),
    std::make_tuple(RecordType::SET_ATOMIC, AtomicMode::SUM, false, false),
    std::make_tuple(RecordType::SET_ATOMIC, AtomicMode::F32, true, true),
    std::make_tuple(RecordType::SET_ATOMIC, AtomicMode::F16, true, true),
    std::make_tuple(RecordType::SET_ATOMIC, AtomicMode::NONE, true, false)
));

class FormatAddressSpaceTest : public testing::TestWithParam<std::tuple<AddressSpace, std::string>> {};
TEST_P(FormatAddressSpaceTest, format_address_space_given_addr_space_and_expect_correct_string)
{
    auto [addrSpace, expected] = GetParam();
    ASSERT_EQ(FormatAddressSpace(addrSpace), expected);
}
INSTANTIATE_TEST_CASE_P(FormatConverter, FormatAddressSpaceTest, testing::Values(
    std::make_tuple(AddressSpace::PRIVATE, "PRIVATE"),
    std::make_tuple(AddressSpace::GM, "GM"),
    std::make_tuple(AddressSpace::L1, "L1"),
    std::make_tuple(AddressSpace::L0A, "L0A"),
    std::make_tuple(AddressSpace::L0B, "L0B"),
    std::make_tuple(AddressSpace::L0C, "L0C"),
    std::make_tuple(AddressSpace::UB, "UB"),
    std::make_tuple(AddressSpace::BT, "BT"),
    std::make_tuple(AddressSpace::FB, "FB"),
    std::make_tuple(AddressSpace::INVALID, "INVALID")
));

TEST(FormatConverter, get_data_size_by_type_given_data_type_and_expect_correct_size)
{
    ASSERT_EQ(FormatConverter::GetDataSizeByType(DataType::DATA_B8), 1U);
    ASSERT_EQ(FormatConverter::GetDataSizeByType(DataType::DATA_B16), 2U);
    ASSERT_EQ(FormatConverter::GetDataSizeByType(DataType::DATA_B32), 4U);
    ASSERT_EQ(FormatConverter::GetDataSizeByType(DataType::DATA_B4), 1U);
    ASSERT_EQ(FormatConverter::GetDataSizeByType(static_cast<DataType>(100)), 1U);
}

class GetAlignSizeTest : public testing::TestWithParam<std::tuple<MemOpInfo, AddressSpace, RecordType, DeviceType, uint16_t>> {};
TEST_P(GetAlignSizeTest, get_align_size_given_mem_op_info_and_expect_correct_align_size)
{
    auto [memInfo, space, recordType, deviceType, expectedAlign] = GetParam();
    ASSERT_EQ(FormatConverter::GetAlignSize(const_cast<MemOpInfo&>(memInfo), space, recordType, deviceType), expectedAlign);
}
INSTANTIATE_TEST_CASE_P(FormatConverter, GetAlignSizeTest, testing::Values(
    std::make_tuple(MemOpInfo{MemType::GM, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 8, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::GM, RecordType::UNARY_OP, DeviceType::ASCEND_310P, 1U),
    std::make_tuple(MemOpInfo{MemType::UB, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::UB, RecordType::VEC_REGPROPCOOR_OP, DeviceType::ASCEND_310P, 4U),
    std::make_tuple(MemOpInfo{MemType::L1, AccessType::WRITE, {64, 0}, MaskMode::MASK_NORM, 64, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::L1, RecordType::VSEL_OP, DeviceType::ASCEND_310P, 8U),
    std::make_tuple(MemOpInfo{MemType::GM, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::GM, RecordType::VMRGSORT4_OP_M200, DeviceType::ASCEND_310P, 32U),
    std::make_tuple(MemOpInfo{MemType::GM, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 16, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::GM, RecordType::VMRGSORT4_OP_M200, DeviceType::ASCEND_310P, 16U),
    std::make_tuple(MemOpInfo{MemType::GM, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::GM, RecordType::VMRGSORT4_OP_C220, DeviceType::ASCEND_910B1, 8U),
    std::make_tuple(MemOpInfo{MemType::GM, AccessType::WRITE, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::GM, RecordType::VMRGSORT4_OP_C220, DeviceType::ASCEND_910B1, 32U),
    std::make_tuple(MemOpInfo{MemType::GM, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::GM, RecordType::VMRGSORT4_OP_C310, DeviceType::ASCEND_950DT_950x, 8U),
    std::make_tuple(MemOpInfo{MemType::GM, AccessType::WRITE, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::GM, RecordType::VMRGSORT4_OP_C310, DeviceType::ASCEND_950DT_950x, 32U),
    std::make_tuple(MemOpInfo{MemType::GM, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 16, 0, 1, 16, 1, 1, 1, 0}, AddressSpace::GM, RecordType::CMPMASK_OP, DeviceType::ASCEND_950DT_950x, 16U),
    std::make_tuple(MemOpInfo{MemType::GM, AccessType::WRITE, {64, 0}, MaskMode::MASK_NORM, 16, 0, 1, 32, 1, 1, 1, 0}, AddressSpace::GM, RecordType::CMPMASK_OP, DeviceType::ASCEND_910B1, 32U),
    std::make_tuple(MemOpInfo{MemType::GM, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::GM, RecordType::DECOMPRESS_HEADER, DeviceType::ASCEND_310P, 32U),
    std::make_tuple(MemOpInfo{MemType::L0C, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 64, 1, 1, 1, 0}, AddressSpace::L0C, RecordType::DMA_MOV_CONV_RELU, DeviceType::ASCEND_310P, 1024U),
    std::make_tuple(MemOpInfo{MemType::GM, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::GM, RecordType::LOAD_SMASK, DeviceType::ASCEND_310P, 2U),
    std::make_tuple(MemOpInfo{MemType::L0C, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1024, 1, 1, 1, 0}, AddressSpace::L0C, RecordType::BROADCAST, DeviceType::ASCEND_310P, 1024U),
    std::make_tuple(MemOpInfo{MemType::L0C, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1024, 1, 1, 1, 0}, AddressSpace::L0C, RecordType::DMA_MOV_DEPTH_WISE, DeviceType::ASCEND_310P, 1024U),
    //进入AlignRecordBySpace分支
    std::make_tuple(MemOpInfo{MemType::UB, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::UB, RecordType::LD, DeviceType::ASCEND_310P, 32U),
    std::make_tuple(MemOpInfo{MemType::L0A, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::L0A, RecordType::LD, DeviceType::ASCEND_310P, 512U),
    std::make_tuple(MemOpInfo{MemType::L0B, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::L0B, RecordType::LD, DeviceType::ASCEND_310P, 512U),
    std::make_tuple(MemOpInfo{MemType::L0C, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::L0C, RecordType::LD, DeviceType::ASCEND_310P, 512U),
    std::make_tuple(MemOpInfo{MemType::L1, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::L1, RecordType::LD, DeviceType::ASCEND_310P, 32U),
    std::make_tuple(MemOpInfo{MemType::GM, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::GM, RecordType::LD, DeviceType::ASCEND_310P, 1U),
    std::make_tuple(MemOpInfo{MemType::BT, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::BT, RecordType::LD, DeviceType::ASCEND_310P, 1U),
    std::make_tuple(MemOpInfo{MemType::FB, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::FB, RecordType::LD, DeviceType::ASCEND_310P, 1U),
    std::make_tuple(MemOpInfo{MemType::PRIVATE, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::PRIVATE, RecordType::LD, DeviceType::ASCEND_310P, 1U),
    std::make_tuple(MemOpInfo{MemType::INVALID, AccessType::READ, {64, 0}, MaskMode::MASK_NORM, 32, 0, 1, 1, 1, 1, 1, 0}, AddressSpace::INVALID, RecordType::LD, DeviceType::ASCEND_310P, 1U)
));
