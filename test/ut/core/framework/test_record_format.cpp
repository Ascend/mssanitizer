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


#include <map>
#include <sstream>
#include <gtest/gtest.h>

#include "core/framework/record_defs.h"
#include "core/framework/record_format.h"
#include "sanitizer_report.h"

using namespace Sanitizer;
namespace SanitizerTest {

enum class SampleEnum {
    ENUM0 = 0,
    ENUM1,
    ENUM2
};

TEST(RecordFormat, format_enum_with_valid_value_expect_return_correct_name)
{
    static const std::map<SampleEnum, std::string> sampleEnumMap = {
        {SampleEnum::ENUM0, "ENUM0"},
        {SampleEnum::ENUM1, "ENUM1"},
        {SampleEnum::ENUM2, "ENUM2"},
    };

    std::stringstream ss;
    FormatEnum(ss, sampleEnumMap, SampleEnum::ENUM0);
    ASSERT_EQ(ss.str(), sampleEnumMap.at(SampleEnum::ENUM0));
    ss.str("");
    FormatEnum(ss, sampleEnumMap, SampleEnum::ENUM1);
    ASSERT_EQ(ss.str(), sampleEnumMap.at(SampleEnum::ENUM1));
    ss.str("");
    FormatEnum(ss, sampleEnumMap, SampleEnum::ENUM2);
    ASSERT_EQ(ss.str(), sampleEnumMap.at(SampleEnum::ENUM2));
}

TEST(RecordFormat, format_enum_with_invalid_value_expect_return_invalid)
{
    static const std::map<SampleEnum, std::string> sampleEnumMap = {
        {SampleEnum::ENUM0, "ENUM0"},
        {SampleEnum::ENUM1, "ENUM1"},
        {SampleEnum::ENUM2, "ENUM2"},
    };

    std::stringstream ss;
    FormatEnum(ss, sampleEnumMap, static_cast<SampleEnum>(3), "SampleEnum");
    ASSERT_EQ(ss.str(), "Unknown SampleEnum(3)");
}

TEST(RecordFormat, format_device_info_summary_expect_return_correct_result)
{
    DeviceInfoSummary deviceInfoSummary = {
        .device = DeviceType::ASCEND_910B1
    };
    std::stringstream ss;
    ss << deviceInfoSummary;
    auto const &format = ss.str();
    ASSERT_EQ(format, std::string("[summary] device:2"));
}

TEST(RecordFormat, format_location_expect_return_correct_result)
{
    Location location = {
        .fileNo = 12,
        .lineNo = 34,
        .pc = 0x1234,
        .blockId = 3
    };
    std::stringstream ss;
    ss << location;
    auto const &format = ss.str();
    ASSERT_EQ(format, std::string("3, loc--1-34, 0x1234"));
}

TEST(RecordFormat, format_compare_mask_expect_return_correct_result)
{
    CompareMask compareMask = {
        .mask0 = 0x12,
        .mask1 = 0x34
    };
    std::stringstream ss;
    ss << compareMask;
    auto const &format = ss.str();
    ASSERT_EQ(format, std::string("(0x34,0x12)"));
}

TEST(RecordFormat, format_load_store_record_expect_correct_result)
{
    LoadStoreRecord record = {
        .addr = 0x12,
        .size = 34,
        .location = {56, 78, 0, 111},
        .space = AddressSpace::UB,
    };
    std::stringstream ss;
    ss << record;
    std::string format = ss.str();
    ASSERT_NE(format.find("0x12"), std::string::npos);
    ASSERT_NE(format.find("34"), std::string::npos);
    ASSERT_NE(format.find("78"), std::string::npos);
    ASSERT_NE(format.find("111"), std::string::npos);
    ASSERT_NE(format.find("UB"), std::string::npos);
}

TEST(RecordFormat, format_dma_mov_record_expect_correct_result)
{
    DmaMovRecord record = {
        .dst = 0x12,
        .src = 0x34,
        .location = {56, 78, 0, 555},
        .nBurst = 111,
        .lenBurst = 222,
        .srcStride = 333,
        .dstStride = 444,
        .dstMemType = MemType::GM,
        .srcMemType = MemType::UB,
        .padMode = PadMode::PAD_MODE2,
        .byteMode = ByteMode::BM_ENABLE
    };
    std::stringstream ss;
    ss << record;
    std::string format = ss.str();
    ASSERT_NE(format.find("0x12"), std::string::npos);
    ASSERT_NE(format.find("0x34"), std::string::npos);
    ASSERT_NE(format.find("78"), std::string::npos);
    ASSERT_NE(format.find("111"), std::string::npos);
    ASSERT_NE(format.find("222"), std::string::npos);
    ASSERT_NE(format.find("333"), std::string::npos);
    ASSERT_NE(format.find("444"), std::string::npos);
    ASSERT_NE(format.find("555"), std::string::npos);
    ASSERT_NE(format.find("GM"), std::string::npos);
    ASSERT_NE(format.find("UB"), std::string::npos);
    ASSERT_NE(format.find("MODE2"), std::string::npos);
    ASSERT_NE(format.find("ENABLE"), std::string::npos);
}

TEST(RecordFormat, format_dma_mov_conv_relu_record_expect_correct_result)
{
    DmaMovConvReluRecord record = {
        .dst = 0x12,
        .src = 0x34,
        .location = {56, 78, 0, 555},
        .nBurst = 111,
        .lenBurst = 222,
        .srcStride = 333,
        .dstStride = 444,
        .crMode = ConvRelu::CRMODE_NONE,
        .dstMemType = MemType::L0C,
        .srcMemType = MemType::UB,
        .srcDataType = DataType::DATA_B16,
        .dstDataType = DataType::DATA_B16
    };
    std::stringstream ss;
    ss << record;
    std::string format = ss.str();
    ASSERT_NE(format.find("0x12"), std::string::npos);
    ASSERT_NE(format.find("0x34"), std::string::npos);
    ASSERT_NE(format.find("78"), std::string::npos);
    ASSERT_NE(format.find("111"), std::string::npos);
    ASSERT_NE(format.find("222"), std::string::npos);
    ASSERT_NE(format.find("333"), std::string::npos);
    ASSERT_NE(format.find("444"), std::string::npos);
    ASSERT_NE(format.find("555"), std::string::npos);
    ASSERT_NE(format.find("CRMODE_NONE"), std::string::npos);
    ASSERT_NE(format.find("L0C"), std::string::npos);
    ASSERT_NE(format.find("UB"), std::string::npos);
    ASSERT_NE(format.find("B16"), std::string::npos);
    ASSERT_NE(format.find("B16"), std::string::npos);
}

TEST(RecordFormat, format_dma_mov_nd2nz_record_expect_correct_result)
{
    DmaMovNd2nzRecord record = {
        .dst = 0x12,
        .src = 0x34,
        .location = {56, 78, 0, 999},
        .ndNum = 111,
        .nValue = 222,
        .dValue = 333,
        .srcNdMatrixStride = 444,
        .srcDValue = 555,
        .dstNzC0Stride = 666,
        .dstNzNStride = 777,
        .dstNzMatrixStride = 888,
        .srcMemType = MemType::GM,
        .dstMemType = MemType::L1,
        .dataType = DataType::DATA_B16
    };
    std::stringstream ss;
    ss << record;
    std::string format = ss.str();
    ASSERT_NE(format.find("0x12"), std::string::npos);
    ASSERT_NE(format.find("0x34"), std::string::npos);
    ASSERT_NE(format.find("78"), std::string::npos);
    ASSERT_NE(format.find("111"), std::string::npos);
    ASSERT_NE(format.find("222"), std::string::npos);
    ASSERT_NE(format.find("333"), std::string::npos);
    ASSERT_NE(format.find("444"), std::string::npos);
    ASSERT_NE(format.find("555"), std::string::npos);
    ASSERT_NE(format.find("666"), std::string::npos);
    ASSERT_NE(format.find("777"), std::string::npos);
    ASSERT_NE(format.find("888"), std::string::npos);
    ASSERT_NE(format.find("999"), std::string::npos);
    ASSERT_NE(format.find("GM"), std::string::npos);
    ASSERT_NE(format.find("L1"), std::string::npos);
    ASSERT_NE(format.find("B16"), std::string::npos);
}

TEST(RecordFormat, format_mov_align_record_expect_correct_result)
{
    MovAlignRecord record = {
        .dst = 0x12,
        .src = 0x34,
        .location = {56, 78, 0, 555},
        .srcGap = 111,
        .dstGap = 222,
        .lenBurst = 333,
        .nBurst = 444,
        .dstMemType = MemType::GM,
        .srcMemType = MemType::UB,
        .dataType = DataType::DATA_B16,
        .leftPaddingNum = 66,
        .rightPaddingNum = 77
    };
    std::stringstream ss;
    ss << record;
    std::string format = ss.str();
    ASSERT_NE(format.find("0x12"), std::string::npos);
    ASSERT_NE(format.find("0x34"), std::string::npos);
    ASSERT_NE(format.find("78"), std::string::npos);
    ASSERT_NE(format.find("111"), std::string::npos);
    ASSERT_NE(format.find("222"), std::string::npos);
    ASSERT_NE(format.find("333"), std::string::npos);
    ASSERT_NE(format.find("444"), std::string::npos);
    ASSERT_NE(format.find("555"), std::string::npos);
    ASSERT_NE(format.find("GM"), std::string::npos);
    ASSERT_NE(format.find("UB"), std::string::npos);
    ASSERT_NE(format.find("B16"), std::string::npos);
    ASSERT_NE(format.find("66"), std::string::npos);
    ASSERT_NE(format.find("77"), std::string::npos);
}

TEST(RecordFormat, format_mov_bt_record_expect_correct_result)
{
    MovBtRecord record = {
            .dst = 0x12,
            .src = 0x34,
            .location = {56, 78, 0, 555},
            .srcGap = 111,
            .dstGap = 222,
            .lenBurst = 333,
            .nBurst = 444,
            .convControl = 1,
            .dstMemType = MemType::BT,
            .srcMemType = MemType::L1,
    };
    std::stringstream ss;
    ss << record;
    std::string format = ss.str();
    ASSERT_NE(format.find("0x12"), std::string::npos);
    ASSERT_NE(format.find("0x34"), std::string::npos);
    ASSERT_NE(format.find("78"), std::string::npos);
    ASSERT_NE(format.find("555"), std::string::npos);
    ASSERT_NE(format.find("111"), std::string::npos);
    ASSERT_NE(format.find("222"), std::string::npos);
    ASSERT_NE(format.find("333"), std::string::npos);
    ASSERT_NE(format.find("444"), std::string::npos);
    ASSERT_NE(format.find("1"), std::string::npos);
    ASSERT_NE(format.find("BT"), std::string::npos);
    ASSERT_NE(format.find("L1"), std::string::npos);
}

TEST(RecordFormat, format_mov_fp_record_expect_correct_result)
{
    MovFpRecord record = {
        .dst = 0x12,
        .src = 0x34,
        .location = {56, 78, 0, 555},
        .dstStride = 111,
        .dstNdStride = 777,
        .srcStride = 222,
        .nSize = 333,
        .mSize = 444,
        .ndNum = 666,
        .srcNdStride = 888,
        .srcNzC0Stride = 789,
        .quantPreBits = 999,
        .enUnitFlag = 0,
        .int8ChannelMerge = 0,
        .int4ChannelMerge = 0,
        .channelSplit = 0,
        .enNZ2ND = 1,
        .enNZ2DN = 0,
    };
    std::stringstream ss;
    ss << record;
    std::string format = ss.str();
    ASSERT_NE(format.find("dst:(addr:0x12;stride:111;ndstride:777)"), std::string::npos);
    ASSERT_NE(format.find("src:(addr:0x34;stride:222;ndstride:888;nzC0stride:789)"), std::string::npos);
    ASSERT_NE(format.find("loc--1-78"), std::string::npos);
    ASSERT_NE(format.find("nsize:333"), std::string::npos);
    ASSERT_NE(format.find("msize:444"), std::string::npos);
    ASSERT_NE(format.find("555"), std::string::npos);
    ASSERT_NE(format.find("ndnum:666"), std::string::npos);
    ASSERT_NE(format.find("uintflag:0"), std::string::npos);
    ASSERT_NE(format.find("quantPreBits:999"), std::string::npos);
    ASSERT_NE(format.find("int8channelmerge:0"), std::string::npos);
    ASSERT_NE(format.find("int4channelmerge:0"), std::string::npos);
    ASSERT_NE(format.find("channelsplit:0"), std::string::npos);
    ASSERT_NE(format.find("ennz2nd:1"), std::string::npos);
    ASSERT_NE(format.find("ennz2dn:0"), std::string::npos);
}

TEST(RecordFormat, format_unary_op_record_expect_correct_result)
{
    UnaryOpRecord record = {
        .dst = 0x12,
        .src = 0x34,
        .location = {56, 78, 0, 555},
        .vectorMask = {0x123, 0x456},
        .dstBlockSize = 447,
        .srcBlockSize = 367,
        .dstBlockStride = 111,
        .srcBlockStride = 222,
        .dstRepeatStride = 333,
        .srcRepeatStride = 444,
        .repeat = 66,
        .dstBlockNum = 77,
        .srcBlockNum = 88,
        .dstDataBits = 8,
        .srcDataBits = 8,
        .maskMode = MaskMode::MASK_COUNT
    };
    std::stringstream ss;
    ss << record;
    std::string format = ss.str();
    ASSERT_NE(format.find("0x12"), std::string::npos);
    ASSERT_NE(format.find("0x34"), std::string::npos);
    ASSERT_NE(format.find("78"), std::string::npos);
    ASSERT_NE(format.find("0x123"), std::string::npos);
    ASSERT_NE(format.find("0x456"), std::string::npos);
    ASSERT_NE(format.find("111"), std::string::npos);
    ASSERT_NE(format.find("222"), std::string::npos);
    ASSERT_NE(format.find("333"), std::string::npos);
    ASSERT_NE(format.find("444"), std::string::npos);
    ASSERT_NE(format.find("447"), std::string::npos);
    ASSERT_NE(format.find("367"), std::string::npos);
    ASSERT_NE(format.find("555"), std::string::npos);
    ASSERT_NE(format.find("66"), std::string::npos);
    ASSERT_NE(format.find("77"), std::string::npos);
    ASSERT_NE(format.find("88"), std::string::npos);
    ASSERT_NE(format.find("COUNT"), std::string::npos);
}

TEST(RecordFormat, format_binary_op_record_expect_correct_result)
{
    BinaryOpRecord record = {
        .dst = 0x12, .src0 = 0x34, .src1 = 0x45,
        .location = {56, 78, 0, 777},
        .vectorMask = {0x123, 0x456},
        .dstBlockStride = 111, .src0BlockStride = 222, .src1BlockStride = 333,
        .dstRepeatStride = 444, .src0RepeatStride = 555, .src1RepeatStride = 666,
        .dstBlockSize = 205, .src0BlockSize = 207, .src1BlockSize = 119,
        .repeat = 88,
        .dstBlockNum = 167, .src0BlockNum = 168, .src1BlockNum = 203,
        .dstDataBits = 8, .src0DataBits = 8, .src1DataBits = 8,
        .maskMode = MaskMode::MASK_COUNT
    };
    std::stringstream ss;
    ss << record;
    std::string format = ss.str();
    ASSERT_NE(format.find("0x12"), std::string::npos);
    ASSERT_NE(format.find("0x34"), std::string::npos);
    ASSERT_NE(format.find("0x45"), std::string::npos);
    ASSERT_NE(format.find("78"), std::string::npos);
    ASSERT_NE(format.find("0x123"), std::string::npos);
    ASSERT_NE(format.find("0x456"), std::string::npos);
    ASSERT_NE(format.find("111"), std::string::npos);
    ASSERT_NE(format.find("222"), std::string::npos);
    ASSERT_NE(format.find("333"), std::string::npos);
    ASSERT_NE(format.find("444"), std::string::npos);
    ASSERT_NE(format.find("555"), std::string::npos);
    ASSERT_NE(format.find("666"), std::string::npos);
    ASSERT_NE(format.find("777"), std::string::npos);
    ASSERT_NE(format.find("88"), std::string::npos);
    ASSERT_NE(format.find("167"), std::string::npos);
    ASSERT_NE(format.find("168"), std::string::npos);
    ASSERT_NE(format.find("203"), std::string::npos);
    ASSERT_NE(format.find("205"), std::string::npos);
    ASSERT_NE(format.find("207"), std::string::npos);
    ASSERT_NE(format.find("119"), std::string::npos);
    ASSERT_NE(format.find("COUNT"), std::string::npos);
}

TEST(RecordFormat, format_reduce_op_record_expect_correct_result)
{
    ReduceOpRecord record = {
        .dst = 0x12,
        .src = 0x34,
        .location = {56, 78, 0, 555},
        .vectorMask = {0x123, 0x456},
        .srcBlockStride = 111,
        .dstRepeatStride = 222,
        .srcRepeatStride = 333,
        .dstRepeatLength = 444,
        .dstAlignSize = 32,
        .repeat = 66,
        .dstBlockNum = 67,
        .srcBlockNum = 68,
        .dstBlockSize = 129,
        .srcBlockSize = 145,
        .dstDataBits = 8,
        .srcDataBits = 8,
        .dstDataBitsFactor = 1,
        .maskMode = MaskMode::MASK_COUNT
    };
    std::stringstream ss;
    ss << record;
    std::string format = ss.str();
    ASSERT_NE(format.find("0x12"), std::string::npos);
    ASSERT_NE(format.find("0x34"), std::string::npos);
    ASSERT_NE(format.find("78"), std::string::npos);
    ASSERT_NE(format.find("111"), std::string::npos);
    ASSERT_NE(format.find("222"), std::string::npos);
    ASSERT_NE(format.find("333"), std::string::npos);
    ASSERT_NE(format.find("444"), std::string::npos);
    ASSERT_NE(format.find("32"), std::string::npos);
    ASSERT_NE(format.find("555"), std::string::npos);
    ASSERT_NE(format.find("66"), std::string::npos);
    ASSERT_NE(format.find("67"), std::string::npos);
    ASSERT_NE(format.find("68"), std::string::npos);
    ASSERT_NE(format.find("129"), std::string::npos);
    ASSERT_NE(format.find("145"), std::string::npos);
    ASSERT_NE(format.find("COUNT"), std::string::npos);
}

TEST(RecordFormat, format_reducev2_record_expect_return_correct_result)
{
    ReduceV2Record record = {
        .dst = 0x12,
        .src0 = 0x34,
        .src1 = 0x56,
        .location = {123, 456, 0x789, 14},
        .vectorMask = {0x111, 0x222},
        .compareMask = {0x333, 0x444},
        .repeat = 12,
        .src0RepeatStride = 13,
        .src1RepeatStride = 15,
        .src0BlockStride = 16,
        .dataBytes = 17,
        .patternMode = 18,
        .maskMode = MaskMode::MASK_COUNT
    };
    std::stringstream ss;
    ss << record;
    auto const &format = ss.str();
    ASSERT_NE(format.find("14"), std::string::npos);
    ASSERT_NE(format.find("addr:0x12"), std::string::npos);
    ASSERT_NE(format.find("addr:0x34"), std::string::npos);
    ASSERT_NE(format.find("addr:0x56"), std::string::npos);
    ASSERT_NE(format.find("loc--1-456, 0x789"), std::string::npos);
    ASSERT_NE(format.find("vectormask:(0x222,0x111)"), std::string::npos);
    ASSERT_NE(format.find("cmpmask:(0x444,0x333)"), std::string::npos);
    ASSERT_NE(format.find("repeat:12"), std::string::npos);
    ASSERT_NE(format.find("rptstride:13"), std::string::npos);
    ASSERT_NE(format.find("rptstride:15"), std::string::npos);
    ASSERT_NE(format.find("blockstride:16"), std::string::npos);
    ASSERT_NE(format.find("databytes:17"), std::string::npos);
    ASSERT_NE(format.find("pattern:18"), std::string::npos);
    ASSERT_NE(format.find("COUNT"), std::string::npos);
}

TEST(RecordFormat, format_sync_record_expect_correct_result)
{
    SyncRecord record = {
        .location = {12, 34, 0, 111},
        .src = PipeType::PIPE_V,
        .dst = PipeType::PIPE_MTE2,
        .eventID = 3,
    };
    std::stringstream ss;
    ss << record;
    std::string format = ss.str();
    ASSERT_NE(format.find("34"), std::string::npos);
    ASSERT_NE(format.find("111"), std::string::npos);
    ASSERT_NE(format.find("V"), std::string::npos);
    ASSERT_NE(format.find("PIPE_MTE2"), std::string::npos);
    ASSERT_NE(format.find("3"), std::string::npos);
}

TEST(RecordFormat, format_buf_record_on_pipev_expect_correct_result)
{
    BufRecord record = {
        .location = {01, 56, 0x0001c, 25},
        .pipe = PipeType::PIPE_V,
        .bufId = 10,
        .mode = BufMode::BLOCK_MODE,
    };
    std::stringstream ss;
    ss << record;
    std::string format = ss.str();
    printf("%s", format.c_str());
    ASSERT_NE(format.find("56"), std::string::npos);
    ASSERT_NE(format.find("25"), std::string::npos);
    ASSERT_NE(format.find("V"), std::string::npos);
    ASSERT_NE(format.find("10"), std::string::npos);
    ASSERT_NE(format.find("0"), std::string::npos);
}

TEST(RecordFormat, format_buf_record_on_pipes_expect_correct_result)
{
    BufRecord record = {
        .location = {3, 26, 0x0001d, 10},
        .pipe = PipeType::PIPE_S,
        .bufId = 2,
        .mode = BufMode::NONBLOCK_MODE,
    };
    std::stringstream ss;
    ss << record;
    std::string format = ss.str();
    printf("%s", format.c_str());
    ASSERT_NE(format.find("10"), std::string::npos);
    ASSERT_NE(format.find("26"), std::string::npos);
    ASSERT_NE(format.find("S"), std::string::npos);
    ASSERT_NE(format.find("2"), std::string::npos);
    ASSERT_NE(format.find("1"), std::string::npos);
}

TEST(RecordFormat, format_hard_sync_record_expect_correct_result)
{
    HardSyncRecord record = {
        .location = {12, 34, 0, 111},
        .src = PipeType::PIPE_V,
        .dst = PipeType::PIPE_MTE2,
        .eventID = 3,
        .memory = MemType::UB,
        .v = 1
    };
    std::stringstream ss;
    ss << record;
    std::string format = ss.str();
    ASSERT_NE(format.find("34"), std::string::npos);
    ASSERT_NE(format.find("111"), std::string::npos);
    ASSERT_NE(format.find("PIPE_V"), std::string::npos);
    ASSERT_NE(format.find("PIPE_MTE2"), std::string::npos);
    ASSERT_NE(format.find("3"), std::string::npos);
    ASSERT_NE(format.find("UB"), std::string::npos);
    ASSERT_NE(format.find("1"), std::string::npos);
}

TEST(RecordFormat, format_pipe_barrier_record_expect_correct_result)
{
    PipeBarrierRecord record = {
        .location = {12, 34, 0, 111},
        .pipe = PipeType::PIPE_V,
    };
    std::stringstream ss;
    ss << record;
    std::string format = ss.str();
    ASSERT_NE(format.find("loc--1-34"), std::string::npos);
    ASSERT_NE(format.find("111"), std::string::npos);
    ASSERT_NE(format.find("pipe:PIPE_V"), std::string::npos);
}

TEST(RecordFormat, format_ffts_sync_record_expect_correct_result)
{
    FftsSyncRecord record = {
        .location = {12, 34, 0x56, 111},
        .dst = PipeType::PIPE_M,
        .mode = 222,
        .flagID = 1,
        .vecSubBlockDim = 2
    };
    std::stringstream ss;
    ss << record;
    std::string const &format = ss.str();
    ASSERT_NE(format.find("loc--1-34, 0x56"), std::string::npos);
    ASSERT_NE(format.find("111"), std::string::npos);
    ASSERT_NE(format.find("dst:PIPE_M"), std::string::npos);
    ASSERT_NE(format.find("mode:222"), std::string::npos);
    ASSERT_NE(format.find("flagid:1"), std::string::npos);
}

TEST(RecordFormat, format_kernel_record_expect_correct_result)
{
    auto getFormattedString = [](RecordType type) {
        auto record = KernelRecord{};
        record.recordType = type;
        std::stringstream ss;
        ss << record;
        return ss.str();
    };

    ASSERT_NE(getFormattedString(RecordType::LOAD).find("LOAD"), std::string::npos);
    ASSERT_NE(getFormattedString(RecordType::STORE).find("STORE"), std::string::npos);
    ASSERT_NE(getFormattedString(RecordType::DMA_MOV).find("DMA_MOV"), std::string::npos);
    ASSERT_NE(getFormattedString(RecordType::DMA_MOV_ND2NZ).find("DMA_MOV_ND2NZ"), std::string::npos);
    ASSERT_NE(getFormattedString(RecordType::MOV_ALIGN).find("MOV_ALIGN"), std::string::npos);

    ASSERT_NE(getFormattedString(RecordType::UNARY_OP).find("UNARY_OP"), std::string::npos);
    ASSERT_NE(getFormattedString(RecordType::BINARY_OP).find("BINARY_OP"), std::string::npos);
    ASSERT_NE(getFormattedString(RecordType::TERNARY_OP).find("TERNARY_OP"), std::string::npos);
    ASSERT_NE(getFormattedString(RecordType::REDUCE_OP).find("REDUCE_OP"), std::string::npos);

    ASSERT_NE(getFormattedString(RecordType::SET_FLAG).find("SET_FLAG"), std::string::npos);
    ASSERT_NE(getFormattedString(RecordType::WAIT_FLAG).find("WAIT_FLAG"), std::string::npos);
    ASSERT_NE(getFormattedString(RecordType::HSET_FLAG).find("HSET_FLAG"), std::string::npos);
    ASSERT_NE(getFormattedString(RecordType::HWAIT_FLAG).find("HWAIT_FLAG"), std::string::npos);
    ASSERT_NE(getFormattedString(RecordType::PIPE_BARRIER).find("PIPE_BARRIER"), std::string::npos);

    ASSERT_NE(getFormattedString(RecordType::FINISH).find("FINISH"), std::string::npos);
    ASSERT_NE(getFormattedString(RecordType::BLOCK_FINISH).find("BLOCK_FINISH"), std::string::npos);
}

TEST(RecordFormat, format_vector_dup_record_expect_correct_result)
{
    VecDupRecord record = {
        .dst = 0x5F5F,
        .location = {12, 34, 0, 111},
        .vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) },
        .dstBlockStride = 8,
        .dstRepeatStride = 32,
        .repeat = 10,
        .dataBits = 16,
        .maskMode = MaskMode::MASK_NORM
    };
    std::stringstream ss;
    ss << record;
    std::string format = ss.str();
    ASSERT_NE(format.find("dst:(addr:0x5f5f;blockstride:8;rptstride:32;bits:16)"), std::string::npos);
    ASSERT_NE(format.find("loc--1-34"), std::string::npos);
    ASSERT_NE(format.find("repeat:10"), std::string::npos);
    ASSERT_NE(format.find("111"), std::string::npos);
    ASSERT_NE(format.find("maskmode:NORM"), std::string::npos);
    ASSERT_NE(format.find("vectormask:(-1,-1)"), std::string::npos);
}

