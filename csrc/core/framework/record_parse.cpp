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


#include "record_parse.h"

#include <functional>
#include <map>
#include <utility>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include "record_format.h"
#include "constant.h"
#include "record_defs.h"
#include "utility/log.h"
#include "utility/numeric.h"
#include "platform_config.h"
#include "format_converter.h"
#include "address_sanitizer/align_checker.h"
#include "record_parse_ctrl.h"
#include "record_parse_calc.h"
#include "record_parse_move.h"

namespace Sanitizer {

static bool IsValidHardSyncRecord(const KernelRecord &record)
{
    if (record.blockType == BlockType::AIVEC) {
        return false;
    }
    auto& hardSyncRecord = record.payload.hardSyncRecord;
    if (hardSyncRecord.src != PipeType::PIPE_MTE1 && hardSyncRecord.src != PipeType::PIPE_M &&
        hardSyncRecord.src != PipeType::PIPE_FIX) {
        return false;
    }
    if (hardSyncRecord.dst != PipeType::PIPE_MTE1 && hardSyncRecord.dst != PipeType::PIPE_M &&
        hardSyncRecord.dst != PipeType::PIPE_FIX) {
        return false;
    }
    if (hardSyncRecord.memory == MemType::GM || hardSyncRecord.memory == MemType::UB ||
        hardSyncRecord.memory == MemType::INVALID) {
        return false;
    }
    if (hardSyncRecord.v > 1) {
        return false;
    }
    if (hardSyncRecord.eventID > 3) {
        return false;
    }
    return true;
}

template <typename Payload>
SanEvent CreateCrossPipeSyncEvent(const SyncType syncType, const PipeType pipeSrc, const PipeType pipeDst,
    const KernelRecord &record, const Payload &payload)
{
    // 一般EVENT_ID取值0~7，这里是为了与正常set_flag/wait_flag区分开
    constexpr uint32_t eventID11 = 11U;
    SanEvent event;
    SetLocationInfo(event, payload, record.blockType, record.serialNo);
    event.type = EventType::SYNC_EVENT;
    event.pipe = syncType == SyncType::SET_FLAG ? pipeSrc : pipeDst;
    event.eventInfo.syncInfo.opType = syncType;
    event.eventInfo.syncInfo.srcPipe = pipeSrc;
    event.eventInfo.syncInfo.dstPipe = pipeDst;
    event.eventInfo.syncInfo.eventId = eventID11;
    event.eventInfo.syncInfo.memType = MemType::INVALID;
    event.eventInfo.syncInfo.isGenerated = true;
    return event;
}

template <typename Payload>
SanEvent CreateInnerPipeSyncEvent(const KernelRecord &record, const PipeType pipeType, const Payload &payload)
{
    SanEvent event;
    SetLocationInfo(event, payload, record.blockType, record.serialNo);
    event.type = EventType::SYNC_EVENT;
    event.pipe = pipeType;
    event.eventInfo.syncInfo.opType = SyncType::PIPE_BARRIER;
    event.eventInfo.syncInfo.memType = MemType::INVALID;
    event.eventInfo.syncInfo.isRetrogress = false;
    event.eventInfo.syncInfo.isGenerated = true;
    return event;
}

template <typename Payload>
void CreatePipeAllSyncEvent(const KernelRecord &record, std::vector<SanEvent> &events, const Payload &payload)
{
    for (auto i = EnumToUnderlying(PipeType::PIPE_V);
         i < EnumToUnderlying(PipeType::SIZE); i++) {
        // 流水线内同步
        SanEvent barrierEvent = CreateInnerPipeSyncEvent(record, static_cast<PipeType>(i), payload);
        events.emplace_back(barrierEvent);

        // 流水线间同步
        SanEvent setEvent = CreateCrossPipeSyncEvent(
            SyncType::SET_FLAG, static_cast<PipeType>(i), PipeType::PIPE_S, record, payload);
        SanEvent waitEvent = CreateCrossPipeSyncEvent(
            SyncType::WAIT_FLAG, static_cast<PipeType>(i), PipeType::PIPE_S, record, payload);
        events.emplace_back(setEvent);
        events.emplace_back(waitEvent);
    }
}

/* unit-flag用于CUBE指令（如MMAD）和MTE指令（如MOV_L0C_TO_OUT）之间的同步
 *   相较于set_flag/wait_flag这种指令级别的同步：
 *     PIPE_M     |___________2048B___________|
 *     PIPE_FIX                               |___________2048B___________|
 *   unit-flag能提供基于内存块的更细粒度同步：
 *     PIPE_M     |_512B_|_512B_|_512B_|_512B_|
 *     PIPE_FIX          |_512B_|_512B_|_512B_|_512B_|
 * 通过一个uint8_t的参数unitFlag来说明unit-flag的使能情况，一共有2种使能的模式：
 *   unitFlag = 2 时，对于写操作（CUBE指令），unit-flag为0时才会直接写，如果是1就阻塞等待
 *                    对于读操作（MTE指令），unit-flag为1时才直接读，如果是0就阻塞等待
 *   unitFlag = 3 时，对于写操作（CUBE指令），unit-flag为0时才会直接写，如果是1就阻塞等待。写完后翻转unit-flag
 *                    对于读操作（MTE指令），unit-flag为1时才直接读，如果是0就阻塞等待。读完后翻转unit-flag
 * unitFlag无论是2还是3，都可以避免CUBE指令和MTE指令的读写竞争，因此用enUnitFlag（bool）统一表示，
 * 只要enUnitFlag = true，就插入一对set_flag/wait_flag，表明不会产生竞争
*/
template <typename Payload>
void CreateUnitFlagSyncEvent(const KernelRecord &record, std::vector<SanEvent> &events,
    const Payload &payload, bool isWriteMode)
{
    PipeType srcPipe = isWriteMode ? PipeType::PIPE_M : PipeType::PIPE_FIX;
    PipeType dstPipe = isWriteMode ? PipeType::PIPE_FIX : PipeType::PIPE_M;
    SanEvent setEvent = CreateCrossPipeSyncEvent(SyncType::SET_FLAG, srcPipe, dstPipe, record, payload);
    SanEvent waitEvent = CreateCrossPipeSyncEvent(SyncType::WAIT_FLAG, srcPipe, dstPipe, record, payload);
    events.emplace_back(setEvent);
    events.emplace_back(waitEvent);
}

static void ParseScalarOpRecord(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& loadStoreRecord = record.payload.loadStoreRecord;
    SetLocationInfo(event, loadStoreRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_S_CAL;
    memInfo.memType = FormatConverter::AddrSpaceToMemType(loadStoreRecord.space);

    if (record.recordType == RecordType::LOAD || record.recordType == RecordType::LD || record.recordType == RecordType::LD_IO ||
        record.recordType == RecordType::LDP || record.recordType == RecordType::LD_DEV) {
        memInfo.opType = AccessType::READ;
    } else {
        memInfo.opType = AccessType::WRITE;
    }

    memInfo.vectorMask = {static_cast<uint64_t>(-1), static_cast<uint64_t>(-1)};
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.addr = loadStoreRecord.addr;
    memInfo.blockNum = 1U;
    memInfo.blockSize = loadStoreRecord.size;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = 1U;
    memInfo.repeatStride = 1U;
    memInfo.alignSize = loadStoreRecord.alignSize;
    AlignChecker::Instance().CheckAlign(event, loadStoreRecord.alignSize);
    events.emplace_back(event);
}

static void ParseRedAndAtomRecord(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& redRecord = record.payload.redRecord;
    SetLocationInfo(event, redRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_S;
    memInfo.memType = MemType::GM;
    memInfo.opType = AccessType::MEMCPY_BLOCKS;
    memInfo.vectorMask = {static_cast<uint64_t>(-1), static_cast<uint64_t>(-1)};
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.addr = redRecord.addr;
    memInfo.blockNum = 1U;
    memInfo.blockSize = redRecord.size;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = 1U;
    memInfo.repeatStride = 1U;
    memInfo.alignSize = redRecord.size;
    AlignChecker::Instance().CheckAlign(event, redRecord.size);
    events.emplace_back(event);
}

static void ParseLdvaRecord(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event{};
    auto& memInfo = event.eventInfo.memInfo;
    auto& loadStoreRecord = record.payload.loadStoreRecord;
    SetLocationInfo(event, loadStoreRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    memInfo.memType = MemType::UB;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = {static_cast<uint64_t>(-1), static_cast<uint64_t>(-1)};
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.addr = loadStoreRecord.addr;
    memInfo.blockNum = 1U;
    memInfo.blockSize = loadStoreRecord.size;
    memInfo.blockStride = 0;
    memInfo.repeatTimes = 1U;
    memInfo.repeatStride = 0;
    memInfo.alignSize = loadStoreRecord.alignSize;
    AlignChecker::Instance().CheckAlign(event, loadStoreRecord.alignSize);
    events.emplace_back(event);
}

static void ParseRecordDmaMovImpl(const KernelRecord &record, std::vector<SanEvent> &events, bool alignCheck)
{
    // 搬运模式为A      => B,这里将A/B看成2个内存事件
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& dmaMovRecord = record.payload.dmaMovRecord;
    PipeType pipe = FormatConverter::QueryPipeType(dmaMovRecord.srcMemType, dmaMovRecord.dstMemType);
    if (pipe == PipeType::SIZE) {
        SAN_ERROR_LOG("serialNo: %lu parse failed, pipe has exceeded the maximum limit", record.serialNo);
        return;
    }
    SetLocationInfo(event, dmaMovRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = pipe;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;

    if (dmaMovRecord.byteMode == ByteMode::BM_ENABLE) {
        ParseDmaMovWithByteMode(dmaMovRecord, events, event);
        return;
    }

    if (dmaMovRecord.padMode != PadMode::PAD_NONE) {
        ParseDmaMovWithPadMode(dmaMovRecord, events, event);
        return;
    }

    memInfo.memType = dmaMovRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.addr = dmaMovRecord.src;
    memInfo.blockNum = dmaMovRecord.lenBurst;
    memInfo.blockSize = MOV_LOCAL_BLOCK_SIZE;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = dmaMovRecord.nBurst;
    memInfo.repeatStride = dmaMovRecord.lenBurst + dmaMovRecord.srcStride;
    if (alignCheck) {
        AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV);
    }
    events.emplace_back(event);

    memInfo.memType = dmaMovRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = dmaMovRecord.dst;
    memInfo.repeatStride = dmaMovRecord.lenBurst + dmaMovRecord.dstStride;
    if (alignCheck) {
        AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV);
    }
    events.emplace_back(event);
}

static void ParseRecordDmaMov(const KernelRecord &record, std::vector<SanEvent> &events)
{
    ParseRecordDmaMovImpl(record, events, true);
}

static bool IsSupportFor16(const DmaMovConvReluRecord& record)
{
    bool isSupport = record.crMode == ConvRelu::CRMODE_NONE ||
           record.crMode == ConvRelu::CRMODE_NONE_RELU ||
           record.crMode == ConvRelu::CRMODE_F16_MUL;
    return isSupport;
}

enum class SupportFor32 : uint8_t {
    NOT_SUPPORT32 = 0,
    SUPPORT32TO32,
    SUPPORT32TO16,
    SUPPORT32TO8,
};

static SupportFor32 IsSupportFor32(const DmaMovConvReluRecord& dmaMovConvReluRecord)
{
    SupportFor32 isSupport = SupportFor32::NOT_SUPPORT32;
    const std::vector<ConvRelu> support32To32 = {ConvRelu::CRMODE_NONE, ConvRelu::CRMODE_NONE_RELU};
    const std::vector<ConvRelu> support32To16 = {ConvRelu::CRMODE_F32toF16_NONE,
                                                 ConvRelu::CRMODE_F32toF16_RELU,
                                                 ConvRelu::CRMODE_S32toF16_NONE,
                                                 ConvRelu::CRMODE_S32toF16_DEQSCALE_SPR,
                                                 ConvRelu::CRMODE_DEQSCALE_VDEQ16,
                                                 ConvRelu::CRMODE_DEQSCALE_DEQ16,
                                                 ConvRelu::CRMODE_DEQSCALE_VDEQS16,
                                                 ConvRelu::CRMODE_DEQSCALE_DEQS16};
    const std::vector<ConvRelu> support32To8 = {ConvRelu::CRMODE_DEQSCALE_VDEQ8, ConvRelu::CRMODE_DEQSCALE_DEQ8};
    if (find(support32To32.begin(), support32To32.end(), dmaMovConvReluRecord.crMode) != support32To32.end()) {
        isSupport = SupportFor32::SUPPORT32TO32;
    } else if (find(support32To16.begin(), support32To16.end(), dmaMovConvReluRecord.crMode) != support32To16.end()) {
        isSupport = SupportFor32::SUPPORT32TO16;
    } else if (find(support32To8.begin(), support32To8.end(), dmaMovConvReluRecord.crMode) != support32To8.end()) {
        isSupport = SupportFor32::SUPPORT32TO8;
    }
    return isSupport;
}

static void SetMemInfoWriteForUb(const DmaMovConvReluRecord& dmaMovConvReluRecord, MemOpInfo& memInfo)
{
    constexpr uint32_t blockSize48B = 48UL;
    if (dmaMovConvReluRecord.srcDataType == DataType::DATA_B16) {
        if (IsSupportFor16(dmaMovConvReluRecord)) {
            memInfo.repeatStride = dmaMovConvReluRecord.lenBurst + dmaMovConvReluRecord.dstStride;
        } else {
            SAN_WARN_LOG("ConvRelu does not match.");
            return;
        }
    }
    if (dmaMovConvReluRecord.srcDataType == DataType::DATA_B32) {
        SupportFor32 isSupport = IsSupportFor32(dmaMovConvReluRecord);
        constexpr uint32_t twice = 2U;
        switch (isSupport) {
            case SupportFor32::SUPPORT32TO32:
                memInfo.blockSize = 32U;
                memInfo.repeatStride = dmaMovConvReluRecord.lenBurst * twice + dmaMovConvReluRecord.dstStride;
                memInfo.blockNum = dmaMovConvReluRecord.lenBurst * twice; // block size is 64B(LOC32)
                break;
            case SupportFor32::SUPPORT32TO16:
                memInfo.blockSize = 32U;
                memInfo.repeatStride = dmaMovConvReluRecord.lenBurst + dmaMovConvReluRecord.dstStride;
                break;
            case SupportFor32::SUPPORT32TO8:
                memInfo.blockSize = 16U;
                memInfo.repeatStride = dmaMovConvReluRecord.lenBurst +
                                       dmaMovConvReluRecord.dstStride * blockSize48B / memInfo.blockSize;
                break;
            default:
                SAN_WARN_LOG("ConvRelu does not match.");
                break;
        }
    }
}

static void ParseRecordDmaMovConvRelu(const KernelRecord &record, std::vector<SanEvent> &events)
{
    constexpr uint32_t blockSize1024B = 1024UL;
    constexpr uint32_t blockSize512B = 512UL;
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& dmaMovConvReluRecord = record.payload.dmaMovConvReluRecord;
    PipeType pipe = FormatConverter::QueryPipeType(dmaMovConvReluRecord.srcMemType, dmaMovConvReluRecord.dstMemType);
    if (pipe == PipeType::SIZE) {
        SAN_ERROR_LOG("serialNo: %lu parse failed, pipe has exceeded the maximum limit", record.serialNo);
        return;
    }
    SetLocationInfo(event, dmaMovConvReluRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = pipe;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;

    memInfo.memType = dmaMovConvReluRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.addr = dmaMovConvReluRecord.src;
    memInfo.blockNum = dmaMovConvReluRecord.lenBurst;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = dmaMovConvReluRecord.nBurst;

    memInfo.blockSize = 32U;
    memInfo.repeatStride = dmaMovConvReluRecord.lenBurst +
                           dmaMovConvReluRecord.srcStride * blockSize512B / memInfo.blockSize;
    if (dmaMovConvReluRecord.srcDataType == DataType::DATA_B32) {
        memInfo.blockSize = 64U;
        memInfo.repeatStride = dmaMovConvReluRecord.lenBurst +
                               dmaMovConvReluRecord.srcStride * blockSize1024B / memInfo.blockSize;
    }

    AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV_CONV_RELU);
    events.emplace_back(event);

    memInfo.memType = dmaMovConvReluRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = dmaMovConvReluRecord.dst;
    SetMemInfoWriteForUb(dmaMovConvReluRecord, memInfo);
    AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV_CONV_RELU);
    events.emplace_back(event);
}

static bool ParseRecordDmaMovDepthWiseWrite(const DmaMovConvReluRecord& dmaMovConvReluRecord, MemOpInfo& memInfo)
{
    constexpr uint32_t blockSizeForB32 = 1024UL;
    constexpr uint32_t blockSizeForB16 = 512UL;
    constexpr uint32_t twice = 2U;
    if (dmaMovConvReluRecord.srcDataType == DataType::DATA_B16) {
        if (IsSupportFor16(dmaMovConvReluRecord)) {
            memInfo.blockSize = dmaMovConvReluRecord.lenBurst * blockSizeForB16 / twice;
        } else {
            SAN_ERROR_LOG("Parse DmaMovDepthWise failed, ConvRelu does not match.");
            return false;
        }
    }
    if (dmaMovConvReluRecord.srcDataType == DataType::DATA_B32) {
        if (dmaMovConvReluRecord.crMode == ConvRelu::CRMODE_NONE ||
                dmaMovConvReluRecord.crMode == ConvRelu::CRMODE_NONE_RELU) {
            memInfo.blockSize = dmaMovConvReluRecord.lenBurst * blockSizeForB32 / twice;
        } else if (dmaMovConvReluRecord.crMode == ConvRelu::CRMODE_F32toF16_NONE ||
                dmaMovConvReluRecord.crMode == ConvRelu::CRMODE_F32toF16_RELU) {
            memInfo.blockSize = dmaMovConvReluRecord.lenBurst * blockSizeForB16 / twice;
        } else {
            SAN_ERROR_LOG("Parse DmaMovDepthWise failed, ConvRelu does not match.");
            return false;
        }
    }
    return true;
}

static void ParseRecordDmaMovDepthWise(const KernelRecord &record, std::vector<SanEvent> &events)
{
    constexpr uint32_t blockSizeForB32 = 1024UL;
    constexpr uint32_t blockSizeForB16 = 512UL;
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& dmaMovConvReluRecord = record.payload.dmaMovConvReluRecord;

    SetLocationInfo(event, dmaMovConvReluRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = FormatConverter::QueryPipeType(dmaMovConvReluRecord.srcMemType, dmaMovConvReluRecord.dstMemType);
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    // When moving data from L0C to UB, only burst number = 1 is allowed (source gap is neglected)
    if (dmaMovConvReluRecord.nBurst != 1) {
        SAN_ERROR_LOG("Parse DmaMovDepthWise failed, invalid nBurst.");
        return;
    }

    memInfo.memType = dmaMovConvReluRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.addr = dmaMovConvReluRecord.src;
    memInfo.blockNum = dmaMovConvReluRecord.lenBurst;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = dmaMovConvReluRecord.nBurst;
    memInfo.repeatStride = 1U;
    memInfo.blockSize = blockSizeForB16;
    if (dmaMovConvReluRecord.srcDataType == DataType::DATA_B32) {
        memInfo.blockSize = blockSizeForB32;
    }
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);

    // The first 8 channels would be written into the defined destination base address,
    // the second 8 channels would be written into the dst_base_addr + (burst_length*block_size)/2 + dst_gap*32B
    memInfo.memType = dmaMovConvReluRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = dmaMovConvReluRecord.dst;
    memInfo.blockNum = 1U;
    if (!ParseRecordDmaMovDepthWiseWrite(dmaMovConvReluRecord, memInfo)) {
        return;
    }
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);

    memInfo.addr = dmaMovConvReluRecord.dst + memInfo.blockSize + dmaMovConvReluRecord.dstStride * 32U;
    events.emplace_back(event);
}

static void ParseRecordDmaMovNd2nz(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& dmaMovRecord = record.payload.dmaMovNd2nzRecord;
    PipeType pipe = FormatConverter::QueryPipeType(dmaMovRecord.srcMemType, dmaMovRecord.dstMemType);
    if (pipe == PipeType::SIZE) {
        SAN_ERROR_LOG("Parse DmaMovNd2nz failed, can't find the pipetype.");
        return;
    }
    SetLocationInfo(event, dmaMovRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = pipe;
    memInfo.memType = dmaMovRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.addr = dmaMovRecord.src;
    AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV_ND2NZ);
    constexpr uint64_t c0Size = 32U;
    uint64_t dTypeByteSize = FormatConverter::GetDataSizeByType(dmaMovRecord.dataType);
    // 向上取整，获得C0列数
    uint64_t columnNum = (dmaMovRecord.dValue * dTypeByteSize + c0Size - 1) / c0Size;
    for (uint16_t nd = 0; nd < dmaMovRecord.ndNum; ++nd) {
        memInfo.addr = dmaMovRecord.src + dmaMovRecord.srcNdMatrixStride * dTypeByteSize * nd;
        memInfo.blockNum = dmaMovRecord.dValue;
        memInfo.blockSize = dTypeByteSize;
        memInfo.blockStride = 1U;
        memInfo.repeatTimes = dmaMovRecord.nValue;
        memInfo.repeatStride = dmaMovRecord.srcDValue;
        events.emplace_back(event);
    }

    // 写内存
    memInfo.memType = dmaMovRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = dmaMovRecord.dst;
    AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV_ND2NZ);
    for (uint64_t c = 0; c < columnNum; ++c) {
        memInfo.addr = dmaMovRecord.dst + c0Size * dmaMovRecord.dstNzC0Stride * c;
        memInfo.blockSize = c0Size;
        memInfo.blockNum = dmaMovRecord.nValue;
        memInfo.blockStride = dmaMovRecord.dstNzNStride;
        memInfo.repeatStride = dmaMovRecord.dstNzMatrixStride * dTypeByteSize / c0Size;
        memInfo.repeatTimes = dmaMovRecord.ndNum;
        events.emplace_back(event);
    }
}

static bool SetNdNzDevPubElement(const DmaMovNd2nzDavRecord &record, SanEvent &event, uint64_t serialNo,
                                 BlockType blockType)
{
    auto& memInfo = event.eventInfo.memInfo;
    PipeType pipe = FormatConverter::QueryPipeType(record.srcMemType, record.dstMemType);
    if (pipe == PipeType::SIZE) {
        SAN_ERROR_LOG("serialNo: %lu parse failed, can't find the pipe type", serialNo);
        return false;
    }
    uint64_t dTypeByteSize = FormatConverter::GetDataSizeByType(record.dataType);
    if (record.loop1SrcStride % dTypeByteSize != 0) {
        SAN_ERROR_LOG("serialNo: %lu parse failed, loop1 src stride must align with data type size", serialNo);
        return false;
    }
    SetLocationInfo(event, record, blockType, serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = pipe;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    if (record.smallC0 != 0 && record.dValue > 4) {
        SAN_ERROR_LOG("serialNo: %lu parse failed, D must be less or equal to 4 when enable small C0 mode", serialNo);
        return false;
    }
    return true;
}

static void DmaMovNdorDn2nzDavSmallC0(bool srcIsNd, const DmaMovNd2nzDavRecord &dmaMovRecord, SanEvent &event,
                                      std::vector<SanEvent> &events)
{
    auto& memInfo = event.eventInfo.memInfo;
    uint64_t dTypeByteSize = FormatConverter::GetDataSizeByType(dmaMovRecord.dataType);

    // read
    memInfo.addr = dmaMovRecord.src;
    memInfo.memType = dmaMovRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.blockNum = 1;
    memInfo.blockSize = srcIsNd ? dmaMovRecord.dValue * dTypeByteSize : dmaMovRecord.nValue * dTypeByteSize;
    memInfo.repeatTimes = 1;
    if (srcIsNd) {
        AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV_ND2NZ_D);
    } else {
        AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV_DN2NZ_D);
    }
    uint64_t rowNum = srcIsNd ? dmaMovRecord.nValue : dmaMovRecord.dValue;
    for (uint64_t h = 0; h < dmaMovRecord.ndNum; ++h) {
        uint64_t ndAddr = dmaMovRecord.src + h * dmaMovRecord.loop4SrcStride;
        for (uint64_t i = 0; i < rowNum; ++i) {
            memInfo.addr = ndAddr + i * dmaMovRecord.loop1SrcStride;
            events.emplace_back(event);
        }
    }

    // write
    constexpr uint8_t C0_D_VALUE_NUM = 4;
    constexpr uint64_t C0_SIZE = 32U;
    memInfo.addr = dmaMovRecord.dst;
    memInfo.memType = dmaMovRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.blockSize = C0_D_VALUE_NUM * dTypeByteSize;
    memInfo.repeatTimes = 1;
    if (srcIsNd) {
        AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV_ND2NZ_D);
    } else {
        AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV_DN2NZ_D);
    }
    /// 向上取整计算每个ndNum数据包含多少行数据
    const uint32_t alignSize = C0_SIZE / (C0_D_VALUE_NUM * dTypeByteSize);
    memInfo.blockNum = CeilByAlignSize(dmaMovRecord.nValue, alignSize);
    memInfo.blockStride = 1;
    for (uint64_t h = 0; h < dmaMovRecord.ndNum; ++h) {
        memInfo.addr = memInfo.addr + h * dmaMovRecord.loop4DstStride * C0_SIZE;
        events.emplace_back(event);
    }
}

