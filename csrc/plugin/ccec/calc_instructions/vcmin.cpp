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

// #2446
SANITIZER_REPORT(vcmin, __ubuf__ half *dst, __ubuf__ half *src, uint64_t config)
{
    RecordReduceOpConfigFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, config, 2, 1, 8, 2, 32, 4);
}

// #2447
SANITIZER_REPORT(vcmin, __ubuf__ half *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride)
{
    RecordReduceOpFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, 2, 1, 8, 2, 32, 4);
}

// #2448
SANITIZER_REPORT(vcmin, __ubuf__ half *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    RecordReduceOpFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, 2, 1, 8, 2, 32, 4);
}

#if defined(__DAV_M200_VEC__) || defined(__DAV_M200__)
// #2449
SANITIZER_REPORT(vcmin, __ubuf__ half *dst, __ubuf__ half *src, uint64_t config, bool order)
{
    RecordReduceOpConfigFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, config, 2, 1, 8, 2, 32, 4);
}

// #2450
SANITIZER_REPORT(vcmin, __ubuf__ half *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride, bool repeatStrideMode, bool strideSizeMode, bool order)
{
    RecordReduceOpFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, 2, 1, 8, 2, 32, 4);
}

#else
// #2451
SANITIZER_REPORT(vcmin, __ubuf__ half *dst, __ubuf__ half *src, uint64_t config, Order_t order)
{
    auto repeat = uint8_t{};
    auto dstRepeatStride = uint16_t{};
    auto srcBlockStride = uint16_t{};
    auto srcRepeatStride = uint16_t{};
    uint8_t dataBitsFactor = 1;
    uint16_t dstAddrAlignUnit{};
    ParseReduceConfig(config, repeat, dstRepeatStride, srcBlockStride, srcRepeatStride);
    auto dstRepeatLength = uint16_t{};
    auto enOrder = static_cast<OrderType>(order);
    ParseOrder<half>(enOrder, dstRepeatLength, dataBitsFactor, dstAddrAlignUnit);
    RecordReduceOpFunc<RecordType::REDUCE_OP, InstrName::VCMIN>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, dstRepeatLength, 1, 8, dstRepeatLength, 32, dstAddrAlignUnit,
        dataBitsFactor);
}
// #2452
SANITIZER_REPORT(vcmin, __ubuf__ half *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride, Order_t order)
{
    auto dstRepeatLength = uint16_t{};
    auto enOrder = static_cast<OrderType>(order);
    uint8_t dataBitsFactor = 1;
    uint16_t dstAddrAlignUnit{};
    ParseOrder<half>(enOrder, dstRepeatLength, dataBitsFactor, dstAddrAlignUnit);
    RecordReduceOpFunc<RecordType::REDUCE_OP, InstrName::VCMIN>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, dstRepeatLength, 1, 8, dstRepeatLength, 32,
        dstAddrAlignUnit, dataBitsFactor);
}
#endif

#if defined(__DAV_M200_VEC__) || defined(__DAV_M200__)
// #2453
SANITIZER_REPORT(vcmin, __ubuf__ float *dst, __ubuf__ float *src, uint64_t config, bool order)
{
    RecordReduceOpConfigFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, config, 4, 1, 8, 4, 32, 8);
}
// #2454
SANITIZER_REPORT(vcmin, __ubuf__ float *dst, __ubuf__ float *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride, bool repeatStrideMode, bool strideSizeMode, bool order)
{
    RecordReduceOpFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, 4, 1, 8, 4, 32, 8);
}

#else
// #2455
SANITIZER_REPORT(vcmin, __ubuf__ float *dst, __ubuf__ float *src, uint64_t config, Order_t order)
{
    auto repeat = uint8_t{};
    auto dstRepeatStride = uint16_t{};
    auto srcBlockStride = uint16_t{};
    auto srcRepeatStride = uint16_t{};
    uint8_t dataBitsFactor = 1;
    ParseReduceConfig(config, repeat, dstRepeatStride, srcBlockStride, srcRepeatStride);
    auto dstRepeatLength = uint16_t{};
    auto enOrder = static_cast<OrderType>(order);
    uint16_t dstAddrAlignUnit{};
    ParseOrder<float>(enOrder, dstRepeatLength, dataBitsFactor, dstAddrAlignUnit);
    RecordReduceOpFunc<RecordType::REDUCE_OP, InstrName::VCMIN>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, dstRepeatLength, 1, 8, dstRepeatLength, 32,
        dstAddrAlignUnit, dataBitsFactor);
}
// #2456
SANITIZER_REPORT(vcmin, __ubuf__ float *dst, __ubuf__ float *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride, Order_t order)
{
    auto dstRepeatLength = uint16_t{};
    uint8_t dataBitsFactor = 1;
    auto enOrder = static_cast<OrderType>(order);
    uint16_t dstAddrAlignUnit{};
    ParseOrder<float>(enOrder, dstRepeatLength, dataBitsFactor, dstAddrAlignUnit);
    RecordReduceOpFunc<RecordType::REDUCE_OP, InstrName::VCMIN>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, dstRepeatLength, 1, 8, dstRepeatLength, 32, dstAddrAlignUnit,
        dataBitsFactor);
}
#endif

// #2457
SANITIZER_REPORT(vcmin, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src, uint64_t config, bool order)
{
    RecordReduceOpConfigFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, config, 2, 1, 8, 2, 32, 8);
}

// #2458
SANITIZER_REPORT(vcmin, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride, bool repeatStrideMode, bool strideSizeMode, bool order)
{
    RecordReduceOpFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, 2, 1, 8, 2, 32, 8);
}

#endif