TEST(RecordFormat, format_load_2d_record_expect_correct_result)
{
    Load2DRecord record = {
        .location = {12, 34, 0, 111},
        .dst = 0x5F5F,
        .src = 0x5F5F5F,
        .baseIdx = 1,
        .srcStride = 8,
        .dstStride = 32,
        .blockSize = 512,
        .repeat = 10,
        .dstMemType = MemType::GM,
        .srcMemType = MemType::L1,
        .addrCalMode = AddrCalMode::INC,
    };
    std::stringstream ss;
    ss << record;
    std::string load2dFormat = ss.str();
    ASSERT_NE(load2dFormat.find("dst:(type:GM;addr:0x5f5f;stride:32)"), std::string::npos);
    ASSERT_NE(load2dFormat.find("src:(type:L1;addr:0x5f5f5f;stride:8)"), std::string::npos);
    ASSERT_NE(load2dFormat.find("loc--1-34"), std::string::npos);
    ASSERT_NE(load2dFormat.find("baseidx:1"), std::string::npos);
    ASSERT_NE(load2dFormat.find("repeat:10"), std::string::npos);
    ASSERT_NE(load2dFormat.find("blocksize:512"), std::string::npos);
    ASSERT_NE(load2dFormat.find("111"), std::string::npos);
    ASSERT_NE(load2dFormat.find(";addrcalmode:INC"), std::string::npos);
}

