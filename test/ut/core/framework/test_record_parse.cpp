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
#include <vector>
#include <string>

#define private public
#include "constant.h"
#include "address_sanitizer/align_checker.h"
#include "core/framework/record_parse.h"
#undef private
#include "sanitizer_report.h"

namespace Sanitizer {
inline bool operator==(MemOpInfo const &lhs, MemOpInfo const &rhs)
{
    return lhs.memType == rhs.memType &&
        lhs.opType == rhs.opType &&
        lhs.maskMode == rhs.maskMode &&
        lhs.dataBits == rhs.dataBits &&
        lhs.addr == rhs.addr &&
        lhs.blockNum == rhs.blockNum &&
        lhs.blockSize == rhs.blockSize &&
        lhs.blockStride == rhs.blockStride &&
        lhs.repeatTimes == rhs.repeatTimes &&
        lhs.repeatStride == rhs.repeatStride &&
        lhs.alignSize == rhs.alignSize;
}

}  // namespace Sanitizer

namespace {
using namespace Sanitizer;

static constexpr uint64_t CORE_ID = 7;
static constexpr uint64_t REPEAT = 100;
static constexpr uint64_t SRC0_BLOCK_SIZE = 16;
static constexpr uint64_t SRC1_BLOCK_SIZE = 8;
static constexpr uint64_t DST_BLOCK_SIZE = 32;
static constexpr uint64_t SRC0_BLOCK_NUM = 4;
static constexpr uint64_t SRC1_BLOCK_NUM = 2;
static constexpr uint64_t DST_BLOCK_NUM = 8;
static constexpr uint64_t NORMAL_REPEAT_STRIDE = 8;
static constexpr VectorMask DEFAULT_VECTOR_MASK = VectorMask{static_cast<uint64_t>(-1),
                                                             static_cast<uint64_t>(-1)};
class TestRecordParse : public testing::Test {
public:
    TestRecordParse() {}
    static void SetUpTestCase()
    {
    }
    void RegisterNotifyFunc(LogLv expectLv, std::vector<DetectionInfo> &detectInfo)
    {
        auto func = [expectLv, &detectInfo, this](const LogLv &lv, SanitizerBase::MSG_GEN &&gen) {
            if (lv >= expectLv) {
                detectInfo.push_back(gen());
            }
        };
        AlignChecker::Instance().RegisterNotifyFunc(func);
    }
    void SetUp() override
    {
        RegisterNotifyFunc(expectLv, detectInfo);
        detectInfo.clear();
    }

    void TearDown() override
    {
        AlignChecker::Instance().RegisterNotifyFunc(nullptr);
    }

protected:
    std::vector<DetectionInfo> detectInfo{};
    LogLv expectLv{LogLv::INFO};
};

TEST_F(TestRecordParse, parse_load_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::LOAD;
    record.payload.loadStoreRecord.location.blockId = 7;
    record.payload.loadStoreRecord.addr = 0x12;
    record.payload.loadStoreRecord.size = 100;
    record.payload.loadStoreRecord.alignSize = 100;
    record.payload.loadStoreRecord.space = AddressSpace::GM;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_S_CAL);
    MemOpInfo memOpInfo = {MemType::GM, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x12, 1, 100, 1, 1, 1, 100};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_load_private_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::LOAD;
    record.payload.loadStoreRecord.location.blockId = 7;
    record.payload.loadStoreRecord.addr = 0x12;
    record.payload.loadStoreRecord.size = 100;
    record.payload.loadStoreRecord.alignSize = 100;
    record.payload.loadStoreRecord.space = AddressSpace::PRIVATE;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_S_CAL);
    MemOpInfo memOpInfo = {MemType::PRIVATE, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x12, 1, 100, 1, 1, 1, 100};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_store_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::STORE;
    record.payload.loadStoreRecord.location.blockId = 7;
    record.payload.loadStoreRecord.addr = 0x12;
    record.payload.loadStoreRecord.size = 100;
    record.payload.loadStoreRecord.alignSize = 100;
    record.payload.loadStoreRecord.space = AddressSpace::GM;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_S_CAL);
    MemOpInfo memOpInfo = {MemType::GM, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x12, 1, 100, 1, 1, 1, 100};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_dma_mov_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::DMA_MOV;
    record.payload.dmaMovRecord.dst = 0xaa;
    record.payload.dmaMovRecord.src = 0x55;
    record.payload.dmaMovRecord.nBurst = 100;
    record.payload.dmaMovRecord.lenBurst = 8;
    record.payload.dmaMovRecord.srcStride = 8;
    record.payload.dmaMovRecord.dstStride = 8;
    record.payload.dmaMovRecord.location.blockId = 7;
    record.payload.dmaMovRecord.srcMemType = MemType::UB;
    record.payload.dmaMovRecord.dstMemType = MemType::GM;
    record.payload.dmaMovRecord.padMode = PadMode::PAD_NONE;
    record.payload.dmaMovRecord.byteMode = ByteMode::BM_DISABLE;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);

    MemOpInfo memOpInfo;
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE3);
    memOpInfo = {MemType::UB, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x55, 8, 32, 1, 100, 8 + 8, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    ASSERT_EQ(events[1].loc.coreId, 7);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_MTE3);
    memOpInfo = {MemType::GM, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0xaa, 8, 32, 1, 100, 8 + 8, 1};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_dma_mov_conv_relu_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::DMA_MOV_CONV_RELU;
    record.payload.dmaMovConvReluRecord.dst = 0xaa;
    record.payload.dmaMovConvReluRecord.src = 0x55;
    record.payload.dmaMovConvReluRecord.nBurst = 100;
    record.payload.dmaMovConvReluRecord.lenBurst = 8;
    record.payload.dmaMovConvReluRecord.srcStride = 8;
    record.payload.dmaMovConvReluRecord.dstStride = 8;
    record.payload.dmaMovConvReluRecord.location.blockId = 7;
    record.payload.dmaMovConvReluRecord.crMode = ConvRelu::CRMODE_NONE;
    record.payload.dmaMovConvReluRecord.srcMemType = MemType::L0C;
    record.payload.dmaMovConvReluRecord.dstMemType = MemType::UB;
    record.payload.dmaMovConvReluRecord.srcDataType = DataType::DATA_B16;
    record.payload.dmaMovConvReluRecord.dstDataType = DataType::DATA_B16;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);

    MemOpInfo memOpInfo;
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x55, 8, 32, 1, 100, 8 + 8 * 512 / 32, 512};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    ASSERT_EQ(events[1].loc.coreId, 7);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0xaa, 8, 32, 1, 100, 8 + 8, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}

static DmaMovConvReluRecord CreateDmaMovConvReluRecord(void)
{
    DmaMovConvReluRecord dmaMovConvReluRecord;
    dmaMovConvReluRecord.dst = 0xaa;
    dmaMovConvReluRecord.src = 0x55;
    dmaMovConvReluRecord.nBurst = 1;
    dmaMovConvReluRecord.lenBurst = SRC1_BLOCK_SIZE;
    dmaMovConvReluRecord.srcStride = SRC1_BLOCK_SIZE;
    dmaMovConvReluRecord.dstStride = SRC1_BLOCK_SIZE;
    dmaMovConvReluRecord.location.blockId = CORE_ID;
    dmaMovConvReluRecord.crMode = ConvRelu::CRMODE_NONE;
    dmaMovConvReluRecord.srcMemType = MemType::L0C;
    dmaMovConvReluRecord.dstMemType = MemType::UB;
    dmaMovConvReluRecord.srcDataType = DataType::DATA_B16;
    dmaMovConvReluRecord.dstDataType = DataType::DATA_B16;
    dmaMovConvReluRecord.location.blockId = CORE_ID;
    return dmaMovConvReluRecord;
}
 
TEST_F(TestRecordParse, parse_dma_mov_depth_wise_b16_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::DMA_MOV_DEPTH_WISE;
    record.payload.dmaMovConvReluRecord = CreateDmaMovConvReluRecord();
 
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);
 
    MemOpInfo memOpInfo;
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x55, 8, 512, 1, 1, 1, 512};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
 
    ASSERT_EQ(events[1].loc.coreId, 7);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0xaa, 1, 2048, 1, 1, 1, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
 
    ASSERT_EQ(events[2].loc.coreId, 7);
    ASSERT_EQ(events[2].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[2].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0xaa + 2048 + 32 * 8, 1, 2048, 1, 1, 1, 32};
    ASSERT_EQ(events[2].eventInfo.memInfo, memOpInfo);
}
 
TEST_F(TestRecordParse, parse_dma_mov_depth_wise_b32_to_b16_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::DMA_MOV_DEPTH_WISE;
    record.payload.dmaMovConvReluRecord = CreateDmaMovConvReluRecord();
 
    record.payload.dmaMovConvReluRecord.srcDataType = DataType::DATA_B32;
    record.payload.dmaMovConvReluRecord.dstDataType = DataType::DATA_B16;
    record.payload.dmaMovConvReluRecord.crMode = ConvRelu::CRMODE_F32toF16_NONE;
 
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);
 
    MemOpInfo memOpInfo;
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x55, 8, 1024, 1, 1, 1, 1024};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
 
    ASSERT_EQ(events[1].loc.coreId, 7);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0xaa, 1, 2048, 1, 1, 1, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
 
    ASSERT_EQ(events[2].loc.coreId, 7);
    ASSERT_EQ(events[2].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[2].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0xaa + 2048 + 32 * 8, 1, 2048, 1, 1, 1, 32};
    ASSERT_EQ(events[2].eventInfo.memInfo, memOpInfo);
}
 
TEST_F(TestRecordParse, parse_dma_mov_depth_wise_b16_with_CRMODE_F32toF16_NONE_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::DMA_MOV_DEPTH_WISE;
    record.payload.dmaMovConvReluRecord = CreateDmaMovConvReluRecord();
 
    record.payload.dmaMovConvReluRecord.crMode = ConvRelu::CRMODE_F32toF16_NONE;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
}

TEST_F(TestRecordParse, parse_dma_mov_nd2nz_b16_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::DMA_MOV_ND2NZ;
    record.payload.dmaMovNd2nzRecord.dst = 0x0;
    record.payload.dmaMovNd2nzRecord.src = 0x0;
    record.payload.dmaMovNd2nzRecord.ndNum = 3;
    record.payload.dmaMovNd2nzRecord.nValue = 8;
    record.payload.dmaMovNd2nzRecord.dValue = 24;
    record.payload.dmaMovNd2nzRecord.srcNdMatrixStride = 12 * 32;
    record.payload.dmaMovNd2nzRecord.srcDValue = 32;
    record.payload.dmaMovNd2nzRecord.dstNzC0Stride = 52;
    record.payload.dmaMovNd2nzRecord.dstNzNStride = 2;
    record.payload.dmaMovNd2nzRecord.dstNzMatrixStride = 17 * 16;
    record.payload.dmaMovNd2nzRecord.location.blockId = 0;
    record.payload.dmaMovNd2nzRecord.srcMemType = MemType::GM;
    record.payload.dmaMovNd2nzRecord.dstMemType = MemType::L1;
    record.payload.dmaMovNd2nzRecord.dataType = DataType::DATA_B16;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3 + 2);  // ndNum + C0列数

    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE2);
    memOpInfo = {MemType::GM, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 24, 2, 1, 8, 32, 0};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    memOpInfo = {MemType::GM, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0 + 12 * 32 * 2, 24, 2, 1, 8, 32, 0};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);

    // 校验写内存event
    ASSERT_EQ(events[3].loc.coreId, 0);
    ASSERT_EQ(events[3].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[3].pipe, PipeType::PIPE_MTE2);
    memOpInfo = {MemType::L1, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 8, 32, 2, 3, 17, 32};
    ASSERT_EQ(events[3].eventInfo.memInfo, memOpInfo);

    memOpInfo = {MemType::L1, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0 + 52 * 32, 8, 32, 2, 3, 17, 32};
    ASSERT_EQ(events[4].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_dma_mov_nd2nz_b32_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::DMA_MOV_ND2NZ;
    record.payload.dmaMovNd2nzRecord.dst = 0x0;
    record.payload.dmaMovNd2nzRecord.src = 0x0;
    record.payload.dmaMovNd2nzRecord.ndNum = 3;
    record.payload.dmaMovNd2nzRecord.nValue = 8;
    record.payload.dmaMovNd2nzRecord.dValue = 24;
    record.payload.dmaMovNd2nzRecord.srcNdMatrixStride = 12 * 32;
    record.payload.dmaMovNd2nzRecord.srcDValue = 32;
    record.payload.dmaMovNd2nzRecord.dstNzC0Stride = 52;
    record.payload.dmaMovNd2nzRecord.dstNzNStride = 2;
    record.payload.dmaMovNd2nzRecord.dstNzMatrixStride = 17 * 8;
    record.payload.dmaMovNd2nzRecord.location.blockId = 0;
    record.payload.dmaMovNd2nzRecord.srcMemType = MemType::GM;
    record.payload.dmaMovNd2nzRecord.dstMemType = MemType::L1;
    record.payload.dmaMovNd2nzRecord.dataType = DataType::DATA_B32;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3 + 3);  // ndNum + C0列数

    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE2);
    memOpInfo = {MemType::GM, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 24, 4, 1, 8, 32, 0};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    memOpInfo = {MemType::GM, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0 + 12 * 32 * 4, 24, 4, 1, 8, 32, 0};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);

    // 校验写内存event
    ASSERT_EQ(events[3].loc.coreId, 0);
    ASSERT_EQ(events[3].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[3].pipe, PipeType::PIPE_MTE2);
    memOpInfo = {MemType::L1, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 8, 32, 2, 3, 17, 32};
    ASSERT_EQ(events[3].eventInfo.memInfo, memOpInfo);

    memOpInfo = {MemType::L1, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0 + 52 * 32, 8, 32, 2, 3, 17, 32};
    ASSERT_EQ(events[4].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_dma_mov_nd2nz_b8_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::DMA_MOV_ND2NZ;
    record.payload.dmaMovNd2nzRecord.dst = 0x0;
    record.payload.dmaMovNd2nzRecord.src = 0x0;
    record.payload.dmaMovNd2nzRecord.ndNum = 3;
    record.payload.dmaMovNd2nzRecord.nValue = 8;
    record.payload.dmaMovNd2nzRecord.dValue = 24;
    record.payload.dmaMovNd2nzRecord.srcNdMatrixStride = 12 * 32;
    record.payload.dmaMovNd2nzRecord.srcDValue = 32;
    record.payload.dmaMovNd2nzRecord.dstNzC0Stride = 52;
    record.payload.dmaMovNd2nzRecord.dstNzNStride = 2;
    record.payload.dmaMovNd2nzRecord.dstNzMatrixStride = 17 * 32;
    record.payload.dmaMovNd2nzRecord.location.blockId = 0;
    record.payload.dmaMovNd2nzRecord.srcMemType = MemType::GM;
    record.payload.dmaMovNd2nzRecord.dstMemType = MemType::L1;
    record.payload.dmaMovNd2nzRecord.dataType = DataType::DATA_B8;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3 + 1);  // ndNum + C0列数

    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE2);
    memOpInfo = {MemType::GM, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 24, 1, 1, 8, 32, 0};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    memOpInfo = {MemType::GM, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0 + 12 * 32 * 1, 24, 1, 1, 8, 32, 0};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);

    // 校验写内存event
    ASSERT_EQ(events[3].loc.coreId, 0);
    ASSERT_EQ(events[3].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[3].pipe, PipeType::PIPE_MTE2);
    memOpInfo = {MemType::L1, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 8, 32, 2, 3, 17, 32};
    ASSERT_EQ(events[3].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_mov_align_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::MOV_ALIGN;
    record.payload.movAlignRecord.dst = 0xaa;
    record.payload.movAlignRecord.src = 0x55;
    record.payload.movAlignRecord.nBurst = 100;
    record.payload.movAlignRecord.lenBurst = 8;
    record.payload.movAlignRecord.srcGap = 8;
    record.payload.movAlignRecord.dstGap = 8;
    record.payload.movAlignRecord.location.blockId = 7;
    record.payload.movAlignRecord.srcMemType = MemType::GM;
    record.payload.movAlignRecord.dstMemType = MemType::UB;
    record.payload.movAlignRecord.leftPaddingNum = 15;
    record.payload.movAlignRecord.rightPaddingNum = 5;
    record.payload.movAlignRecord.dataType = DataType::DATA_B32;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE2);
    MemOpInfo memOpInfo = {MemType::GM, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x55, 8, 1, 1, 100, 16, 1};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
    ASSERT_EQ(events[1].loc.coreId, 7);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_MTE2);
    memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0xaa, 96, 1, 1, 100, 352, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}


