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


#include "data_process.h"
#include "record_format.h"

namespace Sanitizer {

int RandInt(int min, int max)
{
    static std::mt19937 mersenneEngine(std::random_device{}());
    std::uniform_int_distribution<> dist(min, max);
    return dist(mersenneEngine);
}

std::vector<uint8_t> CreateMemInfo()
{
    std::vector<uint8_t> memInfo(UT_RECORD_BUF_SIZE, 0);
    RecordGlobalHead globalHead{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&globalHead), sizeof(RecordGlobalHead), memInfo.begin());
    RecordBlockHead head{};
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordBlockHead), memInfo.begin() + sizeof(RecordGlobalHead));
    return memInfo;
}

UnaryOpRecord CreateRandomUnaryOpRecord()
{
    UnaryOpRecord record{};
    record.dst = RandInt(0, 0x1000000);
    record.src = RandInt(0, 0x1000000);
    record.dstBlockStride = RandInt(0, 0xFFFF);
    record.srcBlockStride = RandInt(0, 0xFFFF);
    record.dstRepeatStride = RandInt(0, 0xFF);
    record.srcRepeatStride = RandInt(0, 0xFF);
    record.repeat = RandInt(0, 0xFF);
    record.location.fileNo = RandInt(0, 0x100000);
    record.location.lineNo = RandInt(0, 0x1000);
    return record;
}

VgatherRecord CreateRandomVgatherRecord()
{
    VgatherRecord record{};
    record.dst = RandInt(0, 0x1000000);
    std::array<uint64_t, 256> addrLists;
    uint64_t baseAddr = RandInt(0, 0x1000000);
    for (size_t i = 0; i < 256; ++i) {
        addrLists[i] = baseAddr + i;
    }
    record.src = reinterpret_cast<uint64_t>(&addrLists[0]);
    record.dstBlockSize = RandInt(0, 0xFFFF);
    record.srcBlockSize = RandInt(0, 0xFFFF);
    record.offsetAddr = RandInt(0, 0xFFFF);
    record.dstRepeatStride = RandInt(0, 0xFF);
    record.dstBlockNum = RandInt(0, 0xFF);
    record.srcBlockNum = RandInt(0, 0xFF);
    record.dstAlignSize = RandInt(0, 0xFF);
    record.dstBlockStride = RandInt(0, 0xF);
    record.dstRepeat = RandInt(0, 0xF);
    record.dstDataBits = RandInt(0, 0xF);
    record.location.fileNo = RandInt(0, 0x100000);
    record.location.lineNo = RandInt(0, 0x1000);
    return record;
}

VecRegPropCoordOpRecord CreateRandomVecRegPropCoordOpRecord()
{
    constexpr uint8_t maxRegionRange = 7;
    VecRegPropCoordOpRecord record{};
    record.dst = RandInt(0, 0x1000000);
    record.src = RandInt(0, 0x1000000);
    record.dataType = RandInt(0, 1) == 1 ? DataType::DATA_B16 : DataType::DATA_B32;
    record.regionRange = RandInt(0, maxRegionRange);
    record.repeat = RandInt(0, 0xFF);
    record.location.fileNo = RandInt(0, 0x100000);
    record.location.lineNo = RandInt(0, 0x1000);
    return record;
}

BinaryOpRecord CreateRandomBinaryOpRecord()
{
    BinaryOpRecord record{};
    record.dst = RandInt(0, 0x1000000);
    record.src0 = RandInt(0, 0x1000000);
    record.src1 = RandInt(0, 0x1000000);
    record.dstBlockStride = RandInt(0, 0xFF);
    record.src0BlockStride = RandInt(0, 0xFF);
    record.src1BlockStride = RandInt(0, 0xFF);
    record.dstRepeatStride = RandInt(0, 0xFF);
    record.src0RepeatStride = RandInt(0, 0xFF);
    record.src1RepeatStride = RandInt(0, 0xFF);
    record.repeat = RandInt(0, 0xFF);
    record.location.fileNo = RandInt(0, 0x100000);
    record.location.lineNo = RandInt(0, 0x1000);
    return record;
}

ReduceOpRecord CreateRandomReduceOpRecord()
{
    ReduceOpRecord record{};
    record.dst = RandInt(0, 0x1000000);
    record.src = RandInt(0, 0x1000000);
    record.srcBlockStride = RandInt(0, 0xFFFF);
    record.dstRepeatStride = RandInt(0, 0xFFFF);
    record.srcRepeatStride = RandInt(0, 0xFFFF);
    record.repeat = RandInt(0, 0xFF);
    record.location.fileNo = RandInt(0, 0x100000);
    record.location.lineNo = RandInt(0, 0x1000);
    record.location.pc = RandInt(0, 0x1000);
    return record;
}

