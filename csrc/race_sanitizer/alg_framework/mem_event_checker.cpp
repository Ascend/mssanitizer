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


#include "mem_event_checker.h"

#include "core/framework/constant.h"
#include "core/framework/event_def.h"
#include "core/framework/kernel_manager.h"
#include "core/framework/record_defs.h"
#include "mem_series.h"
#include "core/framework/utility/log.h"

namespace {

using namespace Sanitizer;
    /// 双指针判断地址是否存在交叉
bool IsMemoryOverlap(const MemOpInfo &op1, const MemOpInfo &op2)
{
    if (op1.memType != op2.memType) {
        return false;
    }

    // 进行细粒度的交集判断
    MemSeries memSeries1(op1);
    MemSeries memSeries2(op2);
    Interval interval1;
    Interval interval2;
    if (!memSeries1.GetNextItv(interval1) || !memSeries2.GetNextItv(interval2)) {
        return false;
    }
    do {
        if (interval1.start < interval2.end && interval2.start < interval1.end) {
            return true;
        }
        if (interval1.end < interval2.end) {
            if (!memSeries1.GetNextItv(interval1)) {
                break;
            }
        } else {
            if (!memSeries2.GetNextItv(interval2)) {
                break;
            }
        }
    } while (true);

    return false;
}

inline bool CompareSmRecord(const CompareShadowMemoryRecord &lhs, const CompareShadowMemoryRecord &rhs)
{
    if (lhs.record.addr == rhs.record.addr) {
        if (lhs.record.size == rhs.record.size) {
            return lhs.idx < rhs.idx;
        }
        return lhs.record.size < rhs.record.size;
    }
    return lhs.record.addr < rhs.record.addr;   // 按地址升序排序
}

std::vector<CompareShadowMemoryRecord> TransToRecords(void *buffer, size_t count)
{
    const ShadowMemoryRecord *shRecord = reinterpret_cast<const ShadowMemoryRecord *>(buffer);
    std::vector<CompareShadowMemoryRecord> records;
    records.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        records.push_back({shRecord[i], i});
    }
    std::sort(records.begin(), records.end(), CompareSmRecord);
    return records;
}

bool IsMemoryOverlap(const DynamicOpInfo &op1, const DynamicOpInfo &op2, MemEventChecker::EventIdxInfo &idxInfo1, MemEventChecker::EventIdxInfo &idxInfo2)
{
    bool existRace{};
    if (op1.dynamicType == RecordType::SIMT_ENTRY && op1.count > 0 && op1.buffer != nullptr &&
        op2.dynamicType == RecordType::SIMT_ENTRY && op2.count > 0 && op2.buffer != nullptr) {
        std::vector<CompareShadowMemoryRecord> op1Records = TransToRecords(op1.buffer, op1.count);
        std::vector<CompareShadowMemoryRecord> op2Records = TransToRecords(op2.buffer, op2.count);
        auto it1 = op1Records.cbegin();
        auto end1 = op1Records.cend();
        auto it2 = op2Records.cbegin();
        auto end2 = op2Records.cend();
        while (it1 != end1 && it2 != end2) {
            uint64_t start1 = it1->record.addr;
            uint64_t end1 = it1->record.addr + it1->record.size;
            uint64_t start2 = it2->record.addr;
            uint64_t end2 = it2->record.addr + it2->record.size;
            if (start1 < end2 && start2 < end1 &&
                !((it1->record.accessType == AccessType::READ && it2->record.accessType == AccessType::READ))) {
                idxInfo1.dynamicErrorIdx = it1->idx;
                idxInfo2.dynamicErrorIdx = it2->idx;
                if (it1->record.location.pc != it2->record.location.pc) {
                    return true;
                }
                existRace = true;
            }
            if (end1 < end2) {
                ++it1;
            } else if (end1 == end2) {
                ++it1;
                ++it2;
            } else {
                ++it2;
            }
        }
    }
    return existRace;
}

