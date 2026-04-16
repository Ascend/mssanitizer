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

namespace {

template <typename TI>
AICORE_FUNC_HEAD void RecordVcmpvsByConfig(EXTRA_PARAMS_DEC, __ubuf__ uint8_t *dst,
                                            __ubuf__ TI *src0, TI src1, uint64_t config)
{
    auto repeat = uint8_t{};
    auto dstBlockStride = uint16_t{};
    auto srcBlockStride = uint16_t{};
    auto dstRepeatStride = uint16_t{};
    auto srcRepeatStride = uint16_t{};
    ParseUnaryConfigByArch(config, repeat, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 32 / sizeof(TI), 32, false);
}

} // namespace [Dummy]

#if !(defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1 && defined(__DAV_C220_CUBE__))

// #2539
SANITIZER_REPORT(vcmpvs_eq, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint64_t config)
{
    RecordVcmpvsByConfig(EXTRA_PARAMS, dst, src0, src1, config);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2541
SANITIZER_REPORT(vcmpvs_eq, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 16, 32, false);
}
#else
// #2540
SANITIZER_REPORT(vcmpvs_eq, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 16, 32, false);
}
#endif

// #2542
SANITIZER_REPORT(vcmpvs_eq, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint64_t config)
{
    RecordVcmpvsByConfig(EXTRA_PARAMS, dst, src0, src1, config);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2544
SANITIZER_REPORT(vcmpvs_eq, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 8, 32, false);
}
#else
// #2543
SANITIZER_REPORT(vcmpvs_eq, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 8, 32, false);
}
#endif

// #2545
SANITIZER_REPORT(vcmpvs_eq, __ubuf__ uint8_t *dst, __ubuf__ int32_t *src0, int32_t src1, uint64_t config)
{
    RecordVcmpvsByConfig(EXTRA_PARAMS, dst, src0, src1, config);
}

 // #2546
SANITIZER_REPORT(vcmpvs_eq, __ubuf__ uint8_t *dst, __ubuf__ int32_t *src0, int32_t src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 8, 32, false);
}

// #2547
SANITIZER_REPORT(vcmpvs_ge, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint64_t config)
{
    RecordVcmpvsByConfig(EXTRA_PARAMS, dst, src0, src1, config);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2549
SANITIZER_REPORT(vcmpvs_ge, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 16, 32, false);
}
#else
// #2548
SANITIZER_REPORT(vcmpvs_ge, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 16, 32, false);
}
#endif

// #2550
SANITIZER_REPORT(vcmpvs_ge, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint64_t config)
{
    RecordVcmpvsByConfig(EXTRA_PARAMS, dst, src0, src1, config);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2552
SANITIZER_REPORT(vcmpvs_ge, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 8, 32, false);
}
#else
// #2551
SANITIZER_REPORT(vcmpvs_ge, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 8, 32, false);
}
#endif

// #2553
SANITIZER_REPORT(vcmpvs_gt, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint64_t config)
{
    RecordVcmpvsByConfig(EXTRA_PARAMS, dst, src0, src1, config);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2555
SANITIZER_REPORT(vcmpvs_gt, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 16, 32, false);
}
#else
// #2554
SANITIZER_REPORT(vcmpvs_gt, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 16, 32, false);
}
#endif

// #2556
SANITIZER_REPORT(vcmpvs_gt, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint64_t config)
{
    RecordVcmpvsByConfig(EXTRA_PARAMS, dst, src0, src1, config);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2558
SANITIZER_REPORT(vcmpvs_gt, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 8, 32, false);
}
#else
// #2557
SANITIZER_REPORT(vcmpvs_gt, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 8, 32, false);
}
#endif

// #2559
SANITIZER_REPORT(vcmpvs_le, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint64_t config)
{
    RecordVcmpvsByConfig(EXTRA_PARAMS, dst, src0, src1, config);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2561
SANITIZER_REPORT(vcmpvs_le, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 16, 32, false);
}
#else
// #2560
SANITIZER_REPORT(vcmpvs_le, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 16, 32, false);
}
#endif

// #2562
SANITIZER_REPORT(vcmpvs_le, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint64_t config)
{
    RecordVcmpvsByConfig(EXTRA_PARAMS, dst, src0, src1, config);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2564
SANITIZER_REPORT(vcmpvs_le, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 8, 32, false);
}
#else
// #2563
SANITIZER_REPORT(vcmpvs_le, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 8, 32, false);
}
#endif

// #2565
SANITIZER_REPORT(vcmpvs_lt, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint64_t config)
{
    RecordVcmpvsByConfig(EXTRA_PARAMS, dst, src0, src1, config);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2567
SANITIZER_REPORT(vcmpvs_lt, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 16, 32, false);
}
#else
// #2566
SANITIZER_REPORT(vcmpvs_lt, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 16, 32, false);
}
#endif

// #2568
SANITIZER_REPORT(vcmpvs_lt, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint64_t config)
{
    RecordVcmpvsByConfig(EXTRA_PARAMS, dst, src0, src1, config);
}


#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2570
SANITIZER_REPORT(vcmpvs_lt, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 8, 32, false);
}
#else
// #2569
SANITIZER_REPORT(vcmpvs_lt, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 8, 32, false);
}
#endif

// #2571
SANITIZER_REPORT(vcmpvs_ne, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint64_t config)
{
    RecordVcmpvsByConfig(EXTRA_PARAMS, dst, src0, src1, config);
}

#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
 // #2567
SANITIZER_REPORT(vcmpvs_ne, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 16, 32, false);
}
#else
// #2566
SANITIZER_REPORT(vcmpvs_ne, __ubuf__ uint8_t *dst, __ubuf__ half *src0, half src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 16, 32, false);
}
#endif

// #2568
SANITIZER_REPORT(vcmpvs_ne, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint64_t config)
{
    RecordVcmpvsByConfig(EXTRA_PARAMS, dst, src0, src1, config);
}


#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
// #2570
SANITIZER_REPORT(vcmpvs_ne, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 8, 32, false);
}
#else
// #2569
SANITIZER_REPORT(vcmpvs_ne, __ubuf__ uint8_t *dst, __ubuf__ float *src0, float src1, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint8_t dstRepeatStride, uint8_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src0, repeat,
        1, srcBlockStride, 1, srcRepeatStride, 1, 8, 8, 32, false);
}
#endif

#endif
