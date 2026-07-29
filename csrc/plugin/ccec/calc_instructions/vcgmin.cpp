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
// #2419
SANITIZER_REPORT(vcgmin, __ubuf__ half *dst, __ubuf__ half *src, uint64_t config)
{
    RecordReduceOpConfigFunc<RecordType::REDUCE_OP, InstrName::VCGMIN>(EXTRA_PARAMS, dst, src, config, 16, 1, 8, 16, 32, 16);
}

// #2420
SANITIZER_REPORT(vcgmin, __ubuf__ half *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride)
{
    RecordReduceOpFunc<RecordType::REDUCE_OP, InstrName::VCGMIN>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, 16, 1, 8, 16, 32, 16);
}

// #2421
SANITIZER_REPORT(vcgmin, __ubuf__ half *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    RecordReduceOpFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, 16, 1, 8, 16, 32, 16);
}

// #2422
SANITIZER_REPORT(vcgmin, __ubuf__ float *dst, __ubuf__ float *src, uint64_t config)
{
    RecordReduceOpConfigFunc<RecordType::REDUCE_OP, InstrName::VCGMIN>(EXTRA_PARAMS, dst, src, config, 32, 1, 8, 32, 32, 32);
}

// #2423
SANITIZER_REPORT(vcgmin, __ubuf__ float *dst, __ubuf__ float *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride)
{
    RecordReduceOpFunc<RecordType::REDUCE_OP, InstrName::VCGMIN>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, 32, 1, 8, 32, 32, 32);
}
// #2424
SANITIZER_REPORT(vcgmin, __ubuf__ float *dst, __ubuf__ float *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    RecordReduceOpFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, 32, 1, 8, 32, 32, 32);
}

#endif
