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
#include "alg_framework/cross_core_race_alg_impl.h"
#include "alg_framework/race_alg_factory.h"
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
    dynamicOpInfo.dynamicType = RecordType::SHADOW_MEMORY;
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
    dynamicOpInfo.dynamicType = RecordType::SHADOW_MEMORY;
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
    dynamicOpInfo.dynamicType = RecordType::SHADOW_MEMORY;
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
    ASSERT_EQ(alg.GetRaceCount(), 1U);
}

}
