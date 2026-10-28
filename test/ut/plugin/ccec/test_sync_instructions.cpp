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

#include "../ccec_defs.h"
#include "data_process.h"
#include "plugin/ccec/sync_instructions.cpp"

using namespace Sanitizer;

namespace SanitizerTest {

constexpr uint64_t MEM_INFO_SIZE = 1024 * 1024;

TEST(SyncInstructions, dump_hset_flag_expect_get_correct_records)
{
    constexpr HardSyncRecord record = {
        .location = {10, 10},
        /// host 侧 GetBlockIdx 接口始终返回 0，因此标杆中 coreID 应始终为 0
        .src      = PipeType::PIPE_S,
        .dst      = PipeType::PIPE_V,
        .eventID  = static_cast<uint64_t>(EventID::EVENT_ID1),
        .memory   = MemType::GM,
        .v        = 0
    };

    /// 默认开启内存检测，此时同步指令不记录
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report_hset_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, record.eventID, record.memory, record.v);
    __sanitizer_report_hset_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, static_cast<uint64_t>(record.eventID),
                                 record.memory, record.v);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    /// 开启未初始化检测时，此时同步指令不记录
    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_hset_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, record.eventID, record.memory, record.v);
    __sanitizer_report_hset_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, static_cast<uint64_t>(record.eventID),
                                 record.memory, record.v);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    /// 开启竞争检测，此时同步指令会记录
    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_hset_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, record.eventID, record.memory, record.v);
    __sanitizer_report_hset_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, static_cast<uint64_t>(record.eventID),
                                 record.memory, record.v);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::HSET_FLAG>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(HardSyncRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::HSET_FLAG>(ptr, record));
}

TEST(SyncInstructions, dump_hwait_flag_expect_get_correct_records)
{
    constexpr HardSyncRecord record = {
        .location = {10, 10},
        /// host 侧 GetBlockIdx 接口始终返回 0，因此标杆中 coreID 应始终为 0
        .src      = PipeType::PIPE_S,
        .dst      = PipeType::PIPE_V,
        .eventID  = static_cast<uint64_t>(EventID::EVENT_ID1),
        .memory   = MemType::GM,
        .v        = 0
    };

    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report_hwait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, record.eventID, record.memory, record.v);
    __sanitizer_report_hwait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, static_cast<uint64_t>(record.eventID),
                                 record.memory, record.v);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_hwait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, record.eventID, record.memory, record.v);
    __sanitizer_report_hwait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, static_cast<uint64_t>(record.eventID),
                                 record.memory, record.v);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_hwait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, record.eventID, record.memory, record.v);
    __sanitizer_report_hwait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, static_cast<uint64_t>(record.eventID),
                                 record.memory, record.v);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::HWAIT_FLAG>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(HardSyncRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::HWAIT_FLAG>(ptr, record));
}

TEST(SyncInstructions, dump_set_flag_with_racecheck_expect_get_correct_records)
{
    constexpr SyncRecord record = {
        .location = {10, 10},
        /// host 侧 GetBlockIdx 接口始终返回 0，因此标杆中 coreID 应始终为 0
        .src      = PipeType::PIPE_V,
        .dst      = PipeType::PIPE_MTE1,
        .eventID  = 1,
    };

    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.racecheck = true,
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                record.src, record.dst, record.eventID);
    __sanitizer_report_set_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                record.src, record.dst, static_cast<uint64_t>(record.eventID));
    __sanitizer_report_set_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                record.dst, record.eventID);
    __sanitizer_report_set_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                record.dst, static_cast<uint64_t>(record.eventID));

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_FLAG>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(SyncRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_FLAG>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(SyncRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_FLAG>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(SyncRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_FLAG>(ptr, record));
}

TEST(SyncInstructions, dump_set_flag_with_memcheck_and_initcheck_expect_get_none_records)
{
    constexpr SyncRecord record = {
        .location = {10, 10},
        /// host 侧 GetBlockIdx 接口始终返回 0，因此标杆中 coreID 应始终为 0
        .src      = PipeType::PIPE_V,
        .dst      = PipeType::PIPE_MTE1,
        .eventID  = 1,
    };

    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);

    __sanitizer_report_set_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                record.src, record.dst, record.eventID);
    __sanitizer_report_set_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                record.src, record.dst, static_cast<uint64_t>(record.eventID));
    __sanitizer_report_set_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                record.dst, record.eventID);
    __sanitizer_report_set_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                record.dst, static_cast<uint64_t>(record.eventID));
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                record.src, record.dst, record.eventID);
    __sanitizer_report_set_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                record.src, record.dst, static_cast<uint64_t>(record.eventID));
    __sanitizer_report_set_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                record.dst, record.eventID);
    __sanitizer_report_set_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                record.dst, static_cast<uint64_t>(record.eventID));
    ASSERT_EQ(blockHead.recordWriteCount, 0);
}

