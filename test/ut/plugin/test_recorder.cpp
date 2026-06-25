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
#include "ccec_defs.h"
#define private public
#include "plugin/recorder.h"
#include "ccec/data_process.h"
#undef private

using namespace Sanitizer;

void AssertEqual(UnaryOpRecord const &lhs, UnaryOpRecord const &rhs)
{
    ASSERT_EQ(lhs.dst,             rhs.dst);
    ASSERT_EQ(lhs.src,             rhs.src);
    ASSERT_EQ(lhs.location,        rhs.location);
    ASSERT_EQ(lhs.dstBlockStride,  rhs.dstBlockStride);
    ASSERT_EQ(lhs.srcBlockStride,  rhs.srcBlockStride);
    ASSERT_EQ(lhs.dstRepeatStride, rhs.dstRepeatStride);
    ASSERT_EQ(lhs.srcRepeatStride, rhs.srcRepeatStride);
    ASSERT_EQ(lhs.repeat,          rhs.repeat);
}

void AssertEqual(BinaryOpRecord const &lhs, BinaryOpRecord const &rhs)
{
    ASSERT_EQ(lhs.dst,              rhs.dst);
    ASSERT_EQ(lhs.src0,             rhs.src0);
    ASSERT_EQ(lhs.src1,             rhs.src1);
    ASSERT_EQ(lhs.location,        rhs.location);
    ASSERT_EQ(lhs.dstBlockStride,   rhs.dstBlockStride);
    ASSERT_EQ(lhs.src0BlockStride,  rhs.src0BlockStride);
    ASSERT_EQ(lhs.src1BlockStride,  rhs.src1BlockStride);
    ASSERT_EQ(lhs.dstRepeatStride,  rhs.dstRepeatStride);
    ASSERT_EQ(lhs.src0RepeatStride, rhs.src0RepeatStride);
    ASSERT_EQ(lhs.src1RepeatStride, rhs.src1RepeatStride);
    ASSERT_EQ(lhs.repeat,           rhs.repeat);
}

void AssertEqual(DmaMovRecord const &lhs, DmaMovRecord const &rhs)
{
    ASSERT_EQ(lhs.dst,             rhs.dst);
    ASSERT_EQ(lhs.src,             rhs.src);
    ASSERT_EQ(lhs.location,        rhs.location);
    ASSERT_EQ(lhs.nBurst,          rhs.nBurst);
    ASSERT_EQ(lhs.lenBurst,        rhs.lenBurst);
    ASSERT_EQ(lhs.srcStride,       rhs.srcStride);
    ASSERT_EQ(lhs.dstStride,       rhs.dstStride);
    ASSERT_EQ(lhs.dstMemType,      rhs.dstMemType);
    ASSERT_EQ(lhs.srcMemType,      rhs.srcMemType);
    ASSERT_EQ(lhs.padMode,         rhs.padMode);
    ASSERT_EQ(lhs.byteMode,        rhs.byteMode);
}