TEST_F(TestRecordParse, parse_mov_bt_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::MOV_BT;
    record.payload.movBtRecord.location.blockId = 7;
    record.payload.movBtRecord.src = 0x15;
    record.payload.movBtRecord.nBurst = 1;
    record.payload.movBtRecord.lenBurst = 2;
    record.payload.movBtRecord.srcGap = 3;

    record.payload.movBtRecord.srcMemType = MemType::L1;
    record.payload.movBtRecord.dstMemType = MemType::BT;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    MemOpInfo memOpInfo = {MemType::L1, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x15, 2 * 2, 32, 1, 1, 2 * 2 + 3, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_mov_fp_record_with_normal_movement_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::MOV_FP;
    record.payload.movFpRecord.dst = 0x111;
    record.payload.movFpRecord.src = 0x0;
    record.payload.movFpRecord.dstStride = 80;
    record.payload.movFpRecord.srcStride = 64;
    record.payload.movFpRecord.nSize = 48;
    record.payload.movFpRecord.mSize = 24;
    record.payload.movFpRecord.ndNum = 0;
    record.payload.movFpRecord.dstNdStride = 0;
    record.payload.movFpRecord.srcNdStride = 0;
    record.payload.movFpRecord.srcNzC0Stride = 0;
    record.payload.movFpRecord.quantPreBits= 32;
    record.payload.movFpRecord.enUnitFlag = false;
    record.payload.movFpRecord.int8ChannelMerge = false;
    record.payload.movFpRecord.int4ChannelMerge = false;
    record.payload.movFpRecord.channelSplit = false;
    record.payload.movFpRecord.enNZ2ND = false;
    record.payload.movFpRecord.enNZ2DN = false;
    record.payload.movFpRecord.location.blockId = 0;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);

    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 24, 64, 1, 3, 64, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    // 校验写内存event
    ASSERT_EQ(events[1].loc.coreId, 0);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::GM, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 48, 32, 1, 3, 80, 1};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_mov_fp_record_with_int8_channal_merging_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::MOV_FP;
    record.payload.movFpRecord.dst = 0x111;
    record.payload.movFpRecord.src = 0x0;
    record.payload.movFpRecord.dstStride = 32;
    record.payload.movFpRecord.srcStride = 32;
    record.payload.movFpRecord.nSize = 48;
    record.payload.movFpRecord.mSize = 32;
    record.payload.movFpRecord.ndNum = 0;
    record.payload.movFpRecord.dstNdStride = 0;
    record.payload.movFpRecord.srcNdStride = 0;
    record.payload.movFpRecord.srcNzC0Stride = 0;
    record.payload.movFpRecord.quantPreBits= 8;
    record.payload.movFpRecord.enUnitFlag = false;
    record.payload.movFpRecord.int8ChannelMerge = true;
    record.payload.movFpRecord.int4ChannelMerge = false;
    record.payload.movFpRecord.channelSplit = false;
    record.payload.movFpRecord.enNZ2ND = false;
    record.payload.movFpRecord.enNZ2DN = false;
    record.payload.movFpRecord.location.blockId = 0;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);

    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 32, 64, 1, 3, 32, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    // 校验写内存event
    ASSERT_EQ(events[1].loc.coreId, 0);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::GM, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 32, 32, 1, 1, 32, 1};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);

    memOpInfo = {MemType::GM, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111 + 32 * 32, 32, 16, 1, 1, 64, 1};
    ASSERT_EQ(events[2].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_mov_fp_record_with_int4_channal_merging_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::MOV_FP;
    record.payload.movFpRecord.dst = 0x111;
    record.payload.movFpRecord.src = 0x0;
    record.payload.movFpRecord.dstStride = 32;
    record.payload.movFpRecord.srcStride = 32;
    record.payload.movFpRecord.nSize = 128;
    record.payload.movFpRecord.mSize = 32;
    record.payload.movFpRecord.ndNum = 0;
    record.payload.movFpRecord.dstNdStride = 0;
    record.payload.movFpRecord.srcNdStride = 0;
    record.payload.movFpRecord.srcNzC0Stride = 0;
    record.payload.movFpRecord.quantPreBits= 4;
    record.payload.movFpRecord.enUnitFlag = false;
    record.payload.movFpRecord.int8ChannelMerge = false;
    record.payload.movFpRecord.int4ChannelMerge = true;
    record.payload.movFpRecord.channelSplit = false;
    record.payload.movFpRecord.enNZ2ND = false;
    record.payload.movFpRecord.enNZ2DN = false;
    record.payload.movFpRecord.location.blockId = 0;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);

    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 32, 64, 1, 8, 32, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    // 校验写内存event
    ASSERT_EQ(events[1].loc.coreId, 0);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::GM, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 32, 32, 1, 2, 32, 1};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_mov_fp_record_with_f32_channel_split_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::MOV_FP;
    record.payload.movFpRecord.dst = 0x111;
    record.payload.movFpRecord.src = 0x0;
    record.payload.movFpRecord.dstStride = 64;
    record.payload.movFpRecord.srcStride = 64;
    record.payload.movFpRecord.nSize = 24;
    record.payload.movFpRecord.mSize = 64;
    record.payload.movFpRecord.ndNum = 0;
    record.payload.movFpRecord.dstNdStride = 0;
    record.payload.movFpRecord.srcNdStride = 0;
    record.payload.movFpRecord.srcNzC0Stride = 0;
    record.payload.movFpRecord.quantPreBits= 32;
    record.payload.movFpRecord.enUnitFlag = false;
    record.payload.movFpRecord.int8ChannelMerge = false;
    record.payload.movFpRecord.int4ChannelMerge = false;
    record.payload.movFpRecord.channelSplit = true;
    record.payload.movFpRecord.enNZ2ND = false;
    record.payload.movFpRecord.enNZ2DN = false;
    record.payload.movFpRecord.location.blockId = 0;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);

    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 64, 64, 1, 2, 64, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    // 校验写内存event
    ASSERT_EQ(events[1].loc.coreId, 0);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::GM, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 64, 32, 1, 3, 64, 1};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_mov_fp_record_with_NZ2ND_conversion_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::MOV_FP;
    record.payload.movFpRecord.dst = 0x111;
    record.payload.movFpRecord.src = 0x0;
    record.payload.movFpRecord.dstStride = 48;
    record.payload.movFpRecord.srcStride = 80;
    record.payload.movFpRecord.nSize = 24;
    record.payload.movFpRecord.mSize = 48;
    record.payload.movFpRecord.ndNum = 2;
    record.payload.movFpRecord.dstNdStride = 64 * record.payload.movFpRecord.dstStride;
    record.payload.movFpRecord.srcNdStride = 15;
    record.payload.movFpRecord.srcNzC0Stride = 0;
    record.payload.movFpRecord.quantPreBits= 32;
    record.payload.movFpRecord.enUnitFlag = false;
    record.payload.movFpRecord.int8ChannelMerge = false;
    record.payload.movFpRecord.int4ChannelMerge = false;
    record.payload.movFpRecord.channelSplit = false;
    record.payload.movFpRecord.enNZ2ND = true;
    record.payload.movFpRecord.enNZ2DN = false;
    record.payload.movFpRecord.location.blockId = 0;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 6);

    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 48, 64, 1, 1, 80, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    // 校验写内存event
    ASSERT_EQ(events[4].loc.coreId, 0);
    ASSERT_EQ(events[4].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[4].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::GM, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 24, 4, 1, 48, 48, 1};
    ASSERT_EQ(events[4].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_vec_dup_record_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::VEC_DUP;
    record.payload.vecDupRecord.dst = 0xaa;
    record.payload.vecDupRecord.dstBlockStride = 8;
    record.payload.vecDupRecord.dstRepeatStride = 9;
    record.payload.vecDupRecord.location.blockId = 7;
    record.payload.vecDupRecord.repeat = 100;
    record.payload.vecDupRecord.dataBits = 32;
    record.payload.vecDupRecord.maskMode = MaskMode::MASK_NORM;
    record.payload.vecDupRecord.vectorMask = DEFAULT_VECTOR_MASK;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);

    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    MemOpInfo memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        32, 0xaa, 8, 32, 8, 100, 9, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_vec_dup_record_with_mask_count_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.blockType = BlockType::AIVEC;
    record.recordType = RecordType::VEC_DUP;
    record.payload.vecDupRecord.dst = 0xaa;
    record.payload.vecDupRecord.dstBlockStride = 8;
    record.payload.vecDupRecord.dstRepeatStride = 9;
    record.payload.vecDupRecord.location.blockId = 7;
    record.payload.vecDupRecord.repeat = 100;
    record.payload.vecDupRecord.dataBits = 32;
    record.payload.vecDupRecord.vectorMask.mask0 = 64;
    record.payload.vecDupRecord.vectorMask.mask1 = 0;
    record.payload.vecDupRecord.maskMode = MaskMode::MASK_COUNT;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);

    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    MemOpInfo memOpInfo = {MemType::UB, AccessType::WRITE, {64, 0}, MaskMode::MASK_COUNT,
        32, 0xaa, 8, 32, 8, 1, 9, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_load_2d_record)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::LOAD_2D;
    record.payload.load2DRecord.dst = 0x13;
    record.payload.load2DRecord.src = 0x12;
    record.payload.load2DRecord.baseIdx = 3;
    record.payload.load2DRecord.repeat = 4;
    record.payload.load2DRecord.addrCalMode = AddrCalMode::DEC;
    record.payload.load2DRecord.srcStride = 5;
    record.payload.load2DRecord.dstStride = 6;
    record.payload.load2DRecord.blockSize = 512;
    record.payload.load2DRecord.location.fileNo = 7;
    record.payload.load2DRecord.location.lineNo = 8;

    record.payload.load2DRecord.srcMemType = MemType::GM;
    record.payload.load2DRecord.dstMemType = MemType::L0B;
    record.payload.load2DRecord.location.blockId = 9;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
    sanitizerRecord.payload.kernelRecord.payload.load2DRecord.addrCalMode = AddrCalMode::INC;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 9);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE2);
    MemOpInfo memOpInfo = {MemType::GM, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x12 + 512 * 3, 1, 512, 0, record.payload.load2DRecord.repeat,
        record.payload.load2DRecord.srcStride, 1};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
    MemOpInfo memOpInfo1 = {MemType::L0B, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x13, 1, 512, 0, record.payload.load2DRecord.repeat,
        record.payload.load2DRecord.dstStride, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo1);
}

TEST_F(TestRecordParse, parse_load_2d_sparse_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::LOAD_2D_SPARSE;
    record.payload.load2DSparseRecord.dst = 0x13;
    record.payload.load2DSparseRecord.src0 = 0x12 & 0xFFFFFFFF;
    record.payload.load2DSparseRecord.src1 = (0x12 >> 32U) & 0xFFFFFFFF;
    record.payload.load2DSparseRecord.startId = 2;
    record.payload.load2DSparseRecord.repeat = 4;
    record.payload.load2DSparseRecord.location.fileNo = 7;
    record.payload.load2DSparseRecord.location.lineNo = 8;

    record.payload.load2DSparseRecord.srcMemType = MemType::L1;
    record.payload.load2DSparseRecord.dstMemType = MemType::L0B;
    record.payload.load2DSparseRecord.location.blockId = 9;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);
    ASSERT_EQ(events[0].loc.coreId, 9);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    MemOpInfo memOpInfo = {MemType::L1, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, (0x12 & 0xFFFFFFFF) + 512 * 2, 1, 512, 1, record.payload.load2DSparseRecord.repeat, 1, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
    MemOpInfo memOpInfo1 = {MemType::L1, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, ((0x12 >> 32U) & 0xFFFFFFFF) + 128 * 2, 1, 128, 1, record.payload.load2DSparseRecord.repeat, 1, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo1);
    MemOpInfo memOpInfo2 = {MemType::L0B, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x13, 1, 512, 1, record.payload.load2DSparseRecord.repeat, 1, 512};
    ASSERT_EQ(events[2].eventInfo.memInfo, memOpInfo2);
}

TEST_F(TestRecordParse, parse_load_2d_sparse_record_with_repeat_is_zero_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::LOAD_2D_SPARSE;
    record.payload.load2DSparseRecord.dst = 0x13;
    record.payload.load2DSparseRecord.src0 = 0x12 & 0xFFFFFFFF;
    record.payload.load2DSparseRecord.src1 = (0x12 >> 32U) & 0xFFFFFFFF;
    record.payload.load2DSparseRecord.startId = 2;
    record.payload.load2DSparseRecord.repeat = 0;
    record.payload.load2DSparseRecord.location.fileNo = 7;
    record.payload.load2DSparseRecord.location.lineNo = 8;

    record.payload.load2DSparseRecord.srcMemType = MemType::L1;
    record.payload.load2DSparseRecord.dstMemType = MemType::L0B;
    record.payload.load2DSparseRecord.location.blockId = 9;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
}

TEST_F(TestRecordParse, parse_load_2d_to_cb_transpose_b4_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::LOAD_2D_TRANSPOSE;
    record.payload.load2DTransposeRecord.dst = 0x13;
    record.payload.load2DTransposeRecord.src = 0x12;
    record.payload.load2DTransposeRecord.indexId = 1;
    record.payload.load2DTransposeRecord.repeat = 4;
    record.payload.load2DTransposeRecord.location.fileNo = 7;
    record.payload.load2DTransposeRecord.location.lineNo = 8;
    record.payload.load2DTransposeRecord.addrMode = false;
    record.payload.load2DTransposeRecord.srcStride = 2;
    record.payload.load2DTransposeRecord.dstStride = 5;
    record.payload.load2DTransposeRecord.dstFracStride = 0;

    record.payload.load2DTransposeRecord.dataType = DataType::DATA_B4;
    record.payload.load2DTransposeRecord.srcMemType = MemType::L1;
    record.payload.load2DTransposeRecord.dstMemType = MemType::L0B;
    record.payload.load2DTransposeRecord.location.blockId = 9;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 9);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    MemOpInfo memOpInfo = {MemType::L1, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8,
        0x12 + 512 * 4, 4, 512, 1, record.payload.load2DTransposeRecord.repeat,
        4U * record.payload.load2DTransposeRecord.srcStride, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
    MemOpInfo memOpInfo2 = {MemType::L0B, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8, 0x13,
        4, 512, record.payload.load2DTransposeRecord.dstFracStride + 1U,
        record.payload.load2DTransposeRecord.repeat, record.payload.load2DTransposeRecord.dstStride + 1U, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo2);
}

TEST_F(TestRecordParse, parse_load_2d_to_cb_transpose_b8_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::LOAD_2D_TRANSPOSE;
    record.payload.load2DTransposeRecord.dst = 0x13;
    record.payload.load2DTransposeRecord.src = 0x12;
    record.payload.load2DTransposeRecord.indexId = 1;
    record.payload.load2DTransposeRecord.repeat = 4;
    record.payload.load2DTransposeRecord.location.fileNo = 7;
    record.payload.load2DTransposeRecord.location.lineNo = 8;
    record.payload.load2DTransposeRecord.addrMode = false;
    record.payload.load2DTransposeRecord.srcStride = 2;
    record.payload.load2DTransposeRecord.dstStride = 5;
    record.payload.load2DTransposeRecord.dstFracStride = 0;

    record.payload.load2DTransposeRecord.dataType = DataType::DATA_B8;
    record.payload.load2DTransposeRecord.srcMemType = MemType::L1;
    record.payload.load2DTransposeRecord.dstMemType = MemType::L0B;
    record.payload.load2DTransposeRecord.location.blockId = 9;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 9);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    MemOpInfo memOpInfo = {MemType::L1, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8,
        0x12 + 512 * 2, 2, 512, 1, record.payload.load2DTransposeRecord.repeat,
        2U * record.payload.load2DTransposeRecord.srcStride, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
    MemOpInfo memOpInfo2 = {MemType::L0B, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8, 0x13,
        2, 512, record.payload.load2DTransposeRecord.dstFracStride + 1U,
        record.payload.load2DTransposeRecord.repeat, record.payload.load2DTransposeRecord.dstStride + 1U, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo2);
}

TEST_F(TestRecordParse, parse_load_2d_to_cb_transpose_b8_record_with_addrMode_true_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::LOAD_2D_TRANSPOSE;
    record.payload.load2DTransposeRecord.dst = 0x13;
    record.payload.load2DTransposeRecord.src = 0x12;
    record.payload.load2DTransposeRecord.indexId = 6;
    record.payload.load2DTransposeRecord.repeat = 4;
    record.payload.load2DTransposeRecord.location.fileNo = 7;
    record.payload.load2DTransposeRecord.location.lineNo = 8;
    record.payload.load2DTransposeRecord.addrMode = true;
    record.payload.load2DTransposeRecord.srcStride = 2;
    record.payload.load2DTransposeRecord.dstStride = 5;
    record.payload.load2DTransposeRecord.dstFracStride = 0;

    record.payload.load2DTransposeRecord.dataType = DataType::DATA_B8;
    record.payload.load2DTransposeRecord.srcMemType = MemType::L1;
    record.payload.load2DTransposeRecord.dstMemType = MemType::L0B;
    record.payload.load2DTransposeRecord.location.blockId = 9;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 9);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    MemOpInfo memOpInfo = {MemType::L1, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8,
        0x12U + 512 * 2 * (6 - record.payload.load2DTransposeRecord.srcStride *
        (record.payload.load2DTransposeRecord.repeat - 1)),
        2, 512, 1,
        record.payload.load2DTransposeRecord.repeat, 2U * record.payload.load2DTransposeRecord.srcStride, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
    MemOpInfo memOpInfo2 = {MemType::L0B, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8, 0x13,
        2, 512, record.payload.load2DTransposeRecord.dstFracStride + 1U,
        record.payload.load2DTransposeRecord.repeat, record.payload.load2DTransposeRecord.dstStride + 1U, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo2);
}

TEST_F(TestRecordParse, parse_load_2d_to_cb_transpose_b16_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::LOAD_2D_TRANSPOSE;
    record.payload.load2DTransposeRecord.dst = 0x13;
    record.payload.load2DTransposeRecord.src = 0x12;
    record.payload.load2DTransposeRecord.indexId = 1;
    record.payload.load2DTransposeRecord.repeat = 4;
    record.payload.load2DTransposeRecord.location.fileNo = 7;
    record.payload.load2DTransposeRecord.location.lineNo = 8;
    record.payload.load2DTransposeRecord.addrMode = false;
    record.payload.load2DTransposeRecord.srcStride = 2;
    record.payload.load2DTransposeRecord.dstStride = 5;
    record.payload.load2DTransposeRecord.dstFracStride = 0;

    record.payload.load2DTransposeRecord.dataType = DataType::DATA_B16;
    record.payload.load2DTransposeRecord.srcMemType = MemType::L1;
    record.payload.load2DTransposeRecord.dstMemType = MemType::L0B;
    record.payload.load2DTransposeRecord.location.blockId = 9;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 9);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    MemOpInfo memOpInfo = {MemType::L1, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8,
        0x12 + 512, 1, 512, 1, record.payload.load2DTransposeRecord.repeat,
        record.payload.load2DTransposeRecord.srcStride, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
    MemOpInfo memOpInfo2 = {MemType::L0B, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8, 0x13,
        1, 512, record.payload.load2DTransposeRecord.dstFracStride +1U,
        record.payload.load2DTransposeRecord.repeat, record.payload.load2DTransposeRecord.dstStride + 1U, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo2);
}

TEST_F(TestRecordParse, parse_load_2d_to_cb_transpose_b32_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::LOAD_2D_TRANSPOSE;
    record.payload.load2DTransposeRecord.dst = 0x13;
    record.payload.load2DTransposeRecord.src = 0x12;
    record.payload.load2DTransposeRecord.indexId = 1;
    record.payload.load2DTransposeRecord.repeat = 4;
    record.payload.load2DTransposeRecord.location.fileNo = 7;
    record.payload.load2DTransposeRecord.location.lineNo = 8;
    record.payload.load2DTransposeRecord.addrMode = false;
    record.payload.load2DTransposeRecord.srcStride = 2;
    record.payload.load2DTransposeRecord.dstStride = 5;
    record.payload.load2DTransposeRecord.dstFracStride = 0;

    record.payload.load2DTransposeRecord.dataType = DataType::DATA_B32;
    record.payload.load2DTransposeRecord.srcMemType = MemType::L1;
    record.payload.load2DTransposeRecord.dstMemType = MemType::L0B;
    record.payload.load2DTransposeRecord.location.blockId = 9;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 9);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    MemOpInfo memOpInfo = {MemType::L1, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8,
        0x12 + 512 * 2, 2, 512, 1, record.payload.load2DTransposeRecord.repeat,
        2U * record.payload.load2DTransposeRecord.srcStride, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
    MemOpInfo memOpInfo2 = {MemType::L0B, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8, 0x13,
        2, 512, record.payload.load2DTransposeRecord.dstFracStride + 1U,
        record.payload.load2DTransposeRecord.repeat, record.payload.load2DTransposeRecord.dstStride + 1U, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo2);
}

TEST_F(TestRecordParse, parse_load_2d_to_ca_transpose_b8_record_and_export_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::LOAD_2D_TRANSPOSE;
    record.payload.load2DTransposeRecord.dst = 0x13;
    record.payload.load2DTransposeRecord.src = 0x12;
    record.payload.load2DTransposeRecord.indexId = 1;
    record.payload.load2DTransposeRecord.repeat = 4;
    record.payload.load2DTransposeRecord.location.fileNo = 7;
    record.payload.load2DTransposeRecord.location.lineNo = 8;
    record.payload.load2DTransposeRecord.addrMode = false;
    record.payload.load2DTransposeRecord.srcStride = 2;
    record.payload.load2DTransposeRecord.dstStride = 5;
    record.payload.load2DTransposeRecord.dstFracStride = 0;

    record.payload.load2DTransposeRecord.dataType = DataType::DATA_B8;
    record.payload.load2DTransposeRecord.srcMemType = MemType::L1;
    record.payload.load2DTransposeRecord.dstMemType = MemType::L0A;
    record.payload.load2DTransposeRecord.location.blockId = 9;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 9);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    MemOpInfo memOpInfo = {MemType::L1, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8,
        0x12 + 512 * 2, 2, 512, 1, record.payload.load2DTransposeRecord.repeat,
        2U * record.payload.load2DTransposeRecord.srcStride, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
    MemOpInfo memOpInfo2 = {MemType::L0A, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8, 0x13,
        2, 512, record.payload.load2DTransposeRecord.dstFracStride + 1U,
        record.payload.load2DTransposeRecord.repeat, record.payload.load2DTransposeRecord.dstStride + 1U, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo2);
}

