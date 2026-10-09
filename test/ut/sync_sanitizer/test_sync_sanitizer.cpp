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
#include <any>
#include <mutex>
#include "sanitizer_base.h"
#include "record_pre_process.h"
#define private public
#include "sync_sanitizer.h"
#undef private

namespace {
using namespace Sanitizer;

auto g_fillSyncRecord = [](SanitizerRecord& record, uint16_t coreId = 0U, RecordType type = RecordType::SET_FLAG,
    PipeType srcPipe = PipeType::PIPE_V, PipeType dstPipe = PipeType::PIPE_MTE1, EventID eventId = EventID::EVENT_ID0) {
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = type;
    auto& syncRecord = record.payload.kernelRecord.payload.syncRecord;
    syncRecord.location.blockId = coreId;
    syncRecord.src = srcPipe;
    syncRecord.dst = dstPipe;
    syncRecord.eventID = static_cast<uint64_t>(eventId);
};

TEST(SyncSanitizer, unpaired_set_flag_instruction_expect_return_synccheck_detection)
{
    Config config {};
    config.syncCheck = true;
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    auto syncSan = SanitizerFactory::GetInstance().Create(ToolType::SYNCCHECK);
    ASSERT_FALSE(syncSan->SetDeviceInfo(deviceInfoSummary, config));

    SanitizerRecord record {};
    g_fillSyncRecord(record);
    std::vector<SanEvent> events {};
    RecordPreProcess::GetInstance().Process(record, events);
    std::string msg {};
    syncSan->RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    ASSERT_TRUE(syncSan->CheckRecordBeforeProcess(record));

    SanEvent event {};
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan->Do(record, events);
    ASSERT_TRUE(msg.find("Unpaired set_flag instructions detected") != std::string::npos);
}

TEST(SyncSanitizer, unpaired_set_flag_instructions_on_different_blocks_expect_all_blocks)
{
    SanitizerRecord record {};
    std::vector<SanEvent> events {};
    g_fillSyncRecord(record, 0U);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 1U);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 2U);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 3U);

    SyncSanitizer syncSan {};
    std::string msg {};
    syncSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanEvent event {};
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    ASSERT_EQ(syncSan.syncEvents_.size(), 3U);
    ASSERT_TRUE(msg.find("in block aiv(0) on device 0") != std::string::npos);
    ASSERT_TRUE(msg.find("in block aiv(1) on device 0") != std::string::npos);
    ASSERT_TRUE(msg.find("in block aiv(2) on device 0") != std::string::npos);
}

TEST(SyncSanitizer, unpaired_set_flag_instructions_on_different_blocks_expect_specified_block)
{
    SanitizerRecord record {};
    std::vector<SanEvent> events {};
    g_fillSyncRecord(record, 0U);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 1U);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 2U);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 3U);

    SyncSanitizer syncSan {};
    syncSan.checkBlockId_ = 1U;
    std::string msg {};
    syncSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanEvent event {};
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    ASSERT_EQ(syncSan.syncEvents_.size(), 1U);
    ASSERT_TRUE(msg.find("in block aiv(0) on device 0") == std::string::npos);
    ASSERT_TRUE(msg.find("in block aiv(1) on device 0") != std::string::npos);
    ASSERT_TRUE(msg.find("in block aiv(2) on device 0") == std::string::npos);
}

TEST(SyncSanitizer, multi_instructions_with_no_paired_flag)
{
    SanitizerRecord record {};
    std::vector<SanEvent> events {};
    g_fillSyncRecord(record, 0U, RecordType::SET_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 1U, RecordType::WAIT_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_V, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID1);
    RecordPreProcess::GetInstance().Process(record, events);

    SyncSanitizer syncSan {};
    std::string msg {};
    syncSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanEvent event {};
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    ASSERT_EQ(syncSan.syncEvents_.size(), 4U);
    ASSERT_TRUE(msg.find("from PIPE_V to PIPE_MTE1") != std::string::npos);
    ASSERT_TRUE(msg.find("in block aiv(0) on device 0") != std::string::npos);
}

