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

#ifndef PLUGIN_RECORDER_H
#define PLUGIN_RECORDER_H

#include "online_check.h"

namespace Sanitizer {

AICORE_FUNC_HEAD bool IsTargetBlock(__gm__ uint8_t *memInfo, int16_t blockIdx)
{
    auto head = reinterpret_cast<__gm__ RecordGlobalHead *>(memInfo);
    if (head->checkParms.checkBlockId == CHECK_ALL_BLOCK) {
        return true;
    }
    if (head->checkParms.checkBlockId == blockIdx) {
        return true;
    }
    return false;
}

/// 判断是否需要dump某个核的数据
template<RecordType recordType, typename Record>
AICORE_FUNC_HEAD bool IsTargetIntrinsic(__gm__ uint8_t *memInfo, int16_t blockIdx, Record const *record)
{
    /// ffts所有核上的相关记录只有c220/c310分离架构才会记录；
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    if constexpr (recordType == RecordType::FFTS_SYNC || recordType == RecordType::WAIT_FLAG_DEV ||
            recordType == RecordType::FFTS_SYNC_V || recordType == RecordType::WAIT_FLAG_DEV_PIPE ||
            recordType == RecordType::WAIT_FLAG_DEVI_PIPE || recordType == RecordType::WAIT_FLAG_DEV_PIPE ||
            recordType == RecordType::WAIT_FLAG_DEV_PIPE_V || recordType == RecordType::WAIT_FLAG_DEVI_PIPE_V ||
            recordType == RecordType::SET_INTRA_BLOCK || recordType == RecordType::SET_INTRA_BLOCKI ||
            recordType == RecordType::SET_INTRA_BLOCK_V || recordType == RecordType::SET_INTRA_BLOCKI_V ||
            recordType == RecordType::WAIT_INTRA_BLOCK || recordType == RecordType::WAIT_INTRA_BLOCKI ||
            recordType == RecordType::WAIT_INTRA_BLOCK_V || recordType == RecordType::WAIT_INTRA_BLOCKI_V) {
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__) || \
    (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510))
        return true;
#endif
    }
    /// mstx核间同步相关记录只有c220分离架构才会记录；
    if (recordType == RecordType::MSTX_STUB) {
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__)
        auto mstxRecord = reinterpret_cast<MstxRecord const *>(record);
        if (mstxRecord->interfaceType == InterfaceType::MSTX_SET_CROSS_SYNC ||
            mstxRecord->interfaceType == InterfaceType::MSTX_WAIT_CROSS_SYNC) {
            return true;
        }
#endif
    }
#else // __CCE_IS_AICORE__
    if (recordType == RecordType::FFTS_SYNC || recordType == RecordType::WAIT_FLAG_DEV) {
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__)
        return true;
#endif
    }
    if (recordType == RecordType::MSTX_STUB) {
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__)
        auto mstxRecord = reinterpret_cast<MstxRecord const *>(record);
        if (mstxRecord->interfaceType == InterfaceType::MSTX_SET_CROSS_SYNC ||
            mstxRecord->interfaceType == InterfaceType::MSTX_WAIT_CROSS_SYNC) {
            return true;
        }
#endif
    }
#endif
    return IsTargetBlock(memInfo, blockIdx);
}

AICORE_FUNC_HEAD bool IsInMstxFuseScope(__gm__ uint8_t *memInfoBlock)
{
    auto memInfoBlockHead = reinterpret_cast<__gm__ RecordBlockHead *>(memInfoBlock);
    return memInfoBlockHead->mstxFuseScopeDepth > 0;
}

