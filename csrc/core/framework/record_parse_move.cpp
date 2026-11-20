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


#include "record_parse_move.h"

#include "record_parse.h"
#include "address_sanitizer/align_checker.h"
#include "format_converter.h"

namespace Sanitizer {

void ParseDmaMovWithByteMode(const DmaMovRecord &dmaMovRecord, std::vector<SanEvent> &events,
    SanEvent &event)
{
    // BM_ENABLE
    auto& memInfo = event.eventInfo.memInfo;
    memInfo.memType = dmaMovRecord.srcMemType;
    memInfo.opType = AccessType::READ;
    memInfo.addr = dmaMovRecord.src;
    memInfo.blockNum = CeilByAlignSize<MOV_LOCAL_BLOCK_SIZE>(dmaMovRecord.lenBurst);
    memInfo.blockSize = 1U;
    memInfo.blockStride = 1U;
    memInfo.repeatTimes = dmaMovRecord.nBurst;
    memInfo.repeatStride = memInfo.blockNum + dmaMovRecord.srcStride * MOV_LOCAL_BLOCK_SIZE;
    AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV);
    events.emplace_back(event);
    memInfo.memType = dmaMovRecord.dstMemType;
    memInfo.opType = AccessType::WRITE;
    memInfo.addr = dmaMovRecord.dst;
    memInfo.blockNum = dmaMovRecord.lenBurst;
    memInfo.blockSize = 1U;
    memInfo.blockStride = 1U;
    memInfo.repeatStride = memInfo.blockNum + dmaMovRecord.dstStride * MOV_LOCAL_BLOCK_SIZE;
    AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV);
    events.emplace_back(event);
}

void ParseDmaMovWithPadMode(const DmaMovRecord& dmaMovRecord, std::vector<SanEvent>& events,
    SanEvent& event)
{
    auto& memInfo = event.eventInfo.memInfo;
    if (dmaMovRecord.padMode >= PadMode::PAD_MODE1 && dmaMovRecord.padMode <= PadMode::PAD_MODE5) {
        /// PAD_MODE1 到 PAD_MODE5 源地址的数据都是连续排布并且 lenBurst 必须是 1
        memInfo.memType = dmaMovRecord.srcMemType;
        memInfo.opType = AccessType::READ;
        memInfo.addr = dmaMovRecord.src;
        memInfo.blockNum = 1U;
        memInfo.blockSize = dmaMovRecord.nBurst * MOV_LOCAL_BLOCK_SIZE /
            FormatConverter::GetPadModeScaleValue(dmaMovRecord.padMode);
        memInfo.blockStride = 1U;
        memInfo.repeatTimes = 1U;
        memInfo.repeatStride = 0U;
        AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV);
        events.emplace_back(event);
        memInfo.memType = dmaMovRecord.dstMemType;
        memInfo.opType = AccessType::WRITE;
        memInfo.addr = dmaMovRecord.dst;
        memInfo.blockSize = MOV_LOCAL_BLOCK_SIZE;
        memInfo.repeatTimes = dmaMovRecord.nBurst;
        memInfo.repeatStride = memInfo.blockNum + dmaMovRecord.dstStride;
        AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV);
        events.emplace_back(event);
    } else if (dmaMovRecord.padMode >= PadMode::PAD_MODE6 && dmaMovRecord.padMode <= PadMode::PAD_MODE8) {
        /// PAD_MODE6 到 PAD_MODE8 目的地址的数据都是连续排布并且 lenBurst 必须是 1
        memInfo.memType = dmaMovRecord.srcMemType;
        memInfo.opType = AccessType::READ;
        memInfo.addr = dmaMovRecord.src;
        memInfo.blockNum = 1U;
        memInfo.blockSize = MOV_LOCAL_BLOCK_SIZE;
        memInfo.blockStride = 1U;
        memInfo.repeatTimes = dmaMovRecord.nBurst;
        memInfo.repeatStride = memInfo.blockNum + dmaMovRecord.srcStride;
        AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV);
        events.emplace_back(event);
        memInfo.memType = dmaMovRecord.dstMemType;
        memInfo.opType = AccessType::WRITE;
        memInfo.addr = dmaMovRecord.dst;
        memInfo.blockSize = dmaMovRecord.nBurst * MOV_LOCAL_BLOCK_SIZE /
            FormatConverter::GetPadModeScaleValue(dmaMovRecord.padMode);
        memInfo.repeatTimes = 1U;
        memInfo.repeatStride = 0U;
        AlignChecker::Instance().CheckAlign(event, RecordType::DMA_MOV);
        events.emplace_back(event);
    }
}
} // namespace Sanitizer
