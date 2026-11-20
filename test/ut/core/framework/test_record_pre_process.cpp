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
#include <future>
#include <memory>
#include <mutex>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "event_def.h"
#include "record_parse.h"
#define private public
#include "record_pre_process.h"
#undef private
#include "runtime_context.h"

using namespace Sanitizer;

namespace {
SanitizerRecord MakePreProcessRecord(RecordType type, uint64_t serialNo = 0) {
    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    auto &kernel = record.payload.kernelRecord;
    kernel.recordType = type;
    kernel.blockType = BlockType::AIVEC;
    kernel.serialNo = serialNo;
    if (type == RecordType::LOAD || type == RecordType::STORE) {
        kernel.payload.loadStoreRecord.addr = 0x1000 + serialNo;
        kernel.payload.loadStoreRecord.size = 1;
        kernel.payload.loadStoreRecord.alignSize = 1;
        kernel.payload.loadStoreRecord.space = AddressSpace::GM;
    }
    return record;
}

SanitizerRecord MakePreProcessWait(uint64_t addr, uint32_t core = 0) {
    auto record = MakePreProcessRecord(RecordType::MSTX_STUB);
    auto &mstx = record.payload.kernelRecord.payload.mstxRecord;
    mstx.interfaceType = InterfaceType::MSTX_WAIT_CROSS_SYNC;
    mstx.bufferLens = sizeof(MstxCrossRecord);
    mstx.location.blockId = core;
    mstx.interface.mstxCrossRecord.addr = addr;
    mstx.interface.mstxCrossRecord.pipe = PipeType::PIPE_V;
    mstx.interface.mstxCrossRecord.flagId = 2;
    mstx.interface.mstxCrossRecord.isMore = true;
    return record;
}

SanitizerRecord MakePreProcessSync(RecordType type, uint64_t serialNo, PipeType src, PipeType dst) {
    auto record = MakePreProcessRecord(type, serialNo);
    auto &sync = record.payload.kernelRecord.payload.syncRecord;
    sync.src = src;
    sync.dst = dst;
    sync.eventID = 3;
    sync.location.blockId = 1;
    sync.location.fileNo = 2;
    sync.location.lineNo = serialNo;
    return record;
}

} // namespace