/// 入参为gm指针，该函数主要功能为检测传入的gm指针是否为空;以及不为空时，该指针是否由__sanitizer_init接口申请得到
///主要作用是为了确保传入的指针为工具自己调用接口申请的指针，而不是由第三方传入，保证插桩后的算子不用工具能正常运行
AICORE_FUNC_HEAD bool MemInfoIsInvalid(__gm__ uint8_t *memInfo)
{
    if (memInfo == nullptr) {
        return true;
    }

#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    uint64_t headVal = *reinterpret_cast<__gm__ uint64_t *>(memInfo);
    return headVal != RECORD_HEAD_SECURITY_VALUE;
#else
    return false;
#endif
}

/// 无效的meminfo不再解析该条record
AICORE_FUNC_HEAD bool InvalidMemInfo(__gm__ uint8_t *memInfo)
{
    if (MemInfoIsInvalid(memInfo)) {
        return true;
    }

    return false;
}

AICORE_FUNC_HEAD uint64_t GetRecordHeadSize(uint32_t hostMemoryNum)
{
    return CeilByAlignSize<CACHE_LINE_SIZE>(sizeof(RecordBlockHead) + hostMemoryNum * sizeof(HostMemoryInfo));
}

/// 计算当前核信息写入memInfo时对应的索引和对应的blockType
AICORE_FUNC_HEAD uint64_t CalcDumpBlockIdx(BlockType &blockType, uint64_t blockIdx, uint8_t &vecSubBlockDim)
{
    uint64_t dumpIdx = blockIdx;

#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    int64_t coreId{};

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__) || \
    (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510))
#ifdef SIMT_MODE
    coreId = __cce_simt_get_COREID();
    vecSubBlockDim = __cce_simt_get_SUBBLOCKDIM();
#else
    coreId = get_coreid();
    vecSubBlockDim = get_subblockdim();
#endif // SIMT_MODE
#endif // DAV

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__)
    if ((coreId >= C220_A2_OR_A3_EVEN_DEVICE_VEC_PHYS_CORE_START_IDS &&
        coreId <= C220_A2_OR_A3_EVEN_DEVICE_VEC_PHYS_CORE_END_IDS) ||
        coreId >= C220_A3_ODD_DEVICE_VEC_PHYS_CORE_START_IDS) {
        blockType = BlockType::AIVEC;
        dumpIdx += blockIdx / (C220_MIX_SUB_BLOCKDIM - 1);
    } else {
        blockType = BlockType::AICUBE;
        dumpIdx += (blockIdx + 1) * (C220_MIX_SUB_BLOCKDIM - 1);
    }
#elif defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
    if ((coreId >= C310_A5_DEVICE_VEC_PHYS_SMALL_BOUND_CORE_START_IDS &&
        coreId <= C310_A5_DEVICE_VEC_PHYS_SMALL_BOUND_CORE_END_IDS) ||
        coreId >= C310_A5_DEVICE_VEC_PHYS_GREAT_BOUND_CORE_START_IDS) {
        blockType = BlockType::AIVEC;
        dumpIdx += blockIdx / (C220_MIX_SUB_BLOCKDIM - 1);
    } else {
        blockType = BlockType::AICUBE;
        dumpIdx += (blockIdx + 1) * (C220_MIX_SUB_BLOCKDIM - 1);
    }
#elif defined(__DAV_M200__) || defined(__DAV_M200_VEC__)
    blockType = BlockType::AICORE;
#endif // DAV

#endif // __CCE_IS_AICORE__
    return dumpIdx;
}

AICORE_FUNC_HEAD uint64_t CalcMemInfoOffset(__gm__ RecordGlobalHead *head, uint64_t dumpIdx,
    uint32_t hostMemoryNum, uint64_t &threadOffset)
{
    int16_t checkBlockId = head->checkParms.checkBlockId;
    uint32_t cacheSize = head->checkParms.cacheSize;
    uint64_t simdHeadSize = GetRecordHeadSize(hostMemoryNum);
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
    uint64_t threadId = GetThreadId();
    threadOffset = head->offsetInfo.simtHeadOffset +
        threadId * (head->offsetInfo.threadByteSize + sizeof(SimtRecordBlockHead));
#endif

    if (checkBlockId == CHECK_ALL_BLOCK) {
        return dumpIdx * (cacheSize * MB_TO_BYTES + simdHeadSize);
    }

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__) || \
    (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510))
    uint64_t vecTargetBlockIdx = checkBlockId / C220_VEC_SUB_BLOCKDIM * C220_MIX_SUB_BLOCKDIM +
        checkBlockId % C220_VEC_SUB_BLOCKDIM;
    uint64_t cubeTargetBlockIdx = checkBlockId * C220_MIX_SUB_BLOCKDIM + C220_VEC_SUB_BLOCKDIM;
