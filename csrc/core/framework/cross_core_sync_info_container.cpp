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

#include "cross_core_sync_info_container.h"
#include "core/framework/vector_clock.h"
#include "core/framework/utility/log.h"

namespace Sanitizer {
void CrossCoreSyncInfoContainer::Init(uint32_t blockNum, KernelType kernelType)
{
    maxBlockNum_ = blockNum;
    kernelType_ = kernelType;
    blockSyncEvent_.resize(blockNum);
    blockSoftSyncInfo_.resize(blockNum);
}

bool IsAIC(uint32_t blockIndex, KernelType kernelType)
{
    if (kernelType == KernelType::AICUBE) {
        return true;
    } else if (kernelType == KernelType::MIX) {
        return blockIndex % C220_MIX_SUB_BLOCKDIM == C220_VEC_SUB_BLOCKDIM;
    }
    return false;
}

uint32_t CrossCoreSyncInfoContainer::GetAIVCount()
{
    if (kernelType_ == KernelType::MIX) {
        return maxBlockNum_ / C220_MIX_SUB_BLOCKDIM * C220_VEC_SUB_BLOCKDIM;
    }
    return maxBlockNum_;
}

uint32_t CrossCoreSyncInfoContainer::GetAICCount()
{
    if (kernelType_ == KernelType::MIX) {
        return maxBlockNum_ / C220_MIX_SUB_BLOCKDIM;
    }
    return maxBlockNum_;
}

template <BlockType blockType>
void CrossCoreSyncInfoContainer::UpdateSyncInfoInMode0(uint8_t flagId)
{
    VectorTime vt;
    vt.resize(maxBlockNum_ * static_cast<uint8_t>(PipeType::SIZE), 1);
    if (blockType == BlockType::AIVEC) {
        // 先获取最新向量时钟，再更新到所有block
        for (uint32_t i = 0; i < maxBlockNum_; i++) {
            if (!IsAIC(i, kernelType_) && !blockSyncEvent_[i][flagId].setVec0.empty()) {
                VectorClock::UpdateVectorTime(blockSyncEvent_[i][flagId].setVec0.front(), vt);
                blockSyncEvent_[i][flagId].setVec0.pop();
            }
        }
        for (uint32_t i = 0; i < maxBlockNum_; i++) {
            if (!IsAIC(i, kernelType_)) {
                blockSyncEvent_[i][flagId].waitVec.push(vt);
            }
        }
    } else {
        for (uint32_t block = C220_VEC_SUB_BLOCKDIM; block < maxBlockNum_; block += C220_MIX_SUB_BLOCKDIM) {
            if (!blockSyncEvent_[block][flagId].setVec0.empty()) {
                VectorClock::UpdateVectorTime(blockSyncEvent_[block][flagId].setVec0.front(), vt);
                blockSyncEvent_[block][flagId].setVec0.pop();
            }
        }
        for (uint32_t block = C220_VEC_SUB_BLOCKDIM; block < maxBlockNum_; block += C220_MIX_SUB_BLOCKDIM) {
            blockSyncEvent_[block][flagId].waitVec.push(vt);
        }
    }
}
// mode0:AIV->AIV或AIC->AIC
void CrossCoreSyncInfoContainer::SetMode0SyncInfo(uint8_t flagId, uint32_t blockIdx, const VectorTime &vectorTime)
{
    blockSyncEvent_[blockIdx][flagId].setVec0.push(vectorTime);
    uint32_t setCount = 0;
    // AIC
    if (IsAIC(blockIdx, kernelType_)) {
        for (uint32_t i = 2; i < maxBlockNum_; i += C220_MIX_SUB_BLOCKDIM) {
            setCount += blockSyncEvent_[i][flagId].setVec0.empty() ? 0 : 1;
        }
        if (setCount == GetAICCount()) {
            UpdateSyncInfoInMode0<BlockType::AICUBE>(flagId);
        }
    } else {
        for (uint32_t i = 0; i < maxBlockNum_; i++) {
            if (!IsAIC(i, kernelType_)) {
                setCount += blockSyncEvent_[i][flagId].setVec0.empty() ? 0 : 1;
            }
        }
        if (setCount == GetAIVCount()) {
            UpdateSyncInfoInMode0<BlockType::AIVEC>(flagId);
        }
    }
}
// mode1:group内AIV->AIV
void CrossCoreSyncInfoContainer::SetMode1SyncInfo(uint8_t flagId, uint32_t blockIdx, const VectorTime &vectorTime)
{
    if (IsAIC(blockIdx, kernelType_)) {
        return;
    }
    // 同group对端AIV blockIdx
    uint32_t aivInGroup = (C220_VEC_SUB_BLOCKDIM - blockIdx % C220_MIX_SUB_BLOCKDIM - 1) +
        blockIdx / C220_MIX_SUB_BLOCKDIM * C220_MIX_SUB_BLOCKDIM;
    if (!blockSyncEvent_[aivInGroup][flagId].setVec1.empty()) {
        VectorTime blv = blockSyncEvent_[aivInGroup][flagId].setVec1.front();
        VectorClock::UpdateVectorTime(vectorTime, blv);
        blockSyncEvent_[aivInGroup][flagId].setVec1.pop();
        blockSyncEvent_[blockIdx][flagId].waitVec.push(blv);
        blockSyncEvent_[aivInGroup][flagId].waitVec.push(blv);
        return;
    }
    blockSyncEvent_[blockIdx][flagId].setVec1.push(vectorTime);
}
// mode2:AIV->AIC或AIC->AIV, AIV->AIC需要两个AIV均设置发送同步信号
void CrossCoreSyncInfoContainer::SetMode2SyncInfo(uint8_t flagId, uint32_t blockIdx, const VectorTime &vectorTime,
                                                  uint8_t vecSubBlockDim)
{
    if (vecSubBlockDim == 0) {
        SAN_WARN_LOG("Set SyncInfo Mode 2 failed due to VecSubBlockDim error");
        return;
    }

    blockSyncEvent_[blockIdx][flagId].setVec2.push(vectorTime);

    if (vecSubBlockDim == 1) {
        // vec:cube = 1:1
        if (IsAIC(blockIdx, kernelType_)) {
            // cube核
            uint32_t aivInGroup = blockIdx / C220_MIX_SUB_BLOCKDIM / C220_VEC_SUB_BLOCKDIM * C220_MIX_SUB_BLOCKDIM +
                                  blockIdx / C220_MIX_SUB_BLOCKDIM % C220_VEC_SUB_BLOCKDIM;
            blockSyncEvent_[aivInGroup][flagId].waitVec.push(vectorTime);
            blockSyncEvent_[blockIdx][flagId].setVec2.pop();
        } else {
            // vec核
            uint32_t aicInGroup = C220_MIX_SUB_BLOCKDIM * (blockIdx / C220_MIX_SUB_BLOCKDIM * C220_VEC_SUB_BLOCKDIM +
                                  blockIdx % C220_MIX_SUB_BLOCKDIM) + C220_VEC_SUB_BLOCKDIM;
            blockSyncEvent_[aicInGroup][flagId].waitVec.push(vectorTime);
            blockSyncEvent_[blockIdx][flagId].setVec2.pop();
        }
    } else {
        // vec:cube = 2:1
        if (IsAIC(blockIdx, kernelType_)) {
            // cube核
            blockSyncEvent_[blockIdx - 1U][flagId].waitVec.push(vectorTime);
            blockSyncEvent_[blockIdx - 2U][flagId].waitVec.push(vectorTime);
            blockSyncEvent_[blockIdx][flagId].setVec2.pop();
        } else {
            // vec核
            uint32_t aivInGroup = (C220_VEC_SUB_BLOCKDIM - blockIdx % C220_MIX_SUB_BLOCKDIM - 1) +
                                  blockIdx / C220_MIX_SUB_BLOCKDIM * C220_MIX_SUB_BLOCKDIM;
            if (blockSyncEvent_[aivInGroup][flagId].setVec2.empty()) {
                return;
            }
            uint32_t aicInGroup = 2 + blockIdx / C220_MIX_SUB_BLOCKDIM * C220_MIX_SUB_BLOCKDIM;
            VectorTime vt = blockSyncEvent_[aivInGroup][flagId].setVec2.front();
            VectorClock::UpdateVectorTime(vectorTime, vt);
            blockSyncEvent_[aicInGroup][flagId].waitVec.push(vt);
            blockSyncEvent_[blockIdx][flagId].setVec2.pop();
            blockSyncEvent_[aivInGroup][flagId].setVec2.pop();
        }
    }
}

// mode4:AIV0->AIC, AIV1->AIC或AIC->AIV0, AIC->AIV1, AIV0/AIV1可单独触发AIC等待
void CrossCoreSyncInfoContainer::SetMode4SyncInfo(uint8_t syncId, uint32_t blockIdx, const VectorTime &vectorTime, uint8_t vecSubBlockDim)
{
    if (vecSubBlockDim == 0) {
        SAN_WARN_LOG("Set SyncInfo Mode 4 failed due to VecSubBlockDim error");
        return;
    }
    if (!IsAIC(blockIdx, kernelType_) && syncId >= 16) {
        SAN_WARN_LOG("Set SyncInfo Mode 4 failed due to SyncId invalid in vector core");
        return;
    }
    blockSyncEvent_[blockIdx][syncId].setVec4.push(vectorTime);
    if (vecSubBlockDim == 1) {
        // vec:cube = 1:1
        if (IsAIC(blockIdx, kernelType_)) {
            // cube核
            uint32_t aivInGroup = blockIdx / C220_MIX_SUB_BLOCKDIM / C220_VEC_SUB_BLOCKDIM * C220_MIX_SUB_BLOCKDIM +
                                  blockIdx / C220_MIX_SUB_BLOCKDIM % C220_VEC_SUB_BLOCKDIM;
            blockSyncEvent_[aivInGroup][syncId].intrablockwaitVec.push(vectorTime);
        } else {
            // vec核
            uint32_t aicInGroup = C220_MIX_SUB_BLOCKDIM * (blockIdx / C220_MIX_SUB_BLOCKDIM * C220_VEC_SUB_BLOCKDIM +
                                  blockIdx % C220_MIX_SUB_BLOCKDIM) + C220_VEC_SUB_BLOCKDIM;
            blockSyncEvent_[aicInGroup][syncId].intrablockwaitVec.push(vectorTime);
        }
        blockSyncEvent_[blockIdx][syncId].setVec4.pop();
    } else {
        // vec:cube = 2:1, 该模式下AIV0/1单独与AIC配对
        if (IsAIC(blockIdx, kernelType_)) {
            // cube核
            if (syncId <= 15) {
                // 此时cube core匹配vector core0 的 0~15
                blockSyncEvent_[blockIdx - 2U][syncId].intrablockwaitVec.push(vectorTime);
            } else if (syncId >= 16 && syncId <= 31)
            {
                // 此时cube core匹配vector core1 的 0~15
                blockSyncEvent_[blockIdx - 1U][syncId - 16].intrablockwaitVec.push(vectorTime);
            }
            blockSyncEvent_[blockIdx][syncId].setVec4.pop();
        } else {
            // vec核
            uint32_t aicInGroup = 2 + blockIdx / C220_MIX_SUB_BLOCKDIM * C220_MIX_SUB_BLOCKDIM;
            uint32_t aiv0InGroup = blockIdx / C220_MIX_SUB_BLOCKDIM * C220_MIX_SUB_BLOCKDIM;
            if (blockIdx == aiv0InGroup) {
                // vector core0
                blockSyncEvent_[aicInGroup][syncId].intrablockwaitVec.push(vectorTime);
            } else {
                // vector core1
                blockSyncEvent_[aicInGroup][syncId + 16].intrablockwaitVec.push(vectorTime);
            }
            blockSyncEvent_[blockIdx][syncId].setVec4.pop();
        }
    }
}

void CrossCoreSyncInfoContainer::SetBlockSyncInfo(uint8_t flagId, FftsSyncMode mode, uint32_t blockIdx,
    const VectorTime &vectorTime, uint8_t vecSubBlockDim)
{

    if (flagId > MAX_FLAG_ID) {
        SAN_WARN_LOG("FlagId [%u] exceeds the valid scope.", static_cast<uint32_t>(flagId));
        return;
    }
    if (blockIdx >= maxBlockNum_) {
        return;
    }
    if (mode == FftsSyncMode::MODE0) {
        SetMode0SyncInfo(flagId, blockIdx, vectorTime);
    } else if (mode == FftsSyncMode::MODE1) {
        SetMode1SyncInfo(flagId, blockIdx, vectorTime);
    } else if (mode == FftsSyncMode::MODE2) {
        SetMode2SyncInfo(flagId, blockIdx, vectorTime, vecSubBlockDim);
    } else if (mode == FftsSyncMode::MODE4) {
        SetMode4SyncInfo(flagId, blockIdx, vectorTime, vecSubBlockDim);
    }
}

bool CrossCoreSyncInfoContainer::GetBlockSyncInfo(uint8_t flagId, uint32_t blockIdx, VectorTime &vectorTime)
{
    if (blockIdx >= maxBlockNum_) {
        return false;
    }
    if (!blockSyncEvent_[blockIdx][flagId].waitVec.empty()) {
        VectorClock::UpdateVectorTime(blockSyncEvent_[blockIdx][flagId].waitVec.front(), vectorTime);
        blockSyncEvent_[blockIdx][flagId].waitVec.pop();
        return true;
    }
    return false;
}

bool CrossCoreSyncInfoContainer::GetIntraBlockSyncInfo(uint8_t syncId, uint32_t blockIdx, VectorTime &vectorTime)
{
    if (blockIdx >= maxBlockNum_) {
        return false;
    }
    if (!blockSyncEvent_[blockIdx][syncId].intrablockwaitVec.empty()) {
        VectorClock::UpdateVectorTime(blockSyncEvent_[blockIdx][syncId].intrablockwaitVec.front(), vectorTime);
        blockSyncEvent_[blockIdx][syncId].intrablockwaitVec.pop();
        return true;
    }
    return false;
}

void CrossCoreSyncInfoContainer::SetBlockSoftSyncInfo(int32_t eventID, uint32_t blockIdx, const VectorTime& vectorTime)
{
    if (blockIdx >= maxBlockNum_) {
        return;
    }
    blockSoftSyncInfo_[blockIdx].mapVectorTime[eventID] = vectorTime;
}

bool CrossCoreSyncInfoContainer::GetBlockSoftSyncInfo(int32_t eventID, uint32_t waitBlockIdx, VectorTime& vectorTime)
{
    if (waitBlockIdx >= maxBlockNum_) {
        return false;
    }
    auto& map = blockSoftSyncInfo_[waitBlockIdx].mapVectorTime;
    if (map.find(eventID) != map.end()) {
        VectorClock::UpdateVectorTime(map[eventID], vectorTime);
        map.erase(eventID);
        return true;
    }
    return false;
}

bool CrossCoreSyncInfoContainer::SyncAll(uint32_t blockIdx, int32_t usedCores, uint32_t pipeIdx, VectorTime& vecTime)
{
    if (usedCores < 0 || usedCores > static_cast<int32_t>(maxBlockNum_)) {
        return false;
    }
    if (usedCores == 0) {
        usedCores = static_cast<int32_t>(GetAIVCount());
    }

    if (!semCores_.IsFinished()) {
        if (semCores_.GetCores() != usedCores) {
            // 异常情况：在一组syncAll尚未处理完成，上层调用了一个不同入参的syncAll
            return false;
        }
    } else {
        // 首次的数据重置
        semCores_.Reset(usedCores);
        for (auto& info : blockSoftSyncInfo_) {
            info.syncAll = false;
        }
    }
    if (!blockSoftSyncInfo_[blockIdx].syncAll) {
        // pipe第一次调用的syncAll，更新逻辑时间
        VectorClock::UpdateLogicTime(vecTime, pipeIdx);
        blockSoftSyncInfo_[blockIdx].syncAll = true;
        blockSoftSyncInfo_[blockIdx].pipeIdx = pipeIdx;
    }
    if (blockSoftSyncInfo_[blockIdx].syncAll && semCores_.RemainSyncAllCount() != 0) {
        return false;
    }
    return semCores_.Wait();
}

void CrossCoreSyncInfoContainer::SetMstxCrossInfo(const MstxCrossInfo &stubInfo,
    const VectorTime& vectorTime)
{
    mstxCrossSetMap_[std::make_pair(stubInfo.addr, stubInfo.flagId)].push(vectorTime);
}

bool CrossCoreSyncInfoContainer::GetMstxCrossInfo(const MstxCrossInfo &stubInfo,
    VectorTime& localBlockTime)
{
    auto pairKey = std::make_pair(stubInfo.addr, stubInfo.flagId);
    if (mstxCrossSetMap_.find(pairKey) != mstxCrossSetMap_.end()) {
        auto &vecQue = mstxCrossSetMap_[pairKey];
        if (!vecQue.empty()) {
            VectorClock::UpdateVectorTime(vecQue.front(), localBlockTime);
            vecQue.pop();
            /// vecQue弹出一个元素后，如果vecQue为空，则需要删除map中缓存的set，保证多个set/wait配对时配对正确;
            /// 如：set1 -> set2 -> wait1 -> wait2，配对规则为set1->wait1   set2->wait2
            /// 如下判断保证了弹出set1后，假设set2没有入队，则wait2会被阻塞
            if (vecQue.empty()) {
                mstxCrossSetMap_.erase(pairKey);
            }
        }
        return true;
    }
    return false;
}

void CrossCoreSyncInfoContainer::UpdateSyncAllVectorTime(std::vector<VectorTime>& vec)
{
    for (auto& info : blockSoftSyncInfo_) {
        if (!info.syncAll) {
            continue;
        }
        auto& updateVecTime = vec[info.pipeIdx];

        for (auto& otherInfo : blockSoftSyncInfo_) {
            if (info.pipeIdx == otherInfo.pipeIdx || !otherInfo.syncAll) {
                continue;
            }
            VectorClock::UpdateVectorTime(vec[otherInfo.pipeIdx], updateVecTime);
        }
    }
}

}
