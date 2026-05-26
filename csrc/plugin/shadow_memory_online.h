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

#ifndef SHADOW_MEMORY_ONLINE_H
#define SHADOW_MEMORY_ONLINE_H

#include "kernel_pub_func.h"
#include "parse_record.h"

namespace Sanitizer {

using namespace OnlineShadowMemory;

#if (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(SIMT_MODE)) || defined(__BUILD_TESTS__)

enum class ErrorCode : uint64_t {
    // for MultiLayerTable
    UNALLOCATABLE_FOR_L0 = 0x100U,
    INVALID_L0_IDX_ADDR,
    INVALID_L1_START_ADDR,
    UNALLOCATABLE_FOR_L1,
    INVALID_L1_IDX_ADDR,
    WAIT_FOR_L0_UNLOCK_TIMEOUT,
    WAIT_FOR_L1_UNLOCK_TIMEOUT,

    // for ShadowMemoryHeapAllocator
    ALLOCATOR_NOT_READY = 0x200,
    ALLOCATOR_REMAINING_MEM_NOT_ENOUGH,
    ALLOCATED_ADDR_ILLEGAL,

};

enum class SyncState : uint8_t {
    IMPOSSIBLE_RACE = 0,           // 当前内存状态不可能存在竞争
    POSSIBLE_RACE,                 // 有存在竞争的可能
};

AICORE_FUNC_HEAD OnlineMemoryType SpaceToOnlineMemory(AddressSpace space)
{
    if (space == AddressSpace::GM) {
        return OnlineMemoryType::GM;
    } else {
        return OnlineMemoryType::UB;
    }
}

class ShadowMemoryHeapAllocator {
public:
    AICORE_FUNC_HEAD __attribute__((always_inline)) ShadowMemoryHeapAllocator(): heapHeadPtr_(nullptr), isReady_(false) {}

    AICORE_FUNC_HEAD bool Init(uint64_t heapAddr, uint64_t size)
    {
        if (heapAddr == 0U || (size == 0U)) {
            return false;
        }
        // ShadowMemoryHeapHead 已在host完成赋值，此处只需创建head指针
        heapHeadPtr_ = reinterpret_cast<__gm__ ShadowMemoryHeapHead *>(heapAddr);

        if (heapHeadPtr_->size == 0U) {
            return false;
        }

        maxAddr_ = heapHeadPtr_->startAddr + heapHeadPtr_->size;
        isReady_ = true;
        return true;
    }

    AICORE_FUNC_HEAD uint64_t AllocHeap(uint64_t allocSize, uint64_t &addr)
    {
        if (!isReady_) {
            return static_cast<uint64_t>(ErrorCode::ALLOCATOR_NOT_READY);
        }

        if (AtomicAdd(&(heapHeadPtr_->current), 0U) > maxAddr_) {
            // 已无内存可分配
            return static_cast<uint64_t>(ErrorCode::ALLOCATOR_REMAINING_MEM_NOT_ENOUGH);
        }

        addr = AtomicAdd(&(heapHeadPtr_->current), allocSize);

        // 检查是否超过范围
        if (AtomicAdd(&(heapHeadPtr_->current), 0U) > maxAddr_) {
            // 本次不够分配
            addr = static_cast<uint64_t>(OnlineSmAddrStatus::UNALLOCATABLE);
            return static_cast<uint64_t>(ErrorCode::ALLOCATED_ADDR_ILLEGAL);
        }

        return 0U;
    }

    AICORE_FUNC_HEAD uint64_t FreeHeap(uint64_t size)
    {
        // 暂不支持释放内存
        return 0U;
    }

    AICORE_FUNC_HEAD uint64_t GetHeapStartAddr()
    {
        if (!isReady_) {
            return 0U;
        }
        return heapHeadPtr_->startAddr;
    }

    AICORE_FUNC_HEAD uint64_t GetHeapEndAddr()
    {
        if (!isReady_) {
            return 0U;
        }
        return heapHeadPtr_->startAddr + heapHeadPtr_->size;
    }

    AICORE_FUNC_HEAD bool IsHeapFreshNew()
    {
        return (AtomicAdd(&(heapHeadPtr_->startAddr), 0U)) == (AtomicAdd(&(heapHeadPtr_->current), 0U));
    }

private:
    bool isReady_;
    uint64_t maxAddr_;
    __gm__ ShadowMemoryHeapHead *heapHeadPtr_;
};

template <typename ByteStatus_t>
class MemoryByteStatusParser {
public:
    AICORE_FUNC_HEAD static ByteStatus_t Construct(uint8_t memoryStatus, uint16_t threadId, uint64_t pc,
        OnlineMemoryType memoryType = OnlineMemoryType::GM, SyncState syncThreadState = SyncState::POSSIBLE_RACE)
    {
        static_assert(sizeof(ByteStatus_t) >= sizeof(uint64_t),
            "memory byte status model requires not less than 8 bytes length for every single byte");
        return static_cast<ByteStatus_t>(pc << PC_START_BIT) |
            static_cast<ByteStatus_t>((static_cast<uint8_t>(syncThreadState) & SYNC_STATE_MASK) << SYNC_STATE_START_BIT) |
            static_cast<ByteStatus_t>((static_cast<uint8_t>(memoryType) & MEMORY_TYPE_MASK) << MEMORY_TYPE_START_BIT) |
            static_cast<ByteStatus_t>((memoryStatus & MEMORY_STATUS_MASK) << MEMORY_STATUS_START_BIT) |
            static_cast<ByteStatus_t>(threadId & THREAD_ID_MASK);
    }

