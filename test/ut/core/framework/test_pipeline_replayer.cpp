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
#include <vector>

#include "pipeline_replayer.h"
#include "core/framework/event_def.h"
#include "core/framework/arch_def.h"
#include "core/framework/record_defs.h"
#include "core/framework/sync_event_data_base.h"
#include "core/framework/barrier_database.h"
#include "core/framework/barrier_database.hpp"

using namespace Sanitizer;

static uint64_t g_serialNo = 1;

class TestPipelineReplayer : public testing::Test {
protected:
    void SetUp() override {
        g_serialNo = 1;
        ResetReplayer(kDefaultBlockDim);
    }

    void RegisterCollectingCallback() {
        replayer_.RegisterCallback(
            [this](ReplayerCallbackType type, const SanEvent &event) { callbackRecords_.push_back({type, event}); });
    }

    // 清空状态并重新初始化，用于同一用例内测试多个独立子场景
    void ResetReplayer(uint32_t blockDim = kDefaultBlockDim) {
        callbackRecords_.clear();
        replayer_ = PipelineReplayer{};
        replayer_.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, blockDim);
    }

    // 断言回放卡死，且 ALL_DEVICE_STUCK 回调已触发
    void AssertStuck(const char *msg) {
        bool s = false;
        for (auto &r : callbackRecords_) {
            if (r.type == ReplayerCallbackType::ALL_DEVICE_STUCK) {
                s = true;
                break;
            }
        }
        ASSERT_TRUE(s) << msg;
    }

    // 断言无卡死
    void AssertNoStuck() {
        for (auto &r : callbackRecords_) {
            ASSERT_NE(r.type, ReplayerCallbackType::ALL_DEVICE_STUCK);
        }
    }

    // 辅助结构体，保存回调传出的所有事件方便校验
    struct CallbackRecord {
        ReplayerCallbackType type;
        SanEvent event;
    };

    static constexpr uint32_t kDefaultBlockDim = 2U;
    PipelineReplayer replayer_;
    std::vector<CallbackRecord> callbackRecords_;
};

// 辅助函数：快速构造各类 SanEvent
// 基础构造：统一设置 serialNo、type、pipe、loc 公共字段
static SanEvent MakeEventBase(EventType type, PipeType pipe, uint32_t coreId) {
    SanEvent e{};
    e.serialNo = g_serialNo++;
    e.type = type;
    e.pipe = pipe;
    e.loc.coreId = coreId;
    e.loc.blockType = BlockType::AIVEC;
    e.loc.deviceId = 0;
    return e;
}

// 各类型专用构造
static SanEvent MakeKernelFinishEvent() {
    auto e = MakeEventBase(EventType::SANITIZER_CONTROL_EVENT, PipeType::PIPE_S, 0);
    e.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    return e;
}

static SanEvent MakeMemEvent(PipeType pipe, AccessType accessType, uint32_t coreId = 0) {
    auto e = MakeEventBase(EventType::MEM_EVENT, pipe, coreId);
    e.eventInfo.memInfo.opType = accessType;
    e.eventInfo.memInfo.memType = MemType::UB;
    e.eventInfo.memInfo.addr = 0x1000;
    e.eventInfo.memInfo.blockNum = 1;
    e.eventInfo.memInfo.blockSize = 4;
    e.eventInfo.memInfo.blockStride = 0;
    e.eventInfo.memInfo.repeatTimes = 1;
    e.eventInfo.memInfo.repeatStride = 0;
    e.eventInfo.memInfo.alignSize = 0;
    e.eventInfo.memInfo.ignoreIllegalCheck = false;
    return e;
}

static SanEvent MakeTimeEvent(PipeType pipe, uint32_t coreId = 0) {
    return MakeEventBase(EventType::TIME_EVENT, pipe, coreId);
}

static SanEvent MakeDynamicMemEvent(PipeType pipe, uint32_t coreId = 0) {
    return MakeEventBase(EventType::DYNAMIC_MEM_EVENT, pipe, coreId);
}

