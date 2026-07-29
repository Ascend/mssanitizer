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

#include "plugin/record_calc_instructions.h"

using namespace Sanitizer;
#if !(defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1 && defined(__DAV_C220_CUBE__))
// #3008
SANITIZER_REPORT(vmaddrelu, __ubuf__ half *dst, __ubuf__ half *src0, __ubuf__ half *src1, uint64_t config)
{
    RecordBinaryOpConfigFunc<RecordType::TERNARY_OP, InstrName::VMADDRELU>(EXTRA_PARAMS, dst, src0, src1, config, 8, 8, 8, 32, 32, 32);
}

// #3009
SANITIZER_REPORT(vmaddrelu, __ubuf__ half *dst, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat, \
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride, \
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    RecordBinaryOpFunc<RecordType::TERNARY_OP, InstrName::VMADDRELU>(EXTRA_PARAMS, dst, src0, src1, repeat, \
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        8, 8, 8, 32, 32, 32);
}

// #3010
SANITIZER_REPORT(vmaddrelu, __ubuf__ half *dst, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat, \
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride, \
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    RecordBinaryOpFunc<RecordType::TERNARY_OP>(EXTRA_PARAMS, dst, src0, src1, repeat, \
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        8, 8, 8, 32, 32, 32);
}

// #3011
SANITIZER_REPORT(vmaddrelu, __ubuf__ float *dst, __ubuf__ float *src0, __ubuf__ float *src1, uint64_t config)
{
    RecordBinaryOpConfigFunc<RecordType::TERNARY_OP, InstrName::VMADDRELU>(EXTRA_PARAMS, dst, src0, src1, config, 8, 8, 8, 32, 32, 32);
}

// #3012
SANITIZER_REPORT(vmaddrelu, __ubuf__ float *dst, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat, \
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride, \
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    RecordBinaryOpFunc<RecordType::TERNARY_OP, InstrName::VMADDRELU>(EXTRA_PARAMS, dst, src0, src1, repeat, \
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        8, 8, 8, 32, 32, 32);
}

// #3013
SANITIZER_REPORT(vmaddrelu, __ubuf__ float *dst, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat, \
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride, \
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    RecordBinaryOpFunc<RecordType::TERNARY_OP>(EXTRA_PARAMS, dst, src0, src1, repeat, \
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        8, 8, 8, 32, 32, 32);
}
#endif
