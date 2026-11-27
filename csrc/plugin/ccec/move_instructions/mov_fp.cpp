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


#include "plugin/record_move_instructions.h"

using namespace Sanitizer;

// #601, FIX_L0C_TO_OUT.f32
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ half *dst, __cc__ float *src, uint64_t xm, uint64_t xt)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                     reinterpret_cast<uint64_t>(src), xm, xt, false);
}

// #608 -> /* #601 */
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ half *dst, __cc__ float *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src), NSize,
                     MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE, channelSplit, NZ2ND_EN, false);
}

// #609, FIX_L0C_TO_OUT.f32
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ float *dst, __cc__ float *src, uint64_t xm, uint64_t xt)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                     reinterpret_cast<uint64_t>(src), xm, xt, true);
}

// #650 -> /* #609 */
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ float *dst, __cc__ float *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src), NSize,
                     MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE, channelSplit, NZ2ND_EN, true);
}

// #610, FIX_L0C_TO_OUT.f32
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ __bf16 *dst, __cc__ float *src, uint64_t xm, uint64_t xt)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                     reinterpret_cast<uint64_t>(src), xm, xt, false);
}

// #617 -> /* #610 */
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ __bf16 *dst, __cc__ float *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src), NSize,
                     MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE, channelSplit, NZ2ND_EN, false);
}

// #622, FIX_L0C_TO_OUT.f32
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ int8_t *dst, __cc__ float *src, uint64_t xm, uint64_t xt)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                     reinterpret_cast<uint64_t>(src), xm, xt, false);
}

// #631 -> /* #622 */
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ int8_t *dst, __cc__ float *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src), NSize,
                     MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE, channelSplit, NZ2ND_EN, false);
}

// #632, FIX_L0C_TO_OUT.f32
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ uint8_t *dst, __cc__ float *src, uint64_t xm, uint64_t xt)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                     reinterpret_cast<uint64_t>(src), xm, xt, false);
}

// #641 -> /* #632 */
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ uint8_t *dst, __cc__ float *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src), NSize,
                     MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE, channelSplit, NZ2ND_EN, false);
}

// #651, FIX_L0C_TO_OUT.s32
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ int32_t *dst, __cc__ int32_t *src, uint64_t xm, uint64_t xt)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                     reinterpret_cast<uint64_t>(src), xm, xt, false);
}

// #702 -> /* #651 */
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ int32_t *dst, __cc__ int32_t *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src), NSize,
                     MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE, channelSplit, NZ2ND_EN, false);
}

// #654, FIX_L0C_TO_OUT.s32
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ half *dst, __cc__ int32_t *src, uint64_t xm, uint64_t xt)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                     reinterpret_cast<uint64_t>(src), xm, xt, false);
}

// #663 -> /* #654 */
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ half *dst, __cc__ int32_t *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src), NSize,
                     MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE, channelSplit, NZ2ND_EN, false);
}

// #666, FIX_L0C_TO_OUT.s32
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ int16_t *dst, __cc__ int32_t *src, uint64_t xm, uint64_t xt)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                     reinterpret_cast<uint64_t>(src), xm, xt, false);
}

// #673 -> /* #666 */
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ int16_t *dst, __cc__ int32_t *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src), NSize,
                     MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE, channelSplit, NZ2ND_EN, false);
}

// #674, FIX_L0C_TO_OUT.s32
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ int8_t *dst, __cc__ int32_t *src, uint64_t xm, uint64_t xt)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                     reinterpret_cast<uint64_t>(src), xm, xt, false);
}

// #683 -> /* #674 */
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ int8_t *dst, __cc__ int32_t *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src), NSize,
                     MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE, channelSplit, NZ2ND_EN, false);
}

// #684, FIX_L0C_TO_OUT.s32
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ uint8_t *dst, __cc__ int32_t *src, uint64_t xm, uint64_t xt)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                     reinterpret_cast<uint64_t>(src), xm, xt, false);
}

// #693 -> /* #684 */
SANITIZER_REPORT(copy_matrix_cc_to_gm, __gm__ uint8_t *dst, __cc__ int32_t *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src), NSize,
                     MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE, channelSplit, NZ2ND_EN, false);
}

// #703, FIX_L0C_TO_OUT.f32
SANITIZER_REPORT(copy_matrix_cc_to_gm_b4, __gm__ void *dst, __cc__ float *src, uint64_t xm, uint64_t xt)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                     reinterpret_cast<uint64_t>(src), xm, xt, false);
}

// #704 -> /* #703 */
SANITIZER_REPORT(copy_matrix_cc_to_gm_b4, __gm__ void *dst, __cc__ float *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src), NSize,
                     MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE, channelSplit, NZ2ND_EN, false);
}

