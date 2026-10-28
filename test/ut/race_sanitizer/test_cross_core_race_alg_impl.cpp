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
#define private public
#include "alg_framework/cross_core_race_alg_impl.h"
#include "alg_framework/race_alg_factory.h"
#undef private
using namespace Sanitizer;

namespace SanitizerTest {

TEST(CrossCoreRaceAlgImpl, race_alg_can_detect_race_events_expect_success)
{
    CrossCoreRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 3);
    SanEvent event;
    event.loc.coreId = 0;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_M;
    event.loc.blockType = BlockType::AIVEC;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    alg.Do(event);
    event.loc.coreId = 1;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    alg.Do(event);
    event.loc.coreId = 2;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE1;
    event.eventInfo.memInfo.opType = AccessType::READ;
    alg.Do(event);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 3U);
}

TEST(CrossCoreRaceAlgImpl, race_alg_can_detect_atomic_race_events_expect_success)
{
    CrossCoreRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 3);
    SanEvent event;
    event.loc.coreId = 0;
    event.type = EventType::MEM_EVENT;
    event.loc.blockType = BlockType::AIVEC;
    event.pipe = PipeType::PIPE_MTE3;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.isAtomicMode = true;
    alg.Do(event);
    event.loc.coreId = 1;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    alg.Do(event);
    event.loc.coreId = 2;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE1;
    event.eventInfo.memInfo.opType = AccessType::READ;
    alg.Do(event);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 2U);
}

TEST(CrossCoreRaceAlgImpl, race_alg_detect_diverse_coarse_overlapping_mem_race_events_expect_success)
{
    constexpr uint64_t unitMemLen = 32;
    constexpr uint64_t addr = 0x010000;
    CrossCoreRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 5);
    SanEvent event;
    auto fillSanEvent = [&event](uint32_t coreId, uint64_t addr, uint64_t blockNum, uint64_t blockSize,
                             uint64_t blockStride, uint64_t repeatTimes, uint64_t repeatStride, PipeType pipe) {
        event.loc.coreId = coreId;
        event.pipe = pipe;
        event.eventInfo.memInfo.addr = addr;
        event.eventInfo.memInfo.blockNum = blockNum;
        event.eventInfo.memInfo.blockSize = blockSize;
        event.eventInfo.memInfo.blockStride = blockStride;
        event.eventInfo.memInfo.repeatTimes = repeatTimes;
        event.eventInfo.memInfo.repeatStride = repeatStride;
    };

    event.type = EventType::MEM_EVENT;
    event.loc.blockType = BlockType::AIVEC;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::GM;
    fillSanEvent(0, addr, 4U, unitMemLen * 5, 4U, 1U, 1U, PipeType::PIPE_MTE3);
    alg.Do(event);
    fillSanEvent(1, addr + 2 * unitMemLen, 4U, unitMemLen * 3, 6U, 1U, 1U, PipeType::PIPE_V);
    alg.Do(event);
    fillSanEvent(2, addr + 8 * unitMemLen, 3U, unitMemLen * 4, 5U, 2U, 1U, PipeType::PIPE_MTE1);
    alg.Do(event);
    fillSanEvent(3, addr + 5 * unitMemLen, 5U, unitMemLen * 3, 4U, 1U, 1U, PipeType::PIPE_MTE2);
    alg.Do(event);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 3U);
}

TEST(CrossCoreRaceAlgImpl, race_alg_detect_zero_length_overlapping_mem_race_events_expect_success)
{
    constexpr uint64_t unitMemLen = 32;
    constexpr uint64_t addr = 0x010000;
    CrossCoreRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 5);
    SanEvent event;
    auto fillSanEvent = [&event](uint32_t coreId, uint64_t addr, uint64_t blockNum, uint64_t blockSize,
                             uint64_t blockStride, uint64_t repeatTimes, uint64_t repeatStride, PipeType pipe) {
        event.loc.coreId = coreId;
        event.pipe = pipe;
        event.eventInfo.memInfo.addr = addr;
        event.eventInfo.memInfo.blockNum = blockNum;
        event.eventInfo.memInfo.blockSize = blockSize;
        event.eventInfo.memInfo.blockStride = blockStride;
        event.eventInfo.memInfo.repeatTimes = repeatTimes;
        event.eventInfo.memInfo.repeatStride = repeatStride;
    };

    event.type = EventType::MEM_EVENT;
    event.loc.blockType = BlockType::AIVEC;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::GM;
    fillSanEvent(0, addr, 4U, unitMemLen * 5, 4U, 1U, 1U, PipeType::PIPE_MTE3);
    alg.Do(event);
    fillSanEvent(1, addr, 0U, unitMemLen * 5, 4U, 1U, 1U, PipeType::PIPE_V);
    alg.Do(event);
    fillSanEvent(2, addr, 4U, 0, 4U, 1U, 1U, PipeType::PIPE_MTE1);
    alg.Do(event);
    fillSanEvent(3, addr, 4U, unitMemLen * 5, 4U, 0U, 1U, PipeType::PIPE_MTE1);
    alg.Do(event);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 0U);
}

TEST(CrossCoreRaceAlgImpl, cross_core_race_alg_can_detect_sync_race_events_expect_success)
{
    CrossCoreRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 4);
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
    event.type = EventType::CROSS_CORE_SYNC_EVENT;
    event.pipe = PipeType::PIPE_M;
    event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
    alg.Do(event);
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE1;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    alg.Do(event);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 0U);
}

