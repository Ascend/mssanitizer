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
constexpr uint64_t VSEL_MODESHIFT = 48;
// vsel mode
constexpr uint8_t VSEL_MODE0 = 0;
constexpr uint8_t VSEL_MODE1 = 1;
constexpr uint8_t VSEL_MODE2 = 2;

#if !(defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1 && defined(__DAV_C220_CUBE__))
namespace {

template<typename TO, typename TI0, typename TI1>
AICORE_FUNC_HEAD void RecordVselConfigFunc(EXTRA_PARAMS_DEC,
    __ubuf__ TO *dst, __ubuf__ TI0 *src0, __ubuf__ TI1 *src1, uint64_t config, uint8_t src1BlockSize)
{
    uint8_t selectMode = (config >> VSEL_MODESHIFT) & 0x3;
    if (selectMode == VSEL_MODE0 || selectMode == VSEL_MODE2) {
        RecordBinaryOpConfigFunc<RecordType::VSEL_OP, InstrName::VSEL>(EXTRA_PARAMS, dst, src0, src1, config, 8, 8, 8, 32, 32, 32);
    } else if (selectMode == VSEL_MODE1) {
        uint8_t repeat{};
        uint8_t dstBlockStride{};
        uint8_t src0BlockStride{};
        uint8_t src1BlockStride{};
        uint8_t dstRepeatStride{};
        uint8_t src0RepeatStride{};
        uint8_t src1RepeatStride{};
        ParseBinaryConfig(config, repeat, dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride,
            src0RepeatStride, src1RepeatStride);
        RecordBinaryOpFunc<RecordType::VSEL_OP, InstrName::VSEL>(EXTRA_PARAMS, dst, src0, src1, repeat,
            dstBlockStride, src0BlockStride, 0, dstRepeatStride, src0RepeatStride, 1, 8, 8, 1, 32, 32, src1BlockSize);
    }
}

template <typename TO, typename TI0, typename TI1>
AICORE_FUNC_HEAD void RecordVselOpFunc(EXTRA_PARAMS_DEC, __ubuf__ TO *dst, __ubuf__ TI0 *src0, __ubuf__ TI1 *src1,
    uint8_t repeat, uint16_t dstBlockStride, uint16_t src0BlockStride, uint16_t src1BlockStride,
    uint16_t dstRepeatStride, uint16_t src0RepeatStride, uint16_t src1RepeatStride, uint8_t selectMode,
    uint8_t src1BlockSize) {
    if (selectMode == VSEL_MODE0 || selectMode == VSEL_MODE2) {
        RecordBinaryOpFunc<RecordType::VSEL_OP, InstrName::VSEL>(EXTRA_PARAMS, dst, src0, src1, repeat,
            dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        8, 8, 8, 32, 32, 32);
    } else if (selectMode == VSEL_MODE1) {
        RecordBinaryOpFunc<RecordType::VSEL_OP, InstrName::VSEL>(EXTRA_PARAMS, dst, src0, src1, repeat,
            dstBlockStride, src0BlockStride, 0, dstRepeatStride, src0RepeatStride, 1, 8, 8, 1, 32, 32, src1BlockSize);
    }
}
}

// #3193
SANITIZER_REPORT(vsel, __ubuf__ half *dst, __ubuf__ half *src0, __ubuf__ half *src1, uint64_t config)
{
    RecordVselConfigFunc(EXTRA_PARAMS, dst, src0, src1, config, 16);
}

// #3194
SANITIZER_REPORT(vsel, __ubuf__ half *dst, __ubuf__ half *src0, __ubuf__ void *src1, uint64_t config)
{
    RecordVselConfigFunc(EXTRA_PARAMS, dst, src0, src1, config, 16);
}

SANITIZER_REPORT(vsel, __ubuf__ half *dst, __ubuf__ half *src0, __ubuf__ void *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride) {
    RecordBinaryOpFunc<RecordType::VSEL_OP, InstrName::VSEL>(EXTRA_PARAMS, dst, src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        8, 8, 8, 32, 32, 32);
}

// #3196
SANITIZER_REPORT(vsel, __ubuf__ half *dst, __ubuf__ half *src0, __ubuf__ void *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, uint8_t selectMode, bool repeatStrideMode, bool strideSizeMode)
{
    RecordVselOpFunc(EXTRA_PARAMS, dst, src0, src1, repeat, dstBlockStride, src0BlockStride, src1BlockStride,
     dstRepeatStride, src0RepeatStride, src1RepeatStride, selectMode, 16);
}