TEST_F(TestRecordParse, parse_load_2d_to_ca_transpose_b16_record_and_export_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::LOAD_2D_TRANSPOSE;
    record.payload.load2DTransposeRecord.dst = 0x13;
    record.payload.load2DTransposeRecord.src = 0x12;
    record.payload.load2DTransposeRecord.indexId = 1;
    record.payload.load2DTransposeRecord.repeat = 4;
    record.payload.load2DTransposeRecord.location.fileNo = 7;
    record.payload.load2DTransposeRecord.location.lineNo = 8;
    record.payload.load2DTransposeRecord.addrMode = false;
    record.payload.load2DTransposeRecord.srcStride = 2;
    record.payload.load2DTransposeRecord.dstStride = 5;
    record.payload.load2DTransposeRecord.dstFracStride = 0;

    record.payload.load2DTransposeRecord.dataType = DataType::DATA_B16;
    record.payload.load2DTransposeRecord.srcMemType = MemType::L1;
    record.payload.load2DTransposeRecord.dstMemType = MemType::L0A;
    record.payload.load2DTransposeRecord.location.blockId = 9;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 9);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    MemOpInfo memOpInfo = {MemType::L1, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8,
        0x12 + 512, 1, 512, 1, record.payload.load2DTransposeRecord.repeat,
        record.payload.load2DTransposeRecord.srcStride, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
    MemOpInfo memOpInfo2 = {MemType::L0A, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8, 0x13,
        1, 512, record.payload.load2DTransposeRecord.dstFracStride +1U,
        record.payload.load2DTransposeRecord.repeat, record.payload.load2DTransposeRecord.dstStride + 1U, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo2);
}

TEST_F(TestRecordParse, parse_load_2d_to_ca_transpose_b32_record_and_export_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::LOAD_2D_TRANSPOSE;
    record.payload.load2DTransposeRecord.dst = 0x13;
    record.payload.load2DTransposeRecord.src = 0x12;
    record.payload.load2DTransposeRecord.indexId = 1;
    record.payload.load2DTransposeRecord.repeat = 4;
    record.payload.load2DTransposeRecord.location.fileNo = 7;
    record.payload.load2DTransposeRecord.location.lineNo = 8;
    record.payload.load2DTransposeRecord.addrMode = false;
    record.payload.load2DTransposeRecord.srcStride = 2;
    record.payload.load2DTransposeRecord.dstStride = 5;
    record.payload.load2DTransposeRecord.dstFracStride = 0;

    record.payload.load2DTransposeRecord.dataType = DataType::DATA_B32;
    record.payload.load2DTransposeRecord.srcMemType = MemType::L1;
    record.payload.load2DTransposeRecord.dstMemType = MemType::L0A;
    record.payload.load2DTransposeRecord.location.blockId = 9;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 9);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    MemOpInfo memOpInfo = {MemType::L1, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8,
        0x12 + 512 * 2, 2, 512, 1, record.payload.load2DTransposeRecord.repeat,
        2U * record.payload.load2DTransposeRecord.srcStride, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
    MemOpInfo memOpInfo2 = {MemType::L0A, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8, 0x13,
        2, 512, record.payload.load2DTransposeRecord.dstFracStride + 1U,
        record.payload.load2DTransposeRecord.repeat, record.payload.load2DTransposeRecord.dstStride + 1U, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo2);
}

TEST_F(TestRecordParse, parse_load_2d_to_ca_transpose_record_indexId_less_than_zero_and_export_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::LOAD_2D_TRANSPOSE;
    record.payload.load2DTransposeRecord.dst = 0x13;
    record.payload.load2DTransposeRecord.src = 0x12;
    record.payload.load2DTransposeRecord.indexId = 3;
    record.payload.load2DTransposeRecord.repeat = 4;
    record.payload.load2DTransposeRecord.location.fileNo = 7;
    record.payload.load2DTransposeRecord.location.lineNo = 8;
    record.payload.load2DTransposeRecord.addrMode = true;
    record.payload.load2DTransposeRecord.srcStride = 2;
    record.payload.load2DTransposeRecord.dstStride = 5;
    record.payload.load2DTransposeRecord.dstFracStride = 0;

    record.payload.load2DTransposeRecord.dataType = DataType::DATA_B32;
    record.payload.load2DTransposeRecord.srcMemType = MemType::L1;
    record.payload.load2DTransposeRecord.dstMemType = MemType::L0A;
    record.payload.load2DTransposeRecord.location.blockId = 9;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
}

TEST_F(TestRecordParse, parse_decompress_header_record_and_export_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
 
    record.recordType = RecordType::DECOMPRESS_HEADER;
    record.payload.decompressHeaderRecord.src = 0x12;
    record.payload.decompressHeaderRecord.nBlock = 10;
    record.payload.decompressHeaderRecord.location.fileNo = 7;
    record.payload.decompressHeaderRecord.location.lineNo = 8;
 
    record.payload.decompressHeaderRecord.srcMemType = MemType::GM;
    record.payload.decompressHeaderRecord.location.blockId = 9;
 
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].loc.coreId, 9);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE2);
    MemOpInfo memOpInfo = {MemType::GM, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8,
        0x12, 1, 32 * 10, 1, 1, 1, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_dc_preload_record_and_export_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
 
    record.recordType = RecordType::DC_PRELOAD;
    record.payload.dcPreloadRecord.addr = 0x12;
    record.payload.dcPreloadRecord.offset = 32;
    record.payload.dcPreloadRecord.location.fileNo = 7;
    record.payload.dcPreloadRecord.location.lineNo = 8;

    record.payload.dcPreloadRecord.location.blockId = 9;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].loc.coreId, 9);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_S_CAL);
    MemOpInfo memOpInfo = {MemType::GM, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM, 8,
        0x12 + 32, 1, 0, 1, 1, 1, 1};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
}

static BroadcastRecord CreateBroadcastRecord(void)
{
    BroadcastRecord broadcastRecord;
    broadcastRecord.dst = 0x13;
    broadcastRecord.src = 0x12;
    broadcastRecord.nBurst = 1;
    broadcastRecord.lenBurst = SRC1_BLOCK_NUM;
    broadcastRecord.srcGap = 1;
    broadcastRecord.dstGap = 1;
    broadcastRecord.enableRepeat = false;

    broadcastRecord.srcDataType = DataType::DATA_B16;
    broadcastRecord.dstDataType = DataType::DATA_B16;
    broadcastRecord.srcMemType = MemType::UB;
    broadcastRecord.dstMemType = MemType::L0C;
    broadcastRecord.location.blockId = CORE_ID;
    return broadcastRecord;
}

TEST_F(TestRecordParse, parse_broadcast_L0C16_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::BROADCAST;
    record.payload.broadcastRecord = CreateBroadcastRecord();

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    MemOpInfo memOpInfo = {MemType::UB, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x12, 2, 32, 1, record.payload.broadcastRecord.nBurst, 3, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
    MemOpInfo memOpInfo1 = {MemType::L0C, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x13, 2, 512, 1, record.payload.broadcastRecord.nBurst, 3, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo1);
}

TEST_F(TestRecordParse, parse_broadcast_L0C32_record_with_conv_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::BROADCAST;
    record.payload.broadcastRecord = CreateBroadcastRecord();
    record.payload.broadcastRecord.dstDataType = DataType::DATA_B32;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    MemOpInfo memOpInfo = {MemType::UB, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x12, 2, 32, 1, record.payload.broadcastRecord.nBurst, 3, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
    MemOpInfo memOpInfo1 = {MemType::L0C, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x13, 2, 1024, 1, record.payload.broadcastRecord.nBurst, 3, 1024};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo1);
}

TEST_F(TestRecordParse, parse_broadcast_L0C32_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::BROADCAST;
    record.payload.broadcastRecord = CreateBroadcastRecord();

    record.payload.broadcastRecord.srcDataType = DataType::DATA_B32;
    record.payload.broadcastRecord.dstDataType = DataType::DATA_B32;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    MemOpInfo memOpInfo = {MemType::UB, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x12, 4, 32, 1, record.payload.broadcastRecord.nBurst, 5, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
    MemOpInfo memOpInfo1 = {MemType::L0C, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x13, 2, 1024, 1, record.payload.broadcastRecord.nBurst, 3, 1024};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo1);
}

TEST_F(TestRecordParse, parse_broadcast_L0C16_record_with_nBurst_is_zero_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::BROADCAST;
    record.payload.broadcastRecord = CreateBroadcastRecord();
    record.payload.broadcastRecord.nBurst = 0;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
}

TEST_F(TestRecordParse, parse_broadcast_L0C16_record_with_enable_repeat_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::BROADCAST;
    record.payload.broadcastRecord = CreateBroadcastRecord();
    record.payload.broadcastRecord.enableRepeat = true;
    record.payload.broadcastRecord.dstGap = 2;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    MemOpInfo memOpInfo = {MemType::UB, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x12, 1, 32, 1, record.payload.broadcastRecord.nBurst, 2, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
    MemOpInfo memOpInfo1 = {MemType::L0C, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x13, 2, 512, 1, record.payload.broadcastRecord.nBurst, 4, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo1);
}

TEST_F(TestRecordParse, parse_load_3d_record)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::LOAD_3D;
    record.payload.load3DRecord.location = {12, 34};
    record.payload.load3DRecord.dst = 0x5F5F;
    record.payload.load3DRecord.src = 0x5F5F5F;
    record.payload.load3DRecord.fMapW = 16;
    record.payload.load3DRecord.fMapH = 16;
    record.payload.load3DRecord.fMapC = 16;
    record.payload.load3DRecord.filterW = 3;
    record.payload.load3DRecord.filterH = 3;
    record.payload.load3DRecord.matrixRptStride = 1;
    record.payload.load3DRecord.location.blockId = 111;
    record.payload.load3DRecord.filterWStride = 1;
    record.payload.load3DRecord.filterHStride = 1;
    record.payload.load3DRecord.matrixKPos = 0;
    record.payload.load3DRecord.matrixMPos = 0;
    record.payload.load3DRecord.matrixKStep = 9;
    record.payload.load3DRecord.matrixMStep = 16;
    record.payload.load3DRecord.matrixRptTimes = 1;
    record.payload.load3DRecord.dataType = DataType::DATA_B16;
    record.payload.load3DRecord.dstMemType = MemType::L0A;
    record.payload.load3DRecord.srcMemType = MemType::L1;
    record.payload.load3DRecord.fMapTopPad = 1;
    record.payload.load3DRecord.fMapBottomPad = 1;
    record.payload.load3DRecord.fMapLeftPad = 1;
    record.payload.load3DRecord.fMapRightPad = 1;
    record.payload.load3DRecord.filterWDilation = 1;
    record.payload.load3DRecord.filterHDilation = 1;
    record.payload.load3DRecord.matrixMode = 0;
    record.payload.load3DRecord.matrixRptMode = 0;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2210);
    ASSERT_EQ(events[0].loc.coreId, 111);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    MemOpInfo memOpInfo = {MemType::L1, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x5F5F5F, 1, 32, 1, 1, 1, 0};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
    MemOpInfo memOpInfo1 = {MemType::L0A, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x5F5F, 1, 288, 1, 1, 1, 512};
    ASSERT_EQ(events[2209].eventInfo.memInfo, memOpInfo1);
}

TEST_F(TestRecordParse, parse_load_b2_record)
{
    std::vector<SanEvent> events;
    KernelRecord kRecord;
    kRecord.recordType = RecordType::LOAD_B2;
    auto &record = kRecord.payload.loadB2Record;
    record.location.blockId = 21;
    record.dataType = DataType::DATA_B8;
    record.dst = 512;
    record.src = 32;
    record.dstMemType = MemType::L0B;
    record.srcMemType = MemType::L1;
    record.repeat = 1;
    record.location.fileNo = 1;
    record.location.lineNo = 1;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kRecord;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 21);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    MemOpInfo MemOpInfo0 = {MemType::L1, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 32, 1, 512, 1, 1, 0, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, MemOpInfo0);
    MemOpInfo MemOpInfo1 = {MemType::L0B, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 512, 1, 512, 1, 1, 0, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, MemOpInfo1);
}

TEST_F(TestRecordParse, parse_load_A_winograd_record)
{
    std::vector<SanEvent> events;
    KernelRecord kRecord;
    kRecord.recordType = RecordType::LOAD_A_WINOGRAD;
    auto &record = kRecord.payload.loadAWinogradRecord;
    record.dstMemType = MemType::L0A;
    record.srcMemType = MemType::L1;
    record.location.fileNo = 1;
    record.location.lineNo = 1;
    record.location.blockId = 10;
    record.dataType = DataType::DATA_B8;
    record.dst = 512;
    record.src = 32;
    record.fmSizeH = 16;
    record.fmSizeW = 8;
    record.fmSizeCh = 4;
    record.innerDstGap = 1;
    record.dstStartPointK = 0;
    record.extStepK = 32;
    record.dstStartPointM = 0;
    record.extStepM = 32;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kRecord;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 10);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);

    uint8_t sizeFactor = record.dataType == DataType::DATA_B8 ? 1 : 2;
    uint32_t dstBlkSize = record.extStepK * record.extStepM * sizeFactor;
    MemOpInfo MemOpInfo0 = {MemType::L1, AccessType::READ, DEFAULT_VECTOR_MASK,
                            MaskMode::MASK_NORM, 8, record.src, 1,
                            static_cast<uint32_t>(record.fmSizeW) *
                            static_cast<uint32_t>(record.fmSizeH) *
                            static_cast<uint32_t>(record.fmSizeCh) *
                            static_cast<uint32_t>(sizeFactor),
                            0, 1, 0, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, MemOpInfo0);
    MemOpInfo MemOpInfo1 = {MemType::L0A, AccessType::WRITE, DEFAULT_VECTOR_MASK,
                            MaskMode::MASK_NORM, 8, record.dst, dstBlkSize / 512,
                            512, 1, 4, dstBlkSize / 512 + 1, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, MemOpInfo1);
}

TEST_F(TestRecordParse, parse_load_B_winograd_record)
{
    std::vector<SanEvent> events;
    KernelRecord kRecord;
    kRecord.recordType = RecordType::LOAD_B_WINOGRAD;
    auto &record = kRecord.payload.loadBWinogradRecord;
    record.location.blockId = 13;
    record.dst = 512;
    record.src = 32;
    record.dstMemType = MemType::L0B;
    record.srcMemType = MemType::L1;
    record.repeat = 2;
    record.location.fileNo = 1;
    record.location.lineNo = 1;
    record.srcRptStride = 4;
    record.dstRptStride = 4;
    record.innerDstStride = 0;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kRecord;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 13);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    MemOpInfo MemOpInfo0 = {MemType::L1, AccessType::READ, DEFAULT_VECTOR_MASK,
                            MaskMode::MASK_NORM, 8, record.src, 9, 512U, 1, record.repeat,
                            record.srcRptStride, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, MemOpInfo0);
    MemOpInfo MemOpInfo1 = {MemType::L0B, AccessType::WRITE, DEFAULT_VECTOR_MASK,
                            MaskMode::MASK_NORM, 8, record.dst, 4, 512U, record.innerDstStride,
                            record.repeat, record.dstRptStride, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, MemOpInfo1);
}