TEST(CrossCoreRaceAlgImpl, cross_core_race_alg_can_detect_invalid_param_events_expect_no_dump)
{
    CrossCoreRaceAlgImpl alg(KernelType::MIX, DeviceType::ASCEND_910B1, 3);
    SanEvent event;
    event.serialNo = 1447;
    event.loc.blockType = BlockType::AICUBE;
    event.loc.coreId = 2052;
    event.isAtomicMode = false;
    event.loc.fileNo = -1;
    event.loc.lineNo = 4503603922403329;
    event.loc.pc = 0x1fed43400ec000;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.memInfo.memType = MemType::INVALID;
    event.eventInfo.memInfo.vectorMask = {static_cast<uint64_t>(-1), static_cast<uint64_t>(-1)};
    event.eventInfo.memInfo.maskMode = MaskMode::MASK_NORM;
    event.eventInfo.memInfo.dataBits = 8;

    event.eventInfo.memInfo.opType = AccessType::READ;
    event.eventInfo.memInfo.addr = 0x12c0c00784ec;
    event.eventInfo.memInfo.blockSize = -1;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;

    ASSERT_NO_THROW(alg.Do(event));
}

TEST(CrossCoreRaceAlgImpl, mstx_cross_core_race_alg_no_sync_race_events_expect_success)
{
    CrossCoreRaceAlgImpl alg(KernelType::MIX, DeviceType::ASCEND_910B1, 4);
    SanEvent event;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE3;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.loc.coreId = 0;
    event.loc.blockType = BlockType::AIVEC;
    alg.Do(event);

    event.type = EventType::MSTX_CROSS_SYNC_EVENT;
    event.eventInfo.mstxCrossInfo.opType = SyncType::MSTX_SET_CROSS;
    alg.Do(event);

    event.loc.coreId = 0;
    event.loc.blockType = BlockType::AICUBE;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.mstxCrossInfo.opType = SyncType::MSTX_WAIT_CROSS;
    alg.Do(event);

    event.type = EventType::MEM_EVENT;
    alg.Do(event);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 0U);
}

TEST(CrossCoreRaceAlgImpl, mstx_cross_core_race_alg_with_set_pipe_s_no_sync_race_events_expect_success)
{
    CrossCoreRaceAlgImpl alg(KernelType::MIX, DeviceType::ASCEND_910B1, 4);
    SanEvent event;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.loc.coreId = 1;
    event.loc.blockType = BlockType::AIVEC;
    alg.Do(event);

    event.type = EventType::MSTX_CROSS_SYNC_EVENT;
    event.eventInfo.mstxCrossInfo.opType = SyncType::MSTX_SET_CROSS;
    alg.Do(event);

    event.loc.coreId = 0;
    event.loc.blockType = BlockType::AICUBE;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.mstxCrossInfo.opType = SyncType::MSTX_WAIT_CROSS;
    alg.Do(event);

    event.type = EventType::MEM_EVENT;
    alg.Do(event);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 0U);
}

TEST(CrossCoreRaceAlgImpl, mstx_cross_core_race_alg_with_set_error_pipe_can_detect_sync_race_events_expect_success)
{
    CrossCoreRaceAlgImpl alg(KernelType::MIX, DeviceType::ASCEND_910B1, 4);
    SanEvent event;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE3;
    event.eventInfo.memInfo.opType = AccessType::READ;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 2U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 3U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.loc.coreId = 1;
    event.loc.blockType = BlockType::AICUBE;
    alg.Do(event);

    event.type = EventType::MSTX_CROSS_SYNC_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.mstxCrossInfo.opType = SyncType::MSTX_SET_CROSS;
    alg.Do(event);

    event.loc.coreId = 2;
    event.loc.blockType = BlockType::AIVEC;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.mstxCrossInfo.opType = SyncType::MSTX_WAIT_CROSS;
    alg.Do(event);

    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.type = EventType::MEM_EVENT;
    alg.Do(event);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 1U);
}

TEST(CrossCoreRaceAlgImpl, mstx_cross_core_race_alg_with_wait_error_pipe_can_detect_sync_race_events_expect_success)
{
    CrossCoreRaceAlgImpl alg(KernelType::MIX, DeviceType::ASCEND_910B1, 4);
    SanEvent event;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE3;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.loc.coreId = 1;
    event.loc.blockType = BlockType::AICUBE;
    alg.Do(event);

    event.type = EventType::MSTX_CROSS_SYNC_EVENT;
    event.eventInfo.mstxCrossInfo.opType = SyncType::MSTX_SET_CROSS;
    alg.Do(event);

    event.loc.coreId = 3;
    event.loc.blockType = BlockType::AIVEC;
    event.pipe = PipeType::PIPE_MTE2;
    event.eventInfo.mstxCrossInfo.opType = SyncType::MSTX_WAIT_CROSS;
    alg.Do(event);

    event.pipe = PipeType::PIPE_V;
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo.opType = AccessType::READ;
    alg.Do(event);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 1U);
}

// 反向用例
TEST(CrossCoreRaceAlgImpl, cross_core_race_alg_with_ffts_mode2_1_ratio_2_can_detect_aiv_aic_race_events)
{
    for (uint64_t i = 0; i < RandUint(20, 20); ++i) {
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        CrossCoreRaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        event.loc.coreId = RandUint(0, blockDim * 2 - 1);
        event.loc.blockType = BlockType::AIVEC;
        alg.Do(event);
        PipeType fftsPipe = RandPipe();
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = fftsPipe;
        event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
        event.eventInfo.fftsSyncInfo.dstPipe = fftsPipe;
        event.eventInfo.fftsSyncInfo.mode = 2;
        event.eventInfo.fftsSyncInfo.vecSubBlockDim = 2;
        alg.Do(event);
        event.loc.blockType = BlockType::AICUBE;
        event.loc.coreId = event.loc.coreId / C220_MIX_SUB_BLOCKDIM;
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
        alg.Do(event);
        event.type = EventType::MEM_EVENT;
        event.pipe = RandPipe();
        AssignNewAICCoreIdEvent(blockDim, event);
        event.loc.blockType = BlockType::AICUBE;
        event.eventInfo.memInfo.memType = MemType::GM;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
            static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        uint32_t raceCount = event.eventInfo.memInfo.opType == AccessType::MEMCPY_BLOCKS ? 2 : 1;
        alg.WaitFinished();
        ASSERT_EQ(alg.GetRaceCount(), raceCount);
    }
}