static SanEvent MakeSyncEvent(
    PipeType pipe, PipeType srcPipe, PipeType dstPipe, uint32_t eventId, SyncType opType, uint32_t coreId = 0) {
    auto e = MakeEventBase(EventType::SYNC_EVENT, pipe, coreId);
    e.eventInfo.syncInfo.opType = opType;
    e.eventInfo.syncInfo.srcPipe = srcPipe;
    e.eventInfo.syncInfo.dstPipe = dstPipe;
    e.eventInfo.syncInfo.eventId = eventId;
    e.eventInfo.syncInfo.memType = MemType::UB;
    return e;
}

static SanEvent MakeBlockSyncEvent(PipeType pipe, uint8_t flagId, uint8_t mode, SyncType opType, uint32_t coreId = 0) {
    auto e = MakeEventBase(EventType::CROSS_CORE_SYNC_EVENT, pipe, coreId);
    e.eventInfo.fftsSyncInfo.opType = opType;
    e.eventInfo.fftsSyncInfo.flagId = flagId;
    e.eventInfo.fftsSyncInfo.mode = mode;
    e.eventInfo.fftsSyncInfo.dstPipe = PipeType::PIPE_V;
    return e;
}

static SanEvent MakeSoftSyncEvent(
    PipeType pipe, int32_t eventID, SyncType opType, uint16_t waitCoreID, int32_t usedCores, uint32_t coreId = 0) {
    auto e = MakeEventBase(EventType::CROSS_CORE_SOFT_SYNC_EVENT, pipe, coreId);
    e.eventInfo.softSyncInfo.opType = opType;
    e.eventInfo.softSyncInfo.eventID = eventID;
    e.eventInfo.softSyncInfo.waitCoreID = waitCoreID;
    e.eventInfo.softSyncInfo.usedCores = usedCores;
    return e;
}

static SanEvent MakeMstxCrossSyncEvent(
    PipeType pipe, uint64_t addr, uint64_t flagId, SyncType opType, bool isMore, uint32_t coreId = 0) {
    auto e = MakeEventBase(EventType::MSTX_CROSS_SYNC_EVENT, pipe, coreId);
    e.eventInfo.mstxCrossInfo.addr = addr;
    e.eventInfo.mstxCrossInfo.flagId = flagId;
    e.eventInfo.mstxCrossInfo.pipe = pipe;
    e.eventInfo.mstxCrossInfo.opType = opType;
    e.eventInfo.mstxCrossInfo.isMore = isMore;
    return e;
}

static SanEvent MakeMstxBarrierEvent(PipeType pipe, uint32_t usedCoreNum, uint32_t coreId, bool isAIVOnly = false) {
    auto e = MakeEventBase(EventType::MSTX_CROSS_CORE_BARRIER, pipe, coreId);
    e.eventInfo.mstxCrossCoreBarrier.usedCoreNum = usedCoreNum;
    e.eventInfo.mstxCrossCoreBarrier.isAIVOnly = isAIVOnly;
    return e;
}

static uint32_t g_npuDeviceId = 12345;
static uint32_t g_npuCoreId = 0;

static SanEvent MakeMstxNpuBarrierEvent(PipeType pipe, uint32_t usedDeviceNum, uint32_t usedCoreNum, uint32_t coreId) {
    auto e = MakeEventBase(EventType::MSTX_CROSS_NPU_BARRIER, pipe, coreId);
    e.eventInfo.mstxCrossNpuBarrier.usedDeviceNum = usedDeviceNum;
    e.eventInfo.mstxCrossNpuBarrier.usedDeviceId = &g_npuDeviceId;
    e.eventInfo.mstxCrossNpuBarrier.usedCoreNum = usedCoreNum;
    e.eventInfo.mstxCrossNpuBarrier.usedCoreId = &g_npuCoreId;
    return e;
}

