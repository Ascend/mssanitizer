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

#include "alg_framework/single_pipe_race_alg_impl.h"
#include "core/framework/record_parse.h"

namespace {
using namespace Sanitizer;

TEST(SinglePipeRaceAlgImpl, race_alg_can_detect_race_events_expect_success)
{
    SinglePipeRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent event;

    event.loc.coreId = 0;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    event.loc.blockType = BlockType::AIVEC;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.serialNo = 1U;
    alg.Do(event);
    event.eventInfo.memInfo.opType = AccessType::READ;
    event.serialNo = 2U;
    alg.Do(event);
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.serialNo = 3U;
    alg.Do(event);
    ASSERT_EQ(alg.IsFinished(), false);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 2U);
    ASSERT_EQ(alg.IsFinished(), true);
}

TEST(SinglePipeRaceAlgImpl, race_alg_detect_pipe_barrier_expect_no_race)
{
    SinglePipeRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent event;

    event.loc.coreId = 0;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_V;
    event.loc.blockType = BlockType::AIVEC;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.serialNo = 1U;
    alg.Do(event);
    event.type = EventType::SYNC_EVENT;
    event.eventInfo.syncInfo.opType = SyncType::PIPE_BARRIER;
    event.serialNo = 2U;
    alg.Do(event);
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo.opType = AccessType::READ;
    event.serialNo = 3U;
    alg.Do(event);
    ASSERT_EQ(alg.IsFinished(), false);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 0U);
    ASSERT_EQ(alg.IsFinished(), true);
}

// GET_BUF(BLOCK_MODE, rlsCount>0)起同步屏障作用，前后MEM_EVENT不应检测到竞争
TEST(SinglePipeRaceAlgImpl, get_buf_block_mode_expect_no_race) {
    SinglePipeRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent event;

    event.loc.coreId = 0;
    event.pipe = PipeType::PIPE_V;
    event.loc.blockType = BlockType::AIVEC;

    // 第一条mem事件：写地址0x50
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.serialNo = 1U;
    alg.Do(event);

    // GET_BUF(BLOCK_MODE)，rlsCount>0表示非首个get_buf，起屏障作用
    event.type = EventType::BUF_SYNC_EVENT;
    event.eventInfo.bufSyncInfo.opType = SyncType::GET_BUF;
    event.eventInfo.bufSyncInfo.mode = BufMode::BLOCK_MODE;
    event.eventInfo.bufSyncInfo.bufId = 0;
    event.eventInfo.bufSyncInfo.rlsCount = 1U;
    event.serialNo = 2U;
    alg.Do(event);

    // 第二条mem事件：读地址0x50，union已被bufSyncInfo覆盖，需重新设置所有memInfo字段
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo.opType = AccessType::READ;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.serialNo = 3U;
    alg.Do(event);

    ASSERT_EQ(alg.IsFinished(), false);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 0U);
    ASSERT_EQ(alg.IsFinished(), true);
}

// 首个GET_BUF(rlsCount==0)不具备阻塞作用，前后MEM_EVENT应检测到竞争
TEST(SinglePipeRaceAlgImpl, first_get_buf_expect_race) {
    SinglePipeRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent event;

    event.loc.coreId = 0;
    event.pipe = PipeType::PIPE_V;
    event.loc.blockType = BlockType::AIVEC;

    // 第一条mem事件：写地址0x50
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.serialNo = 1U;
    alg.Do(event);

    // 首个GET_BUF(BLOCK_MODE)，rlsCount==0，不起屏障作用
    event.type = EventType::BUF_SYNC_EVENT;
    event.eventInfo.bufSyncInfo.opType = SyncType::GET_BUF;
    event.eventInfo.bufSyncInfo.mode = BufMode::BLOCK_MODE;
    event.eventInfo.bufSyncInfo.bufId = 0;
    event.eventInfo.bufSyncInfo.rlsCount = 0U;
    event.serialNo = 2U;
    alg.Do(event);

    // 第二条mem事件：读地址0x50，union已被bufSyncInfo覆盖，需重新设置所有memInfo字段
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo.opType = AccessType::READ;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.serialNo = 3U;
    alg.Do(event);

    ASSERT_EQ(alg.IsFinished(), false);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 1U);
    ASSERT_EQ(alg.IsFinished(), true);
}

