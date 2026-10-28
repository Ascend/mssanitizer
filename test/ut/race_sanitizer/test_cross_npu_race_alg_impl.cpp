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

#include <gtest/gtest.h>

#define private public
#include "alg_framework/cross_npu_race_alg_impl.h"
#undef private
#include "core/framework/device_manager.h"
#include "core/framework/event_def.h"
#include "core/framework/kernel_manager.h"
#include "core/framework/record_defs.h"

using namespace Sanitizer;

namespace SanitizerTest {

class TestCrossNpuRaceAlgImpl : public testing::Test {
public:
    void SetUp() override
    {
        DeviceManager::Instance().Clear();
        KernelManager::Instance().Clear();

        DeviceInfoSummary deviceInfo{};
        deviceInfo.deviceId = 0;
        deviceInfo.device = DeviceType::ASCEND_910B1;
        DeviceManager::Instance().Set(deviceInfo.deviceId, deviceInfo);
        deviceInfo.deviceId = 1;
        DeviceManager::Instance().Set(deviceInfo.deviceId, deviceInfo);

        KernelSummary kernelSummary{};
        kernelSummary.blockDim = 8;
        kernelSummary.kernelType = KernelType::MIX;
        KernelManager::Instance().Add(0, kernelSummary);
        KernelManager::Instance().Add(1, kernelSummary);
    }
    void TearDown() override
    {
        DeviceManager::Instance().Clear();
        KernelManager::Instance().Clear();
    }
};

TEST_F(TestCrossNpuRaceAlgImpl, handle_mem_event_not_on_gm_expect_return_no_race)
{
    CrossNpuRaceAlgImpl alg;
    alg.Init();

    SanEvent event;
    event.loc.deviceId = 0;
    event.loc.deviceIdx = 0;
    event.loc.coreId = 0;
    event.loc.blockType = BlockType::AIVEC;
    event.loc.kernelIdx = 0;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE2;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    alg.Do(event);

    event.loc.deviceId = 1;
    event.loc.deviceIdx = 1;
    alg.Do(event);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::FINISH;
    alg.Do(event);

    ASSERT_EQ(alg.GetResult()->size(), 0U);
}

TEST_F(TestCrossNpuRaceAlgImpl, handle_mem_event_on_gm_expect_return_race)
{
    CrossNpuRaceAlgImpl alg;
    alg.Init();

    SanEvent event;
    event.loc.deviceId = 0;
    event.loc.deviceIdx = 0;
    event.loc.coreId = 0;
    event.loc.blockType = BlockType::AIVEC;
    event.loc.kernelIdx = 0;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_MTE2;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    alg.Do(event);

    event.loc.deviceId = 1;
    event.loc.deviceIdx = 1;
    alg.Do(event);

    DeviceManager::Instance().GetSharedMemorySpans(0).Union({0x50, 0x51});
    DeviceManager::Instance().GetSharedMemorySpans(1).Union({0x50, 0x51});

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::FINISH;
    alg.Do(event);

    ASSERT_EQ(alg.GetResult()->size(), 1U);
}

TEST_F(TestCrossNpuRaceAlgImpl, process_various_event_types_expect_no_crash)
{
    CrossNpuRaceAlgImpl alg;
    alg.Init();
    SanEvent e{};
    e.loc.deviceId = 0; e.loc.deviceIdx = 0; e.loc.kernelIdx = 0;
    e.loc.coreId = 0; e.loc.blockType = BlockType::AIVEC; e.pipe = PipeType::PIPE_S;

    e.type = EventType::MEM_EVENT;
    e.eventInfo.memInfo.memType = MemType::UB;
    EXPECT_EQ(alg.ProcessMemEvent(e), ReturnType::PROCESS_OK);
    e.eventInfo.memInfo.memType = MemType::GM;
    e.eventInfo.memInfo.addr = 0x1000;
    e.eventInfo.memInfo.blockNum = 1; e.eventInfo.memInfo.blockSize = 1;
    e.eventInfo.memInfo.blockStride = 1; e.eventInfo.memInfo.repeatTimes = 1;
    e.eventInfo.memInfo.repeatStride = 1;
    EXPECT_EQ(alg.ProcessMemEvent(e), ReturnType::PROCESS_OK);

    e.type = EventType::SYNC_EVENT;
    e.eventInfo.syncInfo.opType = SyncType::SET_FLAG;
    e.eventInfo.syncInfo.srcPipe = PipeType::PIPE_M;
    e.eventInfo.syncInfo.dstPipe = PipeType::PIPE_S;
    e.eventInfo.syncInfo.eventId = 1;
    EXPECT_EQ(alg.ProcessSyncEvent(e), ReturnType::PROCESS_OK);
    e.eventInfo.syncInfo.opType = SyncType::WAIT_FLAG;
    e.eventInfo.syncInfo.eventId = 99;
    EXPECT_EQ(alg.ProcessSyncEvent(e), ReturnType::PROCESS_STALLED);
    e.eventInfo.syncInfo.opType = SyncType::PIPE_BARRIER;
    EXPECT_EQ(alg.ProcessSyncEvent(e), ReturnType::PROCESS_OK);

    e.type = EventType::TIME_EVENT;
    EXPECT_EQ(alg.ProcessTimeEvent(e), ReturnType::PROCESS_OK);

    e.type = EventType::CROSS_CORE_SOFT_SYNC_EVENT;
    e.eventInfo.softSyncInfo.opType = SyncType::IB_SET;
    e.eventInfo.softSyncInfo.eventID = 1;
    EXPECT_EQ(alg.ProcessBlockSoftSyncEvent(e), ReturnType::PROCESS_OK);
    e.eventInfo.softSyncInfo.opType = SyncType::IB_WAIT;
    e.eventInfo.softSyncInfo.eventID = 99;
    EXPECT_EQ(alg.ProcessBlockSoftSyncEvent(e), ReturnType::PROCESS_STALLED);
    e.eventInfo.softSyncInfo.opType = SyncType::SYNC_ALL;
    e.eventInfo.softSyncInfo.usedCores = 2;
    EXPECT_EQ(alg.ProcessBlockSoftSyncEvent(e), ReturnType::PROCESS_STALLED);
    e.loc.deviceId = 99;
    EXPECT_EQ(alg.ProcessBlockSoftSyncEvent(e), ReturnType::PROCESS_OK);
    e.loc.deviceId = 0;

    e.type = EventType::CROSS_CORE_SYNC_EVENT;
    e.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
    e.eventInfo.fftsSyncInfo.flagId = 1; e.eventInfo.fftsSyncInfo.mode = 0;
    e.eventInfo.fftsSyncInfo.vecSubBlockDim = 1;
    EXPECT_EQ(alg.ProcessBlockSyncEvent(e), ReturnType::PROCESS_OK);
    e.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
    EXPECT_EQ(alg.ProcessBlockSyncEvent(e), ReturnType::PROCESS_STALLED);
    e.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_INTRA_BLOCK;
    EXPECT_EQ(alg.ProcessBlockSyncEvent(e), ReturnType::PROCESS_STALLED);

    e.type = EventType::MSTX_CROSS_SYNC_EVENT;
    e.eventInfo.mstxCrossInfo.opType = SyncType::MSTX_SET_CROSS;
    e.eventInfo.mstxCrossInfo.addr = 0x200; e.eventInfo.mstxCrossInfo.flagId = 2;
    EXPECT_EQ(alg.ProcessMstxCrossSyncEvent(e), ReturnType::PROCESS_OK);
    e.eventInfo.mstxCrossInfo.opType = SyncType::MSTX_WAIT_CROSS;
    e.eventInfo.mstxCrossInfo.isMore = true;
    EXPECT_EQ(alg.ProcessMstxCrossSyncEvent(e), ReturnType::PROCESS_OK);

    e.type = EventType::MSTX_SIGNAL_SET_EVENT;
    e.eventInfo.mstxSignalSet.addr = 0x300; e.eventInfo.mstxSignalSet.value = 1;
    EXPECT_EQ(alg.ProcessMstxSignalSetEvent(e), ReturnType::PROCESS_OK);

    e.type = EventType::MSTX_SIGNAL_WAIT_EVENT;
    e.eventInfo.mstxSignalWait.addr = 0x999;
    e.eventInfo.mstxSignalWait.cmpOp = CompareOp::GE;
    e.eventInfo.mstxSignalWait.cmpValue = 1;
    EXPECT_EQ(alg.ProcessMstxSignalWaitEvent(e), ReturnType::PROCESS_STALLED);

    e.type = EventType::MSTX_CROSS_CORE_BARRIER;
    e.eventInfo.mstxCrossCoreBarrier.usedCoreNum = 2;
    e.eventInfo.mstxCrossCoreBarrier.isAIVOnly = false;
    EXPECT_EQ(alg.ProcessMstxCrossCoreBarrier(e), ReturnType::PROCESS_STALLED);

    e.type = EventType::MSTX_CROSS_NPU_BARRIER;
    e.eventInfo.mstxCrossNpuBarrier.usedDeviceNum = 1;
    e.eventInfo.mstxCrossNpuBarrier.usedCoreNum = 2;
    e.eventInfo.mstxCrossNpuBarrier.isAIVOnly = false;
    EXPECT_EQ(alg.ProcessMstxCrossNpuBarrier(e), ReturnType::PROCESS_STALLED);

    e.type = EventType::SANITIZER_CONTROL_EVENT;
    e.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    EXPECT_EQ(alg.ProcessSanitizerControlEvent(e), ReturnType::PROCESS_STALLED);
    e.eventInfo.sanitizerControlInfo.type = SanitizerControlType::FINISH;
    EXPECT_EQ(alg.ProcessSanitizerControlEvent(e), ReturnType::PROCESS_OK);

    e.type = EventType::MSTX_CROSS_SYNC_EVENT;
    e.eventInfo.mstxCrossInfo.opType = SyncType::MSTX_SET_CROSS;
    alg.CacheMstxCrossSet(e); alg.CacheMstxCrossSet(e);
    (void)alg.GetFlagIdWarnInfo();
    alg.ClearFlagIdWarnInfo();

    for (auto t : {EventType::SANITIZER_CONTROL_EVENT, EventType::SYNC_EVENT,
                   EventType::MEM_EVENT, EventType::TIME_EVENT,
                   EventType::CROSS_CORE_SYNC_EVENT, EventType::CROSS_CORE_SOFT_SYNC_EVENT,
                   EventType::MSTX_CROSS_SYNC_EVENT, EventType::MSTX_SIGNAL_SET_EVENT,
                   EventType::MSTX_SIGNAL_WAIT_EVENT, EventType::MSTX_CROSS_CORE_BARRIER,
                   EventType::MSTX_CROSS_NPU_BARRIER}) {
        e.type = t; alg.ProcessEvent(e);
    }
    e.type = EventType::SANITIZER_CONTROL_EVENT;
    e.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(e);
    SUCCEED();
}

} // namespace SanitizerTest