TEST(SyncSanitizer, multi_instructions_with_one_paired_flag)
{
    SanitizerRecord record {};
    std::vector<SanEvent> events {};
    g_fillSyncRecord(record, 0U, RecordType::SET_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 1U, RecordType::WAIT_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_V, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);

    SyncSanitizer syncSan {};
    std::string msg {};
    syncSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanEvent event {};
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    ASSERT_EQ(syncSan.syncEvents_.size(), 2U);
    ASSERT_TRUE(msg.find("Unpaired set_flag instructions detected") == std::string::npos);
}

void syncSanitizerTestClear(std::vector<SanEvent> &events, SyncSanitizer &syncSan, std::string &msg)
{
    events.clear();
    syncSan.syncEvents_.clear();
    syncSan.pipeRedundancyEvents_.clear();
    syncSan.redundancyInfo_.clear();
    msg = "";
}

TEST(SyncSanitizer, redundancy_set_flag_wait_flag_instruction_expect_return_synccheck_detection)
{
    SanitizerRecord record {};
    std::vector<SanEvent> events {};
    SyncSanitizer syncSan {};
    std::string msg {};
    syncSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanEvent event {};

    // set_flag
    g_fillSyncRecord(record, 0U);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 1U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 2U);

    ASSERT_TRUE(msg.find("Redundant set_flag instructions detected") != std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);

    // wait_flag
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 1U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 2U);

    ASSERT_TRUE(msg.find("Redundant wait_flag instructions detected") != std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);
}

TEST(SyncSanitizer, redundancy_continuous_set_flag_wait_flag_instruction_expect_return_multiple_synccheck_detection)
{
    SanitizerRecord record {};
    std::vector<SanEvent> events {};
    SyncSanitizer syncSan {};
    std::string msg {};
    syncSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanEvent event {};

    // set_flag
    g_fillSyncRecord(record, 0U);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 1U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 4U);

    ASSERT_TRUE(msg.find("Redundant set_flag instructions detected") != std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);

    // wait_flag
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 1U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 4U);

    ASSERT_TRUE(msg.find("Redundant wait_flag instructions detected") != std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);
}

TEST(SyncSanitizer, redundancy_set_flag_wait_flag_with_pipe_barrier_instruction_expect_return_synccheck_detection)
{
    SanitizerRecord record {};
    std::vector<SanEvent> events {};
    SyncSanitizer syncSan {};
    std::string msg {};
    syncSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanEvent event {};

    // set_flag
    g_fillSyncRecord(record, 0U);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::PIPE_BARRIER);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 1U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 2U);

    ASSERT_TRUE(msg.find("Redundant set_flag instructions detected") != std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);

    // wait_flag
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::PIPE_BARRIER);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 1U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 2U);

    ASSERT_TRUE(msg.find("Redundant wait_flag instructions detected") != std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);
}

TEST(SyncSanitizer, redundancy_set_flag_wait_flag_with_sync_instruction_expect_no_synccheck_detection)
{
    SanitizerRecord record {};
    std::vector<SanEvent> events {};
    SyncSanitizer syncSan {};
    std::string msg {};
    syncSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanEvent event {};

    // set_flag
    g_fillSyncRecord(record, 0U);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_V);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 1U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 0U);

    ASSERT_TRUE(msg.find("Redundant set_flag instructions detected") == std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);

    // wait_flag
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::SET_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_V);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 1U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 0U);

    ASSERT_TRUE(msg.find("Redundant wait_flag instructions detected") == std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);
}