ReduceV2Record CreateRandomReduceV2Record()
{
    ReduceV2Record record{};
    record.dst = RandInt(0, 0x1000000);
    record.src0 = RandInt(0, 0x1000000);
    record.src1 = RandInt(0, 0x1000000);
    record.repeat = RandInt(0, 0xFFFF);
    record.src0RepeatStride = RandInt(0, 0xFFFF);
    record.src0BlockStride = RandInt(0, 0xFF);
    record.src1RepeatStride = RandInt(0, 0xFF);
    record.location.fileNo = RandInt(0, 0x100000);
    record.location.lineNo = RandInt(0, 0x1000);
    record.location.pc = RandInt(0, 0x1000);
    return record;
}

VecDupRecord CreateRandomVecDupRecord()
{
    VecDupRecord record{};
    record.dst = RandInt(0, 0x1000000);
    record.dstBlockStride = RandInt(0, 0xFFFF);
    record.dstRepeatStride = RandInt(0, 0xFFF);
    record.repeat = RandInt(0, 0xFF);
    record.location.fileNo = RandInt(0, 0x100000);
    record.location.lineNo = RandInt(0, 0x1000);
    return record;
}

CmpMaskRecord CreateRandomCmpMaskRecord()
{
    CmpMaskRecord record{};
    record.addr = RandInt(0, 0x1000000);
    record.size = RandInt(0, 0x1000000);
    record.accessType = AccessType::READ;
    record.location.fileNo = RandInt(0, 0x100000);
    record.location.lineNo = RandInt(0, 0x1000);
    return record;
}

std::vector<UnaryOpRecord> CreateRandomUnaryOpRecords(uint8_t nRecords)
{
    std::vector<UnaryOpRecord> records;
    for (uint8_t i = 0; i < nRecords; ++i) {
        auto record = CreateRandomUnaryOpRecord();
        records.emplace_back(record);
    }
    return records;
}

std::vector<BinaryOpRecord> CreateRandomBinaryOpRecords(uint8_t nRecords)
{
    std::vector<BinaryOpRecord> records;
    for (uint8_t i = 0; i < nRecords; ++i) {
        auto record = CreateRandomBinaryOpRecord();
        records.emplace_back(record);
    }
    return records;
}

std::vector<ReduceOpRecord> CreateRandomReduceOpRecords(uint8_t nRecords)
{
    std::vector<ReduceOpRecord> records;
    for (uint8_t i = 0; i < nRecords; ++i) {
        auto record = CreateRandomReduceOpRecord();
        records.emplace_back(record);
    }
    return records;
}

bool IsEqual(Load2DRecord const &lhs, Load2DRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        (lhs.src == rhs.src) &&
        (lhs.dst == rhs.dst) &&
        (lhs.baseIdx == rhs.baseIdx) &&
        (lhs.srcStride == rhs.srcStride) &&
        (lhs.dstStride == rhs.dstStride) &&
        (lhs.blockSize == rhs.blockSize) &&
        (lhs.dstMemType == rhs.dstMemType) &&
        (lhs.srcMemType == rhs.srcMemType) &&
        (lhs.addrCalMode == rhs.addrCalMode) &&
        (lhs.repeat == rhs.repeat);
}

bool IsEqual(Load2DSparseRecord const &lhs, Load2DSparseRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        (lhs.src0 == rhs.src0) &&
        (lhs.src1 == rhs.src1) &&
        (lhs.dst == rhs.dst) &&
        (lhs.startId == rhs.startId) &&
        (lhs.dstMemType == rhs.dstMemType) &&
        (lhs.srcMemType == rhs.srcMemType) &&
        (lhs.repeat == rhs.repeat);
}

bool IsEqual(Load2DTransposeRecord const &lhs, Load2DTransposeRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        (lhs.src == rhs.src) &&
        (lhs.dst == rhs.dst) &&
        (lhs.indexId == rhs.indexId) &&
        (lhs.srcStride == rhs.srcStride) &&
        (lhs.dstStride == rhs.dstStride) &&
        (lhs.dataType == rhs.dataType) &&
        (lhs.srcMemType == rhs.srcMemType) &&
        (lhs.addrMode == rhs.addrMode) &&
        (lhs.dstFracStride == rhs.dstFracStride) &&
        (lhs.repeat == rhs.repeat);
}

