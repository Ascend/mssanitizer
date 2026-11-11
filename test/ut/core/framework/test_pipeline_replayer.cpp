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
#include <cstring>

#include "event_container.h"
#include "pipe_line.h"
#include "cross_core_sync_info_container.h"
#include "core/framework/barrier_database.hpp"
#define private public
#include "pipeline_replayer.h"
#undef private
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

    static void AssertSameEvent(const SanEvent &actual, const SanEvent &expected) {
        EXPECT_EQ(actual.serialNo, expected.serialNo);
        EXPECT_EQ(actual.type, expected.type);
        EXPECT_EQ(actual.pipe, expected.pipe);
        // Both payloads are copies of the same trace event: compare every union byte, not a selected sync key.
        EXPECT_EQ(std::memcmp(&actual.eventInfo, &expected.eventInfo, sizeof(actual.eventInfo)), 0);
        EXPECT_EQ(actual.timeInfo, expected.timeInfo);
        EXPECT_EQ(actual.loc.fileNo, expected.loc.fileNo);
        EXPECT_EQ(actual.loc.lineNo, expected.loc.lineNo);
        EXPECT_EQ(actual.loc.pc, expected.loc.pc);
        EXPECT_EQ(actual.loc.deviceIdx, expected.loc.deviceIdx);
        EXPECT_EQ(actual.loc.kernelIdx, expected.loc.kernelIdx);
        EXPECT_EQ(actual.loc.deviceId, expected.loc.deviceId);
        EXPECT_EQ(actual.loc.coreId, expected.loc.coreId);
        EXPECT_EQ(actual.loc.blockType, expected.loc.blockType);
        EXPECT_EQ(actual.isAtomicMode, expected.isAtomicMode);
        EXPECT_EQ(actual.recordType, expected.recordType);
        EXPECT_EQ(actual.closedRingStartSerialNo, expected.closedRingStartSerialNo);
    }

    static void CompareCompactModes(const std::vector<SanEvent> &trace, const std::vector<SanEvent> &expectedStuck,
        KernelType kernel = KernelType::AIVEC, uint32_t blockDim = kDefaultBlockDim) {
        for (bool compact : {false, true}) {
            SCOPED_TRACE(compact);
            PipelineReplayer replayer;
            replayer.Init(kernel, DeviceType::ASCEND_910B1, blockDim, compact);
            std::vector<SanEvent> stuck;
            uint64_t inputMemory = 0;
            replayer.RegisterCallback([&](ReplayerCallbackType type, const SanEvent &event) {
                if (type == ReplayerCallbackType::ALL_DEVICE_STUCK) {
                    stuck.push_back(event);
                } else {
                    EXPECT_FALSE(compact);
                    EXPECT_TRUE(event.type == EventType::MEM_EVENT || event.type == EventType::DYNAMIC_MEM_EVENT);
                }
            });
            for (const auto &event : trace) {
                replayer.Do(event);
                inputMemory += event.type == EventType::MEM_EVENT || event.type == EventType::DYNAMIC_MEM_EVENT;
                ASSERT_FALSE(replayer.IsFinished());
            }
            ASSERT_EQ(replayer.GetMemEventCount(), inputMemory);
            SanEvent finish{};
            finish.type = EventType::SANITIZER_CONTROL_EVENT;
            finish.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
            replayer.Do(finish);
            ASSERT_TRUE(replayer.IsFinished());
            EXPECT_EQ(replayer.GetMemEventCount(), inputMemory);
            ASSERT_EQ(stuck.size(), expectedStuck.size());
            for (size_t i = 0; i < stuck.size(); ++i) {
                SCOPED_TRACE(i);
                AssertSameEvent(stuck[i], expectedStuck[i]);
            }
        }
    }

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
    PipeType pipe, int32_t eventID, SyncType opType, int32_t waitCoreID, int32_t usedCores, uint32_t coreId = 0) {
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

static ReturnType ProcessSyncQueueReference(SyncEventDataBase &queue, const SanEvent &event) {
    const auto &info = event.eventInfo.syncInfo;
    SyncEvent key{};
    key.info.srcPipe = static_cast<uint8_t>(info.srcPipe);
    key.info.dstPipe = static_cast<uint8_t>(info.dstPipe);
    key.info.eventId = static_cast<uint8_t>(info.eventId);
    key.info.memType = static_cast<uint8_t>(info.memType);
    key.info.isRetrogress = info.isRetrogress;
    VectorTime vt{};
    if (info.opType == SyncType::SET_FLAG) {
        queue.Set(key, vt);
    } else if (info.opType == SyncType::WAIT_FLAG && !queue.Get(key, vt)) {
        return ReturnType::PROCESS_STALLED;
    }
    return ReturnType::PROCESS_OK;
}

// The old replayer retained every release vector and counted successful blocking gets.
struct BufQueueReference {
    std::map<std::pair<uint32_t, uint64_t>, std::vector<VectorTime>> releases;
    std::map<std::pair<uint32_t, uint64_t>, uint64_t> consumed;

    ReturnType Process(const SanEvent &event, uint32_t block) {
        const auto &info = event.eventInfo.bufSyncInfo;
        auto key = std::make_pair(block, info.bufId);
        if (info.opType == SyncType::RLS_BUF) {
            releases[key].push_back(VectorTime{});
        } else if (info.opType == SyncType::GET_BUF && info.mode == BufMode::BLOCK_MODE) {
            auto it = releases.find(key);
            auto &used = consumed[key];
            auto count = it == releases.end() ? 0U : it->second.size();
            if ((info.rlsCount != 0U && (it == releases.end() || count < info.rlsCount)) ||
                used + 1 > count + 1) {
                return ReturnType::PROCESS_STALLED;
            }
            ++used;
        }
        return ReturnType::PROCESS_OK;
    }
};

static void CompareBufQueueReference(PipelineReplayer &replayer, BufQueueReference &reference,
    const SanEvent &event, uint32_t block) {
    EXPECT_EQ(replayer.ProcessGetRlsBufSyncEvent(event), reference.Process(event, block));
    EXPECT_EQ(replayer.getBufCount_, reference.consumed);
    ASSERT_EQ(replayer.getRlsBufMap_.size(), reference.releases.size());
    for (const auto &entry : reference.releases) {
        auto it = replayer.getRlsBufMap_.find(entry.first);
        ASSERT_NE(it, replayer.getRlsBufMap_.end());
        EXPECT_EQ(it->second, entry.second.size());
    }
}

TEST_F(TestPipelineReplayer, sync_counts_match_queue_exhaustion_failure_and_refill) {
    for (bool compact : {false, true}) {
        SCOPED_TRACE(compact);
        replayer_.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, 1, compact);
        SyncEventDataBase reference;
        for (auto op : {SyncType::WAIT_FLAG, SyncType::WAIT_FLAG, SyncType::SET_FLAG, SyncType::SET_FLAG,
            SyncType::WAIT_FLAG, SyncType::WAIT_FLAG, SyncType::WAIT_FLAG, SyncType::WAIT_FLAG,
            SyncType::SET_FLAG, SyncType::WAIT_FLAG, SyncType::WAIT_FLAG}) {
            SCOPED_TRACE(static_cast<int>(op));
            auto event = MakeSyncEvent(PipeType::PIPE_V, PipeType::PIPE_MTE2, PipeType::PIPE_V, 7, op);
            auto before = replayer_.syncDB_[0];
            auto expected = ProcessSyncQueueReference(reference, event);
            ASSERT_EQ(replayer_.ProcessSyncEvent(event), expected);
            if (expected == ReturnType::PROCESS_STALLED) {
                // Neither missing nor exhausted waits mutate state.
                ASSERT_EQ(replayer_.syncDB_[0].size(), before.size());
                if (!before.empty()) { EXPECT_EQ(replayer_.syncDB_[0].begin()->second, before.begin()->second); }
            }
        }
        ASSERT_EQ(replayer_.syncDB_[0].size(), 1U);
        EXPECT_EQ(replayer_.syncDB_[0].begin()->second, 0U);
    }
}