TEST_F(TestRecordParse, parse_set_l0a_2d_record)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::SET_2D;
    record.payload.set2DRecord.location.blockId = 7;
    record.payload.set2DRecord.dst = 0x15;
    record.payload.set2DRecord.repeat = 1;
    record.payload.set2DRecord.dstBlockNum = 1;
    record.payload.set2DRecord.dstBlockSize = 512;
    record.payload.set2DRecord.repeatGap = 1;

    record.payload.set2DRecord.dstMemType = MemType::L0A;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    MemOpInfo memOpInfo = {MemType::L0A, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x15, 1, 512, 1, 1, 1 + 1, 512};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_set_l1_2d_record)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::SET_2D;
    record.payload.set2DRecord.location.blockId = 7;
    record.payload.set2DRecord.dst = 0x15;
    record.payload.set2DRecord.repeat = 1;
    record.payload.set2DRecord.dstBlockNum = 1;
    record.payload.set2DRecord.dstBlockSize = 32;
    record.payload.set2DRecord.repeatGap = 1;

    record.payload.set2DRecord.dstMemType = MemType::L1;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE2);
    MemOpInfo memOpInfo = {MemType::L1, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x15, 1, 32, 1, 1, 1 + 1, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    record.payload.set2DRecord.dstMemType = MemType::L0A;
    record.payload.set2DRecord.dstBlockSize = 512;
    record.payload.set2DRecord.location.blockId = 9;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[1].loc.coreId, 9);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_MTE1);
    memOpInfo = {MemType::L0A, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x15, 1, 512, 1, 1, 1 + 1, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);

    record.payload.set2DRecord.dstMemType = MemType::L0B;
    record.payload.set2DRecord.location.blockId = 10;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);
    ASSERT_EQ(events[2].loc.coreId, 10);
    ASSERT_EQ(events[2].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[2].pipe, PipeType::PIPE_MTE1);
    memOpInfo = {MemType::L0B, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x15, 1, 512, 1, 1, 1 + 1, 512};
    ASSERT_EQ(events[2].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_load_image_s8_record)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::LOAD_IMAGE;
    record.payload.loadImageRecord.location.blockId = 7;
    record.payload.loadImageRecord.dst = 0x15;
    record.payload.loadImageRecord.horSize = 80;
    record.payload.loadImageRecord.verSize = 80;
    record.payload.loadImageRecord.horStartP = 5;
    record.payload.loadImageRecord.verStartP = 5;
    record.payload.loadImageRecord.sHorRes = 5;
    record.payload.loadImageRecord.topPadSize = 10;
    record.payload.loadImageRecord.botPadSize = 10;
    record.payload.loadImageRecord.lPadSize = 10;
    record.payload.loadImageRecord.rPadSize = 10;
    record.payload.loadImageRecord.dataType = DataType::DATA_B8;
    record.payload.loadImageRecord.dstMemType = MemType::L1;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
    uint32_t blockNum = (record.payload.loadImageRecord.horSize + record.payload.loadImageRecord.rPadSize +
        record.payload.loadImageRecord.lPadSize) * (record.payload.loadImageRecord.verSize +
        record.payload.loadImageRecord.topPadSize + record.payload.loadImageRecord.botPadSize);
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE2);
    MemOpInfo memOpInfo = {MemType::L1, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x15, blockNum, 32, 1, 1, 0, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_load_smask_record_and_expect_success)
{
    ///TEST_F load_smask_table_from_gm
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::LOAD_SMASK;
    record.payload.loadSmaskRecord.location.blockId = 7;
    record.payload.loadSmaskRecord.dst = 0x3F0;
    record.payload.loadSmaskRecord.src = 0x78F00;
    record.payload.loadSmaskRecord.smaskSize = 8;
    record.payload.loadSmaskRecord.srcMemType = MemType::GM;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE2);
    MemOpInfo memOpInfo = {MemType::GM, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
                           8, 0x78F00, 1, 8, 0, 1, 0, 2};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    ///TEST_F load_smask_table_from_ub
    sanitizerRecord.payload.kernelRecord.payload.loadSmaskRecord.srcMemType = MemType::UB;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[1].loc.coreId, 7);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_MTE3);
    memOpInfo.memType = MemType::UB;
    memOpInfo.alignSize = 32;
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_unary_op_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::UNARY_OP;
    record.payload.unaryOpRecord.dst = 0xaa;
    record.payload.unaryOpRecord.src = 0x55;
    record.payload.unaryOpRecord.dstBlockStride = 1;
    record.payload.unaryOpRecord.srcBlockStride = 1;
    record.payload.unaryOpRecord.dstRepeatStride = 8;
    record.payload.unaryOpRecord.srcRepeatStride = 8;
    record.payload.unaryOpRecord.location.blockId = 7;
    record.payload.unaryOpRecord.repeat = 100;
    record.payload.unaryOpRecord.dstBlockSize = 32;
    record.payload.unaryOpRecord.srcBlockSize = 16;
    record.payload.unaryOpRecord.dstBlockNum = 8;
    record.payload.unaryOpRecord.srcBlockNum = 4;
    record.payload.unaryOpRecord.vectorMask = DEFAULT_VECTOR_MASK;
    record.payload.unaryOpRecord.maskMode = MaskMode::MASK_NORM;
    record.payload.unaryOpRecord.srcDataBits = 8;
    record.payload.unaryOpRecord.dstDataBits = 8;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);

    MemOpInfo memOpInfo;
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x55, 4, 16, 1, 100, 8, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    ASSERT_EQ(events[1].loc.coreId, 7);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0xaa, 8, 32, 1, 100, 8, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_vcopy_op_record_with_mask_count_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::VCOPY_OP;
    record.blockType = BlockType::AIVEC;
    record.payload.unaryOpRecord.dst = 0xaa;
    record.payload.unaryOpRecord.src = 0x55;
    record.payload.unaryOpRecord.dstBlockStride = 1;
    record.payload.unaryOpRecord.srcBlockStride = 1;
    record.payload.unaryOpRecord.dstRepeatStride = 8;
    record.payload.unaryOpRecord.srcRepeatStride = 8;
    record.payload.unaryOpRecord.location.blockId = 7;
    record.payload.unaryOpRecord.repeat = 100;
    record.payload.unaryOpRecord.dstBlockSize = 32;
    record.payload.unaryOpRecord.srcBlockSize = 16;
    record.payload.unaryOpRecord.dstBlockNum = 2;
    record.payload.unaryOpRecord.srcBlockNum = 4;
    record.payload.unaryOpRecord.vectorMask.mask0 = 32;
    record.payload.unaryOpRecord.vectorMask.mask1 = 0;
    record.payload.unaryOpRecord.maskMode = MaskMode::MASK_COUNT;
    record.payload.unaryOpRecord.srcDataBits = 8;
    record.payload.unaryOpRecord.dstDataBits = 8;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);

    MemOpInfo memOpInfo;
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::READ, {32, 0}, MaskMode::MASK_COUNT,
        8, 0x55, 2, 16, 1, 100, 8, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    ASSERT_EQ(events[1].loc.coreId, 7);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::WRITE, {32, 0}, MaskMode::MASK_COUNT,
        8, 0xaa, 1, 32, 1, 100, 8, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_vreducev2_unary_record_with_mask_count_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::VREDUCEV2_UNARY;
    record.blockType = BlockType::AIVEC;
    record.payload.unaryOpRecord.dst = 0xaa;
    record.payload.unaryOpRecord.src = 0x55;
    record.payload.unaryOpRecord.dstBlockStride = 1;
    record.payload.unaryOpRecord.srcBlockStride = 1;
    record.payload.unaryOpRecord.dstRepeatStride = 8;
    record.payload.unaryOpRecord.srcRepeatStride = 8;
    record.payload.unaryOpRecord.location.blockId = 7;
    record.payload.unaryOpRecord.repeat = 100;
    record.payload.unaryOpRecord.dstBlockSize = 32;
    record.payload.unaryOpRecord.srcBlockSize = 16;
    record.payload.unaryOpRecord.dstBlockNum = 2;
    record.payload.unaryOpRecord.srcBlockNum = 4;
    record.payload.unaryOpRecord.vectorMask.mask0 = 32;
    record.payload.unaryOpRecord.vectorMask.mask1 = 0;
    record.payload.unaryOpRecord.maskMode = MaskMode::MASK_COUNT;
    record.payload.unaryOpRecord.srcDataBits = 8;
    record.payload.unaryOpRecord.dstDataBits = 8;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);

    MemOpInfo memOpInfo;
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::READ, {32, 0}, MaskMode::MASK_COUNT,
        8, 0x55, 2, 16, 1, 100, 8, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    ASSERT_EQ(events[1].loc.coreId, 7);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::WRITE, {32, 0}, MaskMode::MASK_COUNT,
        8, 0xaa, 1, 32 * 100, 1, 1, 8, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_vmrgsort4_op_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::VMRGSORT4_OP_M200;
    record.payload.unaryOpRecord.dst = 0xaa;
    record.payload.unaryOpRecord.src = 0x55;
    record.payload.unaryOpRecord.dstBlockStride = 1;
    record.payload.unaryOpRecord.srcBlockStride = 1;
    record.payload.unaryOpRecord.dstRepeatStride = 8;
    record.payload.unaryOpRecord.srcRepeatStride = 8;
    record.payload.unaryOpRecord.location.blockId = 7;
    record.payload.unaryOpRecord.repeat = 100;
    record.payload.unaryOpRecord.dstBlockSize = 32;
    record.payload.unaryOpRecord.srcBlockSize = 16;
    record.payload.unaryOpRecord.dstBlockNum = 8;
    record.payload.unaryOpRecord.srcBlockNum = 4;
    record.payload.unaryOpRecord.vectorMask = DEFAULT_VECTOR_MASK;
    record.payload.unaryOpRecord.maskMode = MaskMode::MASK_NORM;
    record.payload.unaryOpRecord.srcDataBits = 16;
    record.payload.unaryOpRecord.dstDataBits = 16;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[1].eventInfo.memInfo.alignSize, 16);
    record.payload.unaryOpRecord.dstDataBits = 32;
    sanitizerRecord.payload.kernelRecord = record;
    events.clear();
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[1].eventInfo.memInfo.alignSize, 32);
}

static BinaryOpRecord CreateBinaryOpRecord(void)
{
    BinaryOpRecord binaryOpRecord;
    binaryOpRecord.dst = 0xcc;
    binaryOpRecord.src0 = 0x55;
    binaryOpRecord.src1 = 0xaa;
    binaryOpRecord.dstBlockStride = 1;
    binaryOpRecord.src0BlockStride = 1;
    binaryOpRecord.src1BlockStride = 1;
    binaryOpRecord.dstRepeatStride = NORMAL_REPEAT_STRIDE;
    binaryOpRecord.src0RepeatStride = NORMAL_REPEAT_STRIDE;
    binaryOpRecord.src1RepeatStride = NORMAL_REPEAT_STRIDE;
    binaryOpRecord.location.blockId = CORE_ID;
    binaryOpRecord.repeat = REPEAT;
    binaryOpRecord.dstBlockSize = DST_BLOCK_SIZE;
    binaryOpRecord.src0BlockSize = SRC0_BLOCK_SIZE;
    binaryOpRecord.src1BlockSize = SRC1_BLOCK_SIZE;
    binaryOpRecord.dstBlockNum = DST_BLOCK_NUM;
    binaryOpRecord.src0BlockNum = SRC0_BLOCK_NUM;
    binaryOpRecord.src1BlockNum = SRC1_BLOCK_NUM;
    binaryOpRecord.vectorMask = DEFAULT_VECTOR_MASK;
    binaryOpRecord.maskMode = MaskMode::MASK_NORM;
    binaryOpRecord.src0DataBits = 8;
    binaryOpRecord.src1DataBits = 8;
    binaryOpRecord.dstDataBits = 8;
    return binaryOpRecord;
}

TEST_F(TestRecordParse, parse_binary_op_record_with_binary_op_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::BINARY_OP;
    record.payload.binaryOpRecord = CreateBinaryOpRecord();

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);

    MemOpInfo memOpInfo;
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x55, 4, 16, 1, 100, 8, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    ASSERT_EQ(events[1].loc.coreId, 7);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0xaa, 2, 8, 1, 100, 8, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);

    ASSERT_EQ(events[2].loc.coreId, 7);
    ASSERT_EQ(events[2].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[2].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0xcc, 8, 32, 1, 100, 8, 32};
    ASSERT_EQ(events[2].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_binary_op_record_with_mask_count_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::BINARY_OP;
    record.payload.binaryOpRecord = CreateBinaryOpRecord();
    record.payload.binaryOpRecord.vectorMask.mask0 = 64;
    record.payload.binaryOpRecord.vectorMask.mask1 = 0;
    record.payload.binaryOpRecord.maskMode = MaskMode::MASK_COUNT;
    record.blockType = BlockType::AIVEC;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);

    MemOpInfo memOpInfo;
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::READ, {64, 0}, MaskMode::MASK_COUNT,
        8, 0x55, 4, 16, 1, 1, 8, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    ASSERT_EQ(events[1].loc.coreId, 7);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::READ, {64, 0}, MaskMode::MASK_COUNT,
        8, 0xaa, 2, 8, 1, 4, 8, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);

    ASSERT_EQ(events[2].loc.coreId, 7);
    ASSERT_EQ(events[2].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[2].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::WRITE, {64, 0}, MaskMode::MASK_COUNT,
        8, 0xcc, 2, 32, 1, 1, 8, 32};
    ASSERT_EQ(events[2].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_cmpmask_op_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
 
    record.recordType = RecordType::CMPMASK_OP;
    record.blockType = BlockType::AIVEC;
    record.payload.cmpMaskRecord.addr = 0xaa;
    record.payload.cmpMaskRecord.size = 123;
    record.payload.cmpMaskRecord.accessType = AccessType::READ;
    record.payload.cmpMaskRecord.location.blockId = 7;
 
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.alignSize, 16);
}

TEST_F(TestRecordParse, parse_reduce_op_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::REDUCE_OP;
    record.blockType = BlockType::AIVEC;
    record.payload.reduceOpRecord.dst = 0xaa;
    record.payload.reduceOpRecord.src = 0x55;
    record.payload.reduceOpRecord.srcBlockStride = 3;
    record.payload.reduceOpRecord.dstRepeatStride = 8;
    record.payload.reduceOpRecord.srcRepeatStride = 8;
    record.payload.reduceOpRecord.dstRepeatLength = 4;
    record.payload.reduceOpRecord.location.blockId = 7;
    record.payload.reduceOpRecord.repeat = 100;
    record.payload.reduceOpRecord.dstBlockSize = 4;
    record.payload.reduceOpRecord.srcBlockSize = 16;
    record.payload.reduceOpRecord.dstBlockNum = 1;
    record.payload.reduceOpRecord.srcBlockNum = 4;
    record.payload.reduceOpRecord.srcDataBits = 8;
    record.payload.reduceOpRecord.dstDataBits = 8;
    record.payload.reduceOpRecord.vectorMask = DEFAULT_VECTOR_MASK;
    record.payload.reduceOpRecord.maskMode = MaskMode::MASK_NORM;
    record.payload.reduceOpRecord.dstAlignSize = 32;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);

    MemOpInfo memOpInfo;
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x55, 4, 16, 3, 100, 8, 32};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    ASSERT_EQ(events[1].loc.coreId, 7);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_V);
    memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0xaa, 1, 4, 1, 100, 8, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);

    sanitizerRecord.payload.kernelRecord.payload.reduceOpRecord.maskMode = MaskMode::MASK_COUNT;
    sanitizerRecord.payload.kernelRecord.payload.reduceOpRecord.dstDataBitsFactor = 1;
    sanitizerRecord.payload.kernelRecord.payload.reduceOpRecord.vectorMask = VectorMask{256, 0};
    events.clear();
    RecordParse::Parse(sanitizerRecord, events);

    memOpInfo = {MemType::UB, AccessType::WRITE, {16, 0}, MaskMode::MASK_COUNT,
        8, 0xaa, 1, 4, 1, 4, 8, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}

constexpr uint16_t MATRIX_RECORD_DST_BLOCKSIZE = 12;
constexpr uint16_t MATRIX_RECORD_SRC0_BLOCKSIZE = 32;
constexpr uint16_t MATRIX_RECORD_SRC1_BLOCKSIZE = 48;
constexpr uint16_t MATRIX_RECORD_DST_BLOCKNUM = 2;
constexpr uint16_t MATRIX_RECORD_SRC0_BLOCKNUM = 4;
constexpr uint16_t MATRIX_RECORD_SRC1_BLOCKNUM = 8;
constexpr uint16_t MATRIX_RECORD_SRC0_REPEATSTRIDE = 18;
constexpr uint16_t MATRIX_RECORD_SRC0_REPEAT = 2;
constexpr uint16_t MATRIX_RECORD_COREID = 7;

static MarixMulOpRecord CreateMatrixMulOpRecord(void)
{
    MarixMulOpRecord matrixMulOpRecord;
    matrixMulOpRecord.dst = 0xcc;
    matrixMulOpRecord.src0 = 0x55;
    matrixMulOpRecord.src1 = 0xaa;
    matrixMulOpRecord.dstBlockSize = MATRIX_RECORD_DST_BLOCKSIZE;
    matrixMulOpRecord.src0BlockSize = MATRIX_RECORD_SRC0_BLOCKSIZE;
    matrixMulOpRecord.src1BlockSize = MATRIX_RECORD_SRC1_BLOCKSIZE;
    matrixMulOpRecord.dstBlockNum = MATRIX_RECORD_DST_BLOCKNUM;
    matrixMulOpRecord.src0BlockNum = MATRIX_RECORD_SRC0_BLOCKNUM;
    matrixMulOpRecord.src1BlockNum = MATRIX_RECORD_SRC1_BLOCKNUM;
    matrixMulOpRecord.src0RepeatStride = MATRIX_RECORD_SRC0_REPEATSTRIDE;
    matrixMulOpRecord.location.blockId = MATRIX_RECORD_COREID;
    matrixMulOpRecord.src0Repeat = MATRIX_RECORD_SRC0_REPEAT;
    matrixMulOpRecord.cmatrixInitVal = 0;
    matrixMulOpRecord.cmatrixSource = 0;
    matrixMulOpRecord.src0AlignSize = 512;
    matrixMulOpRecord.src1AlignSize = 512;
    matrixMulOpRecord.dstAlignSize = 512;
    matrixMulOpRecord.enUnitFlag = 0;
    return matrixMulOpRecord;
}

TEST_F(TestRecordParse, parse_matrix_mul_l0c_op_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::MATRIX_MUL_OP;
    record.payload.matrixMulOpRecord = CreateMatrixMulOpRecord();
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);

    MemOpInfo memOpInfo;
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_M);
    memOpInfo = {MemType::L0A, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x55, 4, 32, 1, 2, 18, 512};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    ASSERT_EQ(events[1].loc.coreId, 7);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_M);
    memOpInfo = {MemType::L0B, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0xaa, 8, 48, 1, 1, 0, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
    ASSERT_EQ(events[2].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[2].pipe, PipeType::PIPE_M);
    memOpInfo = {MemType::L0C, AccessType::MEMCPY_BLOCKS, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0xcc, 2, 12, 1, 1, 0, 512};
    ASSERT_EQ(events[2].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_matrix_mul_btb_op_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::MATRIX_MUL_OP;
    record.payload.matrixMulOpRecord = CreateMatrixMulOpRecord();
    record.payload.matrixMulOpRecord.cmatrixInitVal = 1;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);

    MemOpInfo memOpInfo;
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_M);
    memOpInfo = {MemType::L0A, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x55, 4, 32, 1, 2, 18, 512};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);

    ASSERT_EQ(events[1].loc.coreId, 7);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_M);
    memOpInfo = {MemType::L0B, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0xaa, 8, 48, 1, 1, 0, 512};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
    ASSERT_EQ(events[2].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[2].pipe, PipeType::PIPE_M);
    memOpInfo = {MemType::L0C, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0xcc, 2, 12, 1, 1, 0, 512};
    ASSERT_EQ(events[2].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_vconcat_record)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    constexpr uint64_t src = 0x100;
    constexpr uint64_t dst = 0x500;
    constexpr uint8_t coreID = 7;
    constexpr uint8_t regionRange = 5;
    constexpr uint8_t repeat = 3;
    constexpr uint8_t ppsNum = 16;
    constexpr uint8_t eleNum = 8;
    constexpr uint8_t expectEventNum = 2;
    constexpr DataType dataType = DataType::DATA_B16;
    constexpr uint8_t eleByte = (dataType == DataType::DATA_B16 ? 2 : 4);

    record.recordType = RecordType::VEC_REGPROPCOOR_OP;
    record.payload.vecRegPropCoordOpRecord.location.blockId = coreID;
    record.payload.vecRegPropCoordOpRecord.dst = dst;
    record.payload.vecRegPropCoordOpRecord.src = src;
    record.payload.vecRegPropCoordOpRecord.regionRange = 7;
    record.payload.vecRegPropCoordOpRecord.repeat = repeat;
    record.payload.vecRegPropCoordOpRecord.dataType = dataType;
    record.payload.vecRegPropCoordOpRecord.isExtract = false;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
    sanitizerRecord.payload.kernelRecord.payload.vecRegPropCoordOpRecord.regionRange = regionRange;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), expectEventNum);
    ASSERT_EQ(events[0].loc.coreId, coreID);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    ASSERT_EQ(events[0].eventInfo.memInfo.opType, AccessType::READ);
    ASSERT_EQ(events[0].eventInfo.memInfo.addr, src);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockNum, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockSize, eleByte * ppsNum);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatStride, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatTimes, repeat);
    ASSERT_EQ(events[1].eventInfo.memInfo.opType, AccessType::WRITE);
    ASSERT_EQ(events[1].eventInfo.memInfo.addr, dst + regionRange * eleByte);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockNum, ppsNum);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockSize, eleByte);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockStride, eleNum);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatStride, eleNum * ppsNum);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatTimes, repeat);
}

