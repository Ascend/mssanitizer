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

#ifndef PLUGIN_ONLINE_CHECK_H
#define PLUGIN_ONLINE_CHECK_H

#include "core/framework/record_defs.h"
#include "kernel_pub_func.h"
#include "record_type_map.h"
#include "parse_record.h"
#include "shadow_memory_online.h"

namespace Sanitizer {

/* OnlineCheck kernel侧内存检测类
 * head处记录了host侧的malloc信息
 * 桩函数记录时，会解析head处的malloc信息并做比对，
 * 如果有内存错误则记录错误行为信息，无错误则直接返回；
 *
 * 使用方法如下
 * @code
 * OnlineCheck check();
 * check.Init(memInfo，memInfoBlock);
 * check.Process<RecordType::SIMT_LDG>(record);
 * @endcode
 */

class OnlineCheck {
public:
    AICORE_FUNC_HEAD __attribute__((always_inline)) OnlineCheck() : memInfo_{nullptr}, memInfoSimt_{nullptr}, memInfoSimd_{nullptr},
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(SIMT_MODE)
        globalHead_{nullptr}, simtBlockHead_{nullptr}, simdBlockHead_{nullptr}, sortedLen_{}, blockIdx_{},
        shadowMemory_()
#else
        globalHead_{nullptr}, simtBlockHead_{nullptr}, simdBlockHead_{nullptr}, sortedLen_{}, blockIdx_{}
#endif
        {}

     /* @param  memInfo              全局头部指针
      * @param  memInfoSimt         当前block对应的simt指针
      * @param  memInfoSimd         当前block对应的simd指针
      * @param  blockIdx            当前block数
      * @brief  初始化内存检测类
     */
    AICORE_FUNC_HEAD void Init(__gm__ uint8_t *memInfo, __gm__ uint8_t *memInfoSimt, __gm__ uint8_t *memInfoSimd,
        uint64_t blockIdx);

    /* @tparam  recordType    记录类型枚举
     * @tparam  Record        记录结构体类型
     * @param   record        指令记录信息
     * @brief   传入栈上的记录，判断栈上的记录存在内存错误行为；
     */
    template<RecordType recordType, typename Record>
    AICORE_FUNC_HEAD void Process(Record const &record);

    /*
     * @brief 处理para base addr地址，将kernel入参地址写入到blockHead对应位置
     */
    AICORE_FUNC_HEAD void ProcessParaBaseAddr();

#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(SIMT_MODE)
    AICORE_FUNC_HEAD void ClearSyncThreadState() {
        shadowMemory_.ClearSyncThreadState();
    }
#endif

#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(SIMT_MODE)
    AICORE_FUNC_HEAD void CopyShadowMemoryToMemInfo() {
        shadowMemory_.CopyShadowMemoryToMemInfo();
    }
#endif

private:

    /* @param  addr               待检查的地址
     * @param  size               待检查的地址长度
     * @param  thresholdAddr      地址阈值
     * @param  thresholdSize      长度阈值
     * @brief  计算待检查地址和阈值范围的交集长度，返回值表示交集长度
     */
    AICORE_FUNC_HEAD uint64_t CalIntersectionSize(uint64_t addr, uint64_t size, uint64_t thresholdAddr,
        uint64_t thresholdSize) const;

    /* @tparam  recordType  记录类型枚举
     * @tparam  Record      记录结构体类型
     * @param   addrInfo    simt指令的信息
     * @param   record      指令记录信息
     * @brief 计算内存操作行为的错误信息，支持多种错误类型的同时记录
    */
    template<RecordType recordType, typename Record>
    AICORE_FUNC_HEAD void Do(AddrInfo const &addrInfo, Record const &record);

    /* @param addrInfo      simt指令的信息
     * @param illegalSize   错误长度
     * @brief 检测当前gm指令内存行为是否有非法读写行为，如果有则返回非法读写的长度，返回值表示是否有非法行为
    */
    AICORE_FUNC_HEAD bool GmReadWriteCheck(AddrInfo const &addrInfo, uint64_t &illegalSize) const;

    /* @param addrInfo      simt指令的信息
     * @param illegalSize   错误长度
     * @brief 检测当前ub指令内存行为是否有非法读写行为，如果有则返回非法读写的长度，返回值表示是否有非法行为
    */
    AICORE_FUNC_HEAD bool UbReadWriteCheck(AddrInfo const &addrInfo, uint64_t &illegalSize) const;

    /* @param addrInfo      simt指令的信息
     * @brief 检测当前内存行为是否有非对齐读写行为，如果有则返回true，否则返回false
    */
    AICORE_FUNC_HEAD bool AlignCheck(AddrInfo const &addrInfo) const;