TEST(RecordFormat, format_load_2d_sparse_record_expect_correct_result)
{
    Load2DSparseRecord record = {
        .location = {12, 34, 0, 111},
        .dst = 0x5F5F,
        .src0 = 0x5F5F5F & 0xFFFFFFFF,
        .src1 = (0x5F5F5F >> 32U) & 0xFFFFFFFF,
        .startId = 1,
        .repeat = 10,
        .dstMemType = MemType::L0B,
        .srcMemType = MemType::L1,
    };
    std::stringstream ss;
    ss << record;
    std::string load2dSparseFormat = ss.str();
    ASSERT_NE(load2dSparseFormat.find("dst:(type:L0B;addr:0x5f5f)"), std::string::npos);
    ASSERT_NE(load2dSparseFormat.find("src0:(addr:0x5f5f5f)"), std::string::npos);
    ASSERT_NE(load2dSparseFormat.find("src1:(addr:0x0)"), std::string::npos);
    ASSERT_NE(load2dSparseFormat.find("loc--1-34"), std::string::npos);
    ASSERT_NE(load2dSparseFormat.find("repeat:10"), std::string::npos);
    ASSERT_NE(load2dSparseFormat.find("111"), std::string::npos);
    ASSERT_NE(load2dSparseFormat.find("src:(type:L1)"), std::string::npos);
}

