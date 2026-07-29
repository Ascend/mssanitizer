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
// #2583
SANITIZER_REPORT(vconv_bf162f32, __ubuf__ float *dst, __ubuf__ __bf16 *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2584
SANITIZER_REPORT(vconv_bf162f32, __ubuf__ float *dst, __ubuf__ __bf16 *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

// #2585
SANITIZER_REPORT(vconv_bf162s32a, __ubuf__ int32_t *dst, __ubuf__ __bf16 *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2586
SANITIZER_REPORT(vconv_bf162s32a, __ubuf__ int32_t *dst, __ubuf__ __bf16 *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

// #2587
SANITIZER_REPORT(vconv_bf162s32c, __ubuf__ int32_t *dst, __ubuf__ __bf16 *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2588
SANITIZER_REPORT(vconv_bf162s32c, __ubuf__ int32_t *dst, __ubuf__ __bf16 *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

// #2589
SANITIZER_REPORT(vconv_bf162s32f, __ubuf__ int32_t *dst, __ubuf__ __bf16 *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2590
SANITIZER_REPORT(vconv_bf162s32f, __ubuf__ int32_t *dst, __ubuf__ __bf16 *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

// #2591
SANITIZER_REPORT(vconv_bf162s32r, __ubuf__ int32_t *dst, __ubuf__ __bf16 *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2592
SANITIZER_REPORT(vconv_bf162s32r, __ubuf__ int32_t *dst, __ubuf__ __bf16 *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

// #2593
SANITIZER_REPORT(vconv_bf162s32z, __ubuf__ int32_t *dst, __ubuf__ __bf16 *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2594
SANITIZER_REPORT(vconv_bf162s32z, __ubuf__ int32_t *dst, __ubuf__ __bf16 *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

// #2595/2599
SANITIZER_REPORT(vconv_deq, __ubuf__ half *dst, __ubuf__ int32_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2596/2598
SANITIZER_REPORT(vconv_deq, __ubuf__ half *dst, __ubuf__ int32_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2597
SANITIZER_REPORT(vconv_deq, __ubuf__ half *dst, __ubuf__ int32_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2600
SANITIZER_REPORT(vconv_deqs162b8, __ubuf__ int8_t *dst, __ubuf__ int16_t *src, uint64_t config, bool halfBlock)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2601
SANITIZER_REPORT(vconv_deqs162b8, __ubuf__ int8_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode, bool halfBlock)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2602
SANITIZER_REPORT(vconv_deqs162b8, __ubuf__ uint8_t *dst, __ubuf__ int16_t *src, uint64_t config, bool halfBlock)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2603
SANITIZER_REPORT(vconv_deqs162b8, __ubuf__ uint8_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode, bool halfBlock)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2604
SANITIZER_REPORT(vconv_deqs162b8h, __ubuf__ int8_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2605
SANITIZER_REPORT(vconv_deqs162b8h, __ubuf__ int8_t *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2606
SANITIZER_REPORT(vconv_deqs162b8h, __ubuf__ uint8_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2607
SANITIZER_REPORT(vconv_deqs162b8h, __ubuf__ uint8_t *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2608
SANITIZER_REPORT(vconv_deqs162b8l, __ubuf__ int8_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2609
SANITIZER_REPORT(vconv_deqs162b8l, __ubuf__ int8_t *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2610
SANITIZER_REPORT(vconv_deqs162b8l, __ubuf__ uint8_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2611
SANITIZER_REPORT(vconv_deqs162b8l, __ubuf__ uint8_t *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2612/2616
SANITIZER_REPORT(vconv_f162f32, __ubuf__ float *dst, __ubuf__ half *src, uint64_t config)
{
    uint8_t repeat;
    uint16_t dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride;
    ParseUnaryConfigByArch(config, repeat, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2615
SANITIZER_REPORT(vconv_f162f32, __ubuf__ float *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}
#else
// #2613
SANITIZER_REPORT(vconv_f162f32, __ubuf__ float *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}
#endif

// #2614
SANITIZER_REPORT(vconv_f162f32, __ubuf__ float *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2617
SANITIZER_REPORT(vconv_f162s16a, __ubuf__ int16_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2618
SANITIZER_REPORT(vconv_f162s16a, __ubuf__ int16_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2619
SANITIZER_REPORT(vconv_f162s16c, __ubuf__ int16_t *dst, __ubuf__ half *src, uint8_t repeat,
                 uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2620
SANITIZER_REPORT(vconv_f162s16c, __ubuf__ int16_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2621
SANITIZER_REPORT(vconv_f162s16f, __ubuf__ int16_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2622
SANITIZER_REPORT(vconv_f162s16f, __ubuf__ int16_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2623/2626
SANITIZER_REPORT(vconv_f162s16r, __ubuf__ int16_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2624
SANITIZER_REPORT(vconv_f162s16r, __ubuf__ int16_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2625
SANITIZER_REPORT(vconv_f162s16r, __ubuf__ int16_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2627
SANITIZER_REPORT(vconv_f162s16z, __ubuf__ int16_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2628
SANITIZER_REPORT(vconv_f162s16z, __ubuf__ int16_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2629/2633
SANITIZER_REPORT(vconv_f162s32a, __ubuf__ int32_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2632
SANITIZER_REPORT(vconv_f162s32a, __ubuf__ int32_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}
#else
// #2630
SANITIZER_REPORT(vconv_f162s32a, __ubuf__ int32_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}
#endif

// #2631
SANITIZER_REPORT(vconv_f162s32a, __ubuf__ int32_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2634/2638
SANITIZER_REPORT(vconv_f162s32c, __ubuf__ int32_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2637
SANITIZER_REPORT(vconv_f162s32c, __ubuf__ int32_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}
#else
// #2635
SANITIZER_REPORT(vconv_f162s32c, __ubuf__ int32_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}
#endif

// #2636
SANITIZER_REPORT(vconv_f162s32c, __ubuf__ int32_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2639/2643
SANITIZER_REPORT(vconv_f162s32f, __ubuf__ int32_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2642
SANITIZER_REPORT(vconv_f162s32f, __ubuf__ int32_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}
#else
// #2640
SANITIZER_REPORT(vconv_f162s32f, __ubuf__ int32_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}
#endif

// #2641
SANITIZER_REPORT(vconv_f162s32f, __ubuf__ int32_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2644/2648
SANITIZER_REPORT(vconv_f162s32r, __ubuf__ int32_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2647
SANITIZER_REPORT(vconv_f162s32r, __ubuf__ int32_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}
#else
// #2645
SANITIZER_REPORT(vconv_f162s32r, __ubuf__ int32_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}
#endif

// #2646
SANITIZER_REPORT(vconv_f162s32r, __ubuf__ int32_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2649/2653
SANITIZER_REPORT(vconv_f162s32z, __ubuf__ int32_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2652
SANITIZER_REPORT(vconv_f162s32z, __ubuf__ int32_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}
#else
// #2650
SANITIZER_REPORT(vconv_f162s32z, __ubuf__ int32_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}
#endif

// #2651
SANITIZER_REPORT(vconv_f162s32z, __ubuf__ int32_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2654/2657
SANITIZER_REPORT(vconv_f162s4, __ubuf__ void *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::VCONV_DST_S4_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 2, 8, 32, 32);
}

// #2655
SANITIZER_REPORT(vconv_f162s4, __ubuf__ void *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::VCONV_DST_S4_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 2, 8, 32, 32);
}

// #2656
SANITIZER_REPORT(vconv_f162s4, __ubuf__ void *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::VCONV_DST_S4_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 2, 8, 32, 32);
}

// #2658
SANITIZER_REPORT(vconv_f162s4a, __ubuf__ void *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::VCONV_DST_S4_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 2, 8, 32, 32);
}

// #2659
SANITIZER_REPORT(vconv_f162s4a, __ubuf__ void *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::VCONV_DST_S4_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 2, 8, 32, 32);
}

// #2660
SANITIZER_REPORT(vconv_f162s4c, __ubuf__ void *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::VCONV_DST_S4_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 2, 8, 32, 32);
}

// #2661
SANITIZER_REPORT(vconv_f162s4c, __ubuf__ void *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::VCONV_DST_S4_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 2, 8, 32, 32);
}

// #2662
SANITIZER_REPORT(vconv_f162s4f, __ubuf__ void *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::VCONV_DST_S4_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 2, 8, 32, 32);
}

// #2663
SANITIZER_REPORT(vconv_f162s4f, __ubuf__ void *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::VCONV_DST_S4_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 2, 8, 32, 32);
}

// #2664
SANITIZER_REPORT(vconv_f162s4r, __ubuf__ void *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::VCONV_DST_S4_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 2, 8, 32, 32);
}

// #2665
SANITIZER_REPORT(vconv_f162s4r, __ubuf__ void *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::VCONV_DST_S4_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 2, 8, 32, 32);
}

// #2666
SANITIZER_REPORT(vconv_f162s4z, __ubuf__ void *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::VCONV_DST_S4_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 2, 8, 32, 32);
}

// #2667
SANITIZER_REPORT(vconv_f162s4z, __ubuf__ void *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::VCONV_DST_S4_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 2, 8, 32, 32);
}

// #2668/2672
SANITIZER_REPORT(vconv_f162s8, __ubuf__ int8_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2671
SANITIZER_REPORT(vconv_f162s8, __ubuf__ int8_t *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstBlockStride,
    uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#else
// #2669
SANITIZER_REPORT(vconv_f162s8, __ubuf__ int8_t *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstBlockStride,
    uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#endif

// #2670
SANITIZER_REPORT(vconv_f162s8, __ubuf__ int8_t *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstBlockStride,
    uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2673/2677
SANITIZER_REPORT(vconv_f162s8a, __ubuf__ int8_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2676
SANITIZER_REPORT(vconv_f162s8a, __ubuf__ int8_t *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstBlockStride,
    uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#else
// #2674
SANITIZER_REPORT(vconv_f162s8a, __ubuf__ int8_t *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstBlockStride,
    uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#endif

// #2675
SANITIZER_REPORT(vconv_f162s8a, __ubuf__ int8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2678/2682
SANITIZER_REPORT(vconv_f162s8c, __ubuf__ int8_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2681
SANITIZER_REPORT(vconv_f162s8c, __ubuf__ int8_t *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstBlockStride,
    uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#else
// #2679
SANITIZER_REPORT(vconv_f162s8c, __ubuf__ int8_t *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstBlockStride,
    uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
    dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#endif

// #2680
SANITIZER_REPORT(vconv_f162s8c, __ubuf__ int8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride,
        srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2683/2687
SANITIZER_REPORT(vconv_f162s8f, __ubuf__ int8_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2686
SANITIZER_REPORT(vconv_f162s8f, __ubuf__ int8_t *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstBlockStride,
    uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#else
// #2684
SANITIZER_REPORT(vconv_f162s8f, __ubuf__ int8_t *dst, __ubuf__ half *src, uint8_t repeat, uint16_t dstBlockStride,
    uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#endif

// #2685
SANITIZER_REPORT(vconv_f162s8f, __ubuf__ int8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2688
SANITIZER_REPORT(vconv_f162s8r, __ubuf__ int8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride,
        srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2689
SANITIZER_REPORT(vconv_f162s8r, __ubuf__ int8_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2690/2694
SANITIZER_REPORT(vconv_f162s8z, __ubuf__ int8_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2693
SANITIZER_REPORT(vconv_f162s8z, __ubuf__ int8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#else
// #2691
SANITIZER_REPORT(vconv_f162s8z, __ubuf__ int8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#endif

// #2692
SANITIZER_REPORT(vconv_f162s8z, __ubuf__ int8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2695/2699
SANITIZER_REPORT(vconv_f162u8, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2698
SANITIZER_REPORT(vconv_f162u8, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride,
        srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#else
// #2696
SANITIZER_REPORT(vconv_f162u8, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride,
        srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#endif

// #2697
// is not support 910b
SANITIZER_REPORT(vconv_f162u8, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2700/2704
// is not support 910b
SANITIZER_REPORT(vconv_f162u8a, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2703
SANITIZER_REPORT(vconv_f162u8a, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride,
        srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#else
// #2701
SANITIZER_REPORT(vconv_f162u8a, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride,
        srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#endif

// #2702
SANITIZER_REPORT(vconv_f162u8a, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2705/2709
SANITIZER_REPORT(vconv_f162u8c, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2708
SANITIZER_REPORT(vconv_f162u8c, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#else
// #2706
SANITIZER_REPORT(vconv_f162u8c, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride,
        srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#endif

// #2707
SANITIZER_REPORT(vconv_f162u8c, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2710/2714
SANITIZER_REPORT(vconv_f162u8f, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2713
SANITIZER_REPORT(vconv_f162u8f, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride,
        srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#else
// #2711
SANITIZER_REPORT(vconv_f162u8f, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#endif

// #2712
SANITIZER_REPORT(vconv_f162u8f, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2715
SANITIZER_REPORT(vconv_f162u8r, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2716
SANITIZER_REPORT(vconv_f162u8r, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2717/2721
SANITIZER_REPORT(vconv_f162u8z, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2720
SANITIZER_REPORT(vconv_f162u8z, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#else
// #2718
SANITIZER_REPORT(vconv_f162u8z, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#endif

// #2719
SANITIZER_REPORT(vconv_f162u8z, __ubuf__ uint8_t *dst, __ubuf__ half *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2722
SANITIZER_REPORT(vconv_f322bf16a, __ubuf__ __bf16 *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2723
SANITIZER_REPORT(vconv_f322bf16a, __ubuf__ __bf16 *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2724
SANITIZER_REPORT(vconv_f322bf16c, __ubuf__ __bf16 *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2725
SANITIZER_REPORT(vconv_f322bf16c, __ubuf__ __bf16 *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2726
SANITIZER_REPORT(vconv_f322bf16f, __ubuf__ __bf16 *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2727
SANITIZER_REPORT(vconv_f322bf16f, __ubuf__ __bf16 *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2728
SANITIZER_REPORT(vconv_f322bf16r, __ubuf__ __bf16 *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2729
SANITIZER_REPORT(vconv_f322bf16r, __ubuf__ __bf16 *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2730
SANITIZER_REPORT(vconv_f322bf16z, __ubuf__ __bf16 *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2731
SANITIZER_REPORT(vconv_f322bf16z, __ubuf__ __bf16 *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2732/2736
SANITIZER_REPORT(vconv_f322f16, __ubuf__ half *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2735
SANITIZER_REPORT(vconv_f322f16, __ubuf__ half *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#else
// #2733
SANITIZER_REPORT(vconv_f322f16, __ubuf__ half *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#endif

// #2734
SANITIZER_REPORT(vconv_f322f16, __ubuf__ half *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2737
SANITIZER_REPORT(vconv_f322f16a, __ubuf__ half *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2738
SANITIZER_REPORT(vconv_f322f16a, __ubuf__ half *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2739
SANITIZER_REPORT(vconv_f322f16c, __ubuf__ half *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2740
SANITIZER_REPORT(vconv_f322f16c, __ubuf__ half *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2741
SANITIZER_REPORT(vconv_f322f16f, __ubuf__ half *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2742
SANITIZER_REPORT(vconv_f322f16f, __ubuf__ half *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2743/2747
SANITIZER_REPORT(vconv_f322f16o, __ubuf__ half *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2746
SANITIZER_REPORT(vconv_f322f16o, __ubuf__ half *dst, __ubuf__ float *src, uint8_t repeat, uint16_t dstBlockStride,
    uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride,
        srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#else
// #2744
SANITIZER_REPORT(vconv_f322f16o, __ubuf__ half *dst, __ubuf__ float *src, uint8_t repeat, uint16_t dstBlockStride,
    uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride,
        srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#endif

// #2745
// is not support 910b
SANITIZER_REPORT(vconv_f322f16o, __ubuf__ half *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2748
SANITIZER_REPORT(vconv_f322f16r, __ubuf__ half *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2749
SANITIZER_REPORT(vconv_f322f16r, __ubuf__ half *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2750
SANITIZER_REPORT(vconv_f322f16z, __ubuf__ half *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2751
SANITIZER_REPORT(vconv_f322f16z, __ubuf__ half *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2752
SANITIZER_REPORT(vconv_f322f32a, __ubuf__ float *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2753
SANITIZER_REPORT(vconv_f322f32a, __ubuf__ float *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2754
SANITIZER_REPORT(vconv_f322f32c, __ubuf__ float *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2755
SANITIZER_REPORT(vconv_f322f32c, __ubuf__ float *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2756
SANITIZER_REPORT(vconv_f322f32f, __ubuf__ float *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2757
SANITIZER_REPORT(vconv_f322f32f, __ubuf__ float *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2758
SANITIZER_REPORT(vconv_f322f32r, __ubuf__ float *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2759
SANITIZER_REPORT(vconv_f322f32r, __ubuf__ float *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2760
SANITIZER_REPORT(vconv_f322f32z, __ubuf__ float *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2761
SANITIZER_REPORT(vconv_f322f32z, __ubuf__ float *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2762
SANITIZER_REPORT(vconv_f322s16a, __ubuf__ int16_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2763
SANITIZER_REPORT(vconv_f322s16a, __ubuf__ int16_t *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2764
SANITIZER_REPORT(vconv_f322s16c, __ubuf__ int16_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2765
SANITIZER_REPORT(vconv_f322s16c, __ubuf__ int16_t *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2766
SANITIZER_REPORT(vconv_f322s16f, __ubuf__ int16_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2767
SANITIZER_REPORT(vconv_f322s16f, __ubuf__ int16_t *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2768/2772
SANITIZER_REPORT(vconv_f322s16r, __ubuf__ int16_t *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2771
SANITIZER_REPORT(vconv_f322s16r, __ubuf__ int16_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#else
// #2769
SANITIZER_REPORT(vconv_f322s16r, __ubuf__ int16_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#endif

// #2770
// is not support 910b
SANITIZER_REPORT(vconv_f322s16r, __ubuf__ int16_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2773/2777
SANITIZER_REPORT(vconv_f322s16z, __ubuf__ int16_t *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2776
SANITIZER_REPORT(vconv_f322s16z, __ubuf__ int16_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#else
// #2774
SANITIZER_REPORT(vconv_f322s16z, __ubuf__ int16_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}
#endif

// #2775
SANITIZER_REPORT(vconv_f322s16z, __ubuf__ int16_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2778/2782
SANITIZER_REPORT(vconv_f322s32a, __ubuf__ int32_t *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2781
SANITIZER_REPORT(vconv_f322s32a, __ubuf__ int32_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #2779
SANITIZER_REPORT(vconv_f322s32a, __ubuf__ int32_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #2780
SANITIZER_REPORT(vconv_f322s32a, __ubuf__ int32_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2783/2787
SANITIZER_REPORT(vconv_f322s32c, __ubuf__ int32_t *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2786
SANITIZER_REPORT(vconv_f322s32c, __ubuf__ int32_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #2784
SANITIZER_REPORT(vconv_f322s32c, __ubuf__ int32_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #2785
SANITIZER_REPORT(vconv_f322s32c, __ubuf__ int32_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2788/2792
SANITIZER_REPORT(vconv_f322s32f, __ubuf__ int32_t *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2791
SANITIZER_REPORT(vconv_f322s32f, __ubuf__ int32_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #2789
SANITIZER_REPORT(vconv_f322s32f, __ubuf__ int32_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #2790
// is not support 910b
SANITIZER_REPORT(vconv_f322s32f, __ubuf__ int32_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2793/2797
SANITIZER_REPORT(vconv_f322s32r, __ubuf__ int32_t *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2796
SANITIZER_REPORT(vconv_f322s32r, __ubuf__ int32_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #2794
SANITIZER_REPORT(vconv_f322s32r, __ubuf__ int32_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #2795
SANITIZER_REPORT(vconv_f322s32r, __ubuf__ int32_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2798/2802
SANITIZER_REPORT(vconv_f322s32z, __ubuf__ int32_t *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2801
SANITIZER_REPORT(vconv_f322s32z, __ubuf__ int32_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #2799
SANITIZER_REPORT(vconv_f322s32z, __ubuf__ int32_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #2800
SANITIZER_REPORT(vconv_f322s32z, __ubuf__ int32_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2805
SANITIZER_REPORT(vconv_f322s64a, __ubuf__ int64_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2806
SANITIZER_REPORT(vconv_f322s64a, __ubuf__ int64_t *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

// #2809
SANITIZER_REPORT(vconv_f322s64c, __ubuf__ int64_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2810
SANITIZER_REPORT(vconv_f322s64c, __ubuf__ int64_t *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

// #2813
SANITIZER_REPORT(vconv_f322s64f, __ubuf__ int64_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2814
SANITIZER_REPORT(vconv_f322s64f, __ubuf__ int64_t *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

// #2817
SANITIZER_REPORT(vconv_f322s64r, __ubuf__ int64_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2818
SANITIZER_REPORT(vconv_f322s64r, __ubuf__ int64_t *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

// #2821
SANITIZER_REPORT(vconv_f322s64z, __ubuf__ int64_t *dst, __ubuf__ float *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2822
SANITIZER_REPORT(vconv_f322s64z, __ubuf__ int64_t *dst, __ubuf__ float *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

// #2823/2826
SANITIZER_REPORT(vconv_s162f16, __ubuf__ half *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2824
SANITIZER_REPORT(vconv_s162f16, __ubuf__ half *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2825
SANITIZER_REPORT(vconv_s162f16, __ubuf__ half *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2827
SANITIZER_REPORT(vconv_s162f16a, __ubuf__ half *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2828
SANITIZER_REPORT(vconv_s162f16a, __ubuf__ half *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2829
SANITIZER_REPORT(vconv_s162f16c, __ubuf__ half *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2830
SANITIZER_REPORT(vconv_s162f16c, __ubuf__ half *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2831
SANITIZER_REPORT(vconv_s162f16f, __ubuf__ half *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2832
SANITIZER_REPORT(vconv_s162f16f, __ubuf__ half *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2833
SANITIZER_REPORT(vconv_s162f16r, __ubuf__ half *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2834
SANITIZER_REPORT(vconv_s162f16r, __ubuf__ half *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2835
SANITIZER_REPORT(vconv_s162f16z, __ubuf__ half *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2836
SANITIZER_REPORT(vconv_s162f16z, __ubuf__ half *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2837/2840
SANITIZER_REPORT(vconv_s162f32, __ubuf__ float *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

// #2838
SANITIZER_REPORT(vconv_s162f32, __ubuf__ float *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2839
SANITIZER_REPORT(vconv_s162f32, __ubuf__ float *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2841/2845
SANITIZER_REPORT(vconv_s322f32, __ubuf__ float *dst, __ubuf__ int32_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2844
SANITIZER_REPORT(vconv_s322f32, __ubuf__ float *dst, __ubuf__ int32_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#else
// #2842
SANITIZER_REPORT(vconv_s322f32, __ubuf__ float *dst, __ubuf__ int32_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}
#endif

// #2843
SANITIZER_REPORT(vconv_s322f32, __ubuf__ float *dst, __ubuf__ int32_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2846
SANITIZER_REPORT(vconv_s322f32a, __ubuf__ float *dst, __ubuf__ int32_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2847
SANITIZER_REPORT(vconv_s322f32a, __ubuf__ float *dst, __ubuf__ int32_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2848
SANITIZER_REPORT(vconv_s322f32c, __ubuf__ float *dst, __ubuf__ int32_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2849
SANITIZER_REPORT(vconv_s322f32c, __ubuf__ float *dst, __ubuf__ int32_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2850
SANITIZER_REPORT(vconv_s322f32f, __ubuf__ float *dst, __ubuf__ int32_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2851
SANITIZER_REPORT(vconv_s322f32f, __ubuf__ float *dst, __ubuf__ int32_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2852
SANITIZER_REPORT(vconv_s322f32r, __ubuf__ float *dst, __ubuf__ int32_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2853
SANITIZER_REPORT(vconv_s322f32r, __ubuf__ float *dst, __ubuf__ int32_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2854
SANITIZER_REPORT(vconv_s322f32z, __ubuf__ float *dst, __ubuf__ int32_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2855
SANITIZER_REPORT(vconv_s322f32z, __ubuf__ float *dst, __ubuf__ int32_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

// #2856
SANITIZER_REPORT(vconv_s322s16, __ubuf__ int16_t *dst, __ubuf__ int32_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2857
SANITIZER_REPORT(vconv_s322s16, __ubuf__ int16_t *dst, __ubuf__ int32_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2860
SANITIZER_REPORT(vconv_s322s64, __ubuf__ int64_t *dst, __ubuf__ int32_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2861
SANITIZER_REPORT(vconv_s322s64, __ubuf__ int64_t *dst, __ubuf__ int32_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

// #2862
SANITIZER_REPORT(vconv_s42f16, __ubuf__ half *dst, __ubuf__ void *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::VCONV_SRC_S4_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 2, 32, 32);
}

// #2863
SANITIZER_REPORT(vconv_s42f16, __ubuf__ half *dst, __ubuf__ void *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::VCONV_SRC_S4_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 2, 32, 32);
}

// #2866
SANITIZER_REPORT(vconv_s642f32a, __ubuf__ float *dst, __ubuf__ int64_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2867
SANITIZER_REPORT(vconv_s642f32a, __ubuf__ float *dst, __ubuf__ int64_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2870
SANITIZER_REPORT(vconv_s642f32c, __ubuf__ float *dst, __ubuf__ int64_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2871
SANITIZER_REPORT(vconv_s642f32c, __ubuf__ float *dst, __ubuf__ int64_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2874
SANITIZER_REPORT(vconv_s642f32f, __ubuf__ float *dst, __ubuf__ int64_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2875
SANITIZER_REPORT(vconv_s642f32f, __ubuf__ float *dst, __ubuf__ int64_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2878
SANITIZER_REPORT(vconv_s642f32r, __ubuf__ float *dst, __ubuf__ int64_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2879
SANITIZER_REPORT(vconv_s642f32r, __ubuf__ float *dst, __ubuf__ int64_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2882
SANITIZER_REPORT(vconv_s642f32z, __ubuf__ float *dst, __ubuf__ int64_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2883
SANITIZER_REPORT(vconv_s642f32z, __ubuf__ float *dst, __ubuf__ int64_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2886
SANITIZER_REPORT(vconv_s642s32, __ubuf__ int32_t *dst, __ubuf__ int64_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2887
SANITIZER_REPORT(vconv_s642s32, __ubuf__ int32_t *dst, __ubuf__ int64_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2888/2892
SANITIZER_REPORT(vconv_s82f16, __ubuf__ half *dst, __ubuf__ int8_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2891
SANITIZER_REPORT(vconv_s82f16, __ubuf__ half *dst, __ubuf__ int8_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}
#else
// #2889
SANITIZER_REPORT(vconv_s82f16, __ubuf__ half *dst, __ubuf__ int8_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}
#endif

// #2890
SANITIZER_REPORT(vconv_s82f16, __ubuf__ half *dst, __ubuf__ int8_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2893/2897
SANITIZER_REPORT(vconv_u82f16, __ubuf__ half *dst, __ubuf__ uint8_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 8, 4, 32, 32);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2896
SANITIZER_REPORT(vconv_u82f16, __ubuf__ half *dst, __ubuf__ uint8_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}
#else
// #2894
SANITIZER_REPORT(vconv_u82f16, __ubuf__ half *dst, __ubuf__ uint8_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}
#endif

// #2895
SANITIZER_REPORT(vconv_u82f16, __ubuf__ half *dst, __ubuf__ uint8_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 8, 4, 32, 32);
}

// #2898
SANITIZER_REPORT(vconv_vdeqs162b8, __ubuf__ int8_t *dst, __ubuf__ int16_t *src, uint64_t config, bool halfBlock)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2899
SANITIZER_REPORT(vconv_vdeqs162b8, __ubuf__ int8_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode, bool halfBlock)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2900
SANITIZER_REPORT(vconv_vdeqs162b8, __ubuf__ uint8_t *dst, __ubuf__ int16_t *src, uint64_t config, bool halfBlock)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2901
SANITIZER_REPORT(vconv_vdeqs162b8, __ubuf__ uint8_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride,
    bool repeatStrideMode, bool strideSizeMode, bool halfBlock)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2902
SANITIZER_REPORT(vconv_vdeqs162b8h, __ubuf__ int8_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2903
SANITIZER_REPORT(vconv_vdeqs162b8h, __ubuf__ int8_t *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2904
SANITIZER_REPORT(vconv_vdeqs162b8h, __ubuf__ uint8_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2905
SANITIZER_REPORT(vconv_vdeqs162b8h, __ubuf__ uint8_t *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2906
SANITIZER_REPORT(vconv_vdeqs162b8l, __ubuf__ int8_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2907
SANITIZER_REPORT(vconv_vdeqs162b8l, __ubuf__ int8_t *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

// #2908
SANITIZER_REPORT(vconv_vdeqs162b8l, __ubuf__ uint8_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 4, 8, 32, 32);
}

// #2909
SANITIZER_REPORT(vconv_vdeqs162b8l, __ubuf__ uint8_t *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::UNARY_OP, InstrName::VCONV>(EXTRA_PARAMS, dst, src, config, 4, 8, 32, 32);
}

#endif