    /* @tparam  recordType        记录类型枚举
     * @tparam  Record            记录结构体类型
     * @param   errorRecord       错误信息记录
     * @param   errorDesc          错误信息具体描述
     * @param   record            指令记录信息
     * @param   cacheWriteOffset  缓存的记录写入偏移
     * @brief 将当前记录的错误信息dump到gm上保存
     * dump协议如下：
     * ONLINE_ERROR | KernelErrorRecord | Record | KernelErrorDesc_1 | KernelErrorDesc_2 | .....
     */
    template<RecordType recordType, typename Record>
    AICORE_FUNC_HEAD void DumpErrorInfo(KernelErrorRecord &errorRecord, KernelErrorDesc const &errorDesc,
        Record const &record, uint64_t cacheWriteOffset);

    /*
     * @brief 将kernel入参地址写入到blockHead对应位置
    */
    AICORE_FUNC_HEAD bool WriteParaBaseAddr();

    /*
     * @brief 对simdHead处的内存地址进行插入排序，默认升序
    */
    AICORE_FUNC_HEAD void InsertionSortMemory();

    /*
     * @brief 将simdHead处的内存地址合并为不连续的内存序列，便于后续求越界长度
    */
    AICORE_FUNC_HEAD void MergeMemory();

private:
    __gm__ uint8_t *memInfo_;
    __gm__ uint8_t *memInfoSimt_;
    __gm__ uint8_t *memInfoSimd_;
    __gm__ RecordGlobalHead *globalHead_;
    __gm__ SimtRecordBlockHead *simtBlockHead_;
    __gm__ RecordBlockHead *simdBlockHead_;
    uint32_t sortedLen_;                    // 已经排好序的内存长度
    int16_t blockIdx_;
#if (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(SIMT_MODE)) || defined(__BUILD_TESTS__)
    AICORE_FUNC_HEAD void ShadowMemoryCheck(AddrInfo const &addrInfo, ShadowMemoryOnline::AuxInfo &auxInfo);
    ShadowMemoryOnline shadowMemory_; // 用于在线踩踏检测