TEST(RecordFormat, format_load_2d_transpose_record_expect_correct_result)
{
    Load2DTransposeRecord record = {
        .location = {12, 34, 0, 111},
        .dst = 0x5F5F,
        .src = 0x5F5F5F,
        .indexId = 1,
        .srcStride = 8,
        .dstStride = 32,
        .dstFracStride = 32,
        .repeat = 10,
        .addrMode = true,
        .dataType = DataType::DATA_B8,
        .dstMemType = MemType::L0B,
        .srcMemType = MemType::L1,
    };
    std::stringstream ss;
    ss << record;
    std::string load2dtransposeFormat = ss.str();
    ASSERT_NE(load2dtransposeFormat.find("dst:(type:L0B;addr:0x5f5f;stride:32;fracstride:32)"), std::string::npos);
    ASSERT_NE(load2dtransposeFormat.find("src:(type:L1;addr:0x5f5f5f;stride:8)"), std::string::npos);
    ASSERT_NE(load2dtransposeFormat.find("loc--1-34"), std::string::npos);
    ASSERT_NE(load2dtransposeFormat.find("indexid:1"), std::string::npos);
    ASSERT_NE(load2dtransposeFormat.find("repeat:10"), std::string::npos);
    ASSERT_NE(load2dtransposeFormat.find("type:B8"), std::string::npos);
    ASSERT_NE(load2dtransposeFormat.find("addrmode:1"), std::string::npos);
    ASSERT_NE(load2dtransposeFormat.find("111"), std::string::npos);
}

