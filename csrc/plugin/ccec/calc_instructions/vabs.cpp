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

// #2258
SANITIZER_REPORT(vabs, __ubuf__ half *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VABS>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2261
SANITIZER_REPORT(vabs, __ubuf__ half *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VABS>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #2259
SANITIZER_REPORT(vabs, __ubuf__ half *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #2260
SANITIZER_REPORT(vabs, __ubuf__ half *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2262
SANITIZER_REPORT(vabs, __ubuf__ float *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VABS>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2265
SANITIZER_REPORT(vabs, __ubuf__ float *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VABS>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #2263
SANITIZER_REPORT(vabs, __ubuf__ float *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #2264
SANITIZER_REPORT(vabs, __ubuf__ float *dst, __ubuf__ float *src0, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride,
    uint8_t srcRepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2266
SANITIZER_REPORT(vabs, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2269
SANITIZER_REPORT(vabs, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VABS>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #2267
SANITIZER_REPORT(vabs, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #2268
SANITIZER_REPORT(vabs, __ubuf__ int16_t *dst, __ubuf__ int16_t *src0, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride,
    uint8_t srcRepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

#endif