TEST(RecordPreProcess, records_match_staging_with_allocation_only_for_repeated_wait_groups) {
    RuntimeContext::Instance().deviceSummary_.device = DeviceType::ASCEND_910B1;
    for (auto finishType : {RecordType::BLOCK_FINISH, RecordType::KERNEL_FINISH}) {
        SCOPED_TRACE(static_cast<int>(finishType));
        RecordPreProcess direct;
        RecordPreProcess staged;
        constexpr size_t ordinaryRecordCount = 10;
        const std::vector<SanitizerRecord> records{MakePreProcessRecord(RecordType::LOAD, 17),
            MakePreProcessRecord(RecordType::STORE, 17),
            MakePreProcessSync(RecordType::SET_FLAG, 11, PipeType::PIPE_V, PipeType::PIPE_MTE2),
            MakePreProcessSync(RecordType::WAIT_FLAG, 12, PipeType::PIPE_V, PipeType::PIPE_MTE2),
            MakePreProcessSync(RecordType::SET_FLAG, 13, PipeType::PIPE_MTE2, PipeType::PIPE_V),
            MakePreProcessSync(RecordType::WAIT_FLAG, 14, PipeType::PIPE_MTE2, PipeType::PIPE_V),
            MakePreProcessRecord(RecordType::PIPE_BARRIER, 17),
            MakePreProcessRecord(RecordType::BLOCK_FINISH, 17), MakePreProcessRecord(RecordType::KERNEL_FINISH, 17),
            MakePreProcessRecord(RecordType::LOAD, 17), MakePreProcessRecord(RecordType::LOAD, 1),
            MakePreProcessWait(0x300), MakePreProcessRecord(RecordType::STORE, 2), MakePreProcessWait(0x400),
            MakePreProcessWait(0x300), MakePreProcessRecord(RecordType::LOAD, 3), MakePreProcessWait(0x400),
            MakePreProcessWait(0x500), MakePreProcessWait(0x500, 1), MakePreProcessRecord(finishType),
            MakePreProcessRecord(RecordType::LOAD, 4)};
        std::vector<std::vector<SanEvent>> directEvents(records.size());
        std::vector<std::tuple<size_t, bool, bool, bool>> directStates;
        // Each trace retains its own parser history; instances share thread-local RecordParse state.
        RecordParse::ResetAll();
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(i);
            direct.Process(records[i], directEvents[i]);
            directStates.emplace_back(direct.recordBuffer_.size(), direct.waitMergeInfo_.endTag,
                direct.waitMergeInfo_.throughFirstWait, direct.waitMergeInfo_.throughSecondWait);
            if (i < ordinaryRecordCount) {
                EXPECT_EQ(direct.recordBuffer_.capacity(), 0U);
                EXPECT_TRUE(direct.waitMergeInfo_.endTag);
                EXPECT_FALSE(direct.waitMergeInfo_.throughFirstWait);
            } else if (i > ordinaryRecordCount && i < ordinaryRecordCount + 7) {
                EXPECT_TRUE(directEvents[i].empty());
                EXPECT_FALSE(direct.recordBuffer_.empty());
            }
        }
        EXPECT_FALSE(direct.waitMergeInfo_.throughFirstWait);
        EXPECT_FALSE(direct.waitMergeInfo_.throughSecondWait);
        EXPECT_TRUE(direct.recordBuffer_.empty());

        RecordParse::ResetAll();
        std::vector<std::vector<SanEvent>> stagedEvents(records.size());
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(i);
            // Original push/parse/clear path, outside soft-sync merge regions.
            staged.ProcessMstxCrossWaitRecord(records[i]);
            if (staged.waitMergeInfo_.endTag) {
                for (const auto &record : staged.recordBuffer_) {
                    staged.Parse(record, stagedEvents[i]);
                }
                staged.ClearBuffer();
            }
            EXPECT_EQ(directStates[i], std::make_tuple(staged.recordBuffer_.size(), staged.waitMergeInfo_.endTag,
                staged.waitMergeInfo_.throughFirstWait, staged.waitMergeInfo_.throughSecondWait));
        }
        RecordParse::ResetAll();
        ASSERT_EQ(directEvents[5].size(), 1U);
        EXPECT_EQ(directEvents[5][0].closedRingStartSerialNo, 11U);
        size_t totalWaits = 0;
        for (size_t i = 0; i < records.size(); ++i) {
            SCOPED_TRACE(i);
            const auto &actual = directEvents[i];
            const auto &expected = stagedEvents[i];
            ASSERT_EQ(actual.size(), expected.size());
            for (size_t j = 0; j < actual.size(); ++j) {
                SCOPED_TRACE(j);
                const auto &a = actual[j];
                const auto &e = expected[j];
                EXPECT_EQ(std::tie(a.type, a.serialNo, a.pipe), std::tie(e.type, e.serialNo, e.pipe));
                EXPECT_EQ(a.recordType, e.recordType);
                EXPECT_EQ(a.closedRingStartSerialNo, e.closedRingStartSerialNo);
                EXPECT_EQ(std::tie(a.loc.fileNo, a.loc.lineNo, a.loc.pc, a.loc.deviceIdx, a.loc.kernelIdx,
                    a.loc.deviceId, a.loc.coreId, a.loc.blockType),
                    std::tie(e.loc.fileNo, e.loc.lineNo, e.loc.pc, e.loc.deviceIdx, e.loc.kernelIdx,
                    e.loc.deviceId, e.loc.coreId, e.loc.blockType));
                if (a.type == EventType::MEM_EVENT) {
                    EXPECT_EQ(a.eventInfo.memInfo.addr, e.eventInfo.memInfo.addr);
                    EXPECT_EQ(a.eventInfo.memInfo.opType, e.eventInfo.memInfo.opType);
                    EXPECT_EQ(a.eventInfo.memInfo.memType, e.eventInfo.memInfo.memType);
                } else if (a.type == EventType::SYNC_EVENT) {
                    const auto &as = a.eventInfo.syncInfo;
                    const auto &es = e.eventInfo.syncInfo;
                    EXPECT_EQ(std::tie(as.opType, as.srcPipe, as.dstPipe, as.eventId, as.memType,
                        as.isRetrogress, as.isGenerated),
                        std::tie(es.opType, es.srcPipe, es.dstPipe, es.eventId, es.memType,
                        es.isRetrogress, es.isGenerated));
                } else if (a.type == EventType::MSTX_CROSS_SYNC_EVENT) {
                    EXPECT_EQ(a.eventInfo.mstxCrossInfo.addr, e.eventInfo.mstxCrossInfo.addr);
                    EXPECT_EQ(a.eventInfo.mstxCrossInfo.flagId, e.eventInfo.mstxCrossInfo.flagId);
                    EXPECT_EQ(a.eventInfo.mstxCrossInfo.isMore, e.eventInfo.mstxCrossInfo.isMore);
                    EXPECT_EQ(a.eventInfo.mstxCrossInfo.isMerge, e.eventInfo.mstxCrossInfo.isMerge);
                    EXPECT_EQ(a.eventInfo.mstxCrossInfo.pipe, e.eventInfo.mstxCrossInfo.pipe);
                    EXPECT_EQ(a.eventInfo.mstxCrossInfo.opType, e.eventInfo.mstxCrossInfo.opType);
                    ++totalWaits;
                } else if (a.type == EventType::SANITIZER_CONTROL_EVENT) {
                    EXPECT_EQ(a.eventInfo.sanitizerControlInfo.type, e.eventInfo.sanitizerControlInfo.type);
                }
            }
        }
        EXPECT_EQ(totalWaits, 4U);
    }
}

TEST(RecordPreProcess, direct_parse_preserves_soft_sync_filtering_and_finish_resets) {
    RuntimeContext::Instance().deviceSummary_.device = DeviceType::ASCEND_910B1;
    for (auto finishType : {RecordType::BLOCK_FINISH, RecordType::KERNEL_FINISH}) {
        RecordPreProcess processor;
        std::vector<SanEvent> events;
        processor.Process(MakePreProcessRecord(RecordType::SYNC_ALL_STUB), events);
        EXPECT_TRUE(processor.mstxMergeTag_);
        EXPECT_TRUE(processor.isFirstSyncAllStub_);
        events.clear();
        processor.Process(MakePreProcessRecord(RecordType::LOAD, 1), events);
        EXPECT_TRUE(events.empty());
        processor.Process(MakePreProcessRecord(RecordType::PIPE_BARRIER), events);
        EXPECT_FALSE(events.empty());
        events.clear();
        processor.Process(MakePreProcessRecord(finishType), events);
        EXPECT_FALSE(processor.mstxMergeTag_);
        EXPECT_FALSE(processor.isFirstSyncAllStub_);
        events.clear();
        processor.Process(MakePreProcessRecord(RecordType::LOAD, 2), events);
        ASSERT_EQ(events.size(), 1U);
        EXPECT_EQ(events[0].serialNo, 2U);
        EXPECT_EQ(processor.recordBuffer_.capacity(), 0U);
    }
}