TEST(RecordFormat, format_decompress_header_record_expect_correct_result)
{
    DecompressHeaderRecord record = {
        .location = {12, 34, 0, 111},
        .src = 0x5F5F5F,
        .nBlock = 10,
        .srcMemType = MemType::GM,
    };
    std::stringstream ss;
    ss << record;
    std::string decompressheaderFormat = ss.str();
    ASSERT_NE(decompressheaderFormat.find("src:(type:GM;addr:0x5f5f5f)"), std::string::npos);
    ASSERT_NE(decompressheaderFormat.find("loc--1-34"), std::string::npos);
    ASSERT_NE(decompressheaderFormat.find("nblock:10"), std::string::npos);
    ASSERT_NE(decompressheaderFormat.find("111"), std::string::npos);
}

TEST(RecordFormat, format_dc_preload_record_expect_correct_result)
{
    DcPreloadRecord record = {
        .location = {12, 34, 0, 111},
        .addr = 0x5F5F5F,
        .offset = 32,
    };
    std::stringstream ss;
    ss << record;
    std::string dcPreloadFormat = ss.str();
    ASSERT_NE(dcPreloadFormat.find("addr:0x5f5f5f"), std::string::npos);
    ASSERT_NE(dcPreloadFormat.find("loc--1-34"), std::string::npos);
    ASSERT_NE(dcPreloadFormat.find("offset:32"), std::string::npos);
    ASSERT_NE(dcPreloadFormat.find("111"), std::string::npos);
}

TEST(RecordFormat, format_broadcast_record_expect_correct_result)
{
    BroadcastRecord record = {
        .location = {12, 34, 0, 111},
        .dst = 0x5F5F,
        .src = 0x5F5F5F,
        .nBurst = 1,
        .lenBurst = 1,
        .srcGap = 2,
        .dstGap = 2,
        .enableRepeat = true,
        .srcDataType = DataType::DATA_B16,
        .dstDataType = DataType::DATA_B16,
        .dstMemType = MemType::L0C,
        .srcMemType = MemType::UB,
    };
    std::stringstream ss;
    ss << record;
    std::string broadcastFormat = ss.str();
    ASSERT_NE(broadcastFormat.find("dst:(type:L0C;datatype:B16;addr:0x5f5f;gap:2)"), std::string::npos);
    ASSERT_NE(broadcastFormat.find("src:(type:UB;datatype:B16;addr:0x5f5f5f;gap:2)"), std::string::npos);
    ASSERT_NE(broadcastFormat.find("loc--1-34"), std::string::npos);
    ASSERT_NE(broadcastFormat.find("nBurst:1"), std::string::npos);
    ASSERT_NE(broadcastFormat.find("lenBurst:1"), std::string::npos);
    ASSERT_NE(broadcastFormat.find("enableRepeat:1"), std::string::npos);
    ASSERT_NE(broadcastFormat.find("111"), std::string::npos);
}

inline Load3DRecord GetLoad3DRecord(void)
{
    Load3DRecord record = {
        .location = {12, 34, 0, 111},
        .dst = 0x5F5F,
        .src = 0x5F5F5F,
        .fMapW = 16,
        .fMapH = 16,
        .fMapC = 16,
        .filterW = 3,
        .filterH = 3,
        .matrixRptStride = 1,
        .filterWStride = 1,
        .filterHStride = 1,
        .matrixKPos = 0,
        .matrixMPos = 0,
        .matrixKStep = 9,
        .matrixMStep = 16,
        .matrixRptTimes = 1,
        .dataType = DataType::DATA_B16,
        .dstMemType = MemType::L0A,
        .srcMemType = MemType::L1,
        .fMapTopPad = 1,
        .fMapBottomPad = 1,
        .fMapLeftPad = 1,
        .fMapRightPad = 1,
        .filterWDilation = 1,
        .filterHDilation = 1,
        .matrixMode = 0,
        .matrixRptMode = 0,
    };
    return record;
}

TEST(RecordFormat, format_load_3d_record_expect_correct_result)
{
    Load3DRecord record = GetLoad3DRecord();
    std::stringstream ss;
    ss << record;
    std::string load3dFormat = ss.str();
    ASSERT_NE(load3dFormat.find("dst:(type:L0A;addr:0x5f5f)"), std::string::npos);
    ASSERT_NE(load3dFormat.find("src:(type:L1;addr:0x5f5f5f)"), std::string::npos);
    ASSERT_NE(load3dFormat.find("loc--1-34"), std::string::npos);
    ASSERT_NE(load3dFormat.find("filter:(w:3;wstride:1;wdilation:1;h:3;hstride:1;hdilation:1)"), std::string::npos);
    ASSERT_NE(load3dFormat.find("fmap:(w:16;h:16;c:16;t:1;b:1;l:1;r:1)"), std::string::npos);
    ASSERT_NE(load3dFormat.find("matrix:(mode:0;rptstride:1;kpos:0;mpos:0;kstep:9;mstep:16;rpttimes:1;rptmode:0)"),
                                std::string::npos);
    ASSERT_NE(load3dFormat.find("datatype:B16"), std::string::npos);
    ASSERT_NE(load3dFormat.find("111"), std::string::npos);
}

TEST(RecordFormat, format_atomic_mode_record_expect_return_correct_result)
{
    AtomicModeRecord record = {
        .location = {12, 34, 0x56, 78},
        .mode = AtomicMode::F32
    };
    std::stringstream ss;
    ss << record;
    auto const &format = ss.str();
    ASSERT_NE(format.find("78"), std::string::npos);
    ASSERT_NE(format.find("loc--1-34, 0x56"), std::string::npos);
    ASSERT_NE(format.find("mode:F32"), std::string::npos);
}

inline LoadB2Record GetLoadB2Record(void)
{
    LoadB2Record record = {
        .location = {12, 34, 0, 111},
        .dst = 0x5F5F,
        .src = 0x5F5F5F,
        .repeat = 3,
        .dataType = DataType::DATA_B8,
        .dstMemType = MemType::L0B,
        .srcMemType = MemType::L1,
    };
    return record;
}
inline LoadAWinogradRecord GetLoadAWinogradRecord(void)
{
    LoadAWinogradRecord record = {
        .location = {12, 34, 0, 111},
        .dst = 0x5F5F,
        .src = 0x5F5F5F,

        .fmSizeW = 16,
        .fmSizeH = 16,
        .fmSizeCh = 16,
        .extStepK = 0,
        .extStepM = 0,
        .dstStartPointK = 9,
        .dstStartPointM = 16,
        .innerDstGap = 1,
        .dataType = DataType::DATA_B16,
        .dstMemType = MemType::L0A,
        .srcMemType = MemType::L1,
    };
    return record;
}

TEST(RecordFormat, format_load_A_winograd_record_expect_correct_result)
{
    auto record = GetLoadAWinogradRecord();
    std::stringstream ss;
    ss << record;
    std::string load3dFormat = ss.str();
    ASSERT_NE(load3dFormat.find("dst:(type:L0A;addr:0x5f5f;startpointk:9;startpointmg:16)"), std::string::npos);
    ASSERT_NE(load3dFormat.find("src:(type:L1;addr:0x5f5f5f)"), std::string::npos);
    ASSERT_NE(load3dFormat.find("111"), std::string::npos);
    ASSERT_NE(load3dFormat.find("loc--1-34"), std::string::npos);
    ASSERT_NE(load3dFormat.find("fmsize:(w:16;h:16;ch:16)"), std::string::npos);
    ASSERT_NE(load3dFormat.find("innerdstgap:1"), std::string::npos);
    ASSERT_NE(load3dFormat.find("type:B16"), std::string::npos);
    ASSERT_NE(load3dFormat.find("extstep:(k:0;m:0)"), std::string::npos);
}