bool IsEqual(DecompressHeaderRecord const &lhs, DecompressHeaderRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        (lhs.src == rhs.src) &&
        (lhs.nBlock == rhs.nBlock) &&
        (lhs.srcMemType == rhs.srcMemType);
}

bool IsEqual(DcPreloadRecord const &lhs, DcPreloadRecord const &rhs)
{
    return lhs.location == rhs.location && (lhs.addr == rhs.addr) && (lhs.offset == rhs.offset) &&
        (lhs.space == rhs.space);
}

bool IsEqual(BroadcastRecord const &lhs, BroadcastRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        (lhs.src == rhs.src) &&
        (lhs.dst == rhs.dst) &&
        (lhs.nBurst == rhs.nBurst) &&
        (lhs.srcGap == rhs.srcGap) &&
        (lhs.dstGap == rhs.dstGap) &&
        (lhs.lenBurst == rhs.lenBurst) &&
        (lhs.srcDataType == rhs.srcDataType) &&
        (lhs.dstDataType == rhs.dstDataType) &&
        (lhs.srcMemType == rhs.srcMemType) &&
        (lhs.dstMemType == rhs.dstMemType) &&
        (lhs.enableRepeat == rhs.enableRepeat);
}

bool IsEqual(LoadB2Record const &lhs, LoadB2Record const &rhs)
{
    return
        lhs.location == rhs.location &&
        (lhs.src == rhs.src) &&
        (lhs.dst == rhs.dst) &&
        (lhs.dataType == rhs.dataType) &&
        (lhs.repeat == rhs.repeat);
}

bool IsEqual(LoadAWinogradRecord const &lhs, LoadAWinogradRecord const &rhs)
{
    return lhs.location == rhs.location &&
    (lhs.src ==  rhs.src) &&
    (lhs.dst ==  rhs.dst) &&
    (int(lhs.dataType) ==  int(rhs.dataType)) &&
    (int(lhs.dstMemType) ==  int(rhs.dstMemType)) &&
    (lhs.dstStartPointK ==  rhs.dstStartPointK) &&
    (lhs.dstStartPointM ==  rhs.dstStartPointM) &&
    (lhs.extStepK ==  rhs.extStepK) &&
    (lhs.extStepM ==  rhs.extStepM) &&
    (lhs.fmSizeH ==  rhs.fmSizeH) &&
    (lhs.fmSizeW ==  rhs.fmSizeW) &&
    (lhs.fmSizeCh ==  rhs.fmSizeCh) &&
    (int(lhs.innerDstGap) ==  int(rhs.innerDstGap));
}

bool IsEqual(LoadBWinogradRecord const &lhs, LoadBWinogradRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        (lhs.src == rhs.src) &&
        (lhs.dst == rhs.dst) &&
        (lhs.dstMemType == rhs.dstMemType) &&
        (lhs.innerDstStride == rhs.innerDstStride) &&
        (lhs.repeat == rhs.repeat);
}
bool IsEqual(Load3DRecord const &lhs, Load3DRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        (lhs.dataType == rhs.dataType) &&
        (lhs.src == rhs.src) &&
        (lhs.dst == rhs.dst) &&
        (lhs.dstMemType == rhs.dstMemType) &&
        (lhs.srcMemType == rhs.srcMemType) &&
        (lhs.fMapW == rhs.fMapW) &&
        (lhs.fMapH == rhs.fMapH) &&
        (lhs.fMapC == rhs.fMapC) &&
        (lhs.filterW == rhs.filterW) &&
        (lhs.filterH == rhs.filterH) &&
        (lhs.matrixRptStride == rhs.matrixRptStride) &&
        (lhs.filterWStride == rhs.filterWStride) &&
        (lhs.filterHStride == rhs.filterHStride) &&
        (lhs.matrixKPos == rhs.matrixKPos) &&
        (lhs.matrixMPos == rhs.matrixMPos) &&
        (lhs.matrixKStep == rhs.matrixKStep) &&
        (lhs.matrixMStep == rhs.matrixMStep) &&
        (lhs.matrixRptTimes == rhs.matrixRptTimes) &&
        (lhs.fMapTopPad == rhs.fMapTopPad) &&
        (lhs.fMapBottomPad == rhs.fMapBottomPad) &&
        (lhs.fMapLeftPad == rhs.fMapLeftPad) &&
        (lhs.fMapRightPad == rhs.fMapRightPad) &&
        (lhs.filterWDilation == rhs.filterWDilation) &&
        (lhs.filterHDilation == rhs.filterHDilation) &&
        (lhs.matrixMode == rhs.matrixMode) &&
        (lhs.matrixRptMode == rhs.matrixRptMode);
}

