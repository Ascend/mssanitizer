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


#ifndef CORE_FRAMEWORK_VECTOR_CLOCK_H
#define CORE_FRAMEWORK_VECTOR_CLOCK_H

#include <array>
#include "core/framework/event_def.h"

namespace Sanitizer {

// 该类提供了以下操作向量时钟的方法
// 提供初始化向量时钟的方法
// 提供更新PIPE逻辑时间方法
// 提供更新PIPE向量时间方法
// 提供比较两个时间是否满足happens-before关系方法
class VectorClock {
public:
    // 更新逻辑时间
    static void UpdateLogicTime(VectorTime &t, uint32_t pipe);
    // 更新向量时间
    static void UpdateVectorTime(const VectorTime &in, VectorTime &out);
    // 比较t1和t2是否存在并发关系
    static bool IsNotHappensBefore(const VectorTime &t1, const VectorTime &t2, uint32_t t1Pipe, uint32_t t2Pipe);

    // 测试接口
    static uint32_t GetLogicTime(const VectorTime &t, uint32_t pipe);
    static bool IsEqual(const VectorTime &t1, const VectorTime &t2);
};
}

#endif
