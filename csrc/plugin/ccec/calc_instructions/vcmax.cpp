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

// #2433
SANITIZER_REPORT(vcmax, __ubuf__ half *dst, __ubuf__ half *src, uint64_t config)
{
    RecordReduceOpConfigFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, config, 1 * 2, 1, 8, 2, 32, 4);
}

// #2434
SANITIZER_REPORT(vcmax, __ubuf__ half *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride)
{
    RecordReduceOpFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, 1 * 2, 1, 8, 2, 32, 4);
}

// #2435
SANITIZER_REPORT(vcmax, __ubuf__ half *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    RecordReduceOpFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, 1 * 2, 1, 8, 2, 32, 4);
}

// #2436
SANITIZER_REPORT(vcmax, __ubuf__ half *dst, __ubuf__ half *src, uint64_t config, bool order)
{
    RecordReduceOpConfigFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, config, 1 * 2, 1, 8, 2, 32, 4);
}

// #2437
SANITIZER_REPORT(vcmax, __ubuf__ half *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride, bool repeatStrideMode, bool strideSizeMode, bool order)
{
    RecordReduceOpFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, 1 * 2, 1, 8, 2, 32, 4);
}

// #2438
SANITIZER_REPORT(vcmax, __ubuf__ half *dst, __ubuf__ half *src, uint64_t config, Order_t order)
{
    auto repeat = uint8_t{};
    auto dstRepeatStride = uint16_t{};
    auto srcBlockStride = uint16_t{};
    auto srcRepeatStride = uint16_t{};
    ParseReduceConfig(config, repeat, dstRepeatStride, srcBlockStride, srcRepeatStride);
    auto dstRepeatLength = uint16_t{};
    auto alignSize = uint16_t{};
    auto enOrder = static_cast<OrderType>(order);
    uint8_t dataBitsFactor = 1;
    ParseOrder<half>(enOrder, dstRepeatLength, dataBitsFactor, alignSize);
    RecordReduceOpFunc<RecordType::REDUCE_OP, InstrName::VCMAX>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, dstRepeatLength, 1, 8, dstRepeatLength, 32, alignSize,
        dataBitsFactor);
}

// #2439
SANITIZER_REPORT(vcmax, __ubuf__ half *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride, Order_t order)
{
    uint16_t dstRepeatLength;
    uint8_t dataBitsFactor = 1;
    auto alignSize = uint16_t{};
    auto enOrder = static_cast<OrderType>(order);
    ParseOrder<half>(enOrder, dstRepeatLength, dataBitsFactor, alignSize);
    RecordReduceOpFunc<RecordType::REDUCE_OP, InstrName::VCMAX>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, dstRepeatLength, 1, 8, dstRepeatLength, 32, alignSize,
        dataBitsFactor);
}

// #2440
SANITIZER_REPORT(vcmax, __ubuf__ float *dst, __ubuf__ float *src, uint64_t config, bool order)
{
    RecordReduceOpConfigFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, config, 1 * 4, 1, 8, 4, 32, 8);
}

// #2441
SANITIZER_REPORT(vcmax, __ubuf__ float *dst, __ubuf__ float *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride, bool repeatStrideMode, bool strideSizeMode, bool order)
{
    RecordReduceOpFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, 1 * 4, 1, 8, 4, 32, 8);
}

// #2442
SANITIZER_REPORT(vcmax, __ubuf__ float *dst, __ubuf__ float *src, uint64_t config, Order_t order)
{
    uint8_t repeat;
    uint16_t dstRepeatStride, srcBlockStride, srcRepeatStride;
    ParseReduceConfig(config, repeat, dstRepeatStride, srcBlockStride, srcRepeatStride);
    uint16_t dstRepeatLength;
    uint8_t dataBitsFactor = 1;
    auto alignSize = uint16_t{};
    auto enOrder = static_cast<OrderType>(order);
    ParseOrder<float>(enOrder, dstRepeatLength, dataBitsFactor, alignSize);
    RecordReduceOpFunc<RecordType::REDUCE_OP, InstrName::VCMAX>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, dstRepeatLength, 1, 8, dstRepeatLength, 32, alignSize,
        dataBitsFactor);
}

// #2443
SANITIZER_REPORT(vcmax, __ubuf__ float *dst, __ubuf__ float *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride, Order_t order)
{
    uint16_t dstRepeatLength;
    uint8_t dataBitsFactor = 1;
    auto alignSize = uint16_t{};
    auto enOrder = static_cast<OrderType>(order);
    ParseOrder<float>(enOrder, dstRepeatLength, dataBitsFactor, alignSize);
    RecordReduceOpFunc<RecordType::REDUCE_OP, InstrName::VCMAX>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, dstRepeatLength, 1, 8, dstRepeatLength, 32, alignSize,
        dataBitsFactor);
}

// #2444
SANITIZER_REPORT(vcmax, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src, uint64_t config, bool order)
{
    RecordReduceOpConfigFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, config, 1 * 2, 1, 8, 2, 32, 8);
}

// #2445
SANITIZER_REPORT(vcmax, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src, uint8_t repeat, uint16_t dstRepeatStride,
    uint16_t srcBlockStride, uint16_t srcRepeatStride, bool repeatStrideMode, bool strideSizeMode, bool order)
{
    RecordReduceOpFunc<RecordType::REDUCE_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstRepeatStride, srcBlockStride, srcRepeatStride, 1 * 2, 1, 8, 2, 32, 8);
}

#endif