TEST_F(TestRecordParse, parse_vextract_record)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    constexpr uint64_t src = 0x100;
    constexpr uint64_t dst = 0x500;
    constexpr uint8_t coreID = 7;
    constexpr uint8_t regionRange = 5;
    constexpr uint8_t repeat = 3;
    constexpr uint8_t ppsNum = 16;
    constexpr uint8_t eleNum = 8;
    constexpr uint8_t expectEventNum = 2;
    constexpr DataType dataType = DataType::DATA_B16;
    constexpr uint8_t eleByte = (dataType == DataType::DATA_B16 ? 2 : 4);

    record.recordType = RecordType::VEC_REGPROPCOOR_OP;
    record.payload.vecRegPropCoordOpRecord.location.blockId = coreID;
    record.payload.vecRegPropCoordOpRecord.dst = dst;
    record.payload.vecRegPropCoordOpRecord.src = src;
    record.payload.vecRegPropCoordOpRecord.regionRange = regionRange;
    record.payload.vecRegPropCoordOpRecord.repeat = repeat;
    record.payload.vecRegPropCoordOpRecord.dataType = dataType;
    record.payload.vecRegPropCoordOpRecord.isExtract = true;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), expectEventNum);
    ASSERT_EQ(events[0].loc.coreId, coreID);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    ASSERT_EQ(events[0].eventInfo.memInfo.opType, AccessType::WRITE);
    ASSERT_EQ(events[0].eventInfo.memInfo.addr, src);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockNum, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockSize, eleByte * ppsNum);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatStride, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatTimes, repeat);
    ASSERT_EQ(events[1].eventInfo.memInfo.opType, AccessType::READ);
    ASSERT_EQ(events[1].eventInfo.memInfo.addr, dst + regionRange * eleByte);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockNum, ppsNum);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockSize, eleByte);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockStride, eleNum);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatStride, eleNum * ppsNum);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatTimes, repeat);
}

TEST_F(TestRecordParse, parse_set_flag_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::SET_FLAG;
    record.payload.syncRecord.location.blockId = 7;
    record.payload.syncRecord.src = PipeType::PIPE_MTE1;
    record.payload.syncRecord.dst = PipeType::PIPE_V;
    record.payload.syncRecord.eventID = 1;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);

    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::SYNC_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    ASSERT_EQ(events[0].eventInfo.syncInfo.opType, SyncType::SET_FLAG);
    ASSERT_EQ(events[0].eventInfo.syncInfo.srcPipe, PipeType::PIPE_MTE1);
    ASSERT_EQ(events[0].eventInfo.syncInfo.dstPipe, PipeType::PIPE_V);
    ASSERT_EQ(events[0].eventInfo.syncInfo.eventId, 1);
    ASSERT_EQ(events[0].eventInfo.syncInfo.memType, MemType::INVALID);
    ASSERT_EQ(events[0].eventInfo.syncInfo.isRetrogress, false);
}

TEST_F(TestRecordParse, parse_wait_flag_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::WAIT_FLAG;
    record.payload.syncRecord.location.blockId = 7;
    record.payload.syncRecord.src = PipeType::PIPE_MTE1;
    record.payload.syncRecord.dst = PipeType::PIPE_V;
    record.payload.syncRecord.eventID = 1;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);

    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::SYNC_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    ASSERT_EQ(events[0].eventInfo.syncInfo.opType, SyncType::WAIT_FLAG);
    ASSERT_EQ(events[0].eventInfo.syncInfo.srcPipe, PipeType::PIPE_MTE1);
    ASSERT_EQ(events[0].eventInfo.syncInfo.dstPipe, PipeType::PIPE_V);
    ASSERT_EQ(events[0].eventInfo.syncInfo.eventId, 1);
    ASSERT_EQ(events[0].eventInfo.syncInfo.memType, MemType::INVALID);
    ASSERT_EQ(events[0].eventInfo.syncInfo.isRetrogress, false);
}

TEST_F(TestRecordParse, parse_pipe_barrier_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::PIPE_BARRIER;
    record.payload.pipeBarrierRecord.location.blockId = 7;
    record.payload.pipeBarrierRecord.pipe = PipeType::PIPE_ALL;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 34);
    ASSERT_EQ(events[27].loc.coreId, 7);
    ASSERT_EQ(events[27].type, EventType::SYNC_EVENT);
    ASSERT_EQ(events[27].pipe, PipeType::PIPE_FIX);
    ASSERT_EQ(events[27].eventInfo.syncInfo.opType, SyncType::PIPE_BARRIER);
    ASSERT_EQ(events[27].eventInfo.syncInfo.memType, MemType::INVALID);
    ASSERT_EQ(events[27].eventInfo.syncInfo.isRetrogress, false);

    ASSERT_EQ(events[28].loc.coreId, 7);
    ASSERT_EQ(events[28].type, EventType::SYNC_EVENT);
    ASSERT_EQ(events[28].pipe, PipeType::PIPE_FIX);
    ASSERT_EQ(events[28].eventInfo.syncInfo.opType, SyncType::SET_FLAG);
    ASSERT_EQ(events[28].eventInfo.syncInfo.srcPipe, PipeType::PIPE_FIX);
    ASSERT_EQ(events[28].eventInfo.syncInfo.dstPipe, PipeType::PIPE_S);
    ASSERT_EQ(events[28].eventInfo.syncInfo.eventId, 11);
    ASSERT_EQ(events[28].eventInfo.syncInfo.memType, MemType::INVALID);
    ASSERT_EQ(events[28].eventInfo.syncInfo.isRetrogress, false);

    ASSERT_EQ(events[29].loc.coreId, 7);
    ASSERT_EQ(events[29].type, EventType::SYNC_EVENT);
    ASSERT_EQ(events[29].pipe, PipeType::PIPE_S);
    ASSERT_EQ(events[29].eventInfo.syncInfo.opType, SyncType::WAIT_FLAG);
    ASSERT_EQ(events[29].eventInfo.syncInfo.srcPipe, PipeType::PIPE_FIX);
    ASSERT_EQ(events[29].eventInfo.syncInfo.dstPipe, PipeType::PIPE_S);
    ASSERT_EQ(events[29].eventInfo.syncInfo.eventId, 11);
    ASSERT_EQ(events[29].eventInfo.syncInfo.memType, MemType::INVALID);
    ASSERT_EQ(events[29].eventInfo.syncInfo.isRetrogress, false);
}

TEST_F(TestRecordParse, parse_ffts_sync_record_expect_success_and_correct)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::FFTS_SYNC;
    record.payload.fftsSyncRecord.dst = PipeType::PIPE_MTE1;
    record.payload.fftsSyncRecord.location.blockId = 7;
    record.payload.fftsSyncRecord.mode = 0;
    record.payload.fftsSyncRecord.flagID = 1;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::CROSS_CORE_SYNC_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    ASSERT_EQ(events[0].eventInfo.fftsSyncInfo.opType, SyncType::FFTS_SYNC);
    ASSERT_EQ(events[0].eventInfo.fftsSyncInfo.dstPipe, PipeType::PIPE_MTE1);
    ASSERT_EQ(events[0].eventInfo.fftsSyncInfo.flagId, 1);
    ASSERT_EQ(events[0].eventInfo.fftsSyncInfo.mode, 0);

    ASSERT_EQ(events[1].loc.coreId, 7);
    ASSERT_EQ(events[1].type, EventType::SYNC_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_MTE1);
    ASSERT_EQ(events[1].eventInfo.syncInfo.opType, SyncType::PIPE_BARRIER);
}

TEST_F(TestRecordParse, parse_sync_all_record_expect_success_and_correct)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::SYNC_ALL_STUB;
    record.payload.softSyncRecord.location.blockId = 7;
    record.payload.softSyncRecord.usedCores = 2;
    record.payload.softSyncRecord.isAIVOnly = true;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::CROSS_CORE_SYNC_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_S_CAL);
    ASSERT_EQ(events[0].eventInfo.fftsSyncInfo.opType, SyncType::FFTS_SYNC);
    ASSERT_EQ(events[0].eventInfo.fftsSyncInfo.dstPipe, PipeType::PIPE_S_CAL);
    ASSERT_EQ(events[0].eventInfo.fftsSyncInfo.mode, 0);
}

TEST_F(TestRecordParse, parse_wait_flag_dev_record_expect_success_and_correct)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::WAIT_FLAG_DEV;
    record.payload.waitFlagDevRecord.location.blockId = 7;
    record.payload.waitFlagDevRecord.flagID = 1;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].type, EventType::CROSS_CORE_SYNC_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_S);
    ASSERT_EQ(events[0].eventInfo.fftsSyncInfo.opType, SyncType::WAIT_FLAG_DEV);
    ASSERT_EQ(events[0].eventInfo.fftsSyncInfo.dstPipe, PipeType::PIPE_S);
    ASSERT_EQ(events[0].eventInfo.fftsSyncInfo.flagId, 1);
}

TEST_F(TestRecordParse, parse_hsync_record_with_mode_v_1_expect_no_set_wait_sync_event)
{
    RecordParse::ResetAll();
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::HSET_FLAG;
    record.blockType = BlockType::AICUBE;
    record.payload.hardSyncRecord.location.blockId = 7;
    record.payload.hardSyncRecord.src = PipeType::PIPE_MTE1;
    record.payload.hardSyncRecord.dst = PipeType::PIPE_M;
    record.payload.hardSyncRecord.eventID = 1;
    record.payload.hardSyncRecord.memory = MemType::L0A;
    record.payload.hardSyncRecord.v = 1;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    record.recordType = RecordType::HWAIT_FLAG;
    record.payload.hardSyncRecord.v = 0;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);

    ASSERT_EQ(events[0].loc.coreId, 7);
    ASSERT_EQ(events[0].type, EventType::H_SYNC_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    ASSERT_EQ(events[0].eventInfo.hsyncInfo.opType, SyncType::HSET_FLAG);
    ASSERT_EQ(events[0].eventInfo.hsyncInfo.srcPipe, PipeType::PIPE_MTE1);
    ASSERT_EQ(events[0].eventInfo.hsyncInfo.dstPipe, PipeType::PIPE_M);
    ASSERT_EQ(events[0].eventInfo.hsyncInfo.eventId, 1);
    ASSERT_EQ(events[0].eventInfo.hsyncInfo.memType, MemType::L0A);
    ASSERT_EQ(events[1].eventInfo.hsyncInfo.opType, SyncType::HWAIT_FLAG);
    ASSERT_EQ(events[1].eventInfo.hsyncInfo.srcPipe, PipeType::PIPE_MTE1);
    ASSERT_EQ(events[1].eventInfo.hsyncInfo.dstPipe, PipeType::PIPE_M);
    ASSERT_EQ(events[1].eventInfo.hsyncInfo.eventId, 1);
    ASSERT_EQ(events[1].eventInfo.hsyncInfo.memType, MemType::L0A);
}

TEST_F(TestRecordParse, parse_atomic_record)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    KernelRecord atomicRecord;

    record.recordType = RecordType::STORE;
    record.payload.loadStoreRecord.location.blockId = 7;
    record.payload.loadStoreRecord.addr = 0x12;
    record.payload.loadStoreRecord.size = 100;
    record.payload.loadStoreRecord.space = AddressSpace::GM;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_FALSE(events[0].isAtomicMode);

    atomicRecord.recordType = RecordType::SET_ATOMIC;
    atomicRecord.payload.atomicModeRecord.location.blockId = 7;
    atomicRecord.payload.atomicModeRecord.mode = AtomicMode::F16;
    sanitizerRecord.payload.kernelRecord = atomicRecord;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);

    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_TRUE(events[1].isAtomicMode);

    atomicRecord.payload.atomicModeRecord.mode = AtomicMode::NONE;
    sanitizerRecord.payload.kernelRecord = atomicRecord;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);

    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);
    ASSERT_FALSE(events[2].isAtomicMode);
}

TEST_F(TestRecordParse, parse_mstx_hccl_record_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::MSTX_STUB;
    record.payload.mstxRecord.interfaceType = InterfaceType::MSTX_HCCL;
    record.payload.mstxRecord.bufferLens = sizeof(MstxHcclRecord);
    record.payload.mstxRecord.location.blockId = 0;
    record.payload.mstxRecord.error = false;

    auto &hcclRecord = record.payload.mstxRecord.interface.mstxHcclRecord;
    hcclRecord.src = 0x12c041200000;
    hcclRecord.dst = 0x12c042600000;
    hcclRecord.srcCount = 4587520;
    hcclRecord.dstCount = 2293760;
    hcclRecord.srcStride = 16;
    hcclRecord.dstStride = 4;
    hcclRecord.srcRepeatStride = 1;
    hcclRecord.dstRepeatStride = 1;
    hcclRecord.srcDataTypeSize = 1;
    hcclRecord.dstDataTypeSize = 1;
    hcclRecord.repeat = 1;
    hcclRecord.rankDim = 8;
    hcclRecord.flagId = 3; // AlltoAll接口

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 16); // 读事件8个，写事件8个
    // 第一个读事件
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].eventInfo.memInfo.addr, 0x12c041200000);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockNum, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockSize, 4587520);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockStride, 0);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatTimes, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatStride, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.memType, MemType::GM);
    // 第二个读事件
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    // 20616935637014转换为十六进制是12C0 4120 0016，即原本的dst地址偏移16个字节
    ASSERT_EQ(events[1].eventInfo.memInfo.addr, 20616935637008);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockNum, 1);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockSize, 4587520);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockStride, 0);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatTimes, 1);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatStride, 1);
    ASSERT_EQ(events[1].eventInfo.memInfo.memType, MemType::GM);
    // 第一个写事件
    ASSERT_EQ(events[8].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[8].eventInfo.memInfo.addr, 0x12c042600000);
    ASSERT_EQ(events[8].eventInfo.memInfo.blockNum, 1);
    ASSERT_EQ(events[8].eventInfo.memInfo.blockSize, 2293760);
    ASSERT_EQ(events[8].eventInfo.memInfo.blockStride, 0);
    ASSERT_EQ(events[8].eventInfo.memInfo.repeatTimes, 1);
    ASSERT_EQ(events[8].eventInfo.memInfo.repeatStride, 1);
    const size_t eventIdx2 = 8;
    ASSERT_EQ(events[eventIdx2].eventInfo.memInfo.memType, MemType::GM);
    // 第二个写事件
    ASSERT_EQ(events[9].type, EventType::MEM_EVENT);
    // 20616956608516转换为十六进制是12C0 4260 0004，即原本的dst地址偏移4个字节
    ASSERT_EQ(events[9].eventInfo.memInfo.addr, 20616956608516);
    ASSERT_EQ(events[9].eventInfo.memInfo.blockNum, 1);
    ASSERT_EQ(events[9].eventInfo.memInfo.blockSize, 2293760);
    ASSERT_EQ(events[9].eventInfo.memInfo.blockStride, 0);
    ASSERT_EQ(events[9].eventInfo.memInfo.repeatTimes, 1);
    ASSERT_EQ(events[9].eventInfo.memInfo.repeatStride, 1);
    const size_t eventIdx3 = 9;
    ASSERT_EQ(events[eventIdx3].eventInfo.memInfo.memType, MemType::GM);
}

TEST_F(TestRecordParse, parse_mstx_cross_record_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::MSTX_STUB;
    record.payload.mstxRecord.interfaceType = InterfaceType::MSTX_SET_CROSS_SYNC;
    record.payload.mstxRecord.bufferLens = sizeof(MstxCrossRecord);
    record.payload.mstxRecord.location.blockId = 7;
    record.payload.mstxRecord.error = false;

    auto &crossRecord = record.payload.mstxRecord.interface.mstxCrossRecord;
    crossRecord.addr = 0x12;
    crossRecord.flagId = 12;
    crossRecord.pipe = PipeType::PIPE_MTE1;
    crossRecord.isMore = false;
    crossRecord.isMerge = true;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_MTE1);
    ASSERT_EQ(events[0].type, EventType::MSTX_CROSS_SYNC_EVENT);
    ASSERT_EQ(events[0].eventInfo.mstxCrossInfo.addr, 0x12);
    ASSERT_EQ(events[0].eventInfo.mstxCrossInfo.flagId, 12);
    ASSERT_EQ(events[0].eventInfo.mstxCrossInfo.isMore, false);
    ASSERT_EQ(events[0].eventInfo.mstxCrossInfo.isMerge, true);
    ASSERT_EQ(events[0].eventInfo.mstxCrossInfo.opType, SyncType::MSTX_SET_CROSS);

    record.payload.mstxRecord.interfaceType = InterfaceType::MSTX_WAIT_CROSS_SYNC;
    crossRecord.isMore = true;
    crossRecord.isMerge = false;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[1].type, EventType::MSTX_CROSS_SYNC_EVENT);
    ASSERT_EQ(events[1].eventInfo.mstxCrossInfo.isMore, true);
    ASSERT_EQ(events[1].eventInfo.mstxCrossInfo.isMerge, false);
    ASSERT_EQ(events[1].eventInfo.mstxCrossInfo.opType, SyncType::MSTX_WAIT_CROSS);
}

TEST_F(TestRecordParse, parse_mstx_cross_error_record_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    record.recordType = RecordType::MSTX_STUB;
    record.payload.mstxRecord.interfaceType = InterfaceType::MSTX_SET_CROSS_SYNC;
    record.payload.mstxRecord.bufferLens = sizeof(MstxCrossRecord);
    record.payload.mstxRecord.location.blockId = 7;
    record.payload.mstxRecord.error = true;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
}

TEST_F(TestRecordParse, parse_mstx_signal_set_record_expect_get_correct_events)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    MstxSignalSet mstxSignalSet{};
    mstxSignalSet.addr = 0x1234;
    mstxSignalSet.value = 100;

    record.recordType = RecordType::MSTX_STUB;
    record.payload.mstxRecord.interfaceType = InterfaceType::MSTX_SIGNAL_SET;
    record.payload.mstxRecord.bufferLens = sizeof(mstxSignalSet);
    record.payload.mstxRecord.interface.mstxSignalSet = mstxSignalSet;
    record.payload.mstxRecord.location.blockId = 7;
    record.payload.mstxRecord.error = false;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].type, EventType::MSTX_SIGNAL_SET_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_S_CAL);
    ASSERT_EQ(events[0].eventInfo.mstxSignalSet.addr, mstxSignalSet.addr);
    ASSERT_EQ(events[0].eventInfo.mstxSignalSet.value, mstxSignalSet.value);
}

TEST_F(TestRecordParse, parse_mstx_signal_wait_record_expect_get_correct_events)
{
    std::vector<SanEvent> events;
    KernelRecord record{};

    MstxSignalWait mstxSignalWait{};
    mstxSignalWait.addr = 0x1234;
    mstxSignalWait.cmpValue = 100;
    mstxSignalWait.cmpOp = CompareOp::EQ;

    record.recordType = RecordType::MSTX_STUB;
    record.payload.mstxRecord.interfaceType = InterfaceType::MSTX_SIGNAL_WAIT;
    record.payload.mstxRecord.bufferLens = sizeof(mstxSignalWait);
    record.payload.mstxRecord.interface.mstxSignalWait = mstxSignalWait;
    record.payload.mstxRecord.location.blockId = 7;
    record.payload.mstxRecord.error = false;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].type, EventType::MSTX_SIGNAL_WAIT_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_S);
    ASSERT_EQ(events[0].eventInfo.mstxSignalWait.addr, mstxSignalWait.addr);
    ASSERT_EQ(events[0].eventInfo.mstxSignalWait.cmpOp, mstxSignalWait.cmpOp);
    ASSERT_EQ(events[0].eventInfo.mstxSignalWait.cmpValue, mstxSignalWait.cmpValue);
}