    AICORE_FUNC_HEAD bool UpdateSyncThreadPcNum(uint64_t pc);
    AICORE_FUNC_HEAD bool SortSyncThreadPcNumInPlace(
        __gm__ SimtRecordBlockHead *simtBlockHead0, uint16_t &validPcNum, uint32_t *tmpCounts);
    AICORE_FUNC_HEAD void GetMaxSyncThreadPcNum(uint16_t &validPcNum, uint32_t *tmpCounts);
#endif
};

AICORE_FUNC_HEAD void OnlineCheck::Init(__gm__ uint8_t *memInfo, __gm__ uint8_t *memInfoSimt,
    __gm__ uint8_t *memInfoSimd, uint64_t blockIdx)
{
    memInfo_ = memInfo;
    memInfoSimt_ = memInfoSimt;
    memInfoSimd_ = memInfoSimd;
    blockIdx_ = blockIdx;
    globalHead_ = reinterpret_cast<__gm__ RecordGlobalHead *>(memInfo);
    simtBlockHead_ = reinterpret_cast<__gm__ SimtRecordBlockHead *>(memInfoSimt_);
    simdBlockHead_ = reinterpret_cast<__gm__ RecordBlockHead *>(memInfoSimd_);
#if (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(SIMT_MODE)) || defined(__BUILD_TESTS__)
    shadowMemory_.Init((uint64_t)(memInfoSimd + globalHead_->offsetInfo.shadowMemoryInfo.offset),
        globalHead_->offsetInfo.shadowMemoryInfo.size, memInfo, memInfoSimt, memInfoSimd);
#endif
    simdBlockHead_->blockInfo.blockId = GetBlockIdx();
}

template<RecordType recordType, typename Record>
AICORE_FUNC_HEAD void OnlineCheck::Process(Record const &record)
{
    if (memInfo_ == nullptr) {
        return;
    }

    AddrInfo addrInfo = ParseRecord<recordType>(record);
    Do<recordType>(addrInfo, record);
}

AICORE_FUNC_HEAD void OnlineCheck::ProcessParaBaseAddr()
{
    if (memInfo_ == nullptr) {
        return;
    }
    if (!WriteParaBaseAddr()) {
        return;
    }
    InsertionSortMemory();
    MergeMemory();
    simdBlockHead_->extraWriteSuccess = true;
    Flush(memInfoSimd_);
}

AICORE_FUNC_HEAD uint64_t OnlineCheck::CalIntersectionSize(uint64_t addr, uint64_t size, uint64_t thresholdAddr,
    uint64_t thresholdSize) const
{
    /// 不存在交集
    if (addr + size <= thresholdAddr || addr >= thresholdAddr + thresholdSize || thresholdSize == 0U) {
        return 0U;
    }
    if (addr <= thresholdAddr && addr + size >= thresholdAddr + thresholdSize) {
        /// 左右均越界
        return thresholdSize;
    } else if (addr + size > thresholdAddr + thresholdSize) {
        /// 仅右边界越界
        return thresholdAddr - addr + thresholdSize;
    } else if (addr + size > thresholdAddr + thresholdSize) {
        /// 仅左边界越界
        return addr - thresholdAddr + size;
    } else {
        /// 子集
        return size;
    }
}

template<RecordType recordType, typename Record>
AICORE_FUNC_HEAD void OnlineCheck::Do(AddrInfo const &addrInfo, Record const &record)
{
    uint64_t cacheWriteOffset = simtBlockHead_->writeOffset;
    KernelErrorRecord errorRecord{};
    errorRecord.recordType = recordType;
    errorRecord.recordSize = sizeof(Record);
    KernelErrorDesc errorDesc{};
    errorDesc.location = addrInfo.location;
    errorDesc.threadLoc = addrInfo.threadLoc;
    errorDesc.space = addrInfo.space;

    /// 1. 越界读写检测，后续拓展其他检测能力
    uint64_t illegalSize = 0U;
    if (DoMemCheck(memInfo_) && (GmReadWriteCheck(addrInfo, illegalSize) ||
        UbReadWriteCheck(addrInfo, illegalSize))) {
        auto &illegalDesc = errorDesc.payload.illegalDesc;
        illegalDesc.addr = addrInfo.addr;
        illegalDesc.illegalSize = illegalSize;
        /// 如果是MEMCPY_BLOCKS，则应有读写2个错误类型，否则为1个
        if (addrInfo.opType == AccessType::MEMCPY_BLOCKS) {
            errorDesc.errorType = KernelErrorType::ILLEGAL_ADDR_READ;
            DumpErrorInfo<recordType>(errorRecord, errorDesc, record, cacheWriteOffset);
            errorDesc.errorType = KernelErrorType::ILLEGAL_ADDR_WRITE;
            DumpErrorInfo<recordType>(errorRecord, errorDesc, record, cacheWriteOffset);
        } else {
            errorDesc.errorType = addrInfo.opType == AccessType::READ ?
                                  KernelErrorType::ILLEGAL_ADDR_READ : KernelErrorType::ILLEGAL_ADDR_WRITE;
            DumpErrorInfo<recordType>(errorRecord, errorDesc, record, cacheWriteOffset);
        }
    }

    /// 2. 非对齐检测
    if (DoMemCheck(memInfo_) && AlignCheck(addrInfo)) {
        auto &misAlignDesc = errorDesc.payload.misAlignDesc;
        misAlignDesc.addr = addrInfo.addr;
        misAlignDesc.misAlignSize = addrInfo.alignSize;
        errorDesc.errorType = KernelErrorType::MISALIGNED_ACCESS;
        DumpErrorInfo<recordType>(errorRecord, errorDesc, record, cacheWriteOffset);
    }

#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(SIMT_MODE)
    /// 3. 线程间内存踩踏检测和竞争检测，设计前提：SIMT每个线程访问的GM空间相互隔离无交叉，原子类操作的地址除外
    /// 4. SIMD和SIMT间未初始化检测，与踩踏和竞争同时开启时，复用原逻辑；单独开启时，仅记录第一条读/写
    if (DoMemCheck(memInfo_) || DoRaceCheck(memInfo_) || DoInitCheck(memInfo_)) {
        ShadowMemoryOnline::AuxInfo auxInfo{};
        ShadowMemoryCheck(addrInfo, auxInfo);
        errorDesc.l1StartAddr = auxInfo.l1StartAddr;
        errorDesc.l2StartAddr = auxInfo.l2StartAddr;
        errorDesc.l2MemStatusAddr = auxInfo.l2MemStatusAddr;
        auto &overLapError = auxInfo.errorInfo[ShadowMemoryOnline::overLapErrorIdx];
        if (overLapError.errorType == KernelErrorType::THREAD_OVERLAP) {
            errorDesc.errorType = overLapError.errorType;
            auto &overLapDesc = errorDesc.payload.overLapDesc;
            overLapDesc.addr = addrInfo.addr;
            overLapDesc.overLapSize = overLapError.nBadBytes;
            overLapDesc.conflictedThreadLoc = overLapError.conflictedThreadLoc;
            DumpErrorInfo<recordType>(errorRecord, errorDesc, record, cacheWriteOffset);
        }

        auto &raceError = auxInfo.errorInfo[ShadowMemoryOnline::raceErrorIdx];
        if (raceError.errorType == KernelErrorType::THREAD_RW_RACE ||
            raceError.errorType == KernelErrorType::THREAD_WR_RACE ||
            raceError.errorType == KernelErrorType::THREAD_WW_RACE) {
            errorDesc.errorType = raceError.errorType;
            auto &raceDesc = errorDesc.payload.raceDesc;
            raceDesc.addr = addrInfo.addr;
            raceDesc.conflictedThreadLoc = raceError.conflictedThreadLoc;
            raceDesc.conflictedLocation.pc = raceError.pc;
            DumpErrorInfo<recordType>(errorRecord, errorDesc, record, cacheWriteOffset);
        }

        auto &initError = auxInfo.errorInfo[ShadowMemoryOnline::initErrorIdx];
        if (initError.errorType == KernelErrorType::UNINITIALIZED_READ) {
            errorDesc.errorType = initError.errorType;
            auto &unitializedDesc = errorDesc.payload.unitializedDesc;
            unitializedDesc.addr = addrInfo.addr;
            unitializedDesc.errorSize = initError.nBadBytes;
            unitializedDesc.threadLoc = initError.conflictedThreadLoc;
            unitializedDesc.pc = initError.pc;
            DumpErrorInfo<recordType>(errorRecord, errorDesc, record, cacheWriteOffset);
        }

        auto &writeLoss = auxInfo.errorInfo[ShadowMemoryOnline::writeLossIdx];
        if (writeLoss.errorType == KernelErrorType::WRITE_LOSS) {
            errorDesc.errorType = writeLoss.errorType;
            auto &writeLossDesc = errorDesc.payload.writeLossDesc;
            writeLossDesc.addr = addrInfo.addr;
            writeLossDesc.memSize = writeLoss.nBadBytes;
            writeLossDesc.pc = writeLoss.pc;
            DumpErrorInfo<recordType>(errorRecord, errorDesc, record, cacheWriteOffset);
        }
    }

    /// 5.同步检测，syncthreads是否正确使用
    if (DoSyncCheck(memInfo_)) {
        if (recordType == RecordType::THREAD_BLOCK_BARRIER) {
            if (!UpdateSyncThreadPcNum(addrInfo.location.pc)) {
                errorDesc.errorType = KernelErrorType::SYNC_THREADS_RECORD_LOSS;
                DumpErrorInfo<recordType>(errorRecord, errorDesc, record, cacheWriteOffset);
            }
        }

        if (recordType == RecordType::SIMT_END) {
            auto &blockInfo = simdBlockHead_->blockInfo;
            uint64_t oldVal = AtomicAdd(&blockInfo.simtEndLastThread, 1);
            if (IsSimtLastThread(oldVal)) {
                uint16_t validPcNum{0};
                uint32_t tmpCounts[SIMT_THREAD_MAX_PC_NUM] = {0};
                uint64_t threadOffset0 = globalHead_->offsetInfo.simtErrorInfo.offset;
                __gm__ uint8_t *simtBlock0 = memInfoSimd_ + threadOffset0;
                __gm__ SimtRecordBlockHead *simtBlockHead0 = reinterpret_cast<__gm__ SimtRecordBlockHead *>(simtBlock0);
                if (!SortSyncThreadPcNumInPlace(simtBlockHead0, validPcNum, tmpCounts)) {
                    errorDesc.errorType = KernelErrorType::SYNC_THREADS_RECORD_LOSS;
                    DumpErrorInfo<recordType>(errorRecord, errorDesc, record, cacheWriteOffset);
                }
                GetMaxSyncThreadPcNum(validPcNum, tmpCounts);

                // 检查当前pc是否存在异常：是否小于tmpCounts对应pc上的num
                //                 pc-0   pc-1   pc-2   ...
                // tmpCounts   :     1      1      2
                // thread-0    ：    1      1      1
                // thread-0    ：    1      0      2
                //    ...                  ...
                // thread-2047 ：    1      1      1
                // correct     ：    Y      N      N
                errorDesc.errorType = KernelErrorType::THREADS_ASYNC_IN_BLOCK;
                auto &syncDesc = errorDesc.payload.syncDesc;
                Location loc;
                SimtThreadLocation threadLoc{};
                for (size_t pcIdx = 0; pcIdx < validPcNum; ++pcIdx) {
                    for (size_t threadIdx = 0; threadIdx < blockInfo.simtEndLastThread; ++threadIdx) {
                        uint64_t threadOffset = globalHead_->offsetInfo.simtErrorInfo.offset +
                            threadIdx * (globalHead_->offsetInfo.simtErrorInfo.size + sizeof(SimtRecordBlockHead));
                        __gm__ uint8_t *simtBlock = memInfoSimd_ + threadOffset;
                        __gm__ SimtRecordBlockHead *simtBlockHead = reinterpret_cast<__gm__ SimtRecordBlockHead *>(simtBlock);

                        if (simtBlockHead->syncThreadNum[pcIdx] < tmpCounts[pcIdx]) {
                            loc.pc = simtBlockHead0->syncThreadPC[pcIdx];
                            syncDesc.syncLocation = loc;
                            DecomposeThreadId(threadIdx, threadLoc.idX, threadLoc.idY, threadLoc.idZ);
                            syncDesc.syncThreadLoc = threadLoc;
                            DumpErrorInfo<recordType>(errorRecord, errorDesc, record, cacheWriteOffset);
                        }
                    }
                }
            }
        }
    }
#endif
}

#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(SIMT_MODE)
AICORE_FUNC_HEAD void OnlineCheck::ShadowMemoryCheck(AddrInfo const &addrInfo, ShadowMemoryOnline::AuxInfo &auxInfo) {
    if (addrInfo.space != AddressSpace::GM && addrInfo.space != AddressSpace::UB) {
        return;
    }
    if (!shadowMemory_.IsReady() || memInfo_ == nullptr) {
        return;
    }
    if (shadowMemory_.InvalidRange(addrInfo)) {
        return;
    }

    if (addrInfo.opType == AccessType::READ) {
        shadowMemory_.LoadNBytes(addrInfo, auxInfo);
    } else if (addrInfo.opType == AccessType::MEMCPY_BLOCKS) {
        shadowMemory_.LoadNBytes(addrInfo, auxInfo);
        shadowMemory_.StoreNBytes(addrInfo, auxInfo);
    } else {
        shadowMemory_.StoreNBytes(addrInfo, auxInfo);
    }
}

AICORE_FUNC_HEAD bool OnlineCheck::UpdateSyncThreadPcNum(uint64_t pc)
{
    for (size_t i = 0; i < SIMT_THREAD_MAX_PC_NUM; ++i) {
        if (simtBlockHead_->syncThreadPC[i] == 0) {
            // 出现0说明数组没找到当前pc，将pc加入数组，num++
            simtBlockHead_->syncThreadPC[i] = pc;
            ++simtBlockHead_->syncThreadNum[i];
            return true;
        } else if (simtBlockHead_->syncThreadPC[i] == pc) {
            // 相等说明数组已经存在当前pc，直接num++
            ++simtBlockHead_->syncThreadNum[i];
            return true;
        }
    }
    // for循环正常结束说明预留数组空间不够
    return false;
}

AICORE_FUNC_HEAD bool OnlineCheck::SortSyncThreadPcNumInPlace(
    __gm__ SimtRecordBlockHead *simtBlockHead0, uint16_t &validPcNum, uint32_t *tmpCounts) {
    bool isSortedAll{true};
    // 线程间循环：遍历simt threads；线程内循环：遍历pc和num
    // thread-0:[pc0,pc1,...,0,0][num0,num1,...,0,0],thread-1:[pc0,pc1,...,0,0][num0,num1,...,0,0]...
    //    |_________________线程间循环__________________|       |__线程内循环__|   |__线程内循环__|
    // 遍历所有线程，将所有pc以追加的形式不重复地记录到thread-0的pc数组位置，
    // 同时按照thread-0的pc顺序重新整理每个线程的num，记录在临时数组tmpCounts中，并刷回num数组（原地更新）
    // thread-0只更新validPcNum，不做其他处理
    for (size_t numIdx = 0; numIdx < SIMT_THREAD_MAX_PC_NUM; ++numIdx) {
        if (simtBlockHead0->syncThreadPC[numIdx] == 0) {
            break; // 出现空的地址，说明有效地址已经遍历完，结束循环
        }
        ++validPcNum;
    }
    if (simdBlockHead_->blockInfo.simtEndLastThread <= 1) {
        return true;
    }
    for (size_t threadIdx = 1; threadIdx < simdBlockHead_->blockInfo.simtEndLastThread; ++threadIdx) { // 线程间循环
        uint64_t threadOffset = globalHead_->offsetInfo.simtErrorInfo.offset +
            threadIdx * (globalHead_->offsetInfo.simtErrorInfo.size + sizeof(SimtRecordBlockHead));
        __gm__ uint8_t *simtBlock = memInfoSimd_ + threadOffset;
        __gm__ SimtRecordBlockHead *simtBlockHead = reinterpret_cast<__gm__ SimtRecordBlockHead *>(simtBlock);

        for (size_t numIdx = 0; numIdx < SIMT_THREAD_MAX_PC_NUM; ++numIdx) {  // 线程内循环
            uint32_t pc = simtBlockHead->syncThreadPC[numIdx];
            if (pc == 0) {
                break; // 出现空的地址，说明有效地址已经遍历完，结束循环
            }
            uint32_t count = simtBlockHead->syncThreadNum[numIdx];

            bool found = false;
            for (uint16_t j = 0; j < validPcNum; ++j) {
                if (simtBlockHead0->syncThreadPC[j] == pc) {
                    tmpCounts[j] = count;
                    found = true;
                    break;
                }
            }
            if (!found) {
                if (validPcNum < SIMT_THREAD_MAX_PC_NUM) {
                    simtBlockHead0->syncThreadPC[validPcNum] = pc;
                    tmpCounts[validPcNum] = count;
                    ++validPcNum;
                } else {
                    isSortedAll = false;
                    break;
                }
            }
        }
        // 把tmpCounts写回simtBlockHead->syncThreadNum，并重置tmpCounts
        for (size_t each = 0; each < validPcNum; ++each) {
            simtBlockHead->syncThreadNum[each] = tmpCounts[each];
            tmpCounts[each] = 0;
        }
    }
    return isSortedAll;
}

AICORE_FUNC_HEAD void OnlineCheck::GetMaxSyncThreadPcNum(uint16_t &validPcNum, uint32_t *tmpCounts) {
    for (size_t pcIdx = 0; pcIdx < validPcNum; ++pcIdx) {
        uint32_t maxNum = 0;
        for (size_t threadIdx = 0; threadIdx < simdBlockHead_->blockInfo.simtEndLastThread; ++threadIdx) {
            uint64_t threadOffset = globalHead_->offsetInfo.simtErrorInfo.offset +
                threadIdx * (globalHead_->offsetInfo.simtErrorInfo.size + sizeof(SimtRecordBlockHead));
            __gm__ uint8_t *simtBlock = memInfoSimd_ + threadOffset;
            __gm__ SimtRecordBlockHead *simtBlockHead = reinterpret_cast<__gm__ SimtRecordBlockHead *>(simtBlock);
            if (simtBlockHead->syncThreadNum[pcIdx] > maxNum) {
                maxNum = simtBlockHead->syncThreadNum[pcIdx];
            }
        }
        tmpCounts[pcIdx] = maxNum;
    }
}

#endif

AICORE_FUNC_HEAD bool HasPermission(AccessType accessType, uint32_t permission) {
    if (accessType == AccessType::READ) {
        return (permission & MSTX_MEM_PERMISSIONS_REGION_FLAGS_READ) != 0;
    } else if (accessType == AccessType::WRITE) {
        return (permission & MSTX_MEM_PERMISSIONS_REGION_FLAGS_WRITE) != 0;
    } else {
        return (permission & MSTX_MEM_PERMISSIONS_REGION_FLAGS_READ) != 0 &&
            (permission & MSTX_MEM_PERMISSIONS_REGION_FLAGS_WRITE) != 0;
    }
}

AICORE_FUNC_HEAD bool OnlineCheck::GmReadWriteCheck(AddrInfo const &addrInfo, uint64_t &illegalSize) const
{
    if (addrInfo.space != AddressSpace::GM) {
        return false;
    }

    uint64_t intersectionSize{};
    uint64_t addr = addrInfo.addr;
    uint64_t size = addrInfo.size;
    /// 计算当前addrInfo和所有malloc地址的交集
    for (size_t memIdx = 0; memIdx < simdBlockHead_->hostMemoryNum; ++memIdx) {
        __gm__ HostMemoryInfo const &mallocInfo = simdBlockHead_->hostMemoryInfoPtr[memIdx];
        // 内存无对应权限则跳过
        if (!HasPermission(addrInfo.opType, mallocInfo.permission)) {
            continue;
        }
        intersectionSize += CalIntersectionSize(addr, size, mallocInfo.addr, mallocInfo.size);
    }

    /// 当前指令的长度减去和所有malloc 地址的交集长度，即为越界长度；
    illegalSize += size - intersectionSize;
    return illegalSize > 0U;
}

AICORE_FUNC_HEAD bool OnlineCheck::UbReadWriteCheck(AddrInfo const &addrInfo, uint64_t &illegalSize) const
{
    if (addrInfo.space != AddressSpace::UB) {
        return false;
    }

    uint64_t addr = addrInfo.addr;
    uint64_t size = addrInfo.size;
    uint32_t ubSize = globalHead_->simtInfo.ubDynamicSize;
    if (addr >= ubSize) {
        illegalSize = size;
    } else if (addr + size > ubSize) {
        illegalSize = addr + size - ubSize;
    }
    return illegalSize > 0U;
}

/// 同一个指令桩记录可能对应多条错误类型，多条错误类型的RecordType/Record公用；
/// 记录第一个错误类型时，会记录RecordType/KernelErrorRecord/Record/KernelErrorDesc；
/// 后续其余的错误类型只会记录KernelErrorDesc，其余的信息只会刷新
template<RecordType recordType, typename Record>
AICORE_FUNC_HEAD void OnlineCheck::DumpErrorInfo(KernelErrorRecord &errorRecord, KernelErrorDesc const &errorDesc,
    Record const &record, uint64_t cacheWriteOffset)
{
    constexpr uint32_t FIRST_ERROR_NUM = 1;
    errorRecord.errorNum++;
    __gm__ uint8_t *startPtr = memInfoSimt_ + sizeof(SimtRecordBlockHead) + cacheWriteOffset;
    __gm__ RecordType *errorType = reinterpret_cast<__gm__ RecordType *>(startPtr);
    *errorType = RecordType::ONLINE_ERROR;
    __gm__ KernelErrorRecord *gmErrorRecord = reinterpret_cast<__gm__ KernelErrorRecord *>(errorType + 1);
    __gm__ Record *gmRecord = reinterpret_cast<__gm__ Record *>(gmErrorRecord + 1);
    __gm__ KernelErrorDesc *gmErrorDesc =  reinterpret_cast<__gm__ KernelErrorDesc *>(
        reinterpret_cast<__gm__ uint8_t *>(gmRecord + 1) + sizeof(KernelErrorDesc) * (errorRecord.errorNum - 1));
    uint64_t stepSize = errorRecord.errorNum == FIRST_ERROR_NUM ? sizeof(RecordType) + sizeof(KernelErrorRecord) +
        sizeof(Record) + sizeof(KernelErrorDesc) : sizeof(KernelErrorDesc);

    if (simtBlockHead_->writeOffset + CACHE_LINE_SIZE + stepSize < globalHead_->offsetInfo.simtErrorInfo.size &&
      simtBlockHead_->recordCount == simtBlockHead_->recordWriteCount) {
        CopyRecordToGm(gmErrorRecord, &errorRecord);
        CopyRecordToGm(gmRecord, &record);
        CopyRecordToGm(gmErrorDesc, &errorDesc);
        simtBlockHead_->writeOffset += stepSize;
        /// 同一个指令有多条错误信息时，仅记录第一条错误信息更新recordWriteCount，多条错误信息会认为是一个错误记录
        if (errorRecord.errorNum == FIRST_ERROR_NUM) {
            simtBlockHead_->recordWriteCount++;
            simtBlockHead_->recordCount++;
        }
    } else { // 记录超过限制时
        simtBlockHead_->recordCount++;
    }
    simtBlockHead_->offset += stepSize;
    Flush(memInfoSimt_);
}

AICORE_FUNC_HEAD bool CheckRegIdxValid(int64_t regIdx)
{
    return (regIdx >= 0) && (regIdx <= C220_A2_OR_A3_EVEN_DEVICE_VEC_PHYS_CORE_END_IDS);
}

// 获取跟当前 coreid 匹配的寄存器状态保存结构
AICORE_FUNC_HEAD int64_t GetRegisterIdx()
{
    int64_t coreId{};

#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__) || \
    (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510))
#ifdef SIMT_MODE
    coreId = __cce_simt_get_COREID();
#else
    coreId = get_coreid();
#endif // SIMT_MODE
#endif // DAV