TEST(Recorder, dump_two_records_and_parse_expect_success)
{
    uint64_t blockDim = 3;
    uint64_t cacheSize = 100 * MB_TO_BYTES + sizeof(RecordBlockHead);
    std::vector<uint8_t> memInfo(sizeof(RecordGlobalHead) + cacheSize * blockDim, 0);
    RecordGlobalHead head{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    Recorder handler = Recorder(memInfo.data(), blockDim - 1);
    __gm__  RecordBlockHead* recordBlockHead = reinterpret_cast<__gm__ RecordBlockHead *>(handler.memInfoSimdBlock_);
    uint64_t calDst = 234234234;
    uint64_t calSrc = 745634645;
    struct Location calLoc = {3, 5};
    struct UnaryOpRecord unaryOpVal = {calDst, calSrc, calLoc, 1, 3, 2, 10, 5, 8};
    handler.DumpRecord<RecordType::UNARY_OP>(unaryOpVal);
    ASSERT_EQ(recordBlockHead->recordCount, 1);
    ASSERT_EQ(recordBlockHead->recordWriteCount, 1);
    ASSERT_EQ(recordBlockHead->writeOffset, sizeof(RecordType) + sizeof(UnaryOpRecord));
    uint64_t movDst = 8455325;
    uint64_t movSrc = 43242342;
    struct Location movLoc = {6, 8, 0, 5};
    struct DmaMovRecord dmaMovVal = {movDst, movSrc, movLoc, 32, 126, 15, 17, MemType::GM, \
              MemType::UB, PadMode::PAD_MODE6, ByteMode::BM_DISABLE};
    handler.DumpRecord<RecordType::DMA_MOV>(dmaMovVal);
    ASSERT_EQ(recordBlockHead->recordCount, 2);
    ASSERT_EQ(recordBlockHead->recordWriteCount, 2);
    ASSERT_EQ(recordBlockHead->writeOffset, sizeof(RecordType) * 2 + sizeof(UnaryOpRecord) +\
             sizeof(DmaMovRecord));

    UnaryOpRecord *unaryOp = reinterpret_cast<UnaryOpRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType) + (blockDim - 1) * cacheSize);
    AssertEqual(unaryOpVal, *unaryOp);

    DmaMovRecord *dmaMovOp =  reinterpret_cast<DmaMovRecord *>((reinterpret_cast<uint8_t *>(unaryOp) \
                + sizeof(UnaryOpRecord) + sizeof(RecordType)));
    AssertEqual(dmaMovVal, *dmaMovOp);
}

TEST(Recorder, get_nullptr_expect_eq)
{
    uint64_t blockDim = 8;
    __gm__ uint8_t *memInfo = nullptr;
    uint64_t blockIdx = blockDim - 1;
    Recorder handler = Recorder(memInfo, blockIdx);
    uint64_t calDst = 3453453453;
    uint64_t calSrc = 3464564564;
    struct Location calLoc = {3, 5};
    struct UnaryOpRecord unaryOpVal = {calDst, calSrc, calLoc, 1, 3, 2, 10, 8};
    handler.DumpRecord<RecordType::UNARY_OP>(unaryOpVal);
    ASSERT_EQ(handler.memInfoSimdBlock_, nullptr);
    free(memInfo);
}

TEST(Recorder, dump_two_records_first_normal_and_second_gt_max_sizes_expect_success)
{
    uint64_t blockDim = 2;
    uint64_t cacheSize = 100 * MB_TO_BYTES + sizeof(RecordBlockHead);
    std::vector<uint8_t> memInfo(sizeof(RecordGlobalHead) + cacheSize * blockDim, 0);
    RecordGlobalHead head{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    Recorder handler = Recorder(memInfo.data(), blockDim - 1);
    __gm__  RecordBlockHead* recordBlockHead = reinterpret_cast<__gm__ RecordBlockHead *>(handler.memInfoSimdBlock_);
    uint64_t offsetStart = cacheSize - sizeof(RecordBlockHead) -
        sizeof(BinaryOpRecord) - sizeof(MovAlignRecord) / 2 - CACHE_LINE_SIZE;
    recordBlockHead->writeOffset += offsetStart;
    uint64_t calDst = 7567564764;
    uint64_t calSrc0 = 645645645;
    uint64_t calSrc1 = 354242423;
    struct Location calLoc = {13, 52, 0, 54};
    struct BinaryOpRecord binaryOpVal = {calDst, calSrc0, calSrc1, calLoc, 1, 3, 2, 10, 8, 12, 5};
    handler.DumpRecord<RecordType::BINARY_OP>(binaryOpVal);
    ASSERT_EQ(recordBlockHead->recordCount, 1);
    ASSERT_EQ(recordBlockHead->recordWriteCount, 1);

    uint64_t movAlignDst = 8455325;
    uint64_t movAlignSrc = 43242342;
    struct Location movLoc = {61, 18, 0, 5};
    struct MovAlignRecord movAlignVal = {movAlignDst, movAlignSrc, movLoc, 32, 126, 15, 17, MemType::UB, \
              MemType::L0A, DataType::DATA_B16, 1, 3};
    handler.DumpRecord<RecordType::MOV_ALIGN>(movAlignVal);
    ASSERT_EQ(recordBlockHead->recordCount, 2);
    ASSERT_EQ(recordBlockHead->recordWriteCount, 1);

    BinaryOpRecord *binaryOp = reinterpret_cast<BinaryOpRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType) + offsetStart + (blockDim - 1) * cacheSize);
    AssertEqual(binaryOpVal, *binaryOp);
}

