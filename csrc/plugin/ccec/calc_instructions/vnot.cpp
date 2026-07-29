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
// #3127
SANITIZER_REPORT(vnot, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VNOT>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #3130
SANITIZER_REPORT(vnot, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VNOT>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #3128
SANITIZER_REPORT(vnot, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #3129
SANITIZER_REPORT(vnot, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #3131
SANITIZER_REPORT(vnot, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VNOT>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #3134
SANITIZER_REPORT(vnot, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VNOT>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #3132
SANITIZER_REPORT(vnot, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #3133
SANITIZER_REPORT(vnot, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src0, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride,
    uint8_t srcRepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

#endif