#endif

    uint64_t offset{};
    for (size_t i = 0; i < dumpIdx; ++i) {
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__) || \
    (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510))
        if (i == vecTargetBlockIdx || i == cubeTargetBlockIdx) {
#else
        if (i == checkBlockId) {
#endif
            offset +=  cacheSize * MB_TO_BYTES + simdHeadSize;
        } else {
            offset += SINGLE_CHECK_OTHER_BLOCK_CACHE_SIZE * MB_TO_BYTES + simdHeadSize;
        }
    }
    return offset;
}

/* Recorder 记录写入类
 * 该类功能为将传入的记录写入 GM 上预分配的内存 memInfo 中。memInfo 内存总长度
 * 为 cache_size * blockDim，每个核使用独立的一块内存进行记录。
 * 调用方通过实例化时指定 blockIdx 使 Recorder 对指定核的记录进行写入，
 * Recorder 会根据内存头中的记录记数和偏移量将记录写入指定地址，并更新记数
 * 和偏移量。
 * 内存头中recordCount表示当前block中总的record数，recordWriteCount表示当前block
 * 写入到GM中的record数。写入超过最大上限时，recordWriteCount停止更新，recordCount
 * 继续更新
 *
 * 使用方法如下
 * @code
 * Recorder recorder(memInfo, blockIdx);
 * recorder.DumpRecord<RecordType::DMA_MOVE>(record);
 * 内存检测用法：
 * recorder.Check<RecordType::SIMT_LDG>(record);
 * @endcode
 */

class Recorder {
public:
    AICORE_FUNC_HEAD __attribute__((always_inline)) Recorder(__gm__ uint8_t *memInfo, uint64_t blockIdx) :
        memInfo_(memInfo), blockIdx_(blockIdx), check_()
    {
        if (MemInfoIsInvalid(memInfo)) {
            memInfoSimdBlock_ = nullptr;
            memInfoSimtBlock_ = nullptr;
            return;
        }
        auto globalHead = reinterpret_cast<__gm__ RecordGlobalHead *>(memInfo);
        auto simdBlockZeroHead = reinterpret_cast<__gm__ RecordBlockHead *>(globalHead + 1);
        BlockType blockType{};
        uint8_t vecSubBlockDim = 0;
        uint64_t dumpIdx = CalcDumpBlockIdx(blockType, blockIdx, vecSubBlockDim);
        /// 如果开启单核检测，sanitizer_init内存申请接口只会按照单核去申请内存，
        /// 则memInfoSimdBlock_记录的核数就等于blockOffset，不需要加上blockIdx，此时单核的记录都会记录到mix 0核的位置；
        uint64_t threadOffset{};
        memInfoSimdBlock_ = memInfo + sizeof(RecordGlobalHead) +
            CalcMemInfoOffset(globalHead, dumpIdx, simdBlockZeroHead->hostMemoryNum, threadOffset);
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
        memInfoSimtBlock_ = memInfoSimdBlock_ + threadOffset;
#endif
        auto memInfoBlockHead = reinterpret_cast<__gm__ RecordBlockHead *>(memInfoSimdBlock_);
        /// 后续增加新的字段，字段信息需要记录到memInfoBlockHead，记录到memInfoHead由于dcci的特性可能导致记录信息丢失
        memInfoBlockHead->blockInfo.vecSubBlockDim = vecSubBlockDim;
        memInfoBlockHead->blockInfo.blockType = blockType;
        check_.Init(memInfo, memInfoSimtBlock_, memInfoSimdBlock_, blockIdx_);
    }