TEST(Recorder, write_records_with_tail_block_and_expect_success)
{
    struct TestTailRecord {
        Location location;
        uint16_t dst;
        uint16_t src;
        uint16_t repeat;
    };
    uint64_t blockDim = 3;
    uint64_t cacheSize = 100 * MB_TO_BYTES + sizeof(RecordBlockHead);
    std::vector<uint8_t> memInfo(sizeof(RecordGlobalHead) + cacheSize * blockDim, 0);
    RecordGlobalHead head{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    Recorder handler = Recorder(memInfo.data(), blockDim - 1);
    __gm__  RecordBlockHead* recordBlockHead = reinterpret_cast<__gm__ RecordBlockHead *>(handler.memInfoSimdBlock_);
    uint16_t calDst = 2342;
    uint16_t calSrc = 6456;
    struct TestTailRecord tailOpVal = {{}, calDst, calSrc, 15};
    // 单元测试里的特殊情况，这里关闭类型检查
    handler.DumpRecord<RecordType::UNARY_OP, TestTailRecord, record_type_check<false>>(tailOpVal);
    ASSERT_EQ(recordBlockHead->recordCount, 1);
    ASSERT_EQ(recordBlockHead->recordWriteCount, 1);

    TestTailRecord *tailOp = reinterpret_cast<TestTailRecord *>(memInfo.data() + sizeof(RecordGlobalHead) + \
        sizeof(RecordBlockHead) + sizeof(RecordType) + (blockDim - 1) * cacheSize);
    ASSERT_EQ(tailOp->dst, calDst);
    ASSERT_EQ(tailOp->src, calSrc);
    ASSERT_EQ(tailOp->repeat, 15);
}

TEST(Recorder, dump_block_data_with_not_equal_block_records_default_cache_size_and_get_none)
{
    uint64_t defaultCacheSize = 100;
    RecordGlobalHead head{};
    uint64_t blockIdx = RandInt(1, 2);
    head.checkParms.checkBlockId = RandInt(0, blockIdx - 1);
    uint64_t totalSize = (defaultCacheSize * MB_TO_BYTES + sizeof(RecordBlockHead)) *
                            blockIdx * C220_MIX_SUB_BLOCKDIM;
    std::vector<uint8_t> memInfo(sizeof(RecordGlobalHead) + totalSize, 0);
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordBlockHead), memInfo.begin());

    SyncRecord record = {
        .location = {10, 10},
        .src      = PipeType::PIPE_V,
        .dst      = PipeType::PIPE_MTE1,
        .eventID  = 1,
    };

    Recorder handler = Recorder(memInfo.data(),
        RandInt(head.checkParms.checkBlockId + 1, blockIdx * C220_MIX_SUB_BLOCKDIM - 1));
    handler.DumpRecord<RecordType::SET_FLAG>(record);
    __gm__  RecordBlockHead* recordBlockHead = reinterpret_cast<__gm__ RecordBlockHead *>(handler.memInfoSimdBlock_);
    ASSERT_EQ(recordBlockHead->recordCount, 0);
    ASSERT_EQ(recordBlockHead->recordWriteCount, 0);
}