TEST(CrossCoreRaceAlgImpl, cross_core_race_alg_with_ffts_mode2_1_ratio_2_can_detect_aic_aic_race_events)
{
    for (uint64_t i = 0; i < RandUint(20, 20); ++i) {
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        CrossCoreRaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        event.loc.coreId = RandUint(0, blockDim - 1);
        event.loc.blockType = BlockType::AICUBE;
        alg.Do(event);
        PipeType fftsPipe = RandPipe();
        event.loc.blockType = BlockType::AIVEC;
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = fftsPipe;
        event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
        event.eventInfo.fftsSyncInfo.dstPipe = fftsPipe;
        event.eventInfo.fftsSyncInfo.mode = 2;
        event.eventInfo.fftsSyncInfo.vecSubBlockDim = 2;
        alg.Do(event);
        event.loc.blockType = BlockType::AICUBE;
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
        alg.Do(event);
        event.type = EventType::MEM_EVENT;
        event.pipe = RandPipe();
        AssignNewAICCoreIdEvent(blockDim, event);
        event.loc.blockType = BlockType::AICUBE;
        event.eventInfo.memInfo.memType = MemType::GM;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
            static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        uint32_t raceCount = event.eventInfo.memInfo.opType == AccessType::MEMCPY_BLOCKS ? 2 : 1;
        alg.WaitFinished();
        ASSERT_EQ(alg.GetRaceCount(), raceCount);
    }
}

TEST(CrossCoreRaceAlgImpl, cross_core_race_alg_with_ffts_mode2_1_ratio_2_can_detect_aiv_aiv_race_events)
{
    for (uint64_t i = 0; i < RandUint(20, 20); ++i) {
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        CrossCoreRaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        uint32_t memCoreId = RandUint(0, blockDim * 2 - 1);
        event.loc.coreId = memCoreId;
        event.loc.blockType = BlockType::AIVEC;
        alg.Do(event);
        PipeType fftsPipe = RandPipe();
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = fftsPipe;
        event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
        event.eventInfo.fftsSyncInfo.dstPipe = fftsPipe;
        event.eventInfo.fftsSyncInfo.mode = 2;
        event.eventInfo.fftsSyncInfo.vecSubBlockDim = 2;
        alg.Do(event);
        event.loc.blockType = BlockType::AICUBE;
        event.loc.coreId = event.loc.coreId / C220_MIX_SUB_BLOCKDIM;
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
        alg.Do(event);
        event.type = EventType::MEM_EVENT;
        event.pipe = RandPipe();
        event.loc.coreId = memCoreId;
        AssignNewAIVCoreIdEvent(blockDim, event);
        event.loc.blockType = BlockType::AIVEC;
        event.eventInfo.memInfo.memType = MemType::GM;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
            static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        uint32_t raceCount = event.eventInfo.memInfo.opType == AccessType::MEMCPY_BLOCKS ? 2 : 1;
        alg.WaitFinished();
        ASSERT_EQ(alg.GetRaceCount(), raceCount);
    }
}

TEST(CrossCoreRaceAlgImpl, cross_core_race_alg_with_ffts_mode2_1_ratio_1_can_detect_race_events)
{
    for (uint64_t i = 0; i < RandUint(20, 20); ++i) {
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        CrossCoreRaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        event.loc.coreId = RandUint(0, blockDim - 1);
        event.loc.blockType = static_cast<BlockType>(RandUint(static_cast<uint64_t>(BlockType::AIVEC),
                         static_cast<uint64_t>(BlockType::AICUBE)));
        alg.Do(event);
        auto fftsBlockType = static_cast<BlockType>(RandUint(static_cast<uint64_t>(BlockType::AIVEC),
                         static_cast<uint64_t>(BlockType::AICUBE)));
        event.loc.blockType = fftsBlockType;
        PipeType fftsPipe = RandPipe();
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = fftsPipe;
        event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
        event.eventInfo.fftsSyncInfo.dstPipe = fftsPipe;
        event.eventInfo.fftsSyncInfo.mode = 2;
        event.eventInfo.fftsSyncInfo.vecSubBlockDim = 1;
        alg.Do(event);
        // 反转fftsBlockType
        event.loc.blockType = fftsBlockType == BlockType::AICUBE ? BlockType::AIVEC : BlockType::AICUBE;
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
        alg.Do(event);
        event.type = EventType::MEM_EVENT;
        event.pipe = RandPipe();
        AssignNewAICCoreIdEvent(blockDim, event);
        event.eventInfo.memInfo.memType = MemType::GM;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
            static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        uint32_t raceCount = event.eventInfo.memInfo.opType == AccessType::MEMCPY_BLOCKS ? 2 : 1;
        alg.WaitFinished();
        ASSERT_EQ(alg.GetRaceCount(), raceCount);
    }
}

