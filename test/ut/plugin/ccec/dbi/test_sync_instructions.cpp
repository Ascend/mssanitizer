#include <gtest/gtest.h>
#include <iostream>
#define BUILD_DYNAMIC_PROBE

#include "../../ccec_defs.h"
#include "../data_process.h"
#include "plugin/ccec/dbi/probes/sync_instructions.cpp"

using namespace Sanitizer;

namespace SanitizerTest {

constexpr uint64_t MEM_INFO_SIZE = 1024 * 1024;


TEST(SyncInstructions, dbi_ffts_cross_core_sync_expect_get_correct_records)
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
    __sanitizer_report_set_cross_core(memInfo.data(), record.location.pc, 0, record.dst, config);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_cross_core(memInfo.data(), record.location.pc, 0, record.dst, config);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_cross_core(memInfo.data(), record.location.pc, 0, record.dst, config);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::FftsSyncRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::FFTS_SYNC>(ptr, record));
}

TEST(SyncInstructions, dbi_wait_flag_dev_expect_get_correct_records)
{
    WaitFlagDevPipeRecord record{};
    record.location = {10, 10};
    record.flagID = 2;
    record.pipe = PipeType::PIPE_MTE3;
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report_wait_flag_dev_pipe(memInfo.data(), record.location.pc, 0, PipeType::PIPE_S, 2);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.memcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_wait_flag_dev_pipe(memInfo.data(), record.location.pc, 0, PipeType::PIPE_MTE2, 2);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_wait_flag_dev_pipe(memInfo.data(), record.location.pc, 0, PipeType::PIPE_MTE3, 2);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::WaitFlagDevPipeRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::WAIT_FLAG_DEV_PIPE>(ptr, record));
}

TEST(SyncInstructions, dbi_wait_flag_devi_expect_get_correct_records)
{
    WaitFlagDevPipeRecord record{};
    record.location = {10, 10};
    record.flagID = 2;
    record.pipe = PipeType::PIPE_MTE3;
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report_wait_flag_devi_pipe(memInfo.data(), record.location.pc, 0, PipeType::PIPE_S, 2);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.memcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_wait_flag_devi_pipe(memInfo.data(), record.location.pc, 0, PipeType::PIPE_MTE2, 2);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_wait_flag_devi_pipe(memInfo.data(), record.location.pc, 0, PipeType::PIPE_MTE3, 2);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::WaitFlagDevPipeRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::WAIT_FLAG_DEVI_PIPE>(ptr, record));
}

TEST(SyncInstructions, dbi_set_intra_block_sync_expect_get_correct_records)
{
    IntraBlockSyncRecord record{};
    record.location = {10, 10};
    record.pipe = PipeType::PIPE_MTE3;
    record.syncID = 2;
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report_set_intra_block(memInfo.data(), record.location.pc, 0, record.pipe, record.syncID);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_intra_block(memInfo.data(), record.location.pc, 0, record.pipe, record.syncID);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_intra_block(memInfo.data(), record.location.pc, 0, record.pipe, record.syncID);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::IntraBlockSyncRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_INTRA_BLOCK>(ptr, record));
}

TEST(SyncInstructions, dbi_wait_intra_block_sync_expect_get_correct_records)
{
    IntraBlockSyncRecord record{};
    record.location = {10, 10};
    record.pipe = PipeType::PIPE_MTE3;
    record.syncID = 2;
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report_wait_intra_block(memInfo.data(), record.location.pc, 0, record.pipe, record.syncID);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_wait_intra_block(memInfo.data(), record.location.pc, 0, record.pipe, record.syncID);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_wait_intra_block(memInfo.data(), record.location.pc, 0, record.pipe, record.syncID);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::IntraBlockSyncRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::WAIT_INTRA_BLOCK>(ptr, record));
}

TEST(SyncInstructions, dbi_set_intra_blocki_sync_expect_get_correct_records)
{
    IntraBlockSyncRecord record{};
    record.location = {10, 10};
    record.pipe = PipeType::PIPE_MTE2;
    record.syncID = 2;
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report_set_intra_blocki(memInfo.data(), record.location.pc, 0, record.pipe, record.syncID);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_intra_blocki(memInfo.data(), record.location.pc, 0, record.pipe, record.syncID);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_intra_blocki(memInfo.data(), record.location.pc, 0, record.pipe, record.syncID);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::IntraBlockSyncRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_INTRA_BLOCKI>(ptr, record));
}

TEST(SyncInstructions, dbi_wait_intra_blocki_sync_expect_get_correct_records)
{
    IntraBlockSyncRecord record{};
    record.location = {10, 10};
    record.pipe = PipeType::PIPE_MTE2;
    record.syncID = 2;
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report_wait_intra_blocki(memInfo.data(), record.location.pc, 0, record.pipe, record.syncID);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_wait_intra_blocki(memInfo.data(), record.location.pc, 0, record.pipe, record.syncID);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_wait_intra_blocki(memInfo.data(), record.location.pc, 0, record.pipe, record.syncID);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::IntraBlockSyncRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::WAIT_INTRA_BLOCKI>(ptr, record));
}

