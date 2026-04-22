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


#include <gtest/gtest.h>

#include "rand_data.h"
#include "alg_framework/race_alg_impl.h"

using namespace Sanitizer;

TEST(RaceAlgImpl, pipe_flow_can_exec_all_events_expect_success)
{
    EventContainer events;
    SanEvent event;
    events.Init(1);
    for (uint32_t i = 0U; i < 100U; i++) {
        event.pipe = PipeType::PIPE_M;
        events.Push(event, event.pipe, 0);

        event.pipe = PipeType::PIPE_V;
        events.Push(event, event.pipe, 0);

        event.pipe = PipeType::PIPE_MTE1;
        events.Push(event, event.pipe, 0);
    }

    auto f = [](const SanEvent&) -> ReturnType { return ReturnType::PROCESS_OK; };

    PipeLine pipeLine(events);
    pipeLine.RegisterEventFunc(f);

    ASSERT_EQ(events.GetAllQueSize(), 300U);
    pipeLine.Run();
    ASSERT_EQ(events.GetAllQueSize(), 0U);
}

TEST(RaceAlgImpl, race_alg_can_detect_race_events_expect_success)
{
    RaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent event;

    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_M;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    alg.Do(event);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    alg.Do(event);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE1;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    alg.Do(event);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    ASSERT_EQ(alg.GetRaceCount(), 3U);
    ASSERT_EQ(alg.IsFinished(), true);
}

TEST(RaceAlgImpl, race_alg_can_detect_cross_core_sync_events_expect_success)
{
    RaceAlgImpl alg(KernelType::MIX, DeviceType::ASCEND_910B1, 2);
    SanEvent event;

    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_M;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    alg.Do(event);
    event.type = EventType::CROSS_CORE_SYNC_EVENT;
    event.pipe = PipeType::PIPE_M;
    event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
    event.eventInfo.fftsSyncInfo.mode = static_cast<uint8_t>(FftsSyncMode::MODE0);
    alg.Do(event);
    event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
    alg.Do(event);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE1;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    alg.Do(event);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    ASSERT_EQ(alg.GetRaceCount(), 1U);
}

TEST(RaceAlgImpl, race_alg_can_detect_cross_core_soft_sync_events_expect_success)
{
    RaceAlgImpl alg(KernelType::AICUBE, DeviceType::ASCEND_910B1, 2);
    SanEvent event;

    event.type = EventType::CROSS_CORE_SOFT_SYNC_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.loc.blockType = BlockType::AIVEC;
    event.loc.coreId = 0;
    event.isAtomicMode = false;
    event.loc.fileNo = 10;
    event.loc.lineNo = 10;
    event.eventInfo.softSyncInfo.opType = SyncType::IB_SET;
    event.eventInfo.softSyncInfo.eventID = 11;
    event.eventInfo.softSyncInfo.waitCoreID = 0;
    event.eventInfo.softSyncInfo.isAIVOnly = true;
    alg.Do(event);
    event.eventInfo.softSyncInfo.opType = SyncType::SYNC_ALL;
    event.eventInfo.softSyncInfo.usedCores = 2;
    alg.Do(event);
    event.eventInfo.softSyncInfo.opType = SyncType::IB_WAIT;
    event.loc.coreId = 1;
    alg.Do(event);
    event.eventInfo.softSyncInfo.opType = SyncType::SYNC_ALL;
    alg.Do(event);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    ASSERT_EQ(alg.GetRaceCount(), 0U);
}

void PushFftsMode2SubBlockOneEvents(RaceAlgImpl *alg, SanEvent &event)
{
    event.type = EventType::CROSS_CORE_SYNC_EVENT;
    event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
    const uint8_t fftsMode2 = 2;
    event.eventInfo.fftsSyncInfo.mode = fftsMode2;
    const uint8_t fftsVecSubBlockDim = 1;
    event.eventInfo.fftsSyncInfo.vecSubBlockDim = fftsVecSubBlockDim;
    alg->Do(event);
    event.loc.blockType = event.loc.blockType == BlockType::AICUBE ? BlockType::AIVEC : BlockType::AICUBE;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
    event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
    alg->Do(event);
}