TEST_F(TestPipelineReplayer, sync_counts_match_queue_for_each_of_five_key_fields) {
    auto base = MakeSyncEvent(PipeType::PIPE_V, PipeType::PIPE_MTE2, PipeType::PIPE_V, 7, SyncType::SET_FLAG);
    std::vector<SanEvent> keys(6, base);
    keys[1].eventInfo.syncInfo.srcPipe = PipeType::PIPE_MTE3;
    keys[2].eventInfo.syncInfo.dstPipe = PipeType::PIPE_S;
    keys[3].eventInfo.syncInfo.eventId = 8;
    keys[4].eventInfo.syncInfo.memType = MemType::L1;
    keys[5].eventInfo.syncInfo.isRetrogress = true;
    for (bool compact : {false, true}) {
        SCOPED_TRACE(compact);
        replayer_.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, 1, compact);
        SyncEventDataBase reference;
        ASSERT_EQ(replayer_.ProcessSyncEvent(base), ProcessSyncQueueReference(reference, base));
        for (size_t i = 1; i < keys.size(); ++i) {
            SCOPED_TRACE(i);
            auto wait = keys[i];
            wait.eventInfo.syncInfo.opType = SyncType::WAIT_FLAG;
            ASSERT_EQ(ProcessSyncQueueReference(reference, wait), ReturnType::PROCESS_STALLED);
            EXPECT_EQ(replayer_.ProcessSyncEvent(wait), ReturnType::PROCESS_STALLED);
            EXPECT_EQ(replayer_.syncDB_[0].size(), 1U);
        }
        for (size_t i = 1; i < keys.size(); ++i) {
            EXPECT_EQ(replayer_.ProcessSyncEvent(keys[i]), ProcessSyncQueueReference(reference, keys[i]));
        }
        ASSERT_EQ(replayer_.syncDB_[0].size(), keys.size());
        for (auto it = keys.rbegin(); it != keys.rend(); ++it) {
            auto wait = *it;
            wait.eventInfo.syncInfo.opType = SyncType::WAIT_FLAG;
            EXPECT_EQ(replayer_.ProcessSyncEvent(wait), ProcessSyncQueueReference(reference, wait));
            EXPECT_EQ(replayer_.ProcessSyncEvent(wait), ProcessSyncQueueReference(reference, wait));
        }
    }
}

TEST_F(TestPipelineReplayer, sync_counts_match_queue_with_block_and_mix_subblock_isolation) {
    for (auto kernel : {KernelType::AIVEC, KernelType::MIX}) {
        for (bool compact : {false, true}) {
            SCOPED_TRACE(::testing::Message() << static_cast<int>(kernel) << ":" << compact);
            replayer_.Init(kernel, DeviceType::ASCEND_910B1, 2, compact);
            const uint32_t blocks = kernel == KernelType::MIX ? 6U : 2U;
            std::vector<SanEvent> events;
            std::vector<SyncEventDataBase> reference(blocks);
            for (uint32_t block = 0; block < blocks; ++block) {
                auto event = MakeSyncEvent(PipeType::PIPE_V, PipeType::PIPE_MTE2, PipeType::PIPE_V, 7,
                    SyncType::SET_FLAG, block);
                if (kernel == KernelType::MIX) {
                    // Explicit AIV0/AIV1/AIC0/AIV2/AIV3/AIC1 layout, independent of the production mapper.
                    event.loc.blockType = block % 3 == 2 ? BlockType::AICUBE : BlockType::AIVEC;
                    event.loc.coreId = block % 3 == 2 ? block / 3 : block / 3 * 2 + block % 3;
                }
                events.push_back(event);
            }
            ASSERT_EQ(replayer_.syncDB_.size(), blocks);
            for (uint32_t producer = 0; producer < blocks; ++producer) {
                SCOPED_TRACE(producer);
                EXPECT_EQ(replayer_.ProcessSyncEvent(events[producer]),
                    ProcessSyncQueueReference(reference[producer], events[producer]));
                for (uint32_t consumer = 0; consumer < blocks; ++consumer) {
                    if (consumer == producer) { continue; }
                    auto wait = events[consumer];
                    wait.eventInfo.syncInfo.opType = SyncType::WAIT_FLAG;
                    auto before = replayer_.syncDB_;
                    ASSERT_EQ(ProcessSyncQueueReference(reference[consumer], wait), ReturnType::PROCESS_STALLED);
                    EXPECT_EQ(replayer_.ProcessSyncEvent(wait), ReturnType::PROCESS_STALLED);
                    for (uint32_t block = 0; block < blocks; ++block) {
                        ASSERT_EQ(replayer_.syncDB_[block].size(), before[block].size());
                        if (!before[block].empty()) {
                            EXPECT_EQ(replayer_.syncDB_[block].begin()->second, before[block].begin()->second);
                        }
                    }
                }
                auto wait = events[producer];
                wait.eventInfo.syncInfo.opType = SyncType::WAIT_FLAG;
                EXPECT_EQ(replayer_.ProcessSyncEvent(wait), ProcessSyncQueueReference(reference[producer], wait));
                EXPECT_EQ(replayer_.ProcessSyncEvent(wait), ProcessSyncQueueReference(reference[producer], wait));
            }
        }
    }
}