// RLS_BUF不阻塞流水线，前后MEM_EVENT应检测到竞争
TEST(SinglePipeRaceAlgImpl, rls_buf_expect_race) {
    SinglePipeRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent event;

    event.loc.coreId = 0;
    event.pipe = PipeType::PIPE_V;
    event.loc.blockType = BlockType::AIVEC;

    // 第一条mem事件：写地址0x50
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.serialNo = 1U;
    alg.Do(event);

    // RLS_BUF不阻塞流水线，不递增barrierNo
    event.type = EventType::BUF_SYNC_EVENT;
    event.eventInfo.bufSyncInfo.opType = SyncType::RLS_BUF;
    event.eventInfo.bufSyncInfo.bufId = 0;
    event.serialNo = 2U;
    alg.Do(event);

    // 第二条mem事件：读地址0x50，union已被bufSyncInfo覆盖，需重新设置所有memInfo字段
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo.opType = AccessType::READ;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0x50;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 1U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.serialNo = 3U;
    alg.Do(event);

    ASSERT_EQ(alg.IsFinished(), false);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 1U);
    ASSERT_EQ(alg.IsFinished(), true);
}

// 同一流水上两次写同一地址，中间无pipe_barrier，应检测到WAW竞争
TEST(SinglePipeRaceAlgImpl, same_pipe_write_write_without_barrier_expect_race) {
    SinglePipeRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent event;

    event.loc.coreId = 0;
    event.pipe = PipeType::PIPE_MTE2;
    event.loc.blockType = BlockType::AIVEC;

    auto fillWrite = [&event](uint64_t serialNo, uint64_t addr) {
        event.type = EventType::MEM_EVENT;
        event.eventInfo.memInfo.opType = AccessType::WRITE;
        event.eventInfo.memInfo.memType = MemType::UB;
        event.eventInfo.memInfo.addr = addr;
        event.eventInfo.memInfo.blockNum = 1U;
        event.eventInfo.memInfo.blockSize = 1024U;
        event.eventInfo.memInfo.blockStride = 1U;
        event.eventInfo.memInfo.repeatTimes = 1U;
        event.eventInfo.memInfo.repeatStride = 1U;
        event.serialNo = serialNo;
    };

    // 两次写同一UB地址，中间没有任何同步
    fillWrite(1U, 0x2e100);
    alg.Do(event);
    fillWrite(2U, 0x2e100);
    alg.Do(event);

    ASSERT_EQ(alg.IsFinished(), false);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 1U);
    ASSERT_EQ(alg.IsFinished(), true);
}

// 同一流水上两次写同一地址，中间有pipe_barrier，不应检测到WAW竞争
TEST(SinglePipeRaceAlgImpl, same_pipe_write_write_with_barrier_expect_no_race) {
    SinglePipeRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    SanEvent event;

    event.loc.coreId = 0;
    event.pipe = PipeType::PIPE_MTE2;
    event.loc.blockType = BlockType::AIVEC;

    auto fillWrite = [&event](uint64_t serialNo, uint64_t addr) {
        event.type = EventType::MEM_EVENT;
        event.eventInfo.memInfo.opType = AccessType::WRITE;
        event.eventInfo.memInfo.memType = MemType::UB;
        event.eventInfo.memInfo.addr = addr;
        event.eventInfo.memInfo.blockNum = 1U;
        event.eventInfo.memInfo.blockSize = 1024U;
        event.eventInfo.memInfo.blockStride = 1U;
        event.eventInfo.memInfo.repeatTimes = 1U;
        event.eventInfo.memInfo.repeatStride = 1U;
        event.serialNo = serialNo;
    };

    // 第一次写
    fillWrite(1U, 0x2e100);
    alg.Do(event);

    // MTE2 的流水内 pipe_barrier
    event.type = EventType::SYNC_EVENT;
    event.eventInfo.syncInfo.opType = SyncType::PIPE_BARRIER;
    event.serialNo = 2U;
    alg.Do(event);

    // 第二次写同一地址
    fillWrite(3U, 0x2e100);
    alg.Do(event);

    ASSERT_EQ(alg.IsFinished(), false);
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 0U);
    ASSERT_EQ(alg.IsFinished(), true);
}

