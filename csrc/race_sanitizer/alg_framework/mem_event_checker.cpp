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

// 以 MemType 为下标的定长数组上界（L1=0 … INVALID=9，共 10 个）
constexpr size_t kMemTypeCount = static_cast<size_t>(MemType::INVALID) + 1;

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

void MemEventChecker::ScanSingleBlockFast(const std::vector<SortMetaData> &eventsMetaData, const ScanlineContext &ctx,
    RaceMemEventsIdx &raceMemEventsIdx) const {
    (void)ctx;   // 判定已内联，不使用 ctx.checkFunc
    // ==== SINGLE_BLOCK 快速路径 ====
    // 把 IsInnerCoreRaceEvent 内联进扫描循环：
    //  - 预计算每个事件的 VC pipe 索引 pipeIdx 与"自时钟分量" ownTime=vt.Get(ownPipe)，
    //    使 IsNotHappensBefore 的 4 次 Get 降为 2 次（a1=ownTime[cur]、b2=ownTime[hist]）；
    //  - (memType,pipe) 桶用连续 std::vector 存活跃事件索引 + O(1) swap-and-pop 删除，
    //    取代 unordered_set 的逐节点遍历（消除 1.6 亿次指针追逐）；
    //  - 用 events_.data() 裸指针 + operator[] 取代 std::vector::at 与成员函数指针间接调用。
    // KV: wB/rB 下标 = [memType][pipe]；value = 该桶内"已开始未结束"的历史写/读事件 idx 的连续 vector
    //     posInBucket[i] = 事件 i 在其当前所属桶 vector 中的下标（供 O(1) swap-and-pop 删除）
    constexpr uint32_t NPIPE = static_cast<uint32_t>(PipeType::SIZE);
    const size_t nEvents = this->events_.size();
    std::vector<uint32_t> pipeIdx(nEvents);
    std::vector<uint32_t> ownTime(nEvents);
    for (size_t i = 0; i < nEvents; ++i) {
        const uint32_t pIdx = GetPipeIdxByMemEvent<RaceCheckType::SINGLE_BLOCK_CHECK>(
            this->events_[i], this->kernelType_, this->deviceType_);
        pipeIdx[i] = pIdx;
        ownTime[i] = this->events_[i].vt.Get(pIdx);
    }
    std::array<std::array<std::vector<uint64_t>, NPIPE>, kMemTypeCount> wB;
    std::array<std::array<std::vector<uint64_t>, NPIPE>, kMemTypeCount> rB;
    std::vector<uint32_t> posInBucket(nEvents);
    const MemEvent *evData = this->events_.data();
    for (const auto &metaData : eventsMetaData) {
        const uint64_t curIdx = metaData.eventIdx;
        const MemEvent &ce = evData[curIdx];
        const uint8_t mt = EffMemType(ce);
        const uint32_t sub = static_cast<uint32_t>(ce.pipe);
        const bool curWrite = (EffOpType(ce) == AccessType::WRITE);
        if (!metaData.isStart) {
            // 结束端点：从所属桶 O(1) 删除（把桶尾元素搬到删除位，再 pop_back）
            auto &bucket = (curWrite ? wB[mt][sub] : rB[mt][sub]);
            const uint32_t pos = posInBucket[curIdx];
            const uint64_t moved = bucket.back();
            bucket[pos] = moved;
            posInBucket[moved] = pos;
            bucket.pop_back();
            continue;
        }
        // 开始端点：只与"异 pipe"的历史事件比较（同 pipe 不可能构成核内竞争）
        const uint32_t t1Pipe = pipeIdx[curIdx];
        const uint32_t a1 = ownTime[curIdx];
        const int passes = curWrite ? 2 : 1;
        for (int pass = 0; pass < passes; ++pass) {
            auto &slots = (pass == 0) ? wB[mt] : rB[mt];
            for (uint32_t q = 0; q < NPIPE; ++q) {
                if (q == sub) { continue; }
                auto &bucket = slots[q];
                if (bucket.empty()) { continue; }
                for (uint64_t hIdx : bucket) {
                    const MemEvent &he = evData[hIdx];
                    if (!IsInnerCoreRaceCandidate(ce, he, t1Pipe, a1, pipeIdx[hIdx], ownTime[hIdx])) {
                        continue;
                    }
                    EventIdxInfo i1{};
                    i1.eventIdx = curIdx;
                    EventIdxInfo i2{};
                    i2.eventIdx = hIdx;
                    if (IsMemSpaceOverlap(ce, he, i1, i2)) {
                        raceMemEventsIdx.emplace_back(i1, i2);
                    }
                }
            }
        }
        auto &bucket = (curWrite ? wB[mt][sub] : rB[mt][sub]);
        posInBucket[curIdx] = static_cast<uint32_t>(bucket.size());
        bucket.push_back(curIdx);
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