bool IsEqual(Set2DRecord const &lhs, Set2DRecord const &rhs)
{
    return
            lhs.location == rhs.location &&
            (lhs.dst == rhs.dst) &&
            (lhs.repeat == rhs.repeat) &&
            (lhs.dstBlockNum == rhs.dstBlockNum) &&
            (lhs.repeatGap == rhs.repeatGap);
}

bool IsEqual(LoadImageRecord const &lhs, LoadImageRecord const &rhs)
{
    return
           lhs.location == rhs.location &&
           (lhs.dst == rhs.dst) &&
           (lhs.horSize == rhs.horSize) &&
           (lhs.verSize == rhs.verSize) &&
           (lhs.horStartP == rhs.horStartP) &&
           (lhs.verStartP == rhs.verStartP) &&
           (lhs.sHorRes == rhs.sHorRes) &&
           (lhs.lPadSize == rhs.lPadSize) &&
           (lhs.rPadSize == rhs.rPadSize) &&
           (lhs.topPadSize == rhs.topPadSize) &&
           (lhs.botPadSize == rhs.botPadSize) &&
           (lhs.dataType == rhs.dataType) &&
           (lhs.dstMemType == rhs.dstMemType);
}

bool IsEqual(LoadSmaskRecord const &lhs, LoadSmaskRecord const &rhs)
{
    return
            lhs.location == rhs.location &&
            (lhs.dst == rhs.dst) &&
            (lhs.src == rhs.src) &&
            (lhs.srcMemType == rhs.srcMemType) &&
            (lhs.smaskSize == rhs.smaskSize);
}

bool IsEqual(UnaryOpRecord const &lhs, UnaryOpRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        (lhs.src == rhs.src) &&
        (lhs.dst == rhs.dst) &&
        (lhs.dstBlockStride == rhs.dstBlockStride) &&
        (lhs.srcBlockStride == rhs.srcBlockStride) &&
        (lhs.dstRepeatStride == rhs.dstRepeatStride) &&
        (lhs.srcRepeatStride == rhs.srcRepeatStride) &&
        (lhs.repeat == rhs.repeat);
}

bool IsEqual(BinaryOpRecord const &lhs, BinaryOpRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        (lhs.src0 == rhs.src0) &&
        (lhs.src1 == rhs.src1) &&
        (lhs.dst == rhs.dst) &&
        (lhs.dstBlockStride == rhs.dstBlockStride) &&
        (lhs.src0BlockStride == rhs.src0BlockStride) &&
        (lhs.src1BlockStride == rhs.src1BlockStride) &&
        (lhs.dstRepeatStride == rhs.dstRepeatStride) &&
        (lhs.src0RepeatStride == rhs.src0RepeatStride) &&
        (lhs.src1RepeatStride == rhs.src1RepeatStride) &&
        (lhs.repeat == rhs.repeat);
}

bool IsEqual(ReduceOpRecord const &lhs, ReduceOpRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        (lhs.src == rhs.src) &&
        (lhs.dst == rhs.dst) &&
        (lhs.srcBlockStride == rhs.srcBlockStride) &&
        (lhs.dstRepeatStride == rhs.dstRepeatStride) &&
        (lhs.srcRepeatStride == rhs.srcRepeatStride) &&
        (lhs.repeat == rhs.repeat);
}

bool IsEqual(UnaryOpRecord const &lhs, BinaryOpRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        (lhs.src == rhs.src0) &&
        (lhs.dst == rhs.src1) &&
        (lhs.dst == rhs.dst) &&
        (lhs.dstBlockStride == rhs.dstBlockStride) &&
        (lhs.srcBlockStride == rhs.src0BlockStride) &&
        (lhs.dstBlockStride == rhs.src1BlockStride) &&
        (lhs.dstRepeatStride == rhs.dstRepeatStride) &&
        (lhs.srcRepeatStride == rhs.src0RepeatStride) &&
        (lhs.dstRepeatStride == rhs.src1RepeatStride) &&
        (lhs.repeat == rhs.repeat);
}

bool IsEqual(DmaMovRecord const &lhs, DmaMovRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        (lhs.src == rhs.src) &&
        (lhs.dst == rhs.dst) &&
        (lhs.nBurst == rhs.nBurst) &&
        (lhs.lenBurst == rhs.lenBurst) &&
        (lhs.srcStride == rhs.srcStride) &&
        (lhs.dstStride == rhs.dstStride) &&
        (lhs.dstMemType == rhs.dstMemType) &&
        (lhs.srcMemType == rhs.srcMemType) &&
        (lhs.padMode == rhs.padMode) &&
        (lhs.byteMode == rhs.byteMode);
}