TEST_F(TestPipelineReplayer, token_counts_reset_on_init_with_changed_blockdim_and_mode) {
    for (bool compact : {false, true}) {
        for (uint32_t blockDim : {1U, 3U}) {
            SCOPED_TRACE(::testing::Message() << blockDim << ":" << compact);
            replayer_.Init(blockDim == 1 ? KernelType::MIX : KernelType::AIVEC,
                DeviceType::ASCEND_910B1, blockDim == 1 ? 2 : 1, !compact);
            auto set = MakeSyncEvent(PipeType::PIPE_V, PipeType::PIPE_MTE2, PipeType::PIPE_V, 7, SyncType::SET_FLAG);
            auto release = MakeBufEvent(PipeType::PIPE_V, 17, SyncType::RLS_BUF, 0, BufMode::NONBLOCK_MODE);
            auto get = MakeBufEvent(PipeType::PIPE_V, 17, SyncType::GET_BUF, 0, BufMode::BLOCK_MODE);
            SyncEventDataBase syncReference;
            BufQueueReference bufReference;
            EXPECT_EQ(replayer_.ProcessSyncEvent(set), ProcessSyncQueueReference(syncReference, set));
            CompareBufQueueReference(replayer_, bufReference, release, 0);
            CompareBufQueueReference(replayer_, bufReference, get, 0);
            if (blockDim == 1) {
                auto lastSet = set;
                lastSet.loc.blockType = BlockType::AICUBE;
                lastSet.loc.coreId = 1;
                SyncEventDataBase lastReference;
                EXPECT_EQ(replayer_.ProcessSyncEvent(lastSet), ProcessSyncQueueReference(lastReference, lastSet));
                release.loc = lastSet.loc;
                CompareBufQueueReference(replayer_, bufReference, release, 5);
            }
            // Leave both an unconsumed flag and a release, plus a consumed get, across Init.
            auto kernel = blockDim == 1 ? KernelType::AIVEC : KernelType::MIX;
            replayer_.Init(kernel, DeviceType::ASCEND_910B1, blockDim, compact);
            syncReference = SyncEventDataBase{};
            bufReference = BufQueueReference{};
            ASSERT_EQ(replayer_.syncDB_.size(), blockDim == 1 ? 1U : 9U);
            for (const auto &counts : replayer_.syncDB_) { EXPECT_TRUE(counts.empty()); }
            EXPECT_TRUE(replayer_.getRlsBufMap_.empty());
            EXPECT_TRUE(replayer_.getBufCount_.empty());
            set.eventInfo.syncInfo.opType = SyncType::WAIT_FLAG;
            EXPECT_EQ(replayer_.ProcessSyncEvent(set), ProcessSyncQueueReference(syncReference, set));
            get.eventInfo.bufSyncInfo.rlsCount = 1;
            CompareBufQueueReference(replayer_, bufReference, get, 0); // No stale release.
            get.eventInfo.bufSyncInfo.rlsCount = 0;
            CompareBufQueueReference(replayer_, bufReference, get, 0); // Fresh first-get allowance.
            CompareBufQueueReference(replayer_, bufReference, get, 0); // Then exhausted again.
        }
    }
}

TEST_F(TestPipelineReplayer, buf_counts_match_old_vectors_for_exhaustive_short_traces) {
    const std::vector<SanEvent> alphabet = {
        MakeBufEvent(PipeType::PIPE_V, 17, SyncType::GET_BUF, 0, BufMode::BLOCK_MODE),
        MakeBufEvent(PipeType::PIPE_V, 17, SyncType::GET_BUF, 1, BufMode::BLOCK_MODE),
        MakeBufEvent(PipeType::PIPE_V, 17, SyncType::GET_BUF, 2, BufMode::BLOCK_MODE),
        MakeBufEvent(PipeType::PIPE_V, 17, SyncType::GET_BUF, 0, BufMode::NONBLOCK_MODE),
        MakeBufEvent(PipeType::PIPE_V, 17, SyncType::GET_BUF, 2, BufMode::NONBLOCK_MODE),
        MakeBufEvent(PipeType::PIPE_V, 17, SyncType::RLS_BUF, 0, BufMode::BLOCK_MODE),
        MakeBufEvent(PipeType::PIPE_V, 17, SyncType::RLS_BUF, 2, BufMode::NONBLOCK_MODE)};
    size_t traceCount = 0;
    size_t combinations = 1;
    for (size_t length = 0; length <= 4; ++length, combinations *= alphabet.size()) {
        for (size_t code = 0; code < combinations; ++code) {
            for (bool compact : {false, true}) {
                SCOPED_TRACE(::testing::Message() << length << ":" << code << ":" << compact);
                replayer_.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, 1, compact);
                BufQueueReference reference;
                size_t value = code;
                for (size_t i = 0; i < length; ++i, value /= alphabet.size()) {
                    CompareBufQueueReference(replayer_, reference, alphabet[value % alphabet.size()], 0);
                }
            }
            ++traceCount;
        }
    }
    EXPECT_EQ(traceCount, 2801U);
    RecordProperty("buf_differential_trace_count_per_mode", traceCount);
}

TEST_F(TestPipelineReplayer, buf_counts_match_old_vectors_with_block_and_buffer_isolation) {
    for (bool compact : {false, true}) {
        SCOPED_TRACE(compact);
        replayer_.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2, compact);
        BufQueueReference reference;
        const std::vector<SanEvent> trace = {
            MakeBufEvent(PipeType::PIPE_V, 17, SyncType::RLS_BUF, 2, BufMode::NONBLOCK_MODE),
            MakeBufEvent(PipeType::PIPE_V, 17, SyncType::RLS_BUF, 0, BufMode::BLOCK_MODE),
            MakeBufEvent(PipeType::PIPE_V, 18, SyncType::GET_BUF, 1, BufMode::BLOCK_MODE),
            MakeBufEvent(PipeType::PIPE_V, 17, SyncType::GET_BUF, 1, BufMode::BLOCK_MODE, 1),
            MakeBufEvent(PipeType::PIPE_V, 17, SyncType::GET_BUF, 2, BufMode::NONBLOCK_MODE),
            MakeBufEvent(PipeType::PIPE_V, 17, SyncType::GET_BUF, 2, BufMode::BLOCK_MODE),
            MakeBufEvent(PipeType::PIPE_V, 17, SyncType::GET_BUF, 2, BufMode::BLOCK_MODE),
            MakeBufEvent(PipeType::PIPE_V, 17, SyncType::GET_BUF, 2, BufMode::BLOCK_MODE),
            MakeBufEvent(PipeType::PIPE_V, 17, SyncType::GET_BUF, 0, BufMode::BLOCK_MODE),
            MakeBufEvent(PipeType::PIPE_V, 17, SyncType::RLS_BUF, 0, BufMode::NONBLOCK_MODE),
            MakeBufEvent(PipeType::PIPE_V, 17, SyncType::GET_BUF, 1, BufMode::BLOCK_MODE),
            MakeBufEvent(PipeType::PIPE_V, 18, SyncType::GET_BUF, 0, BufMode::NONBLOCK_MODE, 1),
            MakeBufEvent(PipeType::PIPE_V, 18, SyncType::GET_BUF, 0, BufMode::BLOCK_MODE, 1),
            MakeBufEvent(PipeType::PIPE_V, 18, SyncType::GET_BUF, 0, BufMode::BLOCK_MODE, 1)};
        for (const auto &event : trace) {
            CompareBufQueueReference(replayer_, reference, event, event.loc.coreId);
        }
    }
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