// 端到端验证：MTE2 两次写同一UB地址，中间有 eventid>=8 的 set_flag/wait_flag 同步对，
// 修复前 wait_flag 被 eventId 越界校验丢弃，不会生成 MTE2 的 pipe_barrier，导致 WAW 误报；
// 修复后按硬件语义截断 eventId(8->0,9->1)，同步对正常参与检测，生成 pipe_barrier，不再误报
TEST(SinglePipeRaceAlgImpl, mte2_write_write_with_high_event_id_sync_expect_no_race) {
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;

    auto parseRecord = [&events](const KernelRecord &record) {
        SanitizerRecord sanitizerRecord;
        sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
        sanitizerRecord.payload.kernelRecord = record;
        RecordParse::Parse(sanitizerRecord, events);
    };

    KernelRecord record{};
    record.blockType = BlockType::AIVEC;

    // 第一次 MTE2 写 0x2e100
    record.recordType = RecordType::MOV_ALIGN_V2;
    auto &movAlign = record.payload.movAlignRecordV2;
    movAlign.dst = 0x2e100;
    movAlign.src = 0x120000000000;
    movAlign.dstStride = 1024;
    movAlign.srcStride = 1024;
    movAlign.nBurst = 2;
    movAlign.lenBurst = 1024;
    movAlign.loop1Size = 1;
    movAlign.loop2Size = 1;
    movAlign.location.blockId = 0;
    movAlign.dstMemType = MemType::UB;
    movAlign.srcMemType = MemType::GM;
    movAlign.dataType = DataType::DATA_B16;
    record.serialNo = 100U;
    parseRecord(record);

    // 同步对1：SET_FLAG/WAIT_FLAG MTE2 -> MTE3, eventid 8
    // 注：环的起始点/终点位置取自指令 serialNo，用例必须给出单调递增的 serialNo
    record.recordType = RecordType::SET_FLAG;
    record.payload.syncRecord.location.blockId = 0;
    record.payload.syncRecord.src = PipeType::PIPE_MTE2;
    record.payload.syncRecord.dst = PipeType::PIPE_MTE3;
    record.payload.syncRecord.eventID = 8;
    record.serialNo = 101U;
    parseRecord(record);
    record.recordType = RecordType::WAIT_FLAG;
    record.serialNo = 102U;
    parseRecord(record);

    // 同步对2：SET_FLAG/WAIT_FLAG MTE3 -> MTE2, eventid 8（硬件截断后为通道 0）
    // 注：链根按"边" (src,dst,eventId) 传递；这里两对各自配对并共同构成一次闭环往返
    record.recordType = RecordType::SET_FLAG;
    record.payload.syncRecord.src = PipeType::PIPE_MTE3;
    record.payload.syncRecord.dst = PipeType::PIPE_MTE2;
    record.payload.syncRecord.eventID = 8;
    record.serialNo = 103U;
    parseRecord(record);
    record.recordType = RecordType::WAIT_FLAG;
    record.serialNo = 104U;
    parseRecord(record);

    // 第二次 MTE2 写 0x2e100
    record.recordType = RecordType::MOV_ALIGN_V2;
    movAlign.dst = 0x2e100;
    movAlign.src = 0x120000000800;
    record.serialNo = 110U;
    parseRecord(record);

    // 解析阶段应识别出 MTE2 上的闭环往返（不再注入合成 barrier，而是把"环水位线"打戳到事件上）
    bool ringRecognized = false;
    for (const auto &event : events) {
        if (event.type == EventType::SYNC_EVENT && event.pipe == PipeType::PIPE_MTE2 && event.closedRingStartSerialNo != 0U) {
            ringRecognized = true;
            break;
        }
    }
    ASSERT_TRUE(ringRecognized);

    // 将解析产物喂给单流水竞争检测，两次写同一地址不应再报 WAW
    SinglePipeRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_950DT_950x, 2);
    for (const auto &event : events) {
        alg.Do(event);
    }
    ASSERT_EQ(alg.IsFinished(), false);
    SanEvent finishEvent;
    finishEvent.type = EventType::SANITIZER_CONTROL_EVENT;
    finishEvent.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(finishEvent);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 0U);
    ASSERT_EQ(alg.IsFinished(), true);

    RecordParse::ResetSyncInPipeInfo();
}

// ===== 环保护判定（环区间 + 包含关系）用例 =====
//   环：同一个 pipe 的 set 到 wait 的一次闭合往返。
//   SET 把本 pipe 已继承的起始点并入当前边，WAIT 再并入本 pipe 的继承表；含本 pipe 自身即闭环。
//   只有环区间被两条同 pipe 访存完整包含时才豁免竞争。