void SplitMem(const MemOpInfo &op, MemEventChecker::AddrRangeVec &addrRangeVec)
{
    addrRangeVec.reserve(op.repeatTimes * op.blockNum);
    for (uint64_t repeatIdx = 0; repeatIdx < op.repeatTimes; ++repeatIdx) {
        uint64_t repeatAddr = op.addr + repeatIdx * op.repeatStride * op.blockSize;
        for (uint64_t blockIdx = 0; blockIdx < op.blockNum; ++blockIdx) {
            uint64_t startAddr = repeatAddr + blockIdx * op.blockStride * op.blockSize;
            uint64_t endAddr = startAddr + op.blockSize;
            addrRangeVec.emplace_back(startAddr, endAddr);
        }
    }
}

bool IsMemoryOverlap(const MemOpInfo &op1, const DynamicOpInfo &op2, MemEventChecker::EventIdxInfo &idxInfo2)
{
    if (op2.dynamicType == RecordType::SIMT_ENTRY && op2.count > 0 && op2.buffer != nullptr) {
        std::vector<CompareShadowMemoryRecord> op2Records = TransToRecords(op2.buffer, op2.count);
        MemEventChecker::AddrRangeVec op1AddrRangeVec;
        SplitMem(op1, op1AddrRangeVec);
        std::sort(op1AddrRangeVec.begin(), op1AddrRangeVec.end());
        MemEventChecker::AddrRangeVec::const_iterator it1 = op1AddrRangeVec.cbegin();
        MemEventChecker::AddrRangeVec::const_iterator end1 = op1AddrRangeVec.cend();
        auto it2 = op2Records.cbegin();
        auto end2 = op2Records.cend();
        while (it1 != end1 && it2 != end2) {
            uint64_t start1 = it1->first;
            uint64_t end1 = it1->second;
            uint64_t start2 = it2->record.addr;
            uint64_t end2 = it2->record.addr + it2->record.size;
            if (start1 < end2 && start2 < end1 &&
                !((it2->record.accessType == AccessType::READ && op1.opType == AccessType::READ))) {
                idxInfo2.dynamicErrorIdx = it2->idx;
                return true;   // 发现重叠，返回true
            }
            if (end1 < end2) {
                ++it1;
            } else if (end1 == end2) {
                ++it1;
                ++it2;
            } else {
                ++it2;
            }
        }
    }
    return false;
}

}

namespace Sanitizer {

void MemEventChecker::Init(KernelType kernelType, DeviceType deviceType, RaceCheckType checkType)
{
    this->kernelType_ = kernelType;
    this->deviceType_ = deviceType;
    this->checkType_ = checkType;
    checkTypeMap_[RaceCheckType::SINGLE_BLOCK_CHECK] = &MemEventChecker::IsInnerCoreRaceEvent;
    checkTypeMap_[RaceCheckType::SINGLE_PIPE_CHECK] = &MemEventChecker::IsSinglePipeRaceEvent;
    checkTypeMap_[RaceCheckType::CROSS_BLOCK_CHECK] = &MemEventChecker::IsCrossCoreRaceEvent;
    result_ = std::make_shared<std::vector<RaceDispInfo>>();
}

void MemEventChecker::Init(uint32_t blockNum)
{
    blockNum_ = blockNum;
    this->checkType_ = RaceCheckType::CROSS_NPU_CHECK;
    checkTypeMap_[RaceCheckType::CROSS_NPU_CHECK] = &MemEventChecker::IsCrossNpuRaceEvent;
    result_ = std::make_shared<std::vector<RaceDispInfo>>();
}

void MemEventChecker::PushEvent(const MemEvent &event)
{
    events_.push_back(event);
}

inline uint64_t GetMemMaxLength(MemEvent const &event)
{
    if (event.isDynamic) {
        return event.dynamicMemInfo.maxAddr - event.dynamicMemInfo.minAddr;
    }
    auto &op = event.memInfo;
    if (op.maskMode == MaskMode::MASK_COUNT && (op.vectorMask.mask0 == 0 || op.dataBits == 0)) {
        return 0UL;
    }

    if (op.repeatTimes == 0 || op.blockNum == 0 || op.blockSize == 0) {
        return 0UL;
    }

    return (op.repeatTimes - 1) * op.repeatStride * op.blockSize +
        (op.blockNum - 1) * op.blockStride * op.blockSize + op.blockSize;
}

// 扫描线算法需要用到的结构体
struct SortMetaData {
    uint64_t addr;
    uint64_t eventIdx;
    bool isStart;