TEST_F(TestRecordParse, parse_wait_flag_and_hwait_flag_end_to_end_in_pipe_record_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    RecordParse::ResetAll();

    record.recordType = RecordType::WAIT_FLAG;
    record.payload.syncRecord.location.blockId = 0;
    record.payload.syncRecord.src = PipeType::PIPE_MTE1;
    record.payload.syncRecord.dst = PipeType::PIPE_MTE2;
    record.payload.syncRecord.eventID = 0;
    record.blockType = BlockType::AICUBE;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);

    record.payload.syncRecord.src = PipeType::PIPE_MTE2;
    record.payload.syncRecord.dst = PipeType::PIPE_MTE3;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);

    record.payload.syncRecord.src = PipeType::PIPE_MTE3;
    record.payload.syncRecord.dst = PipeType::PIPE_MTE4;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);

    record.payload.syncRecord.src = PipeType::PIPE_MTE4;
    record.payload.syncRecord.dst = PipeType::PIPE_MTE1;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 5);

    record.recordType = RecordType::HWAIT_FLAG;
    record.payload.hardSyncRecord.location.blockId = 0;
    record.payload.hardSyncRecord.src = PipeType::PIPE_MTE1;
    record.payload.hardSyncRecord.dst = PipeType::PIPE_FIX;
    record.payload.hardSyncRecord.eventID = 0;
    record.payload.hardSyncRecord.memory = MemType::L0A;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 6);

    record.payload.hardSyncRecord.src = PipeType::PIPE_FIX;
    record.payload.hardSyncRecord.dst = PipeType::PIPE_MTE1;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 7);
}

TEST_F(TestRecordParse, parse_hset_hwait_flag_with_mte1_record_expect_success)
{
    RecordParse::ResetAll();
    std::vector<SanEvent> events;
    KernelRecord record{};
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    RecordParse::ResetSyncInPipeInfo();

    record.recordType = RecordType::HSET_FLAG;
    record.blockType = BlockType::AICUBE;
    record.payload.hardSyncRecord.location.blockId = 0;
    record.payload.hardSyncRecord.src = PipeType::PIPE_MTE1;
    record.payload.hardSyncRecord.dst = PipeType::PIPE_FIX;
    record.payload.hardSyncRecord.memory = MemType::L1;
    record.payload.hardSyncRecord.eventID = 1;
    record.payload.hardSyncRecord.v = 0;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);

    record.recordType = RecordType::LOAD_L1_MX_2D;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);
    ASSERT_EQ(events[2].type, EventType::SYNC_EVENT);
    ASSERT_EQ(events[2].eventInfo.syncInfo.opType, SyncType::SET_FLAG);
    ASSERT_EQ(events[2].eventInfo.syncInfo.srcPipe, PipeType::PIPE_MTE1);
    ASSERT_EQ(events[2].eventInfo.syncInfo.dstPipe, PipeType::PIPE_FIX);
    ASSERT_EQ(events[2].eventInfo.syncInfo.eventId, 120 + 1);

    record.recordType = RecordType::HWAIT_FLAG;
    record.payload.hardSyncRecord.src = PipeType::PIPE_MTE1;
    record.payload.hardSyncRecord.dst = PipeType::PIPE_FIX;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 5);
    ASSERT_EQ(events[4].type, EventType::SYNC_EVENT);
    ASSERT_EQ(events[4].eventInfo.syncInfo.opType, SyncType::WAIT_FLAG);
    ASSERT_EQ(events[4].eventInfo.syncInfo.srcPipe, PipeType::PIPE_MTE1);
    ASSERT_EQ(events[4].eventInfo.syncInfo.dstPipe, PipeType::PIPE_FIX);
    ASSERT_EQ(events[4].eventInfo.syncInfo.eventId, 120 + 1);
}

TEST_F(TestRecordParse, parse_cycle_cross_pipe_event_with_two_sync_expect_get_inner_pipe_event_success)
{
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;
    KernelRecord record{};
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
 
    record.recordType = RecordType::SET_FLAG;
    record.payload.syncRecord.location.blockId = 0;
    record.payload.syncRecord.src = PipeType::PIPE_V;
    record.payload.syncRecord.dst = PipeType::PIPE_MTE2;
    record.payload.syncRecord.eventID = 0;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    record.recordType = RecordType::WAIT_FLAG;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
 
    record.recordType = RecordType::SET_FLAG;
    record.payload.syncRecord.src = PipeType::PIPE_MTE2;
    record.payload.syncRecord.dst = PipeType::PIPE_V;
    record.payload.syncRecord.eventID = 5;
    RecordParse::Parse(sanitizerRecord, events);
    record.recordType = RecordType::WAIT_FLAG;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    const size_t targetEventSize = 5;
    ASSERT_EQ(events.size(), targetEventSize);
    size_t lastEventIdx = events.size() - 1;
    ASSERT_EQ(events[lastEventIdx].type, EventType::SYNC_EVENT);
    ASSERT_EQ(events[lastEventIdx].pipe, PipeType::PIPE_V);
    ASSERT_EQ(events[lastEventIdx].eventInfo.syncInfo.opType, SyncType::PIPE_BARRIER);
    RecordParse::ResetSyncInPipeInfo();
}
 
TEST_F(TestRecordParse, parse_cycle_cross_pipe_event_with_three_sync_expect_get_inner_pipe_event_success)
{
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;
    KernelRecord record{};
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
 
    record.recordType = RecordType::SET_FLAG;
    record.payload.syncRecord.location.blockId = 0;
    record.payload.syncRecord.src = PipeType::PIPE_V;
    record.payload.syncRecord.dst = PipeType::PIPE_MTE2;
    record.payload.syncRecord.eventID = 3;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    record.recordType = RecordType::WAIT_FLAG;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
 
    record.recordType = RecordType::SET_FLAG;
    record.payload.syncRecord.src = PipeType::PIPE_MTE2;
    record.payload.syncRecord.dst = PipeType::PIPE_MTE3;
    record.payload.syncRecord.eventID = 5;
    RecordParse::Parse(sanitizerRecord, events);
    record.recordType = RecordType::WAIT_FLAG;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
 
    record.recordType = RecordType::SET_FLAG;
    record.payload.syncRecord.src = PipeType::PIPE_MTE3;
    record.payload.syncRecord.dst = PipeType::PIPE_V;
    record.payload.syncRecord.eventID = 6;
    RecordParse::Parse(sanitizerRecord, events);
    record.recordType = RecordType::WAIT_FLAG;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
 
    const size_t targetEventSize = 7;
    ASSERT_EQ(events.size(), targetEventSize);
    size_t lastEventIdx = events.size() - 1;
    ASSERT_EQ(events[lastEventIdx].type, EventType::SYNC_EVENT);
    ASSERT_EQ(events[lastEventIdx].pipe, PipeType::PIPE_V);
    ASSERT_EQ(events[lastEventIdx].eventInfo.syncInfo.opType, SyncType::PIPE_BARRIER);
    RecordParse::ResetSyncInPipeInfo();
}
 
TEST_F(TestRecordParse, parse_cycle_cross_pipe_event_with_pipe_s_sync_expect_get_inner_pipe_event_success)
{
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;
    KernelRecord record{};
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
 
    record.recordType = RecordType::SET_FLAG;
    record.payload.syncRecord.location.blockId = 0;
    record.payload.syncRecord.src = PipeType::PIPE_MTE2;
    record.payload.syncRecord.dst = PipeType::PIPE_S;
    record.payload.syncRecord.eventID = 2;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    record.recordType = RecordType::WAIT_FLAG;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
 
    const size_t targetEventSize = 3;
    ASSERT_EQ(events.size(), targetEventSize);
    size_t lastEventIdx = events.size() - 1;
    ASSERT_EQ(events[lastEventIdx].type, EventType::SYNC_EVENT);
    ASSERT_EQ(events[lastEventIdx].pipe, PipeType::PIPE_MTE2);
    ASSERT_EQ(events[lastEventIdx].eventInfo.syncInfo.opType, SyncType::PIPE_BARRIER);
    RecordParse::ResetSyncInPipeInfo();
}
 
TEST_F(TestRecordParse,
    parse_cycle_cross_pipe_event_with_three_event_pipe_s_sync_expect_get_inner_pipe_event_success)
{
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;
    KernelRecord record{};
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
 
    record.recordType = RecordType::SET_FLAG;
    record.payload.syncRecord.location.blockId = 0;
    record.payload.syncRecord.src = PipeType::PIPE_V;
    record.payload.syncRecord.dst = PipeType::PIPE_MTE1;
    record.payload.syncRecord.eventID = 2;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
    record.recordType = RecordType::WAIT_FLAG;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
 
    record.recordType = RecordType::SET_FLAG;
    record.payload.syncRecord.src = PipeType::PIPE_MTE1;
    record.payload.syncRecord.dst = PipeType::PIPE_MTE2;
    record.payload.syncRecord.eventID = 0;
    RecordParse::Parse(sanitizerRecord, events);
    record.recordType = RecordType::WAIT_FLAG;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
 
    record.recordType = RecordType::SET_FLAG;
    record.payload.syncRecord.src = PipeType::PIPE_MTE2;
    record.payload.syncRecord.dst = PipeType::PIPE_S;
    record.payload.syncRecord.eventID = 7;
    RecordParse::Parse(sanitizerRecord, events);
    record.recordType = RecordType::WAIT_FLAG;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
 
    const size_t targetEventSize = 9;
    ASSERT_EQ(events.size(), targetEventSize);
    size_t lastEventIdx = events.size() - 1;
    size_t pipevEventIdx = lastEventIdx - 2;
    ASSERT_EQ(events[pipevEventIdx].type, EventType::SYNC_EVENT);
    ASSERT_EQ(events[pipevEventIdx].pipe, PipeType::PIPE_V);
    ASSERT_EQ(events[pipevEventIdx].eventInfo.syncInfo.opType, SyncType::PIPE_BARRIER);
    ASSERT_EQ(events[lastEventIdx-1].type, EventType::SYNC_EVENT);
    // ASSERT_EQ(events[lastEventIdx-1].pipe, PipeType::PIPE_MTE1);
    ASSERT_EQ(events[lastEventIdx-1].eventInfo.syncInfo.opType, SyncType::PIPE_BARRIER);
    ASSERT_EQ(events[lastEventIdx].type, EventType::SYNC_EVENT);
    // ASSERT_EQ(events[lastEventIdx].pipe, PipeType::PIPE_MTE2);
    ASSERT_EQ(events[lastEventIdx].eventInfo.syncInfo.opType, SyncType::PIPE_BARRIER);
    ASSERT_EQ(events[lastEventIdx].type, EventType::SYNC_EVENT);
    RecordParse::ResetSyncInPipeInfo();
}

TEST_F(TestRecordParse, parse_record_mov_align_v2_expect_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::MOV_ALIGN_V2;
    auto &movAlignRecord = record.payload.movAlignRecordV2;
    movAlignRecord.dst = 0;
    movAlignRecord.src = 0x100;
    movAlignRecord.loop1DstStride = 256;
    movAlignRecord.loop2DstStride = 512;
    movAlignRecord.dstStride = 256;
    movAlignRecord.srcStride = 32;
    movAlignRecord.loop1SrcStride = 256;
    movAlignRecord.loop2SrcStride = 512;
    movAlignRecord.nBurst = 16;
    movAlignRecord.lenBurst = 32;
    movAlignRecord.loop1Size = 1;
    movAlignRecord.loop2Size = 1;
    movAlignRecord.location.blockId = 0;
    movAlignRecord.dstMemType = MemType::UB;
    movAlignRecord.srcMemType = MemType::GM;
    movAlignRecord.dataType = DataType::DATA_B32;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() != 0);
    events.clear();

    movAlignRecord.dstMemType = MemType::UB;
    movAlignRecord.srcMemType = MemType::GM;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() != 0);
    events.clear();

    movAlignRecord.loop1Size = 0;
    movAlignRecord.loop2Size = 0;
    movAlignRecord.leftPaddingNum = 1;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() != 0);
}

TEST_F(TestRecordParse, parse_record_mov_dma_out_to_ub_expect_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::ND_DMA_OUT_TO_UB;
    auto &dmaRecord = record.payload.ndDMAOut2UbRecord;
    dmaRecord.dst = 0;
    dmaRecord.src = 0x100;
    dmaRecord.location.blockId = 0;
    dmaRecord.dataType = DataType::DATA_B32;
    for (uint64_t i = 0; i < NdDMAOut2UbRecord::LOOP; ++i) {
        dmaRecord.loop[i].loopSrcStride = 1;
        dmaRecord.loop[i].loopDstStride = 1;
        dmaRecord.loop[i].loopSize = 1;
        dmaRecord.loop[i].loopLpSize = 1;
        dmaRecord.loop[i].loopRpSize = 1;
    }
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() != 0);
    events.clear();
}

void CreateDmaMovNd2NzDavRecord(DmaMovNd2nzDavRecord &dmaMovRecord)
{
    dmaMovRecord.srcMemType = MemType::GM;
    dmaMovRecord.dstMemType = MemType::L1;
    dmaMovRecord.dataType = DataType::DATA_B8;
    dmaMovRecord.dst = 0x100;
    dmaMovRecord.src = 0x200;
    dmaMovRecord.loop1SrcStride = 8;
    dmaMovRecord.nValue = 16;
    dmaMovRecord.dValue = 8;
    dmaMovRecord.loop4SrcStride = 2048;
    dmaMovRecord.smallC0 = 0;
    dmaMovRecord.ndNum = 2;
    dmaMovRecord.loop2DstStride = 128;
    dmaMovRecord.loop3DstStride = 512;
    dmaMovRecord.loop4DstStride = 2048;
}

TEST_F(TestRecordParse, parse_record_copy_gm_to_cbuf_multi_nd2nz_d_b8_expect_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::DMA_MOV_ND2NZ_D;
    auto &dmaMovRecord = record.payload.dmaMovNd2nzDavRecord;
    CreateDmaMovNd2NzDavRecord(dmaMovRecord);
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() != 0);
}

TEST_F(TestRecordParse, parse_record_copy_gm_to_cbuf_multi_nd2nz_d_b8_small_c0_mode_expect_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::DMA_MOV_ND2NZ_D;
    auto &dmaMovRecord = record.payload.dmaMovNd2nzDavRecord;
    CreateDmaMovNd2NzDavRecord(dmaMovRecord);
    dmaMovRecord.dValue = 4;
    dmaMovRecord.smallC0 = 1;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() != 0);
}

TEST_F(TestRecordParse, parse_record_copy_gm_to_cbuf_multi_nd2nz_d_b8_expect_failed)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::DMA_MOV_ND2NZ_D;
    auto &dmaMovRecord = record.payload.dmaMovNd2nzDavRecord;
    CreateDmaMovNd2NzDavRecord(dmaMovRecord);
    dmaMovRecord.smallC0 = 1;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() == 0);

    dmaMovRecord.loop1SrcStride = 1;
    dmaMovRecord.dataType = DataType::DATA_B32;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() == 0);

    dmaMovRecord.dst = 1;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() == 0);
}

TEST_F(TestRecordParse, parse_record_copy_gm_to_cbuf_multi_dn2nz_d_b8_expect_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::DMA_MOV_DN2NZ_D;
    auto &dmaMovRecord = record.payload.dmaMovNd2nzDavRecord;
    CreateDmaMovNd2NzDavRecord(dmaMovRecord);
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() != 0);
}

TEST_F(TestRecordParse, parse_record_copy_gm_to_cbuf_multi_dn2nz_d_b8_small_c0_mode_expect_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::DMA_MOV_DN2NZ_D;
    auto &dmaMovRecord = record.payload.dmaMovNd2nzDavRecord;
    CreateDmaMovNd2NzDavRecord(dmaMovRecord);
    dmaMovRecord.dValue = 4;
    dmaMovRecord.smallC0 = 1;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() != 0);
}

void CreateSetL12DRecord(SetL12DRecord &setL12DRecord)
{
    setL12DRecord.dst = 0x1000;
    setL12DRecord.repeat = 1;
    setL12DRecord.dstBlockNum =  16;
    setL12DRecord.repeatGap = 16;
}

void CreateLoadL12DRecord(LoadL12DRecord &loadL12DRecord)
{
    loadL12DRecord.dst = 0x1000;
    loadL12DRecord.src = 0x1000;
    loadL12DRecord.mStartPosition = 2;
    loadL12DRecord.kStartPosition = 2;
    loadL12DRecord.mStep = 2;
    loadL12DRecord.kStep = 3;
    loadL12DRecord.srcStride = 5;
    loadL12DRecord.dstStride = 3;
    loadL12DRecord.dstMemType = MemType::L0A;
    loadL12DRecord.transposeMode = TransposeMode::DISABLE;
    loadL12DRecord.detailedDataType = DetailedDataType::B32;
}

void CreateLoadL12DTransposeRecord(LoadL12DTransposeRecord &loadL12DTransposeRecord)
{
    loadL12DTransposeRecord.dst = 0x1000;
    loadL12DTransposeRecord.src = 0x1000;
    loadL12DTransposeRecord.repeat = 2;
    loadL12DTransposeRecord.srcStride = 4;
    loadL12DTransposeRecord.dstStride = 8;
    loadL12DTransposeRecord.srcFracStride = 1;
    loadL12DTransposeRecord.dstFracStride = 1;
    loadL12DTransposeRecord.detailedDataType = DetailedDataType::B4;
}

void CreateLoadL1Mx2DRecord(LoadL1Mx2DRecord &loadL1Mx2DRecord)
{
    loadL1Mx2DRecord.dst = 0x1000;
    loadL1Mx2DRecord.src = 0x1000;
    loadL1Mx2DRecord.xStartPosition = 2;
    loadL1Mx2DRecord.yStartPosition = 2;
    loadL1Mx2DRecord.xStep = 2;
    loadL1Mx2DRecord.yStep = 3;
    loadL1Mx2DRecord.srcStride = 5;
    loadL1Mx2DRecord.dstStride = 3;
    loadL1Mx2DRecord.dstMemType = MemType::L0A;
}

TEST_F(TestRecordParse, parse_report_set_l1_2d_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::SET_L1_2D;
    auto &setL12DRecord = record.payload.setL12DRecord;
    CreateSetL12DRecord(setL12DRecord);
    setL12DRecord.detailedDataType = DetailedDataType::B16;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() != 0);
}

TEST_F(TestRecordParse, parse_report_load_l1_2d_test)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::LOAD_L1_2D;
    auto &loadL12DRecord = record.payload.loadL12DRecord;
    CreateLoadL12DRecord(loadL12DRecord);
    RecordParse::Parse(sanitizerRecord, events);

    loadL12DRecord.transposeMode = TransposeMode::ENABLE;
    loadL12DRecord.detailedDataType = DetailedDataType::B32;
    RecordParse::Parse(sanitizerRecord, events);
    loadL12DRecord.kStep = 2;
    RecordParse::Parse(sanitizerRecord, events);

    loadL12DRecord.detailedDataType = DetailedDataType::B16;
    RecordParse::Parse(sanitizerRecord, events);

    loadL12DRecord.detailedDataType = DetailedDataType::B8;
    RecordParse::Parse(sanitizerRecord, events);
    loadL12DRecord.mStep = 3;
    RecordParse::Parse(sanitizerRecord, events);

    loadL12DRecord.detailedDataType = DetailedDataType::B4;
    RecordParse::Parse(sanitizerRecord, events);
    loadL12DRecord.mStep = 4;
    RecordParse::Parse(sanitizerRecord, events);

    loadL12DRecord.detailedDataType = DetailedDataType::E1M2;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() != 0);
}