TEST(Recorder, dump_block_data_with_not_equal_block_records_set_cache_size_and_get_none)
{
    uint64_t cacheSize = RandInt(1, 10);
    RecordGlobalHead head{};
    uint64_t blockIdx = RandInt(1, 9);
    head.checkParms.checkBlockId = RandInt(0, blockIdx - 1);
    head.checkParms.cacheSize = cacheSize;
    uint64_t totalSize = (cacheSize * MB_TO_BYTES + sizeof(RecordBlockHead)) * blockIdx * C220_MIX_SUB_BLOCKDIM;
    std::vector<uint8_t> memInfo(sizeof(RecordGlobalHead) + totalSize, 0);
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    SyncRecord record = {
        .location = {10, 10},
        .src      = PipeType::PIPE_V,
        .dst      = PipeType::PIPE_MTE1,
        .eventID  = 1,
    };

    Recorder handler = Recorder(memInfo.data(),
        RandInt(head.checkParms.checkBlockId + 1, blockIdx * C220_MIX_SUB_BLOCKDIM - 1));
    handler.DumpRecord<RecordType::SET_FLAG>(record);
    __gm__  RecordBlockHead* recordBlockHead = reinterpret_cast<__gm__ RecordBlockHead *>(handler.memInfoSimdBlock_);
    ASSERT_EQ(recordBlockHead->recordCount, 0);
    ASSERT_EQ(recordBlockHead->recordWriteCount, 0);
}

TEST(Recorder, dump_block_data_with_equal_block_records_dafault_cache_size_and_get_success)
{
    uint64_t cacheSize = 100;
    RecordGlobalHead head{};
    uint64_t blockIdx = RandInt(1, 2);
    head.checkParms.checkBlockId = RandInt(0, blockIdx - 1);
    uint64_t totalSize = (cacheSize * MB_TO_BYTES + sizeof(RecordBlockHead)) * blockIdx * C220_MIX_SUB_BLOCKDIM;
    std::vector<uint8_t> memInfo(sizeof(RecordGlobalHead) + totalSize, 0);
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    SyncRecord record = {
        .location = {10, 10},
        .src      = PipeType::PIPE_V,
        .dst      = PipeType::PIPE_MTE1,
        .eventID  = 1,
    };

    Recorder handler = Recorder(memInfo.data(), head.checkParms.checkBlockId);
    handler.DumpRecord<RecordType::SET_FLAG>(record);
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_FLAG>(handler.memInfoSimdBlock_ + sizeof(RecordBlockHead), record));
}

TEST(Recorder, dump_block_data_with_equal_block_records_set_cache_size_and_get_success)
{
    uint64_t cacheSize = RandInt(1, 10);
    RecordGlobalHead head{};
    uint64_t blockIdx = RandInt(1, 9);
    head.checkParms.checkBlockId = RandInt(0, blockIdx - 1);
    head.checkParms.cacheSize = cacheSize;
    uint64_t totalSize = (cacheSize * MB_TO_BYTES + sizeof(RecordBlockHead)) * blockIdx * C220_MIX_SUB_BLOCKDIM;
    std::vector<uint8_t> memInfo(sizeof(RecordGlobalHead) + totalSize, 0);
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    SyncRecord record = {
        .location = {10, 10},
        .src      = PipeType::PIPE_V,
        .dst      = PipeType::PIPE_MTE1,
        .eventID  = 1,
    };

    Recorder handler = Recorder(memInfo.data(), head.checkParms.checkBlockId);
    handler.DumpRecord<RecordType::SET_FLAG>(record);
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_FLAG>(handler.memInfoSimdBlock_ + sizeof(RecordBlockHead), record));
}