    /* @tparam recordType 记录类型枚举
     * @tparam Record     记录结构体类型
     * @tparam Check      类型检查开关，默认打开
     * @param  record     要写入的记录
     * @brief 将记录写入 GM 预分配内存，recordType 作为记录头写入，用于标记后续
     *        写入的 Record 类型，使得解析时可以根据记录头正确解析 Record
     */
    template<RecordType recordType, typename Record, typename Check = record_type_check<true>>
    AICORE_FUNC_HEAD void DumpRecord(Record const &record);
    
    /* @param  type      fmatrix fmatrixB l3dRpt
     * @param  value     寄存器值
     * @brief 将需要的寄存器的值写入header
     */
    template<typename T>
    AICORE_FUNC_HEAD void SetRegister(T Register::*reg, T value) const;
 
    /* @param  type      fmatrix fmatrixB l3dRpt
     * @param  value     寄存器值
     * @brief 获取需要的寄存器的值
     */
    template<typename T>
    AICORE_FUNC_HEAD void GetRegister(T Register::*reg, T &value) const;

    AICORE_FUNC_HEAD void SetMstxFuseScope(bool inMstxFuseScope) const;

    /* @tparam recordType 记录类型枚举
     * @tparam Record     记录结构体类型
     * @tparam Check      类型检查开关，默认打开
     * @param  record     要写入的记录
     * @brief 将记录写入 GM 预分配内存，recordType 作为记录头写入，用于标记后续
     *        写入的 Record 类型，使得解析时可以根据记录头正确解析 Record
     */
    template<RecordType recordType, typename Record, typename Check = record_type_check<true>>
    AICORE_FUNC_HEAD void Check(Record const &record);

     /*
     * @brief 处理para base addr地址，将kernel入参地址写入到blockHead对应位置
     */
    AICORE_FUNC_HEAD void ProcessParaBaseAddr();

    template<RecordType recordType, typename Record>
    AICORE_FUNC_HEAD void UpdateSyncThreadCount(Record const &record);
    
    AICORE_FUNC_HEAD void SetParaBaseAddr(uint64_t size);

private:
    template<RecordType recordType, typename Record>
    AICORE_FUNC_HEAD void DumpSimdRecord(Record const &record);

    template<RecordType recordType, typename Record>
    AICORE_FUNC_HEAD void DumpSimtRecord(Record const &record);

private:
    __gm__ uint8_t *memInfoSimtBlock_ = nullptr;     // simt信息记录的位置
    __gm__ uint8_t *memInfoSimdBlock_ = nullptr;     // simd信息记录的位置
    __gm__ uint8_t *memInfo_ = nullptr;
    int16_t blockIdx_{};
    OnlineCheck check_;
};

template<RecordType recordType, typename Record, typename Check>
AICORE_FUNC_HEAD void Recorder::DumpRecord(Record const &record)
{
// 目前大概确认8.1-8.4, 9.1-9.4，11.1-11.4版本有问题，因此这些版本暂时去除检查
#if defined(__GNUC__) && (__GNUC__ == 8 || __GNUC__ == 9 || __GNUC__ == 11) && (__GNUC_MINOR__ <= 4)
#else
    // 在DumpRecord的开始添加编译期检查
    // 相比于放到模板参数里，放在这里可以实现更好的编译错误提示。
    // Check类型用于手动关闭类型检查，用类型而不是直接用bool是为了提高可读性
    static_assert((!Check::value) || is_record_match<recordType, Record>::value,
                  "The RecordType enum did not match with the actual Record Type.");
#endif

    if (memInfo_ == nullptr) {
        return;
    }

    if (!IsTargetIntrinsic<recordType>(memInfo_, blockIdx_, &record)) {
        return;
    }

    if (IsInMstxFuseScope(memInfoSimdBlock_)) {
        return;
    }

#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(SIMT_MODE)
    DumpSimtRecord<recordType>(record);
#else
    DumpSimdRecord<recordType>(record);
#endif
}