void PushFftsMode2SubBlockTwoEvents(RaceAlgImpl *alg, SanEvent &event)
{
    event.type = EventType::CROSS_CORE_SYNC_EVENT;
    event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
    const uint8_t fftsMode2 = 2;
    event.eventInfo.fftsSyncInfo.mode = fftsMode2;
    const uint8_t fftsVecSubBlockDim = 2;
    event.eventInfo.fftsSyncInfo.vecSubBlockDim = fftsVecSubBlockDim;
    alg->Do(event);

    if (event.loc.blockType == BlockType::AIVEC) {
        event.loc.coreId = event.loc.coreId % C220_VEC_SUB_BLOCKDIM == 0 ? ++event.loc.coreId : --event.loc.coreId;
        alg->Do(event);
    }

    event.type = EventType::CROSS_CORE_SYNC_EVENT;
    event.loc.blockType = event.loc.blockType == BlockType::AICUBE ? BlockType::AIVEC : BlockType::AICUBE;
    event.loc.coreId = event.loc.blockType == BlockType::AIVEC ? event.loc.coreId * C220_VEC_SUB_BLOCKDIM +
        RandUint(0, 1) : event.loc.coreId / C220_VEC_SUB_BLOCKDIM;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
    event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
    alg->Do(event);

    if (event.loc.blockType == BlockType::AIVEC) {
        event.loc.coreId = event.loc.coreId % C220_VEC_SUB_BLOCKDIM == 0 ? ++event.loc.coreId : --event.loc.coreId;
        alg->Do(event);
    }
}

void PushFftsMode0Events(RaceAlgImpl *alg, SanEvent &event, PipeType setPipe, uint32_t totalBlockNum)
{
    event.type = EventType::CROSS_CORE_SYNC_EVENT;
    event.eventInfo.fftsSyncInfo.mode = 0;
    const uint8_t fftsVecSubBlockDim = 2;
    event.eventInfo.fftsSyncInfo.vecSubBlockDim = fftsVecSubBlockDim;

    for (size_t i = 0; i < totalBlockNum; ++i) {
        event.pipe = setPipe;
        event.eventInfo.fftsSyncInfo.dstPipe = setPipe;
        event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
        event.loc.coreId = i;
        alg->Do(event);

        event.pipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
        alg->Do(event);
    }
}

void PushSetWaitFlagEvents(RaceAlgImpl *alg, SanEvent &event, const std::pair<PipeType, PipeType> &pairPipe,
    bool enablePipeS = false)
{
    event.type = EventType::SYNC_EVENT;
    event.pipe = pairPipe.first;
    event.eventInfo.syncInfo.opType = SyncType::SET_FLAG;
    event.eventInfo.syncInfo.srcPipe = pairPipe.first;
    if (enablePipeS) {
        event.eventInfo.syncInfo.dstPipe = pairPipe.second;
    } else {
        event.eventInfo.syncInfo.dstPipe = RandUint(0, 1) == 0U ? pairPipe.second : PipeType::PIPE_S;
    }
    alg->Do(event);
    event.pipe = event.eventInfo.syncInfo.dstPipe;
    event.eventInfo.syncInfo.opType = SyncType::WAIT_FLAG;
    alg->Do(event);
}

TEST(RaceAlgImpl, race_alg_with_ffts_mode2_1_ratio_1_expect_success)
{
    for (uint64_t i = 0; i < 20; ++i) {
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        RaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        auto memBlockType = static_cast<BlockType>(RandUint(static_cast<uint64_t>(BlockType::AIVEC),
                            static_cast<uint64_t>(BlockType::AICUBE)));
        auto pairPipe = RandTwoDiffPipe();
        auto pairFlagId = RandTwoDiffUint(0, 15);
        uint32_t memCoreId = RandUint(0, blockDim - 1);

        event.loc.blockType = memBlockType;
        event.loc.coreId = memCoreId;
        event.pipe = pairPipe.first;
        alg.Do(event);

        if (RandUint(0, 1) == 0U) {
            PushSetWaitFlagEvents(&alg, event, pairPipe);
            event.pipe = pairPipe.second;
            event.eventInfo.fftsSyncInfo.dstPipe = pairPipe.second;
        } else {
            event.pipe = pairPipe.first;
            event.eventInfo.fftsSyncInfo.dstPipe = pairPipe.first;
        }

        event.eventInfo.fftsSyncInfo.flagId = pairFlagId.first;
        PushFftsMode2SubBlockOneEvents(&alg, event);

        auto ffts2Pipe = RandPipe();
        event.pipe = ffts2Pipe;
        event.eventInfo.fftsSyncInfo.dstPipe = ffts2Pipe;
        event.eventInfo.fftsSyncInfo.flagId = pairFlagId.second;
        PushFftsMode2SubBlockOneEvents(&alg, event);

        event.eventInfo.memInfo.memType = MemType::GM;
        event.loc.coreId = memCoreId;
        event.type = EventType::MEM_EVENT;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
                                         static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        event.loc.blockType = memBlockType;
        event.pipe = RandDiffPipe(pairPipe).first;
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        ASSERT_EQ(alg.GetRaceCount(), 0U);
    }
}