inline LoadBWinogradRecord GetLoadBWinogradRecord(void)
{
    LoadBWinogradRecord record = {
        .location = {12, 34, 0, 111},
        .dst = 0x5F5F,
        .src = 0x5F5F5F,
        .srcRptStride = 1,
        .dstRptStride = 2,
        .innerDstStride = 1,
        .repeat = 3,
        .dstMemType = MemType::L0B,
        .srcMemType = MemType::L1,
    };
    return record;
}

TEST(RecordFormat, format_load_B2_record_expect_correct_result)
{
    auto record = GetLoadB2Record();
    std::stringstream ss;
    ss << record;
    std::string loadB2Format = ss.str();
    ASSERT_NE(loadB2Format.find("dst:(type:L0B;addr:0x5f5f)"), std::string::npos);
    ASSERT_NE(loadB2Format.find("src:(type:L1;addr:0x5f5f5f)"), std::string::npos);
    ASSERT_NE(loadB2Format.find("111"), std::string::npos);
    ASSERT_NE(loadB2Format.find("loc--1-34"), std::string::npos);
    ASSERT_NE(loadB2Format.find("repeat:3"), std::string::npos);
    ASSERT_NE(loadB2Format.find("type:B8"), std::string::npos);
}

TEST(RecordFormat, format_load_B_winograd_record_expect_correct_result)
{
    auto record = GetLoadBWinogradRecord();
    std::stringstream ss;
    ss << record;
    std::string load3dFormat = ss.str();
    ASSERT_NE(load3dFormat.find("st:(type:L0B;addr:0x5f5f;rptstride:2)"), std::string::npos);
    ASSERT_NE(load3dFormat.find("src:(type:L1;addr:0x5f5f5f;rptstride:1)"), std::string::npos);
    ASSERT_NE(load3dFormat.find("111"), std::string::npos);
    ASSERT_NE(load3dFormat.find("loc--1-34"), std::string::npos);
    ASSERT_NE(load3dFormat.find("innerdststride:1"), std::string::npos);
    ASSERT_NE(load3dFormat.find("repeat:3"), std::string::npos);
}

TEST(RecordFormat, format_set_2d_record_expect_correct_result)
{
    Set2DRecord record = {
            .location = {12, 34, 0, 111},
            .dst = 0x5F5F,
            .dstBlockSize = 512,
            .repeat = 1,
            .dstBlockNum = 2,
            .repeatGap = 3,
            .dstMemType = MemType::L0A
    };
    std::stringstream ss;
    ss << record;
    std::string set2dFormat = ss.str();
    ASSERT_NE(set2dFormat.find("dst:(type:L0A;addr:0x5f5f;blocknum:2;blocksize:512)"), std::string::npos);
    ASSERT_NE(set2dFormat.find("loc--1-34"), std::string::npos);
    ASSERT_NE(set2dFormat.find("repeat:1"), std::string::npos);
    ASSERT_NE(set2dFormat.find("111"), std::string::npos);
    ASSERT_NE(set2dFormat.find("rptgap:3"), std::string::npos);
}

inline LoadImageRecord GetLoadImageRecord(void)
{
    LoadImageRecord record = {
        .location = {12, 34, 0, 111},
        .dst = 0x5F5F,
        .horSize = 80,
        .verSize = 80,
        .horStartP = 5,
        .verStartP = 5,
        .sHorRes = 5,
        .lPadSize = 10,
        .rPadSize = 10,
        .dataType = DataType::DATA_B8,
        .dstMemType = MemType::L1,
        .topPadSize = 1,
        .botPadSize = 1
    };
    return record;
}

TEST(RecordFormat, format_load_image_record_expect_correct_result)
{
    LoadImageRecord record = GetLoadImageRecord();
    std::stringstream ss;
    ss << record;

    std::string loadImageFormat = ss.str();
    ASSERT_NE(loadImageFormat.find("dst:(type:L1;addr:0x5f5f)"), std::string::npos);
    ASSERT_NE(loadImageFormat.find("loc--1-34"), std::string::npos);
    ASSERT_NE(loadImageFormat.find("111"), std::string::npos);
    ASSERT_NE(loadImageFormat.find("hor:(size:80;startp:5)"), std::string::npos);
    ASSERT_NE(loadImageFormat.find("ver:(size:80;startp:5)"), std::string::npos);
    ASSERT_NE(loadImageFormat.find("shorres:5"), std::string::npos);
    ASSERT_NE(loadImageFormat.find("lpadsize:10"), std::string::npos);
    ASSERT_NE(loadImageFormat.find("rpadsize:10"), std::string::npos);;
    ASSERT_NE(loadImageFormat.find("type:B8"), std::string::npos);
    ASSERT_NE(loadImageFormat.find("toppadsize:1"), std::string::npos);
    ASSERT_NE(loadImageFormat.find("botpadsize:1"), std::string::npos);
}

inline LoadSmaskRecord GetLoadSmaskRecord(MemType srcMemType)
{
    LoadSmaskRecord record = {
        .location = {12, 34, 0, 111},
        .src = 0x78F00,
        .dst = 0x3F0,
        .smaskSize = 32,
        .srcMemType = srcMemType
    };
    return record;
}

TEST(RecordFormat, format_load_smask_record_expect_correct_result)
{
    LoadSmaskRecord record_gm = GetLoadSmaskRecord(MemType::GM);
    std::stringstream ss;
    ss << record_gm;

    std::string loadSmaskFormatGM = ss.str();
    ASSERT_NE(loadSmaskFormatGM.find("src:(type:GM;addr:0x78f00)"), std::string::npos);
    ASSERT_NE(loadSmaskFormatGM.find("dst:(addr:0x3f0)"), std::string::npos);
    ASSERT_NE(loadSmaskFormatGM.find("111"), std::string::npos);
    ASSERT_NE(loadSmaskFormatGM.find("smaskSize:32"), std::string::npos);
    ASSERT_NE(loadSmaskFormatGM.find("loc--1-34"), std::string::npos);

    LoadSmaskRecord record_ub = GetLoadSmaskRecord(MemType::UB);
    ss.clear();
    ss << record_ub;
    std::string loadSmaskFormatUB = ss.str();
    ASSERT_NE(loadSmaskFormatUB.find("src:(type:UB;addr:0x78f00)"), std::string::npos);
    ASSERT_NE(loadSmaskFormatUB.find("dst:(addr:0x3f0)"), std::string::npos);
    ASSERT_NE(loadSmaskFormatUB.find("111"), std::string::npos);
    ASSERT_NE(loadSmaskFormatUB.find("smaskSize:32"), std::string::npos);
    ASSERT_NE(loadSmaskFormatUB.find("loc--1-34"), std::string::npos);
}

TEST(RecordFormat, format_matrix_mul_op_record_expect_correct_result)
{
    MarixMulOpRecord record = {
            .dst = 0x5F5F,
            .src0 = 0x6f,
            .src1 = 0x7f,
            .m = 10,
            .k = 13,
            .n = 14,
            .location = {12, 34, 0, 111},
            .src0RepeatStride = 2,
            .dstBlockSize = 16,
            .src0BlockSize = 32,
            .src1BlockSize = 32,
            .src0Repeat = 5,
            .dstBlockNum = 6,
            .src0BlockNum = 7,
            .src1BlockNum = 8,
            .dstAlignSize = 64,
            .src0AlignSize = 32,
            .src1AlignSize = 16,
            .cmatrixSource = true,
            .cmatrixInitVal = false,
            .enUnitFlag = true,
        };
    std::stringstream ss;
    ss << record;

    std::string marixMulOpFormat = ss.str();
    ASSERT_NE(marixMulOpFormat.find("dst:(addr:0x5f5f;blocksize:16;blocknum:6;align:64)"), std::string::npos);
    ASSERT_NE(marixMulOpFormat.find("src0:(addr:0x6f;repeat:5;blocksize:32;blocknum:7;rptstride:2;align:32)"),
              std::string::npos);
    ASSERT_NE(marixMulOpFormat.find("src1:(addr:0x7f;blocksize:32;blocknum:8;align:16)"), std::string::npos);
    ASSERT_NE(marixMulOpFormat.find("m:10"), std::string::npos);
    ASSERT_NE(marixMulOpFormat.find("k:13"), std::string::npos);
    ASSERT_NE(marixMulOpFormat.find("n:14"), std::string::npos);
    ASSERT_NE(marixMulOpFormat.find("111"), std::string::npos);
    ASSERT_NE(marixMulOpFormat.find("bias:(initval:0;source:1;uintflag:1)"), std::string::npos);
}