TEST(CrossCoreRaceAlgImpl, cross_core_race_alg_with_ffts_mode0_can_detect_aic_aiv_race_events)
{
    for (uint64_t i = 0; i < RandUint(20, 20); ++i) {
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        CrossCoreRaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        auto memBlockType = static_cast<BlockType>(RandUint(static_cast<uint64_t>(BlockType::AIVEC),
                            static_cast<uint64_t>(BlockType::AICUBE)));
        event.loc.blockType = memBlockType;
        event.loc.coreId = memBlockType == BlockType::AIVEC ? RandUint(0, blockDim * 2 - 1) : RandUint(0, blockDim - 1);
        alg.Do(event);
        PipeType fftsPipe = RandPipe();
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = fftsPipe;
        event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
        event.eventInfo.fftsSyncInfo.dstPipe = fftsPipe;
        event.eventInfo.fftsSyncInfo.mode = 0;
        event.eventInfo.fftsSyncInfo.vecSubBlockDim = RandUint(1, 2);
        alg.Do(event);
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
        alg.Do(event);
        event.type = EventType::MEM_EVENT;
        event.pipe = RandPipe();
        // 反转fftsBlockType
        event.loc.blockType = memBlockType == BlockType::AICUBE ? BlockType::AIVEC : BlockType::AICUBE;
        event.loc.coreId = event.loc.blockType == BlockType::AIVEC ? RandUint(0, blockDim * 2 - 1) :
            RandUint(0, blockDim - 1);
        event.eventInfo.memInfo.memType = MemType::GM;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
            static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        uint32_t raceCount = event.eventInfo.memInfo.opType == AccessType::MEMCPY_BLOCKS ? 2 : 1;
        alg.WaitFinished();
        ASSERT_EQ(alg.GetRaceCount(), raceCount);
    }
}

TEST(CrossCoreRaceAlgImpl, cross_core_race_alg_with_ffts_mode0_can_detect_aiv_aiv_or_aic_aic_race_events)
{
    for (uint64_t i = 0; i < RandUint(20, 20); ++i) {
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        CrossCoreRaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        auto memBlockType = static_cast<BlockType>(RandUint(static_cast<uint64_t>(BlockType::AIVEC),
                            static_cast<uint64_t>(BlockType::AICUBE)));
        event.loc.blockType = memBlockType;
        uint32_t coreId = memBlockType == BlockType::AIVEC ? RandUint(0, blockDim * 2 - 1) : RandUint(0, blockDim - 1);
        event.loc.coreId = coreId;
        alg.Do(event);
        event.loc.blockType = memBlockType == BlockType::AICUBE ? BlockType::AIVEC : BlockType::AICUBE;
        event.loc.coreId = event.loc.blockType == BlockType::AIVEC ? RandUint(0, blockDim * 2 - 1) :
            RandUint(0, blockDim - 1);
        PipeType fftsPipe = RandPipe();
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = fftsPipe;
        event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
        event.eventInfo.fftsSyncInfo.dstPipe = fftsPipe;
        event.eventInfo.fftsSyncInfo.mode = 0;
        event.eventInfo.fftsSyncInfo.vecSubBlockDim = RandUint(1, 2);
        alg.Do(event);
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
        alg.Do(event);
        event.type = EventType::MEM_EVENT;
        event.pipe = RandPipe();
        event.loc.blockType = memBlockType;
        event.loc.coreId = coreId;
        AssignNewAICAIVCoreIdEvent(blockDim, event);
        event.eventInfo.memInfo.memType = MemType::GM;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
            static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        uint32_t raceCount = event.eventInfo.memInfo.opType == AccessType::MEMCPY_BLOCKS ? 2 : 1;
        alg.WaitFinished();
        ASSERT_EQ(alg.GetRaceCount(), raceCount);
    }
}

TEST(CrossCoreRaceAlgImpl, cross_core_race_alg_with_ffts_mode1_can_detect_aic_aiv_race_events)
{
    for (uint64_t i = 0; i < RandUint(20, 20); ++i) {
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        CrossCoreRaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        auto memBlockType = static_cast<BlockType>(RandUint(static_cast<uint64_t>(BlockType::AIVEC),
                            static_cast<uint64_t>(BlockType::AICUBE)));
        event.loc.blockType = memBlockType;
        event.loc.coreId = memBlockType == BlockType::AIVEC ? RandUint(0, blockDim * 2 - 1) : RandUint(0, blockDim - 1);
        alg.Do(event);
        PipeType fftsPipe = RandPipe();
        event.loc.blockType = BlockType::AIVEC;
        event.loc.coreId = event.loc.blockType == BlockType::AIVEC ? RandUint(0, blockDim * 2 - 1) :
            RandUint(0, blockDim - 1);
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = fftsPipe;
        event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
        event.eventInfo.fftsSyncInfo.dstPipe = fftsPipe;
        event.eventInfo.fftsSyncInfo.mode = 1;
        event.eventInfo.fftsSyncInfo.vecSubBlockDim = 2;
        alg.Do(event);
        event.loc.coreId = event.loc.coreId % C220_VEC_SUB_BLOCKDIM == 0 ? ++event.loc.coreId : --event.loc.coreId;
        alg.Do(event);
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
        alg.Do(event);
        event.loc.coreId = event.loc.coreId % C220_VEC_SUB_BLOCKDIM == 0 ? ++event.loc.coreId : --event.loc.coreId;
        alg.Do(event);
        event.type = EventType::MEM_EVENT;
        event.pipe = RandPipe();
        // 反转fftsBlockType
        event.loc.blockType = memBlockType == BlockType::AICUBE ? BlockType::AIVEC : BlockType::AICUBE;
        event.loc.coreId = event.loc.blockType == BlockType::AIVEC ? RandUint(0, blockDim * 2 - 1) :
            RandUint(0, blockDim - 1);
        event.eventInfo.memInfo.memType = MemType::GM;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
            static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        event.eventInfo.memInfo.opType = AccessType::READ;
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        uint32_t raceCount = event.eventInfo.memInfo.opType == AccessType::MEMCPY_BLOCKS ? 2 : 1;
        alg.WaitFinished();
        ASSERT_EQ(alg.GetRaceCount(), raceCount);
    }
}