    // A2/A3偶数卡：vec核编号范围：[25, 74]，cube核范围：[0, 24]，获取到后可以直接当索引使用
    if (coreId >= 0 && coreId <= C220_A2_OR_A3_EVEN_DEVICE_VEC_PHYS_CORE_END_IDS) {
        return coreId;
    }

    // A3奇数卡：vec核编号范围：[32793, 32842]，cube核范围：[32768, 32792]，需要转换一下把范围限制到0-74
    if (coreId >= C220_A3_ODD_DEVICE_VEC_CUBE_CORE_START_IDS && coreId <= C220_A3_ODD_DEVICE_VEC_PHYS_CORE_END_IDS) {
        return coreId - C220_A3_ODD_DEVICE_VEC_CUBE_CORE_START_IDS;
    }
#endif // __CCE_IS_AICORE__
    return coreId;
}

AICORE_FUNC_HEAD bool OnlineCheck::WriteParaBaseAddr()
{
    /// 如果开启单核检测，则只有目标核会写入extra地址
    if (globalHead_->checkParms.checkBlockId != CHECK_ALL_BLOCK && globalHead_->checkParms.checkBlockId != blockIdx_) {
        return false;
    }
    if (simdBlockHead_->extraWriteSuccess) {
        return false;
    }
    int64_t regIdx = GetRegisterIdx();
    if (!CheckRegIdxValid(regIdx)) {
        return false;
    }
    uint64_t *addrInfo = reinterpret_cast<uint64_t *>(simdBlockHead_->paraBase.addr);
    uint32_t extraIndex = 0;
    for (uint32_t i = 0; i < simdBlockHead_->hostMemoryNum; ++i) {
        if (simdBlockHead_->hostMemoryInfoPtr[i].addr == 0x0) {
            break;
        }
        extraIndex++;
    }
    sortedLen_ = extraIndex;
    for (uint32_t i = 0; i < globalHead_->kernelInfo.kernelParamNum; ++i) {
        if (extraIndex + i >= simdBlockHead_->hostMemoryNum) {
            break;
        }
        simdBlockHead_->hostMemoryInfoPtr[extraIndex + i].addr = addrInfo[i];
    }
    return true;
}

