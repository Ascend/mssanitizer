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


#ifndef RACE_SANITIZER_ALG_FRAMEWORK_CROSS_CORE_SYNC_INFO_CONTAINER_H
#define RACE_SANITIZER_ALG_FRAMEWORK_CROSS_CORE_SYNC_INFO_CONTAINER_H
#include <queue>
#include <map>
#include "core/framework/event_def.h"
namespace Sanitizer {

class SemCores {
public:
    bool Wait()
    {
        if (semSync_ == 0) {
            semAfterSync_--;
            return true;
        }
        --semSync_;
        if (semSync_ == 0) {
            semAfterSync_--;
        }
        return semSync_ == 0;
    }

    int32_t GetCores() const
    {
        return cores_;
    }

    uint8_t RemainSyncAllCount() const
    {
        return semSync_;
    }

    bool IsFinished() const
    {
        return semSync_ == 0 && semAfterSync_ == 0;
    }

    void Reset(int32_t cores)
    {
        cores_ = cores;
        semSync_ = static_cast<uint32_t>(cores);
        semAfterSync_ = static_cast<uint32_t>(cores);
    }
private:
    int32_t cores_ = 0; // 初始化的usedCores值
    uint32_t semSync_ = 0; // 调用syncAll的pipe数
    uint32_t semAfterSync_ = 0; // 跳出syncAll等待的pipe数
};

// 核间硬同步向量时钟
struct BlockSyncInfo {
    std::queue<VectorTime> setVec0;
    std::queue<VectorTime> setVec1;
    std::queue<VectorTime> setVec2;
    std::queue<VectorTime> setVec4;
    std::queue<VectorTime> waitVec;
    std::queue<VectorTime> intrablockwaitVec;
};

struct BlockSoftSyncInfo {
    std::map<int32_t, VectorTime> mapVectorTime;
    bool syncAll = false;
    uint32_t pipeIdx;
};
constexpr uint8_t MAX_FLAG_ID = 32;
using BlockSyncEvent = std::array<BlockSyncInfo, MAX_FLAG_ID>;

// 该类用于维护核间同步指令产生的核间同步信息
// setVec用于储存ffts_cross_core_sync指定mode、flag的向量时钟, waitVec用于存储block收到的指定mode、flag的同步向量时钟
class CrossCoreSyncInfoContainer {
public:
    void Init(uint32_t blockNum, KernelType kernelType);
    // 设置同步向量时钟
    void SetBlockSyncInfo(uint8_t flagId, FftsSyncMode mode, uint32_t blockIdx, const VectorTime &vectorTime,
                          uint8_t vecSubBlockDim = 2);
    // 获取同步向量时钟并更新本地时钟
    bool GetBlockSyncInfo(uint8_t flagId, uint32_t blockIdx, VectorTime &vectorTime);
    bool GetIntraBlockSyncInfo(uint8_t flagId, uint32_t blockIdx, VectorTime &vectorTime);

    // 设置同步向量时钟-软同步
    void SetBlockSoftSyncInfo(int32_t eventID, uint32_t blockIdx, const VectorTime& vectorTime);
    // 获取同步向量时钟并更新本地时钟-软同步
    bool GetBlockSoftSyncInfo(int32_t eventID, uint32_t waitBlockIdx, VectorTime& vectorTime);
    // 任意多核软同步
    bool SyncAll(uint32_t blockIdx, int32_t usedCores, uint32_t pipeIdx, VectorTime& vecTime);
    // 参与软同步的全体向量时钟同步处理
    void UpdateSyncAllVectorTime(std::vector<VectorTime>& vec);

    // 设置同步向量时钟-mstx多核同步
    void SetMstxCrossInfo(const MstxCrossInfo &stubInfo, const VectorTime& vectorTime);
    // 获取同步向量时钟并更新本地时钟-mstx多核同步
    bool GetMstxCrossInfo(const MstxCrossInfo &stubInfo, VectorTime& localBlockTime);
private:
    uint32_t GetAIVCount();
    uint32_t GetAICCount();
    void SetMode0SyncInfo(uint8_t flagId, uint32_t blockIdx, const VectorTime &vectorTime);
    void SetMode1SyncInfo(uint8_t flagId, uint32_t blockIdx, const VectorTime &vectorTime);
    void SetMode2SyncInfo(uint8_t flagId, uint32_t blockIdx, const VectorTime &vectorTime,
                          uint8_t vecSubBlockDim = 2);
    void SetMode4SyncInfo(uint8_t flagId, uint32_t blockIdx, const VectorTime &vectorTime,
                          uint8_t vecSubBlockDim = 2);
    template<BlockType blockType>
    void UpdateSyncInfoInMode0(uint8_t flagId);
private:
    uint32_t maxBlockNum_ = 0;
    KernelType kernelType_{};
    std::vector<BlockSyncEvent> blockSyncEvent_;
    std::vector<BlockSoftSyncInfo> blockSoftSyncInfo_;
    /// map-key: std::pair<addr, flagId>，key为addr和flagId
    /// map-value: queue<VectorTime>
    std::map<std::pair<uint64_t, uint64_t>, std::queue<VectorTime>> mstxCrossSetMap_;
    SemCores semCores_;
};

}

#endif