template<RecordType recordType, typename Record>
AICORE_FUNC_HEAD void Recorder::DumpSimdRecord(Record const &record)
{
    __gm__ RecordBlockHead *simdBlockHead = reinterpret_cast<__gm__ RecordBlockHead*>(memInfoSimdBlock_);
    uint64_t writeOffset = simdBlockHead->writeOffset;
    __gm__ RecordGlobalHead *globalHead = reinterpret_cast<__gm__ RecordGlobalHead *>(memInfo_);
    uint64_t stepSizes = sizeof(RecordType) + sizeof(Record);
    uint64_t simdEndOffset = globalHead->supportSimt ?
        globalHead->offsetInfo.simtHeadOffset : globalHead->checkParms.cacheSize * MB_TO_BYTES;
    if (writeOffset + CACHE_LINE_SIZE + stepSizes < simdEndOffset &&
        simdBlockHead->recordCount == simdBlockHead->recordWriteCount) {
        auto recordTypePtr = reinterpret_cast<__gm__ RecordType*>(memInfoSimdBlock_ +
            GetRecordHeadSize(simdBlockHead->hostMemoryNum) + writeOffset);
        *recordTypePtr = recordType;
        __gm__ Record *recordPtr = reinterpret_cast<__gm__ Record*>(recordTypePtr + 1);
        CopyRecordToGm(recordPtr, &record);
        simdBlockHead->writeOffset += stepSizes;
        simdBlockHead->recordWriteCount++;
    }
    simdBlockHead->recordCount++;
    simdBlockHead->offset += stepSizes;
    Flush(memInfoSimdBlock_);
}

template<RecordType recordType, typename Record>
AICORE_FUNC_HEAD void Recorder::DumpSimtRecord(Record const &record)
{
    __gm__ SimtRecordBlockHead *simtBlockHead = reinterpret_cast<__gm__ SimtRecordBlockHead*>(memInfoSimtBlock_);
    uint64_t writeOffset = simtBlockHead->writeOffset;
    __gm__ RecordGlobalHead *globalHead = reinterpret_cast<__gm__ RecordGlobalHead *>(memInfo_);
    uint64_t stepSizes = sizeof(RecordType) + sizeof(Record);
    if (writeOffset + CACHE_LINE_SIZE + stepSizes < globalHead->offsetInfo.threadByteSize &&
        simtBlockHead->recordCount == simtBlockHead->recordWriteCount) {
        auto recordTypePtr = reinterpret_cast<__gm__ RecordType*>(memInfoSimtBlock_ + sizeof(SimtRecordBlockHead) +
           writeOffset);
        *recordTypePtr = recordType;
        __gm__ Record *recordPtr = reinterpret_cast<__gm__ Record*>(recordTypePtr + 1);
        CopyRecordToGm(recordPtr, &record);
        simtBlockHead->writeOffset += stepSizes;
        simtBlockHead->recordWriteCount++;
    }
    simtBlockHead->recordCount++;
    simtBlockHead->offset += stepSizes;
    Flush(memInfoSimtBlock_);
}

template<typename T>
AICORE_FUNC_HEAD void Recorder::SetRegister(T Register::*reg, T value) const
{
    if (memInfo_ == nullptr) {
        return;
    }

    if (!IsTargetBlock(memInfo_, blockIdx_)) {
        return;
    }

    __gm__ RecordGlobalHead *globalHead = reinterpret_cast<__gm__ RecordGlobalHead *>(memInfo_);
    int64_t regIdx = GetRegisterIdx();
    if (!CheckRegIdxValid(regIdx)) {
        return;
    }
    globalHead->registers[regIdx].*reg = value;
    // 强制刷新 cacheline 数据写回 GM
    Flush(memInfo_);
}