TEST(RaceAlgImpl, race_alg_with_ffts_mode2_1_ratio_1_non_circular_ffts_expect_can_detect_race_events)
{
    for (uint64_t i = 0; i < 20; ++i) {
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        RaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        auto memBlockType = static_cast<BlockType>(RandUint(static_cast<uint64_t>(BlockType::AIVEC),
                            static_cast<uint64_t>(BlockType::AICUBE)));
        auto pairPipe = RandTwoDiffPipe();
        uint32_t memCoreId = RandUint(0, blockDim - 1);

        event.loc.blockType = memBlockType;
        event.loc.coreId = memCoreId;
        event.pipe = pairPipe.first;
        alg.Do(event);

        if (RandUint(0, 1) == 0U) {
            PushSetWaitFlagEvents(&alg, event, pairPipe, true);
            event.pipe = pairPipe.second;
            event.eventInfo.fftsSyncInfo.dstPipe = pairPipe.second;
        } else {
            event.pipe = pairPipe.first;
            event.eventInfo.fftsSyncInfo.dstPipe = pairPipe.first;
        }

        if (RandUint(0, 1) != 1U) {
            PushFftsMode2SubBlockOneEvents(&alg, event);
        } else {
            auto ffts2Pipe = RandPipe();
            event.pipe = ffts2Pipe;
            event.eventInfo.fftsSyncInfo.dstPipe = ffts2Pipe;
            PushFftsMode2SubBlockOneEvents(&alg, event);
        }

        event.eventInfo.memInfo.memType = MemType::GM;
        event.loc.coreId = memCoreId;
        event.loc.blockType = memBlockType;
        event.type = EventType::MEM_EVENT;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
                                         static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        event.pipe = RandDiffPipe(pairPipe).first;
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        uint32_t raceCount = event.eventInfo.memInfo.opType == AccessType::MEMCPY_BLOCKS ? 2 : 1;
        ASSERT_EQ(alg.GetRaceCount(), raceCount);
    }
}

TEST(RaceAlgImpl, race_alg_with_ffts_mode2_1_ratio_2_expect_success)
{
    for (uint64_t i = 0; i < 20 ; ++i) {
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        RaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        auto memBlockType = static_cast<BlockType>(RandUint(static_cast<uint64_t>(BlockType::AIVEC),
                            static_cast<uint64_t>(BlockType::AICUBE)));
        auto pairPipe = RandTwoDiffPipe();
        auto pairFlagId = RandTwoDiffUint(0, 15);
        uint32_t memCoreId = memBlockType == BlockType::AIVEC ? RandUint(0, blockDim * 2 - 1) :
            RandUint(0, blockDim - 1);

        event.loc.blockType = memBlockType;
        event.loc.coreId = memCoreId;
        event.pipe = pairPipe.first;
        alg.Do(event);

        if (RandUint(0, 1) == 0U) {
            PushSetWaitFlagEvents(&alg, event, pairPipe);
            event.pipe = pairPipe.second;
            event.eventInfo.fftsSyncInfo.dstPipe = pairPipe.second;
        } else {
            event.pipe = pairPipe.first;
            event.eventInfo.fftsSyncInfo.dstPipe = pairPipe.first;
        }

        event.eventInfo.fftsSyncInfo.flagId = pairFlagId.first;
        PushFftsMode2SubBlockTwoEvents(&alg, event);

        event.pipe = pairPipe.first;
        event.eventInfo.fftsSyncInfo.dstPipe = pairPipe.second;
        event.eventInfo.fftsSyncInfo.flagId = pairFlagId.second;
        PushFftsMode2SubBlockTwoEvents(&alg, event);

        event.eventInfo.memInfo.memType = MemType::GM;
        event.loc.coreId = memCoreId;
        event.loc.blockType = memBlockType;
        event.type = EventType::MEM_EVENT;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
                                         static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        event.pipe = RandDiffPipe(pairPipe).first;
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        ASSERT_EQ(alg.GetRaceCount(), 0U);
    }
}