// MTE2 的 UB 写（MOV_ALIGN_V2，GM->UB），用于构造同址写对
KernelRecord MakeRingMte2Write(uint64_t dst, uint64_t serialNo) {
    KernelRecord record{};
    record.blockType = BlockType::AIVEC;
    record.recordType = RecordType::MOV_ALIGN_V2;
    auto &movAlign = record.payload.movAlignRecordV2;
    movAlign.dst = dst;
    movAlign.src = 0x120000000000ULL;
    movAlign.dstStride = 1024;
    movAlign.srcStride = 1024;
    movAlign.nBurst = 2;
    movAlign.lenBurst = 1024;
    movAlign.loop1Size = 1;
    movAlign.loop2Size = 1;
    movAlign.location.blockId = 0;
    movAlign.dstMemType = MemType::UB;
    movAlign.srcMemType = MemType::GM;
    movAlign.dataType = DataType::DATA_B16;
    record.serialNo = serialNo;
    return record;
}

KernelRecord MakeRingSync(RecordType type, PipeType src, PipeType dst, uint32_t eventId, uint64_t serialNo) {
    KernelRecord record{};
    record.blockType = BlockType::AIVEC;
    record.recordType = type;
    record.payload.syncRecord.location.blockId = 0;
    record.payload.syncRecord.src = src;
    record.payload.syncRecord.dst = dst;
    record.payload.syncRecord.eventID = eventId;
    record.serialNo = serialNo;
    return record;
}

void ParseRingRecord(const KernelRecord &record, std::vector<SanEvent> &events) {
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
    RecordParse::Parse(sanitizerRecord, events);
}

uint64_t ClosedRingStartSerialNoOf(const std::vector<SanEvent> &events, uint64_t serialNo) {
    for (const auto &event : events) {
        if (event.serialNo == serialNo) {
            return event.closedRingStartSerialNo;
        }
    }
    return 0U;
}

uint32_t RunSinglePipeAlg(const std::vector<SanEvent> &events) {
    SinglePipeRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_950DT_950x, 2);
    for (const auto &event : events) {
        alg.Do(event);
    }
    SanEvent finishEvent;
    finishEvent.type = EventType::SANITIZER_CONTROL_EVENT;
    finishEvent.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(finishEvent);
    return alg.GetRaceCount();
}

// A×C：环两端的两条同址写被完整环保护，不应上报
TEST(SinglePipeRaceAlgImpl, ring_protects_both_ends_expect_no_race) {
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;
    ParseRingRecord(MakeRingMte2Write(0x1000U, 100U), events); // A：环之前
    ParseRingRecord(
        MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 101U), events); // 起始点
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 102U), events);
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 0U, 103U), events);
    ParseRingRecord(
        MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 0U, 104U), events); // 终点
    ParseRingRecord(MakeRingMte2Write(0x1000U, 105U), events); // C：环之后（与 A 同址）

    ASSERT_EQ(ClosedRingStartSerialNoOf(events, 104U), 101U); // 环 [101,104] 已闭环，水位线 = 起始点
    ASSERT_EQ(ClosedRingStartSerialNoOf(events, 105U), 101U); // C 发射时环已闭环
    ASSERT_EQ(RunSinglePipeAlg(events), 0U);
    RecordParse::ResetSyncInPipeInfo();
}

// A×B：B 是环中间的写，A（环之前）与 B 同址 —— 环未被完整包含，应上报
TEST(SinglePipeRaceAlgImpl, ring_middle_vs_before_expect_race) {
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;
    ParseRingRecord(MakeRingMte2Write(0x1000U, 100U), events); // A：环之前
    ParseRingRecord(
        MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 101U), events); // 起始点
    ParseRingRecord(MakeRingMte2Write(0x1000U, 102U), events); // B：环内
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 103U), events);
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 0U, 104U), events);
    ParseRingRecord(
        MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 0U, 105U), events); // 终点

    ASSERT_EQ(ClosedRingStartSerialNoOf(events, 102U), 0U); // B 发射时环尚未闭环
    ASSERT_GE(RunSinglePipeAlg(events), 1U);
    RecordParse::ResetSyncInPipeInfo();
}

