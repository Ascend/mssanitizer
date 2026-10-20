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


#include "constant.h"
#include "record_parse.h"
#include "format_converter.h"
#include "address_sanitizer/align_checker.h"
#include "record_parse_calc.h"

namespace Sanitizer {

inline uint64_t ParseUbAddrFromVaRegister(VaRegister const &vaRegister, uint8_t elemIdx)
{
    uint64_t reg = elemIdx > 3 ? vaRegister.h64 : vaRegister.l64;
    uint64_t maskShift = elemIdx > 3 ? (elemIdx - 4) * 16 : elemIdx * 16;
    return ((reg >> maskShift) & 0xFFFFUL) << 5UL;
}

inline void ParseVaRegisterEvents(VaRegister const &vaRegister,
                                  bool highHalf,
                                  SanEvent &event,
                                  std::vector<SanEvent> &events)
{
    constexpr uint8_t vaRegisterNum = 8;
    for (uint8_t elemIdx = 0; elemIdx < vaRegisterNum; ++elemIdx) {
        event.eventInfo.memInfo.addr = ParseUbAddrFromVaRegister(vaRegister, elemIdx);
        if (highHalf) {
            event.eventInfo.memInfo.addr += 16UL;
        }
        events.emplace_back(event);
    }
}

void ParseRecordScatterVnchwconv(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& scatterVnchwconvecord = record.payload.scatterVnchwconvRecord;
    SetLocationInfo(event, scatterVnchwconvecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    memInfo.memType = MemType::UB;
    memInfo.opType = AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) }; // mask 不生效在该指令
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.blockStride = 1U;
    memInfo.blockNum = 1U;
    memInfo.blockSize = scatterVnchwconvecord.dataType == DataType::DATA_B8 ? 16U : 32U; // B8 读写数据为B16和B32的一半
    memInfo.repeatTimes = scatterVnchwconvecord.repeat;
    constexpr uint64_t twice = 2;
    memInfo.repeatStride = scatterVnchwconvecord.dataType == DataType::DATA_B8 ?
        scatterVnchwconvecord.srcStride * twice : scatterVnchwconvecord.srcStride;
    bool srcHighHalf = scatterVnchwconvecord.dataType == DataType::DATA_B8 && scatterVnchwconvecord.srcHighHalf;
    ParseVaRegisterEvents(scatterVnchwconvecord.src0, srcHighHalf, event, events);
    ParseVaRegisterEvents(scatterVnchwconvecord.src1, srcHighHalf, event, events);

    memInfo.memType = MemType::UB;
    memInfo.opType = AccessType::WRITE;
    memInfo.repeatStride = scatterVnchwconvecord.dataType == DataType::DATA_B8 ?
        scatterVnchwconvecord.dstStride * twice : scatterVnchwconvecord.dstStride;
    bool dstHighHalf = scatterVnchwconvecord.dataType == DataType::DATA_B8 && scatterVnchwconvecord.dstHighHalf;
    ParseVaRegisterEvents(scatterVnchwconvecord.dst0, dstHighHalf, event, events);
    ParseVaRegisterEvents(scatterVnchwconvecord.dst1, dstHighHalf, event, events);
}

void ParseVRegPropCoorOPRecord(const KernelRecord &record, std::vector<SanEvent> &events)
{
    SanEvent event;
    auto& memInfo = event.eventInfo.memInfo;
    auto& vecRecord = record.payload.vecRegPropCoordOpRecord;
    if (vecRecord.dataType != DataType::DATA_B16 && vecRecord.dataType != DataType::DATA_B32) {
        SAN_WARN_LOG("Unsupported dataType[%u] for vector calculation command (region proposal coordinate).",
            static_cast<uint8_t>(vecRecord.dataType));
        return;
    }
    constexpr uint8_t regionRangeMax = 5; // 一个proposal中高位的两个字节不会被操作
    if (vecRecord.regionRange > regionRangeMax) {
        SAN_WARN_LOG("Invalid regionRange value %u", vecRecord.regionRange);
        return;
    }
    auto elementSize = FormatConverter::GetDataSizeByType(vecRecord.dataType);
    constexpr uint8_t proposalNum = 16;
    constexpr uint16_t elementNum = 8; // 一个proposal中包含的element数目

    SetLocationInfo(event, vecRecord, record.blockType, record.serialNo);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    memInfo.memType = MemType::UB;
    memInfo.opType = vecRecord.isExtract ? AccessType::WRITE : AccessType::READ;
    memInfo.vectorMask = { static_cast<uint64_t>(-1), static_cast<uint64_t>(-1) };
    memInfo.maskMode = MaskMode::MASK_NORM;
    memInfo.dataBits = BITS_EACH_BYTE;
    memInfo.addr = vecRecord.src;
    memInfo.blockNum = 1;
    memInfo.blockSize = elementSize * proposalNum;
    memInfo.blockStride = 0;
    memInfo.repeatTimes = vecRecord.repeat;
    memInfo.repeatStride = 1;
    events.emplace_back(event);
    AlignChecker::Instance().CheckAlign(event, record.recordType);

    memInfo.opType = vecRecord.isExtract ? AccessType::READ : AccessType::WRITE;
    memInfo.blockNum = proposalNum;
    memInfo.blockSize = elementSize;
    memInfo.blockStride = elementNum;
    memInfo.repeatStride = proposalNum * elementNum;
    // 建模地址由dst偏移得到
    memInfo.addr = vecRecord.dst + elementSize * vecRecord.regionRange;
    events.emplace_back(event);
    // 对dst做对齐检查
    memInfo.addr = vecRecord.dst;
    AlignChecker::Instance().CheckAlign(event, record.recordType);
}

}
