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

#include <array>
#include <limits>
#include <set>
#include "core/framework/arch_def.h"
#include "core/framework/config_manager.h"
#include "core/framework/constant.h"
#include "core/framework/event_def.h"
#include "core/framework/kernel_manager.h"
#include "core/framework/record_defs.h"
#include "mem_series.h"
#include "core/framework/utility/log.h"
#include "core/framework/vector_clock.h"

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

// 业务侧规则：(m/16)*(n/16) < 10 时在相邻 mmad 间插 asc_sync_pipe(PIPE_M)，>= 10 时不插
constexpr uint32_t MMAD_FRACTAL_SIZE = 16U;
constexpr uint32_t MMAD_K_ACCUM_MIN_FRACTAL_NUM = 10U;

bool IsLargeMatmulAccess(const MemEvent &event)
{
    const MemOpInfo &op = event.memInfo;
    if (op.instrClass != InstrClass::CUBE_MATMUL) {
        return false;
    }
    return (op.matrixM / MMAD_FRACTAL_SIZE) * (op.matrixN / MMAD_FRACTAL_SIZE) >= MMAD_K_ACCUM_MIN_FRACTAL_NUM;
}

// 大规模矩阵乘沿 K 轴累加时，相邻两次 mmad 对同一 L0C 的写-读顺序由硬件保证，业务侧不插同步，不算竞争
bool IsMatmulAccumOrderedByHardware(const MemEvent &event1, const MemEvent &event2)
{
    uint64_t diff = (event1.pipeSerialNo > event2.pipeSerialNo) ?
        (event1.pipeSerialNo - event2.pipeSerialNo) : (event2.pipeSerialNo - event1.pipeSerialNo);
    // 相邻两条指令的 pipeSerialNo 相差 1
    if (diff != 1U) {
        return false;
    }
    return IsLargeMatmulAccess(event1) && IsLargeMatmulAccess(event2);
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
    uint32_t eventIdx;
    uint32_t isStart;   // 0=end, 1=start；打包成 16B，降低排序内存带宽

    SortMetaData(uint64_t addr, uint32_t eventIdx, bool isStart)
        : addr(addr), eventIdx(eventIdx), isStart(isStart ? 1U : 0U) {}
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
        return lhs.isStart < rhs.isStart;   // 0=end 先于 1=start
    }
    return lhs.addr < rhs.addr;   // 按地址升序
}

// 按 addr 做稳定 LSD radix sort（16 bit × 4 趟）。
// 输入已按“end 在前、start 在后，组内 eventIdx 升序”排列，
// 稳定排序后与 CompareEvent 的 (addr, isStart, eventIdx) 顺序完全一致。
inline void RadixSortByAddr(std::vector<SortMetaData> &data)
{
    const size_t n = data.size();
    if (n < 2U) {
        return;
    }
    constexpr uint32_t kRadixBits = 16U;
    constexpr uint32_t kRadixMask = (1U << kRadixBits) - 1U;
    std::vector<SortMetaData> tmp(n, SortMetaData(0U, 0U, false));
    std::vector<size_t> count((1U << kRadixBits) + 1U);
    for (uint32_t shift = 0U; shift < 64U; shift += kRadixBits) {
        std::fill(count.begin(), count.end(), 0U);
        for (const auto &item : data) {
            ++count[((item.addr >> shift) & kRadixMask) + 1U];
        }
        for (size_t i = 1U; i < count.size(); ++i) {
            count[i] += count[i - 1U];
        }
        for (const auto &item : data) {
            tmp[count[(item.addr >> shift) & kRadixMask]++] = item;
        }
        data.swap(tmp);
    }
}

// 动态内存事件先统一转换为写事件，保证不漏检，后续细粒度判断会过滤掉读读竞争事件
inline AccessType EffOpType(const MemEvent &e) {
    return e.isDynamic ? AccessType::WRITE : e.memInfo.opType;
}

// 事件归属的地址空间(MemType)：动态事件取 dynamicMemInfo
inline uint8_t EffMemType(const MemEvent &e) {
    return static_cast<uint8_t>(e.isDynamic ? e.dynamicMemInfo.memType : e.memInfo.memType);
}

// 以 MemType 为下标的定长数组上界（L1=0 … UNKNOWN=9、SSBUF=10，共 11 个）
constexpr size_t kMemTypeCount = static_cast<size_t>(MemType::SSBUF) + 1;

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
bool MemEventChecker::IsInnerCoreRaceCandidate(const MemEvent &event1, const MemEvent &event2,
    uint32_t pipeIdx1, uint32_t ownTime1, uint32_t pipeIdx2, uint32_t ownTime2) const
{
    // 排序之后已经丢失了event1 event2的先后关系，故均需判断
    const uint32_t a1 = ownTime1;
    const uint32_t a2 = event2.vt.Get(pipeIdx1);
    if (a1 == a2) {
        return false;
    }
    const uint32_t b1 = event1.vt.Get(pipeIdx2);
    const uint32_t b2 = ownTime2;
    if (!VectorClock::IsNotHappensBefore(a1, a2, b1, b2)) {
        return false;
    }

    return !IsAtomicAgainst(event1, event2);
}