// B×C：B（环中间）与 C（环之后）同址 —— 对应现场 95981×96003 的漏报，必须上报
TEST(SinglePipeRaceAlgImpl, ring_middle_vs_after_expect_race) {
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;
    ParseRingRecord(
        MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 101U), events); // 起始点
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 102U), events);
    ParseRingRecord(MakeRingMte2Write(0x1000U, 103U), events); // B：环内
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 0U, 104U), events);
    ParseRingRecord(
        MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 0U, 105U), events); // 终点
    ParseRingRecord(MakeRingMte2Write(0x1000U, 106U), events); // C：环之后（与 B 同址）

    ASSERT_GE(RunSinglePipeAlg(events), 1U);
    RecordParse::ResetSyncInPipeInfo();
}

// 不同 (src,dst) 的边 eventId 可以不同：去程边 (MTE2,MTE1) 用 eid0、回程边 (MTE1,MTE2) 用 eid1，
// 只要每条边内 set/wait 同 eid，往返即可闭合，环两端同址写不应上报
TEST(SinglePipeRaceAlgImpl, ring_closes_across_legs_with_different_eventids_expect_no_race) {
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;
    ParseRingRecord(MakeRingMte2Write(0x1000U, 100U), events);
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 101U), events);
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 102U), events);
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 1U, 103U), events);
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 1U, 104U), events);
    ParseRingRecord(MakeRingMte2Write(0x1000U, 105U), events);

    ASSERT_EQ(ClosedRingStartSerialNoOf(events, 104U), 101U); // 跨 eid 仍闭环，水位线 = 起始点
    ASSERT_EQ(ClosedRingStartSerialNoOf(events, 105U), 101U);
    ASSERT_EQ(RunSinglePipeAlg(events), 0U);
    RecordParse::ResetSyncInPipeInfo();
}

// 同一条边上有两个未闭合起始点 o1(10)、o2(11) 时，聚合取最大起始点，
// 水位线应推进到 o2(11)，而非停在 o1(10)
TEST(SinglePipeRaceAlgImpl, ring_pairing_takes_latest_unclosed_set) {
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;
    ParseRingRecord(
        MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 10U), events); // o1
    ParseRingRecord(
        MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 11U), events); // o2
    ParseRingRecord(
        MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 12U), events); // 取 o2
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 0U, 13U), events);
    ParseRingRecord(
        MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 0U, 14U), events); // 闭环

    ASSERT_EQ(ClosedRingStartSerialNoOf(events, 14U), 11U); // 取最近的 o2，而非队首的 o1
    RecordParse::ResetSyncInPipeInfo();
}

// 同一 (src,dst) 但不同 eventId 的两条边并存（eid0 的 o_stale、eid1 的 o_new）：
// wait 走 eid1 边取到 o_new，水位线应推进到 200 而非停在陈旧的 100
TEST(SinglePipeRaceAlgImpl, ring_pairs_with_latest_open_ignoring_stale_head) {
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;
    ParseRingRecord(
        MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_M, PipeType::PIPE_MTE1, 0U, 100U), events); // 陈旧 o_stale
    ParseRingRecord(
        MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_M, PipeType::PIPE_MTE1, 1U, 200U), events); // 最近 o_new
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_M, PipeType::PIPE_MTE1, 1U, 201U), events);
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_M, 1U, 202U), events);
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_M, 1U, 203U), events);

    ASSERT_EQ(ClosedRingStartSerialNoOf(events, 203U), 200U); // 取最近的 o_new，而非队首的 o_stale
    RecordParse::ResetSyncInPipeInfo();
}

// 现场 FA（AIV）误报根因：同一条边上较旧的 token 携带 dstPipe 的链根、较新的 token 不携带。
// 只挑一个 token（FIFO/LIFO）都会漏识别该环；必须按同一条边 (src,dst,eventId) 聚合所有 token 的链根。
TEST(SinglePipeRaceAlgImpl, ring_aggregates_roots_from_all_pending_tokens)
{
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_V, 0U, 100U), events);
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_V, 0U, 101U), events);
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE2, 0U, 102U), events); // 含 MTE2:100
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE2, 0U, 103U), events); // 不含
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE2, 0U, 104U), events);

    ASSERT_EQ(ClosedRingStartSerialNoOf(events, 104U), 100U); // 聚合保留较旧 token 携带的 MTE2 链根
    RecordParse::ResetSyncInPipeInfo();
}