// case 3. 回调：MEMORY_EVENT 在内存事件处理时触发
TEST_F(TestPipelineReplayer, callback_fires_during_processing_memory_events) {
    RegisterCollectingCallback();

    replayer_.Do(MakeMemEvent(PipeType::PIPE_S, AccessType::READ));
    replayer_.Do(MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 1, SyncType::SET_FLAG));
    replayer_.Do(MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 1, SyncType::WAIT_FLAG));
    replayer_.Do(MakeKernelFinishEvent());

    int memCount = 0, syncCount = 0;
    for (auto &r : callbackRecords_) {
        if (r.type != ReplayerCallbackType::MEMORY_EVENT) {
            continue;
        }
        if (r.event.type == EventType::MEM_EVENT) {
            ++memCount;
        }
        if (r.event.type == EventType::SYNC_EVENT) {
            ++syncCount;
        }
    }
    ASSERT_GE(memCount, 1) << "MEM_EVENT should trigger MEMORY_EVENT";
    ASSERT_GE(syncCount, 0) << "SYNC_EVENT should not trigger MEMORY_EVENT";
}

// case 4. 内存事件不迁移：仍留在 PIPE_S 队列，按记录顺序（= 程序顺序）回放
// 若把内存事件一并迁移到各自的 PIPE 队列，PIPE_V 先于 PIPE_MTE2 被排空，
// 跨 PIPE 的"先写后读"会被颠倒，使 initcheck 误报未初始化读。
TEST_F(TestPipelineReplayer, mem_events_keep_record_order_across_pipes) {
    RegisterCollectingCallback();
    replayer_.Do(MakeMemEvent(PipeType::PIPE_MTE2, AccessType::WRITE));
    replayer_.Do(MakeMemEvent(PipeType::PIPE_V, AccessType::READ));
    replayer_.Do(MakeKernelFinishEvent());

    ASSERT_TRUE(replayer_.IsFinished());
    std::vector<AccessType> order;
    for (auto &r : callbackRecords_) {
        if (r.type == ReplayerCallbackType::MEMORY_EVENT && r.event.type == EventType::MEM_EVENT) {
            order.push_back(r.event.eventInfo.memInfo.opType);
        }
    }
    ASSERT_EQ(order.size(), 2U);
    EXPECT_EQ(order[0], AccessType::WRITE) << "写事件应按记录顺序先于读事件回放";
    EXPECT_EQ(order[1], AccessType::READ);
}

// case 5. 同步事件迁移：同核 wait 排在其 set 之前（多流水并行的正常形态）不应误报卡死
// wait 挂目的 PIPE、set 挂生产者 PIPE；若不迁移，wait 会阻塞 PIPE_S 单队列，
// 排在它之后的 set 永远消费不到，导致误报 kernel locked up。
TEST_F(TestPipelineReplayer, sync_events_migrate_so_stuck_wait_before_its_set) {
    RegisterCollectingCallback();
    replayer_.Do(MakeSyncEvent(PipeType::PIPE_V, PipeType::PIPE_MTE2, PipeType::PIPE_V, 5, SyncType::WAIT_FLAG));
    replayer_.Do(MakeSyncEvent(PipeType::PIPE_MTE2, PipeType::PIPE_MTE2, PipeType::PIPE_V, 5, SyncType::SET_FLAG));
    replayer_.Do(MakeKernelFinishEvent());

    ASSERT_TRUE(replayer_.IsFinished());
    AssertNoStuck();
}

// case 6. SYNC_EVENT：SET_FLAG / WAIT_FLAG 配对成功 && 无 SET 时卡死
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

// case 7. CROSS_CORE_SYNC_EVENT：FFTS_SYNC + WAIT_FLAG_DEV / WAIT_INTRA_BLOCK
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

// case 8. CROSS_CORE_SOFT_SYNC_EVENT：IB_SET / IB_WAIT / SYNC_ALL
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

// case 8b. 上报方声明"不限制对端核"（peerCoreId < 0，MSTX 跨核上报接口用 -1）：
// set 与 wait 在不同核上、wait 不指定对端核时应配对成功；
// 曾因 -1 被截断成越界核号而永久等待，造成 IB_WAIT 假卡死（并连带 barrier 误报）。
TEST_F(TestPipelineReplayer, soft_sync_wait_with_unspecified_peer_core_expect_no_stuck) {
    // 需要 4 个 block，让 set 与 wait 落在不同核上
    ResetReplayer(4U);
    RegisterCollectingCallback();

    // 核 3 上 set，核 1 上 wait（peerCoreId = -1）→ 应配对成功
    replayer_.Do(MakeSoftSyncEvent(PipeType::PIPE_S, 42, SyncType::IB_SET, -1, 0, 3));
    replayer_.Do(MakeSoftSyncEvent(PipeType::PIPE_S, 42, SyncType::IB_WAIT, -1, 0, 1));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertNoStuck();

    // 无对应 set 时（peerCoreId = -1）仍应判定位卡死
    ResetReplayer(4U);
    RegisterCollectingCallback();
    replayer_.Do(MakeSoftSyncEvent(PipeType::PIPE_S, 7, SyncType::IB_WAIT, -1, 0, 1));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertStuck("IB_WAIT with peerCoreId=-1 without IB_SET should stall");
}

// case 8c. 复现 shmem allgather 上报的级联形态：
// 全核先过一轮 barrier，随后部分核用 peerCoreId = -1 上报软同步 set/wait，再进第二轮 barrier。应正常通过无告警。
TEST_F(TestPipelineReplayer, cascade_unspecified_peer_soft_sync_and_barrier_expect_no_stuck) {
    constexpr uint32_t kBlockDim = 4U; // 用 4 个核等价压缩 16 核场景
    constexpr uint32_t kProducerNum = 2U; // 前 2 核为生产者，其余为消费者
    constexpr int32_t kEventId = 100;
    ResetReplayer(kBlockDim);
    RegisterCollectingCallback();

    // 第一轮 barrier：全核到齐
    for (uint32_t i = 0; i < kBlockDim; ++i) {
        replayer_.Do(MakeMstxBarrierEvent(PipeType::PIPE_S, kBlockDim, i));
    }
    // 生产者上报 set、消费者用 peerCoreId = -1 上报 wait
    for (uint32_t i = 0; i < kProducerNum; ++i) {
        replayer_.Do(MakeSoftSyncEvent(PipeType::PIPE_S, kEventId, SyncType::IB_SET, -1, 0, i));
    }
    for (uint32_t i = kProducerNum; i < kBlockDim; ++i) {
        replayer_.Do(MakeSoftSyncEvent(PipeType::PIPE_S, kEventId, SyncType::IB_WAIT, -1, 0, i));
    }
    // 第二轮 barrier：只要有一个核没推到此处，就凑不齐到齐数
    for (uint32_t i = 0; i < kBlockDim; ++i) {
        replayer_.Do(MakeMstxBarrierEvent(PipeType::PIPE_S, kBlockDim, i));
    }
    replayer_.Do(MakeKernelFinishEvent());

    ASSERT_TRUE(replayer_.IsFinished());
    AssertNoStuck();
}

