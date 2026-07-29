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
// #3225
SANITIZER_REPORT(vshr, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, int32_t shrDistance, uint64_t config, bool round)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VSHR>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #3226
SANITIZER_REPORT(vshr, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, int32_t shrDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode, bool round)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #3227
SANITIZER_REPORT(vshr, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, int32_t shrDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride,
    bool round)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VSHR>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #3228
SANITIZER_REPORT(vshr, __ubuf__ int32_t *dst, __ubuf__ int32_t *src, int32_t shrDistance, uint64_t config, bool round)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VSHR>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #3229
SANITIZER_REPORT(vshr, __ubuf__ int32_t *dst, __ubuf__ int32_t *src, int32_t shrDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode, bool round)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #3230
SANITIZER_REPORT(vshr, __ubuf__ int32_t *dst, __ubuf__ int32_t *src, int32_t shrDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride,
    bool round)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VSHR>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #3231
SANITIZER_REPORT(vshr, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src, uint32_t shrDistance,
    uint64_t config, bool round)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VSHR>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #3232
SANITIZER_REPORT(vshr, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src, uint32_t shrDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode, bool round)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #3233
SANITIZER_REPORT(vshr, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src, uint32_t shrDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride,
    bool round)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VSHR>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #3234
SANITIZER_REPORT(vshr, __ubuf__ uint32_t *dst, __ubuf__ uint32_t *src, uint32_t shrDistance,
    uint64_t config, bool round)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VSHR>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #3235
SANITIZER_REPORT(vshr, __ubuf__ uint32_t *dst, __ubuf__ uint32_t *src, uint32_t shrDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode, bool round)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #3236
SANITIZER_REPORT(vshr, __ubuf__ uint32_t *dst, __ubuf__ uint32_t *src, uint32_t shrDistance, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride,
    bool round)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VSHR>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

#endif
