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


#ifndef RACE_SANITIZER_ALG_FRAMEWORK_MEM_EVENT_CHECKER_H
#define RACE_SANITIZER_ALG_FRAMEWORK_MEM_EVENT_CHECKER_H

#include <vector>
#include <memory>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include "core/framework/event_def.h"
#include "core/framework/platform_config.h"
#include "core/framework/record_defs.h"
#include "vector_clock.h"

namespace Sanitizer {


inline RaceDispInfo FillRaceDispInfo(const MemEvent &event1, const MemEvent &event2)
{
    auto info = RaceDispInfo{};
    info.p1.Init(event1);
    info.p2.Init(event2);
    return info;
}

inline bool NeedExpandBlockDim(KernelType kernelType, DeviceType deviceType)
{
    return kernelType == KernelType::MIX && HasSubBlocks(deviceType);
}

inline uint32_t GetEventExpandBlockIndex(const SanEvent &event)
{
    uint32_t aicoreIndex =
        event.loc.blockType == BlockType::AIVEC ? event.loc.coreId / C220_VEC_SUB_BLOCKDIM : event.loc.coreId;
    uint32_t subBlockIndex =
        event.loc.blockType == BlockType::AIVEC ? event.loc.coreId % C220_VEC_SUB_BLOCKDIM : C220_VEC_SUB_BLOCKDIM;
    return aicoreIndex * C220_MIX_SUB_BLOCKDIM + subBlockIndex;
}

inline uint32_t GetEventBlockIndex(const SanEvent &event, KernelType kernelType, DeviceType deviceType, RaceCheckType checkType)
{
    if (!NeedExpandBlockDim(kernelType, deviceType)) {
        if (checkType == RaceCheckType::SINGLE_BLOCK_CHECK) {
            return 0U;
        }
        return event.loc.coreId;
    }
    return GetEventExpandBlockIndex(event);
}

template <RaceCheckType checkType>
inline uint32_t GetPipeIdxByMemEvent(const MemEvent &event, KernelType kernelType, DeviceType deviceType)
{
    SanEvent sanEvent{};
    sanEvent.loc = event.loc;
    uint32_t blockIdx = GetEventBlockIndex(sanEvent, kernelType, deviceType, checkType);
    return (blockIdx * static_cast<uint16_t>(PipeType::SIZE)) + static_cast<uint16_t>(event.pipe);
}

inline uint32_t GetExpandPipeIdx(MemEvent const &event, uint32_t blockNum)
{
    SanEvent sanEvent{};
    sanEvent.loc = event.loc;
    uint32_t blockIdx = GetEventExpandBlockIndex(sanEvent);
    return event.loc.deviceIdx * blockNum * static_cast<uint32_t>(PipeType::SIZE) +
        blockIdx * static_cast<uint32_t>(PipeType::SIZE) + static_cast<uint32_t>(event.pipe);
}

// 该类集成了内存块之间做竞争比较的功能。
// 需要记录算子运行过程已经发生的所有内存事件(携带时间戳)
// 新发生的内存事件需要和已经记录的所有内存事件一一比较,并返回相互冲突的事件信息
class MemEventChecker {
public:
    // 存在竞争的事件对的索引
    using RaceMemEventsIdx = std::vector<std::pair<uint64_t, uint64_t>>;
    void RunAlgorithm();
    // 扫描线算法，事件拆分为开始和结束，按地址排序，再进行检测
    void ScanlineAlgorithm(RaceMemEventsIdx &raceMemEventsIdx);
    void PushEvent(const MemEvent& event);
    void Init(KernelType kernelType, DeviceType deviceType, RaceCheckType checkType);
    void Init(uint32_t blockNum);
    std::shared_ptr<std::vector<RaceDispInfo>> GetResult() const;
    // 测试接口
    uint32_t GetRaceCount() const;
    std::vector<MemEvent> events_;
private:
    uint32_t raceCnt_ = 0U;
    uint32_t blockNum_{};
    KernelType kernelType_{};
    DeviceType deviceType_{};
    RaceCheckType checkType_{};
     // 用来表示地址区间的数据结构，pair<起始地址, 结束地址>
    using AddrRangeVec = std::vector<std::pair<uint64_t, uint64_t>>;
    // 缓存检测结果
    std::shared_ptr<std::vector<RaceDispInfo>> result_;
    std::unordered_set<RaceDispInfo, RaceEventHash, RaceEventEqual> raceSet_;
    bool IsMemSpaceOverlap(const MemOpInfo &op1, const MemOpInfo &op2);
    bool IsAtomicAgainst(const MemEvent& event1, const MemEvent& event2);
    bool IsInnerCoreRaceEvent(uint64_t eventIdx1, uint64_t eventIdx2);
    bool IsSinglePipeRaceEvent(uint64_t eventIdx1, uint64_t eventIdx2);
    bool IsCrossCoreRaceEvent(uint64_t eventIdx1, uint64_t eventIdx2);
    bool IsCrossNpuRaceEvent(uint64_t eventIdx1, uint64_t eventIdx2);

    using CheckTypeFunc = bool (MemEventChecker::*)(uint64_t, uint64_t);
    void CheckExistRaceEvents(const std::unordered_set<uint64_t> &historyEventsIdx, CheckTypeFunc checkTypeFunc,
        uint64_t curEventIdx, RaceMemEventsIdx &raceMemEventsIdx);
    std::unordered_map<RaceCheckType, CheckTypeFunc> checkTypeMap_;
};

}
#endif