inline VecRegPropCoordOpRecord GetVecRegPropCoordOpRecord()
{
    VecRegPropCoordOpRecord record{};
    record.location.blockId = 1;
    record.dataType = DataType::DATA_B16;
    record.dst = 0x000100;
    record.src = 0x000100;
    record.location = {12, 34};
    record.regionRange = 1;
    record.repeat = 1;
    return record;
}

TEST(RecordFormat, format_vector_region_proprosal_coordinate_expect_correct_result)
{
    auto record = GetVecRegPropCoordOpRecord();
    std::stringstream ss;
    ss << record;
    std::string vRegPropCorFormat = ss.str();
    ASSERT_NE(vRegPropCorFormat.find("1"), std::string::npos);
    ASSERT_NE(vRegPropCorFormat.find("loc--1-34"), std::string::npos);
    ASSERT_NE(vRegPropCorFormat.find("dst:(addr:0x100)"), std::string::npos);
    ASSERT_NE(vRegPropCorFormat.find("src:(addr:0x100)"), std::string::npos);
    ASSERT_NE(vRegPropCorFormat.find("regionrange:1"), std::string::npos);
    ASSERT_NE(vRegPropCorFormat.find("repeat:1"), std::string::npos);
    ASSERT_NE(vRegPropCorFormat.find("type:B16"), std::string::npos);
}

TEST(RecordFormat, format_soft_sync_record_expect_return_correct_result)
{
    SoftSyncRecord record = {
        .location = {12, 34, 0x56, 78},
        .waitCoreID = 11,
        .usedCores = 22,
        .eventID = 33,
        .isAIVOnly = true
    };
    std::stringstream ss;
    ss << record;
    auto const &format = ss.str();
    ASSERT_NE(format.find("78"), std::string::npos);
    ASSERT_NE(format.find("loc--1-34, 0x56"), std::string::npos);
    ASSERT_NE(format.find("waitcoreid:11"), std::string::npos);
    ASSERT_NE(format.find("usedcores:22"), std::string::npos);
    ASSERT_NE(format.find("eventid:33"), std::string::npos);
    ASSERT_NE(format.find("isaivonly:1"), std::string::npos);
}

TEST(RecordFormat, format_cmp_mask_record_expect_return_correct_result)
{
    CmpMaskRecord record = {
        .location = {12, 34, 0x56, 78},
        .addr = 0x11,
        .size = 22,
        .accessType = AccessType::READ,
    };
    std::stringstream ss;
    ss << record;
    auto const &format = ss.str();
    ASSERT_NE(format.find("78"), std::string::npos);
    ASSERT_NE(format.find("loc--1-34, 0x56"), std::string::npos);
    ASSERT_NE(format.find("addr:0x11"), std::string::npos);
    ASSERT_NE(format.find("size:22"), std::string::npos);
    ASSERT_NE(format.find("accessType:READ"), std::string::npos);
}

TEST(RecordFormat, format_mstx_cross_record_expect_correct_result)
{
    MstxRecord record = {
        .interfaceType = InterfaceType::MSTX_SET_CROSS_SYNC,
        .bufferLens = 18,
        .location = {12, 34, 0xef, 111},
        .error = false,
    };

    MstxCrossRecord crossRecord = {
        .addr = 0x300,
        .flagId = 2,
        .pipe = PipeType::PIPE_V,
        .isMore = false,
        .isMerge = true,
    };
    record.interface.mstxCrossRecord = crossRecord;

    std::stringstream ss;
    ss << record;
    std::string mstxCrossFormat = ss.str();
    ASSERT_NE(mstxCrossFormat.find("interfaceType:SET_CROSS"), std::string::npos);
    ASSERT_NE(mstxCrossFormat.find("bufferlens:18"), std::string::npos);
    ASSERT_NE(mstxCrossFormat.find("loc--1-34"), std::string::npos);
    ASSERT_NE(mstxCrossFormat.find("0xef"), std::string::npos);
    ASSERT_NE(mstxCrossFormat.find("111"), std::string::npos);
    ASSERT_NE(mstxCrossFormat.find("error:0"), std::string::npos);
    ASSERT_NE(mstxCrossFormat.find("addr:0x300"), std::string::npos);
    ASSERT_NE(mstxCrossFormat.find("flagid:2"), std::string::npos);
    ASSERT_NE(mstxCrossFormat.find("pipe:PIPE_V"), std::string::npos);
    ASSERT_NE(mstxCrossFormat.find("ismore:0"), std::string::npos);
    ASSERT_NE(mstxCrossFormat.find("ismerge:1"), std::string::npos);
}

TEST(RecordFormat, format_mstx_hccl_record_expect_correct_result)
{
    MstxRecord record = {
        .interfaceType = InterfaceType::MSTX_HCCL,
        .bufferLens = 18,
        .location = {12, 34, 0xef, 111},
        .error = false,
    };

    MstxHcclRecord hcclRecord = {
        .src = 0x12c041200000,
        .dst = 0x12c042600000,
        .srcCount = 4587520,
        .dstCount = 2293760,
        .srcStride = 1,
        .dstStride = 2621440,
        .srcRepeatStride = 1,
        .dstRepeatStride = 1,
        .srcDataTypeSize = 2,
        .dstDataTypeSize = 2,
        .repeat = 1,
        .rankDim = 8,
        .flagId = 1,
    };
    record.interface.mstxHcclRecord = hcclRecord;

    std::stringstream ss;
    ss << record;
    std::string mstxHcclFormat = ss.str();
    ASSERT_NE(mstxHcclFormat.find("interfaceType:HCCL"), std::string::npos);
    ASSERT_NE(mstxHcclFormat.find("bufferlens:18"), std::string::npos);
    ASSERT_NE(mstxHcclFormat.find("loc--1-34"), std::string::npos);
    ASSERT_NE(mstxHcclFormat.find("0xef"), std::string::npos);
    ASSERT_NE(mstxHcclFormat.find("111"), std::string::npos);
    ASSERT_NE(mstxHcclFormat.find("error:0"), std::string::npos);
    ASSERT_NE(mstxHcclFormat.find("src:0x12c041200000"), std::string::npos);
    ASSERT_NE(mstxHcclFormat.find("dst:0x12c042600000"), std::string::npos);
    ASSERT_NE(mstxHcclFormat.find("srcCount:4587520"), std::string::npos);
    ASSERT_NE(mstxHcclFormat.find("dstCount:2293760"), std::string::npos);
    ASSERT_NE(mstxHcclFormat.find("srcStride:1"), std::string::npos);
    ASSERT_NE(mstxHcclFormat.find("dstStride:2621440"), std::string::npos);
    ASSERT_NE(mstxHcclFormat.find("srcRepeatStride:1"), std::string::npos);
    ASSERT_NE(mstxHcclFormat.find("dstRepeatStride:1"), std::string::npos);
    ASSERT_NE(mstxHcclFormat.find("repeat:1"), std::string::npos);
    ASSERT_NE(mstxHcclFormat.find("rankDim:8"), std::string::npos);
}

TEST(RecordFormat, format_mstx_cross_core_barrier_record_expect_correct_result)
{
    MstxCrossCoreBarrier record;
    record.usedCoreNum = 10;
    record.usedCoreId = nullptr;
    record.isAIVOnly = true;
    record.pipeBarrierAll = true;

    std::stringstream ss;
    ss << record;
    std::string result = ss.str();
    ASSERT_NE(result.find("usedCoreNum:10"), std::string::npos);
    ASSERT_NE(result.find("usedCoreId:0"), std::string::npos);
    ASSERT_NE(result.find("isAIVOnly:true"), std::string::npos);
    ASSERT_NE(result.find("pipeBarrierAll:true"), std::string::npos);
}

