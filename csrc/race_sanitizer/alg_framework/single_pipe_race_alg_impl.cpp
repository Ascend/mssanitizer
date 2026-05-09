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


#include "single_pipe_race_alg_impl.h"

namespace Sanitizer {
SinglePipeRaceAlgImpl::SinglePipeRaceAlgImpl(KernelType kernelType, DeviceType deviceType, uint32_t blockDim)
    : RaceAlgBase(kernelType, deviceType, blockDim)
{
    eventContainer_.Init(1U);
    memChecker_.Init(kernelType, deviceType, RaceCheckType::SINGLE_PIPE_CHECK);
}

void SinglePipeRaceAlgImpl::Do(const SanEvent& event)
{
    // 阶段一 除了PIPE_S，其他流水都有可能出现竞争行为，将事件推入对应PIPE
    if (event.type != EventType::SANITIZER_CONTROL_EVENT ||
        event.eventInfo.sanitizerControlInfo.type != SanitizerControlType::KERNEL_FINISH) {
        if (event.pipe > PipeType::PIPE_S && event.pipe < PipeType::SIZE) {
            eventContainer_.Push(event, event.pipe, 0);
        }
        return;
    }

    // 阶段二 模拟PIPE流水执行，进行竞争分析
    PipeLine pipeLine(eventContainer_);
    pipeLine.RegisterEventFunc(std::bind(&SinglePipeRaceAlgImpl::ProcessEvent, this, std::placeholders::_1));
    // 这一步耗时会较长，可以单独放一个线程去处理
    pipeLine.Run();
    memChecker_.RunAlgorithm();
    isFinished_ = true;
}

bool SinglePipeRaceAlgImpl::IsFinished() const
{
    return isFinished_;
}

std::shared_ptr<std::vector<RaceDispInfo>> SinglePipeRaceAlgImpl::GetResult() const
{
    return std::move(memChecker_.GetResult());
}

uint32_t SinglePipeRaceAlgImpl::GetRaceCount() const
{
    return memChecker_.GetRaceCount();
}

// 在PipeLine的Process()中被调用，处理不同类型event并推入memChecker_事件集，用于最后集中检测
ReturnType SinglePipeRaceAlgImpl::ProcessEvent(const SanEvent& event)
{
    switch (event.type) {
        case EventType::SYNC_EVENT:
            return ProcessSyncEvent(event);
        case EventType::MEM_EVENT:
            if(IsAscend95(this->deviceType_)) {
                // 硬件已经解决竞争问题，因此PIPE_V跳过处理
                if (event.pipe == PipeType::PIPE_V) {
                    return ReturnType::PROCESS_OK;
                }
            }
            return ProcessMemEvent(event);
        default:
            break;
    }
    return ReturnType::PROCESS_OK;
}

ReturnType SinglePipeRaceAlgImpl::ProcessMemEvent(const SanEvent& event)
{
    auto e = MemEvent(event);
    uint8_t pipeNum = static_cast<uint8_t>(event.pipe) - 1;  // PIPE_S除外
    e.barrierNo = pipeBarrierNo_[pipeNum];
    // 一条指令被拆成多个事件，所以连续事件的serialNo类似10,10,11,11，需要去重后再更新pipeSerialNo
    if (pipeSerialNo_[pipeNum][0] != event.serialNo) {
        pipeSerialNo_[pipeNum][0] = event.serialNo;
        pipeSerialNo_[pipeNum][1]++;
    }
    e.pipeSerialNo = pipeSerialNo_[pipeNum][1];
    memChecker_.PushEvent(e);

    return ReturnType::PROCESS_OK;
}

ReturnType SinglePipeRaceAlgImpl::ProcessSyncEvent(const SanEvent& event)
{
    // 该函数只处理pipe_barrier
    if (event.eventInfo.syncInfo.opType != SyncType::PIPE_BARRIER) {
        return ReturnType::PROCESS_OK;
    }
    pipeBarrierNo_[static_cast<uint8_t>(event.pipe) - 1]++;
    return ReturnType::PROCESS_OK;
}

}  // namespace Sanitizer
