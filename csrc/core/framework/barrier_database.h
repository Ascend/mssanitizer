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

#ifndef CORE_FRAMEWORK_BARRIER_DATABASE_H
#define CORE_FRAMEWORK_BARRIER_DATABASE_H

#include <unordered_map>
#include <unordered_set>

#include "core/framework/event_def.h"
#include "core/framework/utility/numeric.h"

namespace Sanitizer {

/**
 * @brief 用来管理 barrier 事件配对的管理类
 * @tparam Worker - 处理线程对象类型，需要实现哈希和判等运算
 *
 * barrier 同步分为三个阶段：
 * 1. 当到达同步的线程数不足时，会记录当前到达的线程信息，并更新全局时间，Wait 返回 false 表示线程阻塞；
 * 2. 当到达同步的线程数到达指定数量后，进入消耗阶段，会消耗当前的线程信息，并返回全局时间，Wait
 *    返回 true 表示线程不阻塞；
 * 3. 当所有记录的线程都被消耗后，恢复初始状态；
 */
template <typename Worker>
class BarrierEvent {
public:
    /**
     * @brief 同步等待。线程到达同步点后进行等待，并返回同步后的全局向量时间
     * @param rank [IN] 期待到达同步点的总线程数
     * @param worker [IN] 当前到达同步点的线程对象
     * @param vtSelf [IN] 当前线程的本地时间
     * @param vtGlobal [OUT] 返回同步后的全局时间，仅返回值为 true 时有效
     * @return 是否有足够的线程到达同步点
     */
    bool Wait(std::size_t rank, Worker worker, VectorTime const &vtSelf, VectorTime &vtGlobal);

private:
    /**
     * @brief 是否有足够的线程到达同步点
     */
    bool IsAllReachBarrier(std::size_t rank) const;

    /**
     * @brief 是否所有线程都已被消耗
     */
    bool IsAllConsumeBarrier() const;

private:
    bool allReachBarrier_{};
    VectorTime vtGlobal_{};
    std::unordered_set<Worker, Hash<Worker>> reachedWorkers_{};
};

/**
 * @brief 针对不同的配置对同步事件进行独立管理
 * @tparam Conf - 配置类型，需要实现哈希和判等运算
 * @tparam Worker - 处理线程对象类型，需要实现哈希和判等运算
 */
template <typename Conf, typename Worker>
class BarrierDatabase {
public:
    BarrierEvent<Worker> &operator[](Conf conf);
    void clear() { barrierEvents_.clear(); }

private:
    std::unordered_map<Conf, BarrierEvent<Worker>, Hash<Conf>> barrierEvents_;
};

struct CrossNpuBarrierConf {
    bool isAIVOnly;
    uint32_t usedDeviceNum;
    uint32_t usedCoreNum;
};

template <>
struct Hash<CrossNpuBarrierConf> {
    std::size_t operator()(CrossNpuBarrierConf const &item) const {
        return
            std::hash<bool>()(item.isAIVOnly) ^ std::hash<uint32_t>()(item.usedDeviceNum) ^
            std::hash<uint32_t>()(item.usedCoreNum);
    }
};

inline bool operator==(CrossNpuBarrierConf const &lhs, CrossNpuBarrierConf const &rhs)
{
    return
        lhs.isAIVOnly == rhs.isAIVOnly && lhs.usedDeviceNum == rhs.usedDeviceNum &&
        lhs.usedCoreNum == rhs.usedCoreNum;
}

} // namespace Sanitizer

#endif // RACE_SANITIZER_ALG_FRAMEWORK_BARRIER_DATABASE_H