    AICORE_FUNC_HEAD static MemoryByteStatus ExtractMemoryStatus(ByteStatus_t val)
    {
        return static_cast<MemoryByteStatus>((val >> MEMORY_STATUS_START_BIT) & MEMORY_STATUS_MASK);
    }

    AICORE_FUNC_HEAD static uint16_t ExtractThreadId(ByteStatus_t val)
    {
        return static_cast<uint16_t>(val & THREAD_ID_MASK);
    }

    AICORE_FUNC_HEAD static uint32_t ExtractPc(ByteStatus_t val)
    {
        return static_cast<uint32_t>((val >> PC_START_BIT) & PC_MASK);
    }

    AICORE_FUNC_HEAD static SyncState ExtractSyncStatus(ByteStatus_t val)
    {
        return static_cast<SyncState>((val >> SYNC_STATE_START_BIT) & SYNC_STATE_MASK);
    }

    AICORE_FUNC_HEAD static OnlineMemoryType ExtractMemoryType(ByteStatus_t val)
    {
        return static_cast<OnlineMemoryType>((val >> MEMORY_TYPE_START_BIT) & MEMORY_TYPE_MASK);
    }

    AICORE_FUNC_HEAD static void ResetSyncStatus(__gm__ ByteStatus_t &value,
        uint8_t syncState = static_cast<uint8_t>(SyncState::IMPOSSIBLE_RACE))
    {
        value = (value & ~(1ULL << SYNC_STATE_START_BIT)) |
            (static_cast<ByteStatus_t>((syncState & SYNC_STATE_MASK)) << SYNC_STATE_START_BIT);
    }

    AICORE_FUNC_HEAD static bool StatusIsValid(ByteStatus_t val)
    {
        if ((val == 0x0) || val == static_cast<ByteStatus_t>(OnlineSmAddrStatus::UNALLOCATABLE) ||
            val == static_cast<ByteStatus_t>(OnlineSmAddrStatus::LOCKED_BY_OTHER_THREADS)) {
            return false;
        }
        return true;
    }

private:
    AICORE_FUNC_HEAD MemoryByteStatusParser() {}
};

struct TableLayout {
    uint64_t listPtr;
    uint64_t mask;
    uint64_t blockNum;
    uint64_t blockSize; // block 表达的内存范围大小

    AICORE_FUNC_HEAD TableLayout(): listPtr(0U), mask(0U), blockNum(0U), blockSize(0U) {}
};

template <typename ByteStatus_t>
class MultiLayerTable {
public:
    AICORE_FUNC_HEAD MultiLayerTable(): heapAllocator_()
    {
        l0Tbl_.listPtr = 0U;
        l0Tbl_.mask = ONLINE_GLOBAL_MEM_MASK;
        l0Tbl_.blockSize = ONLINE_LOCAL_MEM_MASK;
        l0Tbl_.blockNum = (ONLINE_GLOBAL_MEM_MASK + l0Tbl_.blockSize - 1U) / l0Tbl_.blockSize;

        l1Tbl_.listPtr = 0U;
        l1Tbl_.mask = ONLINE_LOCAL_MEM_MASK;
        l1Tbl_.blockSize = ONLINE_ONE_SM_STAND_FOR_BYTE;
        l1Tbl_.blockNum = (ONLINE_LOCAL_MEM_MASK + l1Tbl_.blockSize - 1U) / l1Tbl_.blockSize;

        l2Tbl_.listPtr = 0U;
        l2Tbl_.mask = ONLINE_ONE_SM_STAND_FOR_BYTE - 1U;
        l2Tbl_.blockSize = 1;
        l2Tbl_.blockNum = ONLINE_ONE_SM_STAND_FOR_BYTE;
    }

    AICORE_FUNC_HEAD bool Init(uint64_t heapAddr, uint64_t size)
    {
        if (!heapAllocator_.Init(heapAddr, size)) {
            return false;
        }

        // Host已预分配了L0级表空间，每个节点占8字节，节点数：
        // 节点数 = (ONLINE_GLOBAL_MEM_MASK + ONLINE_LOCAL_MEM_MASK - 1U) / ONLINE_LOCAL_MEM_MASK) * sizeof(uint64_t)
        // L0层起始地址，始终与heap起始地址一致
        l0Tbl_.listPtr = heapAllocator_.GetHeapStartAddr();

        return true;
    }