static void DmaMovNdorDn2nzDavDefault(bool srcIsNd, const DmaMovNd2nzDavRecord &dmaMovRecord, SanEvent &event,
                                      std::vector<SanEvent> &events)
{
    auto& memInfo = event.eventInfo.memInfo;
    uint64_t dTypeByteSize = FormatConverter::GetDataSizeByType(dmaMovRecord.dataType);

    // read
    memInfo.addr = dmaMovRecord.src;
    memInfo.memType = dmaMovRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.blockNum = 1;
    memInfo.blockSize = srcIsNd ? dmaMovRecord.dValue * dTypeByteSize : dmaMovRecord.nValue * dTypeByteSize;
    memInfo.repeatTimes = 1;
    AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV_ND2NZ_D);
    uint64_t rowNum = srcIsNd ? dmaMovRecord.nValue : dmaMovRecord.dValue;
    for (uint64_t h = 0; h < dmaMovRecord.ndNum; ++h) {
        uint64_t ndAddr = dmaMovRecord.src + h * dmaMovRecord.loop4SrcStride;
        for (uint64_t i = 0; i < rowNum; ++i) {
            memInfo.addr = ndAddr + i * dmaMovRecord.loop1SrcStride;
            events.emplace_back(event);
        }
    }

    // write
    constexpr uint64_t C0_SIZE = 32U;
    memInfo.addr = dmaMovRecord.dst;
    memInfo.memType = dmaMovRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.blockNum = 1;
    memInfo.blockSize = C0_SIZE;
    memInfo.repeatTimes = 1;
    AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV_ND2NZ_D);
    /// 向上取整计算每个ndNum数据包含多少个完整的分型矩阵
    uint32_t matrixNum = CeilByAlignSize<C0_SIZE>(dmaMovRecord.dValue * dTypeByteSize) / C0_SIZE;
    for (uint64_t i = 0; i < matrixNum; ++i) {
        uint64_t loop3Addr = dmaMovRecord.dst + i * dmaMovRecord.loop3DstStride * C0_SIZE;
        for (uint64_t h = 0; h < dmaMovRecord.ndNum; ++h) {
            uint64_t loop4Addr = loop3Addr + h * dmaMovRecord.loop4DstStride * C0_SIZE;
            for (uint64_t k = 0; k < rowNum; ++k) {
                memInfo.addr = loop4Addr + k * dmaMovRecord.loop2DstStride * C0_SIZE;
                events.emplace_back(event);
            }
        }
    }
}

static void ParseRecordDmaMovNd2NzDav(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event{};
    auto& dmaMovRecord = record.payload.dmaMovNd2nzDavRecord;
    if (!SetNdNzDevPubElement(dmaMovRecord, event, record.serialNo, record.blockType)) {
        return;
    };
    if (dmaMovRecord.smallC0 != 0) {
        DmaMovNdorDn2nzDavSmallC0(true, dmaMovRecord, event, events);
    } else {
        DmaMovNdorDn2nzDavDefault(true, dmaMovRecord, event, events);
    }
}

static void ParseRecordDmaMovDn2nzDav(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event{};
    auto& dmaMovRecord = record.payload.dmaMovNd2nzDavRecord;
    if (!SetNdNzDevPubElement(dmaMovRecord, event, record.serialNo, record.blockType)) {
        return;
    };
    if (dmaMovRecord.smallC0 != 0) {
        DmaMovNdorDn2nzDavSmallC0(false, dmaMovRecord, event, events);
    } else {
        DmaMovNdorDn2nzDavDefault(false, dmaMovRecord, event, events);
    }
}

static void ParseRecordMovAlignImpl(const KernelRecord &record, std::vector<SanEvent> &events, bool alignCheck)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& movAlignRecord = record.payload.movAlignRecord;
    PipeType pipe = FormatConverter::QueryPipeType(movAlignRecord.srcMemType, movAlignRecord.dstMemType);
    if (pipe == PipeType::SIZE) {
        SAN_ERROR_LOG("Parse MovAlign failed, can't find the pipetype.");
        return;
    }
    if (movAlignRecord.lenBurst == 0) {
        SAN_ERROR_LOG("Parse MovAlign failed, invalid lenBurst.");
        return;
    }

    SetLocationInfo(event, movAlignRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = pipe;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.blockSize = 1U;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = movAlignRecord.nBurst;

    memInfo.memType = movAlignRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.addr = movAlignRecord.src;
    // 不管是从 GM 还是 UB 读，读取的字节数都认为是 lenBurst，防止因读取了 dummy 数据
    // 导致未初始化误报。
    memInfo.blockNum = movAlignRecord.lenBurst;
    memInfo.repeatStride = memInfo.memType == MemType::GM ?
        movAlignRecord.lenBurst + movAlignRecord.srcGap :
        CeilByAlignSize<UB_ALIGN_SIZE>(movAlignRecord.lenBurst) + movAlignRecord.srcGap * UB_ALIGN_SIZE;
    if (alignCheck) {
        AlignChecker::Instance().CheckAlign(event, record.recordType);
    }
    events.emplace_back(event);

    uint64_t paddingSize = (movAlignRecord.leftPaddingNum + movAlignRecord.rightPaddingNum) *
        FormatConverter::GetDataSizeByType(movAlignRecord.dataType);
    uint64_t ubWriteSize = CeilByAlignSize<UB_ALIGN_SIZE>(movAlignRecord.lenBurst + paddingSize);
    memInfo.memType = movAlignRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = movAlignRecord.dst;
    // 往 UB 写时，包含原始数据 + padding 数据 + dummy 数据
    memInfo.blockNum = memInfo.memType == MemType::GM ? movAlignRecord.lenBurst : ubWriteSize;
    memInfo.repeatStride = memInfo.memType == MemType::GM ?
        movAlignRecord.lenBurst + movAlignRecord.dstGap :
        ubWriteSize + movAlignRecord.dstGap * UB_ALIGN_SIZE;
    if (alignCheck) {
        AlignChecker::Instance().CheckAlign(event, record.recordType);
    }
    events.emplace_back(event);
}

static void ParseRecordMovAlign(const KernelRecord &record, std::vector<SanEvent> &events)
{
    return ParseRecordMovAlignImpl(record, events, true);
}

// Generate events from record. Return true on success, false otherwise.
static bool CheckMovAlignV2Event(const MovAlignRecordV2 &record, bool &isCompactModeAble)
{
    if (record.lenBurst == 0) {
        SAN_ERROR_LOG("parse MovAlignV2 failed, lenBurst is 0.");
        return false;
    }

    // loop1/2_size != 0/1 means enable loop mode
    bool isLoopMode = (record.loop1Size > 0) && (record.loop2Size > 1);
    bool isUbStrideNot32BAligned{};
    bool isLoopStrideNot32BAligned{};
    if (record.srcMemType == MemType::GM) {
        bool isLRPMode = (record.rightPaddingNum > 0) || (record.leftPaddingNum > 0);
        if (isLoopMode && isLRPMode) {
            SAN_ERROR_LOG("LP/RP mode can not be enabled with Loop mode.");
            return false;
        }
        isCompactModeAble = record.dstStride == record.lenBurst;
        isUbStrideNot32BAligned = record.dstStride & 0x1F;
        isLoopStrideNot32BAligned = (record.loop1DstStride & 0x1F) | (record.loop2DstStride & 0x1F);
        if (isLRPMode) {
            if (record.dataType < DataType::DATA_B8 || record.dataType > DataType::DATA_B32) {
                SAN_ERROR_LOG("MovAlignV2 invalid data type, %d", static_cast<int32_t>(record.dataType));
                return false;
            }
            // padding data cannot bigger than 32B
            size_t size = FormatConverter::GetDataSizeByType(record.dataType);
            if (record.leftPaddingNum * size > 0x20) {
                SAN_ERROR_LOG("MovAlignV2 left padding bigger than 32B");
                return false;
            } else if (record.rightPaddingNum * size > 0x20) {
                SAN_ERROR_LOG("MovAlignV2 right padding bigger than 32B");
                return false;
            }
        }
    } else {
        isCompactModeAble = record.srcStride == record.lenBurst;
        isUbStrideNot32BAligned = record.srcStride & 0x1F;
        isLoopStrideNot32BAligned = (record.loop1SrcStride & 0x1F) | (record.loop2SrcStride & 0x1F);
    }

    if (!isCompactModeAble && isUbStrideNot32BAligned) {
        SAN_ERROR_LOG("Stride need to be 32B aligned for UB buffer under non-compact mode.");
        return false;
    } else if (isLoopMode && isLoopStrideNot32BAligned) {
        SAN_ERROR_LOG("Loop stride need to be 32B aligned for UB buffer.");
        return false;
    }
    return true;
}

static bool SetDumpParam(const MovAlignRecordV2 &record, SanEvent &eventRead, SanEvent &eventWrite,
                         bool isCompactModeAble)
{
    PipeType pipe = FormatConverter::QueryPipeType(record.srcMemType, record.dstMemType);
    if (pipe == PipeType::SIZE) {
        SAN_ERROR_LOG("parse MovAlign failed, can't find the pipetype.");
        return false;
    }
    auto initSanEvent = [pipe, &record](SanEvent &event) {
        event.type = EventType::MEM_EVENT;
        event.pipe = pipe;
        event.eventInfo.memInfo.vectorMask = {~0ULL, ~0ULL};
        event.eventInfo.memInfo.maskMode = MaskMode::MASK_NORM;
        event.eventInfo.memInfo.blockSize = 1U;
        event.eventInfo.memInfo.blockStride = 1U;
        event.eventInfo.memInfo.repeatTimes = record.nBurst;
        // 和V1版本一致: 不管是从 GM 还是 UB 读，读取的字节数都认为是 lenBurst，防止因读取了 dummy 数据导致未初始化误报。
        event.eventInfo.memInfo.blockNum = record.lenBurst;
    };
    initSanEvent(eventRead);
    initSanEvent(eventWrite);

    auto &memInfoRead = eventRead.eventInfo.memInfo;
    memInfoRead.repeatStride = record.srcStride;
    memInfoRead.memType = record.srcMemType;
    memInfoRead.opType = AccessType::READ;
    memInfoRead.addr = record.src;

    auto &memInfoWrite = eventWrite.eventInfo.memInfo;
    memInfoWrite.repeatStride = record.dstStride;
    memInfoWrite.memType = record.dstMemType;
    memInfoWrite.opType = AccessType::WRITE;
    memInfoWrite.addr = record.dst;
    if (record.srcMemType == MemType::GM) {
        // LP/RP 和其他模式合并处理
        auto padding = (record.leftPaddingNum + record.rightPaddingNum) *
            FormatConverter::GetDataSizeByType(record.dataType);
        if (isCompactModeAble) {
            // 文档没有说明LP/RP和compact同时使能的情况，先按padding在每个burst都填充处理
            memInfoWrite.blockNum = CeilByAlignSize<UB_ALIGN_SIZE>((padding + record.lenBurst) * record.nBurst);
            memInfoWrite.repeatTimes = memInfoWrite.repeatStride = 1U;
        } else {
            memInfoWrite.blockNum = CeilByAlignSize<UB_ALIGN_SIZE>(padding + record.lenBurst);
        }
    }
    return true;
}

static void ParseRecordMovAlignV2(const KernelRecord &inRecord, std::vector<SanEvent> &events)
{
    auto record = inRecord;
    SanEvent eventRead;
    SanEvent eventWrite;
    auto &memInfoRead = eventRead.eventInfo.memInfo;
    auto &memInfoWrite = eventWrite.eventInfo.memInfo;
    auto &movAlignRecord = record.payload.movAlignRecordV2;
    bool isCompactModeAble{};
    if (!CheckMovAlignV2Event(movAlignRecord, isCompactModeAble)) {
        return;
    }
    auto loop1Size = movAlignRecord.loop1Size;
    auto loop2Size = movAlignRecord.loop2Size;
    bool isLoopMode = (loop1Size > 1) && (loop2Size > 1);
    SetLocationInfo(eventRead, movAlignRecord, record.blockType, record.serialNo);
    SetLocationInfo(eventWrite, movAlignRecord, record.blockType, record.serialNo);

    // 不支持同时开启lepp模式和LP/RP模式
    if (!isLoopMode) {
        movAlignRecord.loop1Size = 1;
        movAlignRecord.loop2Size = 1;
        movAlignRecord.loop1SrcStride = 0;
        movAlignRecord.loop1DstStride = 0;
        movAlignRecord.loop2SrcStride = 0;
        movAlignRecord.loop2DstStride = 0;
    } else {
        movAlignRecord.rightPaddingNum = 0;
        movAlignRecord.leftPaddingNum = 0;
    }

    if (!SetDumpParam(movAlignRecord, eventRead, eventWrite, isCompactModeAble)) {
        return;
    }

    AlignChecker::Instance().CheckAlign(eventRead, record.recordType);
    AlignChecker::Instance().CheckAlign(eventWrite, record.recordType);

    // 所有模式都能按loop模式实现
    for (uint32_t k = 0; k < movAlignRecord.loop2Size; ++k) {
        for (uint32_t j = 0; j < movAlignRecord.loop1Size; ++j) {
            memInfoRead.addr =
                movAlignRecord.src + k * movAlignRecord.loop2SrcStride + j * movAlignRecord.loop1SrcStride;
            events.emplace_back(eventRead);
            memInfoWrite.addr =
                movAlignRecord.dst + k * movAlignRecord.loop2DstStride + j * movAlignRecord.loop1DstStride;
            events.emplace_back(eventWrite);
        }
    }
}

// 详见文档 Software Constraints 小节
static bool CheckNdDmaMovOut2UbConstraints(const NdDMAOut2UbRecord &ndDmaOut2UbRecord)
{
    bool flag = true;
    const auto &loops = ndDmaOut2UbRecord.loop;
    for (size_t i = 1; i < NdDMAOut2UbRecord::LOOP; ++i) {
        if (loops[i].loopDstStride <= loops[i - 1].loopDstStride) {
            flag = false;
            break;
        }
    }
    constexpr size_t ADDR_CONSTRAINT = 1ULL << 40;
    size_t cur = 0;
    uint64_t stride = 1;
    for (size_t i = 0; i < NdDMAOut2UbRecord::LOOP; ++i) {
        size_t tmp = static_cast<size_t>(loops[i].loopLpSize) + loops[i].loopSize + loops[i].loopRpSize - 1;
        cur += tmp * (i < 2 ? loops[i].loopSrcStride : loops[i].loopDstStride); // 2 由文档决定
        if (cur >= ADDR_CONSTRAINT) {
            SAN_ERROR_LOG("NdDmaMovOut2Ub loop stride params exceed addr constraints");
            return false;
        }
        if (flag) {
            if (loops[i].loopDstStride < stride) {
                SAN_ERROR_LOG("NdDmaMovOut2Ub loop stride params does not meet software constraints");
                return false;
            }
            stride += tmp * loops[i].loopDstStride;
        }
    }
    return true;
}

// 最大递归深度为ndDMAOut2UbRecord.loop大小，目前是5
template <bool isRead>
static void AppendNdDmaMovOut2UbLoopData(const NdDMAOut2UbRecord &ndDmaOut2UbRecord,
                                         const std::function<void(uint64_t)> &callback,
                                         uint64_t elementLen,
                                         uint64_t offset,
                                         int i)
{
    auto &loopInfo = ndDmaOut2UbRecord.loop[i];
    uint64_t start = 0;
    uint64_t end = loopInfo.loopSize;
    if (!isRead) {
        // padding数据会有写入ub操作，但是读gm操作的范围仅限于有效数据
        end += static_cast<uint64_t>(loopInfo.loopLpSize) + loopInfo.loopRpSize;
    }
    for (auto idx = start; idx < end; ++idx) {
        auto offset1 = offset;
        if (isRead) {
            offset1 += idx * loopInfo.loopSrcStride * elementLen;
        } else {
            offset1 += idx * loopInfo.loopDstStride * elementLen;
        }
        if (i == 0) {
            callback(offset1);
        } else {
            AppendNdDmaMovOut2UbLoopData<isRead>(ndDmaOut2UbRecord, callback, elementLen, offset1, i - 1);
        }
    }
}

static void ParseRecordNdDmaMovOut2Ub(const KernelRecord &record, std::vector<SanEvent> &events)
{
    auto &ndDmaOut2UbRecord = record.payload.ndDMAOut2UbRecord;
    if (ndDmaOut2UbRecord.dataType < DataType::DATA_B8 || ndDmaOut2UbRecord.dataType > DataType::DATA_B32) {
        SAN_ERROR_LOG("nd_dma_mov_out_to_ub invalid data type %d", static_cast<int32_t>(ndDmaOut2UbRecord.dataType));
        return;
    } else if (!CheckNdDmaMovOut2UbConstraints(ndDmaOut2UbRecord)) {
        return;
    }
    uint8_t elementLen = FormatConverter::GetDataSizeByType(ndDmaOut2UbRecord.dataType);
    SanEvent event;
    auto &memInfo = event.eventInfo.memInfo;
    SetLocationInfo(event, ndDmaOut2UbRecord, record.blockType, record.serialNo);
    event.pipe = PipeType::PIPE_MTE2;
    event.type = EventType::MEM_EVENT;
    memInfo.blockSize = elementLen;
    memInfo.blockNum = 1;
    memInfo.blockStride = 1;
    memInfo.repeatTimes = 1;
    memInfo.memType = MemType::GM;
    memInfo.opType = AccessType::READ;
    memInfo.addr = ndDmaOut2UbRecord.src;
    // gm按元素类型对齐，ub字节对齐
    AlignChecker::Instance().CheckAlign(event, elementLen);

    // read gm
    std::function<void(uint64_t)> appendEvent = [&](uint64_t addr) {
        event.eventInfo.memInfo.addr = addr;
        events.emplace_back(event);
    };
    AppendNdDmaMovOut2UbLoopData<true>(ndDmaOut2UbRecord, appendEvent, elementLen, ndDmaOut2UbRecord.src,
                                       NdDMAOut2UbRecord::LOOP - 1);
    // write ub
    memInfo.memType = MemType::UB;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = ndDmaOut2UbRecord.dst;
    AlignChecker::Instance().CheckAlign(event, 1U);
    AppendNdDmaMovOut2UbLoopData<false>(ndDmaOut2UbRecord, appendEvent, elementLen, ndDmaOut2UbRecord.dst,
                                        NdDMAOut2UbRecord::LOOP - 1);
}

