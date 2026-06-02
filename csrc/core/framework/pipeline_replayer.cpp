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

#include "pipeline_replayer.h"
#include "core/framework/utility/log.h"

namespace Sanitizer {

void PipelineReplayer::Init(KernelType kernelType, DeviceType deviceType, uint32_t blockDim)
{
    kernelType_ = kernelType;
    deviceType_ = deviceType;
    blockDim_ = blockDim;
    uint32_t totalBlockNum = NeedExpandBlockDim(kernelType, deviceType)
        ? blockDim * C220_MIX_SUB_BLOCKDIM : blockDim;
    eventContainer_.Init(totalBlockNum);
    syncDB_.resize(totalBlockNum);
    crossCoreSyncInfoContainer_.Init(totalBlockNum, kernelType);

    getRlsBufMap_.clear();
    getBufCount_.clear();
    mstxSetCrossMap_.clear();
    crossCoreBarrier_.clear();
    isFinished_ = false;
}

void PipelineReplayer::Do(const SanEvent& event)
{
    if (event.type != EventType::SANITIZER_CONTROL_EVENT ||
        event.eventInfo.sanitizerControlInfo.type != SanitizerControlType::KERNEL_FINISH) {
        // 收集阶段：缓存 MSTX set 计数，并将事件推入 eventContainer_
        CacheMstxCrossSet(event);
        auto blockIndex = GetEventBlockIndex(event, kernelType_, deviceType_);
        eventContainer_.Push(event, PipeType::PIPE_S, blockIndex);
        return;
    }

    // KERNEL_FINISH：启动流水线回放
    PipeLine pipeLine(eventContainer_);
    pipeLine.RegisterEventFunc(
        std::bind(&PipelineReplayer::ProcessEvent, this, std::placeholders::_1));
    pipeLine.Run();

    // 若回放因卡死退出（队列非空），通知外部各队列首个未处理事件
    if (callback_ && !eventContainer_.IsEmpty()) {
        eventContainer_.ForEachFrontEvent(
            [this](const SanEvent &stuckEvent) { callback_(ReplayerCallbackType::ALL_DEVICE_STUCK, stuckEvent); });
    }

    isFinished_ = true;
}

bool PipelineReplayer::IsFinished() const
{
    return isFinished_;
}

void PipelineReplayer::RegisterCallback(const EventCallback &callback) { callback_ = callback; }

// MSTX set 计数缓存，与原版 CacheMstxCrossSet 逻辑一致
void PipelineReplayer::CacheMstxCrossSet(const SanEvent& event)
{
    if (event.type == EventType::MSTX_CROSS_SYNC_EVENT &&
        event.eventInfo.mstxCrossInfo.opType == SyncType::MSTX_SET_CROSS) {
        /// 查找mstx中多核同步对应的set指令
        auto key = std::make_pair(event.eventInfo.mstxCrossInfo.addr,
                                  event.eventInfo.mstxCrossInfo.flagId);
        auto iter = mstxSetCrossMap_.find(key);
        if (iter != mstxSetCrossMap_.end()) {
            iter->second++;                     // 次数+1
        } else {
            mstxSetCrossMap_[key] = 1;          // 初始值设置为1
        }
    }
}

// 事件分发
ReturnType PipelineReplayer::ProcessEvent(const SanEvent& event)
{
    // 通知外部当前正在处理的事件
    if (callback_) {
        callback_(ReplayerCallbackType::EVENT_PROCESSING, event);
    }

    // PIPE_S 发射到目标 PIPE：不需要 TIME_EVENT 和向量时钟，直接推到目标 PIPE
    if (eventContainer_.GetPipeIndex() != event.pipe) {
        auto blockIdx = GetEventBlockIndex(event, kernelType_, deviceType_);
        eventContainer_.Push(event, event.pipe, blockIdx);
        return ReturnType::PROCESS_OK;
    }

    switch (event.type) {
        case EventType::SYNC_EVENT:
            return ProcessSyncEvent(event);
        case EventType::CROSS_CORE_SYNC_EVENT:
            return ProcessBlockSyncEvent(event);
        case EventType::CROSS_CORE_SOFT_SYNC_EVENT:
            return ProcessBlockSoftSyncEvent(event);
        case EventType::MSTX_CROSS_SYNC_EVENT:
            return ProcessMstxCrossSyncEvent(event);
        case EventType::MSTX_CROSS_CORE_BARRIER:
            return ProcessMstxCrossCoreBarrier(event);
        case EventType::MSTX_CROSS_NPU_BARRIER:
            return ProcessMstxCrossNpuBarrier(event);
        case EventType::BUF_SYNC_EVENT:
            return ProcessGetRlsBufSyncEvent(event);
        // 内存/时间/动态内存事件 —— 不涉及同步，直接通过
        case EventType::MEM_EVENT:
        case EventType::TIME_EVENT:
        case EventType::DYNAMIC_MEM_EVENT:
            return ReturnType::PROCESS_OK;
        default:
            break;
    }
    return ReturnType::PROCESS_OK;
}

// PIPE 间同步 (set_flag / wait_flag)，复用 SyncEventDataBase
ReturnType PipelineReplayer::ProcessSyncEvent(const SanEvent& event)
{
    auto e = SyncEvent{};
    e.info.srcPipe = static_cast<uint8_t>(event.eventInfo.syncInfo.srcPipe);
    e.info.dstPipe = static_cast<uint8_t>(event.eventInfo.syncInfo.dstPipe);
    e.info.eventId = static_cast<uint8_t>(event.eventInfo.syncInfo.eventId);
    e.info.memType = static_cast<uint8_t>(event.eventInfo.syncInfo.memType);
    e.info.isRetrogress = event.eventInfo.syncInfo.isRetrogress;

    uint32_t blockIdx = GetEventBlockIndex(event, kernelType_, deviceType_);

    if (event.eventInfo.syncInfo.opType == SyncType::SET_FLAG) {
        VectorTime vt{};
        syncDB_[blockIdx].Set(e, vt);
        return ReturnType::PROCESS_OK;
    }

    if (event.eventInfo.syncInfo.opType == SyncType::WAIT_FLAG) {
        VectorTime vt{};
        if (syncDB_[blockIdx].Get(e, vt)) {
            return ReturnType::PROCESS_OK;
        } else {
            return ReturnType::PROCESS_STALLED;
        }
    }

    return ReturnType::PROCESS_OK;
}

// 核间硬同步 (FFTS: Mode0~4)，复用 CrossCoreSyncInfoContainer
ReturnType PipelineReplayer::ProcessBlockSyncEvent(const SanEvent& event)
{
    uint32_t blockIndex = GetEventBlockIndex(event, kernelType_, deviceType_);
    auto& fftsInfo = event.eventInfo.fftsSyncInfo;
    VectorTime vt{};

    if (fftsInfo.opType == SyncType::FFTS_SYNC) {
        crossCoreSyncInfoContainer_.SetBlockSyncInfo(fftsInfo.flagId,
            static_cast<FftsSyncMode>(fftsInfo.mode), blockIndex, vt, fftsInfo.vecSubBlockDim);
        return ReturnType::PROCESS_OK;
    } else if (fftsInfo.opType == SyncType::WAIT_FLAG_DEV) {
        if (crossCoreSyncInfoContainer_.GetBlockSyncInfo(fftsInfo.flagId, blockIndex, vt)) {
            return ReturnType::PROCESS_OK;
        }
    } else if (fftsInfo.opType == SyncType::WAIT_INTRA_BLOCK) {
        if (crossCoreSyncInfoContainer_.GetIntraBlockSyncInfo(fftsInfo.flagId, blockIndex, vt)) {
            return ReturnType::PROCESS_OK;
        }
    }
    return ReturnType::PROCESS_STALLED;
}

// 核间软同步 (IB_SET / IB_WAIT / SYNC_ALL)，复用 CrossCoreSyncInfoContainer
ReturnType PipelineReplayer::ProcessBlockSoftSyncEvent(const SanEvent& event)
{
    uint32_t blockIndex = GetEventBlockIndex(event, kernelType_, deviceType_);
    uint32_t curPipe = eventContainer_.GetQueIndex();
    auto& softSyncInfo = event.eventInfo.softSyncInfo;
    VectorTime vt(curPipe + 1, 0); // SyncAll 内部会调用 UpdateLogicTime(vt, pipeIdx)，需保证 vt 足够大

    if (softSyncInfo.opType == SyncType::SYNC_ALL) {
        if (!crossCoreSyncInfoContainer_.SyncAll(blockIndex, softSyncInfo.usedCores, curPipe, vt)) {
            return ReturnType::PROCESS_STALLED;
        }
        return ReturnType::PROCESS_OK;
    } else if (softSyncInfo.opType == SyncType::IB_SET) {
        crossCoreSyncInfoContainer_.SetBlockSoftSyncInfo(softSyncInfo.eventID, blockIndex, vt);
        return ReturnType::PROCESS_OK;
    } else if (softSyncInfo.opType == SyncType::IB_WAIT) {
        if (crossCoreSyncInfoContainer_.GetBlockSoftSyncInfo(softSyncInfo.eventID,
                                                             softSyncInfo.waitCoreID, vt)) {
            return ReturnType::PROCESS_OK;
        }
    }
    return ReturnType::PROCESS_STALLED;
}

// MSTX 跨核同步 (MSTX_SET_CROSS / MSTX_WAIT_CROSS)， 复用 CrossCoreSyncInfoContainer + mstxSetCrossMap_
ReturnType PipelineReplayer::ProcessMstxCrossSyncEvent(const SanEvent& event)
{
    auto& mstxCrossInfo = event.eventInfo.mstxCrossInfo;
    VectorTime vt{};

    if (mstxCrossInfo.opType == SyncType::MSTX_SET_CROSS) {
        crossCoreSyncInfoContainer_.SetMstxCrossInfo(mstxCrossInfo, vt);
        return ReturnType::PROCESS_OK;
    } else if (mstxCrossInfo.opType == SyncType::MSTX_WAIT_CROSS) {
        // wait模式为多wait模式
        auto key = std::make_pair(mstxCrossInfo.addr, mstxCrossInfo.flagId);
        auto iter = mstxSetCrossMap_.find(key);
        if (mstxCrossInfo.isMore) {
            if (iter == mstxSetCrossMap_.end() || iter->second == 0) {
                // 如果当前wait找不到对应的set或者对应的set次数<=0，则跳过当前wait，处理下一条指令；
                return ReturnType::PROCESS_OK;
            }
        }
        if (crossCoreSyncInfoContainer_.GetMstxCrossInfo(mstxCrossInfo, vt)) {
            if (iter != mstxSetCrossMap_.end()) {
                // 匹配成功，set次数-1
                iter->second--;
            }
            return ReturnType::PROCESS_OK;
        }
    }
    return ReturnType::PROCESS_STALLED;
}

// MSTX 核间 Barrier，复用 BarrierDatabase
ReturnType PipelineReplayer::ProcessMstxCrossCoreBarrier(const SanEvent& event)
{
    auto& barrier = event.eventInfo.mstxCrossCoreBarrier;

    CrossNpuBarrierConf conf;
    conf.isAIVOnly = barrier.isAIVOnly;
    conf.usedDeviceNum = 1;
    conf.usedCoreNum = barrier.usedCoreNum;

    BarrierEvent<uint32_t>& barrierEvent = crossCoreBarrier_[conf];

    VectorTime vtSelf{};
    VectorTime vtGlobal;
    if (!barrierEvent.Wait(conf.usedCoreNum, event.loc.coreId, vtSelf, vtGlobal)) {
        return ReturnType::PROCESS_STALLED;
    }
    return ReturnType::PROCESS_OK;
}

// 卡间同步
ReturnType PipelineReplayer::ProcessMstxCrossNpuBarrier(const SanEvent& event)
{
    // 退化成核间同步（与原版逻辑一致）
    SanEvent decayEvent(event);
    decayEvent.type = EventType::MSTX_CROSS_CORE_BARRIER;
    MstxCrossCoreBarrier& crossCoreBarrier = decayEvent.eventInfo.mstxCrossCoreBarrier;
    MstxCrossNpuBarrier const &crossNpuBarrier = event.eventInfo.mstxCrossNpuBarrier;
    crossCoreBarrier.usedCoreId = crossNpuBarrier.usedCoreId;
    crossCoreBarrier.usedCoreNum = crossNpuBarrier.usedCoreNum;
    crossCoreBarrier.isAIVOnly = crossNpuBarrier.isAIVOnly;
    crossCoreBarrier.pipeBarrierAll = crossNpuBarrier.pipeBarrierAll;
    return ProcessMstxCrossCoreBarrier(decayEvent);
}

// BUF 同步 (GET_BUF / RLS_BUF)，复用 getRlsBufMap_ 的 .size() 计数能力
// 使用 getBufCount_ 追踪 get 消费次数来判断卡死
ReturnType PipelineReplayer::ProcessGetRlsBufSyncEvent(const SanEvent& event)
{
    uint32_t blockIndex = GetEventBlockIndex(event, kernelType_, deviceType_);
    const auto& bufSync = event.eventInfo.bufSyncInfo;
    auto bufKey = std::make_pair(blockIndex, bufSync.bufId);

    if (bufSync.opType == SyncType::GET_BUF && bufSync.mode == BufMode::BLOCK_MODE) {
        auto it = getRlsBufMap_.find(bufKey);
        uint64_t consumed = getBufCount_[bufKey];
        uint64_t available = (it == getRlsBufMap_.cend() ? 0 : it->second.size()) + 1;

        if (bufSync.rlsCount == 0U) {
            if (consumed + 1 > available) {
                return ReturnType::PROCESS_STALLED;
            }
            ++getBufCount_[bufKey];

            // 每个buf_id的第一个get_buf，不具备阻塞作用
            return ReturnType::PROCESS_OK;
        }
        if (it == getRlsBufMap_.cend() || it->second.size() < bufSync.rlsCount ||
            consumed + bufSync.rlsCount > available) {
            return ReturnType::PROCESS_STALLED;
        }
        getBufCount_[bufKey] += bufSync.rlsCount;
        return ReturnType::PROCESS_OK;
    } else if (bufSync.opType == SyncType::RLS_BUF) {
        VectorTime vt{};
        getRlsBufMap_[bufKey].push_back(vt);
    }
    return ReturnType::PROCESS_OK;
}

} // namespace Sanitizer
