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

    KernelRecord movAlignRecord{};
    movAlignRecord.blockType = BlockType::AIVEC;
    movAlignRecord.recordType = RecordType::MOV_ALIGN_V2;

    // 第一次 MTE2 写 0x2e100
    auto &movAlign = movAlignRecord.payload.movAlignRecordV2;
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
    parseRecord(movAlignRecord);

    KernelRecord syncRecord{};
    syncRecord.blockType = BlockType::AIVEC;

    // 同步对1：SET_FLAG/WAIT_FLAG MTE2 -> MTE3, eventid 8
    syncRecord.recordType = RecordType::SET_FLAG;
    syncRecord.payload.syncRecord.location.blockId = 0;
    syncRecord.payload.syncRecord.src = PipeType::PIPE_MTE2;
    syncRecord.payload.syncRecord.dst = PipeType::PIPE_MTE3;
    syncRecord.payload.syncRecord.eventID = 8;
    parseRecord(syncRecord);
    syncRecord.recordType = RecordType::WAIT_FLAG;
    parseRecord(syncRecord);

    // 同步对2：SET_FLAG/WAIT_FLAG MTE3 -> MTE2, eventid 9
    syncRecord.recordType = RecordType::SET_FLAG;
    syncRecord.payload.syncRecord.src = PipeType::PIPE_MTE3;
    syncRecord.payload.syncRecord.dst = PipeType::PIPE_MTE2;
    syncRecord.payload.syncRecord.eventID = 9;
    parseRecord(syncRecord);
    syncRecord.recordType = RecordType::WAIT_FLAG;
    parseRecord(syncRecord);

    // 第二次 MTE2 写 0x2e100
    movAlign.dst = 0x2e100;
    movAlign.src = 0x120000000800;
    parseRecord(movAlignRecord);

    // 解析阶段应生成 MTE2 的流水内 pipe_barrier
    bool findMte2Barrier = false;
    for (const auto &event : events) {
        if (event.type == EventType::SYNC_EVENT && event.pipe == PipeType::PIPE_MTE2 &&
            event.eventInfo.syncInfo.opType == SyncType::PIPE_BARRIER) {
            findMte2Barrier = true;
            break;
        }
    }
    ASSERT_TRUE(findMte2Barrier);

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
    ASSERT_EQ(alg.GetRaceCount(), 0U);
    ASSERT_EQ(alg.IsFinished(), true);

    RecordParse::ResetSyncInPipeInfo();
}
}