static void ParseRecordMovBt(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& movBtRecord = record.payload.movBtRecord;
    PipeType pipe = FormatConverter::QueryPipeType(movBtRecord.srcMemType, movBtRecord.dstMemType);
    if (pipe == PipeType::SIZE) {
        SAN_ERROR_LOG("Parse MovBt failed, can't find the pipetype.");
        return;
    }
    if (movBtRecord.lenBurst == 0) {
        SAN_ERROR_LOG("Parse MovBt failed, invalid lenBurst.");
        return;
    }

    SetLocationInfo(event, movBtRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = pipe;
    memInfo.memType = movBtRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.addr = movBtRecord.src;
    memInfo.blockNum = movBtRecord.lenBurst * 2U;
    memInfo.blockSize = 32U;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = movBtRecord.nBurst;
    memInfo.repeatStride = memInfo.blockNum + movBtRecord.srcGap;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
    // dst is BT, do not develop
}

void FixPipeReadFromL0C(const MovFpRecord &record, std::vector<SanEvent> &events,
    SanEvent &event)
{
    constexpr uint32_t fractalNum = 16U; // 分形为16*16
    constexpr uint16_t c0Size = 64U;
    auto& memInfo = event.eventInfo.memInfo;
    memInfo.addr = record.src;
    memInfo.memType = MemType::L0C;
    memInfo.opType = AccessType::READ;
    memInfo.blockSize = c0Size;
    memInfo.blockStride = 1U;
    memInfo.repeatStride = record.srcStride;
    memInfo.blockNum = record.mSize;
    AlignChecker::Instance().CheckAlign(event, 64U);
    // 950/910b: Normal/Int8/Int4/F32
    if (!record.enNZ2ND && !record.enNZ2DN) {
        memInfo.repeatTimes = (record.nSize + fractalNum - 1) / fractalNum; // N维列数，向上取整
        events.emplace_back(event);
        return;
    }

    // NZ2ND/NZ2DN: 整除部分
    uint16_t fullColumn = record.nSize / fractalNum;
    memInfo.repeatTimes = fullColumn;
    memInfo.blockStride = record.enNZ2DN ? record.srcNzC0Stride : 1U;
    uint16_t ndStrideSize = record.isC310 ? 64U : 1024U;
    for (uint16_t nd = 0; nd < record.ndNum; ++nd) {
        memInfo.addr = record.src + record.srcNdStride * nd * ndStrideSize;
        events.emplace_back(event);
    }
    // NZ2ND/NZ2DN: 余数部分
    if (record.nSize % fractalNum != 0) {
        memInfo.blockSize = FormatConverter::GetDataSizeByType(DataType::DATA_B32);
        memInfo.blockStride = 1U;
        memInfo.blockNum = record.nSize % fractalNum;
        memInfo.repeatTimes = record.mSize;
        memInfo.repeatStride = record.enNZ2DN ? record.srcNzC0Stride * fractalNum : fractalNum;
        for (uint16_t nd = 0; nd < record.ndNum; ++nd) {
            memInfo.addr = record.src + record.srcNdStride * nd * ndStrideSize +
                record.srcStride * fullColumn * c0Size;
            events.emplace_back(event);
        }
    }
}

void FixPipeReadFromL0CA5(const MovFpRecord &record, std::vector<SanEvent> &events,
    SanEvent &event)
{
    // 默认是normal模式
    constexpr uint32_t fractalNum = 16U; // 分形为16*16
    constexpr uint16_t c0Size = 32U;
    auto& memInfo = event.eventInfo.memInfo;
    memInfo.addr = record.src;
    memInfo.memType = MemType::L0C;
    memInfo.opType = AccessType::READ;
    memInfo.blockSize = c0Size;
    memInfo.blockStride = 1U;
    memInfo.repeatStride = record.srcStride; // loop1_src_stride
    memInfo.blockNum = record.mSize;
    AlignChecker::Instance().CheckAlign(event, 64U); // f32 s32
    // 950/910b: Normal/Int8/Int4/F32
    if (!record.enNZ2ND && !record.enNZ2DN) {
        memInfo.repeatTimes = (record.nSize + fractalNum - 1) / fractalNum; // N维列数，向上取整
        events.emplace_back(event);
        return;
    }

    // NZ2ND/NZ2DN: 整除部分
    uint16_t fullColumn = record.nSize / fractalNum;
    memInfo.repeatTimes = fullColumn;
    memInfo.blockStride = record.enNZ2DN ? record.srcNzC0Stride : 1U;
    uint16_t ndStrideSize = 32U;
    for (uint16_t nd = 0; nd < record.ndNum; ++nd) {
        memInfo.addr = record.src + record.srcNdStride * nd * ndStrideSize; // Loop3 src stride
        events.emplace_back(event);
    }
    // NZ2ND/NZ2DN: 余数部分
    if (record.nSize % fractalNum != 0) {
        memInfo.blockSize = FormatConverter::GetDataSizeByType(DataType::DATA_B32); // 根据元素-即数据类型确定
        memInfo.blockStride = 1U;
        memInfo.blockNum = record.nSize % fractalNum;
        memInfo.repeatTimes = record.mSize;
        memInfo.repeatStride = record.enNZ2DN ? record.srcNzC0Stride * fractalNum : fractalNum;
        for (uint16_t nd = 0; nd < record.ndNum; ++nd) {
            memInfo.addr = record.src + record.srcNdStride * nd * ndStrideSize +
                record.srcStride * fullColumn * c0Size;
            events.emplace_back(event);
        }
    }
}

void ParseFixPipeWriteByMultiMode(const MovFpRecord &record, std::vector<SanEvent> &events,
    SanEvent &event)
{
    auto& memInfo = event.eventInfo.memInfo;
    uint32_t nColumn = (record.nSize + 15U) / 16U; // 除以16向上取整
    if (record.channelSplit) {
        memInfo.blockNum = record.isC310 ? record.mSize * 8 : record.mSize;
        memInfo.repeatTimes = (record.nSize + 7U) / 8U; // 除以8向上取整
        events.emplace_back(event);
    } else if (record.int4ChannelMerge) {
        if (record.isC310) {
            memInfo.blockNum = record.mSize * 32U; // 4bits时2个合并为1个 64个元素，但blcoksize是2个4bit合一，因此这里要改为32
            memInfo.repeatTimes = (record.nSize + 63U) / 64U; // 950会自动PADDING，所以除以64向上取整
            memInfo.repeatStride = record.dstStride / 2; // 4bits时2个合并为1个
        } else {
            memInfo.blockNum = record.mSize;
            memInfo.repeatTimes = nColumn / 4; // 从16x16转换为16x64，N维列数除以4
        }
        events.emplace_back(event);
    } else if (record.int8ChannelMerge) {
        memInfo.blockNum = record.isC310 ? record.mSize * 32 : record.mSize;
        memInfo.repeatTimes = nColumn / 2; // 从16x16转换为16x32，N维列数除以2
        events.emplace_back(event);
        // 如果N维不是16的偶数倍（被2整除），剩下数据直接搬出
        if ((nColumn % 2) != 0) {
            memInfo.addr = record.dst + memInfo.repeatStride * memInfo.repeatTimes * memInfo.blockSize;
            if (!record.isC310) {
                memInfo.blockSize = 16U;
            }
            memInfo.blockNum = record.isC310 ? record.mSize * 16 : record.mSize;
            memInfo.repeatTimes = 1U;
            memInfo.repeatStride = record.dstStride * 2;  // 单位是blockSize，需要乘以2
            events.emplace_back(event);
        }
    } else { // Normal
        memInfo.blockNum = record.isC310 ? record.mSize * 16 : (record.mSize * record.quantPreBits / BITS_EACH_BYTE / 2);
        memInfo.repeatTimes = nColumn;
        events.emplace_back(event);
    }
}

void FixPipeWriteToOut(const MovFpRecord &record, std::vector<SanEvent> &events,
    SanEvent &event)
{
    auto& memInfo = event.eventInfo.memInfo;
    memInfo.memType = MemType::GM;
    memInfo.opType = AccessType::WRITE;
    memInfo.blockStride = 1U;
    memInfo.addr = record.dst;
    AlignChecker::Instance().CheckAlign(event, 1U);
    memInfo.blockSize = record.quantPreBits == 4 ? 1U : (record.quantPreBits / BITS_EACH_BYTE); // 4bits时2个合并为1个
    if (record.enNZ2ND || record.enNZ2DN) {
        uint16_t blockNum = record.enNZ2DN ? record.mSize : record.nSize;
        memInfo.blockNum = record.quantPreBits == 4 ? (blockNum / 2) : blockNum; // 4bits时nSize是2的倍数
        memInfo.repeatTimes = record.enNZ2DN ? record.nSize : record.mSize;
        // 4bits时dstStride是2的倍数
        memInfo.repeatStride = record.quantPreBits == 4 ? (record.dstStride / 2) : record.dstStride;
        for (uint16_t nd = 0; nd < record.ndNum; ++nd) {
            memInfo.addr = record.dst + record.dstNdStride * nd * record.quantPreBits / BITS_EACH_BYTE;
            events.emplace_back(event);
        }
        return;
    }

    if (!record.isC310) {
        memInfo.blockSize = 32U;
    }
    memInfo.repeatStride = record.dstStride; // loop1dststride或loop2dststride(NZ2ND/DN)
    memInfo.addr = record.dst;
    ParseFixPipeWriteByMultiMode(record, events, event);
}

void FixPipeWriteToL1OrUB(const MovFpRecord &record, std::vector<SanEvent> &events,
    SanEvent &event, MemType dstType)
{
    auto& memInfo = event.eventInfo.memInfo;
    memInfo.memType = dstType;
    memInfo.opType = AccessType::WRITE;
    memInfo.blockStride = 1U;
    memInfo.addr = record.dst;
    AlignChecker::Instance().CheckAlign(event, 32U); // L1 dst必须32字节对齐
    memInfo.blockSize = record.quantPreBits == 4 ? 1U : (record.quantPreBits / BITS_EACH_BYTE); // 4bits时2个合并为1个
    if (record.enNZ2ND || record.enNZ2DN) {
        uint16_t blockNum = record.enNZ2DN ? record.mSize : record.nSize;
        memInfo.blockNum = record.quantPreBits == 4 ? (blockNum / 2) : blockNum; // 4bits时nSize是2的倍数
        memInfo.repeatTimes = record.enNZ2DN ? record.nSize : record.mSize;
        // 4bits时dstStride是2的倍数
        memInfo.repeatStride = record.quantPreBits == 4 ? (record.dstStride / 2) : record.dstStride;
        for (uint16_t nd = 0; nd < record.ndNum; ++nd) {
            memInfo.addr = record.dst + record.dstNdStride * nd * record.quantPreBits / BITS_EACH_BYTE;
            events.emplace_back(event);
        }
        return;
    }
    memInfo.repeatStride = record.dstStride;
    memInfo.addr = record.dst;
    ParseFixPipeWriteByMultiMode(record, events, event);
}

bool CheckRecordMovFpValid(const MovFpRecord &payload)
{
    // M = 0或N = 0或nd_number = 0表示不执行，该指令将被视为NOP
    if (payload.mSize == 0 || payload.nSize == 0 || (payload.enNZ2ND && payload.ndNum == 0)) {
        SAN_WARN_LOG("NOP FIX_L0C_TO_OUT.");
        return false;
    }
    // channelSplit/int4ChannelMerge/int8ChannelMerge/enNZ2ND最多只能使能一个
    if (payload.int8ChannelMerge + payload.int4ChannelMerge + payload.channelSplit + payload.enNZ2ND > 1) {
        SAN_WARN_LOG("Don't parse FIX_L0C_TO_OUT with more than one of ChannelMerge/Split/NZ2ND enabled.");
    }
    if ((payload.srcStride % 16) != 0) {  // srcStride必须是16的倍数
        SAN_WARN_LOG("SrcStride must be multiples of 16 when parse FIX_L0C_TO_OUT.");
    }
    if (!payload.int4ChannelMerge && !payload.channelSplit && !payload.enNZ2ND) {
        // normal_movement或者int8ChannelMerge使能时，nSize必须是16的倍数
        if (payload.nSize % 16 != 0) {
            SAN_WARN_LOG("nSize shoule be multiples of 16 when parse FIX_L0C_TO_OUT.");
        }
    }
    // 910b上，int4ChannelMerge使能时，nSize必须是64的倍数
    if (!payload.isC310 && payload.int4ChannelMerge && (payload.nSize % 64) != 0) {
        SAN_WARN_LOG("nSize shoule be multiples of 64 when parse FIX_L0C_TO_OUT with int4 channel merge.");
    }
    if (payload.channelSplit && (payload.nSize % 8) != 0) {  // channelSplit使能时，nSize必须是8的倍数
        SAN_WARN_LOG("nSize shoule be multiples of 8 when parse FIX_L0C_TO_OUT with f32 channel split.");
    }
    if (payload.isC310 && (payload.int8ChannelMerge || payload.int4ChannelMerge)) {
        if ((payload.nSize % 16) != 0) { // 对于在正常DMA模式下转换为s8、u8、HiF8、FP8 s4或u4的目标数据类型，N的配置必须是16的倍数
            SAN_WARN_LOG("nSize shoule be multiples of 16 when destination data types are converted into s8, u8, HiF8, FP8, s4 or u4 in normal DMA mode.");
        }
    }
    return true;
}

static void ParseRecordMovFp(const KernelRecord &record, std::vector<SanEvent> &events)
{
    auto& movFpRecord = record.payload.movFpRecord;
    if (!CheckRecordMovFpValid(movFpRecord)) {
        return;
    }
    SanEvent event;
    SetLocationInfo(event, movFpRecord, record.blockType, record.serialNo);

    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_FIX;
    event.eventInfo.memInfo.vectorMask = {static_cast<uint64_t>(-1), static_cast<uint64_t>(-1)};
    event.eventInfo.memInfo.maskMode = MaskMode::MASK_NORM;
    event.eventInfo.memInfo.dataBits = BITS_EACH_BYTE;

    FixPipeReadFromL0C(movFpRecord, events, event);
    FixPipeWriteToOut(movFpRecord, events, event);

    // 指令使能enUnitFlag，需要插入set_flag/wait_flag模拟同步行为
    if (movFpRecord.enUnitFlag) {
        CreateUnitFlagSyncEvent(record, events, movFpRecord, false);
    }
}

static void ParseRecordFixL0CToL1(const KernelRecord &record, std::vector<SanEvent> &events)
{
    auto& movFpRecord = record.payload.movFpRecord;
    if (!CheckRecordMovFpValid(movFpRecord)) {
        return;
    }
    SanEvent event;
    SetLocationInfo(event, movFpRecord, record.blockType, record.serialNo);

    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_FIX;
    event.eventInfo.memInfo.vectorMask = {static_cast<uint64_t>(-1), static_cast<uint64_t>(-1)};
    event.eventInfo.memInfo.maskMode = MaskMode::MASK_NORM;
    event.eventInfo.memInfo.dataBits = BITS_EACH_BYTE;

    FixPipeReadFromL0CA5(movFpRecord, events, event);
    FixPipeWriteToL1OrUB(movFpRecord, events, event, MemType::L1);

    // 指令使能enUnitFlag，需要插入set_flag/wait_flag模拟同步行为
    if (movFpRecord.enUnitFlag) {
        CreateUnitFlagSyncEvent(record, events, movFpRecord, false);
    }
}

static void ParseRecordFixL0CToUB(const KernelRecord &record, std::vector<SanEvent> &events)
{
    auto& movFpRecord = record.payload.movFpRecord;
    if (!CheckRecordMovFpValid(movFpRecord)) {
        return;
    }
    SanEvent event;
    SetLocationInfo(event, movFpRecord, record.blockType, record.serialNo);

    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_FIX;
    event.eventInfo.memInfo.vectorMask = {static_cast<uint64_t>(-1), static_cast<uint64_t>(-1)};
    event.eventInfo.memInfo.maskMode = MaskMode::MASK_NORM;
    event.eventInfo.memInfo.dataBits = BITS_EACH_BYTE;

    FixPipeReadFromL0CA5(movFpRecord, events, event);
    FixPipeWriteToL1OrUB(movFpRecord, events, event, MemType::UB);

    // 指令使能enUnitFlag，需要插入set_flag/wait_flag模拟同步行为
    if (movFpRecord.enUnitFlag) {
        CreateUnitFlagSyncEvent(record, events, movFpRecord, false);
    }
}

static void ParseRecordLoad2D(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& load2DRecord = record.payload.load2DRecord;
    SetLocationInfo(event, load2DRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = FormatConverter::QueryPipeType(load2DRecord.srcMemType, load2DRecord.dstMemType);
    memInfo.memType = load2DRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    if (load2DRecord.repeat == 0) {
        // repeat == 0 means no execution
        return;
    }
    if (load2DRecord.addrCalMode == AddrCalMode::INC) {
        memInfo.addr = load2DRecord.src + load2DRecord.baseIdx * MATRIX_FRACTAL_SIZE;
    } else {
        uint64_t offset = (load2DRecord.repeat - 1) * load2DRecord.srcStride * MATRIX_FRACTAL_SIZE;
        if (load2DRecord.src + load2DRecord.baseIdx * MATRIX_FRACTAL_SIZE < offset) {
            SAN_ERROR_LOG("Parse Load2DSparse failed, invalid addr");
            return;
        }
        memInfo.addr = load2DRecord.src + load2DRecord.baseIdx * MATRIX_FRACTAL_SIZE - offset;
    }
    memInfo.blockNum = 1U;
    memInfo.blockSize = load2DRecord.blockSize;
    memInfo.blockStride = 0U;
    memInfo.repeatTimes = load2DRecord.repeat;
    memInfo.repeatStride = load2DRecord.srcStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);

    memInfo.memType = load2DRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = load2DRecord.dst;
    memInfo.repeatStride = load2DRecord.dstStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
}

static void ParseRecordLoad2DSparse(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& load2DSparseRecord = record.payload.load2DSparseRecord;
    SetLocationInfo(event, load2DSparseRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = FormatConverter::QueryPipeType(load2DSparseRecord.srcMemType, load2DSparseRecord.dstMemType);
    memInfo.memType = load2DSparseRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    if (load2DSparseRecord.repeat == 0) {
        // repeat == 0 means this instruction will be regarded as NOP and an warning will be reported
        SAN_ERROR_LOG("Parse Load2DSparse failed, invalid repeat");
        return;
    }
    memInfo.addr = load2DSparseRecord.src0 + load2DSparseRecord.startId * MATRIX_FRACTAL_SIZE;
    memInfo.blockNum = 1U;
    memInfo.blockSize = MATRIX_FRACTAL_SIZE;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = load2DSparseRecord.repeat;
    memInfo.repeatStride = 1U;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);

    constexpr uint64_t indexMatrixSize = 128;
    memInfo.addr = load2DSparseRecord.src1 + load2DSparseRecord.startId * indexMatrixSize;
    memInfo.blockSize = indexMatrixSize;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);

    memInfo.memType = load2DSparseRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = load2DSparseRecord.dst;
    memInfo.blockSize = MATRIX_FRACTAL_SIZE;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
}

static std::map<DataType, uint8_t> g_fractalNumMap = {
    {DataType::DATA_B4,  4U},
    {DataType::DATA_B8,  2U},
    {DataType::DATA_B16, 1U},
    {DataType::DATA_B32, 2U},
};

void ParseRecordLoad2DTransposeWrite(const KernelRecord &record, std::vector<SanEvent> &events,
                                     SanEvent &event, MemOpInfo &memInfo, uint8_t fractalNum)
{
    auto& load2DTransposeRecord = record.payload.load2DTransposeRecord;
    memInfo.memType = load2DTransposeRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.repeatStride = load2DTransposeRecord.dstStride + 1U;
    memInfo.blockStride = load2DTransposeRecord.dstFracStride + 1U;
    if (memInfo.repeatStride < static_cast<uint32_t>(fractalNum +
                                load2DTransposeRecord.dstFracStride * (fractalNum - 1))) {
        SAN_WARN_LOG("Transposed Fractals have overlaps, hardware behavior is undeterminded");
    }
    memInfo.addr = load2DTransposeRecord.dst;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
}

static void ParseRecordLoad2DTranspose(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& load2DTransposeRecord = record.payload.load2DTransposeRecord;
    SetLocationInfo(event, load2DTransposeRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = FormatConverter::QueryPipeType(load2DTransposeRecord.srcMemType, load2DTransposeRecord.dstMemType);
    memInfo.memType = load2DTransposeRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    if (load2DTransposeRecord.repeat == 0) {
        SAN_ERROR_LOG("Parse Load2DTranspose failed, invalid repeat");
        return;
    }
    uint8_t fractalNum = 1;
    auto it = g_fractalNumMap.find(load2DTransposeRecord.dataType);
    if (it != g_fractalNumMap.end()) {
        fractalNum = it->second;
    } else {
        SAN_WARN_LOG("Invalid DataType when parse Load2DTranspose");
    }
    memInfo.blockNum = fractalNum;
    memInfo.blockSize = MATRIX_FRACTAL_SIZE;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = load2DTransposeRecord.repeat;
    memInfo.repeatStride = fractalNum * load2DTransposeRecord.srcStride;
    const uint64_t memInfoAddr = load2DTransposeRecord.src + load2DTransposeRecord.indexId
                                     * fractalNum * MATRIX_FRACTAL_SIZE;
    if (!load2DTransposeRecord.addrMode) {
        memInfo.addr = memInfoAddr;
    } else {
        const uint64_t offset = memInfo.repeatStride * MATRIX_FRACTAL_SIZE * (load2DTransposeRecord.repeat - 1);
        memInfo.addr = memInfoAddr - offset;
        if (memInfoAddr < offset) {
            SAN_WARN_LOG("Invalid addr when parse Load2DTranspose");
        }
        // The index ID in each iteration cannot be less than zero, otherwise an exception will be raised
        if (load2DTransposeRecord.indexId < load2DTransposeRecord.srcStride * (memInfo.repeatTimes - 1)) {
            SAN_ERROR_LOG("Parse Load2DTranspose failed, index ID in each iteration cannot be less than zero");
            return;
        }
    }
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);

    ParseRecordLoad2DTransposeWrite(record, events, event, memInfo, fractalNum);
}

static bool CheckTranspose(const LoadL12DRecord &loadL12DRecord)
{
    if (loadL12DRecord.transposeMode == TransposeMode::DISABLE) {
        return true;
    }
    switch (loadL12DRecord.detailedDataType) {
        case DetailedDataType::B4: {
            if (loadL12DRecord.mStep % 4 != 0) {
                SAN_ERROR_LOG("Parse RecordLoadL12D failed, M_Step must be multiples of 4"
                    " when transpose is enabled and .type = .b4.");
                return false;
            }
            return true;
        }
        case DetailedDataType::B8: {
            if (loadL12DRecord.mStep % 2 != 0) {
                SAN_ERROR_LOG("Parse RecordLoadL12D failed, M_Step must be multiples of 2"
                    " when transpose is enabled and .type = .b8.");
                return false;
            }
            return true;
        }
        case DetailedDataType::B16: {
            return true;
        }
        case DetailedDataType::B32: {
            if (loadL12DRecord.kStep % 2 != 0) {
                SAN_ERROR_LOG("Parse RecordLoadL12D failed, K_Step must be multiples of 2"
                    " when transpose is enabled and .type = .b32.");
                return false;
            }
            return true;
        }
        default: {
            break;
        }
    }
    SAN_ERROR_LOG("Parse RecordLoadL12D failed, Datatype is unsupported");
    return false;
}

static void HandleTransposeCase(MemOpInfo &memInfo, const LoadL12DRecord &loadL12DRecord)
{
    memInfo.blockNum = loadL12DRecord.kStep;
    memInfo.repeatTimes = loadL12DRecord.mStep;
    switch (loadL12DRecord.detailedDataType) {
        case DetailedDataType::B4: {
            memInfo.blockNum *= 4;
            memInfo.repeatTimes /= 4;
            break;
        }
        case DetailedDataType::B8: {
            memInfo.blockNum *= 2;
            memInfo.repeatTimes /= 2;
            break;
        }
        case DetailedDataType::B16: {
            break;
        }
        case DetailedDataType::B32: {
            memInfo.blockNum /= 2;
            memInfo.repeatTimes *= 2;
            break;
        }
        default: {
            break;
        }
    }
}

static void ParseRecordLoadL12D(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& loadL12DRecord = record.payload.loadL12DRecord;
    SetLocationInfo(event, loadL12DRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = FormatConverter::QueryPipeType(MemType::L1, loadL12DRecord.dstMemType);
    memInfo.memType = MemType::L1;
    memInfo.opType = AccessType::READ;
    if (!CheckTranspose(loadL12DRecord)) {
        return;
    }

    memInfo.addr = loadL12DRecord.src + (loadL12DRecord.kStartPosition * loadL12DRecord.srcStride +
        loadL12DRecord.mStartPosition) * MATRIX_FRACTAL_SIZE;
    memInfo.blockNum = loadL12DRecord.mStep;
    memInfo.blockSize = MATRIX_FRACTAL_SIZE;
    memInfo.blockStride = 1;
    memInfo.repeatTimes = loadL12DRecord.kStep;
    memInfo.repeatStride = loadL12DRecord.srcStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);

    memInfo.memType = loadL12DRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = loadL12DRecord.dst;
    memInfo.blockNum = loadL12DRecord.mStep;
    memInfo.blockSize = MATRIX_FRACTAL_SIZE;
    memInfo.blockStride = 1;
    memInfo.repeatTimes = loadL12DRecord.kStep;
    if (loadL12DRecord.transposeMode == TransposeMode::ENABLE) {
        HandleTransposeCase(memInfo, loadL12DRecord);
    }
    memInfo.repeatStride = loadL12DRecord.dstStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
}