// 正向用例
TEST(CrossCoreRaceAlgImpl, cross_core_race_alg_with_ffts_mode2_1_ratio_2_aiv_set_aic_wait_expect_success)
{
    for (uint64_t i = 0; i < RandUint(20, 20); ++i) {
        PipeType pipe = RandPipe();
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        CrossCoreRaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        event.loc.coreId = RandUint(0, blockDim * 2 - 1);
        event.loc.blockType = BlockType::AIVEC;
        event.pipe = pipe;
        alg.Do(event);
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
        event.eventInfo.fftsSyncInfo.dstPipe = pipe;
        event.eventInfo.fftsSyncInfo.mode = 2;
        event.eventInfo.fftsSyncInfo.vecSubBlockDim = 2;
        alg.Do(event);
        event.loc.coreId = event.loc.coreId % C220_VEC_SUB_BLOCKDIM == 0 ? ++event.loc.coreId : --event.loc.coreId;
        alg.Do(event);
        event.loc.blockType = BlockType::AICUBE;
        event.loc.coreId = event.loc.coreId / C220_MIX_SUB_BLOCKDIM;
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
        alg.Do(event);
        event.type = EventType::MEM_EVENT;
        event.pipe = pipe;
        event.loc.blockType = BlockType::AICUBE;
        event.eventInfo.memInfo.memType = MemType::GM;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
            static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        alg.WaitFinished();
        ASSERT_EQ(alg.GetRaceCount(), 0);
    }
}

TEST(CrossCoreRaceAlgImpl, cross_core_race_alg_with_ffts_mode2_1_ratio_2_aic_set_aiv_wait_expect_success)
{
    for (uint64_t i = 0; i < RandUint(20, 20); ++i) {
        PipeType pipe = RandPipe();
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        CrossCoreRaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        event.loc.coreId = RandUint(0, blockDim - 1);
        event.loc.blockType = BlockType::AICUBE;
        event.pipe = pipe;
        alg.Do(event);
        event.loc.blockType = BlockType::AICUBE;
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = pipe;
        event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
        event.eventInfo.fftsSyncInfo.dstPipe = pipe;
        event.eventInfo.fftsSyncInfo.mode = 2;
        event.eventInfo.fftsSyncInfo.vecSubBlockDim = 2;
        alg.Do(event);
        event.loc.blockType = BlockType::AIVEC;
        event.loc.coreId = event.loc.coreId * C220_VEC_SUB_BLOCKDIM;
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
        alg.Do(event);
        event.loc.coreId = event.loc.coreId % C220_VEC_SUB_BLOCKDIM == 0 ? ++event.loc.coreId : --event.loc.coreId;
        alg.Do(event);
        event.type = EventType::MEM_EVENT;
        event.pipe = pipe;
        event.eventInfo.memInfo.memType = MemType::GM;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
            static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        alg.WaitFinished();
        ASSERT_EQ(alg.GetRaceCount(), 0);
    }
}

TEST(CrossCoreRaceAlgImpl, cross_core_race_alg_with_ffts_mode2_1_ratio_1_expect_success)
{
    for (uint64_t i = 0; i < RandUint(20, 20); ++i) {
        PipeType pipe = RandPipe();
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        CrossCoreRaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        event.loc.coreId = RandUint(0, blockDim - 1);
        event.loc.blockType = static_cast<BlockType>(RandUint(static_cast<uint64_t>(BlockType::AIVEC),
                         static_cast<uint64_t>(BlockType::AICUBE)));
        event.pipe = pipe;
        alg.Do(event);
        auto fftsBlockType = static_cast<BlockType>(RandUint(static_cast<uint64_t>(BlockType::AIVEC),
                         static_cast<uint64_t>(BlockType::AICUBE)));
        event.loc.blockType = fftsBlockType;
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = pipe;
        event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
        event.eventInfo.fftsSyncInfo.dstPipe = pipe;
        event.eventInfo.fftsSyncInfo.mode = 2;
        event.eventInfo.fftsSyncInfo.vecSubBlockDim = 1;
        alg.Do(event);
        // 反转fftsBlockType
        event.loc.blockType = fftsBlockType == BlockType::AICUBE ? BlockType::AIVEC : BlockType::AICUBE;
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
        alg.Do(event);
        event.type = EventType::MEM_EVENT;
        event.pipe = pipe;
        event.eventInfo.memInfo.memType = MemType::GM;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
            static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        alg.WaitFinished();
        ASSERT_EQ(alg.GetRaceCount(), 0);
    }
}

TEST(CrossCoreRaceAlgImpl, cross_core_race_alg_with_ffts_mode0_expect_success)
{
    for (uint64_t i = 0; i < RandUint(20, 20); ++i) {
        PipeType pipe = RandPipe();
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        CrossCoreRaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        auto memBlockType = static_cast<BlockType>(RandUint(static_cast<uint64_t>(BlockType::AIVEC),
                            static_cast<uint64_t>(BlockType::AICUBE)));
        event.loc.blockType = memBlockType;
        event.loc.coreId = memBlockType == BlockType::AIVEC ? RandUint(0, blockDim * 2 - 1) : RandUint(0, blockDim - 1);
        event.pipe = pipe;
        alg.Do(event);
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = pipe;
        event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
        event.eventInfo.fftsSyncInfo.dstPipe = pipe;
        event.eventInfo.fftsSyncInfo.mode = 0;
        event.eventInfo.fftsSyncInfo.vecSubBlockDim = RandUint(1, 2);
        alg.Do(event);
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
        alg.Do(event);
        event.type = EventType::MEM_EVENT;
        event.pipe = pipe;
        // 反转fftsBlockType
        event.eventInfo.memInfo.memType = MemType::GM;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
            static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        alg.WaitFinished();
        ASSERT_EQ(alg.GetRaceCount(), 0);
    }
}