    AICORE_FUNC_HEAD uint64_t LookUp(const uint64_t addr,
        uint64_t &l1StartAddr, uint64_t &l2StartAddr, uint64_t &l2MemStatusAddr)
    {
        uint64_t ret = 0U;
        ret = LookUpInL0(addr, l1StartAddr);
        if (ret != 0U) {
            l2StartAddr = ret; // 承载错误码
            return ret;
        }

        ret = LookUpInL1(addr, l1StartAddr, l2StartAddr);
        if (ret != 0U) {
            l2StartAddr = ret; // 承载错误码
            return ret;
        }

        l2MemStatusAddr = LookUpInL2(addr, l2StartAddr);
        return ret;
    }

private:
    AICORE_FUNC_HEAD uint64_t LookUpInL0(uint64_t addr, uint64_t &l1StartAddr)
    {
        uint64_t ret = 0U;
        uint64_t l0Idx = (addr & l0Tbl_.mask) / l0Tbl_.blockSize;

        __gm__ uint64_t *l0Ptr = reinterpret_cast<__gm__ uint64_t *>(l0Tbl_.listPtr) + l0Idx;

        if (reinterpret_cast<uint64_t>(l0Ptr) < heapAllocator_.GetHeapStartAddr() ||
            reinterpret_cast<uint64_t>(l0Ptr) > heapAllocator_.GetHeapEndAddr()) {
            return static_cast<uint64_t>(ErrorCode::INVALID_L0_IDX_ADDR);
        }
        if (*l0Ptr == static_cast<uint64_t>(OnlineSmAddrStatus::UNALLOCATABLE)) {
            // 内存耗尽
            return static_cast<uint64_t>(ErrorCode::UNALLOCATABLE_FOR_L0);
        }

        uint64_t newHeap = 0U;
        uint64_t lock = AtomicCAS(l0Ptr, 0U, static_cast<uint64_t>(OnlineSmAddrStatus::LOCKED_BY_OTHER_THREADS));
        if (lock == 0) {
            // 获取锁的线程
            ret = heapAllocator_.AllocHeap(l1Tbl_.blockNum * sizeof(uint64_t), newHeap);
            if (ret == 0U) {
                AtomicExch(l0Ptr, newHeap);
            } else {
                AtomicExch(l0Ptr, static_cast<uint64_t>(OnlineSmAddrStatus::UNALLOCATABLE));
            }
        } else {
            // 未获取锁的线程，等待锁释放
            uint64_t count = 0;
            constexpr uint64_t max_count = 2000;
            while ((lock == static_cast<uint64_t>(OnlineSmAddrStatus::LOCKED_BY_OTHER_THREADS)) &&
                (count < max_count)) {
                lock = AtomicAdd(l0Ptr, 0);
                count++;
            }
        }
        l1StartAddr = AtomicAdd(l0Ptr, 0);
        if (l1StartAddr == static_cast<uint64_t>(OnlineSmAddrStatus::UNALLOCATABLE)) {
            // 无内存可分配
            ret = static_cast<uint64_t>(OnlineSmAddrStatus::UNALLOCATABLE);
        } else if (l1StartAddr == static_cast<uint64_t>(OnlineSmAddrStatus::LOCKED_BY_OTHER_THREADS)) {
            // 状态异常
            ret = static_cast<uint64_t>(OnlineSmAddrStatus::LOCKED_BY_OTHER_THREADS);
        } else if (l1StartAddr == 0) {
            ret = static_cast<uint64_t>(ErrorCode::WAIT_FOR_L0_UNLOCK_TIMEOUT);
        } else {
            // 正常地址
            ret = 0U;
        }

        return ret;
    }

    AICORE_FUNC_HEAD uint64_t LookUpInL1(uint64_t addr, uint64_t l1StartAddr, uint64_t &l2StartAddr)
    {
        uint64_t ret = 0U;
        if (l1StartAddr == 0U) {
            return static_cast<uint64_t>(ErrorCode::INVALID_L1_START_ADDR);
        }
        l1Tbl_.listPtr = l1StartAddr;
        uint64_t l1Idx = (addr & l1Tbl_.mask) / l1Tbl_.blockSize;

        __gm__ uint64_t *l1Ptr = reinterpret_cast<__gm__ uint64_t *>(l1Tbl_.listPtr) + l1Idx;

        if (reinterpret_cast<uint64_t>(l1Ptr) < heapAllocator_.GetHeapStartAddr() ||
            reinterpret_cast<uint64_t>(l1Ptr) > heapAllocator_.GetHeapEndAddr()) {
            return static_cast<uint64_t>(ErrorCode::INVALID_L1_IDX_ADDR);
        }
        if (*l1Ptr == static_cast<uint64_t>(OnlineSmAddrStatus::UNALLOCATABLE)) {
            // 内存耗尽
            return static_cast<uint64_t>(ErrorCode::UNALLOCATABLE_FOR_L1);
        }

        uint64_t newHeap = 0U;
        uint64_t lock = AtomicCAS(l1Ptr, 0U, static_cast<uint64_t>(OnlineSmAddrStatus::LOCKED_BY_OTHER_THREADS));
        if (lock == 0) {
            // 获取锁的线程
            ret = heapAllocator_.AllocHeap(l2Tbl_.blockNum * sizeof(ByteStatus_t), newHeap);
            if (ret == 0U) {
                AtomicExch(l1Ptr, newHeap);
            } else {
                AtomicExch(l1Ptr, static_cast<uint64_t>(OnlineSmAddrStatus::UNALLOCATABLE));
            }
        } else {
            // 未获取锁的线程，等待锁释放
            uint64_t count = 0;
            constexpr uint64_t max_count = 2000;
            while (lock == static_cast<uint64_t>(OnlineSmAddrStatus::LOCKED_BY_OTHER_THREADS) &&
                (count < max_count)) {
                lock = AtomicAdd(l1Ptr, 0);
                count++;
            }
        }

        l2StartAddr = AtomicAdd(l1Ptr, 0);
        if (l2StartAddr == static_cast<uint64_t>(OnlineSmAddrStatus::UNALLOCATABLE)) {
            ret = static_cast<uint64_t>(OnlineSmAddrStatus::UNALLOCATABLE);
        } else if (l2StartAddr == static_cast<uint64_t>(OnlineSmAddrStatus::LOCKED_BY_OTHER_THREADS)) {
            // 异常
            ret = static_cast<uint64_t>(OnlineSmAddrStatus::LOCKED_BY_OTHER_THREADS);
        } else if (l2StartAddr == 0) {
            ret = static_cast<uint64_t>(ErrorCode::WAIT_FOR_L1_UNLOCK_TIMEOUT);
        } else {
            // 正常地址
            ret = 0U;
        }

        return ret;
    }