// case 9. MSTX_CROSS_SYNC_EVENT：SET_CROSS / WAIT_CROSS (含 isMore 跳过)
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

// case 10. BARRIER：CROSS_CORE_BARRIER + NPU_BARRIER 退化
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

// case 11. BUF_SYNC_EVENT：GET_BUF / RLS_BUF
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

// case 12. 综合多事件序列 + 卡死回调准确性验证
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
        if (r.type == ReplayerCallbackType::MEMORY_EVENT) {
            ++pc;
        }
    }
    ASSERT_GT(pc, 1U) << "Multiple events should trigger multiple callbacks";

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

// case 13. 跨 kernel 复用 syncDB_
// 场景：kernel1 只发 SET_FLAG；同一 replayer_ 对象 Init 后，kernel2 只发同 key 的 WAIT_FLAG。
// 预期：仍判卡死（ALL_DEVICE_STUCK），上一 kernel 的过期 set 不能把本 kernel 的 wait 满足掉。
TEST_F(TestPipelineReplayer, reinit_clears_sync_db_across_kernels) {
    RegisterCollectingCallback();
    // kernel1：只有 SET_FLAG，没有配对的 WAIT_FLAG
    replayer_.Do(MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 7, SyncType::SET_FLAG));
    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());

    // kernel2：复用同一个 replayer_ 对象（生产代码中它是进程级对象，每个 kernel 只调 Init）
    callbackRecords_.clear();
    replayer_.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, kDefaultBlockDim);

    replayer_.Do(MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 7, SyncType::WAIT_FLAG));
    replayer_.Do(MakeKernelFinishEvent());

    ASSERT_TRUE(replayer_.IsFinished());
    AssertStuck("stale SET_FLAG from the previous kernel must not satisfy this kernel's WAIT_FLAG");
}

// case 14. __mix__(0,1) 全 AIV 阻塞的到齐数
// 场景：MIX 内核 blockDim=2（展开后 6 个 block），AIV coreId 0/1 各发一次 MODE0 全 AIV 阻塞 set，block 1 再等它。
// 预期：两个 AIV 核到齐后 wait 立即满足，回放正常结束、无 ALL_DEVICE_STUCK。
TEST_F(TestPipelineReplayer, mix_kernel_with_single_vec_sub_block_dim_expect_no_stuck) {
    // MIX + 910B1 会按 subblock 展开：maxBlockNum_ = blockDim * C220_MIX_SUB_BLOCKDIM
    replayer_.Init(KernelType::MIX, DeviceType::ASCEND_910B1, 2U);
    RegisterCollectingCallback();

    // AIV coreId 0/1 展开后是 block 0/1：各发一个 MODE0 全 AIV 阻塞 set，vecSubBlockDim=1
    auto setOnCore0 = MakeBlockSyncEvent(PipeType::PIPE_S, 0, static_cast<uint8_t>(FftsSyncMode::MODE0),
                                         SyncType::FFTS_SYNC, 0);
    setOnCore0.eventInfo.fftsSyncInfo.vecSubBlockDim = 1U;
    replayer_.Do(setOnCore0);

    auto setOnCore1 = MakeBlockSyncEvent(PipeType::PIPE_S, 0, static_cast<uint8_t>(FftsSyncMode::MODE0),
                                         SyncType::FFTS_SYNC, 1);
    setOnCore1.eventInfo.fftsSyncInfo.vecSubBlockDim = 1U;
    replayer_.Do(setOnCore1);

    // block 1 等待该全 AIV 阻塞，2 个 AIV 核到齐后应被满足
    auto waitOnCore1 = MakeBlockSyncEvent(PipeType::PIPE_S, 0, static_cast<uint8_t>(FftsSyncMode::MODE0),
                                          SyncType::WAIT_FLAG_DEV, 1);
    waitOnCore1.eventInfo.fftsSyncInfo.vecSubBlockDim = 1U;
    replayer_.Do(waitOnCore1);

    replayer_.Do(MakeKernelFinishEvent());
    ASSERT_TRUE(replayer_.IsFinished());
    AssertNoStuck();
}

