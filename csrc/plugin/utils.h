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

#ifndef PLUGIN_UTILS_H
#define PLUGIN_UTILS_H

#include "core/framework/utility/macros.h"
#include "ccec_defines.h"

#if !(defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1)
#include "plugin/clang_interface.h"
#endif

#if defined(BUILD_TRACEKIT)
#define INSTR_STUB_PREFIX __mstrace_report_
#else
#define INSTR_STUB_PREFIX __sanitizer_report_
#endif

#if defined(BUILD_DYNAMIC_PROBE)
#define EXTRA_PARAMS memInfo, pc, bid
#define EXTRA_PARAMS_DEC __gm__ uint8_t *memInfo, int64_t pc, uint32_t bid
#else
#define EXTRA_PARAMS memInfo, fileNo, lineNo, pc
#define EXTRA_PARAMS_DEC __gm__ uint8_t *memInfo, uint64_t fileNo, uint64_t lineNo, int64_t pc
#endif

// AICORE 平台使用的 SANITIZER_REPORT 宏，用于声明桩函数接口
#ifdef SIMT_MODE // simt
#define SANITIZER_REPORT_AICORE(func, ...) \
    extern __attribute__((noinline)) __attribute__((weak)) __simt_callee__ __aicore__ void \
    MACRO_CONCAT(INSTR_STUB_PREFIX, func)(EXTRA_PARAMS_DEC, ## __VA_ARGS__)
    #define AICORE_FUNC_HEAD __simt_callee__ __aicore__ inline
#else         // simd
#define SANITIZER_REPORT_AICORE(func, ...) \
    extern __attribute__((noinline)) __attribute__((weak)) __aicore__ void \
    MACRO_CONCAT(INSTR_STUB_PREFIX, func)(EXTRA_PARAMS_DEC, ## __VA_ARGS__)
     #define AICORE_FUNC_HEAD __aicore__ inline
#endif // SIMT_MODE

// HOST 平台使用的 SANITIZER_REPORT 宏，在函数声明前通过 REPORT_API_REGISTER
// 宏将接口注册到查询库中，同时通过指定 hidden 属性在查询动态库中隐藏桩接口定义
#define SANITIZER_REPORT_HOST(func, ...) \
    REPORT_API_REGISTER(func); \
    __attribute__((visibility("hidden"))) void \
    __sanitizer_report_##func(EXTRA_PARAMS_DEC, ## __VA_ARGS__)

#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
#define SANITIZER_REPORT SANITIZER_REPORT_AICORE
#else
#define SANITIZER_REPORT SANITIZER_REPORT_HOST
#endif

// transform config[leftBit, rightBit] into an unsigned integer
template<uint8_t leftBit, uint8_t rightBit, typename confT>
AICORE_FUNC_HEAD uint64_t GetUintFromConf(confT config)
{
    constexpr uint8_t bitsPerByte= 8U;
    constexpr uint8_t maxBit = sizeof(config) * bitsPerByte - 1;
    constexpr uint64_t mask = ~0x0ULL;
    static_assert(leftBit >= rightBit);
    static_assert(leftBit <= maxBit);
    return (config >> rightBit) & (mask >> (63 - leftBit + rightBit));
}

// get a single bit
template<uint8_t leftBit, typename confT>
AICORE_FUNC_HEAD uint64_t GetUintFromConf(confT config)
{
    return GetUintFromConf<leftBit, leftBit>(config);
}

// config[leftBit, rightBit] = val
template<uint8_t leftBit, uint8_t rightBit, typename confT, typename uintT>
AICORE_FUNC_HEAD void SetConfByUint(confT &config, uintT val)
{
    constexpr uint8_t bitsPerByte= 8U;
    constexpr uint8_t maxBit = sizeof(config) * bitsPerByte - 1;
    static_assert(leftBit >= rightBit);
    static_assert(leftBit <= maxBit);

    uint64_t mask = ~0x0ULL;
    mask >>= 63 - leftBit + rightBit;
    uint64_t otherMask = ~(mask << rightBit);
    config &= otherMask;
    mask = val & mask;
    mask <<= rightBit;
    config |= mask;
}

#endif  // PLUGIN_UTILS_H