TEST(SyncInstructions, dump_wait_flag_with_racecheck_expect_get_correct_records)
{
    constexpr SyncRecord record = {
        .location = {10, 10},
        /// host 侧 GetBlockIdx 接口始终返回 0，因此标杆中 coreID 应始终为 0
        .src      = PipeType::PIPE_V,
        .dst      = PipeType::PIPE_MTE1,
        .eventID  = 1,
    };

    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_wait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, record.eventID);
    __sanitizer_report_wait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, static_cast<uint64_t>(record.eventID));
    __sanitizer_report_wait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.dst, record.eventID);
    __sanitizer_report_wait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.dst, static_cast<uint64_t>(record.eventID));

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::WAIT_FLAG>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(SyncRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::WAIT_FLAG>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(SyncRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::WAIT_FLAG>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(SyncRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::WAIT_FLAG>(ptr, record));
}

TEST(SyncInstructions, dump_wait_flag_with_memcheck_and_initcheck_expect_get_none_records)
{
    constexpr SyncRecord record = {
        .location = {10, 10},
        /// host 侧 GetBlockIdx 接口始终返回 0，因此标杆中 coreID 应始终为 0
        .src      = PipeType::PIPE_V,
        .dst      = PipeType::PIPE_MTE1,
        .eventID  = 1,
    };

    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);

    __sanitizer_report_wait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, record.eventID);
    __sanitizer_report_wait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, static_cast<uint64_t>(record.eventID));
    __sanitizer_report_wait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.dst, record.eventID);
    __sanitizer_report_wait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.dst, static_cast<uint64_t>(record.eventID));
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_wait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, record.eventID);
    __sanitizer_report_wait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.src, record.dst, static_cast<uint64_t>(record.eventID));
    __sanitizer_report_wait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.dst, record.eventID);
    __sanitizer_report_wait_flag(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
                                 record.dst, static_cast<uint64_t>(record.eventID));
    ASSERT_EQ(blockHead.recordWriteCount, 0);
}

TEST(SyncInstructions, dump_pipe_barrier_expect_get_correct_records)
{
    constexpr PipeBarrierRecord record = {
        .location = {10, 10},
        /// host 侧 GetBlockIdx 接口始终返回 0，因此标杆中 coreID 应始终为 0
        .pipe     = PipeType::PIPE_V,
    };

    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report_pipe_barrier(memInfo.data(), record.location.fileNo, record.location.lineNo,
                                    record.location.pc, record.pipe);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_pipe_barrier(memInfo.data(), record.location.fileNo, record.location.lineNo,
                                    record.location.pc, record.pipe);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_pipe_barrier(memInfo.data(), record.location.fileNo, record.location.lineNo,
                                    record.location.pc, record.pipe);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::PIPE_BARRIER>(ptr, record));
}

TEST(SyncInstructions, ffts_cross_core_sync_expect_get_correct_records)
{
    FftsSyncRecord record{};
    record.location = {10, 10};
    record.dst = PipeType::PIPE_MTE3;
    record.mode = 0;
    record.flagID = 2;
    uint8_t mode = 0;
    uint8_t id = 2;
    uint64_t config = (0x1 | (mode << 4) | (id << 8));
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report_ffts_cross_core_sync(memInfo.data(), record.location.fileNo, record.location.lineNo,
                                            record.location.pc, record.dst, config);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_ffts_cross_core_sync(memInfo.data(), record.location.fileNo, record.location.lineNo,
                                            record.location.pc, record.dst, config);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_ffts_cross_core_sync(memInfo.data(), record.location.fileNo, record.location.lineNo,
                                            record.location.pc, record.dst, config);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::FFTS_SYNC>(ptr, record));
}