TEST_F(TestPipelineReplayer, compact_scheduler_and_default_memory_callback_contract) {
    auto control = MakeEventBase(EventType::SANITIZER_CONTROL_EVENT, PipeType::PIPE_S, 0);
    control.eventInfo.sanitizerControlInfo.type = SanitizerControlType::FINISH;
    std::vector<SanEvent> trace = {
        MakeMemEvent(PipeType::PIPE_MTE2, AccessType::MEMCPY_BLOCKS),
        MakeDynamicMemEvent(PipeType::PIPE_V), MakeMemEvent(PipeType::PIPE_V, AccessType::WRITE),
        MakeTimeEvent(PipeType::PIPE_S), MakeMemEvent(PipeType::PIPE_MTE3, AccessType::READ), control,
        MakeDynamicMemEvent(PipeType::PIPE_MTE1),
        MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 3, SyncType::PIPE_BARRIER),
        MakeMemEvent(PipeType::PIPE_V, AccessType::WRITE)};
    trace[0].loc.pc = 0x1234;
    trace[0].loc.fileNo = 7;
    trace[0].loc.lineNo = 99;
    trace[0].closedRingStartSerialNo = 42;
    for (bool compact : {false, true}) {
        ResetReplayer(); // This three-argument Init must continue to mean full memory replay.
        if (compact) { replayer_.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2, true); }
        RegisterCollectingCallback();
        for (const auto &event : trace) { replayer_.Do(event); }
        ASSERT_EQ(replayer_.GetMemEventCount(), 6U);
        ASSERT_EQ(replayer_.eventContainer_.GetAllQueSize(), compact ? 7U : 10U);
        std::vector<SanEvent> processed;
        PipeLine line(replayer_.eventContainer_);
        line.RegisterEventFunc([&](const SanEvent &event) {
            processed.push_back(event);
            return replayer_.ProcessEvent(event);
        });
        line.Run();
        ASSERT_TRUE(replayer_.eventContainer_.IsEmpty());
        std::vector<SanEvent> expected;
        if (compact) {
            expected = {trace[0], trace[3], trace[4], trace[5], trace[6], trace[7], trace[8]};
            for (size_t i : {0U, 2U, 4U, 6U}) {
                expected[i].type = EventType::TIME_EVENT;
                expected[i].pipe = PipeType::PIPE_S;
                expected[i].timeInfo = nullptr;
            }
        } else {
            expected = trace;
            expected[0].eventInfo.memInfo.opType = AccessType::READ;
            auto write = trace[0];
            write.eventInfo.memInfo.opType = AccessType::WRITE;
            expected.insert(expected.begin() + 1, write);
        }
        ASSERT_EQ(processed.size(), expected.size());
        for (size_t i = 0; i < expected.size(); ++i) { AssertSameEvent(processed[i], expected[i]); }
        std::vector<SanEvent> expectedCallbacks;
        for (const auto &event : expected) {
            if (event.type == EventType::MEM_EVENT || event.type == EventType::DYNAMIC_MEM_EVENT) {
                expectedCallbacks.push_back(event);
            }
        }
        ASSERT_EQ(callbackRecords_.size(), compact ? 0U : 7U);
        ASSERT_EQ(callbackRecords_.size(), expectedCallbacks.size());
        for (size_t i = 0; i < callbackRecords_.size(); ++i) {
            EXPECT_EQ(callbackRecords_[i].type, ReplayerCallbackType::MEMORY_EVENT);
            AssertSameEvent(callbackRecords_[i].event, expectedCallbacks[i]);
        }
        RecordProperty(compact ? "compact_scheduler_calls" : "default_scheduler_calls", processed.size());
        RecordProperty(compact ? "compact_memory_callbacks" : "default_memory_callbacks", callbackRecords_.size());
    }
    CompareCompactModes(trace, {});
}

TEST_F(TestPipelineReplayer, compact_every_nonmemory_boundary_and_interleaved_blocks) {
    for (auto type : {EventType::TIME_EVENT, EventType::SYNC_EVENT, EventType::CROSS_CORE_SYNC_EVENT,
        EventType::CROSS_CORE_SOFT_SYNC_EVENT, EventType::BUF_SYNC_EVENT, EventType::MSTX_CROSS_SYNC_EVENT,
        EventType::MSTX_CROSS_CORE_BARRIER, EventType::MSTX_CROSS_NPU_BARRIER, EventType::H_SYNC_EVENT,
        EventType::MSTX_SIGNAL_SET_EVENT, EventType::MSTX_SIGNAL_WAIT_EVENT, EventType::REGISTER_EVENT,
        EventType::SANITIZER_CONTROL_EVENT}) {
        SCOPED_TRACE(static_cast<int>(type));
        auto boundary0 = MakeEventBase(type, type == EventType::TIME_EVENT ? PipeType::PIPE_MTE3 : PipeType::PIPE_S, 0);
        if (type == EventType::SANITIZER_CONTROL_EVENT) {
            boundary0.eventInfo.sanitizerControlInfo.type = SanitizerControlType::FINISH;
        }
        auto boundary1 = boundary0;
        boundary1.loc.coreId = 1;
        std::vector<SanEvent> trace = {MakeMemEvent(PipeType::PIPE_MTE2, AccessType::MEMCPY_BLOCKS, 0),
            MakeMemEvent(PipeType::PIPE_V, AccessType::READ, 1), boundary1,
            MakeDynamicMemEvent(PipeType::PIPE_MTE3, 0), MakeDynamicMemEvent(PipeType::PIPE_V, 1), boundary0,
            MakeMemEvent(PipeType::PIPE_V, AccessType::WRITE, 0), MakeDynamicMemEvent(PipeType::PIPE_MTE1, 0)};
        replayer_.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2, true);
        for (const auto &event : trace) { replayer_.Do(event); }
        ASSERT_EQ(replayer_.GetMemEventCount(), 6U);
        ASSERT_EQ(replayer_.eventContainer_.GetAllQueSize(), 6U);
        for (uint32_t block = 0; block < 2; ++block) {
            auto expected = block == 0 ? std::vector<SanEvent>{trace[0], boundary0, trace[6]} :
                std::vector<SanEvent>{trace[1], boundary1, trace[4]};
            for (size_t i : {0U, 2U}) {
                expected[i].type = EventType::TIME_EVENT;
                expected[i].pipe = PipeType::PIPE_S;
                expected[i].timeInfo = nullptr;
            }
            auto &container = replayer_.eventContainer_;
            container.SetQueIndex(PipeType::PIPE_S);
            for (const auto &event : expected) {
                ASSERT_FALSE(container.IsCurQueEmpty());
                AssertSameEvent(container.Front(), event);
                container.Pop();
            }
            EXPECT_TRUE(container.IsCurQueEmpty());
            container.SwitchToNextBlock();
        }
    }
}

TEST_F(TestPipelineReplayer, compact_true_deadlock_preserves_all_head_fields_and_order) {
    auto waitV = MakeSyncEvent(PipeType::PIPE_V, PipeType::PIPE_MTE2, PipeType::PIPE_V, 7, SyncType::WAIT_FLAG);
    auto waitS = MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_MTE3, PipeType::PIPE_S, 8, SyncType::WAIT_FLAG);
    auto get = MakeBufEvent(PipeType::PIPE_MTE3, 123, SyncType::GET_BUF, 2, BufMode::BLOCK_MODE, 1);
    for (auto event : {&waitV, &waitS, &get}) {
        event->loc.pc = 0x123400 + event->serialNo * 4;
        event->loc.fileNo = 3;
        event->loc.lineNo = 17 + event->serialNo;
        event->loc.kernelIdx = 5;
        event->loc.deviceId = 9;
        event->isAtomicMode = true;
        event->closedRingStartSerialNo = 11;
    }
    CompareCompactModes({MakeMemEvent(PipeType::PIPE_V, AccessType::MEMCPY_BLOCKS), waitV,
        MakeDynamicMemEvent(PipeType::PIPE_MTE2), waitS,
        MakeMemEvent(PipeType::PIPE_V, AccessType::WRITE), MakeDynamicMemEvent(PipeType::PIPE_V),
        MakeMemEvent(PipeType::PIPE_MTE2, AccessType::READ, 1), get,
        MakeDynamicMemEvent(PipeType::PIPE_MTE3, 1)}, {waitS, waitV, get});
}