    SortMetaData(uint64_t addr, uint64_t eventIdx, bool isStart) : addr(addr), eventIdx(eventIdx), isStart(isStart) {}
};

/// 排序规则：按照事件地址从小到大排序，
/// 如果两个事件地址相同，则将结束事件（标记为false）放到开始事件（标记为true）前面
inline bool CompareEvent(const SortMetaData &lhs, const SortMetaData &rhs)
{
    if (lhs.addr == rhs.addr) {
        if (lhs.isStart == rhs.isStart) {
            // 地址相同，并且事件属性相同，则按照事件index升序排列
            return lhs.eventIdx < rhs.eventIdx;
        }
        return !lhs.isStart;
    }
    return lhs.addr < rhs.addr;   // 按地址升序排序
}

bool MemEventChecker::NeedCrossCoreCheck(const MemEvent &event1, const MemEvent &event2) const {
    // 同一个block的无需比较
    if (event1.loc.coreId == event2.loc.coreId && event1.loc.blockType == event2.loc.blockType) {
        return false;
    }

    // 非950系列芯片无需检查UB物理差异
    if (!IsAscend95(this->deviceType_)) {
        return true;
    }

    // 非UB内存类型无需检查UB物理差异，直接进入后续检测
    auto mt1 = event1.isDynamic ? event1.dynamicMemInfo.memType : event1.memInfo.memType;
    auto mt2 = event2.isDynamic ? event2.dynamicMemInfo.memType : event2.memInfo.memType;
    if (mt1 != MemType::UB || mt2 != MemType::UB) {
        return true;
    }

    // 同 blockType 的 UB 事件位于不同物理 UB，无需检测
    // （前面的相同core同block已返回false，此处覆盖不同core同block）
    if (event1.loc.blockType == event2.loc.blockType) {
        return false;
    }

    // AIVEC与AICUBE跨类型：判断是否属于同一物理组
    if ((event1.loc.blockType == BlockType::AIVEC && event2.loc.blockType == BlockType::AICUBE)) {
        if (event1.loc.coreId != event2.memInfo.dstCoreId) {
            return false;
        }
    }

    if ((event1.loc.blockType == BlockType::AICUBE && event2.loc.blockType == BlockType::AIVEC)) {
        if (event1.memInfo.dstCoreId != event2.loc.coreId) {
            return false;
        }
    }

    return true;
}

/// 扫描线算法已经保证了存在地址交叉，由于核内竞争是统一对GM和片上内存进行排序，故核内竞争判断条件需要判断
/// 存在地址交叉的两个事件是否memType相同
bool MemEventChecker::IsInnerCoreRaceEvent(EventIdxInfo &idxInfo1, EventIdxInfo &idxInfo2) const
{
    auto &event1 = this->events_.at(idxInfo1.eventIdx);
    auto &event2 = this->events_.at(idxInfo2.eventIdx);

    if (event1.pipe == event2.pipe) {
        // 同一个pipe的无需比较
        return false;
    }

    // 排序之后已经丢失了event1 event2的先后关系，故均需判断
    if (!VectorClock::IsNotHappensBefore(event1.vt, event2.vt,
        GetPipeIdxByMemEvent<RaceCheckType::SINGLE_BLOCK_CHECK>(event1, this->kernelType_, this->deviceType_),
        GetPipeIdxByMemEvent<RaceCheckType::SINGLE_BLOCK_CHECK>(event2, this->kernelType_, this->deviceType_))) {
        return false;
    }

    if (IsAtomicAgainst(event1, event2)) {
        return false;
    }

    // 比较内存空间是否存在重叠
    return IsMemSpaceOverlap(event1, event2, idxInfo1, idxInfo2);
}

/// 判断核内同一pipe上的竞争
bool MemEventChecker::IsSinglePipeRaceEvent(EventIdxInfo &idxInfo1, EventIdxInfo &idxInfo2) const
{
    auto &event1 = this->events_.at(idxInfo1.eventIdx);
    auto &event2 = this->events_.at(idxInfo2.eventIdx);

    // 内存地址类型不相同，则直接返回
    if (event1.memInfo.memType != event2.memInfo.memType) {
        return false;
    }

    // 只检查相同流水，不同流水在IsInnerCoreRaceEvent中判断，这里直接返回
    if (event1.pipe != event2.pipe) {
        return false;
    }

    // 同一条指令内不检测竞争，直接返回
    if (event1.serialNo == event2.serialNo) {
        return false;
    }

    if (IsAtomicAgainst(event1, event2)) {
        return false;
    }

    // 一条流水内指令队列长度上限为 16 或 32, 视架构和流水类型而定。
    // 此处出于性能和实际场景考虑，当两条指令序间隔 16 以上时不检测流水内竞争
    static constexpr uint64_t MAX_SERIAL_NUMBER_DIFF = 16;
    uint64_t diff = (event1.pipeSerialNo > event2.pipeSerialNo) ?
                (event1.pipeSerialNo - event2.pipeSerialNo) : (event2.pipeSerialNo - event1.pipeSerialNo);
    if (diff > MAX_SERIAL_NUMBER_DIFF) {
        return false;
    }

    // barrierNo记录pipe_barrier出现的次数，不相等说明两条指令之间有pipe_barrier，不会竞争
    if (event1.barrierNo != event2.barrierNo) {
        return false;
    }

    // 比较内存空间是否存在重叠
    return IsMemSpaceOverlap(event1, event2, idxInfo1, idxInfo2);
}

/// 扫描线算法已经保证了存在地址交叉，进入到核间竞争的事件已经默认为GM事件，判断条件无需写判断内存类型是否相同
bool MemEventChecker::IsCrossCoreRaceEvent(EventIdxInfo &idxInfo1, EventIdxInfo &idxInfo2) const
{
    auto &event1 = this->events_.at(idxInfo1.eventIdx);
    auto &event2 = this->events_.at(idxInfo2.eventIdx);

    if (!NeedCrossCoreCheck(event1, event2)) {
        return false;
    }

    if (!VectorClock::IsNotHappensBefore(event1.vt, event2.vt,
        GetPipeIdxByMemEvent<RaceCheckType::CROSS_BLOCK_CHECK>(event1, this->kernelType_, this->deviceType_),
        GetPipeIdxByMemEvent<RaceCheckType::CROSS_BLOCK_CHECK>(event2, this->kernelType_, this->deviceType_))) {
        return false;
    }

    if (IsAtomicAgainst(event1, event2)) {
        return false;
    }

    // 比较内存空间是否存在重叠
    return IsMemSpaceOverlap(event1, event2, idxInfo1, idxInfo2);
}

bool MemEventChecker::IsCrossNpuRaceEvent(EventIdxInfo &idxInfo1, EventIdxInfo &idxInfo2) const
{
    auto &event1 = this->events_.at(idxInfo1.eventIdx);
    auto &event2 = this->events_.at(idxInfo2.eventIdx);
    // 同一个 device 无需比较
    if (event1.loc.deviceId == event2.loc.deviceId) {
        return false;
    }

    if (!VectorClock::IsNotHappensBefore(event1.vt, event2.vt,
                                         GetExpandPipeIdx(event1, blockNum_),
                                         GetExpandPipeIdx(event2, blockNum_))) {
        return false;
    }

    // 仅比较相同 kernelname 的算子是否竞争
    KernelSummary kernelSummary1{};
    KernelSummary kernelSummary2{};
    if (!KernelManager::Instance().Get(event1.loc.deviceId, event1.loc.kernelIdx, kernelSummary1) ||
        !KernelManager::Instance().Get(event2.loc.deviceId, event2.loc.kernelIdx, kernelSummary2)) {
        return false;
    }
    if (std::string(kernelSummary1.kernelName) != std::string(kernelSummary2.kernelName)) {
        return false;
    }

    // 比较内存空间是否存在重叠
    return IsMemSpaceOverlap(event1, event2, idxInfo1, idxInfo2);
}

void MemEventChecker::CheckExistRaceEvents(const std::unordered_set<uint64_t> &historyEventsIdx,
    CheckTypeFunc checkTypeFunc, uint64_t curEventIdx, RaceMemEventsIdx &raceMemEventsIdx) const
{
    EventIdxInfo idxInfo1{};
    idxInfo1.eventIdx = curEventIdx;
    EventIdxInfo idxInfo2{};
    for (const auto &eventIdx : historyEventsIdx) {
        idxInfo2.eventIdx = eventIdx;
        if ((this->*checkTypeFunc)(idxInfo1, idxInfo2)) {
            raceMemEventsIdx.emplace_back(idxInfo1, idxInfo2);
        }
    }
}

void MemEventChecker::ScanlineAlgorithm(RaceMemEventsIdx &raceMemEventsIdx) const
{
    std::vector<SortMetaData> eventsMetaData;
    uint8_t multiSize = 2;
    eventsMetaData.reserve(this->events_.size() * multiSize);
    // 1. 将单个事件拆分为开始事件和结束事件
    for (size_t eventIdx = 0; eventIdx < this->events_.size(); ++eventIdx) {
        auto& event = this->events_[eventIdx];
        auto maxLength = GetMemMaxLength(event);
        if (maxLength == 0U) { // 如果当前事件的地址最大长度为0，则直接跳过，不需要进行竞争事件对的分析
            continue;
        }
        if (event.isDynamic) {
            eventsMetaData.emplace_back(SortMetaData(event.dynamicMemInfo.minAddr, eventIdx, true));   // 开始事件
            eventsMetaData.emplace_back(SortMetaData(event.dynamicMemInfo.maxAddr, eventIdx, false));  // 结束事件
        } else {
            eventsMetaData.emplace_back(SortMetaData(event.memInfo.addr, eventIdx, true));               // 开始事件
            eventsMetaData.emplace_back(SortMetaData(event.memInfo.addr + maxLength, eventIdx, false));  // 结束事件
        }
    }
    // 2. 按照事件地址升序排列
    std::sort(eventsMetaData.begin(), eventsMetaData.end(), CompareEvent);
    /// key：MemType为地址空间，value：历史读写事件，这里用eventIdx存储，以便节省内存
    /// 目的是为了节省cube核检测的耗时，cube中的L1/L0A/L0B/L0C地址空间按照MemType拆分后，后续的历史事件比对
    /// 只需要比对相同地址空间的地址是否存在交叉，提升程序性能；
    std::unordered_map<MemType, std::unordered_set<uint64_t>> historyWriteEventsIdxMap;
    std::unordered_map<MemType, std::unordered_set<uint64_t>> historyReadEventsIdxMap;

    auto checkFuncIter = checkTypeMap_.find(this->checkType_);
    if (checkFuncIter == checkTypeMap_.end()) {
        SAN_WARN_LOG("RaceCheckType %u is NOT found in checkTypeMap", static_cast<uint8_t>(this->checkType_));
        return;
    }
    // 3.遍历事件，做地址重叠和读写判断
    for (const auto &metaData : eventsMetaData) {
        uint64_t curEventIdx = metaData.eventIdx;
        auto &curEvent = this->events_.at(curEventIdx);
        auto opType = curEvent.memInfo.opType;
        auto memType = curEvent.memInfo.memType;
        if (curEvent.isDynamic) {
            // 动态内存事件先统一转换为写事件，保证不漏检，后续细粒度判断会过滤掉读读竞争事件
            opType = AccessType::WRITE;
            memType = curEvent.dynamicMemInfo.memType;
        }
        if (metaData.isStart) {
            // 检查与历史写事件的冲突
            CheckExistRaceEvents(historyWriteEventsIdxMap[memType],
                                 checkFuncIter->second, curEventIdx, raceMemEventsIdx);
            if (opType == AccessType::WRITE) {
                // 检查与历史读事件的冲突
                CheckExistRaceEvents(historyReadEventsIdxMap[memType],
                                     checkFuncIter->second, curEventIdx, raceMemEventsIdx);
                historyWriteEventsIdxMap[memType].insert(curEventIdx);
            } else {
                historyReadEventsIdxMap[memType].insert(curEventIdx);
            }
        } else {
            // 结束事件，从集合中移除对应的开始事件
            historyWriteEventsIdxMap[memType].erase(curEventIdx);
            historyReadEventsIdxMap[memType].erase(curEventIdx);
        }
    }
}

void MemEventChecker::RunAlgorithm()
{
    MemEventChecker::RaceMemEventsIdx raceMemEventsIdx;
    // 当前发生的GM内存事件和已经发生的GM内存事件做一一比较，返回存在竞争的事件列表
    ScanlineAlgorithm(raceMemEventsIdx);
    for (const auto &idxInfos : raceMemEventsIdx) {
        auto info = FillRaceDispInfo(events_.at(idxInfos.first.eventIdx),
                                     events_.at(idxInfos.second.eventIdx), idxInfos.first.dynamicErrorIdx,
                                    idxInfos.second.dynamicErrorIdx);
        auto ret = raceSet_.emplace(info);
        if (ret.second) {
            raceCnt_++;
            result_->emplace_back(info);
        }
    }
}

uint32_t MemEventChecker::GetRaceCount() const
{
    return raceCnt_;
}

std::shared_ptr<std::vector<RaceDispInfo>> MemEventChecker::GetResult() const
{
    return result_;
}

bool MemEventChecker::IsMemSpaceOverlap(const MemEvent &op1, const MemEvent &op2,
    EventIdxInfo &idxInfo1, EventIdxInfo &idxInfo2) const
{
    if (!op1.isDynamic && !op2.isDynamic) {
        return IsMemoryOverlap(op1.memInfo, op2.memInfo);
    } else if (!op1.isDynamic && op2.isDynamic) {
        return IsMemoryOverlap(op1.memInfo, op2.dynamicMemInfo, idxInfo2);
    } else if (op1.isDynamic && !op2.isDynamic) {
        return IsMemoryOverlap(op2.memInfo, op1.dynamicMemInfo, idxInfo1);
    } else {
        return IsMemoryOverlap(op1.dynamicMemInfo, op2.dynamicMemInfo, idxInfo1, idxInfo2);
    }
    return false;
}

bool MemEventChecker::IsAtomicAgainst(const MemEvent& event1, const MemEvent& event2) const
{
    // 均为原子写指令且均非标量写的指令，两者不会发生竞争
    if (!event1.isAtomicMode || !event2.isAtomicMode) {
        return false;
    }

    if (event1.memInfo.opType != AccessType::WRITE || event2.memInfo.opType != AccessType::WRITE) {
        return false;
    }

    if (event1.pipe == PipeType::PIPE_S || event2.pipe == PipeType::PIPE_S) {
        return false;
    }
    return true;
}
}