TEST_F(TestRecordParse, parse_report_load_l1_2d_transpose_test)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::LOAD_L1_2D_TRANSPOSE;
    auto &loadL12DTransposeRecord = record.payload.loadL12DTransposeRecord;
    CreateLoadL12DTransposeRecord(loadL12DTransposeRecord);
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() != 0);
}

void CreateLoad3DV2Record(Load3DV2Record &load3DV2Record)
{
    load3DV2Record.dst = 0x1000;
    load3DV2Record.src = 0x1000;

    load3DV2Record.matrixKStep = 8;
    load3DV2Record.matrixMStep = 8;
    load3DV2Record.matrixKPos = 0;
    load3DV2Record.matrixMPos = 0;

    load3DV2Record.filterWStride = 1;
    load3DV2Record.filterHStride = 1;
    load3DV2Record.filterW = 3;
    load3DV2Record.filterH = 3;
    load3DV2Record.filterWDilation = 1;
    load3DV2Record.filterHDilation = 1;
    load3DV2Record.transposeMode = 0;
    load3DV2Record.matrixMode = 0;
    load3DV2Record.fMapC = 8;

    load3DV2Record.fMapW = 32;
    load3DV2Record.fMapH = 32;
    load3DV2Record.fMapLeftPad = 0;
    load3DV2Record.fMapRightPad = 0;
    load3DV2Record.fMapTopPad = 0;
    load3DV2Record.fMapBottomPad = 0;
    
    load3DV2Record.matrixRptStride = 32;
    load3DV2Record.matrixRptTimes = 1;
    load3DV2Record.matrixRptMode = 0;
    load3DV2Record.dstStride = 32;
    load3DV2Record.outputMPos = 0;

    load3DV2Record.dataType = DataType::DATA_B32;
    load3DV2Record.dstMemType = MemType::L0A;
    load3DV2Record.srcMemType = MemType::L1;
}

TEST_F(TestRecordParse, parse_report_load_3d_test)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::LOAD_3D_V2;
    auto &Load3DV2Record = record.payload.load3DV2Record;
    CreateLoad3DV2Record(Load3DV2Record);
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() != 0);
}

void CreateL1OrUbMovL1UbRecord(MovL1UBRecord &movL1UbRecord)
{
    movL1UbRecord.dst = 0x100;
    movL1UbRecord.src = 0x200;
    movL1UbRecord.nBurst = 16;
    movL1UbRecord.lenBurst = 32;
    movL1UbRecord.srcGap = 32;
    movL1UbRecord.dstGap = 256;
    movL1UbRecord.location.fileNo = 7;
    movL1UbRecord.location.lineNo = 8;
    movL1UbRecord.location.blockId = 0;
}
 
TEST_F(TestRecordParse, parse_record_copy_cbuf_to_ubuf_expect_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::MOV_L1_TO_UB;
    auto &movL1UbRecord = record.payload.movL1UbRecord;
    movL1UbRecord.srcMemType = MemType::L1;
    movL1UbRecord.dstMemType = MemType::UB;
    CreateL1OrUbMovL1UbRecord(movL1UbRecord);
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() != 0);
    // 读事件
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].eventInfo.memInfo.addr, 0x200);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockNum, 32);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockSize, MOV_LOCAL_BLOCK_SIZE);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockStride, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatTimes, 16);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatStride, 64);
    ASSERT_EQ(events[0].eventInfo.memInfo.memType, MemType::L1);
    // 写事件
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].eventInfo.memInfo.addr, 0x100);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockNum, 32);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockSize, MOV_LOCAL_BLOCK_SIZE);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockStride, 1);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatTimes, 16);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatStride, 288);
    ASSERT_EQ(events[1].eventInfo.memInfo.memType, MemType::UB);
}

TEST_F(TestRecordParse, parse_record_copy_ubuf_to_cbuf_expect_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::MOV_UB_TO_L1;
    auto &movL1UbRecord = record.payload.movL1UbRecord;
    movL1UbRecord.srcMemType = MemType::UB;
    movL1UbRecord.dstMemType = MemType::L1;
    CreateL1OrUbMovL1UbRecord(movL1UbRecord);
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() != 0);
    // 读事件
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].eventInfo.memInfo.addr, 0x200);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockNum, 32);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockSize, MOV_LOCAL_BLOCK_SIZE);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockStride, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatTimes, 16);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatStride, 64);
    ASSERT_EQ(events[0].eventInfo.memInfo.memType, MemType::UB);
    // 写事件
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].eventInfo.memInfo.addr, 0x100);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockNum, 32);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockSize, MOV_LOCAL_BLOCK_SIZE);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockStride, 1);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatTimes, 16);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatStride, 288);
    ASSERT_EQ(events[1].eventInfo.memInfo.memType, MemType::L1);
}

TEST_F(TestRecordParse, parse_record_copy_ubuf_to_ubuf_expect_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::MOV_UB_TO_UB;
    auto &movL1UbRecord = record.payload.movL1UbRecord;
    movL1UbRecord.srcMemType = MemType::UB;
    movL1UbRecord.dstMemType = MemType::UB;
    CreateL1OrUbMovL1UbRecord(movL1UbRecord);
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() != 0);
    // 读事件
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].eventInfo.memInfo.addr, 0x200);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockNum, 32);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockSize, MOV_LOCAL_BLOCK_SIZE);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockStride, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatTimes, 16);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatStride, 64);
    ASSERT_EQ(events[0].eventInfo.memInfo.memType, MemType::UB);
    // 写事件
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].eventInfo.memInfo.addr, 0x100);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockNum, 32);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockSize, MOV_LOCAL_BLOCK_SIZE);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockStride, 1);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatTimes, 16);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatStride, 288);
    ASSERT_EQ(events[1].eventInfo.memInfo.memType, MemType::UB);
}

TEST_F(TestRecordParse, parse_report_load_l1_mx_2d_test)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::LOAD_L1_MX_2D;
    auto &loadL1Mx2DRecord = record.payload.loadL1Mx2DRecord;
    CreateLoadL1Mx2DRecord(loadL1Mx2DRecord);
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() != 0);
}

TEST_F(TestRecordParse, parse_ldva_expect_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord &record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::LDVA;
    auto &loadStoreRecord = record.payload.loadStoreRecord;
    loadStoreRecord.addr = 0x2000;
    loadStoreRecord.size = 32U; // 固定读32B
    loadStoreRecord.space = AddressSpace::UB;
    loadStoreRecord.alignSize = 32U;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() == 1);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].eventInfo.memInfo.addr, loadStoreRecord.addr);
}

TEST_F(TestRecordParse, parse_mmada5_expect_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::MMAD_A5;
    auto &mmadA5Record = record.payload.mmadA5Record;
    mmadA5Record.dst = 0;
    mmadA5Record.dstBlockNum = 16;
    mmadA5Record.src0 = 0;
    mmadA5Record.src0BlockNum = 32;
    mmadA5Record.src0AlignSize = 512;
    mmadA5Record.src1 = 0;
    mmadA5Record.src1BlockNum = 64;
    mmadA5Record.src1AlignSize = 512;
    mmadA5Record.m = 64;
    mmadA5Record.k = 128;
    mmadA5Record.n = 64;
    mmadA5Record.cmatrixInitVal = 1;
    mmadA5Record.src0Dtype = DetailedDataType::F16;
    mmadA5Record.src1Dtype = DetailedDataType::FLOAT;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() == 3);
    // 读事件
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].eventInfo.memInfo.addr, 0);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockNum, 32);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockSize, 512);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockStride, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatTimes, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatStride, 0);
    ASSERT_EQ(events[0].eventInfo.memInfo.memType, MemType::L0A);
    // 写事件
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].eventInfo.memInfo.addr, 0);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockNum, 64);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockSize, 512);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockStride, 1);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatTimes, 1);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatStride, 0);
    ASSERT_EQ(events[1].eventInfo.memInfo.memType, MemType::L0B);
    // 写事件
    ASSERT_EQ(events[2].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[2].eventInfo.memInfo.addr, 0);
    ASSERT_EQ(events[2].eventInfo.memInfo.blockNum, 16);
    ASSERT_EQ(events[2].eventInfo.memInfo.blockSize, 1024);
    ASSERT_EQ(events[2].eventInfo.memInfo.blockStride, 1);
    ASSERT_EQ(events[2].eventInfo.memInfo.repeatTimes, 1);
    ASSERT_EQ(events[2].eventInfo.memInfo.repeatStride, 0);
    ASSERT_EQ(events[2].eventInfo.memInfo.memType, MemType::L0C);
}

TEST_F(TestRecordParse, parse_mov_cbuf_to_bt_with_illegal_params_expect_get_none)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::MOV_CBUF_TO_BT;
    auto &movL1BtRecord = record.payload.movL1BtRecord;
    movL1BtRecord.dataType = DetailedDataType::FLOAT;
    movL1BtRecord.lenBurst = 15;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() == 0);
    movL1BtRecord.lenBurst = 16;
    movL1BtRecord.cvtEnable = 1;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() == 0);
    movL1BtRecord.dataType = DetailedDataType::S32;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() == 0);
    movL1BtRecord.lenBurst = 15;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() == 0);
    movL1BtRecord.lenBurst = 16;
    movL1BtRecord.cvtEnable = 0;
    movL1BtRecord.dstGap = 1;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() == 0);
    movL1BtRecord.dataType = DetailedDataType::F16;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() == 0);
}

TEST_F(TestRecordParse, parse_mov_cbuf_to_bt_with_expect_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::MOV_CBUF_TO_BT;
    auto &movL1BtRecord = record.payload.movL1BtRecord;
    movL1BtRecord.dst = 0x1000;
    movL1BtRecord.src = 0x200;
    movL1BtRecord.dataType = DetailedDataType::FLOAT;
    movL1BtRecord.lenBurst = 6;
    movL1BtRecord.nBurst = 2;
    movL1BtRecord.srcGap = 1;
    movL1BtRecord.dstGap = 4;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);

     // 读事件
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].eventInfo.memInfo.opType, AccessType::READ);
    ASSERT_EQ(events[0].eventInfo.memInfo.addr, 0x200);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockNum, 6);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockSize, 32);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockStride, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatTimes, 2);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatStride, 7);
    ASSERT_EQ(events[0].eventInfo.memInfo.memType, MemType::L1);

    // 写事件
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].eventInfo.memInfo.opType, AccessType::WRITE);
    ASSERT_EQ(events[1].eventInfo.memInfo.addr, 0x1000);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockNum, 6);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockSize, 32);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockStride, 1);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatTimes, 2);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatStride, 10);
    ASSERT_EQ(events[1].eventInfo.memInfo.memType, MemType::BT);

    events.clear();
    movL1BtRecord.dataType = DetailedDataType::F16;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockNum, 12);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatStride, 16);
}

TEST_F(TestRecordParse, parse_mov_cbuf_to_fbuf_with_illegal_params_expect_get_none)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::MOV_CBUF_TO_FB;
    auto &movL1FbRecord = record.payload.movL1FbRecord;
    movL1FbRecord.dstMemBlock = 0;
    movL1FbRecord.lenBurst = 5;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() == 0);
    movL1FbRecord.lenBurst = 6;
    movL1FbRecord.dstStride = 3;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() == 0);
    movL1FbRecord.dstMemBlock = 2;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_TRUE(events.size() == 0);
}

TEST_F(TestRecordParse, parse_mov_cbuf_to_fbuf_with_expect_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord& record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::MOV_CBUF_TO_FB;
    auto &movL1FbRecord = record.payload.movL1FbRecord;
    movL1FbRecord.location.blockId = 3;
    movL1FbRecord.dst = 0x2000;
    movL1FbRecord.src = 0x300;
    movL1FbRecord.lenBurst = 6;
    movL1FbRecord.nBurst = 2;
    movL1FbRecord.srcStride = 3;
    movL1FbRecord.dstStride = 6;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);

     // 读事件
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].eventInfo.memInfo.addr, 0x300);
    ASSERT_EQ(events[0].eventInfo.memInfo.opType, AccessType::READ);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockNum, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockSize, 384);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatTimes, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.memType, MemType::L1);
    ASSERT_EQ(events[1].eventInfo.memInfo.addr, 0x360);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockNum, 1);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockSize, 384);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatTimes, 1);

    // 写事件
    ASSERT_EQ(events[2].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[2].eventInfo.memInfo.opType, AccessType::WRITE);
    ASSERT_EQ(events[2].eventInfo.memInfo.addr, 0x2000);
    ASSERT_EQ(events[2].eventInfo.memInfo.blockNum, 6);
    ASSERT_EQ(events[2].eventInfo.memInfo.blockSize, 64);
    ASSERT_EQ(events[2].eventInfo.memInfo.blockStride, 1);
    ASSERT_EQ(events[2].eventInfo.memInfo.repeatTimes, 2);
    ASSERT_EQ(events[2].eventInfo.memInfo.repeatStride, 6);
    ASSERT_EQ(events[2].eventInfo.memInfo.memType, MemType::FB);
}

TEST_F(TestRecordParse, parse_vbs32_repeat0_expect_none)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord &record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::VBS32_A5;
    auto &vbs32Record = record.payload.vbs32Record;
    vbs32Record.repeat = 0;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
}

TEST_F(TestRecordParse, parse_vbs32_repeat0_expect_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord &record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::VBS32_A5;
    auto &vbs32Record = record.payload.vbs32Record;
    vbs32Record.dst = 0x2000;
    vbs32Record.src0 = 0;
    vbs32Record.src1 = 0x100;
    vbs32Record.repeat = 1;
    vbs32Record.dataType = DataType::DATA_B16;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);
    // read score
    ASSERT_EQ(events[0].eventInfo.memInfo.addr, vbs32Record.src0);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockSize, 64);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockNum, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockStride, 0);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatTimes, vbs32Record.repeat);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatStride, 1);
    // read index
    ASSERT_EQ(events[1].eventInfo.memInfo.addr, vbs32Record.src1);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockSize, 128);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockNum, 1);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockStride, 0);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatTimes, vbs32Record.repeat);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatStride, 1);
    // write
    ASSERT_EQ(events[2].eventInfo.memInfo.addr, vbs32Record.dst);
    ASSERT_EQ(events[2].eventInfo.memInfo.blockSize, 256);
    ASSERT_EQ(events[2].eventInfo.memInfo.blockNum, 1);
    ASSERT_EQ(events[2].eventInfo.memInfo.blockStride, 0);
    ASSERT_EQ(events[2].eventInfo.memInfo.repeatTimes, vbs32Record.repeat);
    ASSERT_EQ(events[2].eventInfo.memInfo.repeatStride, 1);
}

void InitVms4v2RecordA5(Vms4v2RecordA5 &record)
{
    record.detailedDataType = DetailedDataType::F16;
    record.dst = 0x2000;
    record.src[0] = 0x80;
    record.src[1] = 0x100;
    record.src[2] = 0x180;
    record.src[3] = 0x200;
    record.elementNum[0] = 1;
    record.elementNum[1] = 2;
    record.elementNum[2] = 3;
    record.elementNum[3] = 4;
    record.repeat = 10;
    record.validMask = 0b1111;
    record.isAllStored = 1;
}

TEST_F(TestRecordParse, parse_vms4v2_repeat1_expect_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord &record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::VMRGSORT4_OP_C310;
    auto &vms4V2RecordA5 = record.payload.vms4V2RecordA5;
    InitVms4v2RecordA5(vms4V2RecordA5);
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 5);
    uint32_t num = 0;
    for (uint32_t i = 0; i < Vms4v2RecordA5::ARRAY_NUM; ++i) {
        ASSERT_EQ(events[i].eventInfo.memInfo.addr, vms4V2RecordA5.src[i]);
        ASSERT_EQ(events[i].eventInfo.memInfo.blockNum, vms4V2RecordA5.elementNum[i]);
        ASSERT_EQ(events[i].eventInfo.memInfo.blockSize, 8);
        ASSERT_EQ(events[i].eventInfo.memInfo.blockStride, 1);
        ASSERT_EQ(events[i].eventInfo.memInfo.repeatTimes, 1);
        ASSERT_EQ(events[i].eventInfo.memInfo.repeatStride, 0);
        num += vms4V2RecordA5.elementNum[i];
    }
    ASSERT_EQ(events[4].eventInfo.memInfo.addr, vms4V2RecordA5.dst);
    ASSERT_EQ(events[4].eventInfo.memInfo.blockNum, num);
    ASSERT_EQ(events[4].eventInfo.memInfo.blockSize, 8);
    ASSERT_EQ(events[4].eventInfo.memInfo.blockStride, 1);
    ASSERT_EQ(events[4].eventInfo.memInfo.repeatTimes, 1);
    ASSERT_EQ(events[4].eventInfo.memInfo.repeatStride, 0);
}

TEST_F(TestRecordParse, parse_vms4v2_repeat_expect_success)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord &record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::VMRGSORT4_OP_C310;
    auto &vms4V2RecordA5 = record.payload.vms4V2RecordA5;
    InitVms4v2RecordA5(vms4V2RecordA5);
    vms4V2RecordA5.isAllStored = false;
    vms4V2RecordA5.validMask = 0b1111;
    for (uint32_t i = 0; i < Vms4v2RecordA5::ARRAY_NUM; ++i) {
        vms4V2RecordA5.elementNum[i] = 10;
    }
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    // read
    ASSERT_EQ(events[0].eventInfo.memInfo.addr, vms4V2RecordA5.src[0]);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockNum, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockSize, vms4V2RecordA5.elementNum[0] * 32);
    ASSERT_EQ(events[0].eventInfo.memInfo.blockStride, 1);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatTimes, vms4V2RecordA5.repeat);
    ASSERT_EQ(events[0].eventInfo.memInfo.repeatStride, 1);
    // write
    ASSERT_EQ(events[1].eventInfo.memInfo.addr, vms4V2RecordA5.dst);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockNum, 1);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockSize, vms4V2RecordA5.elementNum[0] * 32);
    ASSERT_EQ(events[1].eventInfo.memInfo.blockStride, 1);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatTimes, vms4V2RecordA5.repeat);
    ASSERT_EQ(events[1].eventInfo.memInfo.repeatStride, 1);
}

TEST_F(TestRecordParse, parse_vms4v2_isAllStored_input_empty_expect_nop)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord &record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::VMRGSORT4_OP_C310;
    auto &vms4V2RecordA5 = record.payload.vms4V2RecordA5;
    InitVms4v2RecordA5(vms4V2RecordA5);
    vms4V2RecordA5.isAllStored = true;
    vms4V2RecordA5.validMask = 1;
    vms4V2RecordA5.elementNum[0] = 0;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
}

TEST_F(TestRecordParse, parse_vms4v2_not_isAllStored_repeat0_expect_nop)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord &record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::VMRGSORT4_OP_C310;
    auto &vms4V2RecordA5 = record.payload.vms4V2RecordA5;
    InitVms4v2RecordA5(vms4V2RecordA5);
    vms4V2RecordA5.isAllStored = false;
    vms4V2RecordA5.repeat = 0;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
}

