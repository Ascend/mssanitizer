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
    // 稀疏 -> 稀疏 合并
    static void UpdateVectorTime(const VectorTime &in, VectorTime &out);
    // 比较t1和t2是否存在并发关系（FastTrack 式）
    static bool IsNotHappensBefore(const VectorTime &t1, const VectorTime &t2,
        uint32_t t1Pipe, uint32_t t2Pipe);
    // 分量级并发判定：a1/a2 为 t1Pipe 上的两个分量，b1/b2 为 t2Pipe 上的两个分量
    /// 两个事件在两者各自的pipe上，其中一个pipe上的向量时间大于另外一个事件、另外一个pipe上的向量时间小于另外一个事件，
    /// 说明两个事件存在并发关系
    static bool IsNotHappensBefore(uint32_t a1, uint32_t a2, uint32_t b1, uint32_t b2) {
        if (a1 == a2) { return false; }
        return (a1 < a2) ? (b1 > b2) : (b1 < b2);
    }
    // 判断 t1 是否发生在 t2 之前
    static bool IsHappensBefore(const VectorTime &t1, const VectorTime &t2,
        uint32_t t1Pipe, uint32_t t2Pipe);

    // 测试接口
    static bool IsEqual(const VectorTime &t1, const VectorTime &t2);
};
}

#endif