TEST(SyncInstructions, dbi_hset_flag_expect_get_correct_records)
{
    HardSyncRecord record{};
    record.location = {10, 10};
    record.src = PipeType::PIPE_MTE3;
    record.dst = PipeType::PIPE_MTE2;
    record.eventID = 2;
    record.memory = MemType::L0A;
    record.v = true;
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report_hset_flag(memInfo.data(), record.location.pc, 0, record.src, record.dst, record.eventID, record.memory, record.v);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_hset_flag(memInfo.data(), record.location.pc, 0, record.src, record.dst, record.eventID, record.memory, record.v);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_hset_flag(memInfo.data(), record.location.pc, 0, record.src, record.dst, record.eventID, record.memory, record.v);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::HardSyncRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::HSET_FLAG>(ptr, record));
}

TEST(SyncInstructions, dbi_hwait_flag_expect_get_correct_records)
{
    HardSyncRecord record{};
    record.location = {10, 10};
    record.src = PipeType::PIPE_MTE3;
    record.dst = PipeType::PIPE_MTE2;
    record.eventID = 2;
    record.memory = MemType::L0A;
    record.v = true;
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report_hwait_flag(memInfo.data(), record.location.pc, 0, record.src, record.dst, record.eventID, record.memory, record.v);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_hwait_flag(memInfo.data(), record.location.pc, 0, record.src, record.dst, record.eventID, record.memory, record.v);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_hwait_flag(memInfo.data(), record.location.pc, 0, record.src, record.dst, record.eventID, record.memory, record.v);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::HardSyncRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::HWAIT_FLAG>(ptr, record));
}

TEST(SyncInstructions, dbi_hset_flagi_expect_get_correct_records)
{
    HardSyncRecord record{};
    record.location = {10, 10};
    record.src = PipeType::PIPE_S;
    record.dst = PipeType::PIPE_V;
    record.eventID = static_cast<uint64_t>(EventID::EVENT_ID1);
    record.memory = MemType::GM;
    record.v = 0;

    /// 默认开启内存检测，此时同步指令不记录
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report_hset_flagi(memInfo.data(), record.location.pc, 0, record.src, record.dst, record.eventID, record.memory, record.v);
    __sanitizer_report_hset_flagi(memInfo.data(), record.location.pc, 0, record.src, record.dst, static_cast<uint64_t>(record.eventID),
                                 record.memory, record.v);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    /// 开启未初始化检测时，此时同步指令不记录
    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_hset_flagi(memInfo.data(), record.location.pc, 0, record.src, record.dst, record.eventID, record.memory, record.v);
    __sanitizer_report_hset_flagi(memInfo.data(), record.location.pc, 0, record.src, record.dst, static_cast<uint64_t>(record.eventID),
                                 record.memory, record.v);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    /// 开启竞争检测，此时同步指令会记录
    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_hset_flagi(memInfo.data(), record.location.pc, 0, record.src, record.dst, record.eventID, record.memory, record.v);
    __sanitizer_report_hset_flagi(memInfo.data(), record.location.pc, 0, record.src, record.dst, static_cast<uint64_t>(record.eventID),
                                 record.memory, record.v);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    // 从写入的记录中读取 location 字段
    auto x = reinterpret_cast<Sanitizer::HardSyncRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::HSET_FLAGI>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(HardSyncRecord);
    // 从第二个写入的记录中读取 location 字段
    x = reinterpret_cast<Sanitizer::HardSyncRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::HSET_FLAGI>(ptr, record));
}

TEST(SyncInstructions, dbi_hwait_flagi_expect_get_correct_records)
{
    HardSyncRecord record{};
    record.location = {10, 10};
    record.src = PipeType::PIPE_S;
    record.dst = PipeType::PIPE_V;
    record.eventID = static_cast<uint64_t>(EventID::EVENT_ID1);
    record.memory = MemType::GM;
    record.v = 0;
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    __sanitizer_report_hwait_flagi(memInfo.data(), record.location.pc, 0, record.src, record.dst, record.eventID, record.memory, record.v);
    __sanitizer_report_hwait_flagi(memInfo.data(), record.location.pc, 0, record.src, record.dst, static_cast<uint64_t>(record.eventID),
                                 record.memory, record.v);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_hwait_flagi(memInfo.data(), record.location.pc, 0, record.src, record.dst, record.eventID, record.memory, record.v);
    __sanitizer_report_hwait_flagi(memInfo.data(), record.location.pc, 0, record.src, record.dst, static_cast<uint64_t>(record.eventID),
                                 record.memory, record.v);
    ASSERT_EQ(blockHead.recordWriteCount, 0);

    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_hwait_flagi(memInfo.data(), record.location.pc, 0, record.src, record.dst, record.eventID, record.memory, record.v);
    __sanitizer_report_hwait_flagi(memInfo.data(), record.location.pc, 0, record.src, record.dst, static_cast<uint64_t>(record.eventID),
                                 record.memory, record.v);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    // 从写入的记录中读取 location 字段
    auto x = reinterpret_cast<Sanitizer::HardSyncRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::HWAIT_FLAGI>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(HardSyncRecord);
    // 从第二个写入的记录中读取 location 字段
    x = reinterpret_cast<Sanitizer::HardSyncRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::HWAIT_FLAGI>(ptr, record));
}

}
