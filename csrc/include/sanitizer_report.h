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

#pragma once
#ifndef MSSANITIZER_SANITIZER_REPORT_H
#define MSSANITIZER_SANITIZER_REPORT_H

#if !(defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1)
#include <cstdint>
#endif

namespace Sanitizer {

constexpr std::size_t MSTX_API_NAME_LENGTH = 64UL;

enum class InterfaceType : uint32_t {
    MSTX_SET_CROSS_SYNC = 0,
    MSTX_WAIT_CROSS_SYNC,
    MSTX_HCCL,
    MSTX_HCCLV,
    MSTX_CROSS_CORE_BARRIER = 4,
    MSTX_CROSS_CORE_SET_FLAG,
    MSTX_CROSS_CORE_WAIT_FLAG,
    MSTX_SIGNAL_SET,
    MSTX_SIGNAL_WAIT,
    MSTX_CROSS_NPU_BARRIER,

    MSTX_FUSE_SCOPE_START = 1000,  // 融合语义范围开始标记，范围内的指令记录会被忽略
    MSTX_FUSE_SCOPE_END,           // 融合语义范围结束标记

    MSTX_VEC_UNARY_OP = 3000,
    MSTX_VEC_BINARY_OP,

    MSTX_DATA_COPY = 4001,
    MSTX_DATA_COPY_PAD,
    MSTX_WITH_TENSOR,
};

enum class AddressSpace : int32_t {
    PRIVATE = 0,
    GM,
    L1,
    L0A,
    L0B,
    L0C,
    UB,
    BT,         // bias table
    FB,         // fixPipe buffer
    INVALID = -1,
};

enum class MaskMode : uint8_t {
    MASK_NORM = 0U,
    MASK_COUNT,
};

enum class CompareOp {
    EQ = 0,
    NE,
    GT,
    GE,
    LT,
    LE
};

struct VectorMask {
    uint64_t mask0;
    uint64_t mask1;
};

struct MstxCrossCoreBarrier {
    uint32_t usedCoreNum;
    uint32_t *usedCoreId;
    bool isAIVOnly;
    bool pipeBarrierAll;
};

struct MstxCrossCoreSetFlag {
    int32_t eventId;
    int32_t peerCoreId;
    bool pipeBarrierAll;
};

struct MstxSignalSet {
    uint64_t addr;
    int64_t value;
};

struct MstxSignalWait {
    uint64_t addr;
    int64_t cmpValue;
    CompareOp cmpOp;
};

struct MstxCrossCoreWaitFlag {
    int32_t eventId;
    int32_t peerCoreId;
    bool pipeBarrierAll;
};

struct MstxCrossNpuBarrier {
    uint32_t usedDeviceNum;
    uint32_t *usedDeviceId;
    uint32_t usedCoreNum;
    uint32_t *usedCoreId;
    bool isAIVOnly;
    bool pipeBarrierAll;
};

struct MstxTensorDesc {
    AddressSpace space;
    uint64_t addr;
    uint64_t size;
    uint8_t dataBits;
};

struct MstxVecWrapper {
    MaskMode maskMode;
    VectorMask mask;
    uint32_t reserveBufSize;
    bool useMask;
};

struct MstxVecUnaryDesc {
    MstxTensorDesc dst;
    MstxTensorDesc src;
    MstxVecWrapper wrapper;
    uint32_t blockNum;
    uint32_t dstBlockStride;
    uint32_t srcBlockStride;
    uint32_t repeatTimes;
    uint32_t dstRepeatStride;
    uint32_t srcRepeatStride;
    char name[MSTX_API_NAME_LENGTH];
};

struct MstxVecBinaryDesc {
    MstxTensorDesc dst;
    MstxTensorDesc src0;
    MstxTensorDesc src1;
    MstxVecWrapper wrapper;
    uint32_t blockNum;
    uint32_t dstBlockStride;
    uint32_t src0BlockStride;
    uint32_t src1BlockStride;
    uint32_t repeatTimes;
    uint32_t dstRepeatStride;
    uint32_t src0RepeatStride;
    uint32_t src1RepeatStride;
    char name[MSTX_API_NAME_LENGTH];
};

struct MstxDataCopyDesc {
    MstxTensorDesc dst;
    MstxTensorDesc src;
    uint32_t lenBurst;
    uint32_t nBurst;
    uint32_t srcGap;
    uint32_t dstGap;
    char name[MSTX_API_NAME_LENGTH];
};

struct MstxDataCopyPadDesc {
    MstxTensorDesc dst;
    MstxTensorDesc src;
    uint32_t lenBurst;
    uint32_t nBurst;
    uint32_t srcGap;
    uint32_t dstGap;
    uint32_t leftPad;
    uint32_t rightPad;
    char name[MSTX_API_NAME_LENGTH];
};

template<typename RecordT>
struct InterfaceTypeTraits {};

#define INTERFACE_TYPE_TRAITS_SPEC(RecordT, interfaceType)     \
    template<>                                                 \
    struct InterfaceTypeTraits<RecordT> {                      \
        constexpr static InterfaceType value = interfaceType;  \
    }

INTERFACE_TYPE_TRAITS_SPEC(MstxCrossCoreBarrier, InterfaceType::MSTX_CROSS_CORE_BARRIER);
INTERFACE_TYPE_TRAITS_SPEC(MstxCrossCoreSetFlag, InterfaceType::MSTX_CROSS_CORE_SET_FLAG);
INTERFACE_TYPE_TRAITS_SPEC(MstxCrossCoreWaitFlag, InterfaceType::MSTX_CROSS_CORE_WAIT_FLAG);
INTERFACE_TYPE_TRAITS_SPEC(MstxSignalSet, InterfaceType::MSTX_SIGNAL_SET);
INTERFACE_TYPE_TRAITS_SPEC(MstxSignalWait, InterfaceType::MSTX_SIGNAL_WAIT);
INTERFACE_TYPE_TRAITS_SPEC(MstxCrossNpuBarrier, InterfaceType::MSTX_CROSS_NPU_BARRIER);
INTERFACE_TYPE_TRAITS_SPEC(MstxVecUnaryDesc, InterfaceType::MSTX_VEC_UNARY_OP);
INTERFACE_TYPE_TRAITS_SPEC(MstxVecBinaryDesc, InterfaceType::MSTX_VEC_BINARY_OP);
INTERFACE_TYPE_TRAITS_SPEC(MstxDataCopyDesc, InterfaceType::MSTX_DATA_COPY);
INTERFACE_TYPE_TRAITS_SPEC(MstxDataCopyPadDesc, InterfaceType::MSTX_DATA_COPY_PAD);

#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1

/**
 * @ingroup MSSANITIZER_REPORT
 * @brief report mstx varadic record
 *
 * @param record        [IN] record payload
 */
template<typename RecordT>
inline [aicore] void SanitizerReport(RecordT const &record)
{
#ifdef __MSTX_DFX_REPORT__
    constexpr static InterfaceType interfaceType = InterfaceTypeTraits<RecordT>::value;
    __mstx_dfx_report_stub(interfaceType, sizeof(record), &record);
#endif // __MSTX_DFX_REPORT__
}

/**
 * @ingroup MSSANITIZER_REPORT
 * @brief report fuse scope start mark
 */
inline [aicore] void SanitizerFuseScopeStart()
{
#ifdef __MSTX_DFX_REPORT__
    __mstx_dfx_report_stub(InterfaceType::MSTX_FUSE_SCOPE_START, 0, nullptr);
#endif // __MSTX_DFX_REPORT__
}

/**
 * @ingroup MSSANITIZER_REPORT
 * @brief report fuse scope end mark
 */
inline [aicore] void SanitizerFuseScopeEnd()
{
#ifdef __MSTX_DFX_REPORT__
    __mstx_dfx_report_stub(InterfaceType::MSTX_FUSE_SCOPE_END, 0, nullptr);
#endif // MSSANITIZER_REPORT_REPORT_H
}

#endif // __CCE_IS_AICORE__

} // namespace Sanitizer

#endif // MSSANITIZER_SANITIZER_REPORT_H