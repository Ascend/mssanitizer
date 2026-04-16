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

#ifndef TEST_RACE_SANITIZER_RAND_DATA_H
#define TEST_RACE_SANITIZER_RAND_DATA_H

#include <random>

#include "core/framework/event_def.h"

namespace Sanitizer {

inline uint64_t RandUint(uint64_t min, uint64_t max)
{
    static std::mt19937_64 mersenneEngine(std::random_device{}());
    std::uniform_int_distribution<uint64_t> dist(min, max);
    return dist(mersenneEngine);
}

inline std::pair<uint64_t, uint64_t> RandTwoDiffUint(uint64_t min, uint64_t max)
{
    std::pair<uint64_t, uint64_t> pairUint;
    pairUint.first = RandUint(min, max);
    uint64_t uint2 = RandUint(min, max);
    if (min == max) {
        pairUint.second = uint2;
        return pairUint;
    }
    while (uint2 == pairUint.first) {
        uint2 = RandUint(min, max);
    }
    pairUint.second = uint2;
    return pairUint;
}

inline PipeType RandPipe()
{
    uint64_t randSelect = RandUint(0, 1);
    if (randSelect == 1U) {
        return static_cast<PipeType>(RandUint(static_cast<uint64_t>(PipeType::PIPE_V),
            static_cast<uint64_t>(PipeType::PIPE_MTE3)));
    } else {
        return static_cast<PipeType>(RandUint(static_cast<uint64_t>(PipeType::PIPE_MTE4),
            static_cast<uint64_t>(PipeType::PIPE_FIX)));
    }
}

inline std::pair<PipeType, PipeType> RandTwoDiffPipe()
{
    std::pair<PipeType, PipeType> pairPipe;
    pairPipe.first = RandPipe();
    PipeType pipe2 = RandPipe();
    while (pipe2 == pairPipe.first) {
        pipe2 = RandPipe();
    }
    pairPipe.second = pipe2;
    return pairPipe;
}

inline std::pair<PipeType, PipeType> RandDiffPipe(const std::pair<PipeType, PipeType> &existPipe)
{
    std::pair<PipeType, PipeType> pairPipe;
    auto pipe = RandPipe();
    while (pipe == existPipe.first || pipe == existPipe.second) {
        pipe = RandPipe();
    }
    pairPipe.first = pipe;
    while (pipe == existPipe.first || pipe == existPipe.second || pipe == pairPipe.first) {
        pipe = RandPipe();
    }
    pairPipe.second = pipe;
    return pairPipe;
}

inline SanEvent CreateMemEvent()
{
    SanEvent event;
    event.type = EventType::MEM_EVENT;
    event.pipe = RandPipe();
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = RandUint(0, 0x12ffffffffff);
    const uint64_t minSize = 1;
    const uint64_t maxSize = 128;
    event.eventInfo.memInfo.blockNum = RandUint(minSize, maxSize);
    event.eventInfo.memInfo.blockSize = RandUint(minSize, maxSize);
    event.eventInfo.memInfo.repeatTimes = RandUint(minSize, maxSize);
    return event;
}

inline void AssignNewAICCoreIdEvent(uint64_t blockDim, SanEvent &event)
{
    auto coreId = event.loc.coreId;
    if (coreId == 0U) {
        event.loc.coreId = RandUint(1, blockDim - 1);
    } else if (coreId == blockDim - 1) {
        const uint64_t rightOffset = 2;
        event.loc.coreId = RandUint(0, blockDim - rightOffset);
    } else {
        event.loc.coreId = RandUint(0, 1) == 1U ? RandUint(0, coreId - 1) : RandUint(coreId + 1, blockDim - 1);
    }
}

inline void AssignNewAIVCoreIdEvent(uint64_t blockDim, SanEvent &event)
{
    auto coreId = event.loc.coreId;
    if (coreId <= 1) {
        // 位于第一个vec大核
        const uint64_t leftOffset = 2;
        event.loc.coreId = RandUint(leftOffset, blockDim * C220_VEC_SUB_BLOCKDIM - 1);
        return;
    }
    const uint64_t vecMaxRightOffset = 2;
    if (coreId >= blockDim * C220_VEC_SUB_BLOCKDIM - vecMaxRightOffset) {
        // 位于最后一个vec大核
        const uint64_t rightOffset = 3;
        event.loc.coreId = RandUint(0, blockDim * C220_VEC_SUB_BLOCKDIM - rightOffset);
        return;
    }
    if (coreId % C220_VEC_SUB_BLOCKDIM == 0U) {
        // 位于第1个子核
        const uint64_t leftOffset = 2;
        event.loc.coreId = RandUint(0, 1) == 1U ? RandUint(0, coreId - 1) :
                           RandUint(coreId + leftOffset, blockDim * C220_VEC_SUB_BLOCKDIM - 1);
    } else {
        // 位于第2个子核
        const uint64_t rightOffset = 2;
        event.loc.coreId = RandUint(0, 1) == 1U ? RandUint(0, coreId - rightOffset) :
                           RandUint(coreId + 1, blockDim * C220_VEC_SUB_BLOCKDIM - 1);
    }
}

inline void AssignNewAICAIVCoreIdEvent(uint64_t blockDim, SanEvent &event)
{
    if (event.loc.blockType == BlockType::AIVEC) {
        AssignNewAIVCoreIdEvent(blockDim, event);
    } else {
        AssignNewAICCoreIdEvent(blockDim, event);
    }
}

inline std::vector<ShadowMemoryRecord> GenShodowMemorys(uint64_t startAddr, AccessType accessType, uint64_t count, size_t coreId)
{
    std::vector<ShadowMemoryRecord> shRecords;
    shRecords.reserve(count);
    ShadowMemoryRecord smRecord{};
    smRecord.space = AddressSpace::GM;
    smRecord.accessType = accessType;
    smRecord.size = 100;
    smRecord.location.blockId = coreId;
    for (size_t i = 0; i < count; ++i) {
        smRecord.addr = startAddr + i * 0x100;
        smRecord.location.pc = 0x10 + i * 0x10;
        shRecords.emplace_back(smRecord);
    }
    return shRecords;
}

}  // namespace Sanitizer

#endif  // TEST_RACE_SANITIZER_RAND_DATA_H
