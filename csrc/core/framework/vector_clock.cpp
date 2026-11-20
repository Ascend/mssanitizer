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

#include <cstddef>
#include <cstdint>

namespace Sanitizer {

void VectorClock::UpdateLogicTime(VectorTime &t, uint32_t pipe) {
    t.Set(pipe, t.Get(pipe) + 1U);
}

void VectorClock::UpdateVectorTime(const VectorTime &in, VectorTime &out) {
    constexpr std::size_t kLinearMergeMin = 64U;
    const std::size_t inN = in.heap ? in.heapEntries.size() : in.n;
    if (inN >= kLinearMergeMin) {
        out.MergeMax(in);
        return;
    }
    // 分量较少时逐分量 Set 更省（避免线性归并的统计/搬移开销）
    if (!in.heap) {
        for (uint16_t k = 0; k < in.n; ++k) { out.Set(in.idx[k], in.val[k]); }
        return;
    }
    for (const auto &e : in.heapEntries) { out.Set(e.first, e.second); }
}

bool VectorClock::IsNotHappensBefore(const VectorTime &t1, const VectorTime &t2,
    uint32_t t1Pipe, uint32_t t2Pipe) {
    // 短路：t1Pipe 分量相等时两式首项均不成立，直接 false（省 2 次 Get）
    const uint32_t a1 = t1.Get(t1Pipe);
    const uint32_t a2 = t2.Get(t1Pipe);
    if (a1 == a2) {
        return false;
    }
    const uint32_t b1 = t1.Get(t2Pipe);
    const uint32_t b2 = t2.Get(t2Pipe);
    return IsNotHappensBefore(a1, a2, b1, b2);
}

bool VectorClock::IsHappensBefore(const VectorTime &t1, const VectorTime &t2,
    uint32_t t1Pipe, uint32_t t2Pipe) {
    if (t1.Get(t1Pipe) > t2.Get(t1Pipe)) {
        return false;
    }
    return t1.Get(t2Pipe) <= t2.Get(t2Pipe);
}

bool VectorClock::IsEqual(const VectorTime &t1, const VectorTime &t2) {
    return (t1 == t2);
}
}
