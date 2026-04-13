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

#include "core/framework/device_manager.h"
#include "core/framework/event_def.h"
#include "core/framework/kernel_manager.h"
#include "core/framework/record_defs.h"
#include "race_sanitizer/alg_framework/cross_core_sync_info_container.h"
#include "race_sanitizer/alg_framework/event_container.h"
#include "race_sanitizer/alg_framework/mem_event_checker.h"

#include "cross_npu_race_alg_impl.h"

namespace Sanitizer {

CrossNpuRaceAlgImpl::CrossNpuRaceAlgImpl()
{
}

void CrossNpuRaceAlgImpl::Init()
{
    uint32_t maxBlockDim = 1;
    auto const &deviceList = DeviceManager::Instance().GetDeviceList();
    std::size_t deviceNum = DeviceManager::Instance().GetDeviceCount();
    crossCoreSyncInfoContainer_.resize(deviceNum);
    mstxSetCrossMap_.resize(deviceNum);

    for (std::size_t deviceIdx = 0; deviceIdx < deviceNum; ++deviceIdx) {
        uint32_t deviceId = deviceList[deviceIdx];
        DeviceInfoSummary deviceInfo{};
        std::size_t kernelCount;
        DeviceManager::Instance().Get(deviceId, deviceInfo);
        KernelManager::Instance().GetKernelCount(deviceId, kernelCount);

        crossCoreSyncInfoContainer_[deviceIdx].resize(kernelCount);
        mstxSetCrossMap_[deviceIdx].resize(kernelCount);

        for (std::size_t kernelIdx = 0; kernelIdx < kernelCount; ++kernelIdx) {
            KernelSummary kernelSummary{};
            KernelManager::Instance().Get(deviceId, kernelIdx, kernelSummary);

            maxBlockDim = std::max(maxBlockDim, kernelSummary.blockDim);
            uint32_t totalBlockNum = NeedExpandBlockDim(kernelSummary.kernelType, deviceInfo.device) ?
                kernelSummary.blockDim * C220_MIX_SUB_BLOCKDIM : kernelSummary.blockDim;
            crossCoreSyncInfoContainer_[deviceIdx][kernelIdx].Init(totalBlockNum, kernelSummary.kernelType);
        }
    }

    // 向量时钟、eventContainer、核内同步数据库都用最大的 blockdim 进行初始化
    totalBlockNum_ = maxBlockDim * C220_MIX_SUB_BLOCKDIM;
    vc_.resize(deviceNum * totalBlockNum_ * static_cast<uint8_t>(PipeType::SIZE));
    for (auto &it : vc_) {
        it.resize(deviceNum * totalBlockNum_ * static_cast<uint8_t>(PipeType::SIZE), 1);
    }
    eventContainer_.Init(totalBlockNum_, deviceNum);
    syncDB_.resize(deviceNum * totalBlockNum_);
    memChecker_.Init(totalBlockNum_);
}

bool CrossNpuRaceAlgImpl::SetDeviceInfo(DeviceInfoSummary const &deviceInfo, Config const &config)
{
    (void)deviceInfo;
    (void)config;
    return true;
}

bool CrossNpuRaceAlgImpl::SetKernelInfo(KernelSummary const &kernelInfo)
{
    (void)kernelInfo;
    return true;
}

void CrossNpuRaceAlgImpl::Do(const SanEvent& event)
{
    if (!event.isEndFrame) {
        CacheMstxCrossSet(event);
        auto blockIndex = GetEventExpandBlockIndex(event);
        eventContainer_.Push(event, PipeType::PIPE_S, blockIndex, event.loc.deviceIdx);
        return;
    }

    PipeLine pipeLine(eventContainer_);
    pipeLine.RegisterEventFunc(std::bind(&CrossNpuRaceAlgImpl::ProcessEvent, this, std::placeholders::_1));
    pipeLine.Run();
    memChecker_.RunAlgorithm();
    isFinished_ = true;
}

bool CrossNpuRaceAlgImpl::IsFinished() const
{
    return isFinished_;
}

std::shared_ptr<std::vector<RaceDispInfo>> CrossNpuRaceAlgImpl::GetResult() const
{
    return memChecker_.GetResult();
}

ReturnType CrossNpuRaceAlgImpl::ProcessEvent(const SanEvent& event)
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
        auto blockIdx = GetEventExpandBlockIndex(event);
        eventContainer_.Push(e, e.pipe, blockIdx, event.loc.deviceIdx);
        eventContainer_.Push(event, event.pipe, blockIdx, event.loc.deviceIdx);
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
            return ProcessBlockSyncEvent(event);
        case EventType::CROSS_CORE_SOFT_SYNC_EVENT:
            return ProcessBlockSoftSyncEvent(event);
        case EventType::MSTX_CROSS_SYNC_EVENT:
            return ProcessMstxCrossSyncEvent(event);
        case EventType::MSTX_SIGNAL_SET_EVENT:
            return ProcessMstxSignalSetEvent(event);
        case EventType::MSTX_SIGNAL_WAIT_EVENT:
            return ProcessMstxSignalWaitEvent(event);
        default:
            break;
    }
    return ReturnType::PROCESS_OK;
}