// 同一 pipe 连续两次 set（真实流水常见 SET,SET,WAIT,WAIT）：第二次 set 必须仍携带该 pipe 已继承的
// 链根，否则回程那一跳断链、该 pipe 的环闭不上（现场 MTE2 数百条误报即此因）。
TEST(SinglePipeRaceAlgImpl, ring_keeps_inherited_root_across_consecutive_sets)
{
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 100U), events);
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 101U), events);
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_M, 0U, 102U), events);
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 0U, 103U), events);
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 0U, 104U), events);

    ASSERT_EQ(ClosedRingStartSerialNoOf(events, 104U), 100U); // 第二次 set 仍带 MTE2:100 -> 环闭合
    RecordParse::ResetSyncInPipeInfo();
}

// 水位线随迭代推进：连续多轮往返时水位线应取最新一轮起始点（取 max），而非停在首轮；
// 否则循环体里后续各轮之间的同址访问会被误报
TEST(SinglePipeRaceAlgImpl, ring_watermark_advances_across_rounds) {
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 10U), events);
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 11U), events);
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 0U, 12U), events);
    ParseRingRecord(
        MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 0U, 13U), events); // 第 1 轮闭环
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 14U), events);
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 15U), events);
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 0U, 16U), events);
    ParseRingRecord(
        MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 0U, 17U), events); // 第 2 轮闭环

    ASSERT_EQ(ClosedRingStartSerialNoOf(events, 13U), 10U); // 第 1 轮：水位线 = 10
    ASSERT_EQ(ClosedRingStartSerialNoOf(events, 17U), 14U); // 第 2 轮：水位线推进到 14（取 max）
    RecordParse::ResetSyncInPipeInfo();
}

// 现场 FA 用例：M↔MTE1 往返中，去程边 (M→MTE1) 用 eid0、回程边 (MTE1→M) 用 eid1；每条边内 set/wait 同 eid。
// 环应闭合在 M 上（水位线 = 起始点 95169），保护 M 上同址的 95168×95177。
// 护栏：跨边 eid 不同不影响闭环（边由 (src,dst,eventId) 确定）。
TEST(SinglePipeRaceAlgImpl, ring_closes_when_leg_eventids_differ_expect_no_race) {
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_M, PipeType::PIPE_MTE1, 0U, 95169U), events);
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 95170U), events);
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 95171U), events);
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_M, PipeType::PIPE_MTE1, 0U, 95172U), events);
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_M, 1U, 95175U), events);
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_M, 1U, 95176U), events);

    ASSERT_EQ(ClosedRingStartSerialNoOf(events, 95176U), 95169U);
    RecordParse::ResetSyncInPipeInfo();
}

// 三方环：MTE2→MTE1→MTE3→MTE2，每跳 set 都在其 wait 之后，整条链闭合在 MTE2 上 → 环 [1,6]
// 护栏：配对不能收窄成"两腿方向对"，否则会丢掉长度 >=3 的环
TEST(SinglePipeRaceAlgImpl, transitive_three_pipe_ring_is_recognized) {
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 1U), events);
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 2U), events);
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE3, 0U, 3U), events);
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE3, 0U, 4U), events);
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE3, PipeType::PIPE_MTE2, 0U, 5U), events);
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE3, PipeType::PIPE_MTE2, 0U, 6U), events);

    ASSERT_EQ(ClosedRingStartSerialNoOf(events, 6U), 1U);
    RecordParse::ResetSyncInPipeInfo();
}

// 一次往返的两条 leg 所在 pipe 都应识别出环（等价于旧"两条 pipe 都命中"的要求）
TEST(SinglePipeRaceAlgImpl, pairwise_round_trip_recognized_on_both_pipes) {
    RecordParse::ResetSyncInPipeInfo();
    std::vector<SanEvent> events;
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 1U), events);
    ParseRingRecord(MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 2U), events);
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 0U, 3U), events);
    ParseRingRecord(
        MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_MTE2, 0U, 4U), events); // MTE2 环 [1,4]
    ParseRingRecord(MakeRingSync(RecordType::SET_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 5U), events);
    ParseRingRecord(
        MakeRingSync(RecordType::WAIT_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1, 0U, 6U), events); // MTE1 环 [3,6]

    ASSERT_EQ(ClosedRingStartSerialNoOf(events, 4U), 1U);
    ASSERT_EQ(ClosedRingStartSerialNoOf(events, 6U), 3U);
    RecordParse::ResetSyncInPipeInfo();
}
}