TEST(SyncSanitizer, redundancy_set_flag_wait_flag_with_mem_instruction_expect_no_synccheck_detection)
{
    SanitizerRecord record {};
    std::vector<SanEvent> events {};
    SyncSanitizer syncSan {};
    std::string msg {};
    syncSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanEvent event {};

    // set_flag
    g_fillSyncRecord(record, 0U, RecordType::SET_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1);
    RecordPreProcess::GetInstance().Process(record, events);

    KernelRecord loadStoreRecord{};
    loadStoreRecord.recordType = RecordType::DMA_MOV_CONV_RELU;
    loadStoreRecord.payload.dmaMovConvReluRecord.dst = 0xaa;
    loadStoreRecord.payload.dmaMovConvReluRecord.src = 0x55;
    loadStoreRecord.payload.dmaMovConvReluRecord.nBurst = 100;
    loadStoreRecord.payload.dmaMovConvReluRecord.lenBurst = 8;
    loadStoreRecord.payload.dmaMovConvReluRecord.srcStride = 8;
    loadStoreRecord.payload.dmaMovConvReluRecord.dstStride = 8;
    loadStoreRecord.payload.dmaMovConvReluRecord.location.blockId = 7;
    loadStoreRecord.payload.dmaMovConvReluRecord.crMode = ConvRelu::CRMODE_NONE;
    loadStoreRecord.payload.dmaMovConvReluRecord.srcMemType = MemType::L0C;
    loadStoreRecord.payload.dmaMovConvReluRecord.dstMemType = MemType::UB;
    loadStoreRecord.payload.dmaMovConvReluRecord.srcDataType = DataType::DATA_B16;
    loadStoreRecord.payload.dmaMovConvReluRecord.dstDataType = DataType::DATA_B16;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = loadStoreRecord;

    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 0U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 0U);

    ASSERT_TRUE(msg.find("Redundant set_flag instructions detected") == std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);

    // wait_flag
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_MTE1, PipeType::PIPE_V);
    RecordPreProcess::GetInstance().Process(record, events);
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 0U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 0U);

    ASSERT_TRUE(msg.find("Redundant wait_flag instructions detected") == std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);
}

TEST(SyncSanitizer, redundancy_set_flag_wait_flag_dst_pipe_diff_instruction_expect_no_synccheck_detection)
{
    SanitizerRecord record {};
    std::vector<SanEvent> events {};
    SyncSanitizer syncSan {};
    std::string msg {};
    syncSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanEvent event {};

    // set_flag
    g_fillSyncRecord(record, 0U);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::SET_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE2);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 1U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 0U);

    ASSERT_TRUE(msg.find("Redundant set_flag instructions detected") == std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);

    // wait_flag
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE2);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 2U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 0U);

    ASSERT_TRUE(msg.find("Redundant wait_flag instructions detected") == std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);
}

TEST(SyncSanitizer, redundancy_set_flag_wait_flag_src_pipe_diff_instruction_expect_no_synccheck_detection)
{
    SanitizerRecord record {};
    std::vector<SanEvent> events {};
    SyncSanitizer syncSan {};
    std::string msg {};
    syncSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanEvent event {};

    // set_flag
    g_fillSyncRecord(record, 0U);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::SET_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 2U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 0U);

    ASSERT_TRUE(msg.find("Redundant set_flag instructions detected") == std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);

    // wait_flag
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE1);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 1U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 0U);

    ASSERT_TRUE(msg.find("Redundant wait_flag instructions detected") == std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);
}

TEST(SyncSanitizer, redundancy_set_flag_wait_flag_src_pipe_diff_dst_pipe_diff_instruction_expect_no_synccheck_detection)
{
    SanitizerRecord record {};
    std::vector<SanEvent> events {};
    SyncSanitizer syncSan {};
    std::string msg {};
    syncSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanEvent event {};

    // set_flag
    g_fillSyncRecord(record, 0U);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::SET_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE3);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 2U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 0U);

    ASSERT_TRUE(msg.find("Redundant set_flag instructions detected") == std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);

    // wait_flag
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE3);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 2U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 0U);

    ASSERT_TRUE(msg.find("Redundant wait_flag instructions detected") == std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);
}

TEST(SyncSanitizer, redundancy_set_flag_wait_flag_blockid_diff_instruction_expect_no_synccheck_detection)
{
    SanitizerRecord record {};
    std::vector<SanEvent> events {};
    SyncSanitizer syncSan {};
    std::string msg {};
    syncSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanEvent event {};

    // set_flag
    g_fillSyncRecord(record, 0U);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 1U);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 1U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 0U);

    ASSERT_TRUE(msg.find("Redundant set_flag instructions detected") == std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);

    // wait_flag
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 1U, RecordType::WAIT_FLAG);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 1U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 0U);

    ASSERT_TRUE(msg.find("Redundant wait_flag instructions detected") == std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);
}

