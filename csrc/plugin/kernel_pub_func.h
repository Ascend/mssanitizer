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

#ifndef PLUGIN_KERNEL_PUB_FUNC_H
#define PLUGIN_KERNEL_PUB_FUNC_H

#include "core/framework/record_defs.h"
#include "ccec_defines.h"
#include "utils.h"

namespace Sanitizer {

AICORE_FUNC_HEAD uint64_t GetBlockIdx()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1 // AICORE

#if (defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__))
    return get_block_idx() * get_subblockdim() + get_subblockid();
#elif defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) // c310
    #if defined(__DAV_VEC__) && defined(SIMT_MODE) // c310-simt
        return __cce_simt_get_BLOCKID();
    #else                                         
        int64_t coreId = get_coreid();
        if ((coreId >= C310_A5_DEVICE_VEC_PHYS_SMALL_BOUND_CORE_START_IDS &&
            coreId <= C310_A5_DEVICE_VEC_PHYS_SMALL_BOUND_CORE_END_IDS) ||
            coreId >= C310_A5_DEVICE_VEC_PHYS_GREAT_BOUND_CORE_START_IDS) {    // c310-vec
            return get_block_idx() * get_subblockdim() + get_subblockid();
        } else {                                                               // c310-cube
            return get_block_idx();
        }
    #endif // SIMT_MODE
#else // NOT C220 C310
    return get_block_idx();
#endif // __DAV

#else // NOT AICORE
    return 0;
#endif
}

AICORE_FUNC_HEAD uint64_t GetSysVaBase()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1 // AICORE

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__) || \
    (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510))
    using namespace __cce_scalar;
    return get_sys_va_base();
#else
    return 0;
#endif

#else // NOT AICORE
    return 0;
#endif
}

AICORE_FUNC_HEAD uint64_t GetStackPhyBase()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1 // AICORE

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__) || \
    (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510))
    using namespace __cce_scalar;
    return get_stack_phy_base();
#else
    return 0;
#endif

#else // NOT AICORE
    return 0;
#endif
}

template<uint8_t shift, uint8_t width>
AICORE_FUNC_HEAD uint64_t GetIntFromConf(uint64_t config)
{
    constexpr uint64_t leftBit = 1;
    static_assert(shift < 64 && width <= 64);
    return (config >> shift) & ((leftBit << width) - 1);
}

AICORE_FUNC_HEAD uint16_t GetThreadIdX()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    return __cce_simt_get_TID_X();
#endif // AICORE
#endif // SIMT_MODE
    return 0;
}

AICORE_FUNC_HEAD uint16_t GetThreadIdY()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    return __cce_simt_get_TID_Y();
#endif // AICORE
#endif // SIMT_MODE
    return 0;
}

AICORE_FUNC_HEAD uint16_t GetThreadIdZ()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    return __cce_simt_get_TID_Z();
#endif // AICORE
#endif // SIMT_MODE
    return 0;
}

/// x/y/z一维展开，从0开始计数
AICORE_FUNC_HEAD uint16_t GetThreadId()
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    int32_t blockDimX = __cce_simt_get_BLOCK_DIM_X();
    int32_t blockDimY = __cce_simt_get_BLOCK_DIM_Y();
    int32_t threadIdX = GetThreadIdX();
    int32_t threadIdY = GetThreadIdY();
    int32_t threadIdZ = GetThreadIdZ();
    return threadIdX + blockDimX * threadIdY + blockDimX * blockDimY * threadIdZ;
#endif // AICORE
#endif // SIMT_MODE
    return 0;
}

/// 把threadId按三维展开为(x,y,z)
AICORE_FUNC_HEAD void DecomposeThreadId(uint16_t threadId, uint16_t &x, uint16_t &y, uint16_t &z)
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    int32_t blockDimX = __cce_simt_get_BLOCK_DIM_X();
    int32_t blockDimY = __cce_simt_get_BLOCK_DIM_Y();
    int32_t blockDimZ = __cce_simt_get_BLOCK_DIM_Z();
    x = threadId % blockDimX;
    y = (threadId % (blockDimX * blockDimY)) / blockDimX;
    z = threadId / (blockDimX * blockDimY);
    return;