TEST_F(TestRecordParse, parse_vms4v2_not_isAllStored_repeat1_input_empty_expect_nop)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord &record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::VMRGSORT4_OP_C310;
    auto &vms4V2RecordA5 = record.payload.vms4V2RecordA5;
    InitVms4v2RecordA5(vms4V2RecordA5);
    vms4V2RecordA5.isAllStored = false;
    vms4V2RecordA5.repeat = 1;
    vms4V2RecordA5.validMask = 1;
    vms4V2RecordA5.elementNum[0] = 0;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
}

TEST_F(TestRecordParse, parse_vms4v2_repeat_not_all_valid_expect_nop)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord &record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::VMRGSORT4_OP_C310;
    auto &vms4V2RecordA5 = record.payload.vms4V2RecordA5;
    InitVms4v2RecordA5(vms4V2RecordA5);
    vms4V2RecordA5.isAllStored = false;
    vms4V2RecordA5.repeat = 2;
    vms4V2RecordA5.validMask = 1;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
}

TEST_F(TestRecordParse, parse_vms4v2_repeat_element_number_not_same_expect_nop)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord &record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::VMRGSORT4_OP_C310;
    auto &vms4V2RecordA5 = record.payload.vms4V2RecordA5;
    InitVms4v2RecordA5(vms4V2RecordA5);
    vms4V2RecordA5.isAllStored = false;
    vms4V2RecordA5.repeat = 2;
    vms4V2RecordA5.validMask = 0b1111;
    vms4V2RecordA5.elementNum[0] = 1;
    vms4V2RecordA5.elementNum[1] = 2;
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
}

TEST_F(TestRecordParse, parse_vms4v2_repeat_element_number_0_expect_nop)
{
    std::vector<SanEvent> events;
    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    KernelRecord &record = sanitizerRecord.payload.kernelRecord;
    record.recordType = RecordType::VMRGSORT4_OP_C310;
    auto &vms4V2RecordA5 = record.payload.vms4V2RecordA5;
    InitVms4v2RecordA5(vms4V2RecordA5);
    vms4V2RecordA5.repeat = 2;
    vms4V2RecordA5.validMask = 0b1111;
    for (uint32_t i = 0; i < Vms4v2RecordA5::ARRAY_NUM; ++i) {
        vms4V2RecordA5.elementNum[i] = 0;
    }
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
}

struct MovFpConfig {
    bool isC310;
    uint64_t dst;
    uint64_t src;
    uint32_t dstStride;
    uint32_t srcStride;
    uint16_t nSize;
    uint16_t mSize;
    uint16_t ndNum;
    uint32_t dstNdStride;
    uint16_t srcNdStride;
    uint16_t srcNzC0Stride;
    uint16_t quantPreBits;
    bool enUnitFlag;
    bool int8ChannelMerge;
    bool int4ChannelMerge;
    bool channelSplit;
    bool enNZ2ND;
    bool enNZ2DN;
    uint16_t blockId;
};

void SetMovFpRecord(MovFpRecord &record, const MovFpConfig &config)
{
    record.isC310 = config.isC310;
    record.dst = config.dst;
    record.src = config.src;
    record.dstStride = config.dstStride;
    record.srcStride = config.srcStride;
    record.nSize = config.nSize;
    record.mSize = config.mSize;
    record.ndNum = config.ndNum;
    record.dstNdStride = config.dstNdStride;
    record.srcNdStride = config.srcNdStride;
    record.srcNzC0Stride = config.srcNzC0Stride;
    record.quantPreBits = config.quantPreBits;
    record.enUnitFlag = config.enUnitFlag;
    record.int8ChannelMerge = config.int8ChannelMerge;
    record.int4ChannelMerge = config.int4ChannelMerge;
    record.channelSplit = config.channelSplit;
    record.enNZ2ND = config.enNZ2ND;
    record.enNZ2DN = config.enNZ2DN;
    record.location.blockId = config.blockId;
}

TEST_F(TestRecordParse, parse_fix_L0C_to_L1_record_with_normal_movement_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::FIX_L0C_TO_L1;
    MovFpConfig cfg = {
        .isC310 = true,
        .dst = 0x111,
        .src = 0x0,
        .dstStride = 80,
        .srcStride = 64,
        .nSize = 48,
        .mSize = 24,
        .ndNum = 0,
        .dstNdStride = 0,
        .srcNdStride = 0,
        .srcNzC0Stride = 0,
        .quantPreBits = 32,
        .enUnitFlag = false,
        .int8ChannelMerge = false,
        .int4ChannelMerge = false,
        .channelSplit = false,
        .enNZ2ND = false,
        .enNZ2DN = false,
        .blockId = 0
    };
    SetMovFpRecord(record.payload.movFpRecord, cfg);
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
 
    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 24, 32, 1, 3, 64, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
 
    // 校验写内存event
    ASSERT_EQ(events[1].loc.coreId, 0);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L1, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 384, 4, 1, 3, 80, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}
 
TEST_F(TestRecordParse, parse_fix_L0C_to_L1_record_with_int8_channal_merging_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::FIX_L0C_TO_L1;
    MovFpConfig cfg = {
        .isC310 = true,
        .dst = 0x111,
        .src = 0x0,
        .dstStride = 32,
        .srcStride = 32,
        .nSize = 48,
        .mSize = 32,
        .ndNum = 0,
        .dstNdStride = 0,
        .srcNdStride = 0,
        .srcNzC0Stride = 0,
        .quantPreBits = 8,
        .enUnitFlag = false,
        .int8ChannelMerge = true,
        .int4ChannelMerge = false,
        .channelSplit = false,
        .enNZ2ND = false,
        .enNZ2DN = false,
        .blockId = 0
    };
    SetMovFpRecord(record.payload.movFpRecord, cfg);
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);
 
    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 32, 32, 1, 3, 32, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
 
    // 校验写内存event
    ASSERT_EQ(events[1].loc.coreId, 0);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L1, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 32 * 32, 1, 1, 1, 32, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
 
    memOpInfo = {MemType::L1, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111 + 32 * 1, 32 * 16, 1, 1, 1, 64, 32};
    ASSERT_EQ(events[2].eventInfo.memInfo, memOpInfo);
}
 
TEST_F(TestRecordParse, parse_fix_L0C_to_L1_record_with_int4_channal_merging_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::FIX_L0C_TO_L1;
    MovFpConfig cfg = {
        .isC310 = true,
        .dst = 0x111,
        .src = 0x0,
        .dstStride = 32,
        .srcStride = 32,
        .nSize = 128,
        .mSize = 32,
        .ndNum = 0,
        .dstNdStride = 0,
        .srcNdStride = 0,
        .srcNzC0Stride = 0,
        .quantPreBits = 4,
        .enUnitFlag = false,
        .int8ChannelMerge = false,
        .int4ChannelMerge = true,
        .channelSplit = false,
        .enNZ2ND = false,
        .enNZ2DN = false,
        .blockId = 0
    };
    SetMovFpRecord(record.payload.movFpRecord, cfg);
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
 
    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 32, 32, 1, 8, 32, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
 
    // 校验写内存event
    ASSERT_EQ(events[1].loc.coreId, 0);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L1, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 32 * 32, 1, 1, 2, 16, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}
 
TEST_F(TestRecordParse, parse_fix_L0C_to_L1_record_with_f32_channel_split_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::FIX_L0C_TO_L1;
    MovFpConfig cfg = {
        .isC310 = true,
        .dst = 0x111,
        .src = 0x0,
        .dstStride = 64,
        .srcStride = 64,
        .nSize = 24,
        .mSize = 64,
        .ndNum = 0,
        .dstNdStride = 0,
        .srcNdStride = 0,
        .srcNzC0Stride = 0,
        .quantPreBits = 32,
        .enUnitFlag = false,
        .int8ChannelMerge = false,
        .int4ChannelMerge = false,
        .channelSplit = true,
        .enNZ2ND = false,
        .enNZ2DN = false,
        .blockId = 0
    };
    SetMovFpRecord(record.payload.movFpRecord, cfg);
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
 
    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 64, 32, 1, 2, 64, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
 
    // 校验写内存event
    ASSERT_EQ(events[1].loc.coreId, 0);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L1, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 64 * 8, 4, 1, 3, 64, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}
 
TEST_F(TestRecordParse, parse_fix_L0C_to_L1_record_with_NZ2ND_conversion_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::FIX_L0C_TO_L1;
    MovFpConfig cfg = {
        .isC310 = true,
        .dst = 0x111,
        .src = 0x0,
        .dstStride = 48,
        .srcStride = 80,
        .nSize = 24,
        .mSize = 48,
        .ndNum = 2,
        .dstNdStride = 64 * 48,
        .srcNdStride = 15,
        .srcNzC0Stride = 0,
        .quantPreBits = 32,
        .enUnitFlag = false,
        .int8ChannelMerge = false,
        .int4ChannelMerge = false,
        .channelSplit = false,
        .enNZ2ND = true,
        .enNZ2DN = false,
        .blockId = 0
    };
    SetMovFpRecord(record.payload.movFpRecord, cfg);
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 6);
 
    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 48, 32, 1, 1, 80, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
 
    // 校验写内存event
    ASSERT_EQ(events[4].loc.coreId, 0);
    ASSERT_EQ(events[4].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[4].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L1, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 24, 4, 1, 48, 48, 32};
    ASSERT_EQ(events[4].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_fix_L0C_to_L1_record_with_NZ2DN_conversion_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::FIX_L0C_TO_L1;
    MovFpConfig cfg = {
        .isC310 = true,
        .dst = 0x111,
        .src = 0x0,
        .dstStride = 48,
        .srcStride = 80,
        .nSize = 24,
        .mSize = 48,
        .ndNum = 2,
        .dstNdStride = 64 * 48,
        .srcNdStride = 15,
        .srcNzC0Stride = 2,
        .quantPreBits = 32,
        .enUnitFlag = false,
        .int8ChannelMerge = false,
        .int4ChannelMerge = false,
        .channelSplit = false,
        .enNZ2ND = false,
        .enNZ2DN = true,
        .blockId = 0
    };
    SetMovFpRecord(record.payload.movFpRecord, cfg);
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 6);
 
    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 48, 32, 2, 1, 80, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
 
    // 校验写内存event
    ASSERT_EQ(events[4].loc.coreId, 0);
    ASSERT_EQ(events[4].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[4].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L1, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 48, 4, 1, 24, 48, 32};
    ASSERT_EQ(events[4].eventInfo.memInfo, memOpInfo);
}
 
TEST_F(TestRecordParse, parse_fix_L0C_to_UB_record_with_normal_movement_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::FIX_L0C_TO_UB;
    MovFpConfig cfg = {
        .isC310 = true,
        .dst = 0x111,
        .src = 0x0,
        .dstStride = 80,
        .srcStride = 64,
        .nSize = 48,
        .mSize = 24,
        .ndNum = 0,
        .dstNdStride = 0,
        .srcNdStride = 0,
        .srcNzC0Stride = 0,
        .quantPreBits = 32,
        .enUnitFlag = false,
        .int8ChannelMerge = false,
        .int4ChannelMerge = false,
        .channelSplit = false,
        .enNZ2ND = false,
        .enNZ2DN = false,
        .blockId = 0
    };
    SetMovFpRecord(record.payload.movFpRecord, cfg);
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
 
    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 24, 32, 1, 3, 64, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
 
    // 校验写内存event
    ASSERT_EQ(events[1].loc.coreId, 0);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 384, 4, 1, 3, 80, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}
 
TEST_F(TestRecordParse, parse_fix_L0C_to_UB_record_with_int8_channal_merging_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::FIX_L0C_TO_UB;
    MovFpConfig cfg = {
        .isC310 = true,
        .dst = 0x111,
        .src = 0x0,
        .dstStride = 32,
        .srcStride = 32,
        .nSize = 48,
        .mSize = 32,
        .ndNum = 0,
        .dstNdStride = 0,
        .srcNdStride = 0,
        .srcNzC0Stride = 0,
        .quantPreBits = 8,
        .enUnitFlag = false,
        .int8ChannelMerge = true,
        .int4ChannelMerge = false,
        .channelSplit = false,
        .enNZ2ND = false,
        .enNZ2DN = false,
        .blockId = 0
    };
    SetMovFpRecord(record.payload.movFpRecord, cfg);
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);
 
    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 32, 32, 1, 3, 32, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
 
    // 校验写内存event
    ASSERT_EQ(events[1].loc.coreId, 0);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 32 * 32, 1, 1, 1, 32, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
 
    memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111 + 32 * 1, 32 * 16, 1, 1, 1, 64, 32};
    ASSERT_EQ(events[2].eventInfo.memInfo, memOpInfo);
}
 
TEST_F(TestRecordParse, parse_fix_L0C_to_UB_record_with_int4_channal_merging_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::FIX_L0C_TO_UB;
    MovFpConfig cfg = {
        .isC310 = true,
        .dst = 0x111,
        .src = 0x0,
        .dstStride = 32,
        .srcStride = 32,
        .nSize = 128,
        .mSize = 32,
        .ndNum = 0,
        .dstNdStride = 0,
        .srcNdStride = 0,
        .srcNzC0Stride = 0,
        .quantPreBits = 4,
        .enUnitFlag = false,
        .int8ChannelMerge = false,
        .int4ChannelMerge = true,
        .channelSplit = false,
        .enNZ2ND = false,
        .enNZ2DN = false,
        .blockId = 0
    };
    SetMovFpRecord(record.payload.movFpRecord, cfg);
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
 
    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 32, 32, 1, 8, 32, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
 
    // 校验写内存event
    ASSERT_EQ(events[1].loc.coreId, 0);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 32 * 32, 1, 1, 2, 16, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}
 
TEST_F(TestRecordParse, parse_fix_L0C_to_UB_record_with_f32_channel_split_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::FIX_L0C_TO_UB;
    MovFpConfig cfg = {
        .isC310 = true,
        .dst = 0x111,
        .src = 0x0,
        .dstStride = 64,
        .srcStride = 64,
        .nSize = 24,
        .mSize = 64,
        .ndNum = 0,
        .dstNdStride = 0,
        .srcNdStride = 0,
        .srcNzC0Stride = 0,
        .quantPreBits = 32,
        .enUnitFlag = false,
        .int8ChannelMerge = false,
        .int4ChannelMerge = false,
        .channelSplit = true,
        .enNZ2ND = false,
        .enNZ2DN = false,
        .blockId = 0
    };
    SetMovFpRecord(record.payload.movFpRecord, cfg);
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
 
    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 64, 32, 1, 2, 64, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
 
    // 校验写内存event
    ASSERT_EQ(events[1].loc.coreId, 0);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 64 * 8, 4, 1, 3, 64, 32};
    ASSERT_EQ(events[1].eventInfo.memInfo, memOpInfo);
}
 
TEST_F(TestRecordParse, parse_fix_L0C_to_UB_record_with_NZ2ND_conversion_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::FIX_L0C_TO_UB;
    MovFpConfig cfg = {
        .isC310 = true,
        .dst = 0x111,
        .src = 0x0,
        .dstStride = 48,
        .srcStride = 80,
        .nSize = 24,
        .mSize = 48,
        .ndNum = 2,
        .dstNdStride = 64 * 48,
        .srcNdStride = 15,
        .srcNzC0Stride = 0,
        .quantPreBits = 32,
        .enUnitFlag = false,
        .int8ChannelMerge = false,
        .int4ChannelMerge = false,
        .channelSplit = false,
        .enNZ2ND = true,
        .enNZ2DN = false,
        .blockId = 0
    };
    SetMovFpRecord(record.payload.movFpRecord, cfg);
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 6);
 
    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 48, 32, 1, 1, 80, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
 
    // 校验写内存event
    ASSERT_EQ(events[4].loc.coreId, 0);
    ASSERT_EQ(events[4].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[4].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 24, 4, 1, 48, 48, 32};
    ASSERT_EQ(events[4].eventInfo.memInfo, memOpInfo);
}
 
TEST_F(TestRecordParse, parse_fix_L0C_to_UB_record_with_NZ2DN_conversion_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::FIX_L0C_TO_UB;
    MovFpConfig cfg = {
        .isC310 = true,
        .dst = 0x111,
        .src = 0x0,
        .dstStride = 48,
        .srcStride = 80,
        .nSize = 24,
        .mSize = 48,
        .ndNum = 2,
        .dstNdStride = 64 * 48,
        .srcNdStride = 15,
        .srcNzC0Stride = 2,
        .quantPreBits = 32,
        .enUnitFlag = false,
        .int8ChannelMerge = false,
        .int4ChannelMerge = false,
        .channelSplit = false,
        .enNZ2ND = false,
        .enNZ2DN = true,
        .blockId = 0
    };
    SetMovFpRecord(record.payload.movFpRecord, cfg);
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
 
    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 6);
 
    MemOpInfo memOpInfo;
    // 校验读内存event
    ASSERT_EQ(events[0].loc.coreId, 0);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::L0C, AccessType::READ, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x0, 48, 32, 2, 1, 80, 64};
    ASSERT_EQ(events[0].eventInfo.memInfo, memOpInfo);
 
    // 校验写内存event
    ASSERT_EQ(events[4].loc.coreId, 0);
    ASSERT_EQ(events[4].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[4].pipe, PipeType::PIPE_FIX);
    memOpInfo = {MemType::UB, AccessType::WRITE, DEFAULT_VECTOR_MASK, MaskMode::MASK_NORM,
        8, 0x111, 48, 4, 1, 24, 48, 32};
    ASSERT_EQ(events[4].eventInfo.memInfo, memOpInfo);
}

TEST_F(TestRecordParse, parse_shadow_memory_record_and_expect_success)
{
    std::vector<SanEvent> events;
    KernelRecord record{};
    record.recordType = RecordType::DYNAMIC_OP;
    std::vector<ShadowMemoryRecord> shRecords;
    ShadowMemoryRecord smRecord{};
    smRecord.addr = 0x100;
    smRecord.size = 10;
    shRecords.push_back(smRecord);
    smRecord.addr = 0x200;
    smRecord.size = 20;
    shRecords.push_back(smRecord);
    record.payload.dynamicRecord.buffer = shRecords.data();
    record.payload.dynamicRecord.count = shRecords.size();
    record.payload.dynamicRecord.dynamicType = RecordType::SIMT_ENTRY;

    SanitizerRecord sanitizerRecord{};
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;

    RecordParse::Parse(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ASSERT_EQ(events[0].eventInfo.dynamicOpInfo.count, 2);
    ASSERT_EQ(events[0].eventInfo.dynamicOpInfo.dynamicType, RecordType::SIMT_ENTRY);
    ASSERT_EQ(events[0].eventInfo.dynamicOpInfo.minAddr, 0x100);
    ASSERT_EQ(events[0].eventInfo.dynamicOpInfo.maxAddr, 0x200 + 20);
    ASSERT_EQ(events[0].pipe, PipeType::PIPE_V);
    ASSERT_EQ(events[0].type, EventType::DYNAMIC_MEM_EVENT);
    auto bufferRecord = reinterpret_cast<const ShadowMemoryRecord*>(events[0].eventInfo.dynamicOpInfo.buffer);
    ASSERT_EQ(bufferRecord->addr, 0x100);
    ASSERT_EQ(bufferRecord->size, 10);
}

}