bool IsEqual(DmaMovConvReluRecord const &lhs, DmaMovConvReluRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        (lhs.src == rhs.src) &&
        (lhs.dst == rhs.dst) &&
        (lhs.nBurst == rhs.nBurst) &&
        (lhs.lenBurst == rhs.lenBurst) &&
        (lhs.srcStride == rhs.srcStride) &&
        (lhs.dstStride == rhs.dstStride) &&
        (lhs.crMode == rhs.crMode) &&
        (lhs.dstMemType == rhs.dstMemType) &&
        (lhs.srcMemType == rhs.srcMemType) &&
        (lhs.srcDataType == rhs.srcDataType) &&
        (lhs.dstDataType == rhs.dstDataType);
}

bool IsEqual(DmaMovNd2nzRecord const &lhs, DmaMovNd2nzRecord const &rhs)
{
    return
        (lhs.dst == rhs.dst) &&
        (lhs.src == rhs.src) &&
        lhs.location == rhs.location &&

        (lhs.ndNum == rhs.ndNum) &&
        (lhs.nValue == rhs.nValue) &&
        (lhs.dValue == rhs.dValue) &&
        (lhs.srcNdMatrixStride == rhs.srcNdMatrixStride) &&
        (lhs.srcDValue == rhs.srcDValue) &&
        (lhs.dstNzC0Stride == rhs.dstNzC0Stride) &&
        (lhs.dstNzNStride == rhs.dstNzNStride) &&
        (lhs.dstNzMatrixStride == rhs.dstNzMatrixStride) &&

        (lhs.srcMemType == rhs.srcMemType) &&
        (lhs.dstMemType == rhs.dstMemType) &&
        (lhs.dataType == rhs.dataType);
}

bool IsEqual(MovAlignRecord const &lhs, MovAlignRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        (lhs.src == rhs.src) &&
        (lhs.dst == rhs.dst) &&
        (lhs.srcGap == rhs.srcGap) &&
        (lhs.dstGap == rhs.dstGap) &&
        (lhs.lenBurst == rhs.lenBurst) &&
        (lhs.nBurst == rhs.nBurst) &&
        (lhs.dstMemType == rhs.dstMemType) &&
        (lhs.srcMemType == rhs.srcMemType) &&
        (lhs.dataType == rhs.dataType) &&
        (lhs.leftPaddingNum == rhs.leftPaddingNum) &&
        (lhs.rightPaddingNum == rhs.rightPaddingNum);
}

bool IsEqual(MovAlignRecordV2 const &lhs, MovAlignRecordV2 const &rhs)
{
    return
        lhs.location == rhs.location &&
        (lhs.src == rhs.src) &&
        (lhs.dst == rhs.dst) &&
        (lhs.lenBurst == rhs.lenBurst) &&
        (lhs.nBurst == rhs.nBurst) &&
        (lhs.dstMemType == rhs.dstMemType) &&
        (lhs.srcMemType == rhs.srcMemType) &&
        (lhs.dataType == rhs.dataType) &&
        (lhs.leftPaddingNum == rhs.leftPaddingNum) &&
        (lhs.rightPaddingNum == rhs.rightPaddingNum) &&
        (lhs.dstStride == rhs.dstStride) &&
        (lhs.srcStride == rhs.srcStride) &&
        (lhs.loop1Size == rhs.loop1Size) &&
        (lhs.loop2Size == rhs.loop2Size) &&
        (lhs.loop1DstStride == rhs.loop1DstStride) &&
        (lhs.loop1SrcStride == rhs.loop1SrcStride) &&
        (lhs.loop2DstStride == rhs.loop2DstStride) &&
        (lhs.loop2SrcStride == rhs.loop2SrcStride);
}

bool IsEqual(MovBtRecord const &lhs, MovBtRecord const &rhs)
{
    return
            lhs.location == rhs.location &&
            (lhs.src == rhs.src) &&
            (lhs.dst == rhs.dst) &&
            (lhs.srcGap == rhs.srcGap) &&
            (lhs.dstGap == rhs.dstGap) &&
            (lhs.lenBurst == rhs.lenBurst) &&
            (lhs.nBurst == rhs.nBurst) &&
            (lhs.dstMemType == rhs.dstMemType) &&
            (lhs.srcMemType == rhs.srcMemType);
}

