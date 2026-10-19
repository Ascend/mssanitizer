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

#include <array>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include "kernel_manager.h"
#include "record_defs.h"
#include "runtime_context.h"
#include "event_def.h"

namespace Sanitizer {

std::map<uint16_t, bool>& GetMapAtomicMode();

inline void SetDeviceIdKernelIdx(SanEvent &event)
{
    event.loc.deviceId = RuntimeContext::Instance().GetDeviceId();
    std::size_t kernelCount{};
    KernelManager::Instance().GetKernelCount(event.loc.deviceId, kernelCount);
    if (kernelCount > 0) event.loc.kernelIdx = kernelCount - 1;
}

template <typename Record>
inline void SetLocationInfo(SanEvent &event, Record const &record, const BlockType &blockType,
                            uint64_t serialNo)
{
    event.serialNo = serialNo;
    event.loc.blockType = blockType;
    SetDeviceIdKernelIdx(event);
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
    // 用图存储流水间同步信息，key:dst val:[src1, src2, ...] 表示 dst 通过 wait_flag 等待过 src（dst 依赖 src）
    using DstSrcGraph = std::unordered_map<PipeType, std::unordered_set<PipeType>>;
    // PIPE_S 分支使用的依赖图：pipe 一旦发生访存就裁剪它作为 src 的边，
    // 避免 wait(dst=PIPE_S) 的传递注入沿陈旧依赖向外扩散
    thread_local static DstSrcGraph dstSrcGraph_;

    // ===== 跨流水同步环保护状态（单流水竞争检测用），规则见 UpdateSyncInPipe =====
    // 环：同一个 pipe 的 set 到 wait 的一次闭合往返。起始点 = 该 set 的 serialNo，终点 = 该 wait 的 serialNo。
    // 边由 (src, dst, eventId) 确定，三者全同才配对；同一 (src,dst) 下不同 eventId 互不影响。
    //   SET_FLAG(P,Q,e)  ：把 inheritedStartPoints_[P] 连同本次 serialNo 并入 edgeStartPoints_[P][Q][e]，逐 pipe 取 max
    //   WAIT_FLAG(S,P,e) ：把 edgeStartPoints_[S][P][e] 并入 inheritedStartPoints_[P]；含 P 则环闭合回 P，
    //                      closedRingStartSerialNo_[P] 推进到该起始点
    // 起始点按 pipe 下标定长存放（PipeType::SIZE ≤ 12），热路径上不产生 map 节点分配
    using RingStartPoints = std::array<uint64_t, static_cast<size_t>(PipeType::SIZE)>; // pipe -> 最大的环起始点（0 表示无）
    using RingStartPointsByEvent = std::map<uint32_t /*eventId*/, RingStartPoints>;    // 同一 (src,dst) 内按 eventId 分边
    using RingStartPointsByDst = std::map<PipeType /*dst*/, RingStartPointsByEvent>;   // per-src：(dst)->(eventId)->起始点
    // per-src：按 (src,dst,eventId) 聚合的环起始点
    thread_local static std::array<RingStartPointsByDst, static_cast<size_t>(PipeType::SIZE)> edgeStartPoints_;
    // per-pipe：已随 wait 继承、待随下一次 set 继续外发的环起始点
    thread_local static std::array<RingStartPoints, static_cast<size_t>(PipeType::SIZE)> inheritedStartPoints_;
    // per-pipe：已闭环环中最大的环起始点 serialNo
    thread_local static std::array<uint64_t, static_cast<size_t>(PipeType::SIZE)> closedRingStartSerialNo_;
    thread_local static std::map<HsetRecordKey, HsetRecordState> hsetSyncMap_;
    // key：(blockId, bufId)，val：表示当前(blockId, bufId)的get_buf需要等待多少个rls_buf事件
    thread_local static std::map<std::pair<uint16_t, uint64_t>, uint64_t> getRlsBufMap_;

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