TEST(RaceAlgImpl, race_alg_with_ffts_mode2_1_ratio_2_non_circular_ffts_expect_can_detect_race_events)
{
    for (uint64_t i = 0; i < 20; ++i) {
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        RaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        auto memBlockType = static_cast<BlockType>(RandUint(static_cast<uint64_t>(BlockType::AIVEC),
                            static_cast<uint64_t>(BlockType::AICUBE)));
        auto pairCoreId = RandTwoDiffUint(0, blockDim - 1);
        auto pairPipe = RandTwoDiffPipe();
        auto pairFlagId = RandTwoDiffUint(0, 15);

        event.loc.blockType = memBlockType;
        event.loc.coreId = pairCoreId.first;
        event.pipe = pairPipe.first;
        alg.Do(event);

        if (RandUint(0, 1) == 0U) {
            PushSetWaitFlagEvents(&alg, event, pairPipe, true);
            event.pipe = pairPipe.second;
            event.eventInfo.fftsSyncInfo.dstPipe = pairPipe.second;
        } else {
            event.pipe = pairPipe.first;
            event.eventInfo.fftsSyncInfo.dstPipe = pairPipe.first;
        }

        if (RandUint(0, 1) != 1U) {
            event.eventInfo.fftsSyncInfo.dstPipe = pairPipe.first;
            event.eventInfo.fftsSyncInfo.flagId = pairFlagId.first;
            PushFftsMode2SubBlockTwoEvents(&alg, event);
        } else {
            event.pipe = pairPipe.first;
            event.loc.coreId = pairCoreId.second;
            event.eventInfo.fftsSyncInfo.dstPipe = pairPipe.second;
            event.eventInfo.fftsSyncInfo.flagId = pairFlagId.second;
            PushFftsMode2SubBlockTwoEvents(&alg, event);
        }

        event.eventInfo.memInfo.memType = MemType::GM;
        event.loc.coreId = pairCoreId.first;;
        event.loc.blockType = memBlockType;
        event.type = EventType::MEM_EVENT;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
                                         static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        event.pipe = RandDiffPipe(pairPipe).first;
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        uint32_t raceCount = event.eventInfo.memInfo.opType == AccessType::MEMCPY_BLOCKS ? 2 : 1;
        ASSERT_EQ(alg.GetRaceCount(), raceCount);
    }
}

TEST(RaceAlgImpl, race_alg_with_ffts_mode0_1_ratio_2_expect_success)
{
    for (uint64_t i = 0; i < 20; ++i) {
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        RaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        auto memBlockType = static_cast<BlockType>(RandUint(static_cast<uint64_t>(BlockType::AIVEC),
                            static_cast<uint64_t>(BlockType::AICUBE)));
        auto pairPipe = RandTwoDiffPipe();
        auto pairCoreId = (memBlockType == BlockType::AIVEC) ? RandTwoDiffUint(0, blockDim * 2 - 1) :
            RandTwoDiffUint(0, blockDim - 1);

        event.loc.blockType = memBlockType;
        event.loc.coreId = pairCoreId.first;
        event.pipe = pairPipe.first;
        alg.Do(event);

        uint32_t totalBlockNum = memBlockType == BlockType::AICUBE ? blockDim : 2 * blockDim;
        if (RandUint(0, 1) == 0U) {
            PushSetWaitFlagEvents(&alg, event, pairPipe);
            PushFftsMode0Events(&alg, event, pairPipe.second, totalBlockNum);
        } else {
            PushFftsMode0Events(&alg, event, pairPipe.first, totalBlockNum);
        }

        event.eventInfo.memInfo.memType = MemType::GM;
        event.loc.coreId = pairCoreId.first;
        event.type = EventType::MEM_EVENT;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
                                         static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        event.pipe = RandDiffPipe(pairPipe).first;
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        ASSERT_EQ(alg.GetRaceCount(), 0U);
    }
}