bool IsEqual(MovFpRecord const &lhs, MovFpRecord const &rhs)
{
    return
        (lhs.dst == rhs.dst) &&
        (lhs.src == rhs.src) &&
        lhs.location == rhs.location &&
        (lhs.dstStride == rhs.dstStride) &&
        (lhs.srcStride == rhs.srcStride) &&
        (lhs.nSize == rhs.nSize) &&
        (lhs.mSize == rhs.mSize) &&
        (lhs.ndNum == rhs.ndNum) &&
        (lhs.dstNdStride == rhs.dstNdStride) &&
        (lhs.srcNdStride == rhs.srcNdStride) &&
        (lhs.srcNzC0Stride == rhs.srcNzC0Stride) &&
        (lhs.quantPreBits == rhs.quantPreBits) &&
        (lhs.enUnitFlag == rhs.enUnitFlag) &&
        (lhs.int8ChannelMerge == rhs.int8ChannelMerge) &&
        (lhs.int4ChannelMerge == rhs.int4ChannelMerge) &&
        (lhs.channelSplit == rhs.channelSplit) &&
        (lhs.enNZ2ND == rhs.enNZ2ND) &&
        (lhs.enNZ2DN == rhs.enNZ2DN);
}

bool IsEqual(VecDupRecord const &lhs, VecDupRecord const &rhs, uint8_t bitsSize)
{
    uint64_t dst = 0;

    if (bitsSize == UT_VECTOR_DUP_B16_BITS) {
        dst = rhs.dst & 0x0000FFFF;
    } else {
        dst = rhs.dst & 0xFFFFFFFF;
    }
    return
        lhs.location == rhs.location &&
        (lhs.dst == dst) &&
        (lhs.dstBlockStride == rhs.dstBlockStride) &&
        (lhs.dstRepeatStride == rhs.dstRepeatStride) &&
        (lhs.repeat == rhs.repeat);
}

bool IsEqual(HardSyncRecord const &lhs, HardSyncRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        lhs.src == rhs.src &&
        lhs.dst == rhs.dst &&
        lhs.eventID == rhs.eventID &&
        lhs.memory == rhs.memory &&
        lhs.v == rhs.v;
}

bool IsEqual(SyncRecord const &lhs, SyncRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        lhs.src == rhs.src &&
        lhs.dst == rhs.dst &&
        lhs.eventID == rhs.eventID;
}

bool IsEqual(PipeBarrierRecord const &lhs, PipeBarrierRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        lhs.pipe == rhs.pipe;
}

bool IsEqual(LoadStoreRecord const &lhs, LoadStoreRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        lhs.addr == rhs.addr &&
        lhs.size == rhs.size &&
        lhs.space == rhs.space;
}

bool IsEqual(FftsSyncRecord const &lhs, FftsSyncRecord const &rhs)
{
    return
        lhs.location == rhs.location &&
        lhs.dst == rhs.dst &&

        lhs.mode == rhs.mode &&
        lhs.flagID == rhs.flagID;
}

bool IsEqual(WaitFlagDevRecord const& lhs, WaitFlagDevRecord const& rhs)
{
    return
        lhs.location == rhs.location &&
        lhs.flagID == rhs.flagID;
}

bool IsEqual(WaitFlagDevPipeRecord const& lhs, WaitFlagDevPipeRecord const& rhs)
{
    return
        lhs.location == rhs.location &&
        lhs.pipe == rhs.pipe &&
        lhs.flagID == rhs.flagID;

}

bool IsEqual(VecRegPropCoordOpRecord const &lhs, VecRegPropCoordOpRecord const &rhs)
{
    return lhs.location == rhs.location &&
           lhs.dataType == rhs.dataType &&
           lhs.dst == rhs.dst &&
           lhs.src == rhs.src &&
           lhs.repeat == rhs.repeat &&
           lhs.regionRange == rhs.regionRange;
}

bool IsEqual(SoftSyncRecord const& lhs, SoftSyncRecord const& rhs)
{
    return
        lhs.location == rhs.location &&
        lhs.waitCoreID == rhs.waitCoreID &&
        lhs.usedCores == rhs.usedCores &&
        lhs.eventID == rhs.eventID &&
        lhs.isAIVOnly == rhs.isAIVOnly;
}

bool BlockSizeNumIsEqual(CheckBinaryOpBlockInfo &blockInfo, BinaryOpRecord const &rhs)
{
    return
        (blockInfo.dstBlockNum == rhs.dstBlockNum) &&
        (blockInfo.src0BlockNum == rhs.src0BlockNum) &&
        (blockInfo.src1BlockNum == rhs.src1BlockNum) &&
        (blockInfo.dstBlockSize == rhs.dstBlockSize) &&
        (blockInfo.src0BlockSize == rhs.src0BlockSize) &&
        (blockInfo.src1BlockSize == rhs.src1BlockSize);
}