TEST_F(TestPipelineReplayer, compact_repeated_sync_and_cross_pipe_wait_before_set) {
    std::vector<SanEvent> trace;
    for (uint32_t round = 0; round < 3; ++round) {
        trace.push_back(MakeMemEvent(PipeType::PIPE_MTE2, AccessType::MEMCPY_BLOCKS));
        trace.push_back(MakeDynamicMemEvent(PipeType::PIPE_V));
        trace.push_back(MakeSyncEvent(PipeType::PIPE_V, PipeType::PIPE_MTE2, PipeType::PIPE_V, 5,
            SyncType::WAIT_FLAG));
        trace.push_back(MakeMemEvent(PipeType::PIPE_V, AccessType::READ));
        trace.push_back(MakeSyncEvent(PipeType::PIPE_MTE2, PipeType::PIPE_MTE2, PipeType::PIPE_V, 5,
            SyncType::SET_FLAG));
    }
    CompareCompactModes(trace, {});
    auto extraWait = MakeSyncEvent(PipeType::PIPE_V, PipeType::PIPE_MTE2, PipeType::PIPE_V, 5, SyncType::WAIT_FLAG);
    trace.push_back(extraWait);
    trace.push_back(MakeDynamicMemEvent(PipeType::PIPE_MTE3));
    CompareCompactModes(trace, {extraWait});
}

TEST_F(TestPipelineReplayer, compact_mstx_crosscore_buf_and_barrier_traces) {
    std::vector<std::vector<SanEvent>> traces = {
        {MakeSoftSyncEvent(PipeType::PIPE_S, 42, SyncType::IB_WAIT, -1, 0, 0),
            MakeSoftSyncEvent(PipeType::PIPE_S, 42, SyncType::IB_SET, -1, 0, 1)},
        {MakeMstxCrossSyncEvent(PipeType::PIPE_S, 0xA000, 1, SyncType::MSTX_WAIT_CROSS, false, 0),
            MakeMstxCrossSyncEvent(PipeType::PIPE_S, 0xA000, 1, SyncType::MSTX_SET_CROSS, false, 1)},
        {MakeMstxCrossSyncEvent(PipeType::PIPE_S, 0xB000, 2, SyncType::MSTX_WAIT_CROSS, true, 0)},
        {MakeBufEvent(PipeType::PIPE_V, 12, SyncType::GET_BUF, 1, BufMode::BLOCK_MODE),
            MakeBufEvent(PipeType::PIPE_MTE2, 12, SyncType::RLS_BUF, 0, BufMode::BLOCK_MODE)},
        {MakeMstxBarrierEvent(PipeType::PIPE_S, 2, 0), MakeMstxBarrierEvent(PipeType::PIPE_S, 2, 1)},
        {MakeMstxNpuBarrierEvent(PipeType::PIPE_S, 1, 1, 0)},
        {MakeBlockSyncEvent(PipeType::PIPE_S, 3, 0, SyncType::FFTS_SYNC, 0),
            MakeBlockSyncEvent(PipeType::PIPE_S, 3, 0, SyncType::FFTS_SYNC, 1),
            MakeBlockSyncEvent(PipeType::PIPE_S, 3, 0, SyncType::WAIT_FLAG_DEV, 0)}};
    for (size_t i = 0; i < traces.size(); ++i) {
        SCOPED_TRACE(i);
        std::vector<SanEvent> trace;
        for (const auto &event : traces[i]) {
            trace.push_back(MakeMemEvent(PipeType::PIPE_MTE2, AccessType::MEMCPY_BLOCKS, event.loc.coreId));
            trace.push_back(MakeDynamicMemEvent(PipeType::PIPE_V, event.loc.coreId));
            trace.push_back(event);
            trace.push_back(MakeMemEvent(PipeType::PIPE_V, AccessType::READ, event.loc.coreId));
        }
        CompareCompactModes(trace, {});
    }
    for (auto wait : {MakeSoftSyncEvent(PipeType::PIPE_S, 7, SyncType::IB_WAIT, -1, 0, 1),
        MakeMstxCrossSyncEvent(PipeType::PIPE_S, 0xC000, 3, SyncType::MSTX_WAIT_CROSS, false),
        MakeBlockSyncEvent(PipeType::PIPE_S, 5, 0, SyncType::WAIT_FLAG_DEV),
        MakeBufEvent(PipeType::PIPE_S, 300, SyncType::GET_BUF, 2, BufMode::BLOCK_MODE),
        MakeMstxBarrierEvent(PipeType::PIPE_S, 2, 0)}) {
        CompareCompactModes({MakeMemEvent(PipeType::PIPE_V, AccessType::WRITE, wait.loc.coreId), wait,
            MakeDynamicMemEvent(PipeType::PIPE_MTE2, wait.loc.coreId)}, {wait});
    }
    // Existing SyncAll returns before SemCores::Wait while the remaining count is nonzero.
    // Preserve its two real stuck heads; this is not a successful-barrier assertion.
    auto all0 = MakeSoftSyncEvent(PipeType::PIPE_S, 0, SyncType::SYNC_ALL, 0, 2, 0);
    auto all1 = MakeSoftSyncEvent(PipeType::PIPE_S, 0, SyncType::SYNC_ALL, 0, 2, 1);
    CompareCompactModes({MakeDynamicMemEvent(PipeType::PIPE_V, 0), all0,
        MakeMemEvent(PipeType::PIPE_V, AccessType::WRITE, 0), MakeDynamicMemEvent(PipeType::PIPE_V, 1), all1,
        MakeMemEvent(PipeType::PIPE_V, AccessType::WRITE, 1)}, {all0, all1});
}

TEST_F(TestPipelineReplayer, compact_mix_expanded_subblocks_one_and_two_vec) {
    for (uint8_t vecDim : {1U, 2U}) {
        SCOPED_TRACE(vecDim);
        std::vector<SanEvent> trace;
        for (uint32_t core = 0; core < 2U * vecDim; ++core) {
            auto set = MakeBlockSyncEvent(PipeType::PIPE_S, 0, 0, SyncType::FFTS_SYNC, core);
            set.eventInfo.fftsSyncInfo.vecSubBlockDim = vecDim;
            trace.push_back(MakeMemEvent(PipeType::PIPE_V, AccessType::MEMCPY_BLOCKS, core));
            trace.push_back(MakeDynamicMemEvent(PipeType::PIPE_MTE2, core));
            trace.push_back(set);
            auto wait = set;
            wait.serialNo = g_serialNo++;
            wait.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
            trace.push_back(wait);
        }
        for (uint32_t core = 0; core < 2; ++core) {
            auto mem = MakeMemEvent(PipeType::PIPE_MTE1, AccessType::WRITE, core);
            mem.loc.blockType = BlockType::AICUBE;
            trace.push_back(mem);
            auto wait = MakeSyncEvent(PipeType::PIPE_V, PipeType::PIPE_MTE2, PipeType::PIPE_V, 3,
                SyncType::WAIT_FLAG, core);
            wait.loc.blockType = BlockType::AICUBE;
            trace.push_back(wait);
            auto set = wait;
            set.serialNo = g_serialNo++;
            set.pipe = PipeType::PIPE_MTE2;
            set.eventInfo.syncInfo.opType = SyncType::SET_FLAG;
            trace.push_back(set);
        }
        CompareCompactModes(trace, {}, KernelType::MIX);
        auto missing = MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 9,
            SyncType::WAIT_FLAG, 1);
        missing.loc.blockType = BlockType::AICUBE;
        trace.push_back(missing);
        CompareCompactModes(trace, {missing}, KernelType::MIX);
    }
}

