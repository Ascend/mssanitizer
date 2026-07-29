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
// #3209
SANITIZER_REPORT(vshl, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, uint32_t shlDistance, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VSHL>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #3212
SANITIZER_REPORT(vshl, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, uint32_t shlDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VSHL>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #3210
SANITIZER_REPORT(vshl, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, uint32_t shlDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #3211
SANITIZER_REPORT(vshl, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, uint32_t shlDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #3213
SANITIZER_REPORT(vshl, __ubuf__ int32_t *dst, __ubuf__ int32_t *src, uint32_t shlDistance, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VSHL>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #3216
SANITIZER_REPORT(vshl, __ubuf__ int32_t *dst, __ubuf__ int32_t *src, uint32_t shlDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VSHL>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #3214
SANITIZER_REPORT(vshl, __ubuf__ int32_t *dst, __ubuf__ int32_t *src, uint32_t shlDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #3215
SANITIZER_REPORT(vshl, __ubuf__ int32_t *dst, __ubuf__ int32_t *src, uint32_t shlDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #3217
SANITIZER_REPORT(vshl, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src, uint32_t shlDistance, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VSHL>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #3220
SANITIZER_REPORT(vshl, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src, uint32_t shlDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VSHL>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #3218
SANITIZER_REPORT(vshl, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src, uint32_t shlDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #3219
SANITIZER_REPORT(vshl, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src, uint32_t shlDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #3221
SANITIZER_REPORT(vshl, __ubuf__ uint32_t *dst, __ubuf__ uint32_t *src, uint32_t shlDistance, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VSHL>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #3224
SANITIZER_REPORT(vshl, __ubuf__ uint32_t *dst, __ubuf__ uint32_t *src, uint32_t shlDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VSHL>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #3222
SANITIZER_REPORT(vshl, __ubuf__ uint32_t *dst, __ubuf__ uint32_t *src, uint32_t shlDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #3223
SANITIZER_REPORT(vshl, __ubuf__ uint32_t *dst, __ubuf__ uint32_t *src, uint32_t shlDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

#endif