static SanEvent MakeBufEvent(
    PipeType pipe, uint64_t bufId, SyncType opType, uint64_t rlsCount, BufMode mode, uint32_t coreId = 0) {
    auto e = MakeEventBase(EventType::BUF_SYNC_EVENT, pipe, coreId);
    e.eventInfo.bufSyncInfo.opType = opType;
    e.eventInfo.bufSyncInfo.pipe = pipe;
    e.eventInfo.bufSyncInfo.bufId = bufId;
    e.eventInfo.bufSyncInfo.rlsCount = rlsCount;
    e.eventInfo.bufSyncInfo.mode = mode;
    return e;
}

// case 1. 基础生命周期：Init → Do(事件) → Do(KERNEL_FINISH) → IsFinished
TEST_F(TestPipelineReplayer, basic_lifecycle) {
    ASSERT_FALSE(replayer_.IsFinished());

    // 收集阶段不结束
    replayer_.Do(MakeMemEvent(PipeType::PIPE_S, AccessType::READ));
    ASSERT_FALSE(replayer_.IsFinished());

    // KERNEL_FINISH 触发回放
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());

    // 不注册回调也应正常运行
    ResetReplayer();
    replayer_.Do(MakeMemEvent(PipeType::PIPE_S, AccessType::READ));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
}

// case 2. 非同步事件直通：MEM / TIME / DYNAMIC_MEM 均返回 PROCESS_OK 不卡死
TEST_F(TestPipelineReplayer, non_sync_events_pass_through) {
    RegisterCollectingCallback();

    replayer_.Do(MakeMemEvent(PipeType::PIPE_S, AccessType::READ));
    replayer_.Do(MakeTimeEvent(PipeType::PIPE_S));
    replayer_.Do(MakeDynamicMemEvent(PipeType::PIPE_S));
    replayer_.Do(MakeKernelFinishEvent());

    ASSERT_TRUE(replayer_.IsFinished());
    AssertNoStuck();
}

// case 3. 回调：EVENT_PROCESSING 在各类事件处理时触发
TEST_F(TestPipelineReplayer, callback_fires_during_processing) {
    RegisterCollectingCallback();

    replayer_.Do(MakeMemEvent(PipeType::PIPE_S, AccessType::READ));
    replayer_.Do(MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 1, SyncType::SET_FLAG));
    replayer_.Do(MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 1, SyncType::WAIT_FLAG));
    replayer_.Do(MakeKernelFinishEvent());

    int memCount = 0, syncCount = 0;
    for (auto &r : callbackRecords_) {
        if (r.type != ReplayerCallbackType::EVENT_PROCESSING) {
            continue;
        }
        if (r.event.type == EventType::MEM_EVENT) {
            ++memCount;
        }
        if (r.event.type == EventType::SYNC_EVENT) {
            ++syncCount;
        }
    }
    ASSERT_GE(memCount, 1) << "MEM_EVENT should trigger EVENT_PROCESSING";
    ASSERT_GE(syncCount, 2) << "SET_FLAG + WAIT_FLAG should both trigger callback";
}

// case 4. PIPE_S 路由：PIPE_S 上的事件根据 event.pipe 发射到目标 PIPE
TEST_F(TestPipelineReplayer, pipe_s_routes_to_target_pipe) {
    RegisterCollectingCallback();
    replayer_.Do(MakeMemEvent(PipeType::PIPE_V, AccessType::READ));
    replayer_.Do(MakeKernelFinishEvent());

    ASSERT_TRUE(replayer_.IsFinished());
    bool memOnPipeV = false;
    for (auto &r : callbackRecords_) {
        if (r.type == ReplayerCallbackType::EVENT_PROCESSING && r.event.type == EventType::MEM_EVENT &&
            r.event.pipe == PipeType::PIPE_V) {
            memOnPipeV = true;
            break;
        }
    }
    ASSERT_TRUE(memOnPipeV);
}