TEST(CrossCoreRaceAlgImpl, cross_core_race_alg_with_ffts_mode1_expect_success)
{
    for (uint64_t i = 0; i < RandUint(20, 20); ++i) {
        PipeType pipe = RandPipe();
        uint64_t blockDim = RandUint(2, 40);
        DeviceType deviceType = static_cast<DeviceType>(RandUint(static_cast<uint64_t>(DeviceType::ASCEND_910B1),
                                static_cast<uint64_t>(DeviceType::ASCEND_910B4)));
        CrossCoreRaceAlgImpl alg(KernelType::MIX, deviceType, blockDim);
        SanEvent event = CreateMemEvent();
        event.loc.blockType = BlockType::AIVEC;
        event.pipe = pipe;
        event.loc.coreId = RandUint(0, blockDim * 2 - 1);
        alg.Do(event);
        event.loc.blockType = BlockType::AIVEC;
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
        event.eventInfo.fftsSyncInfo.dstPipe = pipe;
        event.eventInfo.fftsSyncInfo.mode = 1;
        event.eventInfo.fftsSyncInfo.vecSubBlockDim = 2;
        alg.Do(event);
        event.loc.coreId = event.loc.coreId % C220_VEC_SUB_BLOCKDIM == 0 ? ++event.loc.coreId : --event.loc.coreId;
        alg.Do(event);
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.pipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_S;
        event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
        alg.Do(event);
        event.loc.coreId = event.loc.coreId % C220_VEC_SUB_BLOCKDIM == 0 ? ++event.loc.coreId : --event.loc.coreId;
        alg.Do(event);
        event.type = EventType::MEM_EVENT;
        event.pipe = pipe;
        // 反转fftsBlockType
        event.loc.blockType = BlockType::AIVEC;
        event.eventInfo.memInfo.memType = MemType::GM;
        event.eventInfo.memInfo.opType = static_cast<AccessType>(RandUint(static_cast<uint64_t>(AccessType::READ),
            static_cast<uint64_t>(AccessType::MEMCPY_BLOCKS)));
        event.eventInfo.memInfo.opType = AccessType::READ;
        alg.Do(event);
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        alg.Do(event);
        alg.WaitFinished();
        ASSERT_EQ(alg.GetRaceCount(), 0);
    }
}

TEST(CrossCoreRaceAlgImpl, dynamic_write_event_with_dynamic_write_event_expect_race_error)
{
    CrossCoreRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 3);
    SanEvent event;
    event.loc.coreId = 0;
    event.type = EventType::DYNAMIC_MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    event.loc.blockType = BlockType::AIVEC;
    auto &dynamicOpInfo = event.eventInfo.dynamicOpInfo;
    dynamicOpInfo.memType = MemType::GM;
    dynamicOpInfo.count = 3;
    dynamicOpInfo.minAddr = 0X100;
    dynamicOpInfo.maxAddr = 0x500;
    dynamicOpInfo.dynamicType = RecordType::SIMT_ENTRY;
    std::vector<ShadowMemoryRecord> shRecords1 = GenShodowMemorys(dynamicOpInfo.minAddr, AccessType::WRITE,
        dynamicOpInfo.count, event.loc.coreId);
    dynamicOpInfo.buffer = shRecords1.data();
    alg.Do(event);
    event.loc.coreId = 1;
    dynamicOpInfo.minAddr = 0x200;
    dynamicOpInfo.count = 2;
    std::vector<ShadowMemoryRecord> shRecords2 = GenShodowMemorys(dynamicOpInfo.minAddr, AccessType::WRITE,
        dynamicOpInfo.count, event.loc.coreId);
    event.eventInfo.dynamicOpInfo.buffer = shRecords2.data();
    alg.Do(event);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 1U);
}

TEST(CrossCoreRaceAlgImpl, dynamic_write_event_with_dynamic_read_event_expect_normal)
{
    CrossCoreRaceAlgImpl alg(KernelType::AICUBE, DeviceType::ASCEND_910B3, 5);
    SanEvent event;
    event.loc.coreId = 0;
    event.type = EventType::DYNAMIC_MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    event.loc.blockType = BlockType::AICUBE;
    auto &dynamicOpInfo = event.eventInfo.dynamicOpInfo;
    dynamicOpInfo.memType = MemType::GM;
    dynamicOpInfo.count = 3;
    dynamicOpInfo.minAddr = 0X100;
    dynamicOpInfo.maxAddr = 0x500;
    dynamicOpInfo.dynamicType = RecordType::SIMT_ENTRY;
    std::vector<ShadowMemoryRecord> shRecords1 = GenShodowMemorys(dynamicOpInfo.minAddr, AccessType::WRITE,
        dynamicOpInfo.count, event.loc.coreId);
    dynamicOpInfo.buffer = shRecords1.data();
    alg.Do(event);
    event.loc.coreId = 1;
    dynamicOpInfo.minAddr = 0x400;
    dynamicOpInfo.count = 2;
    std::vector<ShadowMemoryRecord> shRecords2 = GenShodowMemorys(dynamicOpInfo.minAddr, AccessType::READ,
        dynamicOpInfo.count, event.loc.coreId);
    event.eventInfo.dynamicOpInfo.buffer = shRecords2.data();
    alg.Do(event);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 0U);
}