TEST(SyncSanitizer, redundancy_set_flag_wait_flag_eventid_diff_instruction_expect_no_synccheck_detection)
{
    SanitizerRecord record {};
    std::vector<SanEvent> events {};
    SyncSanitizer syncSan {};
    std::string msg {};
    syncSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanEvent event {};

    // set_flag
    g_fillSyncRecord(record, 0U, RecordType::SET_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::SET_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID1);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::SET_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 1U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 0U);

    ASSERT_TRUE(msg.find("Redundant set_flag instructions detected") == std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);

    // wait_flag
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID1);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);

    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(event);
    syncSan.Do(record, events);
    std::cout << msg << std::endl;

    ASSERT_EQ(syncSan.pipeRedundancyEvents_.size(), 1U);
    ASSERT_EQ(syncSan.redundancyInfo_.size(), 0U);

    ASSERT_TRUE(msg.find("Redundant wait_flag instructions detected") == std::string::npos);
    syncSanitizerTestClear(events, syncSan, msg);
}

void InitSyncSanForStuckTest(SyncSanitizer &syncSan, std::string &msg, uint32_t blockDim = 1U) {
    syncSan.deviceType_ = DeviceType::ASCEND_910B1;
    KernelSummary ks{};
    ks.kernelType = KernelType::AIVEC;
    ks.blockDim = blockDim;
    syncSan.Init(ks);
    syncSan.RegisterNotifyFunc([&msg](LogLv const &, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
}

// 填充 MSTX 跨核 set/wait 上报记录（按 interfaceType 选择对应的 union 成员）
auto g_fillMstxCrossCoreFlagRecord = [](SanitizerRecord &record, uint32_t coreId, InterfaceType type, int32_t eventId,
                                         int32_t peerCoreId, bool pipeBarrierAll = false) {
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = RecordType::MSTX_STUB;
    auto &mstxRecord = record.payload.kernelRecord.payload.mstxRecord;
    mstxRecord.interfaceType = type;
    mstxRecord.bufferLens = sizeof(MstxCrossCoreWaitFlag);
    mstxRecord.location.blockId = coreId;
    mstxRecord.error = false;
    if (type == InterfaceType::MSTX_CROSS_CORE_SET_FLAG) {
        auto &flag = mstxRecord.interface.mstxCrossCoreSetFlag;
        flag.eventId = eventId;
        flag.peerCoreId = peerCoreId;
        flag.pipeBarrierAll = pipeBarrierAll;
    } else {
        auto &flag = mstxRecord.interface.mstxCrossCoreWaitFlag;
        flag.eventId = eventId;
        flag.peerCoreId = peerCoreId;
        flag.pipeBarrierAll = pipeBarrierAll;
    }
};

void PushKernelFinish(std::vector<SanEvent> &events) {
    SanEvent e{};
    e.type = EventType::SANITIZER_CONTROL_EVENT;
    e.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    events.emplace_back(e);
}

TEST(SyncSanitizer, stuck_set_wait_pairing_expect_no_stuck_err) {
    SyncSanitizer syncSan{};
    std::string msg{};
    InitSyncSanForStuckTest(syncSan, msg);

    SanitizerRecord record{};
    std::vector<SanEvent> events;
    g_fillSyncRecord(record, 0U, RecordType::SET_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);
    PushKernelFinish(events);

    syncSan.Do(record, events);

    ASSERT_TRUE(syncSan.stuckEvents_.empty());
    ASSERT_TRUE(msg.find("kernel locked up") == std::string::npos);
}

TEST(SyncSanitizer, stuck_wait_flag_without_set_flag_expect_stuck_err) {
    SyncSanitizer syncSan{};
    std::string msg{};
    InitSyncSanForStuckTest(syncSan, msg);

    SanitizerRecord record{};
    std::vector<SanEvent> events;
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);
    PushKernelFinish(events);

    syncSan.Do(record, events);

    ASSERT_FALSE(syncSan.stuckEvents_.empty());
    ASSERT_TRUE(msg.find("kernel locked up") != std::string::npos);
}

TEST(SyncSanitizer, stuck_multiple_wait_flags_some_unpaired_expect_stuck_err) {
    SyncSanitizer syncSan{};
    std::string msg{};
    InitSyncSanForStuckTest(syncSan, msg);

    SanitizerRecord record{};
    std::vector<SanEvent> events;
    // SET + WAIT 配对成功
    g_fillSyncRecord(record, 0U, RecordType::SET_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_V, PipeType::PIPE_MTE1, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);
    // 无配对 SET 的 WAIT → 卡死
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_MTE2, PipeType::PIPE_MTE3, EventID::EVENT_ID1);
    RecordPreProcess::GetInstance().Process(record, events);
    PushKernelFinish(events);

    syncSan.Do(record, events);

    ASSERT_FALSE(syncSan.stuckEvents_.empty());
    ASSERT_TRUE(msg.find("kernel locked up") != std::string::npos);
}