    AICORE_FUNC_HEAD uint64_t LookUpInL2(uint64_t addr, uint64_t l2StartAddr)
    {
        l2Tbl_.listPtr = l2StartAddr;
        __gm__ ByteStatus_t *l2StartPtr = reinterpret_cast<__gm__ ByteStatus_t *>(l2StartAddr);
        uint64_t l2Idx = (addr & l2Tbl_.mask) / l2Tbl_.blockSize;
        return reinterpret_cast<uint64_t>(l2StartPtr + l2Idx);
    }

    ShadowMemoryHeapAllocator heapAllocator_;

    TableLayout l0Tbl_; // 仅一组，长度固定
    TableLayout l1Tbl_; // 每一个PM对应一组
    TableLayout l2Tbl_; // 单字节模型
};

// simplified shadow memory running in SIMT VF only
class ShadowMemoryOnline {
public:
    using ByteStatus_t = uint64_t;
    using MBSP = MemoryByteStatusParser<ByteStatus_t>;
    struct AuxErrorInfo {
        // 线程间踩踏和线程间竞争使用
        SimtThreadLocation conflictedThreadLoc{};                  // 记录被当前线程所踩踏的线程坐标
        uint64_t nBadBytes{}; // 多核踩踏字节数，竞争不会用
        uint32_t pc{};                                             // 记录被当前线程所踩踏的pc
        KernelErrorType errorType = KernelErrorType::INVALID;      // 错误类型
    };

    static constexpr uint8_t maxErrorNum = 2;
    static constexpr uint8_t overLapErrorIdx = 0;
    static constexpr uint8_t raceErrorIdx = 1;
    // 为了建模方便和节省空间，UB和GM采用了统一的ShadowMemory建模，实际场景下，UB和GM的空间也是隔离开的
    // UB和GM的地址界限，小于该值为UB，大于该值为GM
    static constexpr uint32_t ubGmEps = 256 * 1024;
    struct AuxInfo {
        uint64_t l1StartAddr{}; // L1StartAddr
        uint64_t l2StartAddr{}; // L2StartAddr
        uint64_t l2MemStatusAddr{}; // 字节状态位地址
        AuxErrorInfo errorInfo[maxErrorNum]{};
    };

    AICORE_FUNC_HEAD ShadowMemoryOnline() : tables_{}, isReady_{false} {}

    AICORE_FUNC_HEAD bool Init(uint64_t heapAddr, uint64_t size, __gm__ uint8_t *memInfo, __gm__ uint8_t *memInfoSimt,
        __gm__ uint8_t *memInfoSimd)
    {
        heapAddr_ = heapAddr;
        memInfo_ = memInfo;
        memInfoSimt_ = memInfoSimt;
        memInfoSimd_ = memInfoSimd;
        globalHead_ = reinterpret_cast<__gm__ RecordGlobalHead *>(memInfo_);
        simtBlockHead_ = reinterpret_cast<__gm__ SimtRecordBlockHead *>(memInfoSimt_);
        simdBlockHead_ = reinterpret_cast<__gm__ RecordBlockHead *>(memInfoSimd_);
        memInfoSimtEntry_ = memInfoSimd_ + globalHead_->offsetInfo.simtEntryInfo.offset;
        if (!tables_.Init(heapAddr, size)) {
            return false;
        }
        isReady_ = true;
        return true;
    }

    template <KernelErrorType errorType>
    AICORE_FUNC_HEAD void AssignErrorInfo(ByteStatus_t oldValue, uint16_t threadId, AuxInfo &auxInfo);

    AICORE_FUNC_HEAD void LoadNBytes(AddrInfo const &addrInfo, AuxInfo &auxInfo);

    AICORE_FUNC_HEAD void StoreNBytes(AddrInfo const &addrInfo, AuxInfo &auxInfo);

