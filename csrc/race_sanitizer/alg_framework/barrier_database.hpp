/* -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#ifndef RACE_SANITIZER_ALG_FRAMEWORK_BARRIER_DATABASE_HPP
#define RACE_SANITIZER_ALG_FRAMEWORK_BARRIER_DATABASE_HPP

#include "vector_clock.h"

#include "barrier_database.h"

namespace Sanitizer {

template <typename Worker>
bool BarrierEvent<Worker>::Wait(std::size_t rank, Worker worker, VectorTime const &vtSelf, VectorTime &vtGlobal)
{
    // 1. 到达同步的 pipe 数不足，记录 pipe 信息并更新向量时间
    if (!allReachBarrier_) {
        reachedWorkers_.insert(worker);
        if (vtGlobal_.empty()) {
            vtGlobal_ = vtSelf;
        } else {
            VectorClock::UpdateVectorTime(vtSelf, vtGlobal_);
        }

        if (IsAllReachBarrier(rank)) {
            allReachBarrier_ = true;
        } else {
            return false;
        }
    }

    auto it = reachedWorkers_.find(worker);
    if (it == reachedWorkers_.cend()) {
        return false;
    }

    // 2. 进入消耗阶段，消耗当前的 pipe 信息，并返回全局时间
    reachedWorkers_.erase(it);
    vtGlobal = vtGlobal_;

    // 3. 所有记录的 pipe 都被消耗，恢复初始状态
    if (IsAllConsumeBarrier()) {
        allReachBarrier_ = false;
        vtGlobal_.clear();
        reachedWorkers_.clear();
    }
    return true;
}

template <typename Worker>
bool BarrierEvent<Worker>::IsAllReachBarrier(std::size_t rank) const
{
    return reachedWorkers_.size() >= rank;
}

template <typename Worker>
bool BarrierEvent<Worker>::IsAllConsumeBarrier() const
{
    return reachedWorkers_.empty();
}

template <typename Conf, typename Worker>
BarrierEvent<Worker> &BarrierDatabase<Conf, Worker>::operator[](Conf conf)
{
    return barrierEvents_[conf];
}

} // namespace Sanitizer

#endif // RACE_SANITIZER_ALG_FRAMEWORK_BARRIER_DATABASE_HPP