TEST(SyncInstructions, wait_flag_dev_expect_get_correct_records)
{
    WaitFlagDevRecord record{};
    record.location = {10, 10};
    record.flagID = 2;
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report_wait_flag_dev(memInfo.data(), record.location.fileNo, record.location.lineNo,
                                     record.location.pc, 2);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_wait_flag_dev(memInfo.data(), record.location.fileNo, record.location.lineNo,
                                     record.location.pc, 2);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_wait_flag_dev(memInfo.data(), record.location.fileNo, record.location.lineNo,
                                     record.location.pc, 2);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::WAIT_FLAG_DEV>(ptr, record));
}

TEST(SyncInstructions, ib_set_expect_get_correct_records)
{
    SoftSyncRecord record{};
    record.location = { 10, 10 };
    record.waitCoreID = 0;
    record.usedCores = 0;
    record.eventID = 1;
    record.isAIVOnly = true;
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report___ib_set_stub(memInfo.data(), record.location.fileNo, record.location.lineNo,
        record.location.pc, 0, 1, true);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report___ib_set_stub(memInfo.data(), record.location.fileNo, record.location.lineNo,
        record.location.pc, 0, 1, true);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report___ib_set_stub(memInfo.data(), record.location.fileNo, record.location.lineNo,
        record.location.pc, 0, 1, true);
    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::IB_SET_STUB>(ptr, record));
}

TEST(SyncInstructions, ib_wait_expect_get_correct_records)
{
    SoftSyncRecord record{};
    record.location = { 10, 10 };
    record.waitCoreID = 0;
    record.usedCores = 0;
    record.eventID = 1;
    record.isAIVOnly = true;
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report___ib_wait_stub(memInfo.data(), record.location.fileNo, record.location.lineNo,
        record.location.pc, 0, 1, true);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report___ib_wait_stub(memInfo.data(), record.location.fileNo, record.location.lineNo,
        record.location.pc, 0, 1, true);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report___ib_wait_stub(memInfo.data(), record.location.fileNo, record.location.lineNo,
        record.location.pc, 0, 1, true);
    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::IB_WAIT_STUB>(ptr, record));
}

TEST(SyncInstructions, sync_all_expect_get_correct_records)
{
    SoftSyncRecord record{};
    record.location = { 10, 10 };
    record.waitCoreID = 0;
    record.usedCores = 2;
    record.eventID = 0;
    record.isAIVOnly = false;
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report___sync_all_stub(memInfo.data(), record.location.fileNo, record.location.lineNo,
        record.location.pc, 2, false);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report___sync_all_stub(memInfo.data(), record.location.fileNo, record.location.lineNo,
        record.location.pc, 2, false);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report___sync_all_stub(memInfo.data(), record.location.fileNo, record.location.lineNo,
        record.location.pc, 2, false);
    uint8_t* ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::SYNC_ALL_STUB>(ptr, record));
}

TEST(SyncInstructions, event_t_overloads_and_atomic_modes_expect_record)
{
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    event_t ev = static_cast<event_t>(EventID::EVENT_ID1);
    pipe_t src = static_cast<pipe_t>(PipeType::PIPE_S);
    pipe_t dst = static_cast<pipe_t>(PipeType::PIPE_V);
    mem_t mem = static_cast<mem_t>(MemType::GM);

    __sanitizer_report_hset_flag(memInfo.data(), 1, 1, 0, src, dst, ev, mem, false);
    __sanitizer_report_hwait_flag(memInfo.data(), 1, 1, 0, src, dst, ev, mem, false);
    __sanitizer_report_set_flag(memInfo.data(), 1, 1, 0, src, dst, ev);
    __sanitizer_report_set_flag(memInfo.data(), 1, 1, 0, dst, ev);
    __sanitizer_report_wait_flag(memInfo.data(), 1, 1, 0, src, dst, ev);
    __sanitizer_report_wait_flag(memInfo.data(), 1, 1, 0, dst, ev);

    __sanitizer_report_set_atomic_f16(memInfo.data(), 1, 1, 0);
    __sanitizer_report_set_atomic_s16(memInfo.data(), 1, 1, 0);
    __sanitizer_report_set_atomic_s32(memInfo.data(), 1, 1, 0);
    __sanitizer_report_set_atomic_s8(memInfo.data(), 1, 1, 0);
    __sanitizer_report_set_atomic_bf16(memInfo.data(), 1, 1, 0);

    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_GT(blockHead.recordWriteCount, 0u);
}

}