bool MemEventChecker::IsInnerCoreRaceEvent(EventIdxInfo &idxInfo1, EventIdxInfo &idxInfo2) const
{
    const auto &event1 = this->events_.at(idxInfo1.eventIdx);
    const auto &event2 = this->events_.at(idxInfo2.eventIdx);
    if (event1.pipe == event2.pipe) {
        // 同一个pipe的无需比较
        return false;
    }
    const uint32_t pipeIdx1 =
        GetPipeIdxByMemEvent<RaceCheckType::SINGLE_BLOCK_CHECK>(event1, this->kernelType_, this->deviceType_);
    const uint32_t pipeIdx2 =
        GetPipeIdxByMemEvent<RaceCheckType::SINGLE_BLOCK_CHECK>(event2, this->kernelType_, this->deviceType_);

    if (!IsInnerCoreRaceCandidate(event1, event2, pipeIdx1, event1.vt.Get(pipeIdx1),
        pipeIdx2, event2.vt.Get(pipeIdx2))) {
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

    // 大规模矩阵乘沿 K 轴累加，业务侧依赖硬件保证写-读顺序，不算竞争
    if (IsMatmulAccumOrderedByHardware(event1, event2)) {
        return false;
    }

    // 环保护：两条指令之间若完整包含一个已闭环的环，则不会竞争。
    // closedRingStartSerialNo 是该事件所属 pipe 上已闭环环的最大起始点（见 RecordParse）。两事件先后不定，
    // 两个方向都要判。用 <= 而非 <：hset 生成的 SET_FLAG 复用其触发访存的 serialNo，该访存正落在起始点上。
    if (event1.serialNo <= event2.closedRingStartSerialNo || event2.serialNo <= event1.closedRingStartSerialNo) {
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

bool MemEventChecker::IsMissDcciEvent(EventIdxInfo &idxInfo1, EventIdxInfo &idxInfo2) const {
    const MemEvent &event1 = this->events_.at(idxInfo1.eventIdx);
    const MemEvent &event2 = this->events_.at(idxInfo2.eventIdx);

    // 同一个block的无需比较
    if (event1.loc.coreId == event2.loc.coreId && event1.loc.blockType == event2.loc.blockType) {
        return false;
    }

    // DCCI 仅适用于 GM，其他地址不需要 DCCI
    if (event1.memInfo.memType != MemType::GM && event2.memInfo.memType != MemType::GM) {
        return false;
    }

    // 两个事件至少要有一个是标量读写
    if (event1.pipe != PipeType::PIPE_S_CAL && event2.pipe != PipeType::PIPE_S_CAL) {
        return false;
    }

    // 两个事件应该是一读一写，分别取写、读事件指针用于后续判断（不修改 events_）
    const MemEvent *pWrite = nullptr;
    const MemEvent *pRead = nullptr;
    if (event1.memInfo.opType == AccessType::READ && event2.memInfo.opType == AccessType::WRITE) {
        pWrite = &event2;
        pRead = &event1;
    } else if (event1.memInfo.opType == AccessType::WRITE && event2.memInfo.opType == AccessType::READ) {
        pWrite = &event1;
        pRead = &event2;
    } else {
        return false;
    }

    uint32_t pipeIdx1 =
        GetPipeIdxByMemEvent<RaceCheckType::CROSS_BLOCK_CHECK>(*pWrite, this->kernelType_, this->deviceType_);
    uint32_t pipeIdx2 =
        GetPipeIdxByMemEvent<RaceCheckType::CROSS_BLOCK_CHECK>(*pRead, this->kernelType_, this->deviceType_);
    // 读发生在写之前则不在本检测场景之内
    if (VectorClock::IsHappensBefore(pRead->vt, pWrite->vt, pipeIdx2, pipeIdx1)) {
        return false;
    }

    // 检查是否所有标量读写都有满足条件的 dcci
    static constexpr uint32_t DCCI_WINDOW_THRESHOLD = 1;
    if ((pWrite->pipe != PipeType::PIPE_S_CAL ||
            (pWrite->memInfo.dcciDistance > 0 && pWrite->memInfo.dcciDistance <= DCCI_WINDOW_THRESHOLD)) &&
        (pRead->pipe != PipeType::PIPE_S_CAL ||
            (pRead->memInfo.dcciDistance > 0 && pRead->memInfo.dcciDistance <= DCCI_WINDOW_THRESHOLD))) {
        return false;
    }

    // 比较内存空间是否存在重叠（按原始 idxInfo 顺序传入，保证 dynamicErrorIdx 与 eventIdx 对齐）
    if (!IsMemSpaceOverlap(event1, event2, idxInfo1, idxInfo2)) {
        return false;
    }
    return true;
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

void MemEventChecker::BuildScanlineMetaData(std::vector<SortMetaData> &eventsMetaData, bool &hasAnyDynamic) const {
    eventsMetaData.reserve(this->events_.size() * 2U);
    // 1. 先放所有 end 端点，再放所有 start 端点。
    //    这样 stable radix sort 按 addr 排完后，同地址自然满足“end 先于 start、组内 eventIdx 升序”。
    hasAnyDynamic = false;
    for (size_t eventIdx = 0; eventIdx < this->events_.size(); ++eventIdx) {
        auto& event = this->events_[eventIdx];
        if (event.isDynamic) { hasAnyDynamic = true; }
        auto maxLength = GetMemMaxLength(event);
        if (maxLength == 0U) { // 地址最大长度为 0，不参与竞争分析
            continue;
        }
        const uint64_t endAddr = event.isDynamic ? event.dynamicMemInfo.maxAddr : event.memInfo.addr + maxLength;
        eventsMetaData.emplace_back(SortMetaData(endAddr, static_cast<uint32_t>(eventIdx), false));
    }
    for (size_t eventIdx = 0; eventIdx < this->events_.size(); ++eventIdx) {
        auto& event = this->events_[eventIdx];
        auto maxLength = GetMemMaxLength(event);
        if (maxLength == 0U) {
            continue;
        }
        const uint64_t startAddr = event.isDynamic ? event.dynamicMemInfo.minAddr : event.memInfo.addr;
        eventsMetaData.emplace_back(SortMetaData(startAddr, static_cast<uint32_t>(eventIdx), true));
    }
    // 2. 按地址升序；稳定 radix 保持上面的同地址 tie-break 顺序
    RadixSortByAddr(eventsMetaData);
}

void MemEventChecker::ScanlineAlgorithm(RaceMemEventsIdx &raceMemEventsIdx, RaceMemEventsIdx &missDcciMemEventsIdx) const {
    // 扫描元数据与快速路径桶按 uint32_t 存事件编号；超出范围时直接拦截，避免截断后给出错误结论。
    if (this->events_.size() > static_cast<size_t>(std::numeric_limits<uint32_t>::max()) + 1U) {
        SAN_ERROR_LOG("racecheck event count %zu exceeds uint32_t index range, skip race detection on this kernel",
            this->events_.size());
        return;
    }
    std::vector<SortMetaData> eventsMetaData;
    bool hasAnyDynamic = false;
    BuildScanlineMetaData(eventsMetaData, hasAnyDynamic);

    auto checkFuncIter = checkTypeMap_.find(this->checkType_);
    if (checkFuncIter == checkTypeMap_.end()) {
        SAN_WARN_LOG("RaceCheckType %u is NOT found in checkTypeMap", static_cast<uint8_t>(this->checkType_));
        return;
    }
    const ScanlineContext ctx{checkFuncIter->second, ConfigManager::Instance().Get().checkDcci, this->checkType_};

    // 3.遍历事件，做地址重叠和读写判断
    // ==== 分桶扫描 ====
    // 各竞争判定函数的第一条过滤条件决定"哪些历史事件一定返回 false"，据此分桶跳过：
    //   SINGLE_PIPE : 需 同 pipe + |ΔpipeSerialNo|<=16 → 桶键(memType,pipe)，桶内按 pipeSerialNo 取窗口
    //   SINGLE_BLOCK: 需 异 pipe                       → 桶键(memType,pipe)，只比其它桶
    //   CROSS_BLOCK : 需 异 (coreId,blockType)         → 走通用 all-pairs 逻辑
    // DCCI 模式 / CROSS_NPU / CROSS_BLOCK / 含动态事件 保持原通用逻辑
    if (ctx.checkDcci || ctx.checkType == RaceCheckType::CROSS_NPU_CHECK ||
        ctx.checkType == RaceCheckType::CROSS_BLOCK_CHECK || hasAnyDynamic) {
        ScanGeneric(eventsMetaData, ctx, raceMemEventsIdx, missDcciMemEventsIdx);
    } else if (ctx.checkType == RaceCheckType::SINGLE_PIPE_CHECK) {
        ScanSinglePipeWindow(eventsMetaData, ctx, raceMemEventsIdx);
    } else if (ctx.checkType == RaceCheckType::SINGLE_BLOCK_CHECK) {
        ScanSingleBlockFast(eventsMetaData, ctx, raceMemEventsIdx);
    } else {
        SAN_ERROR_LOG("Unexpected RaceCheckType %u in scanline dispatch", static_cast<uint8_t>(ctx.checkType));
    }
}

// 通用路径：按地址空间(MemType)维护"活跃历史读/写事件"集合，逐端点与历史事件做完整判定。
void MemEventChecker::ScanGeneric(const std::vector<SortMetaData> &eventsMetaData, const ScanlineContext &ctx,
    RaceMemEventsIdx &raceMemEventsIdx, RaceMemEventsIdx &missDcciMemEventsIdx) const {
    /// key：MemType为地址空间，value：历史读写事件，这里用eventIdx存储，以便节省内存
    /// 目的是为了节省cube核检测的耗时，cube中的L1/L0A/L0B/L0C地址空间按照MemType拆分后，后续的历史事件比对
    /// 只需要比对相同地址空间的地址是否存在交叉，提升程序性能；
    std::array<std::unordered_set<uint64_t>, kMemTypeCount> historyWriteEventsIdxMap;
    std::array<std::unordered_set<uint64_t>, kMemTypeCount> historyReadEventsIdxMap;
    for (const auto &metaData : eventsMetaData) {
        const uint64_t curEventIdx = metaData.eventIdx;
        const auto &curEvent = this->events_[curEventIdx];
        const auto opType = EffOpType(curEvent);
        const uint8_t memType = EffMemType(curEvent);
        if (metaData.isStart) {
            if (ctx.checkDcci) {
                CheckExistRaceEvents(historyWriteEventsIdxMap[memType], &MemEventChecker::IsMissDcciEvent,
                                     curEventIdx, missDcciMemEventsIdx);
            } else {
                // 检查与历史写事件的冲突
                CheckExistRaceEvents(historyWriteEventsIdxMap[memType], ctx.checkFunc, curEventIdx, raceMemEventsIdx);
            }
            if (opType == AccessType::WRITE) {
                if (ctx.checkDcci) {
                    CheckExistRaceEvents(historyReadEventsIdxMap[memType], &MemEventChecker::IsMissDcciEvent,
                                         curEventIdx, missDcciMemEventsIdx);
                } else {
                    // 检查与历史读事件的冲突
                    CheckExistRaceEvents(historyReadEventsIdxMap[memType], ctx.checkFunc, curEventIdx, raceMemEventsIdx);
                }
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

// 单 pipe 竞争：只与"同 pipe 且 |pipeSerialNo 差| <= WINDOW"的历史事件比较。
void MemEventChecker::ScanSinglePipeWindow(const std::vector<SortMetaData> &eventsMetaData, const ScanlineContext &ctx,
    RaceMemEventsIdx &raceMemEventsIdx) const {
    using OrderedSet = std::set<std::pair<uint64_t, uint64_t>>;   // (pipeSerialNo, eventIdx)
    // KV: 第一下标 = MemType(地址空间)；inner map 的 key = pipe；
    //     value = set<(pipeSerialNo, eventIdx)>，按 pipeSerialNo 有序，便于按 serialNo 取 ±WINDOW 窗口
    std::array<std::unordered_map<uint32_t, OrderedSet>, kMemTypeCount> wSet;
    std::array<std::unordered_map<uint32_t, OrderedSet>, kMemTypeCount> rSet;
    // 与 IsSinglePipeRaceEvent 的 MAX_SERIAL_NUMBER_DIFF 一致：只比较 pipeSerialNo 间隔 <= 16 的历史事件
    // （一条流水内指令队列长度上限为 16 或 32，间隔 16 以上时判定为不构成流水内竞争，无需比较）
    constexpr uint64_t WINDOW = 16;
    for (const auto &metaData : eventsMetaData) {
        const uint64_t curIdx = metaData.eventIdx;
        const auto &ce = this->events_.at(curIdx);
        const uint8_t mt = EffMemType(ce);
        const uint32_t sub = static_cast<uint32_t>(ce.pipe);
        const uint64_t s = ce.pipeSerialNo;
        if (metaData.isStart) {
            OrderedSet &ws = wSet[mt][sub];
            OrderedSet &rs = rSet[mt][sub];
            const uint64_t lo = s > WINDOW ? s - WINDOW : 0;
            const uint64_t hi = s + WINDOW;
            EventIdxInfo i1{};
            i1.eventIdx = curIdx;
            EventIdxInfo i2{};
            // 读-读不构成竞争：仅当当前为写时才需要与历史读比较（与通用路径一致）
            const bool curWrite = (EffOpType(ce) == AccessType::WRITE);
            const int passes = curWrite ? 2 : 1;
            for (int pass = 0; pass < passes; ++pass) {
                OrderedSet &set = (pass == 0) ? ws : rs;
                for (auto it = set.lower_bound({lo, 0}); it != set.end() && it->first <= hi; ++it) {
                    i2.eventIdx = it->second;
                    if ((this->*ctx.checkFunc)(i1, i2)) { raceMemEventsIdx.emplace_back(i1, i2); }
                }
            }
            if (curWrite) { ws.insert({s, curIdx}); } else { rs.insert({s, curIdx}); }
        } else {
            wSet[mt][sub].erase({s, curIdx});
            rSet[mt][sub].erase({s, curIdx});
        }
    }
}

namespace {

// 超过该成员数的桶只维护活跃列表，不建 Fenwick。
constexpr uint32_t kHugeBucketMemberLimit = 256U * 1024U;
// 活跃成员较少时，二分开销不划算，直接枚举活跃成员。
constexpr uint32_t kSmallActiveLimit = 64U;
// 事件不属于任何桶（长度为 0 的事件不参与竞争检查）。
constexpr int32_t kNoBucket = -1;

// SINGLE_BLOCK 扫描的桶索引。
// 桶键为 (memType, pipeSlot, rw)，桶内成员按事件索引升序排列；
// 普通桶用扁平 Fenwick 维护活跃位置，超大桶改用活跃事件列表。
class SingleBlockBuckets {
public:
    SingleBlockBuckets(
        const std::vector<MemEvent> &events, const std::vector<uint32_t> &eventPipeSlot, uint32_t pipeCount)
        : pipeCount_(pipeCount), bucketCount_(static_cast<uint32_t>(kMemTypeCount) * pipeCount * 2U),
          members_(bucketCount_), bucketOfEvent_(events.size(), kNoBucket), posInBucket_(events.size(), 0U),
          isHugeBucket_(bucketCount_, 0U), hugeActive_(bucketCount_) {
        BuildMembers(events, eventPipeSlot);
        MarkHugeBuckets();
        BuildFenwickOffsets();
    }

    uint32_t BucketIndex(uint32_t memType, uint32_t pipeSlot, bool write) const {
        return ((memType * pipeCount_) + pipeSlot) * 2U + (write ? 0U : 1U);
    }

    uint32_t MemberCount(uint32_t bucket) const { return static_cast<uint32_t>(members_[bucket].size()); }

    const std::vector<uint32_t> &Members(uint32_t bucket) const { return members_[bucket]; }

    bool IsHuge(uint32_t bucket) const { return isHugeBucket_[bucket] != 0U; }

    uint32_t ActiveCount(uint32_t bucket) const {
        return IsHuge(bucket) ? static_cast<uint32_t>(hugeActive_[bucket].size()) : activeCount_[bucket];
    }

    const std::vector<uint32_t> &HugeActive(uint32_t bucket) const { return hugeActive_[bucket]; }

    void Activate(uint32_t eventIdx) {
        const int32_t bucket = bucketOfEvent_[eventIdx];
        if (bucket == kNoBucket) {
            return;
        }
        const uint32_t validBucket = static_cast<uint32_t>(bucket);
        if (IsHuge(validBucket)) {
            auto &active = hugeActive_[validBucket];
            posInBucket_[eventIdx] = static_cast<uint32_t>(active.size());
            active.push_back(eventIdx);
            return;
        }
        FenwickAdd(validBucket, posInBucket_[eventIdx], +1);
    }

    void Deactivate(uint32_t eventIdx) {
        const int32_t bucket = bucketOfEvent_[eventIdx];
        if (bucket == kNoBucket) {
            return;
        }
        const uint32_t validBucket = static_cast<uint32_t>(bucket);
        if (IsHuge(validBucket)) {
            auto &active = hugeActive_[validBucket];
            const uint32_t pos = posInBucket_[eventIdx];
            const uint32_t moved = active.back();
            active[pos] = moved;
            posInBucket_[moved] = pos;
            active.pop_back();
            return;
        }
        FenwickAdd(validBucket, posInBucket_[eventIdx], -1);
    }

    // 返回桶内 [0, pos) 区间的活跃成员数。
    uint32_t ActiveCountBefore(uint32_t bucket, uint32_t pos) const {
        uint32_t count = 0U;
        for (uint32_t i = pos; i > 0U; i -= LowBit(i)) {
            count += static_cast<uint32_t>(fenwick_[fenwickOffset_[bucket] + i]);
        }
        return count;
    }

    // 返回桶内第 rank 个（1-based）活跃成员的桶内位置（0-based）。
    uint32_t SelectActive(uint32_t bucket, uint32_t rank) const {
        const uint32_t memberCount = MemberCount(bucket);
        uint32_t pos = 0U;
        uint32_t bit = 1U;
        while ((bit << 1U) <= memberCount) {
            bit <<= 1U;
        }
        for (; bit != 0U; bit >>= 1U) {
            const uint32_t next = pos + bit;
            if (next <= memberCount) {
                const uint32_t val = static_cast<uint32_t>(fenwick_[fenwickOffset_[bucket] + next]);
                if (val < rank) {
                    pos = next;
                    rank -= val;
                }
            }
        }
        return pos;
    }

private:
    static uint32_t LowBit(uint32_t value) { return value & (~value + 1U); }

    void BuildMembers(const std::vector<MemEvent> &events, const std::vector<uint32_t> &eventPipeSlot) {
        for (size_t i = 0; i < events.size(); ++i) {
            if (GetMemMaxLength(events[i]) == 0U) {
                continue;
            }
            const uint32_t bucket =
                BucketIndex(EffMemType(events[i]), eventPipeSlot[i], EffOpType(events[i]) == AccessType::WRITE);
            posInBucket_[i] = static_cast<uint32_t>(members_[bucket].size());
            members_[bucket].push_back(static_cast<uint32_t>(i));
            bucketOfEvent_[i] = static_cast<int32_t>(bucket);
        }
    }

    void MarkHugeBuckets() {
        for (uint32_t bucket = 0U; bucket < bucketCount_; ++bucket) {
            if (MemberCount(bucket) > kHugeBucketMemberLimit) {
                isHugeBucket_[bucket] = 1U;
            }
        }
    }

    void BuildFenwickOffsets() {
        fenwickOffset_.resize(bucketCount_ + 1U, 0U);
        for (uint32_t bucket = 0U; bucket < bucketCount_; ++bucket) {
            const uint32_t segmentSize = IsHuge(bucket) ? 1U : MemberCount(bucket) + 1U;
            fenwickOffset_[bucket + 1U] = fenwickOffset_[bucket] + segmentSize;
        }
        fenwick_.assign(fenwickOffset_[bucketCount_], 0);
        activeCount_.assign(bucketCount_, 0U);
    }

    void FenwickAdd(uint32_t bucket, uint32_t pos, int32_t delta) {
        const uint32_t memberCount = MemberCount(bucket);
        for (uint32_t i = pos + 1U; i <= memberCount; i += LowBit(i)) {
            fenwick_[fenwickOffset_[bucket] + i] += delta;
        }
        activeCount_[bucket] = static_cast<uint32_t>(static_cast<int32_t>(activeCount_[bucket]) + delta);
    }

    uint32_t pipeCount_;     // 压缩后的 pipe 槽位数
    uint32_t bucketCount_;   // 桶总数 = memType 数 × pipeCount_ × 2（读/写）
    // 桶 -> 桶内成员的事件索引，按事件索引升序排列
    std::vector<std::vector<uint32_t>> members_;
    // 事件 -> 所属桶；kNoBucket 表示长度为 0、不参与竞争检查
    std::vector<int32_t> bucketOfEvent_;
    // 事件 -> 在所属桶内的位置，供 Fenwick 或活跃列表 O(1) 删除
    std::vector<uint32_t> posInBucket_;
    // 桶 -> 是否为超大桶；超大桶不建 Fenwick，改用 hugeActive_
    std::vector<uint8_t> isHugeBucket_;
    // 超大桶 -> 当前活跃事件索引列表
    std::vector<std::vector<uint32_t>> hugeActive_;
    // 扁平 Fenwick：桶 b 的树占用 [fenwickOffset_[b], fenwickOffset_[b+1])，内部 1-based
    std::vector<uint32_t> fenwickOffset_;
    // 所有普通桶的 Fenwick 树按桶拼接；1 表示对应位置的成员活跃
    std::vector<int32_t> fenwick_;
    // 普通桶 -> 当前活跃成员数
    std::vector<uint32_t> activeCount_;
};

} // namespace

// ==== SINGLE_BLOCK 快速路径（向量时钟单调性剪枝） ====
// 桶键按完整 pipe 索引划分（blockIdx*NPIPE + pipe，MIX 展开时含子核偏移），
// 记 e1 为当前事件、e2 为桶内历史事件；p1 为当前事件所在 pipe，p2 为历史事件所在 pipe。
// e1p1/e1p2 分别为 e1 在 p1/p2 上的 vector time，e2p1/e2p2 分别为 e2 在 p1/p2 上的 vector time。
// 同一桶内 histPipe 恒定，同一 pipe 上的向量时钟分量单调不减，因此 e2p1、e2p2 都随事件时间单调不减；
// 当前事件固定时 e1p1、e1p2 为常量。并发条件
//   (e1p1 < e2p1 && e1p2 > e2p2) || (e1p1 > e2p1 && e1p2 < e2p2)
// 中，e2p2 <= e1p2 表示当前事件已看到历史事件（e2 HB e1），前缀可整段跳过；
// 剩余 e2p2 > e1p2 的候选中，只有 e2p1 < e1p1（历史事件尚未看到当前事件）才可能并发。
// 因此只需二分两个边界：
//   firstHistNotSeenByCur = first(e2p2 > e1p2)
//   firstHistSeesCur      = first(e2p1 >= e1p1)
// 只枚举 [firstHistNotSeenByCur, firstHistSeesCur) 内的活跃事件，避免逐对查询向量时钟。
// （e2p1 > e1p1 && e2p2 < e1p2 对应互相已知的因果环，精确向量时钟下恒空，不再保留。）
//
// 数据布局：
//  - 每个事件归属唯一的 (memType, pipeSlot, rw) 桶；桶内成员按事件索引升序静态排列；
//  - 普通桶用扁平 Fenwick 维护活跃计数，支持 O(log m) 增删和第 k 个活跃成员定位；
//  - 超大桶只维护活跃事件列表，避免为少量查询维护 Fenwick；
//  - 预计算稠密 clockTable[event][pipeSlot]，把向量时钟查询从稀疏二分降为连续内存访问。
void MemEventChecker::ScanSingleBlockFast(const std::vector<SortMetaData> &eventsMetaData, const ScanlineContext &ctx,
    RaceMemEventsIdx &raceMemEventsIdx) const {
    (void)ctx; // 判定已内联，不使用 ctx.checkFunc
    const size_t eventCount = this->events_.size();
    if (eventCount == 0U) {
        return;
    }

    std::vector<uint32_t> pipeIdx(eventCount);
    std::vector<uint32_t> ownTime(eventCount);
    for (size_t i = 0; i < eventCount; ++i) {
        const uint32_t pIdx = GetPipeIdxByMemEvent<RaceCheckType::SINGLE_BLOCK_CHECK>(
            this->events_[i], this->kernelType_, this->deviceType_);
        pipeIdx[i] = pIdx;
        ownTime[i] = this->events_[i].vt.Get(pIdx);
    }

    // 把稀疏 pipe 索引压缩成稠密槽位；同一槽位内 pipe 索引恒定。
    // 展开 blockDim 时桶键带子核偏移，保证同一桶内 histPipe 恒定；
    // SINGLE_BLOCK 下不同核心的事件不会进入同一个 checker。
    std::vector<uint32_t> uniquePipes(pipeIdx);
    std::sort(uniquePipes.begin(), uniquePipes.end());
    uniquePipes.erase(std::unique(uniquePipes.begin(), uniquePipes.end()), uniquePipes.end());
    const uint32_t pipeCount = static_cast<uint32_t>(uniquePipes.size());
    std::vector<uint32_t> pipeSlotOfEvent(eventCount);
    for (size_t i = 0; i < eventCount; ++i) {
        pipeSlotOfEvent[i] = static_cast<uint32_t>(
            std::lower_bound(uniquePipes.begin(), uniquePipes.end(), pipeIdx[i]) - uniquePipes.begin());
    }

    // 稠密向量时钟表：事件数 × pipe 数；按字节数限制，超过上限时退回稀疏查询，避免异常 trace 撑爆内存。
    constexpr uint64_t kMaxDenseClockBytes = 256ull * 1024ull * 1024ull;
    const uint64_t clockEntryCount = static_cast<uint64_t>(eventCount) * pipeCount;
    std::vector<uint32_t> clockTable;
    if (clockEntryCount <= kMaxDenseClockBytes / sizeof(uint32_t)) {
        clockTable.resize(eventCount * pipeCount);
        for (size_t i = 0; i < eventCount; ++i) {
            for (uint32_t slot = 0U; slot < pipeCount; ++slot) {
                clockTable[i * pipeCount + slot] = this->events_[i].vt.Get(uniquePipes[slot]);
            }
        }
    }
    auto clockAt = [&](uint32_t eventIdx, uint32_t pipeSlot) -> uint32_t {
        if (clockTable.empty()) {
            return this->events_[eventIdx].vt.Get(uniquePipes[pipeSlot]);
        }
        return clockTable[static_cast<size_t>(eventIdx) * pipeCount + pipeSlot];
    };

    SingleBlockBuckets buckets(this->events_, pipeSlotOfEvent, pipeCount);
    const MemEvent *eventData = this->events_.data();
    for (const auto &metaData : eventsMetaData) {
        const uint32_t curIdx = metaData.eventIdx;
        if (!metaData.isStart) {
            buckets.Deactivate(curIdx);
            continue;
        }

        const MemEvent &curEvent = eventData[curIdx];
        const uint32_t curSlot = pipeSlotOfEvent[curIdx];
        const uint32_t e1p1 = ownTime[curIdx]; // e1 在 p1 上的 vector time
        const uint8_t curMemType = EffMemType(curEvent);
        const bool curWrite = (EffOpType(curEvent) == AccessType::WRITE);
        EventIdxInfo curInfo{};
        curInfo.eventIdx = curIdx;
        EventIdxInfo histInfo{};
        auto emitRace = [&](uint32_t histIdx) {
            const MemEvent &histEvent = eventData[histIdx];
            if (IsAtomicAgainst(curEvent, histEvent)) {
                return;
            }
            histInfo.eventIdx = histIdx;
            if (IsMemSpaceOverlap(curEvent, histEvent, curInfo, histInfo)) {
                raceMemEventsIdx.emplace_back(curInfo, histInfo);
            }
        };

        // 当前事件为写时，需要同时比较历史写和历史读；当前为读时只比较历史写。
        const uint32_t comparePassCount = curWrite ? 2U : 1U;
        for (uint32_t comparePass = 0U; comparePass < comparePassCount; ++comparePass) {
            const bool historyIsWrite = (comparePass == 0U);
            for (uint32_t histSlot = 0U; histSlot < pipeCount; ++histSlot) {
                if (histSlot == curSlot) {
                    continue;
                }
                const uint32_t bucket = buckets.BucketIndex(curMemType, histSlot, historyIsWrite);
                const uint32_t memberCount = buckets.MemberCount(bucket);
                const uint32_t activeCount = buckets.ActiveCount(bucket);
                if (memberCount == 0U || activeCount == 0U) {
                    continue;
                }
                const uint32_t e1p2 = clockAt(curIdx, histSlot); // e1 在 p2 上的 vector time
                auto checkClockConcurrent = [&](uint32_t histIdx) {
                    const uint32_t e2p1 = clockAt(histIdx, curSlot); // e2 在 p1 上的 vector time
                    if (e1p1 == e2p1) {
                        return;
                    }
                    const uint32_t e2p2 = clockAt(histIdx, histSlot); // e2 在 p2 上的 vector time
                    if (!VectorClock::IsNotHappensBefore(e1p1, e2p1, e1p2, e2p2)) {
                        return;
                    }
                    emitRace(histIdx);
                };

                if (buckets.IsHuge(bucket)) {
                    for (uint32_t histIdx : buckets.HugeActive(bucket)) {
                        checkClockConcurrent(histIdx);
                    }
                    continue;
                }

                const std::vector<uint32_t> &bucketMembers = buckets.Members(bucket);
                if (activeCount <= kSmallActiveLimit) {
                    for (uint32_t rank = 1U; rank <= activeCount; ++rank) {
                        checkClockConcurrent(bucketMembers[buckets.SelectActive(bucket, rank)]);
                    }
                    continue;
                }

                // 桶内成员的 e2p2、e2p1 均随事件索引单调不减，可二分出候选区间。
                auto findFirstClockAtLeast = [&](uint32_t clockSlot, uint32_t minValue) -> uint32_t {
                    uint32_t lo = 0U;
                    uint32_t hi = memberCount;
                    while (lo < hi) {
                        const uint32_t mid = lo + ((hi - lo) >> 1U);
                        if (clockAt(bucketMembers[mid], clockSlot) >= minValue) {
                            hi = mid;
                        } else {
                            lo = mid + 1U;
                        }
                    }
                    return lo;
                };

                // e2p2 <= e1p2 是 HB 前缀，整体跳过；等值段也要跳过，得到第一个 e2p2 > e1p2。
                const uint32_t firstHistOwnTimeAtLeastCur = findFirstClockAtLeast(histSlot, e1p2);
                uint32_t firstHistNotSeenByCur = firstHistOwnTimeAtLeastCur;
                while (firstHistNotSeenByCur < memberCount &&
                    clockAt(bucketMembers[firstHistNotSeenByCur], histSlot) == e1p2) {
                    ++firstHistNotSeenByCur;
                }
                // e2p1 >= e1p1 表示历史事件已看到当前事件，从这里开始不再是候选。
                const uint32_t firstHistSeesCur = findFirstClockAtLeast(curSlot, e1p1);
                if (firstHistNotSeenByCur >= firstHistSeesCur) {
                    continue;
                }

                const uint32_t activeBefore = buckets.ActiveCountBefore(bucket, firstHistNotSeenByCur);
                const uint32_t activeInRange = buckets.ActiveCountBefore(bucket, firstHistSeesCur) - activeBefore;
                for (uint32_t rank = 1U; rank <= activeInRange; ++rank) {
                    emitRace(bucketMembers[buckets.SelectActive(bucket, activeBefore + rank)]);
                }
            }
        }
        buckets.Activate(curIdx);
    }
}

void MemEventChecker::RunAlgorithm()
{
    MemEventChecker::RaceMemEventsIdx raceMemEventsIdx;
    MemEventChecker::RaceMemEventsIdx missDcciMemEventsIdx;
    // 当前发生的GM内存事件和已经发生的GM内存事件做一一比较，返回存在竞争的事件列表
    ScanlineAlgorithm(raceMemEventsIdx, missDcciMemEventsIdx);
    for (const auto &idxInfos : raceMemEventsIdx) {
        auto info = FillRaceDispInfo(false, events_.at(idxInfos.first.eventIdx),
            events_.at(idxInfos.second.eventIdx), idxInfos.first.dynamicErrorIdx,
            idxInfos.second.dynamicErrorIdx);
        auto ret = raceSet_.emplace(info);
        if (ret.second) {
            raceCnt_++;
            result_->emplace_back(info);
        }
    }

    for (const auto &idxInfos : missDcciMemEventsIdx) {
        auto info = FillRaceDispInfo(true, events_.at(idxInfos.first.eventIdx),
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
    // 涉及原子操作的两种情况，不会产生竞争：
    // 1. set_atomic_xxx和set_atomic_none之间的指令，isAtomicMode均为true且均非标量写
    // 2. SCALAR_RED和SCALAR_ATOM，本身自带原子性，被建模为READ+WRITE后，子事件仍应保持原子性
    auto isAtomicEvent = [](const MemEvent &event) {
        return event.memInfo.isScalarAtomic ||
            (event.isAtomicMode && event.memInfo.opType == AccessType::WRITE &&
            event.pipe != PipeType::PIPE_S);
    };
    return isAtomicEvent(event1) && isAtomicEvent(event2);
}
}
