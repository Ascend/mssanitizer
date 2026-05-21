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

#ifndef CORE_FRAMEWORK_ARCH_DEF_H
#define CORE_FRAMEWORK_ARCH_DEF_H

#include <cstdint>

// 定义和芯片体系相关的数据信息

namespace Sanitizer {

/// AclrtMemcpy2d的高度最大值
constexpr uint64_t MAX_MEMORY_RECORD_HEIGHT = 60ULL * 1024 * 1024 * 1024;

/// 此枚举定义与编译器内置类型 pipe_t 保持一致，编译器内存类型为uint32_t，不会变动
enum class PipeType : uint32_t {
    PIPE_S = 0,      // Scalar Pipe，此处指代发射流水，并非load/store标量的执行流水
    PIPE_V,          // Vector Pipe, including{VectorOP write UB,  L0C->UB write}
    PIPE_M,          // Matrix Pipe, including{}
    PIPE_MTE1,       // L1->L0{A,B}
    PIPE_MTE2,       // OUT ->{L1, L0{A,B}, UB}
    PIPE_MTE3,       // UB ->{OUT,L1}
    PIPE_ALL,
    PIPE_MTE4 = 7,   // MOV_UB_TO_OUT
    PIPE_MTE5 = 8,   // MOV_OUT_TO_UB
    PIPE_V2 = 9,     // Lower priority vector pipe,
    PIPE_FIX = 10,   // {L0C} ->{L1,UB,L1UB}
    PIPE_S_CAL,      // 主要负责pipe_s上的load/store标量指令计算流水
    SIZE,
};

/// 此枚举定义与编译器内置类型 mem_t 保持一致
enum class MemType : uint8_t {
    L1 = 0,
    L0A,
    L0B,
    L0C,
    UB,
    BT,         // bias table
    GM,
    PRIVATE,
    FB,         // fixPipe buffer
    INVALID,
};

enum class DeviceType : uint32_t {
    ASCEND_910_PREMIUM_A = 0U,

    ASCEND_910B_START,
    ASCEND_910B1,
    ASCEND_910B2,
    ASCEND_910B3,
    ASCEND_910B4,
    ASCEND_910B_END,

    ASCEND_310P,

    ASCEND_950_START = 10,
    ASCEND_950DT_950x,
    ASCEND_950DT_950y,
    ASCEND_950DT_9571,
    ASCEND_950DT_9572,
    ASCEND_950DT_9573,
    ASCEND_950DT_9574,
    ASCEND_950DT_9575,
    ASCEND_950DT_9576,
    ASCEND_950DT_9577,
    ASCEND_950DT_9578,
    ASCEND_950DT_9581,
    ASCEND_950DT_9582,
    ASCEND_950DT_9583,
    ASCEND_950DT_9584,
    ASCEND_950DT_9585,
    ASCEND_950DT_9586,
    ASCEND_950DT_9587,
    ASCEND_950DT_9588,
    ASCEND_950DT_9591,
    ASCEND_950DT_9592,
    ASCEND_950DT_9595,
    ASCEND_950DT_9596,
    ASCEND_950DT_95A1,
    ASCEND_950DT_95A2,
    ASCEND_950PR_950z,
    ASCEND_950PR_9579,
    ASCEND_950PR_957b,
    ASCEND_950PR_957c,
    ASCEND_950PR_957d,
    ASCEND_950PR_9589,
    ASCEND_950PR_958b,
    ASCEND_950PR_9599,
    ASCEND_950_END,

    INVALID = 0xFFFFFFFFU,
};

enum class KernelType : uint8_t {
    AICPU = 0,        /// 对应 RTS 中的 RT_DEV_BINARY_MAGIC_ELF_AICPU
    AIVEC,            /// 对应 RTS 中的 RT_DEV_BINARY_MAGIC_ELF_AIVEC
    AICUBE,           /// 对应 RTS 中的 RT_DEV_BINARY_MAGIC_ELF_AICUBE
    MIX,              /// 对应 RTS 中的 RT_DEV_BINARY_MAGIC_ELF
};

}  // namespace Sanitizer

#endif