template<typename T>
AICORE_FUNC_HEAD void Recorder::GetRegister(T Register::*reg, T &value) const
{
    if (memInfo_ == nullptr) {
        return;
    }

    if (!IsTargetBlock(memInfo_, blockIdx_)) {
        return;
    }

    __gm__ RecordGlobalHead *globalHead = reinterpret_cast<__gm__ RecordGlobalHead *>(memInfo_);
    int64_t regIdx = GetRegisterIdx();
    if (!CheckRegIdxValid(regIdx)) {
        return;
    }
    value = globalHead->registers[regIdx].*reg;
}

AICORE_FUNC_HEAD void Recorder::SetMstxFuseScope(bool inMstxFuseScope) const
{
    if (memInfoSimdBlock_ == nullptr) {
        return;
    }

    __gm__ RecordBlockHead *recordBlockHead = reinterpret_cast<__gm__ RecordBlockHead *>(memInfoSimdBlock_);
    if (inMstxFuseScope) {
        ++recordBlockHead->mstxFuseScopeDepth;
    } else {
        if (recordBlockHead->mstxFuseScopeDepth > 0) {
            --recordBlockHead->mstxFuseScopeDepth;
        }
    }
}

template<RecordType recordType, typename Record, typename Check>
AICORE_FUNC_HEAD void Recorder::Check(Record const &record)
{
    // 目前大概确认8.1-8.4, 9.1-9.4，11.1-11.4版本有问题，因此这些版本暂时去除检查
#if defined(__GNUC__) && (__GNUC__ == 8 || __GNUC__ == 9|| __GNUC__ == 11) && (__GNUC_MINOR__ <= 4)
#else
    // 在MemCheck的开始添加编译期检查
    // 相比于放到模板参数里，放在这里可以实现更好的编译错误提示。
    // Check类型用于手动关闭类型检查，用类型而不是直接用bool是为了提高可读性
    static_assert((!Check::value) || is_record_match<recordType, Record>::value,
                  "The RecordType enum did not match with the actual Record Type.");
#endif

    if (memInfo_ == nullptr) {
        return;
    }

    if (!IsTargetIntrinsic<recordType>(memInfo_, blockIdx_, &record)) {
        return;
    }

    check_.Process<recordType>(record);
}

AICORE_FUNC_HEAD void Recorder::ProcessParaBaseAddr()
{
    check_.ProcessParaBaseAddr();
}

template<RecordType recordType, typename Record>
AICORE_FUNC_HEAD void Recorder::UpdateSyncThreadCount(Record const &record)
{
    (void)recordType;
    (void)record;
    if (memInfo_ == nullptr) {
        return;
    }

#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(SIMT_MODE)
    auto memInfoBlockHead = reinterpret_cast<__gm__ RecordBlockHead *>(memInfoSimdBlock_);
    auto &blockInfo = memInfoBlockHead->blockInfo;
    AtomicAdd(&blockInfo.simtSyncThreadCount, 1);
    if (blockInfo.simtSyncThreadCount == blockInfo.threadXDim * blockInfo.threadYDim * blockInfo.threadZDim) {
        check_.ClearSyncThreadState();
        blockInfo.simtSyncThreadCount = 0;
    }
#endif
}

AICORE_FUNC_HEAD void Recorder::SetParaBaseAddr(uint64_t size)
{
    __gm__ RecordBlockHead *recordBlockHead = reinterpret_cast<__gm__ RecordBlockHead *>(memInfoSimdBlock_);
    recordBlockHead->paraBase.addr = size;
    Flush(memInfoSimdBlock_);
}

}  // namespace Sanitizer

#endif  // PLUGIN_RECORDER_H