ReturnType CrossNpuRaceAlgImpl::ProcessMemEvent(const SanEvent& event)
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

ReturnType CrossNpuRaceAlgImpl::ProcessSyncEvent(const SanEvent& event)
{
    auto e = SyncEvent{};
    e.info.srcPipe = static_cast<uint8_t>(event.eventInfo.syncInfo.srcPipe);
    e.info.dstPipe = static_cast<uint8_t>(event.eventInfo.syncInfo.dstPipe);
    e.info.eventId = static_cast<uint8_t>(event.eventInfo.syncInfo.eventId);
    e.info.memType = static_cast<uint8_t>(event.eventInfo.syncInfo.memType);
    e.info.isRetrogress = event.eventInfo.syncInfo.isRetrogress;
    uint32_t blockIdx = GetEventExpandBlockIndex(event);
    uint32_t dstPipe = eventContainer_.FlattenPipeIdx(static_cast<uint32_t>(event.pipe), blockIdx, event.loc.deviceIdx);
    // set事件，更新向量时间，设置同步标志
    if (event.eventInfo.syncInfo.opType == SyncType::SET_FLAG) {
        VectorClock::UpdateLogicTime(vc_[dstPipe], dstPipe);
        syncDB_[event.loc.deviceIdx * totalBlockNum_ + blockIdx].Set(e, vc_[dstPipe]);
        return ReturnType::PROCESS_OK;
    }
    // wait事件，查询同步标志，更新向量时钟或切换pipe
    if (event.eventInfo.syncInfo.opType == SyncType::WAIT_FLAG) {
        VectorTime vt;
        if (syncDB_[event.loc.deviceIdx * totalBlockNum_ + blockIdx].Get(e, vt)) {
            VectorClock::UpdateVectorTime(vt, vc_[dstPipe]);
            VectorClock::UpdateLogicTime(vc_[dstPipe], dstPipe);
            return ReturnType::PROCESS_OK;
        } else {
            return ReturnType::PROCESS_STALLED;
        }
    }
    return ReturnType::PROCESS_OK;
}

ReturnType CrossNpuRaceAlgImpl::ProcessTimeEvent(const SanEvent& event)
{
    uint32_t curPipe = eventContainer_.GetQueIndex();
    VectorClock::UpdateVectorTime(event.timeInfo, vc_[curPipe]);
    VectorClock::UpdateLogicTime(vc_[curPipe], curPipe);
    return ReturnType::PROCESS_OK;
}

