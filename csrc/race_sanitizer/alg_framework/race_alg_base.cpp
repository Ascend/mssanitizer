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


#include "race_alg_base.h"

namespace Sanitizer {

RaceAlgBase::RaceAlgBase(KernelType kernelType, DeviceType deviceType, uint32_t blockDim)
    : kernelType_(kernelType), deviceType_(deviceType), blockDim_(blockDim) {}

ReturnType RaceAlgBase::ProcessBlockSyncEvent(const SanEvent &event, RaceCheckType checkType)
{
    uint32_t blockIndex = GetEventBlockIndex(event, kernelType_, deviceType_, checkType);
    uint32_t curPipe = eventContainer_.GetQueIndex();
    if (event.eventInfo.fftsSyncInfo.opType == SyncType::FFTS_SYNC) {
        VectorClock::UpdateLogicTime(vc_[curPipe], curPipe);
        crossCoreSyncInfoContainer_.SetBlockSyncInfo(event.eventInfo.fftsSyncInfo.flagId,
            static_cast<FftsSyncMode>(event.eventInfo.fftsSyncInfo.mode), blockIndex,
            vc_[curPipe], event.eventInfo.fftsSyncInfo.vecSubBlockDim);
        return ReturnType::PROCESS_OK;
    } else if (event.eventInfo.fftsSyncInfo.opType == SyncType::WAIT_FLAG_DEV) {
        if (crossCoreSyncInfoContainer_.GetBlockSyncInfo(event.eventInfo.fftsSyncInfo.flagId, blockIndex,
                                                         vc_[curPipe])) {
            return ReturnType::PROCESS_OK;
        }
    } else if (event.eventInfo.fftsSyncInfo.opType == SyncType::WAIT_INTRA_BLOCK) {
        if (crossCoreSyncInfoContainer_.GetIntraBlockSyncInfo(event.eventInfo.fftsSyncInfo.flagId, blockIndex,
                                                         vc_[curPipe])) {
            return ReturnType::PROCESS_OK;
        }
    }
    return ReturnType::PROCESS_STALLED;
}

void RaceAlgBase::CacheMstxCrossSet(const SanEvent& event)
{
    if (event.type == EventType::MSTX_CROSS_SYNC_EVENT &&
        event.eventInfo.mstxCrossInfo.opType == SyncType::MSTX_SET_CROSS) {
        /// 查找mstx中多核同步对应的set指令
        auto key = std::make_pair(event.eventInfo.mstxCrossInfo.addr, event.eventInfo.mstxCrossInfo.flagId);
        auto iter = mstxSetCrossMap_.find(key);
        if (iter != mstxSetCrossMap_.end()) {
            iter->second++;                     // 次数+1
        } else {
            mstxSetCrossMap_[key] = 1;          // 初始值设置为1
        }
    }
}

ReturnType RaceAlgBase::ProcessMstxCrossSyncEvent(const SanEvent& event)
{
    uint32_t curPipe = eventContainer_.GetQueIndex();
    auto &mstxCrossInfo = event.eventInfo.mstxCrossInfo;
    if (mstxCrossInfo.opType == SyncType::MSTX_SET_CROSS) {
        VectorClock::UpdateLogicTime(vc_[curPipe], curPipe);
        crossCoreSyncInfoContainer_.SetMstxCrossInfo(mstxCrossInfo, vc_[curPipe]);
        return ReturnType::PROCESS_OK;
    } else if (mstxCrossInfo.opType == SyncType::MSTX_WAIT_CROSS) {
        /// wait模式为多wait模式；
        auto key = std::make_pair(mstxCrossInfo.addr, mstxCrossInfo.flagId);
        auto iter = mstxSetCrossMap_.find(key);
        if (mstxCrossInfo.isMore) {
            if (iter == mstxSetCrossMap_.end() || iter->second == 0) {
                // 如果当前wait找不到对应的set或者对应的set次数<=0，则跳过当前wait，处理下一条指令；
                return ReturnType::PROCESS_OK;
            }
        }
        if (crossCoreSyncInfoContainer_.GetMstxCrossInfo(mstxCrossInfo, vc_[curPipe])) {
            if (iter != mstxSetCrossMap_.end()) {
                // 匹配成功，set次数-1
                iter->second--;
            }
            return ReturnType::PROCESS_OK;
        }
    }
    return ReturnType::PROCESS_STALLED;
}

ReturnType RaceAlgBase::ProcessGetRlsBufSyncEvent(const SanEvent& event, RaceCheckType checkType)
{
    uint32_t blockIndex = GetEventBlockIndex(event, kernelType_, deviceType_, checkType);
    uint32_t curPipe = eventContainer_.GetQueIndex();
    const auto &bufSync = event.eventInfo.bufSyncInfo;
    GetBufKey bufKey = {blockIndex, bufSync.bufId};
    if (bufSync.opType == SyncType::GET_BUF && bufSync.mode == BufMode::BLOCK_MODE) {
        auto it = getRlsBufMap_.find(bufKey);
        if (bufSync.rlsCount == 0U) {
            // 每个buf_id的第一个get_buf，不具备阻塞作用
            return ReturnType::PROCESS_OK;
        }
        if (it == getRlsBufMap_.cend() || it->second.size() < bufSync.rlsCount) {
            return ReturnType::PROCESS_STALLED;
        } else if (it->second.size() == bufSync.rlsCount) {
            // 每个buf_id对应的rls_buf全部执行完，统一更新向量时间
            for (const auto &vt : it->second) {
                VectorClock::UpdateVectorTime(vt, vc_[curPipe]);
            }
        } else {
            SAN_ERROR_LOG("The number of caches is incorrect, serialNo:%lu, rls cache count:%lu, rls count:%lu",
                event.serialNo, it->second.size(), bufSync.rlsCount);
        }
    } else if (bufSync.opType == SyncType::RLS_BUF) {
        getRlsBufMap_[bufKey].push_back(vc_[curPipe]);
    }
    return ReturnType::PROCESS_OK;
}

}

