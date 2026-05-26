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

#ifndef CORE_FRAMEWORK_PIPELINE_REPLAYER_H
#define CORE_FRAMEWORK_PIPELINE_REPLAYER_H

#include <map>
#include <vector>
#include <functional>
#include <cstdint>

#include "event_container.h"
#include "pipe_line.h"
#include "sync_event_data_base.h"
#include "platform_config.h"
#include "cross_core_sync_info_container.h"
#include "core/framework/barrier_database.h"
#include "core/framework/barrier_database.hpp"
#include "core/framework/event_def.h"

namespace Sanitizer {

// 回放器回调类型枚举
enum class ReplayerCallbackType : uint8_t {
    EVENT_PROCESSING, // 正在处理单个事件
    ALL_DEVICE_STUCK, // 所有 device 均卡死，返回各队列首个未处理事件
};

// 流水线回放器 -- 竞争检测回放逻辑提取，剥离掉竞争检测和向量时钟更新等算法，单纯实现pipe事件的顺序回放
// 1. 所有 VectorClock::UpdateLogicTime / UpdateVectorTime 调用全部删除
// 2. 所有容器接口调用时传入局部空 VectorTime{}（值无意义，不会被 RunAlgorithm 消费）
// 3. 跳过 MEM_EVENT / TIME_EVENT / DYNAMIC_MEM_EVENT（直接返回 PROCESS_OK）
// 4. 不调用 memChecker_.RunAlgorithm()，不进行竞争检测
class PipelineReplayer {
public:
    // 初始化
    void Init(KernelType kernelType, DeviceType deviceType, uint32_t blockDim);

    // 输入：接收 SanEvent
    void Do(const SanEvent& event);

    // 是否已完成回放
    bool IsFinished() const;

    // 注册外部回调，用于感知事件处理顺序及卡死状态
    using EventCallback = std::function<void(ReplayerCallbackType, const SanEvent &)>;
    void RegisterCallback(const EventCallback &callback);

private:
    // 事件处理入口
    ReturnType ProcessEvent(const SanEvent& event);

    // 各类事件处理函数
    ReturnType ProcessSyncEvent(const SanEvent& event);
    ReturnType ProcessBlockSyncEvent(const SanEvent& event);
    ReturnType ProcessBlockSoftSyncEvent(const SanEvent& event);
    ReturnType ProcessMstxCrossSyncEvent(const SanEvent& event);
    ReturnType ProcessMstxCrossCoreBarrier(const SanEvent& event);
    ReturnType ProcessMstxCrossNpuBarrier(const SanEvent& event);
    ReturnType ProcessGetRlsBufSyncEvent(const SanEvent& event);

    // 缓存 MSTX SET_CROSS 计数（纯计数器，不涉及 VT，逻辑与基类相同）
    void CacheMstxCrossSet(const SanEvent& event);

private:
    // 事件处理相关容器
    EventContainer eventContainer_;
    std::vector<SyncEventDataBase> syncDB_;
    CrossCoreSyncInfoContainer crossCoreSyncInfoContainer_;

    using CrossCoreBarrierDatabase = BarrierDatabase<CrossNpuBarrierConf, uint32_t>;
    CrossCoreBarrierDatabase crossCoreBarrier_;

    // key: (addr, flagId), value: set 次数（纯计数，无 VT）
    std::map<std::pair<uint64_t, uint64_t>, uint64_t> mstxSetCrossMap_;

    // key: (blockIdx, bufId), value: rls_buf 事件附带的向量时间列表
    //       仅通过 .size() 判断 rls_buf 数量是否满足 get_buf 的 rlsCount
    std::map<std::pair<uint32_t, uint64_t>, std::vector<VectorTime>> getRlsBufMap_;

    // 回放控制
    KernelType kernelType_;
    DeviceType deviceType_;
    uint32_t blockDim_;
    bool isFinished_ = false;

    // 外部回调
    EventCallback callback_;
};

// 公共部分函数，与竞争检测共用
inline bool NeedExpandBlockDim(KernelType kernelType, DeviceType deviceType)
{
    return kernelType == KernelType::MIX && HasSubBlocks(deviceType);
}

inline uint32_t GetEventExpandBlockIndex(const SanEvent &event)
{
    uint32_t aicoreIndex =
        event.loc.blockType == BlockType::AIVEC ? event.loc.coreId / C220_VEC_SUB_BLOCKDIM : event.loc.coreId;
    uint32_t subBlockIndex =
        event.loc.blockType == BlockType::AIVEC ? event.loc.coreId % C220_VEC_SUB_BLOCKDIM : C220_VEC_SUB_BLOCKDIM;
    return aicoreIndex * C220_MIX_SUB_BLOCKDIM + subBlockIndex;
}

inline uint32_t GetEventBlockIndex(const SanEvent &event, KernelType kernelType, DeviceType deviceType)
{
    if (!NeedExpandBlockDim(kernelType, deviceType)) {
        return event.loc.coreId;
    }
    return GetEventExpandBlockIndex(event);
}

inline uint32_t GetEventBlockIndex(const SanEvent &event, KernelType kernelType, DeviceType deviceType, RaceCheckType checkType)
{
    if (!NeedExpandBlockDim(kernelType, deviceType)) {
        if (checkType == RaceCheckType::SINGLE_BLOCK_CHECK) {
            return 0U;
        }
        return event.loc.coreId;
    }
    return GetEventExpandBlockIndex(event);
}

}  // namespace Sanitizer

#endif  // CORE_FRAMEWORK_PIPELINE_REPLAYER_H