TEST(CrossCoreRaceAlgImpl, dynamic_write_event_with_mem_event_expect_race_error)
{
    CrossCoreRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_950DT_950x, 3);
    SanEvent event;
    event.loc.coreId = 0;
    event.type = EventType::DYNAMIC_MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    event.loc.blockType = BlockType::AIVEC;
    auto &dynamicOpInfo = event.eventInfo.dynamicOpInfo;
    dynamicOpInfo.count = 3;
    dynamicOpInfo.memType = MemType::GM;
    dynamicOpInfo.minAddr = 0X100;
    dynamicOpInfo.maxAddr = 0x500;
    dynamicOpInfo.dynamicType = RecordType::SIMT_ENTRY;
    std::vector<ShadowMemoryRecord> shRecords1 = GenShodowMemorys(dynamicOpInfo.minAddr, AccessType::READ,
        dynamicOpInfo.count, event.loc.coreId);
    dynamicOpInfo.buffer = shRecords1.data();
    alg.Do(event);
    event.loc.coreId = 1;
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
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 1U);
}

namespace {
void FillGmMemEvent(SanEvent &event, uint32_t coreId, AccessType opType, uint64_t addr, PipeType pipe)
{
    event.type = EventType::MEM_EVENT;
    event.pipe = pipe;
    event.loc.coreId = coreId;
    event.loc.blockType = BlockType::AIVEC;
    event.eventInfo.memInfo.opType = opType;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = addr;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
}

// 核内 pipe 同步：set 挂生产者 pipe，wait 挂目的 pipe（对应 SetFlag/WaitFlag<HardEvent>）
void FillPipeSyncEvent(SanEvent &event, uint32_t coreId, PipeType srcPipe, PipeType dstPipe,
                       uint32_t eventId, SyncType opType)
{
    event.type = EventType::SYNC_EVENT;
    event.pipe = opType == SyncType::SET_FLAG ? srcPipe : dstPipe;
    event.loc.coreId = coreId;
    event.loc.blockType = BlockType::AIVEC;
    event.eventInfo.syncInfo.opType = opType;
    event.eventInfo.syncInfo.srcPipe = srcPipe;
    event.eventInfo.syncInfo.dstPipe = dstPipe;
    event.eventInfo.syncInfo.eventId = eventId;
}

void FillSignalSetEvent(SanEvent &event, uint32_t coreId, uint64_t addr, int64_t value)
{
    event.type = EventType::MSTX_SIGNAL_SET_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.loc.coreId = coreId;
    event.loc.blockType = BlockType::AIVEC;
    event.eventInfo.mstxSignalSet.addr = addr;
    event.eventInfo.mstxSignalSet.value = value;
}

void FillSignalWaitEvent(SanEvent &event, uint32_t coreId, uint64_t addr, CompareOp cmpOp, int64_t cmpValue)
{
    event.type = EventType::MSTX_SIGNAL_WAIT_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.loc.coreId = coreId;
    event.loc.blockType = BlockType::AIVEC;
    event.eventInfo.mstxSignalWait.addr = addr;
    event.eventInfo.mstxSignalWait.cmpOp = cmpOp;
    event.eventInfo.mstxSignalWait.cmpValue = cmpValue;
}

void FillFinishEvent(SanEvent &event)
{
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
}
}  // namespace

TEST(CrossCoreRaceAlgImpl, mstx_signal_no_report_can_detect_race_events)
{
    // 核对：核 0 写 GM，核 1 读同一块 GM，两者之间没有任何软同步上报 -> 判定为竞争
    CrossCoreRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent event;

    FillGmMemEvent(event, 0, AccessType::WRITE, 0x50, PipeType::PIPE_MTE3);
    alg.Do(event);
    FillGmMemEvent(event, 1, AccessType::READ, 0x50, PipeType::PIPE_MTE2);
    alg.Do(event);

    FillFinishEvent(event);
    alg.Do(event);
    ASSERT_EQ(alg.GetRaceCount(), 1U);
}

TEST(CrossCoreRaceAlgImpl, mstx_signal_set_wait_expect_no_race)
{
    // 与上个用例同一份访存序列，只在中间补软同步上报 -> 竞争消失。
    // 上报挂在 PIPE_S：生产者需先把数据 pipe 同步到 PIPE_S（SetFlag/WaitFlag），
    // 上报的向量时间才会覆盖数据写，消费者通过 Wait 一次拿到该 happens-before
    CrossCoreRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent event;
    constexpr uint32_t syncEventId = 5;
    constexpr uint64_t signalAddr = 0x200;

    FillGmMemEvent(event, 0, AccessType::WRITE, 0x50, PipeType::PIPE_MTE3);
    alg.Do(event);
    FillPipeSyncEvent(event, 0, PipeType::PIPE_MTE3, PipeType::PIPE_S, syncEventId, SyncType::SET_FLAG);
    alg.Do(event);
    FillPipeSyncEvent(event, 0, PipeType::PIPE_MTE3, PipeType::PIPE_S, syncEventId, SyncType::WAIT_FLAG);
    alg.Do(event);
    FillSignalSetEvent(event, 0, signalAddr, 1);
    alg.Do(event);

    FillSignalWaitEvent(event, 1, signalAddr, CompareOp::GE, 1);
    alg.Do(event);
    FillGmMemEvent(event, 1, AccessType::READ, 0x50, PipeType::PIPE_MTE2);
    alg.Do(event);

    FillFinishEvent(event);
    alg.Do(event);
    ASSERT_EQ(alg.GetRaceCount(), 0U);
}