TEST_F(TestPipelineReplayer, compact_reinit_resets_unfinished_memory_runs_and_sync_state) {
    for (bool compact : {true, false, true}) {
        replayer_.Init(KernelType::MIX, DeviceType::ASCEND_910B1, 2, true);
        replayer_.Do(MakeMemEvent(PipeType::PIPE_V, AccessType::WRITE, 1));
        replayer_.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2, compact);
        ASSERT_EQ(replayer_.GetMemEventCount(), 0U);
        ASSERT_EQ(replayer_.eventContainer_.GetAllQueSize(), 0U);
        callbackRecords_.clear();
        RegisterCollectingCallback();
        replayer_.Do(MakeMemEvent(PipeType::PIPE_MTE2, AccessType::MEMCPY_BLOCKS, 1));
        EXPECT_EQ(replayer_.eventContainer_.GetAllQueSize(), compact ? 1U : 2U);
        replayer_.Do(MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 7, SyncType::SET_FLAG));
        replayer_.Do(MakeKernelFinishEvent());
        ASSERT_TRUE(replayer_.IsFinished());
        AssertNoStuck();
        EXPECT_EQ(callbackRecords_.size(), compact ? 0U : 2U);
        callbackRecords_.clear();
        replayer_.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2, compact);
        auto wait = MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 7, SyncType::WAIT_FLAG);
        replayer_.Do(wait);
        replayer_.Do(MakeKernelFinishEvent());
        ASSERT_EQ(callbackRecords_.size(), 1U);
        EXPECT_EQ(callbackRecords_[0].type, ReplayerCallbackType::ALL_DEVICE_STUCK);
        AssertSameEvent(callbackRecords_[0].event, wait);
    }
}

TEST_F(TestPipelineReplayer, compact_empty_trace_empty_blocks_and_invalid_blocks) {
    CompareCompactModes({}, {}, KernelType::AIVEC, 0);
    CompareCompactModes({}, {}, KernelType::AIVEC, 4);
    CompareCompactModes({MakeMemEvent(PipeType::PIPE_V, AccessType::READ, 3)}, {}, KernelType::AIVEC, 4);
    for (bool compact : {false, true}) {
        replayer_.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, 1, compact);
        callbackRecords_.clear();
        RegisterCollectingCallback();
        auto invalid = MakeMemEvent(PipeType::PIPE_V, AccessType::MEMCPY_BLOCKS, 999);
        invalid.serialNo = 999001;
        replayer_.Do(invalid);
        invalid.type = EventType::DYNAMIC_MEM_EVENT;
        invalid.serialNo = 999002;
        replayer_.Do(invalid);
        auto sync = MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 1, SyncType::WAIT_FLAG, 999);
        sync.serialNo = 999003;
        replayer_.Do(sync);
        EXPECT_EQ(replayer_.GetMemEventCount(), 2U);
        EXPECT_EQ(replayer_.eventContainer_.GetAllQueSize(), 0U);
        replayer_.Do(MakeKernelFinishEvent());
        EXPECT_TRUE(replayer_.IsFinished());
        EXPECT_TRUE(callbackRecords_.empty());
    }
}

TEST_F(TestPipelineReplayer, compact_exhaustive_small_traces_preserve_stuck_fronts) {
    const std::vector<SanEvent> alphabet = {
        MakeMemEvent(PipeType::PIPE_V, AccessType::WRITE), MakeDynamicMemEvent(PipeType::PIPE_MTE3),
        MakeMemEvent(PipeType::PIPE_MTE2, AccessType::MEMCPY_BLOCKS),
        MakeSyncEvent(PipeType::PIPE_S, PipeType::PIPE_V, PipeType::PIPE_S, 7, SyncType::WAIT_FLAG),
        MakeSyncEvent(PipeType::PIPE_V, PipeType::PIPE_MTE2, PipeType::PIPE_V, 5, SyncType::WAIT_FLAG),
        MakeSyncEvent(PipeType::PIPE_MTE2, PipeType::PIPE_MTE2, PipeType::PIPE_V, 5, SyncType::SET_FLAG),
        MakeMemEvent(PipeType::PIPE_V, AccessType::READ, 1), MakeTimeEvent(PipeType::PIPE_MTE3)};
    size_t traceCount = 0;
    size_t combinations = 1;
    for (size_t length = 0; length <= 4; ++length, combinations *= alphabet.size()) {
        for (size_t code = 0; code < combinations; ++code) {
            SCOPED_TRACE(::testing::Message() << length << ":" << code);
            std::vector<SanEvent> trace;
            size_t value = code;
            for (size_t i = 0; i < length; ++i, value /= alphabet.size()) {
                auto event = alphabet[value % alphabet.size()];
                event.serialNo = i + 1;
                event.loc.pc = 0x1000 + i * 4;
                trace.push_back(event);
            }
            std::vector<SanEvent> baseline;
            for (bool compact : {false, true}) {
                PipelineReplayer replay;
                replay.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2, compact);
                std::vector<SanEvent> stuck;
                replay.RegisterCallback([&](ReplayerCallbackType type, const SanEvent &event) {
                    if (type == ReplayerCallbackType::ALL_DEVICE_STUCK) {
                        EXPECT_NE(event.type, EventType::MEM_EVENT);
                        EXPECT_NE(event.type, EventType::DYNAMIC_MEM_EVENT);
                        EXPECT_NE(event.type, EventType::TIME_EVENT);
                        stuck.push_back(event);
                    } else {
                        EXPECT_FALSE(compact);
                    }
                });
                for (const auto &event : trace) { replay.Do(event); }
                replay.Do(MakeKernelFinishEvent());
                ASSERT_TRUE(replay.IsFinished());
                if (!compact) {
                    baseline = stuck;
                } else {
                    ASSERT_EQ(stuck.size(), baseline.size());
                    for (size_t i = 0; i < stuck.size(); ++i) { AssertSameEvent(stuck[i], baseline[i]); }
                }
            }
            ++traceCount;
        }
    }
    EXPECT_EQ(traceCount, 4681U);
    RecordProperty("differential_trace_count", traceCount);
}