TEST(RaceAlgImpl, race_alg_with_ffts_mode0_1_ratio_2_non_circular_ffts_expect_can_detect_race_events)
{
    for (uint64_t i = 0; i < 20; ++i) {
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        RaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        auto memBlockType = static_cast<BlockType>(RandUint(static_cast<uint64_t>(BlockType::AIVEC),
                            static_cast<uint64_t>(BlockType::AICUBE)));
        auto pairPipe = RandTwoDiffPipe();
        auto twoDiffPairPipe = RandDiffPipe(pairPipe);
        auto pairCoreId = (memBlockType == BlockType::AIVEC) ? RandTwoDiffUint(0, blockDim * 2 - 1) :
            RandTwoDiffUint(0, blockDim - 1);

        event.loc.blockType = memBlockType;
        event.loc.coreId = pairCoreId.first;
        event.pipe = pairPipe.first;
        alg.Do(event);

        uint32_t totalBlockNum = memBlockType == BlockType::AICUBE ? blockDim : 2 * blockDim;
        if (RandUint(0, 1) == 0U) {
            PushSetWaitFlagEvents(&alg, event, pairPipe, true);
            PushFftsMode0Events(&alg, event, twoDiffPairPipe.first, totalBlockNum);
        } else {
            PushFftsMode0Events(&alg, event, pairPipe.second, totalBlockNum);
        }

        event.eventInfo.memInfo.memType = MemType::GM;
        event.loc.coreId = pairCoreId.first;
        event.type = EventType::MEM_EVENT;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
                                         static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        event.pipe = twoDiffPairPipe.second;
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        uint32_t raceCount = event.eventInfo.memInfo.opType == AccessType::MEMCPY_BLOCKS ? 2 : 1;
        ASSERT_EQ(alg.GetRaceCount(), raceCount);
    }
}

TEST(RaceAlgImpl, dynamic_write_event_with_mem_event_expect_race_error)
{
    RaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_950DT_950x, 3);
    SanEvent event;
    event.loc.coreId = 0;
    event.type = EventType::DYNAMIC_MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    event.loc.blockType = BlockType::AIVEC;
    auto &dynamicOpInfo = event.eventInfo.dynamicOpInfo;
    dynamicOpInfo.count = 3;
    dynamicOpInfo.minAddr = 0X100;
    dynamicOpInfo.maxAddr = 0x500;
    dynamicOpInfo.dynamicType = RecordType::SHADOW_MEMORY;
    std::vector<ShadowMemoryRecord> shRecords1 = GenShodowMemorys(dynamicOpInfo.minAddr, AccessType::READ,
        dynamicOpInfo.count, event.loc.coreId);
    dynamicOpInfo.buffer = shRecords1.data();
    alg.Do(event);
    event.loc.coreId = 0;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = 0x100;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 32U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    alg.Do(event);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    ASSERT_EQ(alg.GetRaceCount(), 1U);
}

TEST(RaceAlgImpl, dynamic_write_event_with_mem_event_expect_normal)
{
    RaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_950DT_950x, 3);
    SanEvent event;
    event.loc.coreId = 0;
    event.type = EventType::DYNAMIC_MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    event.loc.blockType = BlockType::AIVEC;
    auto &dynamicOpInfo = event.eventInfo.dynamicOpInfo;
    dynamicOpInfo.count = 2;
    dynamicOpInfo.minAddr = 0X100;
    dynamicOpInfo.maxAddr = 0x500;
    dynamicOpInfo.dynamicType = RecordType::SHADOW_MEMORY;
    std::vector<ShadowMemoryRecord> shRecords1 = GenShodowMemorys(dynamicOpInfo.minAddr, AccessType::WRITE,
        dynamicOpInfo.count, event.loc.coreId);
    dynamicOpInfo.buffer = shRecords1.data();
    alg.Do(event);
    event.loc.coreId = 0;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = 0x500;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 32U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    alg.Do(event);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    ASSERT_EQ(alg.GetRaceCount(), 0U);
}
