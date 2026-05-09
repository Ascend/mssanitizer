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


#include "cross_core_race_alg_impl.h"

namespace Sanitizer {
CrossCoreRaceAlgImpl::CrossCoreRaceAlgImpl(KernelType kernelType, DeviceType deviceType, uint32_t blockDim)
    : RaceAlgBase(kernelType, deviceType, blockDim)
{
    uint32_t totalBlockNum = NeedExpandBlockDim(kernelType, deviceType) ? blockDim * C220_MIX_SUB_BLOCKDIM : blockDim;
    vc_.resize(totalBlockNum * static_cast<uint8_t>(PipeType::SIZE));
    for (auto &it : vc_) {
        it.resize(totalBlockNum * static_cast<uint8_t>(PipeType::SIZE), 1);
    }
    eventContainer_.Init(totalBlockNum);
    syncDB_.resize(totalBlockNum);
    crossCoreSyncInfoContainer_.Init(totalBlockNum, kernelType);
    memChecker_.Init(kernelType, deviceType, RaceCheckType::CROSS_BLOCK_CHECK);
}

void CrossCoreRaceAlgImpl::Do(const SanEvent &event)
{
    if (event.type != EventType::SANITIZER_CONTROL_EVENT ||
        event.eventInfo.sanitizerControlInfo.type != SanitizerControlType::KERNEL_FINISH) {
        CacheMstxCrossSet(event);
        auto blockIndex = GetEventBlockIndex(event, kernelType_, deviceType_, RaceCheckType::CROSS_BLOCK_CHECK);
        eventContainer_.Push(event, PipeType::PIPE_S, blockIndex);
        return;
    }
    PipeLine pipeLine(eventContainer_);
    pipeLine.RegisterEventFunc(std::bind(&CrossCoreRaceAlgImpl::ProcessEvent, this, std::placeholders::_1));
    pipeLine.Run();
    memChecker_.RunAlgorithm();
    isFinished_ = true;
}

bool CrossCoreRaceAlgImpl::IsFinished() const
{
    return isFinished_;
}

std::shared_ptr<std::vector<RaceDispInfo>> CrossCoreRaceAlgImpl::GetResult() const
{
    return memChecker_.GetResult();
}

ReturnType CrossCoreRaceAlgImpl::ProcessEvent(const SanEvent &event)
{
    uint32_t curPipe = eventContainer_.GetQueIndex();
    if (eventContainer_.GetPipeIndex() != event.pipe) {
        // 当事件从PIPE_S(标量流水)发射到目标PIPE上去执行时，由于发射的动作隐含了"先后"关系
        // 所以需要更新向量时间，并用更新后的向量时间包装一个"时间事件"，插入到目标PIPE中,
        // 让PIPE_S和目标PIPE建立同步关系
        VectorClock::UpdateLogicTime(vc_[curPipe], curPipe);
        SanEvent e;
        e.serialNo = event.serialNo;
        e.loc.coreId = event.loc.coreId;
        e.type = EventType::TIME_EVENT;
        e.pipe = event.pipe;
        e.loc.blockType = event.loc.blockType;
        e.timeInfo = vc_[curPipe];
        auto blockIdx = GetEventBlockIndex(event, kernelType_, deviceType_, RaceCheckType::CROSS_BLOCK_CHECK);
        eventContainer_.Push(e, e.pipe, blockIdx);
        eventContainer_.Push(event, event.pipe, blockIdx);
        return ReturnType::PROCESS_OK;
    }
    switch (event.type) {
        case EventType::SYNC_EVENT:
            return ProcessSyncEvent(event);
        case EventType::MEM_EVENT:
            return ProcessMemEvent(event);
        case EventType::TIME_EVENT:
            return ProcessTimeEvent(event);
        case EventType::CROSS_CORE_SYNC_EVENT:
            return ProcessBlockSyncEvent(event, RaceCheckType::CROSS_BLOCK_CHECK);
        case EventType::CROSS_CORE_SOFT_SYNC_EVENT:
            return ProcessBlockSoftSyncEvent(event);
        case EventType::MSTX_CROSS_SYNC_EVENT:
            return ProcessMstxCrossSyncEvent(event);
        case EventType::MSTX_CROSS_CORE_BARRIER:
            return ProcessMstxCrossCoreBarrier(event);
        case EventType::MSTX_CROSS_NPU_BARRIER:
            return ProcessMstxCrossNpuBarrier(event);
        default:
            break;
    }
    return ReturnType::PROCESS_OK;
}

ReturnType CrossCoreRaceAlgImpl::ProcessBlockSoftSyncEvent(const SanEvent& event)
{
    uint32_t blockIndex = GetEventBlockIndex(event, kernelType_, deviceType_, RaceCheckType::CROSS_BLOCK_CHECK);
    uint32_t curPipe = eventContainer_.GetQueIndex();
    auto softSyncInfo = event.eventInfo.softSyncInfo;
    if (softSyncInfo.opType == SyncType::SYNC_ALL) {
        if (!crossCoreSyncInfoContainer_.SyncAll(blockIndex, softSyncInfo.usedCores, curPipe, vc_[curPipe])) {
            return ReturnType::PROCESS_STALLED;
        }
        crossCoreSyncInfoContainer_.UpdateSyncAllVectorTime(vc_);
        return ReturnType::PROCESS_OK;
    } else if (softSyncInfo.opType == SyncType::IB_SET) {
        VectorClock::UpdateLogicTime(vc_[curPipe], curPipe);
        crossCoreSyncInfoContainer_.SetBlockSoftSyncInfo(softSyncInfo.eventID, blockIndex, vc_[curPipe]);
        return ReturnType::PROCESS_OK;
    } else if (softSyncInfo.opType == SyncType::IB_WAIT) {
        if (crossCoreSyncInfoContainer_.GetBlockSoftSyncInfo(softSyncInfo.eventID,
            softSyncInfo.waitCoreID, vc_[curPipe])) {
            return ReturnType::PROCESS_OK;
        }
    }
    return ReturnType::PROCESS_STALLED;
}

ReturnType CrossCoreRaceAlgImpl::ProcessMemEvent(const SanEvent &event)
{
    if (event.eventInfo.memInfo.memType != MemType::GM) {
        // 非GM内存事件不检测
        return ReturnType::PROCESS_OK;
    }
    auto e = MemEvent(event);
    e.isAtomicMode = event.isAtomicMode;
    uint32_t curPipe = eventContainer_.GetQueIndex();
    VectorClock::UpdateLogicTime(vc_[curPipe], curPipe);
    e.vt = vc_[curPipe];
    memChecker_.PushEvent(e);

    return ReturnType::PROCESS_OK;
}

ReturnType CrossCoreRaceAlgImpl::ProcessSyncEvent(const SanEvent &event)
{
    auto e = SyncEvent{};
    e.info.srcPipe = static_cast<uint8_t>(event.eventInfo.syncInfo.srcPipe);
    e.info.dstPipe = static_cast<uint8_t>(event.eventInfo.syncInfo.dstPipe);
    e.info.eventId = static_cast<uint8_t>(event.eventInfo.syncInfo.eventId);
    e.info.memType = static_cast<uint8_t>(event.eventInfo.syncInfo.memType);
    e.info.isRetrogress = event.eventInfo.syncInfo.isRetrogress;
    uint32_t blockIdx = GetEventBlockIndex(event, kernelType_, deviceType_, RaceCheckType::CROSS_BLOCK_CHECK);
    uint32_t dstPipe = blockIdx * static_cast<uint32_t>(PipeType::SIZE) + static_cast<uint32_t>(event.pipe);
    // set事件，更新向量时间，设置同步标志
    if (event.eventInfo.syncInfo.opType == SyncType::SET_FLAG) {
        VectorClock::UpdateLogicTime(vc_[dstPipe], dstPipe);
        syncDB_[blockIdx].Set(e, vc_[dstPipe]);
        return ReturnType::PROCESS_OK;
    }
    // wait事件，查询同步标志，更新向量时钟或切换pipe
    if (event.eventInfo.syncInfo.opType == SyncType::WAIT_FLAG) {
        VectorTime vt;
        if (syncDB_[blockIdx].Get(e, vt)) {
            VectorClock::UpdateVectorTime(vt, vc_[dstPipe]);
            VectorClock::UpdateLogicTime(vc_[dstPipe], dstPipe);
            return ReturnType::PROCESS_OK;
        } else {
            return ReturnType::PROCESS_STALLED;
        }
    }
    return ReturnType::PROCESS_OK;
}

ReturnType CrossCoreRaceAlgImpl::ProcessMstxCrossCoreBarrier(const SanEvent& event)
{
    uint32_t curPipe = eventContainer_.GetQueIndex();
    MstxCrossCoreBarrier const &mstxCrossCoreBarrier = event.eventInfo.mstxCrossCoreBarrier;

    CrossNpuBarrierConf conf;
    conf.isAIVOnly = mstxCrossCoreBarrier.isAIVOnly;
    conf.usedDeviceNum = 1;
    conf.usedCoreNum = mstxCrossCoreBarrier.usedCoreNum;
    BarrierEvent<CrossCoreBarrierWorker> &barrierEvent = crossCoreBarrier_[conf];

    VectorTime vt;
    if (!barrierEvent.Wait(conf.usedCoreNum, event.loc.coreId, vc_[curPipe], vt)) {
        return ReturnType::PROCESS_STALLED;
    }
    VectorClock::UpdateVectorTime(vt, vc_[curPipe]);
    VectorClock::UpdateLogicTime(vc_[curPipe], curPipe);
    return ReturnType::PROCESS_OK;
}

ReturnType CrossCoreRaceAlgImpl::ProcessMstxCrossNpuBarrier(const SanEvent& event)
{
    // 退化成核间同步
    SanEvent decayEvent(event);
    decayEvent.type = EventType::MSTX_CROSS_CORE_BARRIER;
    MstxCrossCoreBarrier &crossCoreBarrier = decayEvent.eventInfo.mstxCrossCoreBarrier;
    MstxCrossNpuBarrier const &crossNpuBarrier = event.eventInfo.mstxCrossNpuBarrier;
    crossCoreBarrier.usedCoreId = crossNpuBarrier.usedCoreId;
    crossCoreBarrier.usedCoreNum = crossNpuBarrier.usedCoreNum;
    crossCoreBarrier.isAIVOnly = crossNpuBarrier.isAIVOnly;
    crossCoreBarrier.pipeBarrierAll = crossNpuBarrier.pipeBarrierAll;
    return ProcessMstxCrossCoreBarrier(decayEvent);
}

ReturnType CrossCoreRaceAlgImpl::ProcessTimeEvent(const SanEvent &event)
{
    uint32_t curPipe = eventContainer_.GetQueIndex();
    VectorClock::UpdateVectorTime(event.timeInfo, vc_[curPipe]);
    VectorClock::UpdateLogicTime(vc_[curPipe], curPipe);
    return ReturnType::PROCESS_OK;
}

uint32_t CrossCoreRaceAlgImpl::GetRaceCount() const
{
    return memChecker_.GetRaceCount();
}

}  // namespace Sanitizer