bool BlockSizeNumIsEqual(CheckUnaryOpBlockInfo &blockInfo, UnaryOpRecord const &rhs)
{
    return
        (blockInfo.dstBlockNum == rhs.dstBlockNum) &&
        (blockInfo.srcBlockNum == rhs.srcBlockNum) &&
        (blockInfo.dstBlockSize == rhs.dstBlockSize) &&
        (blockInfo.srcBlockSize == rhs.srcBlockSize);
}

bool BlockSizeNumIsEqual(CheckReduceOpBlockInfo &blockInfo, ReduceOpRecord const &rhs)
{
    return
        (blockInfo.dstBlockNum == rhs.dstBlockNum) &&
        (blockInfo.srcBlockNum == rhs.srcBlockNum) &&
        (blockInfo.dstBlockSize == rhs.dstBlockSize) &&
        (blockInfo.srcBlockSize == rhs.srcBlockSize);
}

bool IsEqual(MstxCrossRecord const& lhs, MstxCrossRecord const& rhs)
{
    return
        lhs.addr == rhs.addr &&
        lhs.flagId == rhs.flagId &&
        lhs.pipe == rhs.pipe &&
        lhs.isMore == rhs.isMore &&
        lhs.isMerge == rhs.isMerge;
}

bool IsEqual(MstxRecord const& lhs, MstxRecord const& rhs)
{
    bool interfaceEqual = false;
    if (lhs.interfaceType == InterfaceType::MSTX_SET_CROSS_SYNC ||
        lhs.interfaceType == InterfaceType::MSTX_WAIT_CROSS_SYNC) {
        interfaceEqual = IsEqual(lhs.interface.mstxCrossRecord, rhs.interface.mstxCrossRecord);
    }
    return
        lhs.location == rhs.location &&
        lhs.interfaceType == rhs.interfaceType &&
        lhs.bufferLens == rhs.bufferLens &&
        lhs.error == rhs.error &&
        interfaceEqual;
}

uint64_t ExtractConfigFromUnaryOpRecord(const UnaryOpRecord &record)
{
    constexpr uint64_t repeatShift = 56;
    constexpr uint64_t srcBlockStrideShift = 16;
    constexpr uint64_t dstRepeatStrideShift = 32;
    constexpr uint64_t srcRepeatStrideShift = 40;
    uint64_t config = (((static_cast<uint64_t>(record.repeat) & 0xff) << repeatShift) |
                       (static_cast<uint64_t>(record.dstBlockStride) & 0xffff) |
                       ((static_cast<uint64_t>(record.srcBlockStride) & 0xffff) << srcBlockStrideShift) |
                       ((static_cast<uint64_t>(record.dstRepeatStride) & 0xff) << dstRepeatStrideShift) |
                       ((static_cast<uint64_t>(record.srcRepeatStride) & 0xfff) << srcRepeatStrideShift));

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    constexpr uint64_t dstRepeatStrideShift2 = 44;
    config = config | ((static_cast<uint64_t>(record.dstRepeatStride) & 0xf00) << dstRepeatStrideShift2);
#endif

    return config;
}

uint64_t ExtractConfigFromUnaryOpRecordWithOffset(const UnaryOpRecord &record, uint32_t offset)
{
    constexpr uint64_t repeatShift = 56;
    constexpr uint64_t dstRepeatStrideShift = 32;
    constexpr uint64_t srcRepeatStrideShift = 40;
    uint64_t config = (((static_cast<uint64_t>(record.repeat) & 0xff) << repeatShift) |
                       (static_cast<uint64_t>(offset) & 0xffff) |
                       ((static_cast<uint64_t>(record.dstRepeatStride) & 0xff) << dstRepeatStrideShift) |
                       ((static_cast<uint64_t>(record.srcRepeatStride) & 0xfff) << srcRepeatStrideShift));
    return config;
}

uint64_t ExtractConfigFromBinaryOpRecord(const BinaryOpRecord &record)
{
    constexpr uint64_t repeatShift = 56;
    constexpr uint64_t src0BlockStrideShift = 8;
    constexpr uint64_t src1BlockStrideShift = 16;
    constexpr uint64_t dstRepeatStrideShift = 24;
    constexpr uint64_t src0RepeatStrideShift = 32;
    constexpr uint64_t src1RepeatStrideShift = 40;
    uint64_t config = (((static_cast<uint64_t>(record.repeat) & 0xff) << repeatShift) |
                       (static_cast<uint64_t>(record.dstBlockStride) & 0xff) |
                       ((static_cast<uint64_t>(record.src0BlockStride) & 0xff) << src0BlockStrideShift) |
                       ((static_cast<uint64_t>(record.src1BlockStride) & 0xff) << src1BlockStrideShift) |
                       ((static_cast<uint64_t>(record.dstRepeatStride) & 0xff) << dstRepeatStrideShift) |
                       ((static_cast<uint64_t>(record.src0RepeatStride) & 0xff) << src0RepeatStrideShift) |
                       ((static_cast<uint64_t>(record.src1RepeatStride) & 0xff) << src1RepeatStrideShift));
    return config;
}