TEST(SyncSanitizer, stuck_wait_before_its_set_in_same_core_expect_no_stuck_err) {
    // 同一核内 wait 排在它的 set 之前：多流水并行时属于正常形态（WAIT 只阻塞目的 PIPE，
    // 生产者 PIPE 会继续推进并置位）。回放必须把事件从 PIPE_S 迁移到各自 PIPE 再并发推进，
    // 否则该 SET 永远无法被消费 → 误报卡死。
    SyncSanitizer syncSan{};
    std::string msg{};
    InitSyncSanForStuckTest(syncSan, msg);

    SanitizerRecord record{};
    std::vector<SanEvent> events;
    // MTE2 先等待 MTE3 置位
    g_fillSyncRecord(record, 0U, RecordType::WAIT_FLAG, PipeType::PIPE_MTE3, PipeType::PIPE_MTE2, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);
    // MTE3 稍后才 set（同一个 eventId/pipe 对）
    g_fillSyncRecord(record, 0U, RecordType::SET_FLAG, PipeType::PIPE_MTE3, PipeType::PIPE_MTE2, EventID::EVENT_ID0);
    RecordPreProcess::GetInstance().Process(record, events);
    PushKernelFinish(events);

    syncSan.Do(record, events);

    ASSERT_TRUE(syncSan.stuckEvents_.empty());
    ASSERT_TRUE(msg.find("kernel locked up") == std::string::npos);
}

// 场景：MSTX 跨核软同步上报（peerCoreId = -1 表示“不限制对端核”），生产者核与消费者核不同。
// 预期：不出现卡死误报。
TEST(SyncSanitizer, stuck_mstx_cross_core_wait_with_unspecified_peer_core_expect_no_stuck_err) {
    SyncSanitizer syncSan{};
    std::string msg{};
    InitSyncSanForStuckTest(syncSan, msg, 2U);

    SanitizerRecord record{};
    std::vector<SanEvent> events;
    // 核 0 上报 set
    g_fillMstxCrossCoreFlagRecord(record, 0U, InterfaceType::MSTX_CROSS_CORE_SET_FLAG, 42, -1);
    RecordPreProcess::GetInstance().Process(record, events);
    // 核 1 上报 wait
    g_fillMstxCrossCoreFlagRecord(record, 1U, InterfaceType::MSTX_CROSS_CORE_WAIT_FLAG, 42, -1);
    RecordPreProcess::GetInstance().Process(record, events);
    PushKernelFinish(events);

    syncSan.Do(record, events);

    ASSERT_TRUE(syncSan.stuckEvents_.empty());
    ASSERT_TRUE(msg.find("kernel locked up") == std::string::npos);
}

// 反向用例：确实没有 set 时必须仍然报卡死，避免修复把“误报”变成“漏报”。
TEST(SyncSanitizer, stuck_mstx_cross_core_wait_with_unspecified_peer_core_without_set_expect_stuck_err) {
    SyncSanitizer syncSan{};
    std::string msg{};
    InitSyncSanForStuckTest(syncSan, msg, 1U);

    SanitizerRecord record{};
    std::vector<SanEvent> events;
    g_fillMstxCrossCoreFlagRecord(record, 0U, InterfaceType::MSTX_CROSS_CORE_WAIT_FLAG, 42, -1);
    RecordPreProcess::GetInstance().Process(record, events);
    PushKernelFinish(events);

    syncSan.Do(record, events);

    ASSERT_FALSE(syncSan.stuckEvents_.empty());
    ASSERT_TRUE(msg.find("kernel locked up") != std::string::npos);
}

} // namespace