TEST(Recorder, dump_block_data_with_single_check_expect_get_success)
{
    uint64_t cacheSize = RandInt(1, 10);
    RecordGlobalHead head{};
    uint64_t blockIdx = RandInt(1, 9);
    head.checkParms.checkBlockId = RandInt(0, blockIdx - 1);
    head.checkParms.cacheSize = cacheSize;
    uint64_t totalSize = (cacheSize * MB_TO_BYTES + sizeof(RecordBlockHead)) * blockIdx * C220_MIX_SUB_BLOCKDIM;
    std::vector<uint8_t> memInfo(sizeof(RecordGlobalHead) + totalSize, 0);
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    SyncRecord record = {
        .location = {10, 10},
        .src      = PipeType::PIPE_V,
        .dst      = PipeType::PIPE_MTE1,
        .eventID  = 1,
    };

    Recorder handler1 = Recorder(memInfo.data(), head.checkParms.checkBlockId + 1);
    handler1.DumpRecord<RecordType::SET_FLAG>(record);
    RecordBlockHead recordBlockHead = *reinterpret_cast<RecordBlockHead *>(handler1.memInfoSimdBlock_);
    ASSERT_EQ(recordBlockHead.recordWriteCount, 0);
    ASSERT_EQ(recordBlockHead.recordCount, 0);
    Recorder handler2 = Recorder(memInfo.data(), head.checkParms.checkBlockId);
    handler2.DumpRecord<RecordType::SET_FLAG>(record);
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_FLAG>(handler2.memInfoSimdBlock_ + sizeof(RecordBlockHead), record));
    recordBlockHead = *reinterpret_cast<RecordBlockHead *>(handler2.memInfoSimdBlock_);
    ASSERT_EQ(recordBlockHead.recordWriteCount, 1);
    ASSERT_EQ(recordBlockHead.recordCount, 1);
}

TEST(Recorder, dump_ffts_sync_records_with_single_check_expect_get_success)
{
    uint64_t cacheSize = RandInt(1, 10);
    RecordGlobalHead head{};
    uint64_t blockIdx = RandInt(2, 9);
    head.checkParms.checkBlockId = RandInt(1, blockIdx - 1);
    head.checkParms.cacheSize = cacheSize;
    uint64_t totalSize = (cacheSize * MB_TO_BYTES + sizeof(RecordBlockHead)) * blockIdx * C220_MIX_SUB_BLOCKDIM;
    std::vector<uint8_t> memInfo(sizeof(RecordGlobalHead) + totalSize, 0);
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    FftsSyncRecord setRecord = {
        .location  = {10, 10},
        .dst       = PipeType::PIPE_MTE2,
        .mode      = 1,
        .flagID    = 1,
    };

    struct WaitFlagDevRecord waitRecord = {
        .location  = {10, 10},
        .flagID    = 1,
    };

    Recorder handler1 = Recorder(memInfo.data(), 0);
    handler1.DumpRecord<RecordType::FFTS_SYNC>(setRecord);
    RecordBlockHead recordBlockHead = *reinterpret_cast<RecordBlockHead *>(handler1.memInfoSimdBlock_);
    ASSERT_EQ(recordBlockHead.recordWriteCount, 1);
    ASSERT_EQ(recordBlockHead.recordCount, 1);
    handler1.DumpRecord<RecordType::WAIT_FLAG_DEV>(waitRecord);
    recordBlockHead = *reinterpret_cast<RecordBlockHead const*>(handler1.memInfoSimdBlock_);
    ASSERT_EQ(recordBlockHead.recordWriteCount, 2);
    ASSERT_EQ(recordBlockHead.recordCount, 2);
    Recorder handler2 = Recorder(memInfo.data(), head.checkParms.checkBlockId);
    handler2.DumpRecord<RecordType::FFTS_SYNC>(setRecord);
    recordBlockHead = *reinterpret_cast<RecordBlockHead const*>(handler2.memInfoSimdBlock_);
    ASSERT_EQ(recordBlockHead.recordWriteCount, 1);
    ASSERT_EQ(recordBlockHead.recordCount, 1);
    handler2.DumpRecord<RecordType::WAIT_FLAG_DEV>(waitRecord);
    recordBlockHead = *reinterpret_cast<RecordBlockHead const*>(handler2.memInfoSimdBlock_);
    ASSERT_EQ(recordBlockHead.recordWriteCount, 2);
    ASSERT_EQ(recordBlockHead.recordCount, 2);
}
