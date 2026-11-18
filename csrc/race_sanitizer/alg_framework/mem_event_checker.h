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
#include "core/framework/vector_clock.h"
#include "core/framework/pipeline_replayer.h"

namespace Sanitizer {

inline RaceDispInfo FillRaceDispInfo(bool isMissDcci, const MemEvent &event1, const MemEvent &event2,
    uint64_t event1DynamicErrIdx, uint64_t event2DynamicErrIdx) {
    auto info = RaceDispInfo{};
    info.isMissDcci = isMissDcci;
    info.p1.Init(event1, event1DynamicErrIdx);
    info.p2.Init(event2, event2DynamicErrIdx);
    return info;
}

template <RaceCheckType checkType>
inline uint32_t GetPipeIdxByMemEvent(const MemEvent &event, KernelType kernelType, DeviceType deviceType)
{
    uint32_t blockIdx = GetEventBlockIndexByLoc(event.loc, kernelType, deviceType, checkType);
    return (blockIdx * static_cast<uint16_t>(PipeType::SIZE)) + static_cast<uint16_t>(event.pipe);
}

inline uint32_t GetExpandPipeIdx(MemEvent const &event, uint32_t blockNum)
{
    uint32_t blockIdx = GetEventExpandBlockIndexByLoc(event.loc);
    return event.loc.deviceIdx * blockNum * static_cast<uint32_t>(PipeType::SIZE) +
        blockIdx * static_cast<uint32_t>(PipeType::SIZE) + static_cast<uint32_t>(event.pipe);
}

struct CompareShadowMemoryRecord {
    ShadowMemoryRecord record;
    uint64_t idx;                  // record对应的全局索引
};

// 扫描线算法端点结构（定义见 mem_event_checker.cpp）
struct SortMetaData;

// 该类集成了内存块之间做竞争比较的功能。
// 需要记录算子运行过程已经发生的所有内存事件(携带时间戳)
// 新发生的内存事件需要和已经记录的所有内存事件一一比较,并返回相互冲突的事件信息
class MemEventChecker {
public:
    struct EventIdxInfo {
        uint64_t eventIdx;               // 当前event位于events_的索引
        uint64_t dynamicErrorIdx;        // dynamicEvent存在错误的前提下，对应的错误索引
    };
    // 存在竞争的事件对的索引
    using RaceMemEventsIdx = std::vector<std::pair<EventIdxInfo, EventIdxInfo>>;
    // 用来表示地址区间的数据结构，pair<起始地址, 结束地址>
    using AddrRangeVec = std::vector<std::pair<uint64_t, uint64_t>>;
    void RunAlgorithm();
    // 扫描线算法，事件拆分为开始和结束，按地址排序，再进行检测
    void ScanlineAlgorithm(RaceMemEventsIdx &raceMemEventsIdx, RaceMemEventsIdx &missDcciMemEventsIdx) const;
    void PushEvent(const MemEvent& event);
    void Init(KernelType kernelType, DeviceType deviceType, RaceCheckType checkType);
    void Init(uint32_t blockNum);
    void SetVecSubBlockDim(uint8_t vecSubBlockDim) { vecSubBlockDim_ = vecSubBlockDim; }
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
    uint8_t vecSubBlockDim_ = C220_VEC_SUB_BLOCKDIM; // 当前算子的 vec:cube 配比 (1 或 2)，默认 2
    // 缓存检测结果
    std::shared_ptr<std::vector<RaceDispInfo>> result_;
    std::unordered_set<RaceDispInfo, ErrorEventHash, ErrorEventEqual> raceSet_;
    bool IsMemSpaceOverlap(const MemEvent &op1, const MemEvent &op2,
        EventIdxInfo &idxInfo1, EventIdxInfo &idxInfo2) const;
    bool IsAtomicAgainst(const MemEvent& event1, const MemEvent& event2) const;
    // 单块竞争前置判定（并发/非原子），地址重叠由调用方继续检查；
    // 调用方需保证两个事件属于不同 pipe；pipeIdx/ownTime(=vt.Get(pipeIdx)) 由调用方预计算
    bool IsInnerCoreRaceCandidate(const MemEvent &event1, const MemEvent &event2,
        uint32_t pipeIdx1, uint32_t ownTime1, uint32_t pipeIdx2, uint32_t ownTime2) const;
    bool IsInnerCoreRaceEvent(EventIdxInfo &idxInfo1, EventIdxInfo &idxInfo2) const;
    bool IsSinglePipeRaceEvent(EventIdxInfo &idxInfo1, EventIdxInfo &idxInfo2) const;
    bool IsCrossCoreRaceEvent(EventIdxInfo &idxInfo1, EventIdxInfo &idxInfo2) const;
    bool IsCrossNpuRaceEvent(EventIdxInfo &idxInfo1, EventIdxInfo &idxInfo2) const;
    /// 判断两个事件是否需要进行核间竞争检测（过滤同block、不同物理UB等情况）
    bool NeedCrossCoreCheck(const MemEvent &event1, const MemEvent &event2) const;
    bool IsMissDcciEvent(EventIdxInfo &idxInfo1, EventIdxInfo &idxInfo2) const;

    using CheckTypeFunc = bool (MemEventChecker::*)(EventIdxInfo&, EventIdxInfo&) const;
    void CheckExistRaceEvents(const std::unordered_set<uint64_t> &historyEventsIdx, CheckTypeFunc checkTypeFunc,
        uint64_t curEventIdx, RaceMemEventsIdx &raceMemEventsIdx) const;
    std::unordered_map<RaceCheckType, CheckTypeFunc> checkTypeMap_;

    // 扫描线执行上下文：按 checkType 选定的判定回调 + DCCI 开关，避免各分支重复读取
    struct ScanlineContext {
        CheckTypeFunc checkFunc;  // 竞争判定回调
        bool checkDcci;           // 是否 DCCI 检测模式
        RaceCheckType checkType;  // 当前竞争检测类型
    };
    void BuildScanlineMetaData(std::vector<SortMetaData> &eventsMetaData, bool &hasAnyDynamic) const;
    void ScanGeneric(const std::vector<SortMetaData> &eventsMetaData, const ScanlineContext &ctx,
        RaceMemEventsIdx &raceMemEventsIdx, RaceMemEventsIdx &missDcciMemEventsIdx) const;
    void ScanSinglePipeWindow(const std::vector<SortMetaData> &eventsMetaData, const ScanlineContext &ctx,
        RaceMemEventsIdx &raceMemEventsIdx) const;
    void ScanSingleBlockFast(const std::vector<SortMetaData> &eventsMetaData, const ScanlineContext &ctx,
        RaceMemEventsIdx &raceMemEventsIdx) const;
};

}
#endif