ReturnType CrossNpuRaceAlgImpl::ProcessBlockSoftSyncEvent(const SanEvent& event)
{
    DeviceInfoSummary deviceInfo{};
    KernelSummary kernelSummary{};
    if (!DeviceManager::Instance().Get(event.loc.deviceId, deviceInfo)) {
        SAN_ERROR_LOG("Get device info from device manager failed. deviceId: %u", event.loc.deviceId);
        return ReturnType::PROCESS_OK;
    }
    if (!KernelManager::Instance().Get(event.loc.deviceId, event.loc.kernelIdx, kernelSummary)) {
        SAN_ERROR_LOG("Get kernel summary from kernel manager failed. deviceId: %u, kernelIdx: %u",
                      event.loc.deviceId, event.loc.kernelIdx);
        return ReturnType::PROCESS_OK;
    }

    uint32_t blockIndex = GetEventBlockIndex(event, kernelSummary.kernelType, deviceInfo.device,
                                             RaceCheckType::CROSS_NPU_CHECK);
    uint32_t curPipe = eventContainer_.GetQueIndex();
    auto softSyncInfo = event.eventInfo.softSyncInfo;
    auto &crossCoreSyncInfoContainer = crossCoreSyncInfoContainer_[event.loc.deviceIdx][event.loc.kernelIdx];
    if (softSyncInfo.opType == SyncType::SYNC_ALL) {
        if (!crossCoreSyncInfoContainer.SyncAll(blockIndex, softSyncInfo.usedCores, curPipe, vc_[curPipe])) {
            return ReturnType::PROCESS_STALLED;
        }
        crossCoreSyncInfoContainer.UpdateSyncAllVectorTime(vc_);
        return ReturnType::PROCESS_OK;
    } else if (softSyncInfo.opType == SyncType::IB_SET) {
        VectorClock::UpdateLogicTime(vc_[curPipe], curPipe);
        crossCoreSyncInfoContainer.SetBlockSoftSyncInfo(softSyncInfo.eventID, blockIndex, vc_[curPipe]);
        return ReturnType::PROCESS_OK;
    } else if (softSyncInfo.opType == SyncType::IB_WAIT) {
        if (crossCoreSyncInfoContainer.GetBlockSoftSyncInfo(softSyncInfo.eventID,
            softSyncInfo.waitCoreID, vc_[curPipe])) {
            return ReturnType::PROCESS_OK;
        }
    }
    return ReturnType::PROCESS_STALLED;
}

ReturnType CrossNpuRaceAlgImpl::ProcessBlockSyncEvent(const SanEvent& event)
{
    DeviceInfoSummary deviceInfo{};
    KernelSummary kernelSummary{};
    if (!DeviceManager::Instance().Get(event.loc.deviceId, deviceInfo)) {
        SAN_ERROR_LOG("Get device info from device manager failed. deviceId: %u", event.loc.deviceId);
        return ReturnType::PROCESS_OK;
    }
    if (!KernelManager::Instance().Get(event.loc.deviceId, event.loc.kernelIdx, kernelSummary)) {
        SAN_ERROR_LOG("Get kernel summary from kernel manager failed. deviceId: %u, kernelIdx: %u",
                      event.loc.deviceId, event.loc.kernelIdx);
        return ReturnType::PROCESS_OK;
    }

    uint32_t blockIndex = GetEventBlockIndex(event, kernelSummary.kernelType, deviceInfo.device,
                                             RaceCheckType::CROSS_NPU_CHECK);
    uint32_t curPipe = eventContainer_.GetQueIndex();
    auto &crossCoreSyncInfoContainer = crossCoreSyncInfoContainer_[event.loc.deviceIdx][event.loc.kernelIdx];
    if (event.eventInfo.fftsSyncInfo.opType == SyncType::FFTS_SYNC) {
        VectorClock::UpdateLogicTime(vc_[curPipe], curPipe);
        crossCoreSyncInfoContainer.SetBlockSyncInfo(event.eventInfo.fftsSyncInfo.flagId,
            static_cast<FftsSyncMode>(event.eventInfo.fftsSyncInfo.mode), blockIndex,
            vc_[curPipe], event.eventInfo.fftsSyncInfo.vecSubBlockDim);
        return ReturnType::PROCESS_OK;
    } else if (event.eventInfo.fftsSyncInfo.opType == SyncType::WAIT_FLAG_DEV) {
        if (crossCoreSyncInfoContainer.GetBlockSyncInfo(event.eventInfo.fftsSyncInfo.flagId,
                                                        blockIndex, vc_[curPipe])) {
            return ReturnType::PROCESS_OK;
        }
    } else if (event.eventInfo.fftsSyncInfo.opType == SyncType::WAIT_INTRA_BLOCK) {
        if (crossCoreSyncInfoContainer.GetIntraBlockSyncInfo(event.eventInfo.fftsSyncInfo.flagId,
                                                             blockIndex, vc_[curPipe])) {
            return ReturnType::PROCESS_OK;
        }
    }
    return ReturnType::PROCESS_STALLED;
}