uint64_t ExtractConfigFromReduceOpRecord(const ReduceOpRecord &record)
{
    constexpr uint64_t repeatShift = 56;
    constexpr uint64_t srcBlockStrideShift = 16;
    constexpr uint64_t srcRepeatStrideShift = 32;
    uint64_t config = (((static_cast<uint64_t>(record.repeat) & 0xff) << repeatShift) |
                       (static_cast<uint64_t>(record.dstRepeatStride) & 0xffff) |
                       ((static_cast<uint64_t>(record.srcBlockStride) & 0xffff) << srcBlockStrideShift) |
                       ((static_cast<uint64_t>(record.srcRepeatStride) & 0xffff) << srcRepeatStrideShift));
    return config;
}

bool IsEqual(NdDMAOut2UbRecord const& lhs, NdDMAOut2UbRecord const& rhs)
{
    return
        lhs.dst == rhs.dst &&
        lhs.src == rhs.src &&
        lhs.location == rhs.location &&
        lhs.loop[0] == rhs.loop[0] &&
        lhs.loop[1] == rhs.loop[1] &&
        lhs.loop[2] == rhs.loop[2] &&
        lhs.loop[3] == rhs.loop[3] &&
        lhs.loop[4] == rhs.loop[4] &&
        lhs.dataType == rhs.dataType;
}

bool IsEqual(DmaMovNd2nzDavRecord const& lhs, DmaMovNd2nzDavRecord const& rhs)
{
    return
        lhs.dst == rhs.dst &&
        lhs.src == rhs.src &&
        lhs.location == rhs.location &&
        lhs.loop4SrcStride == rhs.loop4SrcStride &&
        lhs.loop1SrcStride == rhs.loop1SrcStride &&
        lhs.dValue == rhs.dValue &&
        lhs.nValue == rhs.nValue &&
        lhs.ndNum == rhs.ndNum &&
        lhs.loop2DstStride == rhs.loop2DstStride &&
        lhs.loop3DstStride == rhs.loop3DstStride &&
        lhs.loop4DstStride == rhs.loop4DstStride &&
        lhs.smallC0 == rhs.smallC0 &&
        lhs.srcMemType == rhs.srcMemType &&
        lhs.dstMemType == rhs.dstMemType &&
        lhs.dataType == rhs.dataType;
}

bool IsEqual(MovL1UBRecord const &lhs, MovL1UBRecord const &rhs)
{
    return lhs == rhs;
}

bool IsEqual(MovL1BtRecord const& lhs, MovL1BtRecord const& rhs)
{
    return
        lhs.location == rhs.location &&
        lhs.dst == rhs.dst &&
        lhs.src == rhs.src &&
        lhs.nBurst == rhs.nBurst &&
        lhs.lenBurst == rhs.lenBurst &&
        lhs.srcGap == rhs.srcGap &&
        lhs.dstGap == rhs.dstGap &&
        lhs.cvtEnable == rhs.cvtEnable;
}

bool IsEqual(MovL1FbRecord const& lhs, MovL1FbRecord const& rhs)
{
    return
        lhs.location == rhs.location &&
        lhs.dst == rhs.dst &&
        lhs.src == rhs.src &&
        lhs.nBurst == rhs.nBurst &&
        lhs.lenBurst == rhs.lenBurst &&
        lhs.srcStride == rhs.srcStride &&
        lhs.dstStride == rhs.dstStride &&
        lhs.dstMemBlock == rhs.dstMemBlock;
}

bool IsEqual(IntraBlockSyncRecord const& lhs, IntraBlockSyncRecord const& rhs)
{
    return
        lhs.location == rhs.location &&
        lhs.pipe == rhs.pipe &&
        lhs.syncID == rhs.syncID;
}

bool IsEqual(RegisterSetRecord const& lhs, RegisterSetRecord const& rhs)
{
    return
        lhs.location == rhs.location &&
        lhs.regPayLoad.regValType == rhs.regPayLoad.regValType &&
        lhs.regPayLoad.regVal == rhs.regPayLoad.regVal;
}

}  // namespace Sanitizer
