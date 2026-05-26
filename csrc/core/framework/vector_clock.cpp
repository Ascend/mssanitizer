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


#include "vector_clock.h"

namespace Sanitizer {

void VectorClock::UpdateLogicTime(VectorTime &t, uint32_t pipe)
{
    t[pipe]++;
}

/// 更新完向量时间之后，对应pipe的逻辑时间需要 +1
void VectorClock::UpdateVectorTime(const VectorTime &in, VectorTime &out)
{
    auto count = std::min(in.size(), out.size());
    for (uint32_t i = 0; i < count; i++) {
        out[i] = std::max(in[i], out[i]);
    }
}

bool VectorClock::IsNotHappensBefore(const VectorTime &t1, const VectorTime &t2, uint32_t t1Pipe, uint32_t t2Pipe)
{
    /// 两个事件在两者各自的pipe上，其中一个pipe上的向量时间大于另外一个事件、另外一个pipe上的向量时间小于另外一个事件，
    /// 说明两个事件存在并发关系
    return (t1[t1Pipe] < t2[t1Pipe] && t1[t2Pipe] > t2[t2Pipe]) || (t1[t2Pipe] < t2[t2Pipe] && t1[t1Pipe] > t2[t1Pipe]);
}

uint32_t VectorClock::GetLogicTime(const VectorTime &t, uint32_t pipe)
{
    return t[pipe];
}

bool VectorClock::IsEqual(const VectorTime &t1, const VectorTime &t2)
{
    return (t1 == t2);
}
}
