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


#include "race_alg_impl.h"
namespace Sanitizer {

RaceAlgImpl::RaceAlgImpl(KernelType kernelType, DeviceType deviceType, uint32_t blockDim)
    : RaceAlgBase(kernelType, deviceType, blockDim)
{
    uint32_t totalBlockNum = NeedExpandBlockDim(kernelType, deviceType) ? blockDim * C220_MIX_SUB_BLOCKDIM : 1;
    vc_.resize(totalBlockNum * static_cast<uint8_t>(PipeType::SIZE));
    for (auto &it : vc_) {
        it.resize(totalBlockNum * static_cast<uint8_t>(PipeType::SIZE), 1);
    }
    eventContainer_.Init(totalBlockNum);
    crossCoreSyncInfoContainer_.Init(totalBlockNum, kernelType);
    memChecker_.Init(kernelType, deviceType, RaceCheckType::SINGLE_BLOCK_CHECK);
}

void RaceAlgImpl::Do(const SanEvent& event)
{
    // 阶段一 所有事件推入PIPE_S(标量流水)待执行
    if (event.type != EventType::SANITIZER_CONTROL_EVENT ||
        event.eventInfo.sanitizerControlInfo.type != SanitizerControlType::KERNEL_FINISH) {
        CacheMstxCrossSet(event);
        auto blockIndex = GetEventBlockIndex(event, kernelType_, deviceType_, RaceCheckType::SINGLE_BLOCK_CHECK);
        eventContainer_.Push(event, PipeType::PIPE_S, blockIndex);
        return;
    }

    // 阶段二 模拟PIPE流水执行，进行竞争分析
    PipeLine pipeLine(eventContainer_);
    pipeLine.RegisterEventFunc(std::bind(&RaceAlgImpl::ProcessEvent, this, std::placeholders::_1));
    // 这一步耗时会较长，可以单独放一个线程去处理
    pipeLine.Run();
    memChecker_.RunAlgorithm();
    isFinished_ = true;
}

bool RaceAlgImpl::IsFinished() const
{
    return isFinished_;
}

std::shared_ptr<std::vector<RaceDispInfo>> RaceAlgImpl::GetResult() const
{
    return memChecker_.GetResult();
}

uint32_t RaceAlgImpl::GetRaceCount() const
{
    return memChecker_.GetRaceCount();
}

ReturnType RaceAlgImpl::ProcessEvent(const SanEvent& event)
{
    PipeType curPipe = eventContainer_.GetPipeIndex();
    if (curPipe != event.pipe) {
        // 当事件从PIPE_S(标量流水)发射到目标PIPE上去执行时，由于发射的动作隐含了"先后"关系
        // 所以需要更新向量时间，并用更新后的向量时间包装一个"时间事件"，插入到目标PIPE中, 让PIPE_S和目标PIPE建立同步关系
        uint32_t curPipeIdx = eventContainer_.GetQueIndex();
        VectorClock::UpdateLogicTime(vc_[curPipeIdx], curPipeIdx);
        SanEvent e;
        e.type = EventType::TIME_EVENT;
        e.pipe = event.pipe;
        e.timeInfo = vc_[curPipeIdx];
        auto blockIndex = GetEventBlockIndex(event, kernelType_, deviceType_, RaceCheckType::SINGLE_BLOCK_CHECK);
        eventContainer_.Push(e, e.pipe, blockIndex);
        eventContainer_.Push(event, event.pipe, blockIndex);
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
            return ProcessBlockSyncEvent(event, RaceCheckType::SINGLE_BLOCK_CHECK);
        case EventType::MSTX_CROSS_SYNC_EVENT:
            return ProcessMstxCrossSyncEvent(event);
        default:
            break;
    }
    return ReturnType::PROCESS_OK;
}

ReturnType RaceAlgImpl::ProcessMemEvent(const SanEvent& event)
{
    auto e = MemEvent(event);
    // 核内原子相关的标志位不处理，会在识别pipe是否相同时被拦截
    uint32_t curPipe = eventContainer_.GetQueIndex();
    VectorClock::UpdateLogicTime(vc_[curPipe], curPipe);
    e.vt = vc_[curPipe];
    memChecker_.PushEvent(e);

    return ReturnType::PROCESS_OK;
}

ReturnType RaceAlgImpl::ProcessSyncEvent(const SanEvent& event)
{
    auto e = SyncEvent {};
    e.info.srcPipe = static_cast<uint8_t>(event.eventInfo.syncInfo.srcPipe);
    e.info.dstPipe = static_cast<uint8_t>(event.eventInfo.syncInfo.dstPipe);
    e.info.eventId = static_cast<uint8_t>(event.eventInfo.syncInfo.eventId);
    e.info.memType = static_cast<uint8_t>(event.eventInfo.syncInfo.memType);
    e.info.isRetrogress = event.eventInfo.syncInfo.isRetrogress;

    // set事件，更新向量时间，设置同步标志
    if (event.eventInfo.syncInfo.opType == SyncType::SET_FLAG) {
        uint32_t curPipe = eventContainer_.GetQueIndex();
        VectorClock::UpdateLogicTime(vc_[curPipe], curPipe);
        syncDB_.Set(e, vc_[curPipe]);
        return ReturnType::PROCESS_OK;
    }

    // wait事件，查询同步标志，更新向量时钟或切换pipe
    if (event.eventInfo.syncInfo.opType == SyncType::WAIT_FLAG) {
        VectorTime vt;
        if (syncDB_.Get(e, vt)) {
            uint32_t curPipe = eventContainer_.GetQueIndex();
            VectorClock::UpdateVectorTime(vt, vc_[curPipe]);
            VectorClock::UpdateLogicTime(vc_[curPipe], curPipe);
            return ReturnType::PROCESS_OK;
        } else {
            return ReturnType::PROCESS_STALLED;
        }
    }

    return ReturnType::PROCESS_OK;
}

ReturnType RaceAlgImpl::ProcessTimeEvent(const SanEvent& event)
{
    uint32_t curPipe = eventContainer_.GetQueIndex();
    VectorClock::UpdateVectorTime(event.timeInfo, vc_[curPipe]);
    VectorClock::UpdateLogicTime(vc_[curPipe], curPipe);
    return ReturnType::PROCESS_OK;
}

}  // namespace Sanitizer