// #705, FIX_L0C_TO_OUT.s32
SANITIZER_REPORT(copy_matrix_cc_to_gm_b4, __gm__ void *dst, __cc__ int32_t *src, uint64_t xm, uint64_t xt)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                     reinterpret_cast<uint64_t>(src), xm, xt, false);
}

// #706 -> /* #705 */
SANITIZER_REPORT(copy_matrix_cc_to_gm_b4, __gm__ void *dst, __cc__ int32_t *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordMovFpEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src), NSize,
                     MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE, channelSplit, NZ2ND_EN, false);
}

// FIX_L0C_TO_L1, L0C->L1

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ half *dst, __cc__ float *src, uint64_t xm, uint64_t xt)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              xm, xt, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ half *dst, __cc__ float *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              NSize, MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE,
                              channelSplit, NZ2ND_EN, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ float *dst, __cc__ float *src, uint64_t xm, uint64_t xt)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              xm, xt, true);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ float *dst, __cc__ float *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              NSize, MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE,
                              channelSplit, NZ2ND_EN, true);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ __bf16 *dst, __cc__ float *src, uint64_t xm, uint64_t xt)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              xm, xt, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ __bf16 *dst, __cc__ float *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              NSize, MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE,
                              channelSplit, NZ2ND_EN, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ int8_t *dst, __cc__ float *src, uint64_t xm, uint64_t xt)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              xm, xt, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ int8_t *dst, __cc__ float *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              NSize, MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE,
                              channelSplit, NZ2ND_EN, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ uint8_t *dst, __cc__ float *src, uint64_t xm, uint64_t xt)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              xm, xt, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ uint8_t *dst, __cc__ float *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              NSize, MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE,
                              channelSplit, NZ2ND_EN, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ int32_t *dst, __cc__ int32_t *src, uint64_t xm, uint64_t xt)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              xm, xt, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ int32_t *dst, __cc__ int32_t *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              NSize, MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE,
                              channelSplit, NZ2ND_EN, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ half *dst, __cc__ int32_t *src, uint64_t xm, uint64_t xt)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              xm, xt, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ half *dst, __cc__ int32_t *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              NSize, MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE,
                              channelSplit, NZ2ND_EN, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ int16_t *dst, __cc__ int32_t *src, uint64_t xm, uint64_t xt)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              xm, xt, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ int16_t *dst, __cc__ int32_t *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              NSize, MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE,
                              channelSplit, NZ2ND_EN, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ int8_t *dst, __cc__ int32_t *src, uint64_t xm, uint64_t xt)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              xm, xt, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ int8_t *dst, __cc__ int32_t *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              NSize, MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE,
                              channelSplit, NZ2ND_EN, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ uint8_t *dst, __cc__ int32_t *src, uint64_t xm, uint64_t xt)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              xm, xt, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf, __cbuf__ uint8_t *dst, __cc__ int32_t *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              NSize, MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE,
                              channelSplit, NZ2ND_EN, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf_b4, __cbuf__ void *dst, __cc__ float *src, uint64_t xm, uint64_t xt)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              xm, xt, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf_b4, __cbuf__ void *dst, __cc__ float *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              NSize, MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE,
                              channelSplit, NZ2ND_EN, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf_b4, __cbuf__ void *dst, __cc__ int32_t *src, uint64_t xm, uint64_t xt)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              xm, xt, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf_b4, __cbuf__ void *dst, __cc__ int32_t *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              NSize, MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE,
                              channelSplit, NZ2ND_EN, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf_s4, __cbuf__ void *dst, __cc__ float *src, uint64_t xm, uint64_t xt)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              xm, xt, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf_s4, __cbuf__ void *dst, __cc__ float *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              NSize, MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE,
                              channelSplit, NZ2ND_EN, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf_s4, __cbuf__ void *dst, __cc__ int32_t *src, uint64_t xm, uint64_t xt)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              xm, xt, false);
}

SANITIZER_REPORT(copy_matrix_cc_to_cbuf_s4, __cbuf__ void *dst, __cc__ int32_t *src, uint8_t sid,
                 uint16_t NSize, uint16_t MSize, uint32_t dstStride_dst_D, uint16_t srcStride,
                 uint8_t UnitFlagMode, uint64_t QuantPRE, uint8_t ReLUPRE, bool channelSplit, bool NZ2ND_EN)
{
    RecordFixL0CToL1C220Event(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), reinterpret_cast<uint64_t>(src),
                              NSize, MSize, dstStride_dst_D, srcStride, UnitFlagMode, QuantPRE,
                              channelSplit, NZ2ND_EN, false);
}