TEST(RecordFormat, format_mstx_cross_core_set_flag_record_expect_correct_result)
{
    MstxCrossCoreSetFlag record;
    record.eventId = 10;
    record.peerCoreId = -1;
    record.pipeBarrierAll = true;

    std::stringstream ss;
    ss << record;
    std::string result = ss.str();
    ASSERT_NE(result.find("eventId:10"), std::string::npos);
    ASSERT_NE(result.find("peerCoreId:-1"), std::string::npos);
    ASSERT_NE(result.find("pipeBarrierAll:true"), std::string::npos);
}

TEST(RecordFormat, format_mstx_tensor_desc_expect_correct_result)
{
    MstxTensorDesc tensorDesc = {
        .space = AddressSpace::UB,
        .addr = 0x200,
        .size = 100,
        .dataBits = 8,
    };

    std::stringstream ss;
    ss << tensorDesc;
    std::string result = ss.str();
    ASSERT_NE(result.find("addr:0x200"), std::string::npos);
    ASSERT_NE(result.find("size:100"), std::string::npos);
    ASSERT_NE(result.find("space:UB"), std::string::npos);
    ASSERT_NE(result.find("dataBits:8"), std::string::npos);
}

TEST(RecordFormat, format_mstx_vec_unary_desc_expect_correct_result)
{
    MstxTensorDesc tensorDesc = {
        .space = AddressSpace::UB,
        .addr = 0x200,
        .size = 100,
        .dataBits = 8,
    };
    VectorMask mask = {1, 2};
    MstxVecUnaryDesc record{};
    record.dst = tensorDesc;
    record.src = tensorDesc;
    record.wrapper.maskMode = MaskMode::MASK_NORM;
    record.wrapper.mask = mask;
    record.wrapper.reserveBufSize = 0;
    record.wrapper.useMask = true;
    record.blockNum = 8;
    record.repeatTimes = 1;
    record.dstBlockStride = 1;
    record.srcBlockStride = 1;
    record.dstRepeatStride = 1;
    record.srcRepeatStride = 1;

    std::stringstream ss;
    ss << record;
    std::string result = ss.str();
    ASSERT_NE(result.find("dst:" + ToString(tensorDesc)), std::string::npos);
    ASSERT_NE(result.find("src:" + ToString(tensorDesc)), std::string::npos);
    ASSERT_NE(result.find("maskMode:" + ToString(MaskMode::MASK_NORM)), std::string::npos);
    ASSERT_NE(result.find("mask:" + ToString(mask)), std::string::npos);
    ASSERT_NE(result.find("reserveBufSize:0"), std::string::npos);
    ASSERT_NE(result.find("useMask:true"), std::string::npos);
    ASSERT_NE(result.find("blockNum:8"), std::string::npos);
    ASSERT_NE(result.find("repeatTimes:1"), std::string::npos);
    ASSERT_NE(result.find("dstBlockStride:1"), std::string::npos);
    ASSERT_NE(result.find("srcBlockStride:1"), std::string::npos);
    ASSERT_NE(result.find("dstRepeatStride:1"), std::string::npos);
    ASSERT_NE(result.find("srcRepeatStride:1"), std::string::npos);
    ASSERT_NE(result.find("name:<unknown>"), std::string::npos);
}

TEST(RecordFormat, format_mstx_vec_binary_desc_expect_correct_result)
{
    MstxTensorDesc tensorDesc = {
        .space = AddressSpace::UB,
        .addr = 0x200,
        .size = 100,
        .dataBits = 8,
    };
    VectorMask mask = {1, 2};
    MstxVecBinaryDesc record{};
    record.dst = tensorDesc;
    record.src0 = tensorDesc;
    record.src1 = tensorDesc;
    record.wrapper.maskMode = MaskMode::MASK_NORM;
    record.wrapper.mask = mask;
    record.wrapper.reserveBufSize = 0;
    record.wrapper.useMask = true;
    record.blockNum = 8;
    record.repeatTimes = 1;
    record.dstBlockStride = 1;
    record.src0BlockStride = 1;
    record.src1BlockStride = 1;
    record.dstRepeatStride = 1;
    record.src0RepeatStride = 1;
    record.src1RepeatStride = 1;

    std::stringstream ss;
    ss << record;
    std::string result = ss.str();
    ASSERT_NE(result.find("dst:" + ToString(tensorDesc)), std::string::npos);
    ASSERT_NE(result.find("src0:" + ToString(tensorDesc)), std::string::npos);
    ASSERT_NE(result.find("src1:" + ToString(tensorDesc)), std::string::npos);
    ASSERT_NE(result.find("maskMode:" + ToString(MaskMode::MASK_NORM)), std::string::npos);
    ASSERT_NE(result.find("mask:" + ToString(mask)), std::string::npos);
    ASSERT_NE(result.find("reserveBufSize:0"), std::string::npos);
    ASSERT_NE(result.find("useMask:true"), std::string::npos);
    ASSERT_NE(result.find("blockNum:8"), std::string::npos);
    ASSERT_NE(result.find("repeatTimes:1"), std::string::npos);
    ASSERT_NE(result.find("dstBlockStride:1"), std::string::npos);
    ASSERT_NE(result.find("src0BlockStride:1"), std::string::npos);
    ASSERT_NE(result.find("src1BlockStride:1"), std::string::npos);
    ASSERT_NE(result.find("dstRepeatStride:1"), std::string::npos);
    ASSERT_NE(result.find("src0RepeatStride:1"), std::string::npos);
    ASSERT_NE(result.find("src1RepeatStride:1"), std::string::npos);
    ASSERT_NE(result.find("name:<unknown>"), std::string::npos);
}

TEST(RecordFormat, format_scattervnchwconv_record_expect_correct_result)
{
    ScatterVnchwconvRecord record = {
            .location = {12, 34, 0, 111},
            .dst0 = {0x5F00, 0x5F00},
            .dst1 = {0x5F00, 0x5F00},
            .src0 = {0x5F5F5F, 0x5F5F5F},
            .src1 = {0x5F5F5F, 0x5F5F5F},
            .dstStride = 1,
            .srcStride = 2,
            .repeat = 1,
            .dstHighHalf = false,
            .srcHighHalf = false,
            .dataType = DataType::DATA_B16,
    };
    std::stringstream ss;
    ss << record;
    std::string broadcastFormat = ss.str();
    ASSERT_NE(broadcastFormat.find("dst:(va0:(0x5f00, 0x5f00);va1:(0x5f00, 0x5f00);stride:1;highHalf:0);"), std::string::npos);
    ASSERT_NE(broadcastFormat.find("src:(va0:(0x5f5f5f, 0x5f5f5f);va1:(0x5f5f5f, 0x5f5f5f);stride:2;highHalf:0);"), std::string::npos);
    ASSERT_NE(broadcastFormat.find("loc--1-34"), std::string::npos);
    ASSERT_NE(broadcastFormat.find("repeat:1"), std::string::npos);
    ASSERT_NE(broadcastFormat.find("111"), std::string::npos);
}

TEST(RecordFormat, format_mstx_signal_set_record_expect_correct_result)
{
    MstxSignalSet record = {
        .addr = 0x1234,
        .value = 100
    };
    std::stringstream ss;
    ss << record;
    std::string const &str = ss.str();
    ASSERT_NE(str.find("addr:0x1234"), std::string::npos);
    ASSERT_NE(str.find("value:100"), std::string::npos);
}

TEST(RecordFormat, format_mstx_signal_wait_record_expect_correct_result)
{
    MstxSignalWait record = {
        .addr = 0x1234,
        .cmpValue = 100,
        .cmpOp = CompareOp::EQ
    };
    std::stringstream ss;
    ss << record;
    std::string const &str = ss.str();
    ASSERT_NE(str.find("addr:0x1234"), std::string::npos);
    ASSERT_NE(str.find("cmpValue:100"), std::string::npos);
    ASSERT_NE(str.find("cmpOp:EQ"), std::string::npos);
}

}