    AICORE_FUNC_HEAD void ClearSyncThreadState()
    {
        uint64_t l0TblNum = (ONLINE_GLOBAL_MEM_MASK + ONLINE_LOCAL_MEM_MASK - 1U) / ONLINE_LOCAL_MEM_MASK;
        uint64_t l1TblNum = (ONLINE_LOCAL_MEM_MASK + ONLINE_ONE_SM_STAND_FOR_BYTE - 1U) / ONLINE_ONE_SM_STAND_FOR_BYTE;
        __gm__ uint64_t *l0TblPtr = reinterpret_cast<__gm__ uint64_t *>(heapAddr_ + sizeof(ShadowMemoryHeapHead));
        for (size_t l0Idx = 0; l0Idx < l0TblNum; ++l0Idx) {
            uint64_t l0Val = l0TblPtr[l0Idx];
            if (!MemoryByteStatusParser<ByteStatus_t>::StatusIsValid(l0Val)) { continue; }
            auto l1TblPtr = reinterpret_cast<__gm__ uint64_t *>(l0Val);
            for (size_t l1Idx = 0; l1Idx < l1TblNum; ++l1Idx) {
                uint64_t l1Val = l1TblPtr[l1Idx];
                if (!MemoryByteStatusParser<ByteStatus_t>::StatusIsValid(l1Val)) { continue; }
                auto l2TblPtr = reinterpret_cast<__gm__ uint64_t *>(l1Val);
                for (size_t l2Idx = 0; l2Idx < ONLINE_ONE_SM_STAND_FOR_BYTE; ++l2Idx) {
                    uint64_t l2Val = l2TblPtr[l2Idx];
                    if (!MemoryByteStatusParser<ByteStatus_t>::StatusIsValid(l2Val)) { continue; }
                    MemoryByteStatusParser<ByteStatus_t>::ResetSyncStatus(l2TblPtr[l2Idx]);
                }
            }
        }
    }

    AICORE_FUNC_HEAD void CopyShadowMemoryToMemInfo()
    {
        uint64_t l0TblNum = (ONLINE_GLOBAL_MEM_MASK + ONLINE_LOCAL_MEM_MASK - 1U) / ONLINE_LOCAL_MEM_MASK;
        uint64_t l1TblNum = (ONLINE_LOCAL_MEM_MASK + ONLINE_ONE_SM_STAND_FOR_BYTE - 1U) / ONLINE_ONE_SM_STAND_FOR_BYTE;
        __gm__ uint64_t *l0TblPtr = reinterpret_cast<__gm__ uint64_t *>(heapAddr_ + sizeof(ShadowMemoryHeapHead));
        auto memInfoSimtEntryHead = reinterpret_cast<__gm__ SimtEntryBlockHead*>(memInfoSimtEntry_ +
            simdBlockHead_->blockInfo.simtEntryUseSize);
        auto gmEntryRecord = reinterpret_cast<__gm__ SimtEntryRecord*>(memInfoSimtEntryHead + 1);
        SimtEntryBlockHead entryHead{};
        simdBlockHead_->blockInfo.simtEntryUseSize += sizeof(SimtEntryBlockHead);
        uint8_t l1OneBits = CountOneBits(ONLINE_LOCAL_MEM_MASK);
        uint8_t l2OneBits = CountOneBits(ONLINE_ONE_SM_STAND_FOR_BYTE - 1);
        for (size_t l0Idx = 0; l0Idx < l0TblNum; ++l0Idx) {
            uint64_t l0Val = l0TblPtr[l0Idx];
            if (!MemoryByteStatusParser<ByteStatus_t>::StatusIsValid(l0Val)) continue;
            auto l1TblPtr = reinterpret_cast<__gm__ uint64_t *>(l0Val);
            for (size_t l1Idx = 0; l1Idx < l1TblNum; ++l1Idx) {
                uint64_t l1Val = l1TblPtr[l1Idx];
                if (!MemoryByteStatusParser<ByteStatus_t>::StatusIsValid(l1Val)) continue;
                auto l2TblPtr = reinterpret_cast<__gm__ uint64_t *>(l1Val);
                for (size_t l2Idx = 0; l2Idx < ONLINE_ONE_SM_STAND_FOR_BYTE; ++l2Idx) {
                    uint64_t l2Val = l2TblPtr[l2Idx];
                    if (!MemoryByteStatusParser<ByteStatus_t>::StatusIsValid(l2Val)) continue;
                    if (simdBlockHead_->blockInfo.simtEntryUseSize + sizeof(SimtEntryRecord) <=
                        globalHead_->offsetInfo.simtEntryInfo.size) {
                        SimtEntryRecord entryRecord{};
                        entryRecord.addr = (l0Idx << l1OneBits) | (l1Idx << l2OneBits) | l2Idx;
                        entryRecord.size = 1;
                        entryRecord.status = l2Val;
                        auto preEntryRecord = entryHead.recordWriteCount > 0 ?
                            gmEntryRecord + entryHead.recordWriteCount - 1 : gmEntryRecord;
                        // 判断记录是否可以合并，可以合并则更新之前的记录size；否则搬入一个新的记录
                        if ((preEntryRecord->status == entryRecord.status) &&
                            (preEntryRecord->addr + preEntryRecord->size == entryRecord.addr)) {
                            preEntryRecord->size++;
                        } else {
                            CopyRecordToGm(gmEntryRecord + entryHead.recordWriteCount, &entryRecord);
                            entryHead.recordWriteCount++;
                            entryHead.recordCount++;
                            simdBlockHead_->blockInfo.simtEntryUseSize += sizeof(SimtEntryRecord);
                        }
                    } else {
                        entryHead.exceedSize += sizeof(SimtEntryRecord);
                    }
                    // 搬运之后将shadowMemory状态还原为初始状态
#if defined(BISHENG_SUPPORT_SIMT_CALL_DBI)
                    l2TblPtr[l2Idx] = 0;
#endif
                }
            }
        }
        memInfoSimtEntryHead->recordCount = entryHead.recordCount;
        memInfoSimtEntryHead->recordWriteCount = entryHead.recordWriteCount;
        memInfoSimtEntryHead->exceedSize = entryHead.exceedSize > 0 ?
            entryHead.exceedSize + sizeof(SimtEntryBlockHead) : 0;
        simdBlockHead_->blockInfo.simtEndCount++;
    }

private:
    AICORE_FUNC_HEAD bool IsReady() const
    {
        return isReady_;
    }

