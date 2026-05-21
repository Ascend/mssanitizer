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


#ifndef CORE_FRAMEWORK_RECORD_PARSE_H
#define CORE_FRAMEWORK_RECORD_PARSE_H

#include <map>
#include <unordered_map>
#include <unordered_set>

#include "record_defs.h"
#include "runtime_context.h"
#include "event_def.h"

namespace Sanitizer {

std::map<uint16_t, bool>& GetMapAtomicMode();

template <typename Record>
inline void SetLocationInfo(SanEvent &event, Record const &record, const BlockType &blockType,
                            uint64_t serialNo)
{
    event.serialNo = serialNo;
    event.loc.blockType = blockType;
    event.loc.deviceId = RuntimeContext::Instance().GetDeviceId();
    event.loc.kernelIdx = RuntimeContext::Instance().kernelIdx_ - 1;
    event.loc.coreId = record.location.blockId;
    event.isAtomicMode = GetMapAtomicMode()[event.loc.coreId];
    event.loc.fileNo = record.location.fileNo;
    event.loc.lineNo = record.location.lineNo;
    event.loc.pc = record.location.pc;
}

struct HsetRecordKey {
    PipeType srcPipe;
    PipeType dstPipe;
    uint64_t eventId;
    MemType memType;

    bool operator < (const HsetRecordKey& other) const
    {
        if (srcPipe != other.srcPipe)
            return srcPipe < other.srcPipe;
        if (dstPipe != other.dstPipe)
            return dstPipe < other.dstPipe;
        if (eventId != other.eventId)
            return eventId < other.eventId;
        return memType < other.memType;
    }
};

struct HsetRecordState {
    bool cacheMode0; // 历史指令序列中是否缓存了mode 0 的hset
    bool cacheMode1; // 历史指令序列中是否缓存了mode 1 的hset
    bool isTransSet; // 历史指令序列中的Hset指令是否已经转化为set
};

class RecordParse {
public:
    static void Parse(const SanitizerRecord &record, std::vector<SanEvent> &events);
    static void ResetSyncInPipeInfo();
    // for UT
    static void ResetAll();
    thread_local static std::array<bool, static_cast<uint8_t>(PipeType::SIZE)> setWaitStat_;
    // 用图存储流水间同步信息，key:dst val:[src1, src2, ...]
    using DstSrcGraph = std::unordered_map<PipeType, std::unordered_set<PipeType>>;
    thread_local static DstSrcGraph dstSrcGraph_;
    thread_local static std::map<HsetRecordKey, HsetRecordState> hsetSyncMap_;
    // key：bufId，val：表示当前bufId的get_buf需要等待多少个rls_buf事件
    thread_local static std::unordered_map<uint64_t, uint64_t> getRlsBufMap_;
private:
    static void UpdateSyncInPipe(KernelRecord const& record, std::vector<SanEvent> &events);
    /// 递归函数，查找和targetPipe直接或间接相连的src pipe
    static void DfsSrcGraph(PipeType targetPipe, std::unordered_set<PipeType> &visited);
    static void ProcessHsetWaitSync(std::vector<SanEvent> &events);
    // 函数功能为：将同步语句中set事件的pipe-s替换为pipe-s-cal，避免pipe-s和其他pipe间竞争检测误报和漏报
    static void ReplaceSetSyncPipeScalar(std::vector<SanEvent> &events);
};
}
#endif