ReturnType CrossNpuRaceAlgImpl::ProcessMstxCrossSyncEvent(const SanEvent& event)
{
    uint32_t curPipe = eventContainer_.GetQueIndex();
    auto &mstxCrossInfo = event.eventInfo.mstxCrossInfo;
    auto &crossCoreSyncInfoContainer = crossCoreSyncInfoContainer_[event.loc.deviceIdx][event.loc.kernelIdx];
    auto &mstxSetCrossMap = mstxSetCrossMap_[event.loc.deviceIdx][event.loc.kernelIdx];
    if (mstxCrossInfo.opType == SyncType::MSTX_SET_CROSS) {
        VectorClock::UpdateLogicTime(vc_[curPipe], curPipe);
        crossCoreSyncInfoContainer.SetMstxCrossInfo(mstxCrossInfo, vc_[curPipe]);
        return ReturnType::PROCESS_OK;
    } else if (mstxCrossInfo.opType == SyncType::MSTX_WAIT_CROSS) {
        /// wait模式为多wait模式；
        auto key = std::make_pair(mstxCrossInfo.addr, mstxCrossInfo.flagId);
        auto iter = mstxSetCrossMap.find(key);
        if (mstxCrossInfo.isMore) {
            if (iter == mstxSetCrossMap.end() || iter->second == 0) {
                // 如果当前wait找不到对应的set或者对应的set次数<=0，则跳过当前wait，处理下一条指令；
                return ReturnType::PROCESS_OK;
            }
        }
        if (crossCoreSyncInfoContainer.GetMstxCrossInfo(mstxCrossInfo, vc_[curPipe])) {
            if (iter != mstxSetCrossMap.end()) {
                // 匹配成功，set次数-1
                iter->second--;
            }
            return ReturnType::PROCESS_OK;
        }
    }
    return ReturnType::PROCESS_STALLED;
}

ReturnType CrossNpuRaceAlgImpl::ProcessMstxSignalSetEvent(const SanEvent &event)
{
    uint32_t curPipe = eventContainer_.GetQueIndex();
    signalDatabase_.Set(event.eventInfo.mstxSignalSet, vc_[curPipe]);
    return ReturnType::PROCESS_OK;
}

ReturnType CrossNpuRaceAlgImpl::ProcessMstxSignalWaitEvent(const SanEvent &event)
{
    uint32_t curPipe = eventContainer_.GetQueIndex();
    VectorTime vt;
    if (signalDatabase_.Wait(event.eventInfo.mstxSignalWait, vt)) {
        VectorClock::UpdateVectorTime(vt, vc_[curPipe]);
        VectorClock::UpdateLogicTime(vc_[curPipe], curPipe);
        return ReturnType::PROCESS_OK;
    }
    return ReturnType::PROCESS_STALLED;
}

void CrossNpuRaceAlgImpl::CacheMstxCrossSet(const SanEvent& event)
{
    auto &mstxSetCrossMap = mstxSetCrossMap_[event.loc.deviceIdx][event.loc.kernelIdx];
    if (event.type == EventType::MSTX_CROSS_SYNC_EVENT &&
        event.eventInfo.mstxCrossInfo.opType == SyncType::MSTX_SET_CROSS) {
        /// 查找mstx中多核同步对应的set指令
        auto key = std::make_pair(event.eventInfo.mstxCrossInfo.addr, event.eventInfo.mstxCrossInfo.flagId);
        auto iter = mstxSetCrossMap.find(key);
        if (iter != mstxSetCrossMap.end()) {
            iter->second++;                     // 次数+1
        } else {
            mstxSetCrossMap[key] = 1;          // 初始值设置为1
        }
    }
}

} // namespace Sanitizer