static void ParseRecordLoadL1Mx2D(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& loadL1Mx2DRecord = record.payload.loadL1Mx2DRecord;
    SetLocationInfo(event, loadL1Mx2DRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE1;
    memInfo.memType = MemType::L1;
    memInfo.opType = AccessType::READ;

    memInfo.addr = loadL1Mx2DRecord.src + (loadL1Mx2DRecord.xStartPosition * loadL1Mx2DRecord.srcStride +
        loadL1Mx2DRecord.yStartPosition) * MATRIX_FRACTAL_ROW_SIZE;
    memInfo.blockNum = loadL1Mx2DRecord.yStep;
    memInfo.blockSize = MATRIX_FRACTAL_ROW_SIZE;
    memInfo.blockStride = 1;
    memInfo.repeatTimes = loadL1Mx2DRecord.xStep;
    memInfo.repeatStride = loadL1Mx2DRecord.srcStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
}

static uint16_t GetFracNum(DetailedDataType detailedDataType)
{
    switch (detailedDataType) {
        case DetailedDataType::B4:
            return 4;
        case DetailedDataType::B8:
            return 2;
        case DetailedDataType::B16:
            return 1;
        case DetailedDataType::B32:
            return 2;
        default:
            break;
    }
    return 0;
}

static void ParseRecordLoadL12DTranspose(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& loadL12DTransposeRecord = record.payload.loadL12DTransposeRecord;
    SetLocationInfo(event, loadL12DTransposeRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = FormatConverter::QueryPipeType(MemType::L1, MemType::L0B);
    memInfo.memType = MemType::L1;
    memInfo.opType = AccessType::READ;

    memInfo.addr = loadL12DTransposeRecord.src;
    memInfo.blockNum = GetFracNum(loadL12DTransposeRecord.detailedDataType);
    memInfo.blockSize = MATRIX_FRACTAL_SIZE;
    memInfo.blockStride = loadL12DTransposeRecord.srcFracStride;
    memInfo.repeatTimes = loadL12DTransposeRecord.repeat;
    memInfo.repeatStride = loadL12DTransposeRecord.srcStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);

    memInfo.memType = MemType::L0B;
    memInfo.opType = AccessType::WRITE;

    memInfo.addr = loadL12DTransposeRecord.dst;
    memInfo.blockStride = loadL12DTransposeRecord.dstFracStride;
    memInfo.repeatStride = loadL12DTransposeRecord.dstStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
}

static void ParseRecordDecompressHeader(const KernelRecord &record, std::vector<SanEvent> &events)
{
    // load the headers (32B) of each compression block to the internal buffer, 目前不考虑internal buffer
    constexpr uint64_t headerSize = 32;
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& decompressHeaderRecord = record.payload.decompressHeaderRecord;
    SetLocationInfo(event, decompressHeaderRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE2;
    memInfo.memType = decompressHeaderRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
 
    if (decompressHeaderRecord.nBlock == 0) {
        SAN_ERROR_LOG("Parse DecompressHeader failed, invalid nBlock");
        return;
    }
    
    memInfo.blockNum = 1U;
    memInfo.blockSize = headerSize * decompressHeaderRecord.nBlock;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = 1U;
    memInfo.repeatStride = 1U;
 
    memInfo.addr = decompressHeaderRecord.src;
   
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
}

static void ParseRecordBroadcast(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& broadcastRecord = record.payload.broadcastRecord;
    SetLocationInfo(event, broadcastRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    memInfo.memType = broadcastRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;

    if (broadcastRecord.nBurst == 0 || broadcastRecord.lenBurst == 0) {
        SAN_ERROR_LOG("Parse Broadcast failed, invalid nBburst or lenBurst");
        return;
    }

    memInfo.blockSize = 32U;
    memInfo.blockNum = broadcastRecord.enableRepeat ? 1 : broadcastRecord.lenBurst;
    constexpr uint16_t multipleForB32 = 2;
    if (broadcastRecord.srcDataType == DataType::DATA_B32) {
        memInfo.blockNum *= multipleForB32;  // block size is 64B(LOC32 and conv = 0)
    }
    memInfo.repeatStride = broadcastRecord.srcGap + memInfo.blockNum;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = broadcastRecord.nBurst;
    memInfo.addr = broadcastRecord.src;

    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);

    memInfo.memType = broadcastRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    constexpr uint64_t blockSizeForL0C32 = 1024U;
    memInfo.blockSize = 512U;
    if (broadcastRecord.dstDataType == DataType::DATA_B32) {
        memInfo.blockSize = blockSizeForL0C32;
    }
    memInfo.blockNum = broadcastRecord.lenBurst;
    memInfo.repeatStride = broadcastRecord.dstGap + memInfo.blockNum;
    memInfo.addr = broadcastRecord.dst;
    // L0C address should be 1024B(L0C32) or 512B(L0C16) aligned
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
}

static void ParseRecordDcPreload(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& dcPreloadRecord = record.payload.dcPreloadRecord;
    SetLocationInfo(event, dcPreloadRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_S_CAL;
    memInfo.memType = MemType::GM;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;

    memInfo.blockNum = 1U;
    memInfo.blockSize = 0U;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = 1U;
    memInfo.repeatStride = 1U;
    memInfo.alignSize = 1U;

    memInfo.addr = dcPreloadRecord.addr + dcPreloadRecord.offset;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
}

static void ParseRecordLoad3DFMap(const Load3DRecord &load3DRecord, std::vector<SanEvent> &events, SanEvent &event,
    uint16_t bitSize)
{
    uint32_t unitBitSize = 16 * 16;
    uint32_t loadSize = 32;
    uint32_t fMapC1 = (load3DRecord.fMapC * bitSize + unitBitSize - 1) / unitBitSize;

    auto& memInfo = event.eventInfo.memInfo;
    memInfo.memType = load3DRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    // 对src做对齐检查
    memInfo.addr = load3DRecord.src;
    AlignChecker::Instance().CheckAlign(event, RecordType::LOAD_3D);
    std::vector<std::vector<int32_t>> fMapPos(0);
    for (uint32_t fMc = 0; fMc < fMapC1; ++fMc) {
        int32_t fMhMax = load3DRecord.fMapH + load3DRecord.fMapBottomPad;
        for (int32_t fMh = -load3DRecord.fMapTopPad; fMh < fMhMax; fMh += load3DRecord.filterHStride) {
            int32_t fMwMax = load3DRecord.fMapW + load3DRecord.fMapRightPad;
            for (int32_t fMw = -load3DRecord.fMapLeftPad; fMw < fMwMax; fMw += load3DRecord.filterWStride) {
                fMapPos.push_back({ static_cast<int32_t>(fMc), fMh, fMw });
            }
        }
    }
    std::vector<std::vector<uint16_t>> filterPos(0);
    for (uint16_t fw = 0; fw < load3DRecord.filterW; ++fw) {
        for (uint16_t fh = 0; fh < load3DRecord.filterH; ++fh) {
            filterPos.push_back({ fw, fh });
        }
    }
    for (size_t fMapID = 0; fMapID < fMapPos.size(); ++fMapID) {
        int32_t fMc = fMapPos[fMapID][0];
        int32_t fMh = fMapPos[fMapID][1];
        int32_t fMw = fMapPos[fMapID][2];
        for (size_t filterID = 0; filterID < filterPos.size(); ++filterID) {
            int32_t combinedW = fMw + static_cast<int32_t>(filterPos[filterID][0] * load3DRecord.filterWDilation);
            int32_t combinedH = fMh + static_cast<int32_t>(filterPos[filterID][1] * load3DRecord.filterHDilation);
            if (combinedW < 0 || combinedW >= static_cast<int32_t>(load3DRecord.fMapW) ||
                combinedH < 0 || combinedH >= static_cast<int32_t>(load3DRecord.fMapH)) {
                continue;
            }
            uint32_t fMapIdx =
                static_cast<uint32_t>(combinedW) + static_cast<uint32_t>(combinedH) * load3DRecord.fMapW +
                static_cast<uint32_t>(fMc) * load3DRecord.fMapW * load3DRecord.fMapH;
            memInfo.addr = load3DRecord.src + fMapIdx * loadSize;
            memInfo.blockSize = loadSize;
            events.emplace_back(event);
        }
    }
}

static void ParseRecordLoad3DMatrix(const Load3DRecord &load3DRecord, std::vector<SanEvent> &events, SanEvent &event,
    uint16_t bitSize)
{
    uint8_t byteUnit = 8;
    auto& memInfo = event.eventInfo.memInfo;
    if (load3DRecord.matrixRptTimes > 1) {
        uint8_t mRptBase = 16;
        uint8_t kRptBase = 32;
        uint32_t mRptLength = mRptBase * load3DRecord.matrixRptStride;
        uint32_t kRptLength = kRptBase * byteUnit / bitSize * load3DRecord.matrixRptStride;
        if ((load3DRecord.matrixRptMode > 0) && kRptLength < static_cast<uint32_t>(load3DRecord.matrixKStep)) {
            SAN_ERROR_LOG("Parse load3D failed, invalid fm matrix kstep");
            return;
        }
        if ((load3DRecord.matrixRptMode == 0) && mRptLength < static_cast<uint32_t>(load3DRecord.matrixMStep)) {
            SAN_ERROR_LOG("Parse load3D failed, invalid fm matrix mstep");
            return;
        }
    }
    memInfo.memType = load3DRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = load3DRecord.dst;
    memInfo.blockSize = load3DRecord.matrixKStep * load3DRecord.matrixMStep *
        load3DRecord.matrixRptTimes * bitSize / byteUnit;
    AlignChecker::Instance().CheckAlign(event, RecordType::LOAD_3D);
    events.emplace_back(event);
}

static void ParseRecordLoad3D(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& load3DRecord = record.payload.load3DRecord;
    uint16_t bitSize;
    if (!FormatConverter::GetDataBitSizeByType(load3DRecord.dataType, bitSize)) {
        return;
    }
    if (load3DRecord.filterWDilation == 0 || load3DRecord.filterHDilation == 0) {
        SAN_ERROR_LOG("Parse load3Dv2 failed, filterWDilation or filterHDilation is 0");
        return;
    }
    if (!FormatConverter::CheckChannelSize(load3DRecord.dataType, load3DRecord.fMapC)) {
        SAN_ERROR_LOG("Parse load3Dv2 failed, invalid channel size.");
        return;
    }
    SetLocationInfo(event, load3DRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = FormatConverter::QueryPipeType(load3DRecord.srcMemType, load3DRecord.dstMemType);
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.blockNum = 1U;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = 1U;
    memInfo.repeatStride = 1U;
    ParseRecordLoad3DFMap(load3DRecord, events, event, bitSize);
    ParseRecordLoad3DMatrix(load3DRecord, events, event, bitSize);
}

static bool CheckMDirection(const Load3DV2Record &load3DV2Record, int32_t combinedH)
{
    int mPos = load3DV2Record.matrixMPos;
    int mStep = load3DV2Record.matrixMStep;
    if (load3DV2Record.matrixMode == 0) {
        return combinedH < mPos || combinedH >= mPos + mStep;
    } else {
        int mStride = load3DV2Record.matrixRptStride * 16U;
        int mTimes = load3DV2Record.matrixRptTimes;
        if (combinedH < mPos || combinedH >= mPos + mStride * mTimes) {
            return false;
        }
        return ((combinedH - mPos) % mStride) < mStep;
    }
}

static bool CheckKDirection(const Load3DV2Record &load3DV2Record, int32_t combinedW, uint16_t bitSize)
{
    int kPos = load3DV2Record.matrixKPos;
    int kStep = load3DV2Record.matrixKStep;
    if (load3DV2Record.matrixMode == 0) {
        return combinedW < kPos || combinedW >= kPos + kStep;
    }
    int kStride = load3DV2Record.matrixRptStride * MATRIX_FRACTAL_ROW_SIZE * BITS_EACH_BYTE / bitSize;
    int kTimes = load3DV2Record.matrixRptTimes;
    if (combinedW < kPos || combinedW >= kPos + kStride * kTimes) {
        return false;
    }
    return ((combinedW - kPos) % kStride) < kStep;
}

static bool CheckMKDirection(const Load3DV2Record &load3DV2Record, int32_t combinedH, int32_t combinedW, uint16_t bitSize)
{
    return (!CheckMDirection(load3DV2Record, combinedH) || !CheckKDirection(load3DV2Record, combinedW, bitSize));
}

static void HandleLoad3DEvent(const Load3DV2Record &load3DV2Record, std::vector<SanEvent> &events, SanEvent &event,
    uint16_t bitSize, size_t fMapID, std::vector<std::vector<uint16_t>> filterPos)
{
    auto& memInfo = event.eventInfo.memInfo;
    for (size_t filterID = 0; filterID < filterPos.size(); ++filterID) {
        int32_t combinedH = static_cast<int32_t>(fMapID);
        for (size_t fMc = 0; fMc < load3DV2Record.fMapC; ++fMc) {
            int32_t combinedW = static_cast<int32_t>(filterID * load3DV2Record.fMapC) + static_cast<int32_t>(fMc);
            if (CheckMKDirection(load3DV2Record, combinedH, combinedW, bitSize)) {
                continue;
            }
            uint64_t fMapIdx = static_cast<uint64_t>(combinedW) +
                static_cast<uint64_t>(combinedH) * filterPos.size() * load3DV2Record.fMapC;
            memInfo.addr = load3DV2Record.src + fMapIdx * bitSize;
            memInfo.blockSize = bitSize;
            events.emplace_back(event);
        }
    }
}

static void ParseRecordLoad3DV2FMap(const Load3DV2Record &load3DV2Record, std::vector<SanEvent> &events, SanEvent &event,
    uint16_t bitSize)
{
    auto& memInfo = event.eventInfo.memInfo;
    memInfo.memType = load3DV2Record.srcMemType;
    memInfo.opType = AccessType::READ;
    // 对src做对齐检查
    memInfo.addr = load3DV2Record.src;
    uint16_t alignSize = bitSize * load3DV2Record.fMapC / BITS_EACH_BYTE;
    AlignChecker::Instance().CheckAlign(event, alignSize <= 32U ? alignSize : 32U);
    std::vector<std::vector<int32_t>> fMapPos(0);
    int32_t fMhMax = load3DV2Record.fMapH + load3DV2Record.fMapBottomPad -
        load3DV2Record.filterH * load3DV2Record.filterHDilation + 1;
    int32_t fMwMax = load3DV2Record.fMapW + load3DV2Record.fMapRightPad -
        load3DV2Record.filterW * load3DV2Record.filterWDilation + 1;
    for (int32_t fMh = -load3DV2Record.fMapTopPad; fMh < fMhMax; fMh += load3DV2Record.filterHStride) {
        for (int32_t fMw = -load3DV2Record.fMapLeftPad; fMw < fMwMax; fMw += load3DV2Record.filterWStride) {
            fMapPos.push_back({ fMh, fMw });
        }
    }
    std::vector<std::vector<uint16_t>> filterPos(0);
    for (uint16_t fh = 0; fh < load3DV2Record.filterH; ++fh) {
        for (uint16_t fw = 0; fw < load3DV2Record.filterW; ++fw) {
            filterPos.push_back({ static_cast<uint16_t>(fh * load3DV2Record.filterHDilation),
                static_cast<uint16_t>(fw * load3DV2Record.filterWDilation)});
        }
    }
    for (size_t fMapID = 0; fMapID < fMapPos.size(); ++fMapID) {
        HandleLoad3DEvent(load3DV2Record, events, event, bitSize, fMapID, filterPos);
    }
}

static void ParseRecordLoad3DV2Matrix(const Load3DV2Record &load3DV2Record, std::vector<SanEvent> &events, SanEvent &event,
    uint16_t bitSize)
{
    auto& memInfo = event.eventInfo.memInfo;
    uint16_t mPos = load3DV2Record.outputMPos;
    uint16_t mStep = 0;
    uint16_t kStep = 0;
    int elementNum = MATRIX_FRACTAL_ROW_SIZE * BITS_EACH_BYTE / bitSize;
    if (load3DV2Record.transposeMode) {
        mStep = (load3DV2Record.matrixKStep + 15U) / 16U;
        kStep = (load3DV2Record.matrixMStep + elementNum - 1) / elementNum;
    } else {
        mStep = (load3DV2Record.matrixMStep + 15U) / 16U;
        kStep = (load3DV2Record.matrixKStep + elementNum - 1) / elementNum;
    }
    memInfo.blockSize = MATRIX_FRACTAL_SIZE;
    memInfo.blockNum = mStep;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = kStep;
    memInfo.repeatStride = load3DV2Record.dstStride;
    memInfo.memType = load3DV2Record.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = load3DV2Record.dst + mPos * MATRIX_FRACTAL_SIZE;
    AlignChecker::Instance().CheckAlign(event, RecordType::LOAD_3D_V2);
    events.emplace_back(event);
}

static void ParseRecordLoad3DV2(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& load3DV2Record = record.payload.load3DV2Record;
    uint16_t bitSize;
    if (!FormatConverter::GetDataBitSizeByType(load3DV2Record.dataType, bitSize)) {
        return;
    }
    if (load3DV2Record.filterWDilation == 0 || load3DV2Record.filterHDilation == 0) {
        SAN_ERROR_LOG("Parse load3Dv2 failed, filterWDilation or filterHDilation is 0");
        return;
    }
    if (!FormatConverter::CheckChannelSize(load3DV2Record.dataType, load3DV2Record.fMapC)) {
        SAN_ERROR_LOG("Parse load3Dv2 failed, invalid channel size.");
        return;
    }
    SetLocationInfo(event, load3DV2Record, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = FormatConverter::QueryPipeType(load3DV2Record.srcMemType, load3DV2Record.dstMemType);
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.blockNum = 1U;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = 1U;
    memInfo.repeatStride = 1U;
    ParseRecordLoad3DV2FMap(load3DV2Record, events, event, bitSize);
    ParseRecordLoad3DV2Matrix(load3DV2Record, events, event, bitSize);
}

static void ParseRecordLoadB2(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& loadB2Record = record.payload.loadB2Record;
    SetLocationInfo(event, loadB2Record, record.blockType, record.serialNo);
    uint64_t fractalMatrixSize = loadB2Record.dataType == DataType::DATA_B8 ? 128 : 64;
    uint64_t nominalSize = loadB2Record.repeat * fractalMatrixSize;

    event.type = EventType::MEM_EVENT;
    event.pipe = FormatConverter::QueryPipeType(loadB2Record.srcMemType, loadB2Record.dstMemType);
    memInfo.memType = loadB2Record.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.addr = loadB2Record.src;
    memInfo.blockNum = 1U;
    constexpr uint32_t alignSize = 512;
    memInfo.blockSize = CeilByAlignSize<alignSize>(nominalSize);
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = 1U;
    memInfo.repeatStride = 0U;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);

    memInfo.memType = loadB2Record.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = loadB2Record.dst;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
}

static void ParseRecordLoadAWinograd(const KernelRecord &record, std::vector<SanEvent> &events)
{
    constexpr uint64_t fractalMatrixSize = 512;
    constexpr uint8_t outputSubMatrixNum = 4;

    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& curRecord = record.payload.loadAWinogradRecord;
    uint8_t sizeFactor = FormatConverter::GetDataSizeByType(curRecord.dataType);
    uint64_t memSize = curRecord.fmSizeW * curRecord.fmSizeH * curRecord.fmSizeCh * sizeFactor;

    SetLocationInfo(event, curRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = FormatConverter::QueryPipeType(curRecord.srcMemType, curRecord.dstMemType);
    memInfo.memType = curRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.addr = curRecord.src;
    memInfo.blockNum = 1U;
    memInfo.blockSize = memSize;
    memInfo.blockStride = 0;
    memInfo.repeatTimes = 1U;
    memInfo.repeatStride = 0;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);

    // 指令内部保证该值最后是512的倍数
    memSize = (static_cast<uint64_t>(curRecord.extStepK * curRecord.extStepM) * sizeFactor);
    memInfo.memType = curRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = curRecord.dst;
    memInfo.blockNum = memSize / fractalMatrixSize;
    if (memInfo.blockNum == 0 || memSize % fractalMatrixSize != 0) {
        SAN_ERROR_LOG("Parse LoadAWinograd failed, invalid blockNum or memSize");
        return;
    }
    memInfo.blockSize = fractalMatrixSize;
    memInfo.blockStride = 1;
    memInfo.repeatTimes = outputSubMatrixNum;
    memInfo.repeatStride = curRecord.innerDstGap + memInfo.blockNum;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
}

static void ParseRecordLoadBWinograd(const KernelRecord &record, std::vector<SanEvent> &events)
{
    constexpr uint16_t fractalMatrixSize = 512;
    constexpr uint8_t fractalMatrixNumOnce = 9;
    constexpr uint8_t outputSubMatrixNum = 4;

    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& curRecord = record.payload.loadBWinogradRecord;
    SetLocationInfo(event, curRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = FormatConverter::QueryPipeType(curRecord.srcMemType, curRecord.dstMemType);
    memInfo.memType = curRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.addr = curRecord.src;
    memInfo.blockNum = fractalMatrixNumOnce;
    memInfo.blockSize = fractalMatrixSize;
    memInfo.blockStride = 1;
    memInfo.repeatTimes = curRecord.repeat;
    memInfo.repeatStride = curRecord.srcRptStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);

    memInfo.memType = curRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = curRecord.dst;
    memInfo.blockNum = outputSubMatrixNum;
    memInfo.blockStride = curRecord.innerDstStride;
    memInfo.repeatStride = curRecord.dstRptStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
}

static void ParseRecordSet2D(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& set2DRecord = record.payload.set2DRecord;
    SetLocationInfo(event, set2DRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    memInfo.memType = set2DRecord.dstMemType;
    event.pipe = memInfo.memType == MemType::L1 ? PipeType::PIPE_MTE2 : PipeType::PIPE_MTE1;
    memInfo.opType = AccessType::WRITE;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.addr = set2DRecord.dst;
    memInfo.blockNum = set2DRecord.dstBlockNum;
    memInfo.blockSize = set2DRecord.dstBlockSize;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = set2DRecord.repeat;
    memInfo.repeatStride = set2DRecord.dstBlockNum + set2DRecord.repeatGap;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
}

static void ParseRecordLoadImage(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& loadImageRecord = record.payload.loadImageRecord;
    SetLocationInfo(event, loadImageRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE2;
    memInfo.memType = loadImageRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.addr = loadImageRecord.dst;
    memInfo.blockNum = (loadImageRecord.horSize + loadImageRecord.rPadSize +
        loadImageRecord.lPadSize) * (loadImageRecord.verSize +
            loadImageRecord.topPadSize + loadImageRecord.botPadSize);
    memInfo.blockSize = 32U;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = 1U;
    memInfo.repeatStride = 0U;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
}

static void MaskCountProcessContinuous(std::vector<SanEvent> &events, SanEvent &event)
{
    //  VREDUCEV2的MASK_COUNT机制特殊，repeat_times不变，mask0表示单次repeat的有效数据个数；
    //  VREDUCEV2写事件内存连续
    auto& memInfo = event.eventInfo.memInfo;
    memInfo.blockSize = memInfo.vectorMask.mask0 * memInfo.dataBits * memInfo.repeatTimes / BITS_EACH_BYTE;
    memInfo.blockNum = 1;
    memInfo.repeatTimes = 1;
    events.emplace_back(event);
}

static void ParseRecordLoadSmask(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& loadSmaskRecord = record.payload.loadSmaskRecord;
    SetLocationInfo(event, loadSmaskRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = loadSmaskRecord.srcMemType == MemType::GM ? PipeType::PIPE_MTE2 : PipeType::PIPE_MTE3;
    memInfo.memType = loadSmaskRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.addr = loadSmaskRecord.src;
    memInfo.blockNum = 1U;
    memInfo.blockSize = loadSmaskRecord.smaskSize;
    memInfo.blockStride = 0U;
    memInfo.repeatTimes = 1U;
    memInfo.repeatStride = 0U;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
}

static void MaskCountProcessByRepeat(std::vector<SanEvent> &events, SanEvent &event)
{
    // VCOPY的MASK_COUNT机制特殊，repeat_times不变，mask0表示单次repeat的有效数据个数；
    // 先对每次repeat重新计算blockNum，记为一次内存行为
    auto& memInfo = event.eventInfo.memInfo;
    uint64_t elemEachBlock = memInfo.blockSize * BITS_EACH_BYTE / memInfo.dataBits;
    if (elemEachBlock == 0) {
        SAN_WARN_LOG("Element each block equals zero! (serialNo:%lu)", event.serialNo);
        return;
    }
    uint64_t nBlock = memInfo.vectorMask.mask0 / elemEachBlock;
    if (nBlock > 0) {
        memInfo.blockNum = nBlock;
        events.emplace_back(event);
    }
    // 再判断剩下的数据是否满足repeat结构（要求repeatStride是blockSize的整数倍）
    uint64_t elemLastBlock = memInfo.vectorMask.mask0 - nBlock * elemEachBlock;
    if (elemLastBlock > 0) {
        uint64_t newAddr = memInfo.addr + nBlock * memInfo.blockStride * memInfo.blockSize;
        uint64_t oldRepeat = memInfo.repeatTimes;
        uint64_t oldBlockSize = memInfo.blockSize;
        uint64_t newBlockSize = elemLastBlock * memInfo.dataBits / BITS_EACH_BYTE;
        memInfo.repeatTimes = 1;
        memInfo.blockNum = 1;
        memInfo.blockSize = newBlockSize;
        // 尾块处理，repeatStride 无法保证是 blockSize 的整数倍，每个repeat单独拆成一次内存行为
        for (uint64_t nRepeat = 0; nRepeat < oldRepeat; ++nRepeat) {
            memInfo.addr = newAddr + nRepeat * memInfo.repeatStride * oldBlockSize;
            events.emplace_back(event);
        }
        // 还原 repeatTimes
        memInfo.repeatTimes = oldRepeat;
    }
}

static void MaskCountProcess(std::vector<SanEvent> &events, SanEvent &event,
                             RecordType recordType = RecordType::LOAD)
{
    auto &memInfo = event.eventInfo.memInfo;
    // mask为0，不记录内存行为
    if (memInfo.vectorMask.mask0 == 0 || memInfo.dataBits == 0) {
        return;
    }
    if (recordType == RecordType::VCOPY_OP) {
        MaskCountProcessByRepeat(events, event);
        return;
    }
    if (recordType == RecordType::VREDUCEV2_BINARY || recordType == RecordType::VREDUCEV2_UNARY) {
        if (event.eventInfo.memInfo.opType == AccessType::WRITE) {
            MaskCountProcessContinuous(events, event);
        } else {
            MaskCountProcessByRepeat(events, event);
        }
        return;
    }
    uint64_t elemEachBlock = memInfo.blockSize * BITS_EACH_BYTE / memInfo.dataBits;
    uint64_t elemEachRepeat = memInfo.blockNum * elemEachBlock;
    if (elemEachBlock == 0) {
        SAN_WARN_LOG("Element each block equals zero! (serialNo:%lu)", event.serialNo);
        return;
    }
    uint64_t nRepeat = memInfo.vectorMask.mask0 / elemEachRepeat;
    // mask保留的数据可以组成n次repeat，记一次内存行为
    if (nRepeat > 0) {
        memInfo.repeatTimes = nRepeat;
        events.emplace_back(event);
    }
    uint64_t elemLastRepeat = memInfo.vectorMask.mask0 - nRepeat * elemEachRepeat;
    uint64_t lastBlkNum = elemLastRepeat / elemEachBlock;
    uint64_t initAddr = memInfo.addr;
    // 剩余数据可以组成n次block，记一次内存行为
    if (lastBlkNum > 0) {
        memInfo.addr = initAddr + nRepeat * memInfo.repeatStride * memInfo.blockSize;
        memInfo.blockNum = lastBlkNum;
        memInfo.repeatTimes = 1;
        events.emplace_back(event);
    }
    uint64_t elemLastBlock = elemLastRepeat - lastBlkNum * elemEachBlock;
    // 剩余数据不足以组成一个block，直接记为一次内存行为
    if (elemLastBlock > 0) {
        memInfo.addr = initAddr + nRepeat * memInfo.repeatStride * memInfo.blockSize +
                       lastBlkNum * memInfo.blockStride * memInfo.blockSize;
        memInfo.blockNum = 1;
        memInfo.blockSize = elemLastBlock * memInfo.dataBits / BITS_EACH_BYTE;
        memInfo.repeatTimes = 1;
        events.emplace_back(event);
    }
}

// 此函数用于找到 vector mask 中最后一个 set 的位置，作为要计算的元素个数
inline bool ParseVectorMaskCount(VectorMask const &vectorMask, uint64_t &count)
{
    if (vectorMask.mask0 == 0UL && vectorMask.mask1 == 0UL) {
        count = 0UL;
        return true;
    }
#if defined (__GNUC__)
    // 从最高位开始统计有多少个连续的 zero
    int ret = __builtin_clzl(vectorMask.mask1 == 0UL ? vectorMask.mask0 : vectorMask.mask1);
#else
    int ret = -1;
#endif  // __GNUC__
    if (ret < 0) {
        return false;
    }
    static constexpr uint64_t uint64Bits = 64UL;
    count = uint64Bits - static_cast<uint64_t>(ret);
    if (vectorMask.mask1 != 0UL) {
        count += uint64Bits;
    }
    return true;
}

static void MaskNormalProcess(std::vector<SanEvent> &events, SanEvent &event)
{
    // [WORKAROUND] 此处对 mask normal 模式下的 vector mask 进行临时处理，
    // 后续如果需要更精确的检测需要对此部分进行重构
    auto& memInfo = event.eventInfo.memInfo;
    // 没有需要掩掉的元素，直接返回
    if (memInfo.vectorMask.mask0 == ~0ULL && memInfo.vectorMask.mask1 == ~0ULL) {
        events.emplace_back(event);
        return;
    }
    // 只对 element based 指令进行处理，并且数据类型至少应该是 b16, 才能被 128 bits mask 表示
    if (memInfo.blockNum != 8 || memInfo.blockSize != UB_ALIGN_SIZE || memInfo.dataBits < 16) {
        events.emplace_back(event);
        return;
    }

    // 此处采用变通处理，认为从第一个元素到最后一个 set 位之间的数据都参与计算，此条件下
    // 可等效转化为 MASK_COUNT 模式。
    // 1. 如 (0x00, 0xff) 认为前 8 个元素参与计算
    // 2. 如 (0x00, 0x00ff) 认为前 16 个元素参与计算，因为当前无法处理因 mask 导致首地址不对齐的情况
    uint64_t count {};
    if (!ParseVectorMaskCount(memInfo.vectorMask, count)) {
        events.emplace_back(event);
        return;
    }
    VectorMask normalMask = memInfo.vectorMask;
    memInfo.maskMode = MaskMode::MASK_COUNT;
    memInfo.vectorMask = { count, 0x00 };
    MaskCountProcessByRepeat(events, event);
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.vectorMask = normalMask;
}

static void MaskModeProcess(std::vector<SanEvent> &events, SanEvent &event, RecordType recordType = RecordType::LOAD)
{
    auto& memInfo = event.eventInfo.memInfo;
    // 无效数据，不记录内存行为
    if (memInfo.blockNum == 0 || memInfo.blockSize == 0) {
        return;
    }
    // 不是MASK_COUNT模式或者是AICUBE，直接记录；否则根据mask拆分内存行为（最多拆分成3次，最少0次）
    if (event.loc.blockType == BlockType::AICUBE) {
        return;
    } else if (memInfo.maskMode == MaskMode::MASK_NORM) {
        if (memInfo.repeatTimes == 0) {
            return;
        }
        MaskNormalProcess(events, event);
    } else {
        MaskCountProcess(events, event, recordType);
    }
}

static void ParseVecdupRecord(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& vecDupRecord = record.payload.vecDupRecord;
    SetLocationInfo(event, vecDupRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    memInfo.memType = MemType::UB;
    memInfo.opType = AccessType::WRITE;
    memInfo.vectorMask = vecDupRecord.vectorMask;
    memInfo.maskMode = vecDupRecord.maskMode;
    memInfo.dataBits = vecDupRecord.dataBits;
    memInfo.addr = vecDupRecord.dst;
    memInfo.blockNum = 8U;
    memInfo.blockSize = 32U;
    memInfo.blockStride = vecDupRecord.dstBlockStride;
    memInfo.repeatTimes = vecDupRecord.repeat;
    memInfo.repeatStride = vecDupRecord.dstRepeatStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    MaskModeProcess(events, event, record.recordType);
}

static void ParseRecordUnaryOp(const KernelRecord &record, std::vector<SanEvent> &events)
{
    // 向量单目目计算模式为A x s(标量) => B,这里将A/B看成2个内存事件
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& unaryOpRecord = record.payload.unaryOpRecord;
    SetLocationInfo(event, unaryOpRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    memInfo.memType = MemType::UB;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = unaryOpRecord.vectorMask;
    memInfo.maskMode = unaryOpRecord.maskMode;
    memInfo.dataBits = unaryOpRecord.srcDataBits;
    memInfo.addr = unaryOpRecord.src;
    memInfo.blockNum = unaryOpRecord.srcBlockNum;
    memInfo.blockSize = unaryOpRecord.srcBlockSize;
    memInfo.blockStride = unaryOpRecord.srcBlockStride;
    memInfo.repeatTimes = unaryOpRecord.repeat;
    memInfo.repeatStride = unaryOpRecord.srcRepeatStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    MaskModeProcess(events, event, record.recordType);

    memInfo.opType = AccessType::WRITE;
    memInfo.dataBits = unaryOpRecord.dstDataBits;
    memInfo.addr = unaryOpRecord.dst;
    memInfo.blockNum = unaryOpRecord.dstBlockNum;
    memInfo.blockSize = unaryOpRecord.dstBlockSize;
    memInfo.blockStride = unaryOpRecord.dstBlockStride;
    memInfo.repeatStride = unaryOpRecord.dstRepeatStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    MaskModeProcess(events, event, record.recordType);
}

// 返回false表示用户输入有误、或不需要处理成搬运指令
static bool CheckRecordVms4v2A5(const Vms4v2RecordA5 &vms4V2RecordA5)
{
    auto isCertainValidInputListEmpty = [&vms4V2RecordA5]() {
        for (uint32_t i = 0; i < Vms4v2RecordA5::ARRAY_NUM; ++i) {
            if (((vms4V2RecordA5.validMask & (1 << i)) != 0) && vms4V2RecordA5.elementNum[i] == 0) {
                return true;
            }
        }
        return false;
    };
    if (vms4V2RecordA5.isAllStored) {
        if (isCertainValidInputListEmpty()) {
            SAN_INFO_LOG("VMS4v2 enable isAllStored and element count of certain valid input is 0, treat as nop");
            return false;
        }
    } else if (vms4V2RecordA5.repeat == 0) {
        SAN_INFO_LOG("VMS4v2 repeat is 0, treat as nop");
        return false;
    } else if (vms4V2RecordA5.repeat == 1) {
        if (isCertainValidInputListEmpty()) {
            SAN_INFO_LOG("VMS4v2 repeat is 1 and element count of certain valid input is 0, treat as nop");
            return false;
        }
    } else {
        if (vms4V2RecordA5.validMask != 0xf) {
            SAN_ERROR_LOG("VMS4v2 repeat > 1 and not all input is valid");
            return false;
        }
        for (uint32_t i = 1; i < Vms4v2RecordA5::ARRAY_NUM; ++i) {
            if (vms4V2RecordA5.elementNum[i] != vms4V2RecordA5.elementNum[i - 1]) {
                SAN_ERROR_LOG("VMS4v2 repeat > 1 and element count of lists are not same");
                return false;
            }
        }
        if (vms4V2RecordA5.elementNum[0] == 0) {
            SAN_INFO_LOG("VMS4v2 repeat > 1 and element count is 0, treat as nop");
            return false;
        }
    }
    return true;
}

static void ParseRecordVms4v2A5Repeat1(const KernelRecord &record, SanEvent &event,
                                       std::vector<SanEvent> &events)
{
    auto& memInfo = event.eventInfo.memInfo;
    auto& vms4V2RecordA5 = record.payload.vms4V2RecordA5;
    memInfo.blockSize = 8;
    memInfo.blockStride = 1;
    memInfo.repeatTimes = 1;
    memInfo.repeatStride = 0;
    uint32_t dstBlockNum = 0;
    for (uint32_t i = 0; i < Vms4v2RecordA5::ARRAY_NUM; ++i) {
        if ((vms4V2RecordA5.validMask & (1 << i)) == 0) {
            continue;
        }
        dstBlockNum += vms4V2RecordA5.elementNum[i];
        memInfo.addr = vms4V2RecordA5.src[i];
        memInfo.blockNum = vms4V2RecordA5.elementNum[i];
        events.emplace_back(event);
        AlignChecker::Instance().CheckAlign(event, record.recordType);
    }
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = vms4V2RecordA5.dst;
    memInfo.blockNum = dstBlockNum;
    events.emplace_back(event);
    AlignChecker::Instance().CheckAlign(event, record.recordType);
}

static void ParseRecordVms4v2A5(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& vms4V2RecordA5 = record.payload.vms4V2RecordA5;
    if (!CheckRecordVms4v2A5(vms4V2RecordA5)) {
        return;
    }
    SetLocationInfo(event, vms4V2RecordA5, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    memInfo.memType = MemType::UB;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    // isAllStored模式会根据用户存的数据值对4个list排序，在某个list为空时结束，完全复现的成本太大，先当做repeat = 1处理
    if (vms4V2RecordA5.repeat == 1 || vms4V2RecordA5.isAllStored) {
        ParseRecordVms4v2A5Repeat1(record, event, events);
        return;
    }
    // repeat > 1时，4个输入列表连续且元素数量相同，将4个列表合并为一个读操作
    memInfo.addr = vms4V2RecordA5.src[0];
    memInfo.blockNum = 1;
    memInfo.blockSize = vms4V2RecordA5.elementNum[0] * 32; // 每次读elementNum * 4个元素，每个元素大小为8
    memInfo.blockStride = 1;
    memInfo.repeatTimes = vms4V2RecordA5.repeat;
    memInfo.repeatStride = 1;
    events.emplace_back(event);
    AlignChecker::Instance().CheckAlign(event, record.recordType);

    memInfo.opType = AccessType::WRITE;
    memInfo.addr = vms4V2RecordA5.dst;
    events.emplace_back(event);
    AlignChecker::Instance().CheckAlign(event, record.recordType);
}

static void ParseRecordVgather(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& vgatherRecord = record.payload.vgatherRecord;
    SetLocationInfo(event, vgatherRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    memInfo.memType = MemType::UB;
    memInfo.vectorMask = vgatherRecord.vectorMask;
    memInfo.maskMode = vgatherRecord.maskMode;

    memInfo.opType = AccessType::READ;
    memInfo.dataBits = 32U;
    memInfo.addr = vgatherRecord.src;
    memInfo.blockNum = vgatherRecord.srcBlockNum;
    memInfo.blockSize = vgatherRecord.srcBlockSize;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = 1U;
    memInfo.repeatStride = 1U;
    AlignChecker::Instance().CheckAlign(event, 32U);
    events.emplace_back(event);

    memInfo.opType = AccessType::WRITE;
    memInfo.dataBits = vgatherRecord.dstDataBits;
    memInfo.addr = vgatherRecord.dst;
    memInfo.blockNum = vgatherRecord.dstBlockNum;
    memInfo.blockSize = vgatherRecord.dstBlockSize;
    memInfo.blockStride = vgatherRecord.dstBlockStride;
    memInfo.repeatTimes = vgatherRecord.dstRepeat;
    memInfo.repeatStride = vgatherRecord.dstRepeatStride;
    AlignChecker::Instance().CheckAlign(event, vgatherRecord.dstAlignSize);
    events.emplace_back(event);
}

static void ParseRecordElement(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& elementRecord = record.payload.elementRecord;
    SetLocationInfo(event, elementRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    memInfo.memType = MemType::UB;

    memInfo.opType = elementRecord.accessType;
    memInfo.dataBits = elementRecord.dataBits;
    memInfo.addr = elementRecord.addr;
    memInfo.blockNum = elementRecord.blockNum;
    memInfo.blockSize = elementRecord.blockSize;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = 1U;
    memInfo.repeatStride = 1U;
    AlignChecker::Instance().CheckAlign(event, elementRecord.alignSize);
    events.emplace_back(event);
}

static void ParseRecordBinaryOp(const KernelRecord &record, std::vector<SanEvent> &events)
{
    // 向量双目计算模式为A x B => C,这里将A/B/C看成3个内存事件
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& binaryOpRecord = record.payload.binaryOpRecord;
    SetLocationInfo(event, binaryOpRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;

    // 向量运算的源和目的操作数均在UB中
    memInfo.memType = MemType::UB;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = binaryOpRecord.vectorMask;
    memInfo.maskMode = binaryOpRecord.maskMode;
    memInfo.dataBits = binaryOpRecord.src0DataBits;
    memInfo.addr = binaryOpRecord.src0;
    memInfo.blockNum = binaryOpRecord.src0BlockNum;
    memInfo.blockSize = binaryOpRecord.src0BlockSize;
    memInfo.blockStride = binaryOpRecord.src0BlockStride;
    memInfo.repeatTimes = binaryOpRecord.repeat;
    memInfo.repeatStride = binaryOpRecord.src0RepeatStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    MaskModeProcess(events, event);

    memInfo.dataBits = binaryOpRecord.src1DataBits;
    memInfo.addr = binaryOpRecord.src1;
    memInfo.blockNum = binaryOpRecord.src1BlockNum;
    memInfo.blockSize = binaryOpRecord.src1BlockSize;
    memInfo.blockStride = binaryOpRecord.src1BlockStride;
    memInfo.repeatStride = binaryOpRecord.src1RepeatStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    MaskModeProcess(events, event);

    /// 如果枚举为TERNARY_OP，则代表对dst也存在读的行为，需要增加对dst读的记录
    memInfo.opType = record.recordType == RecordType::TERNARY_OP ?
        AccessType::MEMCPY_BLOCKS : AccessType::WRITE;
    memInfo.dataBits = binaryOpRecord.dstDataBits;
    memInfo.addr = binaryOpRecord.dst;
    memInfo.blockNum = binaryOpRecord.dstBlockNum;
    memInfo.blockSize = binaryOpRecord.dstBlockSize;
    memInfo.blockStride = binaryOpRecord.dstBlockStride;
    memInfo.repeatStride = binaryOpRecord.dstRepeatStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    MaskModeProcess(events, event);
}

static void ParseRecordVsel(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& binaryOpRecord = record.payload.binaryOpRecord;
    SetLocationInfo(event, binaryOpRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
 
    memInfo.memType = MemType::UB;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = binaryOpRecord.vectorMask;
    memInfo.maskMode = binaryOpRecord.maskMode;
    memInfo.dataBits = binaryOpRecord.src0DataBits;
    memInfo.addr = binaryOpRecord.src0;
    memInfo.blockNum = binaryOpRecord.src0BlockNum;
    memInfo.blockSize = binaryOpRecord.src0BlockSize;
    memInfo.blockStride = binaryOpRecord.src0BlockStride;
    memInfo.repeatTimes = binaryOpRecord.repeat;
    memInfo.repeatStride = binaryOpRecord.src0RepeatStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    MaskModeProcess(events, event);
 
    memInfo.dataBits = binaryOpRecord.src1DataBits;
    memInfo.addr = binaryOpRecord.src1;
    memInfo.blockNum = binaryOpRecord.src1BlockNum;
    memInfo.blockSize = binaryOpRecord.src1BlockSize;
    memInfo.blockStride = binaryOpRecord.src1BlockStride;
    memInfo.repeatStride = binaryOpRecord.src1RepeatStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    if (memInfo.blockNum == 1 || memInfo.repeatStride == 1) {
        // 如果 blockNum 为 1 说明是 mode 1，不需要处理 mask
        events.emplace_back(event);
    } else {
        MaskModeProcess(events, event);
    }
 
    memInfo.opType = AccessType::WRITE;
    memInfo.dataBits = binaryOpRecord.dstDataBits;
    memInfo.addr = binaryOpRecord.dst;
    memInfo.blockNum = binaryOpRecord.dstBlockNum;
    memInfo.blockSize = binaryOpRecord.dstBlockSize;
    memInfo.blockStride = binaryOpRecord.dstBlockStride;
    memInfo.repeatStride = binaryOpRecord.dstRepeatStride;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    MaskModeProcess(events, event);
}

static void ParseRecordReduceOp(const KernelRecord &record, std::vector<SanEvent> &events)
{
    // 向量规约计算模式为A => B,这里将A/B看成2个内存事件
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& reduceOpRecord = record.payload.reduceOpRecord;
    SetLocationInfo(event, reduceOpRecord, record.blockType, record.serialNo);
    uint16_t reduceRatio = 1;
    if (reduceOpRecord.maskMode == MaskMode::MASK_COUNT) {
        if (reduceOpRecord.dstBlockSize == 0 || reduceOpRecord.dstBlockNum == 0 ||
            reduceOpRecord.dstDataBitsFactor == 0) {
            SAN_ERROR_LOG("Parse ReduceOp failed, invalid record segment.");
            return;
        }
        reduceRatio = reduceOpRecord.srcBlockNum * reduceOpRecord.srcBlockSize /
            reduceOpRecord.dstDataBitsFactor / reduceOpRecord.dstBlockNum /
            reduceOpRecord.dstBlockSize;
        if (reduceRatio == 0) {
            SAN_ERROR_LOG("Parse ReduceOp failed, reduceRatio can not be zero.");
            return;
        }
    }
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    memInfo.memType = MemType::UB;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = reduceOpRecord.vectorMask;
    memInfo.maskMode = reduceOpRecord.maskMode;
    memInfo.dataBits = reduceOpRecord.srcDataBits;
    memInfo.addr = reduceOpRecord.src;
    memInfo.blockNum = reduceOpRecord.srcBlockNum;
    memInfo.blockSize = reduceOpRecord.srcBlockSize;
    memInfo.blockStride = reduceOpRecord.srcBlockStride;
    memInfo.repeatTimes = reduceOpRecord.repeat;
    memInfo.repeatStride = reduceOpRecord.srcRepeatStride;

    AlignChecker::Instance().CheckAlign(event, record.recordType);
    MaskModeProcess(events, event);

    memInfo.opType = AccessType::WRITE;
    if (memInfo.maskMode == MaskMode::MASK_COUNT) {
        memInfo.vectorMask.mask0 = (memInfo.vectorMask.mask0 + reduceRatio - 1) / reduceRatio;
    }
    memInfo.dataBits = reduceOpRecord.dstDataBits;
    memInfo.addr = reduceOpRecord.dst;
    memInfo.blockNum = reduceOpRecord.dstBlockNum;
    memInfo.blockSize = reduceOpRecord.dstRepeatLength;
    /// 归约类计算 dst 数据一个 repeat 只有一个 block，不存在 dstBlockStride
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = reduceOpRecord.repeat;
    memInfo.repeatStride = reduceOpRecord.dstRepeatStride;
    AlignChecker::Instance().CheckAlign(event, reduceOpRecord.dstAlignSize);
    MaskModeProcess(events, event);
}

// 用滑窗统计 mask 中前 readCount 个 bit 位中 1 的个数
static uint64_t CountElemByMask(uint64_t mask, uint64_t readCount)
{
    uint64_t readMask = readCount == 64 ? ~0ULL : (1ULL << readCount) - 1;
    mask = mask & readMask;
    auto writeCount = __builtin_popcountl(mask);
    return writeCount < 0 ? 0 : static_cast<uint64_t>(writeCount);
}

static uint64_t CountElemByPattern(std::pair<uint64_t, uint64_t> const &pattern, uint64_t readCount)
{
    static constexpr uint64_t maskBits = 64UL;
    uint64_t loop = readCount / maskBits;
    uint64_t remain = readCount % maskBits;
    return loop * pattern.second + CountElemByMask(pattern.first, remain);
}

static uint64_t CountElemByCompareMask(CompareMask const &mask, uint16_t dataBytes, uint64_t readCount)
{
    static constexpr uint64_t maskBits = 64UL;
    uint64_t mask0Bits = CountElemByMask(mask.mask0, maskBits);
    uint64_t mask1Bits = CountElemByMask(mask.mask1, maskBits);
    uint64_t loop = readCount / maskBits;
    uint64_t remain = readCount % maskBits;
    if (dataBytes == 2U) {
        uint64_t mask1Loop = loop / 2UL;
        uint64_t mask0Loop = loop - mask1Loop;
        uint64_t remainCount = CountElemByMask(mask0Loop == mask1Loop ? mask.mask0 : mask.mask1, remain);
        return mask0Loop * mask0Bits + mask1Loop * mask1Bits + remainCount;
    } else {
        return loop * mask0Bits + CountElemByMask(mask.mask0, remain);
    }
}

static uint64_t ParseReduceV2WriteElemCount(ReduceV2Record const &record)
{
    // pattern mode 映射到 compare mask 和有效 bit 数
    static const std::vector<std::pair<uint64_t, uint64_t>> maskMap = {
        { 0x0UL, 0UL },
        { 0x5555555555555555UL, 32UL },  // 01010101...
        { 0xAAAAAAAAAAAAAAAAUL, 32UL },  // 10101010...
        { 0x1111111111111111UL, 16UL },  // 00010001...
        { 0x2222222222222222UL, 16UL },  // 00100010...
        { 0x4444444444444444UL, 16UL },  // 01000100...
        { 0x8888888888888888UL, 16UL },  // 10001000...
        { 0xFFFFFFFFFFFFFFFFUL, 64UL },  // 11111111...
    };

    uint64_t elemReadEachRepeat = record.maskMode == MaskMode::MASK_NORM ?
        256U / record.dataBytes : record.vectorMask.mask0;
    uint64_t elemWriteEachRepeat = record.patternMode == 0 ?
        CountElemByCompareMask(record.compareMask, record.dataBytes, elemReadEachRepeat) :
        CountElemByPattern(maskMap[record.patternMode], elemReadEachRepeat);

    return elemWriteEachRepeat * record.repeat;
}

static void ParseRecordReduceV2(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    ReduceV2Record const &reduceV2Record = record.payload.reduceV2Record;
    SetLocationInfo(event, reduceV2Record, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    memInfo.memType = MemType::UB;
    memInfo.vectorMask = reduceV2Record.vectorMask;
    memInfo.maskMode = reduceV2Record.maskMode;
    memInfo.dataBits = reduceV2Record.dataBytes * BITS_EACH_BYTE;

    // src0 read
    memInfo.opType = AccessType::READ;
    memInfo.addr = reduceV2Record.src0;
    memInfo.blockNum = 8U;
    memInfo.blockSize = 32U;
    memInfo.blockStride = reduceV2Record.src0BlockStride;
    memInfo.repeatTimes = reduceV2Record.repeat;
    memInfo.repeatStride = reduceV2Record.src0RepeatStride;
    AlignChecker::Instance().CheckAlign(event, UB_ALIGN_SIZE);
    if (reduceV2Record.maskMode == MaskMode::MASK_NORM) {
        // MASK_NORM 模式下，vector mask 会被忽略，每个 repeat 固定读 256 bytes
        events.emplace_back(event);
    } else {
        // MASK_COUNT 模式下，MASK[31:0] 保存了每个 repeat 要处理的元素个数
        MaskCountProcessByRepeat(events, event);
    }

    // src1 read 只有 pattern mode 为 0 时才会读取 src1
    if (reduceV2Record.patternMode == 0U) {
        memInfo.opType = AccessType::READ;
        memInfo.addr = reduceV2Record.src1;
        // 每个 repeat 计算元素个数，即为 compare mask 要读取的 bit 数
        memInfo.blockNum = 256U / reduceV2Record.dataBytes / BITS_EACH_BYTE;
        memInfo.blockSize = 1U;
        memInfo.blockStride = 1U;
        memInfo.repeatTimes = reduceV2Record.repeat;
        memInfo.repeatStride = reduceV2Record.src1RepeatStride * UB_ALIGN_SIZE;
        AlignChecker::Instance().CheckAlign(event, UB_ALIGN_SIZE);
        events.emplace_back(event);
    }

    // dst write 不管是 repeat 还是 block 间数据都是连续写入的，因此只需要计算一共
    // 写入了多少元素
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = reduceV2Record.dst;
    memInfo.blockNum = 1U;
    memInfo.blockSize = ParseReduceV2WriteElemCount(reduceV2Record) * reduceV2Record.dataBytes;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = 1U;
    memInfo.repeatStride = 1U;
    AlignChecker::Instance().CheckAlign(event, UB_ALIGN_SIZE);
    events.emplace_back(event);
}

static void ParseMatrixMulOpRecord(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& matrixMulOpRecord = record.payload.matrixMulOpRecord;
    SetLocationInfo(event, matrixMulOpRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_M;
    // matrixA,L0A
    memInfo.memType = MemType::L0A;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.addr = matrixMulOpRecord.src0;
    memInfo.blockNum = matrixMulOpRecord.src0BlockNum;
    memInfo.blockSize = matrixMulOpRecord.src0BlockSize;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = matrixMulOpRecord.src0Repeat;
    memInfo.repeatStride = matrixMulOpRecord.src0RepeatStride;
    AlignChecker::Instance().CheckAlign(event, matrixMulOpRecord.src0AlignSize);
    events.emplace_back(event);
    // matrixB,L0B
    memInfo.memType = MemType::L0B;
    memInfo.addr = matrixMulOpRecord.src1;
    memInfo.blockNum = matrixMulOpRecord.src1BlockNum;
    memInfo.blockSize = matrixMulOpRecord.src1BlockSize;
    memInfo.repeatTimes = 1U;
    memInfo.repeatStride = 0U;
    AlignChecker::Instance().CheckAlign(event, matrixMulOpRecord.src1AlignSize);
    events.emplace_back(event);
    // matrixC,LOC
    // 参考ISA手册，当(cmatrixInitVal == 0) && (cmatrixSource == 0)时，L0C存在一次读写操作，否则只存在写操作。
    memInfo.opType = (matrixMulOpRecord.cmatrixInitVal == 0 && matrixMulOpRecord.cmatrixSource == 0) ?
        AccessType::MEMCPY_BLOCKS : AccessType::WRITE;
    memInfo.memType = MemType::L0C;
    memInfo.addr = matrixMulOpRecord.dst;
    memInfo.blockNum = matrixMulOpRecord.dstBlockNum;
    memInfo.blockSize = matrixMulOpRecord.dstBlockSize;
    AlignChecker::Instance().CheckAlign(event, matrixMulOpRecord.dstAlignSize);
    events.emplace_back(event);

    // 指令使能enUnitFlag，需要插入set_flag/wait_flag模拟同步行为
    if (record.payload.matrixMulOpRecord.enUnitFlag) {
        CreateUnitFlagSyncEvent(record, events, record.payload.matrixMulOpRecord, true);
    }
}

static void ParseMmadA5Record(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& mmadA5Record = record.payload.mmadA5Record;
    SetLocationInfo(event, mmadA5Record, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_M;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = 1U;
    memInfo.repeatStride = 0U;
    // matrixA,L0A
    constexpr uint16_t SRC_BLOCK_SIZE = 512; // L0A/B每个分形512B
    memInfo.memType = MemType::L0A;
    memInfo.opType = AccessType::READ;
    memInfo.addr = mmadA5Record.src0;
    memInfo.blockNum = mmadA5Record.src0BlockNum;
    memInfo.blockSize = SRC_BLOCK_SIZE;
    AlignChecker::Instance().CheckAlign(event, mmadA5Record.src0AlignSize);
    events.emplace_back(event);
    // matrixB,L0B
    memInfo.memType = MemType::L0B;
    memInfo.addr = mmadA5Record.src1;
    memInfo.blockNum = mmadA5Record.src1BlockNum;
    AlignChecker::Instance().CheckAlign(event, mmadA5Record.src1AlignSize);
    events.emplace_back(event);
    // matrixC,LOC
    // 参考ISA手册，当(cmatrixInitVal == 0) && (cmatrixSource == 0)时，L0C存在一次读写操作，否则只存在写操作。
    memInfo.opType = (mmadA5Record.cmatrixInitVal == 0 && mmadA5Record.cmatrixSource == 0) ?
                     AccessType::MEMCPY_BLOCKS : AccessType::WRITE;
    memInfo.memType = MemType::L0C;
    memInfo.addr = mmadA5Record.dst;
    memInfo.blockNum = mmadA5Record.dstBlockNum;
    constexpr uint16_t DST_BLOCK_SIZE = 1024; // 固定1024大小/对齐，m * n * sizeof(dstDType) = 16 * 16 * 4
    memInfo.blockSize = DST_BLOCK_SIZE;
    AlignChecker::Instance().CheckAlign(event, DST_BLOCK_SIZE);
    events.emplace_back(event);

    // 指令使能enUnitFlag，需要插入set_flag/wait_flag模拟同步行为
    if (mmadA5Record.enUnitFlag) {
        CreateUnitFlagSyncEvent(record, events, mmadA5Record, true);
    }
}

static void ParseRecordSetFlag(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    SetLocationInfo(event, record.payload.syncRecord, record.blockType, record.serialNo);
    event.type = EventType::SYNC_EVENT;
    event.pipe = record.payload.syncRecord.src;
    event.eventInfo.syncInfo.opType = SyncType::SET_FLAG;
    event.eventInfo.syncInfo.srcPipe = record.payload.syncRecord.src;
    event.eventInfo.syncInfo.dstPipe = record.payload.syncRecord.dst;
    event.eventInfo.syncInfo.eventId = static_cast<uint32_t>(record.payload.syncRecord.eventID);
    event.eventInfo.syncInfo.memType = MemType::INVALID;
    event.eventInfo.syncInfo.isRetrogress = false;
    event.eventInfo.syncInfo.isGenerated = record.payload.syncRecord.isGenerated;
    events.emplace_back(event);
}

static void ParseRecordWaitFlag(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    SetLocationInfo(event, record.payload.syncRecord, record.blockType, record.serialNo);
    event.type = EventType::SYNC_EVENT;
    event.pipe = record.payload.syncRecord.dst;
    event.eventInfo.syncInfo.opType = SyncType::WAIT_FLAG;
    event.eventInfo.syncInfo.srcPipe = record.payload.syncRecord.src;
    event.eventInfo.syncInfo.dstPipe = record.payload.syncRecord.dst;
    event.eventInfo.syncInfo.eventId = static_cast<uint32_t>(record.payload.syncRecord.eventID);
    event.eventInfo.syncInfo.memType = MemType::INVALID;
    event.eventInfo.syncInfo.isRetrogress = false;
    event.eventInfo.syncInfo.isGenerated = record.payload.syncRecord.isGenerated;
    events.emplace_back(event);
}

static void ParseRecordGetBuf(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    SetLocationInfo(event, record.payload.bufRecord, record.blockType, record.serialNo);
    event.type = EventType::BUF_SYNC_EVENT;
    event.pipe = record.payload.bufRecord.pipe;
    event.eventInfo.bufSyncInfo.opType = SyncType::GET_BUF;
    event.eventInfo.bufSyncInfo.pipe = record.payload.bufRecord.pipe;
    event.eventInfo.bufSyncInfo.bufId = record.payload.bufRecord.bufId;
    event.eventInfo.bufSyncInfo.mode = record.payload.bufRecord.mode;
    if (event.eventInfo.bufSyncInfo.mode == BufMode::BLOCK_MODE) {
        event.eventInfo.bufSyncInfo.rlsCount = RecordParse::getRlsBufMap_[event.eventInfo.bufSyncInfo.bufId];
    }
    events.emplace_back(event);
}

static void ParseRecordRlsBuf(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    SetLocationInfo(event, record.payload.bufRecord, record.blockType, record.serialNo);
    event.type = EventType::BUF_SYNC_EVENT;
    event.pipe = record.payload.bufRecord.pipe;
    event.eventInfo.bufSyncInfo.opType = SyncType::RLS_BUF;
    event.eventInfo.bufSyncInfo.pipe = record.payload.bufRecord.pipe;
    event.eventInfo.bufSyncInfo.bufId = record.payload.bufRecord.bufId;
    event.eventInfo.bufSyncInfo.mode = record.payload.bufRecord.mode;
    RecordParse::getRlsBufMap_[event.eventInfo.bufSyncInfo.bufId]++;
    events.emplace_back(event);
}

static void ParseFftsSyncRecord(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    SetLocationInfo(event, record.payload.fftsSyncRecord, record.blockType, record.serialNo);
    event.type = EventType::CROSS_CORE_SYNC_EVENT;
    event.pipe = record.payload.fftsSyncRecord.dst;
    event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
    event.eventInfo.fftsSyncInfo.dstPipe = record.payload.fftsSyncRecord.dst;
    event.eventInfo.fftsSyncInfo.flagId = record.payload.fftsSyncRecord.flagID;
    event.eventInfo.fftsSyncInfo.mode = record.payload.fftsSyncRecord.mode;
    event.eventInfo.fftsSyncInfo.vecSubBlockDim = record.payload.fftsSyncRecord.vecSubBlockDim;
    events.emplace_back(event);
    /// -> ffts_cross_core_sync(pipe_t pipe, ...)的pipe参数类似barrier中的pipeline，
    ///    表示同步点在哪个pipeline中，当该pipeline前面所有指令执行完毕后达到同步点
    /// -> wait_flag_dev(int64_t flagID)和ffts_cross_core_sync配套使用（通过flagID对应），
    ///    功能为等待所有同步对象到达flagID对应的同步点
    /// -> 此处选择在ffts_cross_core_sync中插入pipe_barrier(pipe)
    SanEvent barrierEvent = CreateInnerPipeSyncEvent(
        record, record.payload.fftsSyncRecord.dst, record.payload.fftsSyncRecord);
    events.emplace_back(barrierEvent);
}

static void ParseWaitDevRecord(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    SetLocationInfo(event, record.payload.waitFlagDevRecord, record.blockType, record.serialNo);
    event.type = EventType::CROSS_CORE_SYNC_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
    event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
    event.eventInfo.fftsSyncInfo.flagId = record.payload.waitFlagDevRecord.flagID;
    events.emplace_back(event);
}

static void ParseWaitDevPipeRecord(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    SetLocationInfo(event, record.payload.waitFlagDevPipeRecord, record.blockType, record.serialNo);
    event.type = EventType::CROSS_CORE_SYNC_EVENT;
    event.pipe = record.payload.waitFlagDevPipeRecord.pipe;
    event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
    event.eventInfo.fftsSyncInfo.dstPipe = record.payload.waitFlagDevPipeRecord.pipe;
    event.eventInfo.fftsSyncInfo.flagId = record.payload.waitFlagDevPipeRecord.flagID;
    events.emplace_back(event);
}

static void ParseSetIntraBlockRecord(const KernelRecord &record, std::vector<SanEvent> &events)
{
    /// -> set_intra_block(pipe_t pipe, ...)
    ///    表示同步点在哪个pipeline中，当该pipeline前面所有指令执行完毕后达到同步点
    SanEvent event;
    SetLocationInfo(event, record.payload.intraBlockSyncRecord, record.blockType, record.serialNo);
    event.type = EventType::CROSS_CORE_SYNC_EVENT;
    event.pipe = record.payload.intraBlockSyncRecord.pipe;
    event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
    event.eventInfo.fftsSyncInfo.dstPipe = record.payload.intraBlockSyncRecord.pipe;
    event.eventInfo.fftsSyncInfo.flagId = record.payload.intraBlockSyncRecord.syncID;
    event.eventInfo.fftsSyncInfo.mode = 4;
    event.eventInfo.fftsSyncInfo.vecSubBlockDim = record.payload.intraBlockSyncRecord.vecSubBlockDim;
    events.emplace_back(event);
}

static void ParseWaitIntraBlockRecord(const KernelRecord &record, std::vector<SanEvent> &events)
{
    /// -> wait_intar_block(uint64_t syncID)和set_intra_block配套使用（通过syncID对应），
    ///    功能为等待所有同步对象到达syncID对应的同步点
    SanEvent event;
    SetLocationInfo(event, record.payload.intraBlockSyncRecord, record.blockType, record.serialNo);
    event.type = EventType::CROSS_CORE_SYNC_EVENT;
    event.pipe = record.payload.intraBlockSyncRecord.pipe;
    event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_INTRA_BLOCK;
    event.eventInfo.fftsSyncInfo.dstPipe = record.payload.intraBlockSyncRecord.pipe;
    event.eventInfo.fftsSyncInfo.flagId = record.payload.intraBlockSyncRecord.syncID;
    events.emplace_back(event);
}

static void ParseRecordIBSetStub(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    SetLocationInfo(event, record.payload.softSyncRecord, record.blockType, record.serialNo);
    event.type = EventType::CROSS_CORE_SOFT_SYNC_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.softSyncInfo.opType = SyncType::IB_SET;
    event.eventInfo.softSyncInfo.eventID = record.payload.softSyncRecord.eventID;
    event.eventInfo.softSyncInfo.waitCoreID = record.payload.softSyncRecord.waitCoreID;
    event.eventInfo.softSyncInfo.isAIVOnly = record.payload.softSyncRecord.isAIVOnly;
    events.emplace_back(event);
}

static void ParseRecordIBWaitStub(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    SetLocationInfo(event, record.payload.softSyncRecord, record.blockType, record.serialNo);
    event.type = EventType::CROSS_CORE_SOFT_SYNC_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.softSyncInfo.opType = SyncType::IB_WAIT;
    event.eventInfo.softSyncInfo.eventID = record.payload.softSyncRecord.eventID;
    event.eventInfo.softSyncInfo.waitCoreID = record.payload.softSyncRecord.waitCoreID;
    event.eventInfo.softSyncInfo.isAIVOnly = record.payload.softSyncRecord.isAIVOnly;
    events.emplace_back(event);
}

static void ParseRecordSyncAllStub(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    SetLocationInfo(event, record.payload.softSyncRecord, record.blockType, record.serialNo);
    event.type = EventType::CROSS_CORE_SYNC_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
    event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
    constexpr uint8_t virtualFlagId = 31;
    event.eventInfo.fftsSyncInfo.flagId = virtualFlagId;
    event.eventInfo.fftsSyncInfo.mode = 0;
    event.eventInfo.fftsSyncInfo.vecSubBlockDim = 2;
    events.emplace_back(event);

    event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
    events.emplace_back(event);
}

static void ParseRecordHsetFlag(const KernelRecord &record, std::vector<SanEvent> &events)
{
    if (!IsValidHardSyncRecord(record)) {
        SAN_ERROR_LOG("Parse HsetFlag failed, invalid kernelNo:%lld", static_cast<long long>(record.serialNo));
        return;
    }
    SanEvent event;
    SetLocationInfo(event, record.payload.hardSyncRecord, record.blockType, record.serialNo);
    event.type = EventType::H_SYNC_EVENT;
    event.pipe = record.payload.hardSyncRecord.src;
    event.eventInfo.hsyncInfo.opType = SyncType::HSET_FLAG;
    event.eventInfo.hsyncInfo.srcPipe = record.payload.hardSyncRecord.src;
    event.eventInfo.hsyncInfo.dstPipe = record.payload.hardSyncRecord.dst;
    event.eventInfo.hsyncInfo.eventId = record.payload.hardSyncRecord.eventID;
    event.eventInfo.hsyncInfo.memType = record.payload.hardSyncRecord.memory;
    event.eventInfo.hsyncInfo.v = record.payload.hardSyncRecord.v;
    events.emplace_back(event);
}

static void ParseRecordHwaitFlag(const KernelRecord &record, std::vector<SanEvent> &events)
{
    if (!IsValidHardSyncRecord(record)) {
        SAN_ERROR_LOG("Parse HwaitFlag failed, invalid kernelNo:%lld", static_cast<long long>(record.serialNo));
        return;
    }
    SanEvent event;
    SetLocationInfo(event, record.payload.hardSyncRecord, record.blockType, record.serialNo);
    event.type = EventType::H_SYNC_EVENT;
    event.pipe = record.payload.hardSyncRecord.dst;
    event.eventInfo.hsyncInfo.opType = SyncType::HWAIT_FLAG;
    event.eventInfo.hsyncInfo.srcPipe = record.payload.hardSyncRecord.src;
    event.eventInfo.hsyncInfo.dstPipe = record.payload.hardSyncRecord.dst;
    event.eventInfo.hsyncInfo.eventId = record.payload.hardSyncRecord.eventID;
    event.eventInfo.hsyncInfo.memType = record.payload.hardSyncRecord.memory;
    event.eventInfo.hsyncInfo.v = record.payload.hardSyncRecord.v;
    events.emplace_back(event);
}

static void ParseRecordPipeBarrier(const KernelRecord &record, std::vector<SanEvent> &events)
{
    PipeType pipeType = record.payload.pipeBarrierRecord.pipe;
    if (pipeType == PipeType::PIPE_ALL) {
        CreatePipeAllSyncEvent(record, events, record.payload.pipeBarrierRecord);
    } else {
        SanEvent barrierEvent = CreateInnerPipeSyncEvent(record, pipeType, record.payload.pipeBarrierRecord);
        events.emplace_back(barrierEvent);
    }
}

std::map<uint16_t, bool>& GetMapAtomicMode()
{
// 原子写模式开启期间的标志位
    thread_local static std::map<uint16_t, bool> mapAtomicMode;
    return mapAtomicMode;
}

static void ParseRecordSetAtomic(const KernelRecord &record, std::vector<SanEvent>&)
{
    bool atomicEnabled;
    if (FormatConverter::GetAtomicFlag(record, atomicEnabled)) {
        GetMapAtomicMode()[record.payload.atomicModeRecord.location.blockId] = atomicEnabled;
    }
}

static void AsignRecordHcclRead(MemOpInfo &eventMemInfo, const MstxHcclRecord &hcclCoreRecord)
{
    eventMemInfo.blockNum = 1;
    eventMemInfo.blockSize = hcclCoreRecord.srcCount * hcclCoreRecord.srcDataTypeSize;
    eventMemInfo.blockStride = 0;
    eventMemInfo.repeatTimes = hcclCoreRecord.repeat;
    eventMemInfo.repeatStride = hcclCoreRecord.srcRepeatStride;
    eventMemInfo.opType =  AccessType::READ;
}

static void AsignRecordHcclWrite(MemOpInfo &eventMemInfo, const MstxHcclRecord &hcclCoreRecord)
{
    eventMemInfo.blockNum = 1;
    eventMemInfo.blockSize = hcclCoreRecord.dstCount * hcclCoreRecord.dstDataTypeSize;
    eventMemInfo.blockStride = 0;
    eventMemInfo.repeatTimes = hcclCoreRecord.repeat;
    eventMemInfo.repeatStride = hcclCoreRecord.dstRepeatStride;
    eventMemInfo.opType =  AccessType::WRITE;
}

static void ParseRecordMstxHCCL(const KernelRecord &record, std::vector<SanEvent> &events, SanEvent &event)
{
    auto &mstxRecord = record.payload.mstxRecord;
    auto &eventMemInfo = event.eventInfo.memInfo;
    eventMemInfo.memType = MemType::GM;

    // 分为读写两个事件，读事件中记录读取的地址，写事件中记录写入的地址
    if (mstxRecord.interfaceType == InterfaceType::MSTX_HCCL) {
        event.type = EventType::MEM_EVENT;
        auto &hcclCoreRecord = mstxRecord.interface.mstxHcclRecord;
        // hccl接口为不对称接口，读事件和写事件的个数不一致
        if (static_cast<HcclFlagId>(hcclCoreRecord.flagId) == HcclFlagId::ALLREDUCE) {
            // 此时解析的接口是AllReduce
            eventMemInfo.addr = hcclCoreRecord.src;
            AsignRecordHcclRead(eventMemInfo, hcclCoreRecord);
            events.emplace_back(event);
            eventMemInfo.addr = hcclCoreRecord.dst;
            AsignRecordHcclWrite(eventMemInfo, hcclCoreRecord);
            events.emplace_back(event);
        } else if (static_cast<HcclFlagId>(hcclCoreRecord.flagId) == HcclFlagId::ALLGATHER) {
            // 此时解析的接口是AllGather,读事件1个写事件rankDim个
            eventMemInfo.addr = hcclCoreRecord.src;
            AsignRecordHcclRead(eventMemInfo, hcclCoreRecord);
            events.emplace_back(event);
            for (uint32_t i = 0; i < static_cast<uint32_t>(hcclCoreRecord.rankDim); i++) {
                eventMemInfo.addr = hcclCoreRecord.dst + i * hcclCoreRecord.dstStride * hcclCoreRecord.dstDataTypeSize;
                AsignRecordHcclWrite(eventMemInfo, hcclCoreRecord);
                events.emplace_back(event);
            }
        } else if (static_cast<HcclFlagId>(hcclCoreRecord.flagId) == HcclFlagId::REDUCESCATTER) {
            // 此时解析的接口是reduceScatter,读事件rankDim个写事件1个
            for (uint32_t i = 0; i < static_cast<uint32_t>(hcclCoreRecord.rankDim); i++) {
                eventMemInfo.addr = hcclCoreRecord.src + i * hcclCoreRecord.srcStride * hcclCoreRecord.srcDataTypeSize;
                AsignRecordHcclRead(eventMemInfo, hcclCoreRecord);
                events.emplace_back(event);
            }
            eventMemInfo.addr = hcclCoreRecord.dst;
            AsignRecordHcclWrite(eventMemInfo, hcclCoreRecord);
            events.emplace_back(event);
        } else if (static_cast<HcclFlagId>(hcclCoreRecord.flagId) == HcclFlagId::ALLTOALL) {
            // 此时解析的接口是AlltoAll,读事件rankDim个写事件rankDim个
            for (uint32_t i = 0; i < static_cast<uint32_t>(hcclCoreRecord.rankDim); i++) {
                eventMemInfo.addr = hcclCoreRecord.src + i * hcclCoreRecord.srcStride * hcclCoreRecord.srcDataTypeSize;
                AsignRecordHcclRead(eventMemInfo, hcclCoreRecord);
                events.emplace_back(event);
            }
            for (uint32_t i = 0; i < static_cast<uint32_t>(hcclCoreRecord.rankDim); i++) {
                eventMemInfo.addr = hcclCoreRecord.dst + i * hcclCoreRecord.dstStride * hcclCoreRecord.dstDataTypeSize;
                AsignRecordHcclWrite(eventMemInfo, hcclCoreRecord);
                events.emplace_back(event);
            }
        }
    }
}

static void ParseMstxCrossCoreBarrier(const KernelRecord &record, std::vector<SanEvent> &events)
{
    auto &mstxRecord = record.payload.mstxRecord;
    auto &mstxCrossCoreBarrier = mstxRecord.interface.mstxCrossCoreBarrier;
    if (mstxCrossCoreBarrier.usedCoreNum != 0) {
        SAN_WARN_LOG("MstxCrossCoreBarrier using some cores is unsupported yet. Barrier all cores as default.");
    }
    if (mstxCrossCoreBarrier.usedCoreId != nullptr) {
        SAN_WARN_LOG("MstxCrossCoreBarrier using certain core ids is unsupported yet. Barrier all cores as default.");
    }
    if (!mstxCrossCoreBarrier.isAIVOnly) {
        SAN_WARN_LOG("MstxCrossCoreBarrier barring both AIV and AIC cores is unsupported yet. Use AIV only as default.");
    }

    if (mstxCrossCoreBarrier.pipeBarrierAll) {
        CreatePipeAllSyncEvent(record, events, mstxRecord);
    }

    SanEvent event;
    SetLocationInfo(event, record.payload.mstxRecord, record.blockType, record.serialNo);
    event.type = EventType::CROSS_CORE_SYNC_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
    event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
    constexpr uint8_t virtualFlagId = 31;
    event.eventInfo.fftsSyncInfo.flagId = virtualFlagId;
    event.eventInfo.fftsSyncInfo.mode = 0;
    event.eventInfo.fftsSyncInfo.vecSubBlockDim = 2;
    events.emplace_back(event);

    event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
    events.emplace_back(event);
}

static void ParseMstxCrossCoreSetFlag(const KernelRecord &record, std::vector<SanEvent> &events)
{
    auto &mstxRecord = record.payload.mstxRecord;
    auto &mstxCrossCoreSetFlag = mstxRecord.interface.mstxCrossCoreSetFlag;
    if (mstxCrossCoreSetFlag.peerCoreId >= 0) {
        SAN_WARN_LOG("MstxCrossCoreSetFlag emits signal to certain core is unsupported.");
    }

    if (mstxCrossCoreSetFlag.pipeBarrierAll) {
        CreatePipeAllSyncEvent(record, events, mstxRecord);
    }

    SanEvent event;
    SetLocationInfo(event, record.payload.mstxRecord, record.blockType, record.serialNo);
    event.type = EventType::CROSS_CORE_SOFT_SYNC_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.softSyncInfo.opType = SyncType::IB_SET;
    event.eventInfo.softSyncInfo.eventID = mstxCrossCoreSetFlag.eventId;
    event.eventInfo.softSyncInfo.waitCoreID = mstxCrossCoreSetFlag.peerCoreId;
    event.eventInfo.softSyncInfo.isAIVOnly = true;
    events.emplace_back(event);
}

static void ParseMstxCrossCoreWaitFlag(const KernelRecord &record, std::vector<SanEvent> &events)
{
    auto &mstxRecord = record.payload.mstxRecord;
    auto &mstxCrossCoreWaitFlag = mstxRecord.interface.mstxCrossCoreWaitFlag;

    if (mstxCrossCoreWaitFlag.pipeBarrierAll) {
        CreatePipeAllSyncEvent(record, events, mstxRecord);
    }

    SanEvent event;
    SetLocationInfo(event, record.payload.mstxRecord, record.blockType, record.serialNo);
    event.type = EventType::CROSS_CORE_SOFT_SYNC_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.softSyncInfo.opType = SyncType::IB_WAIT;
    event.eventInfo.softSyncInfo.eventID = mstxCrossCoreWaitFlag.eventId;
    event.eventInfo.softSyncInfo.waitCoreID = mstxCrossCoreWaitFlag.peerCoreId;
    event.eventInfo.softSyncInfo.isAIVOnly = true;
    events.emplace_back(event);
}

static void ParseMstxSignalSet(const KernelRecord &record, std::vector<SanEvent> &events)
{
    auto &mstxRecord = record.payload.mstxRecord;
    auto &mstxSignalSet = mstxRecord.interface.mstxSignalSet;

    SanEvent event;
    SetLocationInfo(event, record.payload.mstxRecord, record.blockType, record.serialNo);
    event.type = EventType::MSTX_SIGNAL_SET_EVENT;
    event.pipe = PipeType::PIPE_S_CAL;
    event.eventInfo.mstxSignalSet = mstxSignalSet;
    events.emplace_back(event);
}

static void ParseMstxSignalWait(const KernelRecord &record, std::vector<SanEvent> &events)
{
    auto &mstxRecord = record.payload.mstxRecord;
    auto &mstxSignalWait = mstxRecord.interface.mstxSignalWait;

    SanEvent event;
    SetLocationInfo(event, record.payload.mstxRecord, record.blockType, record.serialNo);
    event.type = EventType::MSTX_SIGNAL_WAIT_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.mstxSignalWait = mstxSignalWait;
    events.emplace_back(event);
}

static void ParseRecordMstxVecUnary(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto &memInfo = event.eventInfo.memInfo;
    auto &mstxRecord = record.payload.mstxRecord;
    auto &mstxVecUnary = mstxRecord.interface.mstxVecUnaryDesc;
    SetLocationInfo(event, mstxRecord, record.blockType, record.serialNo);

    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    memInfo.memType = MemType::UB;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = mstxVecUnary.wrapper.mask;
    memInfo.maskMode = mstxVecUnary.wrapper.maskMode;
    memInfo.dataBits = mstxVecUnary.src.dataBits;
    memInfo.addr = mstxVecUnary.src.addr;
    memInfo.blockNum = mstxVecUnary.blockNum;
    memInfo.blockSize = 32;
    memInfo.blockStride = mstxVecUnary.srcBlockStride;
    memInfo.repeatTimes = mstxVecUnary.repeatTimes;
    memInfo.repeatStride = mstxVecUnary.srcRepeatStride;
    MaskModeProcess(events, event, RecordType::UNARY_OP);

    memInfo.opType = AccessType::WRITE;
    memInfo.dataBits = mstxVecUnary.dst.dataBits;
    memInfo.addr = mstxVecUnary.dst.addr;
    memInfo.blockStride = mstxVecUnary.dstBlockStride;
    memInfo.repeatStride = mstxVecUnary.dstRepeatStride;
    MaskModeProcess(events, event, RecordType::UNARY_OP);
}

static void ParseRecordMstxVecBinary(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto &memInfo = event.eventInfo.memInfo;
    auto &mstxRecord = record.payload.mstxRecord;
    auto &mstxVecBinary = mstxRecord.interface.mstxVecBinaryDesc;
    SetLocationInfo(event, mstxRecord, record.blockType, record.serialNo);

    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    memInfo.memType = MemType::UB;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = mstxVecBinary.wrapper.mask;
    memInfo.maskMode = mstxVecBinary.wrapper.maskMode;
    memInfo.dataBits = mstxVecBinary.src0.dataBits;
    memInfo.addr = mstxVecBinary.src0.addr;
    memInfo.blockNum = mstxVecBinary.blockNum;
    memInfo.blockSize = 32;
    memInfo.blockStride = mstxVecBinary.src0BlockStride;
    memInfo.repeatTimes = mstxVecBinary.repeatTimes;
    memInfo.repeatStride = mstxVecBinary.src0RepeatStride;
    MaskModeProcess(events, event, RecordType::BINARY_OP);

    memInfo.opType = AccessType::READ;
    memInfo.dataBits = mstxVecBinary.src1.dataBits;
    memInfo.addr = mstxVecBinary.src1.addr;
    memInfo.blockStride = mstxVecBinary.src1BlockStride;
    memInfo.repeatStride = mstxVecBinary.src1RepeatStride;
    MaskModeProcess(events, event, RecordType::BINARY_OP);

    memInfo.opType = AccessType::WRITE;
    memInfo.dataBits = mstxVecBinary.dst.dataBits;
    memInfo.addr = mstxVecBinary.dst.addr;
    memInfo.blockStride = mstxVecBinary.dstBlockStride;
    memInfo.repeatStride = mstxVecBinary.dstRepeatStride;
    MaskModeProcess(events, event, RecordType::BINARY_OP);
}

static void ParseRecordMstxDataCopy(const KernelRecord &record, std::vector<SanEvent> &events)
{
    auto &mstxRecord = record.payload.mstxRecord;
    auto &mstxDataCopy = mstxRecord.interface.mstxDataCopyDesc;

    KernelRecord equivalence;
    equivalence.recordType = RecordType::DMA_MOV;
    equivalence.blockType = record.blockType;
    equivalence.serialNo = record.serialNo;
    DmaMovRecord &dmaMovRecord = equivalence.payload.dmaMovRecord;
    dmaMovRecord.dst = mstxDataCopy.dst.addr;
    dmaMovRecord.src = mstxDataCopy.src.addr;
    dmaMovRecord.location = mstxRecord.location;
    dmaMovRecord.nBurst = mstxDataCopy.nBurst;
    dmaMovRecord.lenBurst = mstxDataCopy.lenBurst;
    dmaMovRecord.srcStride = mstxDataCopy.srcGap;
    dmaMovRecord.dstStride = mstxDataCopy.dstGap;
    dmaMovRecord.dstMemType = FormatConverter::AddrSpaceToMemType(mstxDataCopy.dst.space);
    dmaMovRecord.srcMemType = FormatConverter::AddrSpaceToMemType(mstxDataCopy.src.space);
    dmaMovRecord.padMode = PadMode::PAD_NONE;
    dmaMovRecord.byteMode = ByteMode::BM_DISABLE;
    ParseRecordDmaMovImpl(equivalence, events, false);
}

static void ParseRecordMstxDataCopyPad(const KernelRecord &record, std::vector<SanEvent> &events)
{
    auto &mstxRecord = record.payload.mstxRecord;
    auto &mstxDataCopyPad = mstxRecord.interface.mstxDataCopyPadDesc;

    KernelRecord equivalence;
    equivalence.recordType = RecordType::MOV_ALIGN;
    equivalence.blockType = record.blockType;
    equivalence.serialNo = record.serialNo;
    MovAlignRecord &movAlignRecord = equivalence.payload.movAlignRecord;
    movAlignRecord.dst = mstxDataCopyPad.dst.addr;
    movAlignRecord.src = mstxDataCopyPad.src.addr;
    movAlignRecord.location = mstxRecord.location;
    movAlignRecord.srcGap = mstxDataCopyPad.srcGap;
    movAlignRecord.dstGap = mstxDataCopyPad.dstGap;
    movAlignRecord.lenBurst = mstxDataCopyPad.lenBurst;
    movAlignRecord.nBurst = mstxDataCopyPad.nBurst;
    movAlignRecord.dstMemType = FormatConverter::AddrSpaceToMemType(mstxDataCopyPad.dst.space);
    movAlignRecord.srcMemType = FormatConverter::AddrSpaceToMemType(mstxDataCopyPad.src.space);
    if (!FormatConverter::GetDataTypeByDataBits(mstxDataCopyPad.dst.dataBits, movAlignRecord.dataType)) {
        SAN_ERROR_LOG("Get data type by data bits failed. dataBits: %d\n",
                      static_cast<int>(mstxDataCopyPad.dst.dataBits));
    }
    movAlignRecord.leftPaddingNum = mstxDataCopyPad.leftPad;
    movAlignRecord.rightPaddingNum = mstxDataCopyPad.rightPad;
    ParseRecordMovAlignImpl(equivalence, events, false);
}

static void ParseRecordMstxStub(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto &mstxRecord = record.payload.mstxRecord;
    auto &crossInfo = event.eventInfo.mstxCrossInfo;
    auto &eventMemInfo = event.eventInfo.memInfo;
    SetLocationInfo(event, mstxRecord, record.blockType, record.serialNo);
    if (mstxRecord.error) {
        SAN_ERROR_LOG("Parse MstxStub failed, serialNo:%lu\n", record.serialNo);
        return;
    }

    if (mstxRecord.interfaceType == InterfaceType::MSTX_HCCL) {
        ParseRecordMstxHCCL(record, events, event);
    }

    if (mstxRecord.interfaceType == InterfaceType::MSTX_SET_CROSS_SYNC
        || mstxRecord.interfaceType == InterfaceType::MSTX_WAIT_CROSS_SYNC) {
        event.type = EventType::MSTX_CROSS_SYNC_EVENT;
        auto &crossCoreRecord = mstxRecord.interface.mstxCrossRecord;
        crossInfo.addr = crossCoreRecord.addr;
        crossInfo.flagId = crossCoreRecord.flagId;
        event.pipe = crossCoreRecord.pipe;
        crossInfo.pipe = crossCoreRecord.pipe;
        crossInfo.isMore = crossCoreRecord.isMore;
        crossInfo.isMerge = crossCoreRecord.isMerge;
        crossInfo.opType = mstxRecord.interfaceType == InterfaceType::MSTX_WAIT_CROSS_SYNC ?
            SyncType::MSTX_WAIT_CROSS : SyncType::MSTX_SET_CROSS;
        events.emplace_back(event);
    }

    if (mstxRecord.interfaceType == InterfaceType::MSTX_HCCLV) {
        eventMemInfo.memType = MemType::GM;
        event.type = EventType::MEM_EVENT;
        auto &hcclCoreRecord = mstxRecord.interface.mstxHcclCoreRecord;
        eventMemInfo.repeatTimes = hcclCoreRecord.repeat;
        eventMemInfo.blockNum = 1;
        eventMemInfo.blockStride = 0;
        eventMemInfo.repeatStride = 1;
        // 读事件
        eventMemInfo.addr = hcclCoreRecord.src;
        eventMemInfo.blockSize = hcclCoreRecord.srclenBurst;
        eventMemInfo.opType =  AccessType::READ;
        events.emplace_back(event);
        // 写事件
        eventMemInfo.addr = hcclCoreRecord.dst;
        eventMemInfo.blockSize = hcclCoreRecord.dstlenBurst;
        eventMemInfo.opType =  AccessType::WRITE;
        events.emplace_back(event);
    }
    if (mstxRecord.interfaceType == InterfaceType::MSTX_CROSS_CORE_BARRIER) {
        ParseMstxCrossCoreBarrier(record, events);
    } else if (mstxRecord.interfaceType == InterfaceType::MSTX_CROSS_CORE_SET_FLAG) {
        ParseMstxCrossCoreSetFlag(record, events);
    } else if (mstxRecord.interfaceType == InterfaceType::MSTX_CROSS_CORE_WAIT_FLAG) {
        ParseMstxCrossCoreWaitFlag(record, events);
    } else if (mstxRecord.interfaceType == InterfaceType::MSTX_SIGNAL_SET) {
        ParseMstxSignalSet(record, events);
    } else if (mstxRecord.interfaceType == InterfaceType::MSTX_SIGNAL_WAIT) {
        ParseMstxSignalWait(record, events);
    } else if (mstxRecord.interfaceType == InterfaceType::MSTX_VEC_UNARY_OP) {
        ParseRecordMstxVecUnary(record, events);
    } else if (mstxRecord.interfaceType == InterfaceType::MSTX_VEC_BINARY_OP) {
        ParseRecordMstxVecBinary(record, events);
    } else if (mstxRecord.interfaceType == InterfaceType::MSTX_DATA_COPY) {
        ParseRecordMstxDataCopy(record, events);
    } else if (mstxRecord.interfaceType == InterfaceType::MSTX_DATA_COPY_PAD) {
        ParseRecordMstxDataCopyPad(record, events);
    }
}

static void ParseRecordFinish(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    event.isEndFrame = true;
    events.emplace_back(event);
}

static void ParseRecordSetL12D(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& setL12DRecord = record.payload.setL12DRecord;
    PipeType pipe = PipeType::PIPE_MTE2;

    SetLocationInfo(event, setL12DRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = pipe;
    memInfo.memType = MemType::L1;
    memInfo.opType = AccessType::WRITE;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.addr = setL12DRecord.dst;
    AlignChecker::Instance().CheckAlign(event, 32U);
    
    memInfo.blockSize = 32U;
    memInfo.blockNum = setL12DRecord.dstBlockNum;
    memInfo.blockStride = 1;
    memInfo.repeatStride = setL12DRecord.repeatGap;
    memInfo.repeatTimes = setL12DRecord.repeat;
    events.emplace_back(event);
}

static void AddMemEventForMovL1Ub(std::vector<SanEvent> &events, const KernelRecord &record, Sanitizer::RecordType recordMemType)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& movL1UbRecord = record.payload.movL1UbRecord;
    PipeType pipe = FormatConverter::QueryPipeType(movL1UbRecord.srcMemType, movL1UbRecord.dstMemType);
    if (pipe == PipeType::SIZE) {
        SAN_ERROR_LOG("serialNo: %lu parse failed, pipe has exceeded the maximum limit", record.serialNo);
        return;
    }
    SetLocationInfo(event, movL1UbRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = pipe;
    if (movL1UbRecord.lenBurst == 0) {
        SAN_ERROR_LOG("parse failed, lenBurst is 0.");
        return;
    }
    memInfo.memType = movL1UbRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.addr = movL1UbRecord.src;
    memInfo.blockNum = movL1UbRecord.lenBurst;
    memInfo.blockSize = MOV_LOCAL_BLOCK_SIZE;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = movL1UbRecord.nBurst;
    memInfo.repeatStride = movL1UbRecord.lenBurst + movL1UbRecord.srcGap;
    AlignChecker::Instance().CheckAlign(event, recordMemType);
    events.emplace_back(event);
 
    memInfo.memType = movL1UbRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = movL1UbRecord.dst;
    memInfo.repeatStride = movL1UbRecord.lenBurst + movL1UbRecord.dstGap;
    AlignChecker::Instance().CheckAlign(event, recordMemType);
    events.emplace_back(event);
}

static void ParseRecordMovL2Ub(const KernelRecord &record, std::vector<SanEvent> &events)
{
    AddMemEventForMovL1Ub(events, record, RecordType::MOV_L1_TO_UB);
}

static void ParseRecordMovUb2L1(const KernelRecord &record, std::vector<SanEvent> &events)
{
    AddMemEventForMovL1Ub(events, record, RecordType::MOV_UB_TO_L1);
}

static void ParseRecordMovUb2Ub(const KernelRecord &record, std::vector<SanEvent> &events)
{
    AddMemEventForMovL1Ub(events, record, RecordType::MOV_UB_TO_UB);
}

static bool CheckMovCbuf2BtParam(const MovL1BtRecord &record, uint64_t serialNo)
{
    if (record.dataType == DetailedDataType::FLOAT || record.dataType == DetailedDataType::S32) {
        if (record.lenBurst % 2 != 0) {
            SAN_ERROR_LOG("serialNo: %lu parse failed, lenBurst: %u must be even", serialNo, record.lenBurst);
            return false;
        }
        if (record.cvtEnable != 0) {
            SAN_ERROR_LOG("serialNo: %lu parse failed, cvtEnable: %u must be zero", serialNo, record.cvtEnable);
            return false;
        }
    }
    if (record.dstGap % 2 != 0) {
        SAN_ERROR_LOG("serialNo: %lu parse failed, dstGap:%u must be even", serialNo, record.dstGap);
        return false;
    }
    return true;
}

static void ParseRecordMovCbuf2Bt(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& movL1BtRecord = record.payload.movL1BtRecord;
    if (!CheckMovCbuf2BtParam(movL1BtRecord, record.serialNo)) { return; }
    PipeType pipe = FormatConverter::QueryPipeType(MemType::L1, MemType::BT);
    if (pipe == PipeType::SIZE) {
        SAN_ERROR_LOG("serialNo: %lu parse failed, pipe has exceeded the maximum limit", record.serialNo);
        return;
    }
    SetLocationInfo(event, movL1BtRecord, record.blockType, record.serialNo);
    auto& memInfo = event.eventInfo.memInfo;
    event.type = EventType::MEM_EVENT;
    event.pipe = pipe;
    memInfo.memType = MemType::L1;
    memInfo.opType = AccessType::READ;
    memInfo.addr = movL1BtRecord.src;
    memInfo.blockNum = movL1BtRecord.lenBurst;
    memInfo.blockSize = MOV_LOCAL_BLOCK_SIZE;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = movL1BtRecord.nBurst;
    memInfo.repeatStride = movL1BtRecord.lenBurst + movL1BtRecord.srcGap;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);

    if (movL1BtRecord.dataType == DetailedDataType::F16 || movL1BtRecord.dataType == DetailedDataType::B16) {
        memInfo.blockNum = movL1BtRecord.lenBurst * 2;
        memInfo.repeatStride = movL1BtRecord.lenBurst * 2 + movL1BtRecord.dstGap;
    } else {
        memInfo.blockNum = movL1BtRecord.lenBurst;
        memInfo.repeatStride = movL1BtRecord.lenBurst + movL1BtRecord.dstGap;
    }
    memInfo.memType = MemType::BT;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = movL1BtRecord.dst;
    const uint16_t dstAlignSize = 64;
    AlignChecker::Instance().CheckAlign(event, dstAlignSize);
    events.emplace_back(event);
}

static bool CheckMovCbuf2FbParam(const MovL1FbRecord &record, uint64_t serialNo)
{
    if (record.dstMemBlock == 0) {
        if (record.lenBurst % 2 != 0 || record.dstStride % 2 != 0) {
            SAN_ERROR_LOG("serialNo: %lu parse failed, lenBurst :%u or dstStride :%u must be multiple of 2",
                serialNo, record.lenBurst, record.dstStride);
            return false;
        }
    } else if (record.dstMemBlock >= 2) {
        SAN_ERROR_LOG("serialNo: %lu parse failed, dstMemBlock: %u must be less than 2", serialNo, record.dstMemBlock);
        return false;
    }
    return true;
}

static void ParseRecordMovCbuf2Fb(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& movL1FbRecord = record.payload.movL1FbRecord;
    if (!CheckMovCbuf2FbParam(movL1FbRecord, record.serialNo)) { return; }
    PipeType pipe = FormatConverter::QueryPipeType(MemType::L1, MemType::FB);
    if (pipe == PipeType::SIZE) {
        SAN_ERROR_LOG("serialNo: %lu parse failed, pipe has exceeded the maximum limit", record.serialNo);
        return;
    }
    SetLocationInfo(event, movL1FbRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = pipe;
    auto& memInfo = event.eventInfo.memInfo;
    memInfo.memType = MemType::L1;
    memInfo.opType = AccessType::READ;
    const uint16_t lenBurstUnit = 64;
    const uint16_t srcStrideUnit = 32;
    memInfo.blockNum = 1;
    memInfo.repeatTimes = 1;
    memInfo.addr = movL1FbRecord.src;
    memInfo.blockSize =  movL1FbRecord.lenBurst * lenBurstUnit;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    for (size_t i = 0; i < movL1FbRecord.nBurst; ++i) {
        memInfo.addr = movL1FbRecord.src + i * srcStrideUnit * movL1FbRecord.srcStride;
        events.emplace_back(event);
    }

    uint16_t dstAlignSize = movL1FbRecord.dstMemBlock == 0 ? 128 : 64;
    memInfo.memType = MemType::FB;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = movL1FbRecord.dst;
    memInfo.blockNum = movL1FbRecord.lenBurst;
    memInfo.blockSize = lenBurstUnit;
    memInfo.blockStride = 1;
    memInfo.repeatTimes = movL1FbRecord.nBurst;
    memInfo.repeatStride = movL1FbRecord.dstStride;
    AlignChecker::Instance().CheckAlign(event, dstAlignSize);
    events.emplace_back(event);
}

static void ParseVbs32A5(const KernelRecord &record, std::vector<SanEvent> &events)
{
    auto& vbs32Record = record.payload.vbs32Record;
    if (vbs32Record.repeat == 0) {
        SAN_INFO_LOG("serialNo: %lu, repeat time is 0, no execution", record.serialNo);
        return;
    }
    SanEvent event{};
    auto& memInfo = event.eventInfo.memInfo;
    SetLocationInfo(event, vbs32Record, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    memInfo.memType = MemType::UB;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.repeatTimes = vbs32Record.repeat;
    memInfo.repeatStride = 1;
    memInfo.blockNum = 1;
    memInfo.blockStride = 0;
    constexpr uint32_t DATA_COUNT = 32; // 每次操作32个数字
    // read score
    memInfo.addr = vbs32Record.src0;
    memInfo.blockSize = FormatConverter::GetDataSizeByType(vbs32Record.dataType) * DATA_COUNT;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
    // read index
    memInfo.addr = vbs32Record.src1;
    memInfo.blockSize = 128;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
    // write dst, each element 8B
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = vbs32Record.dst;
    memInfo.blockSize = 256;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
    events.emplace_back(event);
}

static void ParseRecordShadowMemory(const KernelRecord &record, std::vector<SanEvent> &events)
{
    auto& smRecord = record.payload.shadowMemoryRecord;
    SanEvent event{};
    auto& memInfo = event.eventInfo.memInfo;
    SetLocationInfo(event, smRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    memInfo.memType = FormatConverter::AddrSpaceToMemType(smRecord.space);
    memInfo.opType = smRecord.opType == MemOpType::LOAD ? AccessType::READ : AccessType::WRITE;
    memInfo.repeatTimes = 1;
    memInfo.blockNum = 1;
    memInfo.addr = smRecord.addr;
    memInfo.blockSize = smRecord.size;
    memInfo.ignoreIllegalCheck = true;
    events.emplace_back(event);
}

static void ParseRecordRegister(const KernelRecord &record, std::vector<SanEvent> &events, RegisterType regType)
{
    SanEvent event;
    SetLocationInfo(event, record.payload.registerSetRecord, record.blockType, record.serialNo);
    event.type = EventType::REGISTER_EVENT;
    event.eventInfo.regInfo.regType = regType;
    event.eventInfo.regInfo.regPayLoad = record.payload.registerSetRecord.regPayLoad;
    
    events.emplace_back(event);
}

static void ParseRecordRegisterVectorMask0(const KernelRecord &record, std::vector<SanEvent> &events)
{
    ParseRecordRegister(record, events, RegisterType::VECTOR_MASK_0);
}

static void ParseRecordRegisterVectorMask1(const KernelRecord &record, std::vector<SanEvent> &events)
{
    ParseRecordRegister(record, events, RegisterType::VECTOR_MASK_1);
}

static void ParseRecordRegisterCtrl(const KernelRecord &record, std::vector<SanEvent> &events)
{
    ParseRecordRegister(record, events, RegisterType::CTRL);
}

static void ParseRecordRegisterFftsBaseAddr(const KernelRecord &record, std::vector<SanEvent> &events)
{
    ParseRecordRegister(record, events, RegisterType::FFTS_BASE_ADDR);
}

static void ParseRecordRegisterFpc(const KernelRecord &record, std::vector<SanEvent> &events)
{
    ParseRecordRegister(record, events, RegisterType::FPC);
}

static void ParseRecordRegisterQuantPre(const KernelRecord &record, std::vector<SanEvent> &events)
{
    ParseRecordRegister(record, events, RegisterType::QUANT_PRE);
}

static void ParseRecordRegisterQuantPost(const KernelRecord &record, std::vector<SanEvent> &events)
{
    ParseRecordRegister(record, events, RegisterType::QUANT_POST);
}

static void ParseRecordRegisterLreluAlpha(const KernelRecord &record, std::vector<SanEvent> &events)
{
    ParseRecordRegister(record, events, RegisterType::LRELU_ALPHA);
}

using ParseFunc = std::function<void (const KernelRecord &record, std::vector<SanEvent> &events)>;
const std::unordered_map<RecordType, ParseFunc> g_parseFuncs = {
    {RecordType::LOAD, ParseScalarOpRecord},
    {RecordType::LD, ParseScalarOpRecord},
    {RecordType::LD_IO, ParseScalarOpRecord},
    {RecordType::STORE, ParseScalarOpRecord},
    {RecordType::ST, ParseScalarOpRecord},
    {RecordType::ST_IO, ParseScalarOpRecord},
    {RecordType::STI_IO, ParseScalarOpRecord},
    {RecordType::STP, ParseScalarOpRecord},
    {RecordType::STI, ParseScalarOpRecord},
    {RecordType::LDP, ParseScalarOpRecord},
    {RecordType::ST_ATOMIC, ParseScalarOpRecord},
    {RecordType::STI_ATOMIC, ParseScalarOpRecord},
    {RecordType::ST_DEV, ParseScalarOpRecord},
    {RecordType::LD_DEV, ParseScalarOpRecord},
    {RecordType::DMA_MOV, ParseRecordDmaMov},
    {RecordType::DMA_MOV_CONV_RELU, ParseRecordDmaMovConvRelu},
    {RecordType::DMA_MOV_DEPTH_WISE, ParseRecordDmaMovDepthWise},
    {RecordType::DMA_MOV_ND2NZ, ParseRecordDmaMovNd2nz},
    {RecordType::DMA_MOV_ND2NZ_D, ParseRecordDmaMovNd2NzDav},
    {RecordType::DMA_MOV_DN2NZ_D, ParseRecordDmaMovDn2nzDav},
    {RecordType::MOV_ALIGN, ParseRecordMovAlign},
    {RecordType::MOV_ALIGN_V2, ParseRecordMovAlignV2},
    {RecordType::MOV_BT, ParseRecordMovBt},
    {RecordType::MOV_FP, ParseRecordMovFp},
    {RecordType::FIX_L0C_TO_L1, ParseRecordFixL0CToL1},
    {RecordType::FIX_L0C_TO_UB, ParseRecordFixL0CToUB},
    {RecordType::MOV_L1_TO_UB, ParseRecordMovL2Ub},
    {RecordType::ND_DMA_OUT_TO_UB, ParseRecordNdDmaMovOut2Ub},
    {RecordType::VEC_DUP, ParseVecdupRecord},
    {RecordType::LOAD_2D, ParseRecordLoad2D},
    {RecordType::LOAD_L1_2D, ParseRecordLoadL12D},
    {RecordType::LOAD_L1_MX_2D, ParseRecordLoadL1Mx2D},
    {RecordType::LOAD_L1_2D_TRANSPOSE, ParseRecordLoadL12DTranspose},
    {RecordType::LOAD_2D_SPARSE, ParseRecordLoad2DSparse},
    {RecordType::LOAD_2D_TRANSPOSE, ParseRecordLoad2DTranspose},
    {RecordType::DECOMPRESS_HEADER, ParseRecordDecompressHeader},
    {RecordType::BROADCAST, ParseRecordBroadcast},
    {RecordType::DC_PRELOAD, ParseRecordDcPreload},
    {RecordType::SCATTERVNCHWCONV, ParseRecordScatterVnchwconv},
    {RecordType::SCATTERVNCHWCONV_A5, ParseRecordScatterVnchwconv},
    {RecordType::LOAD_3D, ParseRecordLoad3D},
    {RecordType::LOAD_3D_V2, ParseRecordLoad3DV2},
    {RecordType::LOAD_B2, ParseRecordLoadB2},
    {RecordType::LOAD_A_WINOGRAD, ParseRecordLoadAWinograd},
    {RecordType::LOAD_B_WINOGRAD, ParseRecordLoadBWinograd},
    {RecordType::SET_2D, ParseRecordSet2D},
    {RecordType::LOAD_IMAGE, ParseRecordLoadImage},
    {RecordType::LOAD_SMASK, ParseRecordLoadSmask},
    {RecordType::UNARY_OP, ParseRecordUnaryOp},
    {RecordType::VGATHER, ParseRecordVgather},
    {RecordType::ELEMENT, ParseRecordElement},
    {RecordType::VCOPY_OP, ParseRecordUnaryOp},
    {RecordType::VREDUCEV2_UNARY, ParseRecordUnaryOp},
    {RecordType::VREDUCEV2_BINARY, ParseRecordBinaryOp},
    {RecordType::VMRGSORT4_OP_C220, ParseRecordUnaryOp},
    {RecordType::VMRGSORT4_OP_M200, ParseRecordUnaryOp},
    {RecordType::VMRGSORT4_OP_C310, ParseRecordVms4v2A5},
    {RecordType::VBS32_A5, ParseVbs32A5},
    {RecordType::VCONV_DST_S4_OP, ParseRecordUnaryOp},
    {RecordType::VCONV_SRC_S4_OP, ParseRecordUnaryOp},
    {RecordType::BINARY_OP, ParseRecordBinaryOp},
    {RecordType::VSEL_OP, ParseRecordVsel},
    {RecordType::TERNARY_OP, ParseRecordBinaryOp},
    {RecordType::REDUCE_OP, ParseRecordReduceOp},
    {RecordType::VREDUCEV2, ParseRecordReduceV2},
    {RecordType::CMPMASK_OP, ParseRecordCmpMask},
    {RecordType::MATRIX_MUL_OP, ParseMatrixMulOpRecord},
    {RecordType::MMAD_A5, ParseMmadA5Record},
    {RecordType::VEC_REGPROPCOOR_OP, ParseVRegPropCoorOPRecord},
    {RecordType::SET_FLAG, ParseRecordSetFlag},
    {RecordType::WAIT_FLAG, ParseRecordWaitFlag},
    {RecordType::SET_FLAGI, ParseRecordSetFlag},
    {RecordType::WAIT_FLAGI, ParseRecordWaitFlag},
    {RecordType::SET_FLAG_V, ParseRecordSetFlag},
    {RecordType::WAIT_FLAG_V, ParseRecordWaitFlag},
    {RecordType::SET_FLAGI_V, ParseRecordSetFlag},
    {RecordType::WAIT_FLAGI_V, ParseRecordWaitFlag},
    {RecordType::GET_BUF, ParseRecordGetBuf},
    {RecordType::GET_BUFI, ParseRecordGetBuf},
    {RecordType::RLS_BUF, ParseRecordRlsBuf},
    {RecordType::RLS_BUFI, ParseRecordRlsBuf},
    {RecordType::GET_BUF_V, ParseRecordGetBuf},
    {RecordType::GET_BUFI_V, ParseRecordGetBuf},
    {RecordType::RLS_BUF_V, ParseRecordRlsBuf},
    {RecordType::RLS_BUFI_V, ParseRecordRlsBuf},
    {RecordType::FFTS_SYNC, ParseFftsSyncRecord},
    {RecordType::WAIT_FLAG_DEV, ParseWaitDevRecord},
    {RecordType::WAIT_FLAG_DEV_PIPE, ParseWaitDevPipeRecord},
    {RecordType::WAIT_FLAG_DEVI_PIPE, ParseWaitDevPipeRecord},
    {RecordType::FFTS_SYNC_V, ParseFftsSyncRecord},
    {RecordType::WAIT_FLAG_DEV_PIPE_V, ParseWaitDevPipeRecord},
    {RecordType::WAIT_FLAG_DEVI_PIPE_V, ParseWaitDevPipeRecord},
    {RecordType::SET_INTRA_BLOCK, ParseSetIntraBlockRecord},
    {RecordType::SET_INTRA_BLOCKI, ParseSetIntraBlockRecord},
    {RecordType::SET_INTRA_BLOCK_V, ParseSetIntraBlockRecord},
    {RecordType::SET_INTRA_BLOCKI_V, ParseSetIntraBlockRecord},
    {RecordType::WAIT_INTRA_BLOCK, ParseWaitIntraBlockRecord},
    {RecordType::WAIT_INTRA_BLOCKI, ParseWaitIntraBlockRecord},
    {RecordType::WAIT_INTRA_BLOCK_V, ParseWaitIntraBlockRecord},
    {RecordType::WAIT_INTRA_BLOCKI_V, ParseWaitIntraBlockRecord},
    {RecordType::IB_SET_STUB, ParseRecordIBSetStub},
    {RecordType::IB_WAIT_STUB, ParseRecordIBWaitStub},
    {RecordType::SYNC_ALL_STUB, ParseRecordSyncAllStub},
    {RecordType::MSTX_STUB, ParseRecordMstxStub},
    {RecordType::HSET_FLAG, ParseRecordHsetFlag},
    {RecordType::HWAIT_FLAG, ParseRecordHwaitFlag},
    {RecordType::HSET_FLAGI, ParseRecordHsetFlag},
    {RecordType::HWAIT_FLAGI, ParseRecordHwaitFlag},
    {RecordType::PIPE_BARRIER, ParseRecordPipeBarrier},
    {RecordType::SET_ATOMIC, ParseRecordSetAtomic},
    {RecordType::SCALAR_RED, ParseRedAndAtomRecord},
    {RecordType::SCALAR_ATOM, ParseRedAndAtomRecord},
    {RecordType::LDVA, ParseLdvaRecord},
    {RecordType::FINISH, ParseRecordFinish},
    {RecordType::SET_L1_2D, ParseRecordSetL12D},
    {RecordType::MOV_UB_TO_L1, ParseRecordMovUb2L1},
    {RecordType::MOV_UB_TO_UB, ParseRecordMovUb2Ub},
    {RecordType::MOV_CBUF_TO_BT, ParseRecordMovCbuf2Bt},
    {RecordType::MOV_CBUF_TO_FB, ParseRecordMovCbuf2Fb},
    {RecordType::SHADOW_MEMORY, ParseRecordShadowMemory},
    {RecordType::SET_VECTOR_MASK_0, ParseRecordRegisterVectorMask0},
    {RecordType::SET_VECTOR_MASK_1, ParseRecordRegisterVectorMask1},
    {RecordType::SET_CTRL, ParseRecordRegisterCtrl},
    {RecordType::SET_FFTS_BASE_ADDR, ParseRecordRegisterFftsBaseAddr},
    {RecordType::SET_FPC, ParseRecordRegisterFpc},
    {RecordType::SET_QUANT_PRE, ParseRecordRegisterQuantPre},
    {RecordType::SET_QUANT_POST, ParseRecordRegisterQuantPost},
    {RecordType::SET_LRELU_ALPHA, ParseRecordRegisterLreluAlpha},
};

void RecordParse::DfsSrcGraph(PipeType targetPipe, std::unordered_set<PipeType> &visited)
{
    visited.insert(targetPipe);
    // 查找当前节点的邻接节点
    auto it = dstSrcGraph_.find(targetPipe);
    if (it != dstSrcGraph_.end()) {
        for (PipeType neighbor : it->second) {
            if (visited.find(neighbor) == visited.end()) {
                // 递归调用 DFS 访问未访问过的邻接节点
                DfsSrcGraph(neighbor, visited);
            }
        }
    }
}

/// 流水内两个相邻指令A和C之间，插入2对set_flag/wait_flag(hset_flag/hwait_flag)，具有流水内同步的效果：
/// +-------------------------------------------------------------------+
/// |  ___pipe1:A___ |set_1_2                  wait_2_1| ___pipe1:C___  |
/// |                 wait_1_2| ___pipe2:B___ |set_2_1                  |
/// +-------------------------------------------------------------------+
/// |  ___pipe1:A___ |hwait_1_2                hset_1_2| ___pipe1:C___  |
/// |                 hset_2_1| ___pipe2:B___ |hwait_2_1                |
/// +-------------------------------------------------------------------+
/// 只插入一对则没有流水内同步效果，以set_flag/wait_flag为例(.-->和<--.表示竞争关系)：
/// +------------------------------------------+------------------------------------------+
/// |         .--> ___pipe1:C___               |               ___pipe1:C___ <--.         |
/// |  ___pipe1:A___ |set_1_2                  |                 wait_2_1| ___pipe1:A___  |
/// |                 wait_1_2| ___pipe2:B___  |  ___pipe2:B___ |set_2_1                  |
/// +------------------------------------------+------------------------------------------+
/// 常规的set/wait顺序：set(pipe1, pipe2)->wait(pipe1, pipe2)->set(pipe2, pipe1)->wait(pipe2, pipe1)
/// 常规的hset/hwait顺序：hwait(pipe1, pipe2)->hset(pipe2, pipe1)->hwait(pipe2, pipe1)->hset(pipe1, pipe2)
/// 有时用户会调整顺序，如：hset(pipe2, pipe1)->hwait(pipe1, pipe2)->hset(pipe1, pipe2)->hwait(pipe2, pipe1)
/// 因此不适合用全量4条指令作为判断依据，而是取关键的wait指令判断状态迁移：
/// +---+   (h)wait_1_2    +---+   (h)wait_2_1    +------------------+
/// | F |  ------------->  | T |  ------------->  | pipe_barrier & F |
/// +---+                  +---+                  +------------------+
/// 使用数组记录状态值，用boolean表示
///    PIPE_S, PIPE_V, PIPE_M, PIPE_MTE1, PIPE_MTE2, PIPE_MTE3...
/// : [   F  ,    F  ,    F  ,     T    ,     F    ,     F    ...]
/// 按顺序集齐一对wait指令就插入一个pipe_barrier，并将状态值归零；遇到内存指令就结束判断，将对应pipe的状态值归零
void RecordParse::UpdateSyncInPipe(KernelRecord const& record, std::vector<SanEvent> &events)
{
    if (record.recordType == RecordType::BLOCK_FINISH || record.recordType == RecordType::FINISH) {
        ResetSyncInPipeInfo();
        return;
    }

    if (events.empty()) {
        return;
    }

    SanEvent event = events.back();
    // 遇到内存指令，重置状态
    if (event.type == EventType::MEM_EVENT) {
        setWaitStat_[static_cast<uint8_t>(event.pipe)] = false;
        for (size_t i = 0; i < static_cast<uint8_t>(PipeType::SIZE); ++i) {
            dstSrcGraph_[static_cast<PipeType>(i)].erase(event.pipe);
        }
        return;
    }
    // 只使用sync_event(h)的wait_flag更新状态值
    if (event.type != EventType::SYNC_EVENT || event.eventInfo.syncInfo.opType != SyncType::WAIT_FLAG) {
        return;
    }
    // 校验数组的两个维度是否超出上限，避免数组越界
    uint8_t srcPipe = static_cast<uint8_t>(event.eventInfo.syncInfo.srcPipe);
    uint8_t dstPipe = static_cast<uint8_t>(event.eventInfo.syncInfo.dstPipe);
    uint32_t latestEventId = event.eventInfo.syncInfo.eventId;
    if (srcPipe >= static_cast<uint8_t>(PipeType::SIZE) || dstPipe >= static_cast<uint8_t>(PipeType::SIZE)) {
        SAN_ERROR_LOG("The pipe exceeds the maximum limit, src pipe:%u dst pipe:%u.", srcPipe, dstPipe);
        return;
    }
    if (!event.eventInfo.syncInfo.isGenerated && latestEventId >= static_cast<uint32_t>(EventID::VALID_EVENT_ID_SIZE)) {
        SAN_ERROR_LOG("The eventId exceeds the maximum limit, eventId:%u.", latestEventId);
        return;
    }
    // 根据srcPipe/dstPipe更新数组状态，发现有pipe成环就插入pipe_barrier
    dstSrcGraph_[event.eventInfo.syncInfo.dstPipe].insert(event.eventInfo.syncInfo.srcPipe);
    if (!setWaitStat_[srcPipe]) {
        setWaitStat_[srcPipe] = true;
    }

    /// 如果存在pipe_s等待pipe_other的流水间同步，则当前结点之后相当于对pipe_other有流水内同步作用
    if (dstPipe == static_cast<uint8_t>(PipeType::PIPE_S)) {
        std::unordered_set<PipeType> visited;
        DfsSrcGraph(event.eventInfo.syncInfo.srcPipe, visited);
        for (const auto &visitPipe : visited) {
            events.emplace_back(CreateInnerPipeSyncEvent(record, visitPipe, record.payload.syncRecord));
        }
    }

    if (setWaitStat_[dstPipe]) {
        events.emplace_back(CreateInnerPipeSyncEvent(record, event.eventInfo.syncInfo.dstPipe,
                                                     record.payload.syncRecord));
        setWaitStat_[dstPipe] = false;
    }
}

thread_local std::array<bool, static_cast<uint8_t>(PipeType::SIZE)> RecordParse::setWaitStat_ = {};
thread_local RecordParse::DstSrcGraph RecordParse::dstSrcGraph_ = {};
thread_local std::map<HsetRecordKey, HsetRecordState> RecordParse::hsetSyncMap_ = {};
thread_local std::unordered_map<uint64_t, uint64_t> RecordParse::getRlsBufMap_ = {};

void RecordParse::ResetSyncInPipeInfo()
{
    setWaitStat_ = {};
    dstSrcGraph_ = {};
}

void RecordParse::ResetAll()
{
    setWaitStat_ = {};
    dstSrcGraph_ = {};
    hsetSyncMap_ = {};
    getRlsBufMap_ = {};
}

SanEvent HsetWaitCreateSyncEvent(const SyncType syncType, const PipeType pipeSrc, const PipeType pipeDst,
    const SanEvent &event, uint64_t eventId)
{
    // 一般EVENT_ID取值0~7，这里是为了与正常set_flag/wait_flag区分开
    constexpr uint32_t hsetStartEventID = 120U;
    SanEvent dstEvent;
    dstEvent.serialNo = event.serialNo;
    dstEvent.loc = event.loc;
    dstEvent.type = EventType::SYNC_EVENT;
    dstEvent.pipe = syncType == SyncType::SET_FLAG ? pipeSrc : pipeDst;
    dstEvent.eventInfo.syncInfo.opType = syncType;
    dstEvent.eventInfo.syncInfo.srcPipe = pipeSrc;
    dstEvent.eventInfo.syncInfo.dstPipe = pipeDst;
    dstEvent.eventInfo.syncInfo.eventId = hsetStartEventID + eventId;
    dstEvent.eventInfo.syncInfo.memType = MemType::INVALID;
    dstEvent.eventInfo.syncInfo.isRetrogress = true;
    dstEvent.eventInfo.syncInfo.isGenerated = true;
    return dstEvent;
}

void RecordParse::ProcessHsetWaitSync(std::vector<SanEvent> &events)
{
    for (auto &event : events) {
        if (event.type != EventType::H_SYNC_EVENT) { continue; }
        auto &hsyncInfo = event.eventInfo.hsyncInfo;
        HsetRecordKey key = {hsyncInfo.srcPipe, hsyncInfo.dstPipe, hsyncInfo.eventId, hsyncInfo.memType};
        if (hsyncInfo.opType == SyncType::HSET_FLAG && !hsyncInfo.isReplaced) {
            auto &states = hsetSyncMap_[key];
            if (states.isTransSet) { continue; }
            if (hsyncInfo.v == 0) {
                if (states.cacheMode1) {
                    SAN_ERROR_LOG("The hset parameter is invalid, "
                        "Setting mode 0 and mode 1 at the same time is not allowed, serialNo:%lu", event.serialNo);
                }
                states.cacheMode0 = true;
            } else if (hsyncInfo.v == 1) {
                if (states.cacheMode0) {
                    SAN_ERROR_LOG("The hset parameter is invalid. "
                        "Setting mode 0 and mode 1 at the same time is not allowed, serialNo:%lu", event.serialNo);
                }
                states.cacheMode1 = true;
            }
            hsyncInfo.isReplaced = true;
        } else if (hsyncInfo.opType == SyncType::HWAIT_FLAG && !hsyncInfo.isReplaced) {
            auto it = hsetSyncMap_.find(key);
            // 如果有前置hset并且hset已经被替换为set指令，才会启动替换hwait的逻辑
            if (it != hsetSyncMap_.cend() && it->second.isTransSet) {
                auto genEvent = HsetWaitCreateSyncEvent(SyncType::WAIT_FLAG, hsyncInfo.srcPipe, hsyncInfo.dstPipe,
                    events[events.size() - 1], hsyncInfo.eventId);
                events.push_back(genEvent);
            }
            hsetSyncMap_.erase(key);
            hsyncInfo.isReplaced = true;
        }
    }

    for (auto &pair : hsetSyncMap_) {
        auto &key = pair.first;
        auto &val = pair.second;
        if (val.isTransSet || !val.cacheMode0) { continue; }
        for (auto const &event : events) {
            // 查找内存事件中是否有和缓存的hset对应的事件
            if (event.type == EventType::MEM_EVENT && event.pipe == key.srcPipe &&
                event.eventInfo.memInfo.memType == key.memType) {
                // 如果存在和hset对应的写入事件，则事件末尾插入一条SET_FLAG事件并清理对应的hsetSyncMap_状态
                auto genEvent = HsetWaitCreateSyncEvent(SyncType::SET_FLAG, key.srcPipe, key.dstPipe,
                    events[events.size() - 1], key.eventId);
                events.push_back(genEvent);
                val.isTransSet = true;
                return;
            }
        }
    }
}

void ReplacePipeScalar(SanEvent &event)
{
    switch (event.type) {
        case EventType::SYNC_EVENT: {
            if (event.eventInfo.syncInfo.opType == SyncType::SET_FLAG) {
                if (event.eventInfo.syncInfo.srcPipe == PipeType::PIPE_S) {
                    event.pipe = PipeType::PIPE_S_CAL;
                    event.eventInfo.syncInfo.srcPipe = PipeType::PIPE_S_CAL;
                }
            } else if (event.eventInfo.syncInfo.opType == SyncType::WAIT_FLAG) {
                if (event.eventInfo.syncInfo.srcPipe == PipeType::PIPE_S) {
                    event.eventInfo.syncInfo.srcPipe = PipeType::PIPE_S_CAL;
                }
            }
            return;
        }
        case EventType::CROSS_CORE_SYNC_EVENT: {
            if (event.eventInfo.fftsSyncInfo.opType == SyncType::FFTS_SYNC) {
                if (event.eventInfo.fftsSyncInfo.dstPipe == PipeType::PIPE_S) {
                    event.pipe = PipeType::PIPE_S_CAL;
                    event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S_CAL;
                }
            }
            return;
        }
        case EventType::CROSS_CORE_SOFT_SYNC_EVENT: {
            if (event.eventInfo.softSyncInfo.opType == SyncType::IB_SET) {
                if (event.pipe == PipeType::PIPE_S) {
                    event.pipe = PipeType::PIPE_S_CAL;
                }
            }
            return;
        }
        case EventType::MSTX_CROSS_SYNC_EVENT: {
            if (event.eventInfo.mstxCrossInfo.opType == SyncType::MSTX_SET_CROSS) {
                if (event.eventInfo.mstxCrossInfo.pipe == PipeType::PIPE_S) {
                    event.pipe = PipeType::PIPE_S_CAL;
                    event.eventInfo.mstxCrossInfo.pipe = PipeType::PIPE_S_CAL;
                }
            }
            return;
        }
        case EventType::BUF_SYNC_EVENT: {
            if (event.eventInfo.bufSyncInfo.opType == SyncType::RLS_BUF) {
                if (event.eventInfo.syncInfo.srcPipe == PipeType::PIPE_S) {
                    event.pipe = PipeType::PIPE_S_CAL;
                    event.eventInfo.bufSyncInfo.pipe = PipeType::PIPE_S_CAL;
                }
            }
            return;
        }
        default:
            return;
    }
}

void RecordParse::ReplaceSetSyncPipeScalar(std::vector<SanEvent> &events)
{
    for (auto &event : events) {
        ReplacePipeScalar(event);
    }
}

void RecordParse::Parse(const SanitizerRecord &record, std::vector<SanEvent> &events)
{
    KernelRecord const& kernelRecord = record.payload.kernelRecord;
    if (kernelRecord.recordType == RecordType::BLOCK_FINISH || kernelRecord.recordType == RecordType::FINISH) {
        ResetAll();
    }
    auto it = g_parseFuncs.find(kernelRecord.recordType);
    if (it == g_parseFuncs.end()) {
        return;
    }

    it->second(kernelRecord, events);
    if (events.size() == 0) { return; }
    ProcessHsetWaitSync(events);
    UpdateSyncInPipe(kernelRecord, events);
    ReplaceSetSyncPipeScalar(events);
}
}
