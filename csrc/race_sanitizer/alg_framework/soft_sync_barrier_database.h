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

#ifndef RACE_SANITIZER_ALG_FRAMEWORK_SOFT_SYNC_BARRIER_DATABASE_H
#define RACE_SANITIZER_ALG_FRAMEWORK_SOFT_SYNC_BARRIER_DATABASE_H

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "core/framework/event_def.h"
#include "sanitizer_report.h"

namespace Sanitizer {

class SoftSyncBarrierDatabase {
public:
    /**
     * @brief 软同步事件配置，每个配置会有独立的实例进行事件配对管理
     */
    struct BarrierConf {
        bool isAIVOnly;
        uint32_t usedDeviceNum;
        uint32_t usedCoreNum;
    };

    /**
     * @brief 实现 BarrierConf 类哈希方法
     */
    struct BarrierConfHash {
        std::size_t operator()(BarrierConf const &item) const
        {
            return
                std::hash<bool>()(item.isAIVOnly) ^ std::hash<uint32_t>()(item.usedDeviceNum) ^
                std::hash<uint32_t>()(item.usedCoreNum);
        }
    };

    /**
     * @brief 实现 BarrierConf 类判等方法
     */
    struct BarrierConfEqual {
        bool operator()(BarrierConf const &lhs, BarrierConf const &rhs) const noexcept
        {
            return
                lhs.isAIVOnly == rhs.isAIVOnly && lhs.usedDeviceNum == rhs.usedDeviceNum &&
                lhs.usedCoreNum == rhs.usedCoreNum;
        }
    };

    /**
     * @brief 用来管理 barrier 事件配对的管理类
     * barrier 同步分为三个阶段：
     * 1. 当到达同步的 pipe 数不足时，会记录当前到达的 pipe 信息，并更新全局时间，Wait 返回 false 表示
     *    pipe 阻塞；
     * 2. 当到达同步的 pipe 数到达指定数量后，进入消耗阶段，会消耗当前的 pipe 信息，并返回全局时间，Wait
     *    返回 true 表示 pipe 不阻塞；
     * 3. 当所有记录的 pipe 都被消耗后，恢复初始状态；
     */
    class BarrierEvent {
    public:
        /**
         * @brief 核间同步等待。pipe 到达同步点后进行等待，并返回同步后的全局向量时间
         * @param blockIdx [IN] 到达同步点的 blockIdx
         * @param barrier [IN] 同步点 barrier 事件参数
         * @param vtSelf [IN] 当前 pipe 的本地时间
         * @param vtGlobal [OUT] 返回同步后的全局时间，仅返回值为 true 时有效
         * @return 是否有足够的 pipe 到达同步点
         */
        bool Wait(uint32_t blockIdx, MstxCrossCoreBarrier const &barrier,
                  VectorTime const &vtSelf, VectorTime &vtGlobal);

        /**
         * @brief 卡间同步等待。pipe 到达同步点后进行等待，并返回同步后的全局向量时间
         * @param deviceId [IN] 到达同步点的 deviceId
         * @param blockIdx [IN] 到达同步点的 blockIdx
         * @param barrier [IN] 同步点 barrier 事件参数
         * @param vtSelf [IN] 当前 pipe 的本地时间
         * @param vtGlobal [OUT] 返回同步后的全局时间，仅返回值为 true 时有效
         * @return 是否有足够的 pipe 到达同步点
         */
        bool Wait(uint32_t deviceId, uint32_t blockIdx, MstxCrossNpuBarrier const &barrier,
                  VectorTime const &vtSelf, VectorTime &vtGlobal);

    private:
        /**
         * @brief 是否有足够的 pipe 到达同步点
         */
        bool IsAllReachBarrier(MstxCrossNpuBarrier const &barrier) const;

        /**
         * @brief 是否所有 pipe 都已被消耗
         */
        bool IsAllConsumeBarrier() const;

    private:
        BarrierConf conf_;
        bool allReachBarrier_{};
        VectorTime vtGlobal_{};
        std::unordered_map<uint32_t, std::unordered_set<uint32_t>> usedCores_;
    };

public:
    BarrierEvent &operator[](BarrierConf const &conf);

private:
    std::unordered_map<BarrierConf, BarrierEvent, BarrierConfHash, BarrierConfEqual> barrierEvents_;
};

} // namespace Sanitizer

#endif // RACE_SANITIZER_ALG_FRAMEWORK_SOFT_SYNC_BARRIER_DATABASE_H