// case 5. SYNC_EVENT：SET_FLAG / WAIT_FLAG 配对成功 && 无 SET 时卡死
TEST_F(TestPipelineReplayer, sync_event_set_wait) {
    RegisterCollectingCallback();

    // 场景1：SET + WAIT 配对成功，不卡死
    replayer_.Do(MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 10, SyncType::SET_FLAG));
    replayer_.Do(MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 10, SyncType::WAIT_FLAG));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertNoStuck();

    // 场景2：仅 WAIT_FLAG 无 SET → 卡死并触发 ALL_DEVICE_STUCK
    ResetReplayer();
    RegisterCollectingCallback();

    replayer_.Do(MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 99, SyncType::WAIT_FLAG));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());

    bool stuckFired = false;
    for (auto &r : callbackRecords_) {
        if (r.type == ReplayerCallbackType::ALL_DEVICE_STUCK) {
            stuckFired = true;
            ASSERT_EQ(r.event.type, EventType::SYNC_EVENT);
            break;
        }
    }
    ASSERT_TRUE(stuckFired) << "WAIT_FLAG without SET_FLAG should cause stuck";
}

// case 6. CROSS_CORE_SYNC_EVENT：FFTS_SYNC + WAIT_FLAG_DEV / WAIT_INTRA_BLOCK
TEST_F(TestPipelineReplayer, ffts_sync_and_wait_variants) {
    RegisterCollectingCallback();

    // FFTS_SYNC 不卡死
    replayer_.Do(MakeBlockSyncEvent(PipeType::PIPE_S, 1, 0, SyncType::FFTS_SYNC, 0));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());

    // WAIT_FLAG_DEV 无 FFTS → 卡死
    ResetReplayer();
    RegisterCollectingCallback();
    replayer_.Do(MakeBlockSyncEvent(PipeType::PIPE_S, 5, 0, SyncType::WAIT_FLAG_DEV, 0));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertStuck("WAIT_FLAG_DEV without FFTS_SYNC should stall");

    // WAIT_INTRA_BLOCK 无 FFTS → 卡死
    ResetReplayer();
    RegisterCollectingCallback();
    replayer_.Do(MakeBlockSyncEvent(PipeType::PIPE_S, 3, 0, SyncType::WAIT_INTRA_BLOCK, 0));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertStuck("WAIT_INTRA_BLOCK without FFTS_SYNC should stall");
}

// case 7. CROSS_CORE_SOFT_SYNC_EVENT：IB_SET / IB_WAIT / SYNC_ALL
TEST_F(TestPipelineReplayer, soft_sync_set_wait_and_sync_all) {
    RegisterCollectingCallback();

    // IB_SET + IB_WAIT 配对成功
    replayer_.Do(MakeSoftSyncEvent(PipeType::PIPE_S, 42, SyncType::IB_SET, 0, 2, 0));
    replayer_.Do(MakeSoftSyncEvent(PipeType::PIPE_S, 42, SyncType::IB_WAIT, 0, 2, 1));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertNoStuck();

    // IB_WAIT 无 IB_SET → 卡死
    ResetReplayer();
    RegisterCollectingCallback();
    replayer_.Do(MakeSoftSyncEvent(PipeType::PIPE_S, 7, SyncType::IB_WAIT, 0, 2, 1));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertStuck("IB_WAIT without IB_SET should stall");

    // SYNC_ALL usedCores=1 单核无法满足多核同步 → 卡死
    ResetReplayer();
    RegisterCollectingCallback();
    replayer_.Do(MakeSoftSyncEvent(PipeType::PIPE_S, 0, SyncType::SYNC_ALL, 0, 1, 0));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertStuck("SYNC_ALL with single core should stall (needs all cores to arrive)");
}