    AICORE_FUNC_HEAD bool InvalidRange(AddrInfo const &addrInfo) const
    {
        OnlineMemoryType memType = SpaceToOnlineMemory(addrInfo.space);
        // 如下两种异常场景，越界算法会处理，竞争算法直接忽略该异常情况
        if (memType == OnlineMemoryType::UB && (addrInfo.addr + addrInfo.size > ubGmEps)) {
            return true;
        }
        if (memType == OnlineMemoryType::GM && (addrInfo.addr < ubGmEps)) {
            return true;
        }
        return false;
    }

    AICORE_FUNC_HEAD bool ExistRace(ByteStatus_t value, OnlineMemoryType space) const
    {
        if (MBSP::ExtractSyncStatus(value) == SyncState::POSSIBLE_RACE &&
            MBSP::ExtractMemoryType(value) == space) {
            return true;
        }
        return false;
    }

    AICORE_FUNC_HEAD ByteStatus_t ExtractSamePcStatus(MemoryByteStatus memoryStatus, ByteStatus_t oldValue,
        uint16_t threadId, AddrInfo const &addrInfo) const
    {
        uint16_t oldThreadId = MBSP::ExtractThreadId(oldValue);
        uint32_t oldPc = MBSP::ExtractPc(oldValue);
        OnlineMemoryType memType = SpaceToOnlineMemory(addrInfo.space);
        if (addrInfo.location.pc == oldPc && oldThreadId < threadId) {
            return MBSP::Construct(memoryStatus, oldThreadId, addrInfo.location.pc, memType);
        }
        return MBSP::Construct(memoryStatus, threadId, addrInfo.location.pc, memType);
    }

private:
    MultiLayerTable<ByteStatus_t> tables_;
    bool isReady_;
    __gm__ uint8_t *memInfo_;
    __gm__ uint8_t *memInfoSimt_;
    __gm__ uint8_t *memInfoSimd_;
    __gm__ uint8_t *memInfoSimtEntry_;
    __gm__ RecordGlobalHead *globalHead_;
    __gm__ SimtRecordBlockHead *simtBlockHead_;
    __gm__ RecordBlockHead *simdBlockHead_;
    uint64_t heapAddr_{};
};

template <KernelErrorType errorType>
AICORE_FUNC_HEAD void ShadowMemoryOnline::AssignErrorInfo(ShadowMemoryOnline::ByteStatus_t oldValue,
    uint16_t threadId, ShadowMemoryOnline::AuxInfo &auxInfo) {}

template<>
AICORE_FUNC_HEAD void ShadowMemoryOnline::AssignErrorInfo<KernelErrorType::THREAD_OVERLAP>(
    ShadowMemoryOnline::ByteStatus_t oldValue, uint16_t threadId, ShadowMemoryOnline::AuxInfo &auxInfo)
{
    if (!DoMemCheck(memInfo_)) { return; }
    uint16_t oldThreadId = MemoryByteStatusParser<ByteStatus_t>::ExtractThreadId(oldValue);
    auto &overLapError = auxInfo.errorInfo[overLapErrorIdx];
    overLapError.errorType = KernelErrorType::THREAD_OVERLAP;
    overLapError.pc = MemoryByteStatusParser<ByteStatus_t>::ExtractPc(oldValue);
    overLapError.nBadBytes++;
    DecomposeThreadId(oldThreadId, overLapError.conflictedThreadLoc.idX,
        overLapError.conflictedThreadLoc.idY, overLapError.conflictedThreadLoc.idZ);
}

template<>
AICORE_FUNC_HEAD void ShadowMemoryOnline::AssignErrorInfo<KernelErrorType::THREAD_WW_RACE>(
    ShadowMemoryOnline::ByteStatus_t oldValue, uint16_t threadId, ShadowMemoryOnline::AuxInfo &auxInfo)
{
    if (!DoRaceCheck(memInfo_)) { return; }
    uint16_t oldThreadId = MemoryByteStatusParser<ByteStatus_t>::ExtractThreadId(oldValue);
    auto &raceError = auxInfo.errorInfo[raceErrorIdx];
    raceError.errorType = KernelErrorType::THREAD_WW_RACE;
    raceError.pc = MemoryByteStatusParser<ByteStatus_t>::ExtractPc(oldValue);
    DecomposeThreadId(oldThreadId, raceError.conflictedThreadLoc.idX,
        raceError.conflictedThreadLoc.idY, raceError.conflictedThreadLoc.idZ);
}

template<>
AICORE_FUNC_HEAD void ShadowMemoryOnline::AssignErrorInfo<KernelErrorType::THREAD_RW_RACE>(
    ShadowMemoryOnline::ByteStatus_t oldValue, uint16_t threadId, ShadowMemoryOnline::AuxInfo &auxInfo)
{
    if (!DoRaceCheck(memInfo_)) { return; }
    uint16_t oldThreadId = MemoryByteStatusParser<ByteStatus_t>::ExtractThreadId(oldValue);
    auto &raceError = auxInfo.errorInfo[raceErrorIdx];
    raceError.errorType = KernelErrorType::THREAD_RW_RACE;
    raceError.pc = MemoryByteStatusParser<ByteStatus_t>::ExtractPc(oldValue);
    DecomposeThreadId(oldThreadId, raceError.conflictedThreadLoc.idX,
        raceError.conflictedThreadLoc.idY, raceError.conflictedThreadLoc.idZ);
}

template<>
AICORE_FUNC_HEAD void ShadowMemoryOnline::AssignErrorInfo<KernelErrorType::THREAD_WR_RACE>(
    ShadowMemoryOnline::ByteStatus_t oldValue, uint16_t threadId, ShadowMemoryOnline::AuxInfo &auxInfo)
{
    if (!DoRaceCheck(memInfo_)) { return; }
    uint16_t oldThreadId = MemoryByteStatusParser<ByteStatus_t>::ExtractThreadId(oldValue);
    auto &raceError = auxInfo.errorInfo[raceErrorIdx];
    raceError.errorType = KernelErrorType::THREAD_WR_RACE;
    raceError.pc = MemoryByteStatusParser<ByteStatus_t>::ExtractPc(oldValue);
    DecomposeThreadId(oldThreadId, raceError.conflictedThreadLoc.idX,
        raceError.conflictedThreadLoc.idY, raceError.conflictedThreadLoc.idZ);
}

AICORE_FUNC_HEAD void ShadowMemoryOnline::LoadNBytes(AddrInfo const &addrInfo, ShadowMemoryOnline::AuxInfo &auxInfo)
{
    if (!IsReady() || memInfo_ == nullptr) { return; }
    if (!DoRaceCheck(memInfo_)) { return; }
    if (InvalidRange(addrInfo)) { return; }

    OnlineMemoryType memType = SpaceToOnlineMemory(addrInfo.space);
    uint16_t threadId = GetThreadId();
    for (uint64_t i = 0U; i < addrInfo.size; ++i) {
        uint64_t addr = addrInfo.addr + i;
        uint64_t ret = tables_.LookUp(addr, auxInfo.l1StartAddr, auxInfo.l2StartAddr, auxInfo.l2MemStatusAddr);
        // shadow memory内存不够，地址查询失败，中断后续的地址访问
        if (ret != 0U) { return; }

        ByteStatus_t oldValue = 0;
        ByteStatus_t casRet = oldValue + 1;
        // oldValue != casRet，说明其他线程修改了l2MemStatusAddr，重试；
        // oldValue == casRet，说明本线程CAS更新成功，退出循环；
        while (oldValue != casRet) {
            oldValue = *reinterpret_cast<__gm__ ByteStatus_t*>(auxInfo.l2MemStatusAddr);
            ByteStatus_t newValue = oldValue;
            MemoryByteStatus oldStatus = MBSP::ExtractMemoryStatus(oldValue);
            OnlineMemoryType oldSpace = MBSP::ExtractMemoryType(oldValue);
            uint32_t oldPc = MBSP::ExtractPc(oldValue);
            uint16_t oldThreadId = MBSP::ExtractThreadId(oldValue);
            if (oldStatus == MemoryByteStatus::DEFAULT) {
                newValue = MBSP::Construct(MemoryByteStatus::READ, threadId, addrInfo.location.pc, memType);
            } else if (oldStatus == MemoryByteStatus::READ) {
                if (oldThreadId == threadId) {
                    newValue = MBSP::Construct(MemoryByteStatus::READ, threadId, addrInfo.location.pc, memType);
                } else {
                    newValue = ExtractSamePcStatus(MemoryByteStatus::GLOBAL_READ, oldValue, threadId, addrInfo);
                }
            } else if (oldStatus == MemoryByteStatus::GLOBAL_READ) {
                newValue = ExtractSamePcStatus(MemoryByteStatus::GLOBAL_READ, oldValue, threadId, addrInfo);
            } else if (oldStatus == MemoryByteStatus::WRITE) {
                if (oldThreadId != threadId && ExistRace(oldValue, memType)) {
                    /// 写读竞争时，将gm上的状态设置为写线程的状态，以保证后续遇到读事件时能识别到竞争问题
                    newValue = MBSP::Construct(MemoryByteStatus::RACE, oldThreadId, oldPc, memType);
                    AssignErrorInfo<KernelErrorType::THREAD_WR_RACE>(oldValue, threadId, auxInfo);
                }
            } else if (oldStatus == MemoryByteStatus::RACE) {
                if (oldThreadId != threadId && ExistRace(oldValue, memType)) {
                    newValue = ExtractSamePcStatus(MemoryByteStatus::RACE, oldValue, threadId, addrInfo);
                    AssignErrorInfo<KernelErrorType::THREAD_WR_RACE>(oldValue, threadId, auxInfo);
                } else {
                    newValue = MBSP::Construct(MemoryByteStatus::READ, threadId, addrInfo.location.pc, memType);
                }
            }
            casRet = AtomicCAS(reinterpret_cast<__gm__ uint64_t*>(auxInfo.l2MemStatusAddr), oldValue, newValue);
        }
    }
}

AICORE_FUNC_HEAD void ShadowMemoryOnline::StoreNBytes(AddrInfo const &addrInfo, ShadowMemoryOnline::AuxInfo &auxInfo)
{
    if (!IsReady() || memInfo_ == nullptr) { return; }
    if (!DoMemCheck(memInfo_) && !DoRaceCheck(memInfo_)) { return; }
    if (InvalidRange(addrInfo)) { return; }

    OnlineMemoryType memType = SpaceToOnlineMemory(addrInfo.space);
    uint16_t threadId = GetThreadId();
    for (uint64_t i = 0U; i < addrInfo.size; ++i) {
        uint64_t addr = addrInfo.addr + i;
        uint64_t ret = tables_.LookUp(addr, auxInfo.l1StartAddr, auxInfo.l2StartAddr, auxInfo.l2MemStatusAddr);
        if (ret != 0U) { return; }

        ByteStatus_t oldValue = 0;
        ByteStatus_t casRet = oldValue + 1;
        // oldValue != casRet，说明其他线程修改了l2MemStatusAddr，重试；
        // oldValue == casRet，说明本线程CAS更新成功，退出循环；
        bool overlapIsWrite = false;
        while (oldValue != casRet) {
            oldValue = *reinterpret_cast<__gm__ ByteStatus_t*>(auxInfo.l2MemStatusAddr);
            ByteStatus_t newValue = oldValue;
            MemoryByteStatus oldStatus = MBSP::ExtractMemoryStatus(oldValue);
            OnlineMemoryType oldSpace = MBSP::ExtractMemoryType(oldValue);
            uint32_t oldPc = MBSP::ExtractPc(oldValue);
            uint16_t oldThreadId = MBSP::ExtractThreadId(oldValue);
            if (oldStatus == MemoryByteStatus::DEFAULT) {
                newValue = MBSP::Construct(MemoryByteStatus::WRITE, threadId, addrInfo.location.pc, memType);
            } else if (oldStatus == MemoryByteStatus::READ) {
                if (oldThreadId != threadId && ExistRace(oldValue, memType)) {
                    newValue = MBSP::Construct(MemoryByteStatus::RACE, threadId, addrInfo.location.pc, memType);
                    AssignErrorInfo<KernelErrorType::THREAD_RW_RACE>(oldValue, threadId, auxInfo);
                } else {
                    newValue = MBSP::Construct(MemoryByteStatus::WRITE, threadId, addrInfo.location.pc, memType);
                }
            } else if (oldStatus == MemoryByteStatus::GLOBAL_READ) {
                if (ExistRace(oldValue, memType)) {
                    newValue = MBSP::Construct(MemoryByteStatus::RACE, threadId, addrInfo.location.pc, memType);
                    AssignErrorInfo<KernelErrorType::THREAD_RW_RACE>(oldValue, threadId, auxInfo);
                } else {
                    newValue = MBSP::Construct(MemoryByteStatus::WRITE, threadId, addrInfo.location.pc, memType);
                }
            } else if (oldStatus == MemoryByteStatus::WRITE) {
                if (oldThreadId != threadId && ExistRace(oldValue, memType)) {
                    newValue = ExtractSamePcStatus(MemoryByteStatus::RACE, oldValue, threadId, addrInfo);
                    AssignErrorInfo<KernelErrorType::THREAD_WW_RACE>(oldValue, threadId, auxInfo);
                    if (!overlapIsWrite) {
                        AssignErrorInfo<KernelErrorType::THREAD_OVERLAP>(oldValue, threadId, auxInfo);
                        overlapIsWrite = true;
                    }
                } else {
                    newValue = MBSP::Construct(MemoryByteStatus::WRITE, threadId, addrInfo.location.pc, memType);
                }
            } else if (oldStatus == MemoryByteStatus::RACE) {
                if (oldThreadId != threadId && ExistRace(oldValue, memType)) {
                    newValue = ExtractSamePcStatus(MemoryByteStatus::RACE, oldValue, threadId, addrInfo);
                    AssignErrorInfo<KernelErrorType::THREAD_WW_RACE>(oldValue, threadId, auxInfo);
                    if (!overlapIsWrite && oldThreadId != threadId) {
                        AssignErrorInfo<KernelErrorType::THREAD_OVERLAP>(oldValue, threadId, auxInfo);
                        overlapIsWrite = true;
                    }
                } else {
                    newValue = MBSP::Construct(MemoryByteStatus::WRITE, threadId, addrInfo.location.pc, memType);
                }
            }
            casRet = AtomicCAS(reinterpret_cast<__gm__ uint64_t*>(auxInfo.l2MemStatusAddr), oldValue, newValue);
        }
    }
}

#endif

}

#endif
