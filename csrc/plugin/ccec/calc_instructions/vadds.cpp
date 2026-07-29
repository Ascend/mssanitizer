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


#include "plugin/kernel_pub_func.h"
#include "plugin/utils.h"
#include "plugin/record_calc_instructions.h"

using namespace Sanitizer;

#if !(defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1 && defined(__DAV_C220_CUBE__))

// #2311
SANITIZER_REPORT(vadds, __ubuf__ half *dst, __ubuf__ half *src, half a, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VADDS>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2314
SANITIZER_REPORT(vadds, __ubuf__ half *dst, __ubuf__ half *src, half a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VADDS>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #2312
SANITIZER_REPORT(vadds, __ubuf__ half *dst, __ubuf__ half *src, half a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #2313
SANITIZER_REPORT(vadds, __ubuf__ half *dst, __ubuf__ half *src, half a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2315
SANITIZER_REPORT(vadds, __ubuf__ float *dst, __ubuf__ float *src, float a, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VADDS>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2318
SANITIZER_REPORT(vadds, __ubuf__ float *dst, __ubuf__ float *src, float a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VADDS>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #2316
SANITIZER_REPORT(vadds, __ubuf__ float *dst, __ubuf__ float *src, float a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #2317
SANITIZER_REPORT(vadds, __ubuf__ float *dst, __ubuf__ float *src0, float a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride,
    uint8_t srcRepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2319
SANITIZER_REPORT(vadds, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, int16_t a, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VADDS>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2322
SANITIZER_REPORT(vadds, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, int16_t a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VADDS>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #2320
SANITIZER_REPORT(vadds, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, int16_t a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #2321
SANITIZER_REPORT(vadds, __ubuf__ int16_t *dst, __ubuf__ int16_t *src0, int16_t a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride,
    uint8_t srcRepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2323
SANITIZER_REPORT(vadds, __ubuf__ int32_t *dst, __ubuf__ int32_t *src, int32_t a, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VADDS>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2326
SANITIZER_REPORT(vadds, __ubuf__ int32_t *dst, __ubuf__ int32_t *src, int32_t a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VADDS>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #2324
SANITIZER_REPORT(vadds, __ubuf__ int32_t *dst, __ubuf__ int32_t *src, int32_t a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #2325
SANITIZER_REPORT(vadds, __ubuf__ int32_t *dst, __ubuf__ int32_t *src0, int32_t a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride,
    uint8_t srcRepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

#endif
