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
#include "plugin/record_calc_instructions.h"
#include "plugin/record_ctrl_instructions.h"

using namespace Sanitizer;

#if !(defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1 && defined(__DAV_C220_CUBE__))

namespace {

AICORE_FUNC_HEAD void ClearCmpMaskAddr(EXTRA_PARAMS_DEC)
{
    // VCMP 指令会写入 CMPMASK 寄存器，因此对 set_cmpmask 时绑定的 ubuf 地址
    // 不会产生读取行为，解除绑定
    RecordRegister(EXTRA_PARAMS, &Register::cmpMaskAddr, ~0UL);
}

} // namespace [Dummy]

// #2459
SANITIZER_REPORT(vcmp_eq, __ubuf__ half *src0, __ubuf__ half *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2460
SANITIZER_REPORT(vcmp_eq, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2461
SANITIZER_REPORT(vcmp_eq, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2462
SANITIZER_REPORT(vcmp_eq, __ubuf__ float *src0, __ubuf__ float *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2463
SANITIZER_REPORT(vcmp_eq, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2464
SANITIZER_REPORT(vcmp_eq, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2465
SANITIZER_REPORT(vcmp_eq, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2466
SANITIZER_REPORT(vcmp_eq, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2467
SANITIZER_REPORT(vcmp_eq, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2468
SANITIZER_REPORT(vcmp_ge, __ubuf__ half *src0, __ubuf__ half *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2469
SANITIZER_REPORT(vcmp_ge, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2470
SANITIZER_REPORT(vcmp_ge, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2471
SANITIZER_REPORT(vcmp_ge, __ubuf__ float *src0, __ubuf__ float *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2472
SANITIZER_REPORT(vcmp_ge, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2473
SANITIZER_REPORT(vcmp_ge, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2474
SANITIZER_REPORT(vcmp_ge, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2475
SANITIZER_REPORT(vcmp_ge, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2476
SANITIZER_REPORT(vcmp_ge, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2477
SANITIZER_REPORT(vcmp_gt, __ubuf__ half *src0, __ubuf__ half *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2478
SANITIZER_REPORT(vcmp_gt, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2479
SANITIZER_REPORT(vcmp_gt, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2480
SANITIZER_REPORT(vcmp_gt, __ubuf__ float *src0, __ubuf__ float *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2481
SANITIZER_REPORT(vcmp_gt, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2482
SANITIZER_REPORT(vcmp_gt, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2483
SANITIZER_REPORT(vcmp_gt, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2484
SANITIZER_REPORT(vcmp_gt, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2485
SANITIZER_REPORT(vcmp_gt, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2486
SANITIZER_REPORT(vcmp_le, __ubuf__ half *src0, __ubuf__ half *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2487
SANITIZER_REPORT(vcmp_le, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2488
SANITIZER_REPORT(vcmp_le, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2489
SANITIZER_REPORT(vcmp_le, __ubuf__ float *src0, __ubuf__ float *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2490
SANITIZER_REPORT(vcmp_le, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2491
SANITIZER_REPORT(vcmp_le, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2492
SANITIZER_REPORT(vcmp_le, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2493
SANITIZER_REPORT(vcmp_le, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2494
SANITIZER_REPORT(vcmp_le, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2495
SANITIZER_REPORT(vcmp_lt, __ubuf__ half *src0, __ubuf__ half *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2496
SANITIZER_REPORT(vcmp_lt, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2497
SANITIZER_REPORT(vcmp_lt, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2498
SANITIZER_REPORT(vcmp_lt, __ubuf__ float *src0, __ubuf__ float *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2499
SANITIZER_REPORT(vcmp_lt, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2500
SANITIZER_REPORT(vcmp_lt, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2501
SANITIZER_REPORT(vcmp_lt, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2502
SANITIZER_REPORT(vcmp_lt, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2503
SANITIZER_REPORT(vcmp_lt, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2504
SANITIZER_REPORT(vcmp_ne, __ubuf__ half *src0, __ubuf__ half *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2505
SANITIZER_REPORT(vcmp_ne, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2506
SANITIZER_REPORT(vcmp_ne, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2507
SANITIZER_REPORT(vcmp_ne, __ubuf__ float *src0, __ubuf__ float *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2508
SANITIZER_REPORT(vcmp_ne, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2509
SANITIZER_REPORT(vcmp_ne, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2510
SANITIZER_REPORT(vcmp_ne, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint64_t config)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpConfigFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr),
                                                    src0, src1, config, 0, 8, 8, 0, 32, 32);
}

// #2511
SANITIZER_REPORT(vcmp_ne, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}

// #2512
SANITIZER_REPORT(vcmp_ne, __ubuf__ uint16_t *src0, __ubuf__ uint16_t *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, bool repeatStrideMode, bool strideSizeMode)
{
    ClearCmpMaskAddr(EXTRA_PARAMS);
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, static_cast<__ubuf__ void*>(nullptr), src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        0, 8, 8, 0, 32, 32);
}


#endif
