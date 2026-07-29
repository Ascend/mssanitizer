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

// #2333
SANITIZER_REPORT(vaxpy, __ubuf__ half *dst, __ubuf__ half *src, half a, uint64_t config)
{
    auto repeat = uint8_t{};
    auto dstBlockStride = uint16_t{};
    auto srcBlockStride = uint16_t{};
    auto dstRepeatStride = uint16_t{};
    auto srcRepeatStride = uint16_t{};
    ParseUnaryConfigByArch(config, repeat, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
    RecordBinaryOpFunc<RecordType::BINARY_OP, InstrName::VAXPY>(EXTRA_PARAMS, dst, src, dst, repeat,
        dstBlockStride, srcBlockStride, dstBlockStride, dstRepeatStride, srcRepeatStride,
        dstRepeatStride, 8, 8, 8, 32, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2336
SANITIZER_REPORT(vaxpy, __ubuf__ half *dst, __ubuf__ half *src, half a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordBinaryOpFunc<RecordType::BINARY_OP, InstrName::VAXPY>(EXTRA_PARAMS, dst, src, dst, repeat,
        dstBlockStride, srcBlockStride, dstBlockStride, dstRepeatStride, srcRepeatStride,
        dstRepeatStride, 8, 8, 8, 32, 32, 32);
}
#else
// #2334
SANITIZER_REPORT(vaxpy, __ubuf__ half *dst, __ubuf__ half *src, half a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, dst, src, dst, repeat,
        dstBlockStride, srcBlockStride, dstBlockStride, dstRepeatStride, srcRepeatStride,
        dstRepeatStride, 8, 8, 8, 32, 32, 32);
}
#endif

// #2335
SANITIZER_REPORT(vaxpy, __ubuf__ half *dst, __ubuf__ half *src, half a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, dst, src, dst, repeat,
        dstBlockStride, srcBlockStride, dstBlockStride, dstRepeatStride, srcRepeatStride,
        dstRepeatStride, 8, 8, 8, 32, 32, 32);
}

// #2337
SANITIZER_REPORT(vaxpy, __ubuf__ float *dst, __ubuf__ float *src, float a, uint64_t config)
{
    auto repeat = uint8_t{};
    auto dstBlockStride = uint16_t{};
    auto srcBlockStride = uint16_t{};
    auto dstRepeatStride = uint16_t{};
    auto srcRepeatStride = uint16_t{};
    ParseUnaryConfigByArch(config, repeat, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
    RecordBinaryOpFunc<RecordType::BINARY_OP, InstrName::VAXPY>(EXTRA_PARAMS, dst, src, dst, repeat,
        dstBlockStride, srcBlockStride, dstBlockStride, dstRepeatStride, srcRepeatStride,
        dstRepeatStride, 8, 8, 8, 32, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2340
SANITIZER_REPORT(vaxpy, __ubuf__ float *dst, __ubuf__ float *src, float a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordBinaryOpFunc<RecordType::BINARY_OP, InstrName::VAXPY>(EXTRA_PARAMS, dst, src, dst, repeat,
        dstBlockStride, srcBlockStride, dstBlockStride, dstRepeatStride, srcRepeatStride,
        dstRepeatStride, 8, 8, 8, 32, 32, 32);
}
#else
// #2338
SANITIZER_REPORT(vaxpy, __ubuf__ float *dst, __ubuf__ float *src, float a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, dst, src, dst, repeat,
        dstBlockStride, srcBlockStride, dstBlockStride, dstRepeatStride, srcRepeatStride,
        dstRepeatStride, 8, 8, 8, 32, 32, 32);
}
#endif

// #2339
SANITIZER_REPORT(vaxpy, __ubuf__ float *dst, __ubuf__ float *src, float a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, dst, src, dst, repeat,
        dstBlockStride, srcBlockStride, dstBlockStride, dstRepeatStride, srcRepeatStride,
        dstRepeatStride, 8, 8, 8, 32, 32, 32);
}

// #2341
SANITIZER_REPORT(vaxpy, __ubuf__ float *dst, __ubuf__ half *src, half a, uint64_t config)
{
    auto repeat = uint8_t{};
    auto dstBlockStride = uint16_t{};
    auto srcBlockStride = uint16_t{};
    auto dstRepeatStride = uint16_t{};
    auto srcRepeatStride = uint16_t{};
    ParseUnaryConfigByArch(config, repeat, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
    RecordBinaryOpFunc<RecordType::BINARY_OP, InstrName::VAXPY>(EXTRA_PARAMS, dst, src, dst, repeat,
        dstBlockStride, srcBlockStride, dstBlockStride, dstRepeatStride, srcRepeatStride,
        dstRepeatStride, 8, 4, 8, 32, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2344
SANITIZER_REPORT(vaxpy, __ubuf__ float *dst, __ubuf__ half *src, half a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordBinaryOpFunc<RecordType::BINARY_OP, InstrName::VAXPY>(EXTRA_PARAMS, dst, src, dst, repeat,
        dstBlockStride, srcBlockStride, dstBlockStride, dstRepeatStride, srcRepeatStride,
        dstRepeatStride, 8, 4, 8, 32, 32, 32);
}
#else
// #2342
SANITIZER_REPORT(vaxpy, __ubuf__ float *dst, __ubuf__ half *src, half a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, dst, src, dst, repeat,
        dstBlockStride, srcBlockStride, dstBlockStride, dstRepeatStride, srcRepeatStride,
        dstRepeatStride, 8, 4, 8, 32, 32, 32);
}
#endif

// #2343
SANITIZER_REPORT(vaxpy, __ubuf__ float *dst, __ubuf__ half *src, half a, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, dst, src, dst, repeat,
        dstBlockStride, srcBlockStride, dstBlockStride, dstRepeatStride, srcRepeatStride,
        dstRepeatStride, 8, 4, 8, 32, 32, 32);
}

#endif