TEST(CrossCoreRaceAlgImpl, mstx_signal_multi_producer_accumulate_expect_no_race)
{
    // 计数型握手：核 0 / 核 1 各自写数据并累加到同一个 flag，核 2 轮询后读两份数据。
    // Wait 取 >= 100，两种语义都能满足，唯一区别是向量时间：合并语义下核 2 同时排在两个
    // 生产者之后；若只保留最后一个生产者，核 2 读先发布者的数据仍会被误报
    CrossCoreRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 3);
    SanEvent event;
    constexpr uint64_t signalAddr = 0x200;

    FillGmMemEvent(event, 0, AccessType::WRITE, 0x50, PipeType::PIPE_S);
    alg.Do(event);
    FillSignalSetEvent(event, 0, signalAddr, 100);
    alg.Do(event);

    FillGmMemEvent(event, 1, AccessType::WRITE, 0x60, PipeType::PIPE_S);
    alg.Do(event);
    FillSignalSetEvent(event, 1, signalAddr, 200);
    alg.Do(event);

    FillSignalWaitEvent(event, 2, signalAddr, CompareOp::GE, 100);
    alg.Do(event);
    FillGmMemEvent(event, 2, AccessType::READ, 0x50, PipeType::PIPE_S);
    alg.Do(event);
    FillGmMemEvent(event, 2, AccessType::READ, 0x60, PipeType::PIPE_S);
    alg.Do(event);

    FillFinishEvent(event);
    alg.Do(event);
    ASSERT_EQ(alg.GetRaceCount(), 0U);
}

TEST(CrossCoreRaceAlgImpl, do_dynamic_mem_non_cross_core_expect_skip)
{
    CrossCoreRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent event{};
    event.type = EventType::DYNAMIC_MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    event.loc.coreId = 0;
    event.eventInfo.dynamicOpInfo.memType = MemType::UB;
    alg.Do(event);
    SUCCEED();
}

TEST(CrossCoreRaceAlgImpl, process_event_various_types_expect_no_crash)
{
    CrossCoreRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent e{};
    e.loc.coreId = 0; e.loc.blockType = BlockType::AIVEC; e.pipe = PipeType::PIPE_S;

    e.type = EventType::CROSS_CORE_SOFT_SYNC_EVENT;
    e.eventInfo.softSyncInfo.opType = SyncType::IB_SET;
    e.eventInfo.softSyncInfo.eventID = 1;
    EXPECT_EQ(alg.ProcessBlockSoftSyncEvent(e), ReturnType::PROCESS_OK);
    e.eventInfo.softSyncInfo.opType = SyncType::IB_WAIT;
    e.eventInfo.softSyncInfo.eventID = 99; e.eventInfo.softSyncInfo.waitCoreID = 0;
    EXPECT_EQ(alg.ProcessBlockSoftSyncEvent(e), ReturnType::PROCESS_STALLED);
    e.eventInfo.softSyncInfo.opType = SyncType::SYNC_ALL;
    e.eventInfo.softSyncInfo.usedCores = 2;
    EXPECT_EQ(alg.ProcessBlockSoftSyncEvent(e), ReturnType::PROCESS_STALLED);

    e.type = EventType::MSTX_SIGNAL_SET_EVENT;
    e.eventInfo.mstxSignalSet.addr = 0x200; e.eventInfo.mstxSignalSet.value = 1;
    EXPECT_EQ(alg.ProcessMstxSignalSetEvent(e), ReturnType::PROCESS_OK);

    e.type = EventType::MSTX_SIGNAL_WAIT_EVENT;
    e.eventInfo.mstxSignalWait.addr = 0x999;
    e.eventInfo.mstxSignalWait.cmpOp = CompareOp::GE;
    e.eventInfo.mstxSignalWait.cmpValue = 1;
    EXPECT_EQ(alg.ProcessMstxSignalWaitEvent(e), ReturnType::PROCESS_STALLED);

    e.type = EventType::BUF_SYNC_EVENT;
    e.eventInfo.bufSyncInfo.opType = SyncType::RLS_BUF;
    e.eventInfo.bufSyncInfo.bufId = 1; e.eventInfo.bufSyncInfo.mode = BufMode::BLOCK_MODE;
    EXPECT_EQ(alg.ProcessGetRlsBufSyncEvent(e, RaceCheckType::CROSS_BLOCK_CHECK), ReturnType::PROCESS_OK);

    e.type = EventType::MSTX_CROSS_CORE_BARRIER;
    e.eventInfo.mstxCrossCoreBarrier.usedCoreNum = 2;
    e.eventInfo.mstxCrossCoreBarrier.isAIVOnly = false;
    EXPECT_EQ(alg.ProcessMstxCrossCoreBarrier(e), ReturnType::PROCESS_STALLED);

    e.type = EventType::MSTX_CROSS_NPU_BARRIER;
    e.eventInfo.mstxCrossNpuBarrier.usedCoreNum = 2;
    e.eventInfo.mstxCrossNpuBarrier.isAIVOnly = false;
    EXPECT_EQ(alg.ProcessMstxCrossNpuBarrier(e), ReturnType::PROCESS_STALLED);

    e.type = EventType::SYNC_EVENT;
    e.eventInfo.syncInfo.opType = SyncType::PIPE_BARRIER;
    EXPECT_EQ(alg.ProcessSyncEvent(e), ReturnType::PROCESS_OK);
}

}