AICORE_FUNC_HEAD void OnlineCheck::InsertionSortMemory()
{
    auto &memoryInfoPtr = simdBlockHead_->hostMemoryInfoPtr;
    for (uint32_t i = sortedLen_; i < simdBlockHead_->hostMemoryNum; ++i) {
        // 缓存待插入元素
        auto keyAddr = memoryInfoPtr[i].addr;
        auto keySize = memoryInfoPtr[i].size;
        if ((keyAddr == 0x0) || i == 0) { continue; }
        int64_t j = i - 1;
        // 在已排序区间中找到插入位置（移动元素）
        while (j >= 0 && (memoryInfoPtr[j].addr > keyAddr || memoryInfoPtr[j].addr == 0x0)) {
            // 元素后移
            memoryInfoPtr[j + 1].addr = memoryInfoPtr[j].addr;
            memoryInfoPtr[j + 1].size = memoryInfoPtr[j].size;
            j--;
        }
        // 插入元素
        memoryInfoPtr[j + 1].addr = keyAddr;
        memoryInfoPtr[j + 1].size = keySize;
    }
}

AICORE_FUNC_HEAD void OnlineCheck::MergeMemory()
{
    auto &memoryInfoPtr = simdBlockHead_->hostMemoryInfoPtr;
    if (simdBlockHead_->hostMemoryNum <= 1) {
        return;
    }
    uint32_t index{};
    for (uint32_t i = 1; i < simdBlockHead_->hostMemoryNum; ++i) {
        // 解析当前区间和有效区间的start、end
        uint64_t currAddr = memoryInfoPtr[i].addr;
        uint64_t currSize = memoryInfoPtr[i].size;
        uint64_t currEnd = currAddr + currSize;

        uint64_t lastAddr = memoryInfoPtr[index].addr;
        uint64_t lastSize = memoryInfoPtr[index].size;
        uint64_t lastEnd = lastAddr + lastSize;
        // 若当前区间与有效区间重叠（因地址升序，curr_addr >= last_addr）
        if (currAddr <= lastEnd) {
            // 合并：更新有效区间的长度（end取最大值），并将可合并的区间地址和长度置为0
            uint64_t newEnd = lastEnd > currEnd ? lastEnd : currEnd;
            memoryInfoPtr[index].size = newEnd - lastAddr;  // 新长度 = 新end - 原start
        } else {
            // 不重叠：将当前区间移动到有效区间的下一个位置
            index++;
            memoryInfoPtr[index].addr = memoryInfoPtr[i].addr;  // 覆盖原位置（或移动元素）
            memoryInfoPtr[index].size = memoryInfoPtr[i].size;
        }
    }
    /// 有效的合并后区间长度为index + 1，将多余长度置为0，保证越界长度逻辑计算正确
    for (uint32_t i = index + 1; i < simdBlockHead_->hostMemoryNum; ++i) {
        memoryInfoPtr[i].addr = 0x0;
        memoryInfoPtr[i].size = 0;
    }
}

AICORE_FUNC_HEAD bool OnlineCheck::AlignCheck(const AddrInfo &addrInfo) const
{
    return addrInfo.addr % addrInfo.alignSize != 0;
}

}  // namespace Sanitizer

#endif  // PLUGIN_ONLINE_CHECK_H