// #3197
SANITIZER_REPORT(vsel, __ubuf__ half *dst, __ubuf__ half *src0, __ubuf__ void *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, uint8_t selectMode)
{
    RecordVselOpFunc(EXTRA_PARAMS, dst, src0, src1, repeat, dstBlockStride, src0BlockStride, src1BlockStride,
     dstRepeatStride, src0RepeatStride, src1RepeatStride, selectMode, 16);
}

// #3198
SANITIZER_REPORT(vsel, __ubuf__ half *dst, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride) {
    RecordBinaryOpFunc<RecordType::VSEL_OP, InstrName::VSEL>(EXTRA_PARAMS, dst, src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        8, 8, 8, 32, 32, 32);
}

// #3199
SANITIZER_REPORT(vsel, __ubuf__ half *dst, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, uint8_t selectMode, bool repeatStrideMode, bool strideSizeMode)
{
    RecordVselOpFunc(EXTRA_PARAMS, dst, src0, src1, repeat, dstBlockStride, src0BlockStride, src1BlockStride,
     dstRepeatStride, src0RepeatStride, src1RepeatStride, selectMode, 16);
}

// #3200
SANITIZER_REPORT(vsel, __ubuf__ half *dst, __ubuf__ half *src0, __ubuf__ half *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, uint8_t selectMode)
{
    RecordVselOpFunc(EXTRA_PARAMS, dst, src0, src1, repeat, dstBlockStride, src0BlockStride, src1BlockStride,
     dstRepeatStride, src0RepeatStride, src1RepeatStride, selectMode, 16);
}

// #3201
SANITIZER_REPORT(vsel, __ubuf__ float *dst, __ubuf__ float *src0, __ubuf__ float *src1, uint64_t config)
{
    RecordVselConfigFunc(EXTRA_PARAMS, dst, src0, src1, config, 8);
}

// #3202
SANITIZER_REPORT(vsel, __ubuf__ float *dst, __ubuf__ float *src0, __ubuf__ void *src1, uint64_t config)
{
    RecordVselConfigFunc(EXTRA_PARAMS, dst, src0, src1, config, 8);
}

// #3203
SANITIZER_REPORT(vsel, __ubuf__ float *dst, __ubuf__ float *src0, __ubuf__ void *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride) {
    RecordBinaryOpFunc<RecordType::VSEL_OP, InstrName::VSEL>(EXTRA_PARAMS, dst, src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        8, 8, 8, 32, 32, 32);
}

// #3204
SANITIZER_REPORT(vsel, __ubuf__ float *dst, __ubuf__ float *src0, __ubuf__ void *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, uint8_t selectMode, bool repeatStrideMode, bool strideSizeMode)
{
    RecordVselOpFunc(EXTRA_PARAMS, dst, src0, src1, repeat, dstBlockStride, src0BlockStride, src1BlockStride,
     dstRepeatStride, src0RepeatStride, src1RepeatStride, selectMode, 8);
}

// #3205
SANITIZER_REPORT(vsel, __ubuf__ float *dst, __ubuf__ float *src0, __ubuf__ void *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, uint8_t selectMode)
{
    RecordVselOpFunc(EXTRA_PARAMS, dst, src0, src1, repeat, dstBlockStride, src0BlockStride, src1BlockStride,
     dstRepeatStride, src0RepeatStride, src1RepeatStride, selectMode, 8);
}

// #3206
SANITIZER_REPORT(vsel, __ubuf__ float *dst, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride) {
    RecordBinaryOpFunc<RecordType::VSEL_OP, InstrName::VSEL>(EXTRA_PARAMS, dst, src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        8, 8, 8, 32, 32, 32);
}

// #3207
SANITIZER_REPORT(vsel, __ubuf__ float *dst, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, uint8_t selectMode, bool repeatStrideMode, bool strideSizeMode)
{
    RecordVselOpFunc(EXTRA_PARAMS, dst, src0, src1, repeat, dstBlockStride, src0BlockStride, src1BlockStride,
     dstRepeatStride, src0RepeatStride, src1RepeatStride, selectMode, 8);
}

// #3208
SANITIZER_REPORT(vsel, __ubuf__ float *dst, __ubuf__ float *src0, __ubuf__ float *src1, uint8_t repeat,
    uint8_t dstBlockStride, uint8_t src0BlockStride, uint8_t src1BlockStride, uint8_t dstRepeatStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, uint8_t selectMode)
{
    RecordVselOpFunc(EXTRA_PARAMS, dst, src0, src1, repeat, dstBlockStride, src0BlockStride, src1BlockStride,
     dstRepeatStride, src0RepeatStride, src1RepeatStride, selectMode, 8);
}

#endif
