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

#include <tuple>
#include <utility>

#include "race_sanitizer/alg_framework/vector_clock.h"
#include "sanitizer_report.h"

#include "soft_sync_barrier_database.h"

namespace Sanitizer {

bool SoftSyncBarrierDatabase::BarrierEvent::Wait(uint32_t blockIdx, MstxCrossCoreBarrier const &barrier,
                                                 VectorTime const &vtSelf, VectorTime &vtGlobal)
{
    MstxCrossNpuBarrier crossNpuBarrier{};
    crossNpuBarrier.isAIVOnly = barrier.isAIVOnly;
    crossNpuBarrier.usedDeviceNum = 1;
    crossNpuBarrier.usedDeviceId = nullptr;
    crossNpuBarrier.usedCoreNum = barrier.usedCoreNum;
    crossNpuBarrier.usedCoreId = barrier.usedCoreId;
    return Wait(0, blockIdx, crossNpuBarrier, vtSelf, vtGlobal);
}

bool SoftSyncBarrierDatabase::BarrierEvent::Wait(uint32_t deviceId, uint32_t blockIdx,
                                                 MstxCrossNpuBarrier const &barrier,
                                                 VectorTime const &vtSelf, VectorTime &vtGlobal)
{
    // 1. 到达同步的 pipe 数不足，记录 pipe 信息并更新向量时间
    if (!allReachBarrier_) {
        usedCores_[deviceId].insert(blockIdx);
        if (vtGlobal_.empty()) {
            vtGlobal_ = vtSelf;
        } else {
            VectorClock::UpdateVectorTime(vtSelf, vtGlobal_);
        }

        if (IsAllReachBarrier(barrier)) {
            allReachBarrier_ = true;
        } else {
            return false;
        }
    }

    auto deviceIt = usedCores_.find(deviceId);
    if (deviceIt == usedCores_.cend()) {
        return false;
    }
    auto blockIt = deviceIt->second.find(blockIdx);
    if (blockIt == deviceIt->second.cend()) {
        return false;
    }

    // 2. 进入消耗阶段，消耗当前的 pipe 信息，并返回全局时间
    deviceIt->second.erase(blockIt);
    if (deviceIt->second.empty()) {
        usedCores_.erase(deviceIt);
    }

    vtGlobal = vtGlobal_;

    // 3. 所有记录的 pipe 都被消耗，恢复初始状态
    if (IsAllConsumeBarrier()) {
        allReachBarrier_ = false;
        vtGlobal_.clear();
        usedCores_.clear();
    }
    return true;
}

bool SoftSyncBarrierDatabase::BarrierEvent::IsAllReachBarrier(MstxCrossNpuBarrier const &barrier) const
{
    // 到达同步点的 device 数或 block 数有一个不足就返回 false
    if (usedCores_.size() < barrier.usedDeviceNum) {
        return false;
    }

    for (auto const &deviceIt : usedCores_) {
        if (deviceIt.second.size() < barrier.usedCoreNum) {
            return false;
        }
    }
    return true;
}

bool SoftSyncBarrierDatabase::BarrierEvent::IsAllConsumeBarrier() const
{
    for (auto const &devicePair : usedCores_) {
        if (!devicePair.second.empty()) {
            return false;
        }
    }
    return true;
}

SoftSyncBarrierDatabase::BarrierEvent &SoftSyncBarrierDatabase::operator[](BarrierConf const &conf)
{
    return barrierEvents_[conf];
}

} // namespace Sanitizer