// case 8. MSTX_CROSS_SYNC_EVENT：SET_CROSS / WAIT_CROSS (含 isMore 跳过)
TEST_F(TestPipelineReplayer, mstx_cross_sync_variants) {
    RegisterCollectingCallback();

    // MSTX_SET_CROSS + MSTX_WAIT_CROSS 配对成功
    replayer_.Do(MakeMstxCrossSyncEvent(PipeType::PIPE_S, 0xA000, 1, SyncType::MSTX_SET_CROSS, false, 0));
    replayer_.Do(MakeMstxCrossSyncEvent(PipeType::PIPE_S, 0xA000, 1, SyncType::MSTX_WAIT_CROSS, false, 0));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertNoStuck();

    // isMore=true 无对应 SET → 直接跳过，不卡死
    ResetReplayer();
    RegisterCollectingCallback();
    replayer_.Do(MakeMstxCrossSyncEvent(PipeType::PIPE_S, 0xB000, 2, SyncType::MSTX_WAIT_CROSS, true, 0));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertNoStuck();

    // isMore=false 无对应 SET → 卡死
    ResetReplayer();
    RegisterCollectingCallback();
    replayer_.Do(MakeMstxCrossSyncEvent(PipeType::PIPE_S, 0xC000, 3, SyncType::MSTX_WAIT_CROSS, false, 0));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertStuck("MSTX_WAIT_CROSS (!isMore) without set should stall");
}

// case 9. BARRIER：CROSS_CORE_BARRIER + NPU_BARRIER 退化
TEST_F(TestPipelineReplayer, barrier_cross_core_and_npu) {
    RegisterCollectingCallback();

    // 核间 barrier usedCoreNum=1 → 成功
    replayer_.Do(MakeMstxBarrierEvent(PipeType::PIPE_S, 1, 0));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertNoStuck();

    // 核间 barrier usedCoreNum=2 只有 coreId=0 → 卡死
    ResetReplayer();
    RegisterCollectingCallback();
    replayer_.Do(MakeMstxBarrierEvent(PipeType::PIPE_S, 2, 0));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertStuck("Barrier waiting for 2 cores with only 1 should stall");

    // NPU barrier 退化 usedCoreNum=1 → 成功
    ResetReplayer();
    RegisterCollectingCallback();
    replayer_.Do(MakeMstxNpuBarrierEvent(PipeType::PIPE_S, 1, 1, 0));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertNoStuck();

    // NPU barrier 退化 usedCoreNum=3 仅 coreId=0 → 卡死
    ResetReplayer();
    RegisterCollectingCallback();
    replayer_.Do(MakeMstxNpuBarrierEvent(PipeType::PIPE_S, 1, 3, 0));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertStuck("Decayed NPU barrier with insufficient cores should stall");
}