#endif // AICORE
#endif // SIMT_MODE
    x = 0U;
    y = 0U;
    z = 0U;
}

/// 获取thread各维度大小
AICORE_FUNC_HEAD void GetThreadDim(uint16_t &x, uint16_t &y, uint16_t &z)
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    x = __cce_simt_get_BLOCK_DIM_X();
    y = __cce_simt_get_BLOCK_DIM_Y();
    z = __cce_simt_get_BLOCK_DIM_Z();
    return;
#endif // AICORE
#endif // SIMT_MODE
    x = 0U;
    y = 0U;
    z = 0U;
}

// atomicCAS/atomicExch接口支持 uint32_t/int32_t/uint64_t/int64_t/float/half2/bfloat16x2_t
// 因shadow memory中仅使用了uint64_t类型的atomicCAS，因此仅声明uint64_t类型的wrapper函数，其他atomic接口同理
AICORE_FUNC_HEAD uint64_t AtomicCAS(__gm__ uint64_t *gmAddr, uint64_t compare, uint64_t val)
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__) && defined(SIMT_MODE)
    return atomicCAS(gmAddr, compare, val);
#endif // AICORE
#endif // SIMT_MODE
    return compare;
}

AICORE_FUNC_HEAD uint64_t AtomicExch(__gm__ uint64_t *gmAddr, uint64_t val)
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__) && defined(SIMT_MODE)
    return atomicExch(gmAddr, val);
#endif // AICORE
#endif // SIMT_MODE
    return 0U;
}

AICORE_FUNC_HEAD uint64_t AtomicAdd(__gm__ uint64_t *gmAddr, uint64_t val)
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__) && defined(SIMT_MODE)
    return atomicAdd(gmAddr, val);
#endif // AICORE
#endif // SIMT_MODE
    return 0U;
}

template<uint32_t alignSize>
AICORE_FUNC_HEAD uint32_t CeilByAlignSize(uint32_t v)
{
    static_assert(alignSize != 0, "align size cannot be zero");
    return ((v + alignSize - 1) / alignSize) * alignSize;
}

 /* @tparam Record             记录结构体类型
  * @param  gmRecord        GM上要写入的记录地址
  * @param  stackRecord     stack上已经写入的记录地址
  * @brief 将栈上已经写入的记录信息赋值到GM上
*/
 template<typename Record>
AICORE_FUNC_HEAD void CopyRecordToGm(__gm__ Record *gmRecord, Record const *stackRecord)
{
    uint16_t iterations = sizeof(Record) / sizeof(uint32_t);
    uint8_t tail = sizeof(Record) - sizeof(uint32_t) * iterations;
    __gm__  uint32_t* gmBlockHead = reinterpret_cast<__gm__  uint32_t*>(gmRecord);
    uint32_t const *stackBlockHead = reinterpret_cast<uint32_t const*>(stackRecord);
    for (uint16_t i = 0; i < iterations; i++) {
        *gmBlockHead++ = *stackBlockHead++;
    }

    __gm__  uint8_t* gmBlockTail = reinterpret_cast<__gm__ uint8_t*>(gmBlockHead);
    uint8_t const *stackBlockTail = reinterpret_cast<uint8_t const*>(stackBlockHead);
    for (uint8_t i = 0; i < tail; i++) {
        *gmBlockTail++ = *stackBlockTail++;
    }
}

/* @brief 强制将 cacheline 上的数据刷回 GM 内存，防止因缓存一致性问题导致的记录丢失
 *        一些硬件的标量处理器在通过标量 Load/Store 的方式访问 GM 或 UB 上的数据时，
 *        会通过 cacheline 进行访问加速。当记录写入时，记录可能只保存于 cacheline
 *        中，此时如果核函数异常退出，cacheline 中的数据可能未写回 GM 导致数据丢失。
 * @param gm 此处需要传入一个 GM 地址，为保证正确性使用 ENTIRE_DATA_CACHE 模式进行刷
 *        新，因此地址值本身不重要
*/
AICORE_FUNC_HEAD void Flush(__gm__ uint8_t *gm)
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
#if (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510))
        using namespace __cce_scalar;
        dcci((__gm__ uint64_t*)gm, ENTIRE_DATA_CACHE, CACHELINE_ALL);
#else
        dcci(gm, ENTIRE_DATA_CACHE);
#endif
#endif
}

// 目前 simt 指令对 GM/UB/STACK 的访问都没有发现特殊对齐逻辑，先按数据类型对齐
AICORE_FUNC_HEAD uint32_t GetAlignSizeByDataType(DetailedDataType dataType)
{
    switch (dataType) {
        case DetailedDataType::B4:
        case DetailedDataType::E1M2:
        case DetailedDataType::E2M1:
        case DetailedDataType::B8:
        case DetailedDataType::S8:
        case DetailedDataType::U8:
        case DetailedDataType::E4M3:
        case DetailedDataType::E5M2:
            return 1;
        case DetailedDataType::B16:
        case DetailedDataType::S16:
        case DetailedDataType::U16:
        case DetailedDataType::F16:
        case DetailedDataType::HALF:
        case DetailedDataType::BF16:
            return 2;
        case DetailedDataType::B32:
        case DetailedDataType::S32:
        case DetailedDataType::U32:
        case DetailedDataType::FLOAT:
        case DetailedDataType::F16X2:
        case DetailedDataType::BF16X2:
        case DetailedDataType::F32:
            return 4;
        case DetailedDataType::B64:
        case DetailedDataType::S64:
        case DetailedDataType::U64:
            return 8;
        case DetailedDataType::B128:
            return 16;
        default:
            return 1;
    }
}

AICORE_FUNC_HEAD uint64_t GetDataBits(DetailedDataType type)
{
    switch (type) {
        case DetailedDataType::B4:
        case DetailedDataType::E1M2:
        case DetailedDataType::E2M1:
            return 4UL;
        case DetailedDataType::B8:
        case DetailedDataType::S8:
        case DetailedDataType::U8:
        case DetailedDataType::E4M3:
        case DetailedDataType::E5M2:
            return 8UL;
        case DetailedDataType::B16:
        case DetailedDataType::S16:
        case DetailedDataType::U16:
        case DetailedDataType::F16:
        case DetailedDataType::HALF:
        case DetailedDataType::BF16:
            return 16UL;
        case DetailedDataType::B32:
        case DetailedDataType::S32:
        case DetailedDataType::U32:
        case DetailedDataType::FLOAT:
        case DetailedDataType::F16X2:
        case DetailedDataType::BF16X2:
        case DetailedDataType::F32:
            return 32UL;
        case DetailedDataType::B64:
        case DetailedDataType::S64:
        case DetailedDataType::U64:
            return 64UL;
        case DetailedDataType::B128:
        default:
            return 128UL;
    }
}

/// 不开启竞争检测时，过滤掉同步指令记录
AICORE_FUNC_HEAD bool DoRaceCheck(__gm__ uint8_t *memInfo)
{
    auto head = reinterpret_cast<__gm__ RecordGlobalHead *>(memInfo);
    return head->checkParms.racecheck;
}

/// 同步检测当前需要判断冗余，需要其他类型的pipe指令
AICORE_FUNC_HEAD bool DoSyncCheck(__gm__ uint8_t *memInfo)
{
    auto head = reinterpret_cast<__gm__ RecordGlobalHead *>(memInfo);
    return head->checkParms.synccheck;
}

AICORE_FUNC_HEAD bool DoMemCheck(__gm__ uint8_t *memInfo)
{
    auto head = reinterpret_cast<__gm__ RecordGlobalHead *>(memInfo);
    return head->checkParms.defaultcheck;
}

/// 同步检测当前需要判断冗余，需要其他类型的pipe指令
AICORE_FUNC_HEAD bool DoRegisterCheck(__gm__ uint8_t *memInfo)
{
    auto head = reinterpret_cast<__gm__ RecordGlobalHead *>(memInfo);
    return head->checkParms.registerCheck;
}

} // namespace Sanitizer

#endif  // PLUGIN_KERNEL_PUB_FUNC_H