// case 10. BUF_SYNC_EVENT：GET_BUF / RLS_BUF
TEST_F(TestPipelineReplayer, buf_sync_get_rls_variants) {
    RegisterCollectingCallback();

    // rlsCount=0 的 GET_BUF 无阻塞
    replayer_.Do(MakeBufEvent(PipeType::PIPE_S, 100, SyncType::GET_BUF, 0, BufMode::BLOCK_MODE, 0));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertNoStuck();

    // RLS_BUF + GET_BUF(rlsCount=1) 配对成功
    ResetReplayer();
    RegisterCollectingCallback();
    replayer_.Do(MakeBufEvent(PipeType::PIPE_S, 200, SyncType::RLS_BUF, 0, BufMode::BLOCK_MODE, 0));
    replayer_.Do(MakeBufEvent(PipeType::PIPE_S, 200, SyncType::GET_BUF, 1, BufMode::BLOCK_MODE, 0));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertNoStuck();

    // GET_BUF 要求 rlsCount=2 仅 1 个 RLS → 卡死
    ResetReplayer();
    RegisterCollectingCallback();
    replayer_.Do(MakeBufEvent(PipeType::PIPE_S, 300, SyncType::RLS_BUF, 0, BufMode::BLOCK_MODE, 0));
    replayer_.Do(MakeBufEvent(PipeType::PIPE_S, 300, SyncType::GET_BUF, 2, BufMode::BLOCK_MODE, 0));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertStuck("GET_BUF with insufficient RLS_BUF should stall");

    // 连续两条 GET_BUF 耗尽隐含 buffer → 第二条卡死（同 pipe 死锁）
    ResetReplayer();
    RegisterCollectingCallback();
    replayer_.Do(MakeBufEvent(PipeType::PIPE_S, 400, SyncType::GET_BUF, 0, BufMode::BLOCK_MODE, 0));
    replayer_.Do(MakeBufEvent(PipeType::PIPE_S, 400, SyncType::GET_BUF, 0, BufMode::BLOCK_MODE, 0));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertStuck("Consecutive GET_BUF exhausts implicit buffer → stall");

    // 嵌套：get→get→rls→rls，第二条 GET 先卡死，RLS 被挡在队尾
    ResetReplayer();
    RegisterCollectingCallback();
    replayer_.Do(MakeBufEvent(PipeType::PIPE_S, 500, SyncType::GET_BUF, 0, BufMode::BLOCK_MODE, 0));
    replayer_.Do(MakeBufEvent(PipeType::PIPE_S, 500, SyncType::GET_BUF, 0, BufMode::BLOCK_MODE, 0));
    replayer_.Do(MakeBufEvent(PipeType::PIPE_S, 500, SyncType::RLS_BUF, 0, BufMode::BLOCK_MODE, 0));
    replayer_.Do(MakeBufEvent(PipeType::PIPE_S, 500, SyncType::RLS_BUF, 0, BufMode::BLOCK_MODE, 0));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertStuck("Nested GET→GET→RLS→RLS: second GET stalls, RLS behind it unreachable");
}

// case 11. 综合多事件序列 + 卡死回调准确性验证
TEST_F(TestPipelineReplayer, multiple_events_and_stuck_verify) {
    RegisterCollectingCallback();

    replayer_.Do(MakeMemEvent(PipeType::PIPE_S, AccessType::READ, 0));
    replayer_.Do(MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 1, SyncType::SET_FLAG, 0));
    replayer_.Do(MakeMemEvent(PipeType::PIPE_V, AccessType::WRITE, 0));
    replayer_.Do(MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 1, SyncType::WAIT_FLAG, 0));
    replayer_.Do(MakeTimeEvent(PipeType::PIPE_S, 0));
    replayer_.Do(MakeBufEvent(PipeType::PIPE_S, 1, SyncType::RLS_BUF, 0, BufMode::BLOCK_MODE, 0));
    replayer_.Do(MakeBufEvent(PipeType::PIPE_S, 1, SyncType::GET_BUF, 1, BufMode::BLOCK_MODE, 0));
    replayer_.Do(MakeKernelFinishEvent());

    ASSERT_TRUE(replayer_.IsFinished());
    AssertNoStuck();
    size_t pc = 0;
    for (auto &r : callbackRecords_) {
        if (r.type == ReplayerCallbackType::EVENT_PROCESSING) {
            ++pc;
        }
    }
    ASSERT_GT(pc, 3U) << "Multiple events should trigger multiple callbacks";

    // 卡死回调内容验证：应返回卡死队列的首个 SanEvent
    ResetReplayer();
    RegisterCollectingCallback();

    uint64_t stuckSerial = g_serialNo;
    replayer_.Do(MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 99, SyncType::WAIT_FLAG));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());

    const SanEvent *se = nullptr;
    for (auto &r : callbackRecords_) {
        if (r.type == ReplayerCallbackType::ALL_DEVICE_STUCK) {
            se = &r.event;
            break;
        }
    }
    ASSERT_NE(se, nullptr);
    ASSERT_EQ(se->serialNo, stuckSerial);
    ASSERT_EQ(se->type, EventType::SYNC_EVENT);
    ASSERT_EQ(se->eventInfo.syncInfo.opType, SyncType::WAIT_FLAG);
}
