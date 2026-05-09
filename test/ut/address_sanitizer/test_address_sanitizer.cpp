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
#include <memory>
#include <random>

#define private public
#include "sanitizer_base.h"
#include "address_sanitizer.h"
#include "record_parse.h"
#include "record_pre_process.h"
#include "asan_action.h"
#include "mem_error_def.h"
#include "platform_config.h"
#include "shadow_memory.h"
#include "utility/log.h"
#include "core/framework/format_converter.h"
#include "core/framework/file_mapping.h"
#include "core/framework/utility/ustring.h"
#include "core/framework/utility/path.h"
#undef private

namespace {
using namespace Sanitizer;

static constexpr uint64_t LOCAL_BLOCK_SIZE = 32;
static constexpr uint64_t SRC0_ADDR = 0x11;
static constexpr uint64_t SRC1_ADDR = 0x22;
static constexpr uint64_t DST_ADDR = 0x33;
static constexpr uint64_t SRC0_BLOCK_STRIDE = 11;
static constexpr uint64_t SRC1_BLOCK_STRIDE = 22;
static constexpr uint64_t DST_BLOCK_STRIDE = 33;
static constexpr uint64_t N_BURST = 2;
static constexpr uint64_t LEN_BURST = 44;
static constexpr uint64_t LEN_BURST_PAD = 1;
static constexpr uint64_t LEFT_PADDING = 55;
static constexpr uint64_t RIGHT_PADDING = 66;
static constexpr uint64_t REPEAT = 2;
static constexpr uint64_t SRC0_REPEAT_STRIDE = 77;
static constexpr uint64_t SRC1_REPEAT_STRIDE = 88;
static constexpr uint64_t DST_REPEAT_STRIDE = 99;
static constexpr uint64_t SRC0_BLOCK_NUM = 2;
static constexpr uint64_t SRC1_BLOCK_NUM = 2;
static constexpr uint64_t DST_BLOCK_NUM = 2;
static constexpr uint64_t SRC0_BLOCK_SIZE = 12;
static constexpr uint64_t SRC1_BLOCK_SIZE = 34;
static constexpr uint64_t DST_BLOCK_SIZE = 56;
static constexpr uint64_t REDUCE_DST_BLOCK_NUM = 1;
static constexpr uint64_t REDUCE_REPEAT_LENGTH = 32;
static constexpr uint64_t LOAD3D_FMAP_LENGTH = 16;
static constexpr uint64_t LOAD3D_FMAP_PAD = 1;
static constexpr uint64_t LOAD3D_FILTER_LENGTH = 3;
static constexpr uint64_t LOAD3D_FILTER_STRIDE = 1;
static constexpr uint64_t LOAD3D_FILTER_DILATION = 1;
static constexpr uint64_t LOAD3D_MATRIX_POS = 0;
static constexpr uint64_t LOAD3D_MATRIX_KSTEP = 9;
static constexpr uint64_t LOAD3D_MATRIX_MSTEP = 16;
static constexpr uint64_t LOAD3D_RPT_MODE = 0;
static constexpr uint64_t LOAD3D_RPT_TIMES = 1;
static constexpr uint64_t LOAD3D_RPT_STRIDE = 1;

MemOpRecord CreateMemOpRecord(MemOpType type)
{
    MemOpRecord record{};
    record.serialNo = 0;
    record.type = type;
    record.coreId = 1;
    record.moduleId = 0;
    record.srcAddr = 0x1000;
    record.dstAddr = 0x2000;
    record.infoSrc = MemInfoSrc::ACL;
    record.srcSpace = AddressSpace::GM;
    record.dstSpace = AddressSpace::GM;
    record.memSize = 0x1000;
    strncpy(record.fileName, "test.cpp", sizeof(record.fileName) - 1);

    return record;
}

inline void FillFileNameByNo(MemOpRecord& record, uint64_t fileNo)
{
    /// 编译器 filemapping 中获取的路径可能会超过 record.fileName 的长度导致 strncpy_s 接口
    /// 拷贝失败。因此此处截取出文件名，并最长拷贝 sizeof(record.fileName) - 1 个字符
    std::string filePath = FileMapping::Instance().Query(fileNo).fileName;
    std::string fileName = Utility::ReplaceInvalidChar(Path(filePath).Name());

    std::size_t length = std::min(fileName.length(), sizeof(record.fileName) - 1);
    fileName.copy(record.fileName, length);
    record.fileName[length] = '\0';
}

void SetBasicMemInfo(MemOpRecord &record, const SanEvent &event)
{
    record.serialNo = event.serialNo;
    record.type = FormatConverter::AccessTypeToMemOpType(event.eventInfo.memInfo.opType);
    record.coreId = event.loc.coreId;
    record.moduleId = -1;
    // 在kernelRecord转换成sanevent的过程中，srcAddr和dstAddr相等且srcSpace和dstSpace相等
    record.srcSpace = record.dstSpace = FormatConverter::MemTypeToAddrSpace(event.eventInfo.memInfo.memType);
    record.lineNo = event.loc.lineNo;
    FillFileNameByNo(record, event.loc.fileNo);
    record.blockType = event.loc.blockType;
    record.pc = event.loc.pc;
    // 从 SanEvent 转换来的 MemOpRecord 都是 kernel 侧的记录
    record.side = MemOpSide::KERNEL;
}

void ProcessAndStoreMemOp(MemOpRecord &record, std::vector<MemOpRecord> &records)
{
    if (record.type == MemOpType::MEMCPY_BLOCKS) {
        record.srcAddr = record.dstAddr;
    }
    records.emplace_back(record);
}

// MemOpInfo转为MemOpRecord，单次repeat是单条记录
void ConvertSingleRecordRepeats(const SanEvent &event, MemOpRecord &record, std::vector<MemOpRecord> &records)
{
    MemOpInfo memInfo = event.eventInfo.memInfo;
    record.memSize = memInfo.blockStride == 1U ? memInfo.blockSize * memInfo.blockNum : memInfo.blockSize;
    uint64_t repeatStrideSize = memInfo.repeatStride * memInfo.blockSize;
    if (memInfo.repeatStride == 0) {
        record.dstAddr = memInfo.addr;
        ProcessAndStoreMemOp(record, records);
        return;
    }
    
    if (repeatStrideSize <= record.memSize) {
        // repeatStride小于合并后的单条repeat记录长度，则全部合为1条记录
        record.memSize = repeatStrideSize * (memInfo.repeatTimes - 1) + record.memSize;
        record.dstAddr = memInfo.addr;
        ProcessAndStoreMemOp(record, records);
        return;
    }

    if (memInfo.repeatTimes > 0x7FFF) {
        SAN_WARN_LOG("the repeat times exceed the maximum 0x7FFF, event serialNo: %lu", event.serialNo);
        memInfo.repeatTimes = 0x7FFF;
    }

    for (uint64_t repeatIdx = 0; repeatIdx < memInfo.repeatTimes; ++repeatIdx) {
        record.dstAddr = memInfo.addr + repeatIdx * repeatStrideSize;
        ProcessAndStoreMemOp(record, records);
    }
}

void CreateSanitizeRecord(MemOpType type, MemInfoSrc infoSrc, uint64_t dstAddr, SanitizerRecord &record)
{
    record.version = RecordVersion::MEMORY_RECORD;
    MemOpRecord opRecord{};
    opRecord.type = type;
    opRecord.dstAddr = dstAddr;
    opRecord.dstSpace = AddressSpace::GM;
    opRecord.memSize = 100;
    opRecord.infoSrc = infoSrc;
    opRecord.serialNo++;
    record.payload.memoryRecord = opRecord;
}

void ConvertSanEventsToMemOpRecords(const std::vector<SanEvent> &events,
    std::vector<MemOpRecord> &records)
{
    for (auto& event : events) {
        AddressSanitizer::ConvertSanEventToMemOpRecords(event, records);
    }
}

template<uint64_t alignSize>
static inline uint64_t AlignToCeil(uint64_t v)
{
    static_assert(alignSize != 0, "align size cannot be zero");
    return (((v - 1) / alignSize) + 1) * alignSize;
}

int RandInt(int min, int max)
{
    static std::mt19937 mersenneEngine(std::random_device{}());
    std::uniform_int_distribution<> dist(min, max);
    return dist(mersenneEngine);
}

uint64_t RandUint(uint64_t min, uint64_t max)
{
    static std::mt19937_64 mersenneEngine(std::random_device{}());
    std::uniform_int_distribution<uint64_t> dist(min, max);
    return dist(mersenneEngine);
}

TEST(AddressSanitizer, free_twice_expect_return_double_free)
{
    Config config {
        .defaultCheck = true,
        .memCheck = true,
    };
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    auto asan = SanitizerFactory::GetInstance().Create(ToolType::MEMCHECK);
    ASSERT_TRUE(asan->SetDeviceInfo(deviceInfoSummary, config));
    SanitizerRecord record;
    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord = CreateMemOpRecord(MemOpType::MALLOC);
    std::string msg;
    asan->RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("MALLOC") != std::string::npos);

    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord = CreateMemOpRecord(MemOpType::FREE);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("FREE") != std::string::npos);
    
    /// Double Free
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("illegal free") != std::string::npos);
}

TEST(AddressSanitizer, free_out_of_bounds_expect_return_illegal_free)
{
    Config config {
        .defaultCheck = true,
        .memCheck = true,
    };
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    auto asan = SanitizerFactory::GetInstance().Create(ToolType::MEMCHECK);
    ASSERT_TRUE(asan->SetDeviceInfo(deviceInfoSummary, config));
    SanitizerRecord record;
    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord = CreateMemOpRecord(MemOpType::FREE);
    std::string msg;
    asan->RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("illegal free") != std::string::npos);
}

TEST(AddressSanitizer, memcpy_blocks_on_unaccessible_expect_return_illegal_write)
{
    Config config {
        .defaultCheck = true,
        .memCheck = true,
    };
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    auto asan = SanitizerFactory::GetInstance().Create(ToolType::MEMCHECK);
    ASSERT_TRUE(asan->SetDeviceInfo(deviceInfoSummary, config));
    SanitizerRecord record;
    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord = CreateMemOpRecord(MemOpType::MEMCPY_BLOCKS);
    std::string msg;
    asan->RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("illegal write") != std::string::npos);
}

TEST(AddressSanitizer, load_bytes_on_unaccessible_expect_return_illegal_read)
{
    Config config {
        .defaultCheck = true,
        .memCheck = true,
    };
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    auto asan = SanitizerFactory::GetInstance().Create(ToolType::MEMCHECK);
    ASSERT_TRUE(asan->SetDeviceInfo(deviceInfoSummary, config));
    SanitizerRecord record;
    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord = CreateMemOpRecord(MemOpType::LOAD);
    std::string msg;
    asan->RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("illegal read") != std::string::npos);
}

TEST(AddressSanitizer, store_bytes_on_unaccessible_expect_return_illegal_write)
{
    Config config {
        .defaultCheck = true,
        .memCheck = true,
    };
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    auto asan = SanitizerFactory::GetInstance().Create(ToolType::MEMCHECK);
    ASSERT_TRUE(asan->SetDeviceInfo(deviceInfoSummary, config));
    SanitizerRecord record;
    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord = CreateMemOpRecord(MemOpType::STORE);
    std::string msg;
    asan->RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("illegal write") != std::string::npos);
}

TEST(AddressSanitizer, malloc_once_and_forget_free_expect_return_memory_leak)
{
    Config config{};
    config.memCheck = true;
    config.leakCheck = true;
    config.checkUnusedMemory = true;
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    auto asan = SanitizerFactory::GetInstance().Create(ToolType::MEMCHECK);
    ASSERT_TRUE(asan->SetDeviceInfo(deviceInfoSummary, config));
    SanitizerRecord record;
    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord = CreateMemOpRecord(MemOpType::MALLOC);
    std::string msg;
    asan->RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    asan->Exit();
    ASSERT_TRUE(msg.find("memory leaks") != std::string::npos);
    ASSERT_TRUE(msg.find("Unused memory") != std::string::npos);
}

TEST(AddressSanitizer, malloc_free_but_not_use_expect_return_mem_not_use)
{
    Config config{};
    config.memCheck = true;
    config.checkUnusedMemory = true;
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    auto asan = SanitizerFactory::GetInstance().Create(ToolType::MEMCHECK);
    ASSERT_TRUE(asan->SetDeviceInfo(deviceInfoSummary, config));
    SanitizerRecord record;
    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord = CreateMemOpRecord(MemOpType::MALLOC);
    std::string msg;
    asan->RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));

    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord = CreateMemOpRecord(MemOpType::FREE);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("FREE") != std::string::npos);
    ASSERT_TRUE(msg.find("Unused memory") != std::string::npos);
}

TEST(AddressSanitizer, malloc_free_and_use_expect_no_exception)
{
    Config config{};
    config.memCheck = true;
    config.checkUnusedMemory = true;
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    auto asan = SanitizerFactory::GetInstance().Create(ToolType::MEMCHECK);
    ASSERT_TRUE(asan->SetDeviceInfo(deviceInfoSummary, config));
    SanitizerRecord record;
    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord = CreateMemOpRecord(MemOpType::MALLOC);
    std::string msg;
    asan->RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord = CreateMemOpRecord(MemOpType::STORE);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord = CreateMemOpRecord(MemOpType::FREE);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("FREE") != std::string::npos);
    ASSERT_TRUE(msg.find("Unused memory") == std::string::npos);
}

TEST(AddressSanitizer, malloc_use_and_forget_free_expect_no_mem_not_use)
{
    Config config{};
    config.memCheck = true;
    config.leakCheck = true;
    config.checkUnusedMemory = true;
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    auto asan = SanitizerFactory::GetInstance().Create(ToolType::MEMCHECK);
    ASSERT_TRUE(asan->SetDeviceInfo(deviceInfoSummary, config));
    SanitizerRecord record;
    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord = CreateMemOpRecord(MemOpType::MALLOC);
    std::string msg;
    asan->RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord = CreateMemOpRecord(MemOpType::STORE);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    asan->Exit();
    ASSERT_TRUE(msg.find("memory leaks") != std::string::npos);
    ASSERT_TRUE(msg.find("Unused memory") == std::string::npos);
}

TEST(AddressSanitizer, same_record_on_different_blocks_expect_get_reduced_errors)
{
    Config config{};
    config.memCheck = true;
    auto asan = SanitizerFactory::GetInstance().Create(ToolType::MEMCHECK);

    std::string msg;
    asan->RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    ASSERT_TRUE(asan->SetDeviceInfo(deviceInfoSummary, config));
    RuntimeContext::Instance().deviceSummary_.device = deviceInfoSummary.device;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::LOAD;
    kernelRecord.blockType = BlockType::AIVEC;
    kernelRecord.payload.loadStoreRecord = {0x00, 8, {0, 0, 0}, AddressSpace::GM, {}, 0};
    SanitizerRecord record;
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord = kernelRecord;
    ASSERT_TRUE(asan->CheckRecordBeforeProcess(record));
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(record, events);
    asan->Do(record, events);
    // load same addr on blockIdx 1
    record.payload.kernelRecord.payload.loadStoreRecord.location.blockId = 1;
    ASSERT_TRUE(asan->CheckRecordBeforeProcess(record));
    RecordPreProcess::GetInstance().Process(record, events);
    asan->Do(record, events);
    // report finish to reduce error msgs
    record.payload.kernelRecord.recordType = RecordType::KERNEL_FINISH;
    ASSERT_TRUE(asan->CheckRecordBeforeProcess(record));
    RecordPreProcess::GetInstance().Process(record, events);
    asan->Do(record, events);
    asan->Exit();
    ASSERT_TRUE(msg.find("illegal read") != std::string::npos);
    ASSERT_TRUE(msg.find("aiv(0-1)") != std::string::npos);
}

TEST(AddressSanitizer, record_with_aicore_blocks_expect_get_reduced_errors)
{
    Config config{};
    config.memCheck = true;
    auto asan = SanitizerFactory::GetInstance().Create(ToolType::MEMCHECK);

    std::string msg;
    asan->RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_310P;
    ASSERT_TRUE(asan->SetDeviceInfo(deviceInfoSummary, config));
    RuntimeContext::Instance().deviceSummary_.device = deviceInfoSummary.device;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::BROADCAST;
    kernelRecord.blockType = BlockType::AICORE;
    kernelRecord.payload.broadcastRecord = {{0, 0, 0}, 0x240000, 0x250000, 1, 2, 1, 1, false, DataType::DATA_B16,
    DataType::DATA_B16, MemType::L0C, MemType::UB};
    SanitizerRecord record;
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord = kernelRecord;
    ASSERT_TRUE(asan->CheckRecordBeforeProcess(record));
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(record, events);
    asan->Do(record, events);
    // load same addr on blockIdx 1
    record.payload.kernelRecord.payload.broadcastRecord.location.blockId = 1;
    ASSERT_TRUE(asan->CheckRecordBeforeProcess(record));
    RecordPreProcess::GetInstance().Process(record, events);
    asan->Do(record, events);
    // report finish to reduce error msgs
    record.payload.kernelRecord.recordType = RecordType::KERNEL_FINISH;
    ASSERT_TRUE(asan->CheckRecordBeforeProcess(record));
    RecordPreProcess::GetInstance().Process(record, events);
    asan->Do(record, events);
    asan->Exit();
    ASSERT_TRUE(msg.find("illegal read") != std::string::npos);
    ASSERT_TRUE(msg.find("illegal write") != std::string::npos);
    ASSERT_TRUE(msg.find("aicore") != std::string::npos);
}

TEST(AddressSanitizer, parse_memory_record_expect_get_correct_memory_records)
{
    MemOpRecord memOpRecord{};
    memOpRecord.type = MemOpType::LOAD;
    memOpRecord.coreId = 22;
    memOpRecord.moduleId = 33;
    memOpRecord.srcAddr = 0x44;
    memOpRecord.dstAddr = 0x55;
    memOpRecord.srcSpace = AddressSpace::GM;
    memOpRecord.dstSpace = AddressSpace::UB;
    memOpRecord.memSize = 66;
    memOpRecord.lineNo = 77;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::MEMORY_RECORD;
    sanitizerRecord.payload.memoryRecord = memOpRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
}

TEST(AddressSanitizer, parse_load_memory_record_expect_get_correct_memory_records)
{
    LoadStoreRecord loadStoreRecord{};
    loadStoreRecord.addr = SRC0_ADDR;
    loadStoreRecord.size = LEN_BURST;
    loadStoreRecord.space = AddressSpace::UB;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::LOAD;
    kernelRecord.payload.loadStoreRecord = loadStoreRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstSpace, AddressSpace::UB);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, LEN_BURST);
}

TEST(AddressSanitizer, parse_load_private_memory_record_expect_get_correct_memory_records)
{
    LoadStoreRecord loadStoreRecord{};
    loadStoreRecord.addr = SRC0_ADDR;
    loadStoreRecord.size = LEN_BURST;
    loadStoreRecord.space = AddressSpace::PRIVATE;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::LOAD;
    kernelRecord.payload.loadStoreRecord = loadStoreRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstSpace, AddressSpace::PRIVATE);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, LEN_BURST);
}

TEST(AddressSanitizer, parse_store_memory_record_expect_get_correct_memory_records)
{
    LoadStoreRecord loadStoreRecord;
    loadStoreRecord.addr = SRC0_ADDR;
    loadStoreRecord.size = LEN_BURST;
    loadStoreRecord.space = AddressSpace::GM;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::STORE;
    kernelRecord.payload.loadStoreRecord = loadStoreRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].type, MemOpType::STORE);
    ASSERT_EQ(records[0].dstSpace, AddressSpace::GM);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, LEN_BURST);
}

TEST(AddressSanitizer, parse_dma_mov_record_expect_get_correct_memory_records)
{
    DmaMovRecord dmaMovRecord{};
    dmaMovRecord.dst = DST_ADDR;
    dmaMovRecord.src = SRC0_ADDR;
    dmaMovRecord.nBurst = N_BURST;
    dmaMovRecord.lenBurst = LEN_BURST;
    dmaMovRecord.srcStride = SRC0_BLOCK_STRIDE;
    dmaMovRecord.dstStride = DST_BLOCK_STRIDE;
    dmaMovRecord.dstMemType = MemType::GM;
    dmaMovRecord.srcMemType = MemType::UB;
    dmaMovRecord.padMode = PadMode::PAD_NONE;
    dmaMovRecord.byteMode = ByteMode::BM_DISABLE;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::DMA_MOV;
    kernelRecord.payload.dmaMovRecord = dmaMovRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 4);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, LEN_BURST * LOCAL_BLOCK_SIZE);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + (SRC0_BLOCK_STRIDE + LEN_BURST) * LOCAL_BLOCK_SIZE);
    ASSERT_EQ(records[1].memSize, LEN_BURST * LOCAL_BLOCK_SIZE);
    ASSERT_EQ(records[2].type, MemOpType::STORE);
    ASSERT_EQ(records[2].dstAddr, DST_ADDR);
    ASSERT_EQ(records[2].memSize, LEN_BURST * LOCAL_BLOCK_SIZE);
    ASSERT_EQ(records[3].type, MemOpType::STORE);
    ASSERT_EQ(records[3].dstAddr, DST_ADDR + (DST_BLOCK_STRIDE + LEN_BURST) * LOCAL_BLOCK_SIZE);
    ASSERT_EQ(records[3].memSize, LEN_BURST * LOCAL_BLOCK_SIZE);
}

TEST(AddressSanitizer, parse_dma_mov_record_with_byte_mode_expect_get_correct_memory_records)
{
    DmaMovRecord dmaMovRecord{};
    dmaMovRecord.dst = DST_ADDR;
    dmaMovRecord.src = SRC0_ADDR;
    dmaMovRecord.nBurst = N_BURST;
    dmaMovRecord.lenBurst = LEN_BURST;
    dmaMovRecord.srcStride = SRC0_BLOCK_STRIDE;
    dmaMovRecord.dstStride = DST_BLOCK_STRIDE;
    dmaMovRecord.dstMemType = MemType::GM;
    dmaMovRecord.srcMemType = MemType::UB;
    dmaMovRecord.padMode = PadMode::PAD_NONE;
    dmaMovRecord.byteMode = ByteMode::BM_ENABLE;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::DMA_MOV;
    kernelRecord.payload.dmaMovRecord = dmaMovRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    uint64_t alignedLen = AlignToCeil<LOCAL_BLOCK_SIZE>(LEN_BURST);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + alignedLen + SRC0_BLOCK_STRIDE * LOCAL_BLOCK_SIZE);
    ASSERT_EQ(records[2].type, MemOpType::STORE);
    ASSERT_EQ(records[2].dstAddr, DST_ADDR);
    ASSERT_EQ(records[2].memSize, LEN_BURST);
    ASSERT_EQ(records[3].type, MemOpType::STORE);
    ASSERT_EQ(records[3].dstAddr, DST_ADDR + LEN_BURST + DST_BLOCK_STRIDE * LOCAL_BLOCK_SIZE);
    ASSERT_EQ(records[3].memSize, LEN_BURST);
}

TEST(AddressSanitizer, parse_dma_mov_record_with_up_scale_pad_mode_expect_get_correct_memory_records)
{
    DmaMovRecord dmaMovRecord{};
    dmaMovRecord.dst = DST_ADDR;
    dmaMovRecord.src = SRC0_ADDR;
    dmaMovRecord.nBurst = N_BURST;
    dmaMovRecord.lenBurst = LEN_BURST_PAD;
    dmaMovRecord.srcStride = SRC0_BLOCK_STRIDE;
    dmaMovRecord.dstStride = DST_BLOCK_STRIDE;
    dmaMovRecord.dstMemType = MemType::GM;
    dmaMovRecord.srcMemType = MemType::UB;
    dmaMovRecord.padMode = PadMode::PAD_MODE1;
    dmaMovRecord.byteMode = ByteMode::BM_DISABLE;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::DMA_MOV;
    kernelRecord.payload.dmaMovRecord = dmaMovRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 3);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, N_BURST);
    ASSERT_EQ(records[1].type, MemOpType::STORE);
    ASSERT_EQ(records[1].dstAddr, DST_ADDR);
    ASSERT_EQ(records[1].memSize, LOCAL_BLOCK_SIZE);
    ASSERT_EQ(records[2].type, MemOpType::STORE);
    ASSERT_EQ(records[2].dstAddr, DST_ADDR + (1 + DST_BLOCK_STRIDE) * LOCAL_BLOCK_SIZE);
    ASSERT_EQ(records[2].memSize, LOCAL_BLOCK_SIZE);
}

TEST(AddressSanitizer, parse_dma_mov_record_with_down_scale_pad_mode_expect_get_correct_memory_records)
{
    DmaMovRecord dmaMovRecord{};
    dmaMovRecord.dst = DST_ADDR;
    dmaMovRecord.src = SRC0_ADDR;
    dmaMovRecord.nBurst = N_BURST;
    dmaMovRecord.lenBurst = LEN_BURST_PAD;
    dmaMovRecord.srcStride = SRC0_BLOCK_STRIDE;
    dmaMovRecord.dstStride = DST_BLOCK_STRIDE;
    dmaMovRecord.dstMemType = MemType::GM;
    dmaMovRecord.srcMemType = MemType::UB;
    dmaMovRecord.padMode = PadMode::PAD_MODE6;
    dmaMovRecord.byteMode = ByteMode::BM_DISABLE;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::DMA_MOV;
    kernelRecord.payload.dmaMovRecord = dmaMovRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 3);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, LOCAL_BLOCK_SIZE);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + (1 + SRC0_BLOCK_STRIDE) * LOCAL_BLOCK_SIZE);
    ASSERT_EQ(records[1].memSize, LOCAL_BLOCK_SIZE);
    ASSERT_EQ(records[2].type, MemOpType::STORE);
    ASSERT_EQ(records[2].dstAddr, DST_ADDR);
    ASSERT_EQ(records[2].memSize, N_BURST * 4);
}

TEST(AddressSanitizer, parse_dma_mov_conv_relu_record_with_b32_to_b32_expect_get_correct_memory_records)
{
    DmaMovConvReluRecord dmaMovConvReluRecord{};
    dmaMovConvReluRecord.dst = DST_ADDR;
    dmaMovConvReluRecord.src = SRC0_ADDR;
    dmaMovConvReluRecord.nBurst = N_BURST;
    dmaMovConvReluRecord.lenBurst = LEN_BURST;
    dmaMovConvReluRecord.srcStride = SRC0_BLOCK_STRIDE;
    dmaMovConvReluRecord.dstStride = DST_BLOCK_STRIDE;
    dmaMovConvReluRecord.crMode = ConvRelu::CRMODE_NONE;
    dmaMovConvReluRecord.dstMemType = MemType::UB;
    dmaMovConvReluRecord.srcMemType = MemType::L0C;
    dmaMovConvReluRecord.srcDataType = DataType::DATA_B32;
    dmaMovConvReluRecord.dstDataType = DataType::DATA_B32;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::DMA_MOV_CONV_RELU;
    kernelRecord.payload.dmaMovConvReluRecord = dmaMovConvReluRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 4);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, LEN_BURST * 64);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + SRC0_BLOCK_STRIDE * 1024 + LEN_BURST * 64);
    ASSERT_EQ(records[1].memSize, LEN_BURST * 64);
    ASSERT_EQ(records[2].type, MemOpType::STORE);
    ASSERT_EQ(records[2].dstAddr, DST_ADDR);
    ASSERT_EQ(records[2].memSize, LEN_BURST * 64);
    ASSERT_EQ(records[3].type, MemOpType::STORE);
    ASSERT_EQ(records[3].dstAddr, DST_ADDR + DST_BLOCK_STRIDE * 32 + LEN_BURST * 64);
    ASSERT_EQ(records[3].memSize, LEN_BURST * 64);
}

TEST(AddressSanitizer, parse_dma_mov_conv_relu_record_with_b32_to_b16_expect_get_correct_memory_records)
{
    DmaMovConvReluRecord dmaMovConvReluRecord{};
    dmaMovConvReluRecord.dst = DST_ADDR;
    dmaMovConvReluRecord.src = SRC0_ADDR;
    dmaMovConvReluRecord.nBurst = N_BURST;
    dmaMovConvReluRecord.lenBurst = LEN_BURST;
    dmaMovConvReluRecord.srcStride = SRC0_BLOCK_STRIDE;
    dmaMovConvReluRecord.dstStride = DST_BLOCK_STRIDE;
    dmaMovConvReluRecord.crMode = ConvRelu::CRMODE_F32toF16_NONE;
    dmaMovConvReluRecord.dstMemType = MemType::UB;
    dmaMovConvReluRecord.srcMemType = MemType::L0C;
    dmaMovConvReluRecord.srcDataType = DataType::DATA_B32;
    dmaMovConvReluRecord.dstDataType = DataType::DATA_B16;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::DMA_MOV_CONV_RELU;
    kernelRecord.payload.dmaMovConvReluRecord = dmaMovConvReluRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 4);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, LEN_BURST * 64);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + SRC0_BLOCK_STRIDE * 1024 + LEN_BURST * 64);
    ASSERT_EQ(records[1].memSize, LEN_BURST * 64);
    ASSERT_EQ(records[2].type, MemOpType::STORE);
    ASSERT_EQ(records[2].dstAddr, DST_ADDR);
    ASSERT_EQ(records[2].memSize, LEN_BURST * 32);
    ASSERT_EQ(records[3].type, MemOpType::STORE);
    ASSERT_EQ(records[3].dstAddr, DST_ADDR + DST_BLOCK_STRIDE * 32 + LEN_BURST * 32);
    ASSERT_EQ(records[3].memSize, LEN_BURST * 32);
}

TEST(AddressSanitizer, parse_dma_mov_conv_relu_record_with_b16_to_b16_expect_get_correct_memory_records)
{
    DmaMovConvReluRecord dmaMovConvReluRecord{};
    dmaMovConvReluRecord.dst = DST_ADDR;
    dmaMovConvReluRecord.src = SRC0_ADDR;
    dmaMovConvReluRecord.nBurst = N_BURST;
    dmaMovConvReluRecord.lenBurst = LEN_BURST;
    dmaMovConvReluRecord.srcStride = SRC0_BLOCK_STRIDE;
    dmaMovConvReluRecord.dstStride = DST_BLOCK_STRIDE;
    dmaMovConvReluRecord.crMode = ConvRelu::CRMODE_NONE;
    dmaMovConvReluRecord.dstMemType = MemType::UB;
    dmaMovConvReluRecord.srcMemType = MemType::L0C;
    dmaMovConvReluRecord.srcDataType = DataType::DATA_B16;
    dmaMovConvReluRecord.dstDataType = DataType::DATA_B16;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::DMA_MOV_CONV_RELU;
    kernelRecord.payload.dmaMovConvReluRecord = dmaMovConvReluRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 4);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, LEN_BURST * 32);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + SRC0_BLOCK_STRIDE * 512 + LEN_BURST * 32);
    ASSERT_EQ(records[1].memSize, LEN_BURST * 32);
    ASSERT_EQ(records[2].type, MemOpType::STORE);
    ASSERT_EQ(records[2].dstAddr, DST_ADDR);
    ASSERT_EQ(records[2].memSize, LEN_BURST * 32);
    ASSERT_EQ(records[3].type, MemOpType::STORE);
    ASSERT_EQ(records[3].dstAddr, DST_ADDR + DST_BLOCK_STRIDE * 32 + LEN_BURST * 32);
    ASSERT_EQ(records[3].memSize, LEN_BURST * 32);
}

TEST(AddressSanitizer, parse_dma_mov_depth_wise_record_with_b16_to_b16_expect_get_correct_memory_records)
{
    DmaMovConvReluRecord dmaMovConvReluRecord{};
    dmaMovConvReluRecord.dst = DST_ADDR;
    dmaMovConvReluRecord.src = SRC0_ADDR;
    dmaMovConvReluRecord.nBurst = 4;
    dmaMovConvReluRecord.lenBurst = LEN_BURST;
    dmaMovConvReluRecord.srcStride = SRC0_BLOCK_STRIDE;
    dmaMovConvReluRecord.dstStride = DST_BLOCK_STRIDE;
    dmaMovConvReluRecord.crMode = ConvRelu::CRMODE_NONE;
    dmaMovConvReluRecord.dstMemType = MemType::UB;
    dmaMovConvReluRecord.srcMemType = MemType::L0C;
    dmaMovConvReluRecord.srcDataType = DataType::DATA_B16;
    dmaMovConvReluRecord.dstDataType = DataType::DATA_B16;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::DMA_MOV_DEPTH_WISE;
    kernelRecord.payload.dmaMovConvReluRecord = dmaMovConvReluRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
    sanitizerRecord.payload.kernelRecord.payload.dmaMovConvReluRecord.nBurst = 1;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, LEN_BURST * 512);
    ASSERT_EQ(records[1].type, MemOpType::STORE);
    ASSERT_EQ(records[1].dstAddr, DST_ADDR);
    ASSERT_EQ(records[1].memSize, LEN_BURST * 512 / 2);
    ASSERT_EQ(records[2].type, MemOpType::STORE);
    ASSERT_EQ(records[2].dstAddr, DST_ADDR + DST_BLOCK_STRIDE * 32 + LEN_BURST * 512 / 2);
    ASSERT_EQ(records[2].memSize, LEN_BURST * 512 / 2);
}

TEST(AddressSanitizer, parse_dma_mov_depth_wise_record_with_b32_to_b16_expect_get_correct_memory_records)
{
    DmaMovConvReluRecord dmaMovConvReluRecord{};
    dmaMovConvReluRecord.dst = DST_ADDR;
    dmaMovConvReluRecord.src = SRC0_ADDR;
    dmaMovConvReluRecord.nBurst = 1;
    dmaMovConvReluRecord.lenBurst = LEN_BURST;
    dmaMovConvReluRecord.srcStride = SRC0_BLOCK_STRIDE;
    dmaMovConvReluRecord.dstStride = DST_BLOCK_STRIDE;
    dmaMovConvReluRecord.crMode = ConvRelu::CRMODE_F32toF16_NONE;
    dmaMovConvReluRecord.dstMemType = MemType::UB;
    dmaMovConvReluRecord.srcMemType = MemType::L0C;
    dmaMovConvReluRecord.srcDataType = DataType::DATA_B32;
    dmaMovConvReluRecord.dstDataType = DataType::DATA_B16;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::DMA_MOV_DEPTH_WISE;
    kernelRecord.payload.dmaMovConvReluRecord = dmaMovConvReluRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, LEN_BURST * 1024);
    ASSERT_EQ(records[1].type, MemOpType::STORE);
    ASSERT_EQ(records[1].dstAddr, DST_ADDR);
    ASSERT_EQ(records[1].memSize, LEN_BURST * 512 / 2);
    ASSERT_EQ(records[2].type, MemOpType::STORE);
    ASSERT_EQ(records[2].dstAddr, DST_ADDR + DST_BLOCK_STRIDE * 32 + LEN_BURST * 512 / 2);
    ASSERT_EQ(records[2].memSize, LEN_BURST * 512 / 2);
}

TEST(AddressSanitizer, parse_dma_mov_nd2nz_record_with_b16_expect_get_correct_memory_records)
{
    // 原始memRecord构造、变量初始化
    using dtype = uint16_t;  // B16
    DmaMovNd2nzRecord dmaMovNd2nzRecord{};
    dmaMovNd2nzRecord.dst = DST_ADDR;
    dmaMovNd2nzRecord.src = SRC0_ADDR;
    dmaMovNd2nzRecord.ndNum = 3;
    dmaMovNd2nzRecord.nValue = 8;
    dmaMovNd2nzRecord.dValue = 24;
    dmaMovNd2nzRecord.srcDValue = 32;
    dmaMovNd2nzRecord.srcNdMatrixStride = (dmaMovNd2nzRecord.nValue + 4) * dmaMovNd2nzRecord.srcDValue;
    dmaMovNd2nzRecord.dstNzC0Stride = 52;
    dmaMovNd2nzRecord.dstNzNStride = 2;
    constexpr uint16_t c0EleNum = 32U / sizeof(dtype);  // NZ矩阵的C0列一行元素个数
    dmaMovNd2nzRecord.dstNzMatrixStride = 17 * c0EleNum;
    dmaMovNd2nzRecord.dataType = DataType::DATA_B16;
    uint16_t columnNum = (dmaMovNd2nzRecord.dValue * sizeof(dtype) + 32U - 1) / 32U;  // NZ矩阵的C0列数

    // 统一record构造
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::DMA_MOV_ND2NZ;
    kernelRecord.payload.dmaMovNd2nzRecord = dmaMovNd2nzRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    // 预处理后记录检验
    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
    sanitizerRecord.payload.kernelRecord.payload.dmaMovNd2nzRecord.srcMemType = MemType::L1;
    sanitizerRecord.payload.kernelRecord.payload.dmaMovNd2nzRecord.dstMemType = MemType::L0A;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 5);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), dmaMovNd2nzRecord.nValue * dmaMovNd2nzRecord.ndNum * (1 + columnNum));

    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, dmaMovNd2nzRecord.dValue * sizeof(dtype));

    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + dmaMovNd2nzRecord.srcDValue * sizeof(dtype));
    ASSERT_EQ(records[1].memSize, dmaMovNd2nzRecord.dValue * sizeof(dtype));

    ASSERT_EQ(records[8].type, MemOpType::LOAD);
    ASSERT_EQ(records[8].dstAddr, SRC0_ADDR + dmaMovNd2nzRecord.srcNdMatrixStride * sizeof(dtype));
    ASSERT_EQ(records[8].memSize, dmaMovNd2nzRecord.dValue * sizeof(dtype));

    ASSERT_EQ(records[24].type, MemOpType::STORE);
    ASSERT_EQ(records[24].dstAddr, DST_ADDR);
    ASSERT_EQ(records[24].memSize, 32U);

    ASSERT_EQ(records[25].type, MemOpType::STORE);
    ASSERT_EQ(records[25].dstAddr, DST_ADDR + dmaMovNd2nzRecord.dstNzNStride * 32U);
    ASSERT_EQ(records[25].memSize, 32U);

    ASSERT_EQ(records[32].type, MemOpType::STORE);
    ASSERT_EQ(records[32].dstAddr, DST_ADDR + dmaMovNd2nzRecord.dstNzMatrixStride * sizeof(dtype));
    ASSERT_EQ(records[32].memSize, 32U);

    ASSERT_EQ(records[48].type, MemOpType::STORE);
    ASSERT_EQ(records[48].dstAddr, DST_ADDR + dmaMovNd2nzRecord.dstNzC0Stride * 32U);
    ASSERT_EQ(records[48].memSize, 32U);
}

TEST(AddressSanitizer, parse_dma_mov_nd2nz_record_with_b32_expect_get_correct_memory_records)
{
    // 原始memRecord构造、变量初始化
    using dtype = uint32_t;  // B32
    DmaMovNd2nzRecord dmaMovNd2nzRecord{};
    dmaMovNd2nzRecord.dst = DST_ADDR;
    dmaMovNd2nzRecord.src = SRC0_ADDR;
    dmaMovNd2nzRecord.srcMemType = MemType::L1;
    dmaMovNd2nzRecord.dstMemType = MemType::L0A;
    dmaMovNd2nzRecord.ndNum = 3;
    dmaMovNd2nzRecord.nValue = 8;
    dmaMovNd2nzRecord.dValue = 24;
    dmaMovNd2nzRecord.srcDValue = 32;
    dmaMovNd2nzRecord.srcNdMatrixStride = (dmaMovNd2nzRecord.nValue + 4) * dmaMovNd2nzRecord.srcDValue;
    dmaMovNd2nzRecord.dstNzC0Stride = 52;
    dmaMovNd2nzRecord.dstNzNStride = 2;
    constexpr uint16_t c0EleNum = 32U / sizeof(dtype);  // NZ矩阵的C0列一行元素个数
    dmaMovNd2nzRecord.dstNzMatrixStride = 17 * c0EleNum;
    dmaMovNd2nzRecord.dataType = DataType::DATA_B32;
    uint16_t columnNum = (dmaMovNd2nzRecord.dValue * sizeof(dtype) + 32U - 1) / 32U;  // NZ矩阵的C0列数

    // 统一record构造
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::DMA_MOV_ND2NZ;
    kernelRecord.payload.dmaMovNd2nzRecord = dmaMovNd2nzRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    // 预处理后记录检验
    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 6);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), dmaMovNd2nzRecord.nValue * dmaMovNd2nzRecord.ndNum * (1 + columnNum));

    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, dmaMovNd2nzRecord.dValue * sizeof(dtype));

    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + dmaMovNd2nzRecord.srcDValue * sizeof(dtype));
    ASSERT_EQ(records[1].memSize, dmaMovNd2nzRecord.dValue * sizeof(dtype));

    ASSERT_EQ(records[8].type, MemOpType::LOAD);
    ASSERT_EQ(records[8].dstAddr, SRC0_ADDR + dmaMovNd2nzRecord.srcNdMatrixStride * sizeof(dtype));
    ASSERT_EQ(records[8].memSize, dmaMovNd2nzRecord.dValue * sizeof(dtype));

    ASSERT_EQ(records[24].type, MemOpType::STORE);
    ASSERT_EQ(records[24].dstAddr, DST_ADDR);
    ASSERT_EQ(records[24].memSize, 32U);

    ASSERT_EQ(records[25].type, MemOpType::STORE);
    ASSERT_EQ(records[25].dstAddr, DST_ADDR + dmaMovNd2nzRecord.dstNzNStride * 32U);
    ASSERT_EQ(records[25].memSize, 32U);

    ASSERT_EQ(records[32].type, MemOpType::STORE);
    ASSERT_EQ(records[32].dstAddr, DST_ADDR + dmaMovNd2nzRecord.dstNzMatrixStride * sizeof(dtype));
    ASSERT_EQ(records[32].memSize, 32U);

    ASSERT_EQ(records[48].type, MemOpType::STORE);
    ASSERT_EQ(records[48].dstAddr, DST_ADDR + dmaMovNd2nzRecord.dstNzC0Stride * 32U);
    ASSERT_EQ(records[48].memSize, 32U);
}

TEST(AddressSanitizer, parse_dma_mov_nd2nz_record_with_b8_expect_get_correct_memory_records)
{
    // 原始memRecord构造、变量初始化
    using dtype = uint8_t;  // B8
    DmaMovNd2nzRecord dmaMovNd2nzRecord{};
    dmaMovNd2nzRecord.dst = DST_ADDR;
    dmaMovNd2nzRecord.src = SRC0_ADDR;
    dmaMovNd2nzRecord.srcMemType = MemType::L1;
    dmaMovNd2nzRecord.dstMemType = MemType::L0A;
    dmaMovNd2nzRecord.ndNum = 3;
    dmaMovNd2nzRecord.nValue = 8;
    dmaMovNd2nzRecord.dValue = 24;
    dmaMovNd2nzRecord.srcDValue = 32;
    dmaMovNd2nzRecord.srcNdMatrixStride = (dmaMovNd2nzRecord.nValue + 4) * dmaMovNd2nzRecord.srcDValue;
    dmaMovNd2nzRecord.dstNzC0Stride = 52;
    dmaMovNd2nzRecord.dstNzNStride = 2;
    constexpr uint16_t c0EleNum = 32U / sizeof(dtype);  // NZ矩阵的C0列一行元素个数
    dmaMovNd2nzRecord.dstNzMatrixStride = 17 * c0EleNum;
    dmaMovNd2nzRecord.dataType = DataType::DATA_B8;
    uint16_t columnNum = (dmaMovNd2nzRecord.dValue * sizeof(dtype) + 32U - 1) / 32U;  // NZ矩阵的C0列数

    // 统一record构造
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::DMA_MOV_ND2NZ;
    kernelRecord.payload.dmaMovNd2nzRecord = dmaMovNd2nzRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    // 预处理后记录检验
    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 4);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), dmaMovNd2nzRecord.nValue * dmaMovNd2nzRecord.ndNum * (1 + columnNum));

    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, dmaMovNd2nzRecord.dValue * sizeof(dtype));

    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + dmaMovNd2nzRecord.srcDValue * sizeof(dtype));
    ASSERT_EQ(records[1].memSize, dmaMovNd2nzRecord.dValue * sizeof(dtype));

    ASSERT_EQ(records[8].type, MemOpType::LOAD);
    ASSERT_EQ(records[8].dstAddr, SRC0_ADDR + dmaMovNd2nzRecord.srcNdMatrixStride * sizeof(dtype));
    ASSERT_EQ(records[8].memSize, dmaMovNd2nzRecord.dValue * sizeof(dtype));

    ASSERT_EQ(records[24].type, MemOpType::STORE);
    ASSERT_EQ(records[24].dstAddr, DST_ADDR);
    ASSERT_EQ(records[24].memSize, 32U);

    ASSERT_EQ(records[25].type, MemOpType::STORE);
    ASSERT_EQ(records[25].dstAddr, DST_ADDR + dmaMovNd2nzRecord.dstNzNStride * 32U);
    ASSERT_EQ(records[25].memSize, 32U);

    ASSERT_EQ(records[32].type, MemOpType::STORE);
    ASSERT_EQ(records[32].dstAddr, DST_ADDR + dmaMovNd2nzRecord.dstNzMatrixStride * sizeof(dtype));
    ASSERT_EQ(records[32].memSize, 32U);
}

TEST(AddressSanitizer, parse_mov_align_record_expect_get_correct_memory_records)
{
    MovAlignRecord movAlignRecord{};
    movAlignRecord.dst = DST_ADDR;
    movAlignRecord.src = SRC0_ADDR;
    movAlignRecord.srcGap = SRC0_BLOCK_STRIDE;
    movAlignRecord.dstGap = DST_BLOCK_STRIDE;
    movAlignRecord.lenBurst = LEN_BURST;
    movAlignRecord.nBurst = N_BURST;
    movAlignRecord.dstMemType = MemType::UB;
    movAlignRecord.srcMemType = MemType::GM;
    movAlignRecord.dataType = DataType::DATA_B32;
    movAlignRecord.leftPaddingNum = LEFT_PADDING;
    movAlignRecord.rightPaddingNum = RIGHT_PADDING;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::MOV_ALIGN;
    kernelRecord.payload.movAlignRecord = movAlignRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    uint64_t alignedLen = AlignToCeil<LOCAL_BLOCK_SIZE>(LEN_BURST + (LEFT_PADDING + RIGHT_PADDING) * sizeof(uint32_t));
    ASSERT_EQ(records.size(), 4);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, LEN_BURST);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + LEN_BURST + SRC0_BLOCK_STRIDE);
    ASSERT_EQ(records[1].memSize, LEN_BURST);
    ASSERT_EQ(records[2].type, MemOpType::STORE);
    ASSERT_EQ(records[2].dstAddr, DST_ADDR);
    ASSERT_EQ(records[2].memSize, alignedLen);
    ASSERT_EQ(records[3].type, MemOpType::STORE);
    ASSERT_EQ(records[3].dstAddr, DST_ADDR + alignedLen + DST_BLOCK_STRIDE * LOCAL_BLOCK_SIZE);
    ASSERT_EQ(records[3].memSize, alignedLen);
}

TEST(AddressSanitizer, parse_mov_bt_records)
{
    MovBtRecord movBtRecord{};
    movBtRecord.dst = DST_ADDR;
    movBtRecord.src = SRC0_ADDR;
    movBtRecord.srcGap = SRC0_BLOCK_STRIDE;
    movBtRecord.dstGap = DST_BLOCK_STRIDE;
    movBtRecord.lenBurst = LEN_BURST;
    movBtRecord.nBurst = N_BURST;
    movBtRecord.dstMemType = MemType::BT;
    movBtRecord.srcMemType = MemType::L1;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::MOV_BT;
    kernelRecord.payload.movBtRecord = movBtRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), N_BURST);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, LEN_BURST * 64);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + LEN_BURST * 64 + SRC0_BLOCK_STRIDE * 32);
    ASSERT_EQ(records[1].memSize, LEN_BURST * 64);
}

TEST(AddressSanitizer, parse_mov_fp_record_with_normal_movement_s32_expect_get_correct_memory_records)
{
    MovFpRecord movFpRecord{};
    movFpRecord.dst = DST_ADDR;
    movFpRecord.src = SRC0_ADDR;
    movFpRecord.dstStride = 80;
    movFpRecord.srcStride = 64;
    movFpRecord.nSize = 48;
    movFpRecord.mSize = 24;
    movFpRecord.ndNum = 0;
    movFpRecord.dstNdStride = 0;
    movFpRecord.srcNdStride = 0;
    movFpRecord.srcNzC0Stride = 0;
    movFpRecord.quantPreBits = 32;
    movFpRecord.int8ChannelMerge = false;
    movFpRecord.int4ChannelMerge = false;
    movFpRecord.channelSplit = false;
    movFpRecord.enNZ2ND = false;
    movFpRecord.enNZ2DN = false;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::MOV_FP;
    kernelRecord.payload.movFpRecord = movFpRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ConvertSanEventsToMemOpRecords(events, records);
    uint16_t columnNum = (movFpRecord.nSize + 16U - 1) / 16U;  // N维的列数
    ASSERT_EQ(records.size(), columnNum + columnNum);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, movFpRecord.mSize * 64U);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + movFpRecord.srcStride * 64U);
    ASSERT_EQ(records[1].memSize, movFpRecord.mSize * 64U);
    ASSERT_EQ(records[3].type, MemOpType::STORE);
    ASSERT_EQ(records[3].dstAddr, DST_ADDR);
    ASSERT_EQ(records[3].memSize, movFpRecord.mSize * 64U);
    ASSERT_EQ(records[4].type, MemOpType::STORE);
    ASSERT_EQ(records[4].dstAddr, DST_ADDR + movFpRecord.dstStride * 32U);
    ASSERT_EQ(records[4].memSize, movFpRecord.mSize * 64U);
}

TEST(AddressSanitizer, parse_mov_fp_record_with_normal_movement_s16_expect_get_correct_memory_records)
{
    MovFpRecord movFpRecord{};
    movFpRecord.dst = DST_ADDR;
    movFpRecord.src = SRC0_ADDR;
    movFpRecord.dstStride = 40;
    movFpRecord.srcStride = 64;
    movFpRecord.nSize = 48;
    movFpRecord.mSize = 24;
    movFpRecord.ndNum = 0;
    movFpRecord.dstNdStride = 0;
    movFpRecord.srcNdStride = 0;
    movFpRecord.quantPreBits = 16;
    movFpRecord.int8ChannelMerge = false;
    movFpRecord.int4ChannelMerge = false;
    movFpRecord.channelSplit = false;
    movFpRecord.enNZ2ND = false;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::MOV_FP;
    kernelRecord.payload.movFpRecord = movFpRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ConvertSanEventsToMemOpRecords(events, records);
    uint16_t columnNum = (movFpRecord.nSize + 16U - 1) / 16U;  // N维的列数
    ASSERT_EQ(records.size(), columnNum + columnNum);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, movFpRecord.mSize * 64U);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + movFpRecord.srcStride * 64U);
    ASSERT_EQ(records[1].memSize, movFpRecord.mSize * 64U);
    ASSERT_EQ(records[3].type, MemOpType::STORE);
    ASSERT_EQ(records[3].dstAddr, DST_ADDR);
    ASSERT_EQ(records[3].memSize, movFpRecord.mSize * 32U);
    ASSERT_EQ(records[4].type, MemOpType::STORE);
    ASSERT_EQ(records[4].dstAddr, DST_ADDR + movFpRecord.dstStride * 32U);
    ASSERT_EQ(records[4].memSize, movFpRecord.mSize * 32U);
}

TEST(AddressSanitizer, parse_mov_fp_record_with_int8_channal_merging_expect_get_correct_memory_records)
{
    MovFpRecord movFpRecord{};
    movFpRecord.dst = DST_ADDR;
    movFpRecord.src = SRC0_ADDR;
    movFpRecord.dstStride = 32;
    movFpRecord.srcStride = 32;
    movFpRecord.nSize = 48;
    movFpRecord.mSize = 32;
    movFpRecord.ndNum = 0;
    movFpRecord.dstNdStride = 0;
    movFpRecord.srcNdStride = 0;
    movFpRecord.quantPreBits = 8;
    movFpRecord.int8ChannelMerge = true;
    movFpRecord.int4ChannelMerge = false;
    movFpRecord.channelSplit = false;
    movFpRecord.enNZ2ND = false;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::MOV_FP;
    kernelRecord.payload.movFpRecord = movFpRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ConvertSanEventsToMemOpRecords(events, records);
    uint16_t columnNum = (movFpRecord.nSize + 16U - 1) / 16U;  // src中N维的列数
    ASSERT_EQ(records.size(), columnNum);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, movFpRecord.mSize * 64U * columnNum);
    ASSERT_EQ(records[1].type, MemOpType::STORE);
    ASSERT_EQ(records[1].dstAddr, DST_ADDR);
    ASSERT_EQ(records[1].memSize, movFpRecord.mSize * 32U);
    ASSERT_EQ(records[2].type, MemOpType::STORE);
    ASSERT_EQ(records[2].dstAddr, DST_ADDR + movFpRecord.dstStride * 32U);
    ASSERT_EQ(records[2].memSize, movFpRecord.mSize * 16U);
}

TEST(AddressSanitizer, parse_mov_fp_record_with_int4_channal_merging_expect_get_correct_memory_records)
{
    MovFpRecord movFpRecord{};
    movFpRecord.dst = DST_ADDR;
    movFpRecord.src = SRC0_ADDR;
    movFpRecord.dstStride = 32;
    movFpRecord.srcStride = 32;
    movFpRecord.nSize = 128;
    movFpRecord.mSize = 32;
    movFpRecord.ndNum = 0;
    movFpRecord.dstNdStride = 0;
    movFpRecord.srcNdStride = 0;
    movFpRecord.quantPreBits = 4;
    movFpRecord.int8ChannelMerge = false;
    movFpRecord.int4ChannelMerge = true;
    movFpRecord.channelSplit = false;
    movFpRecord.enNZ2ND = false;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::MOV_FP;
    kernelRecord.payload.movFpRecord = movFpRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ConvertSanEventsToMemOpRecords(events, records);
    uint16_t columnNum = (movFpRecord.nSize + 16U - 1) / 16U;  // src中N维的列数
    uint16_t mergeColumn = movFpRecord.nSize / 64U;  // dst中N维的列数
    ASSERT_EQ(records.size(), mergeColumn);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, movFpRecord.mSize * 64U * columnNum);
    ASSERT_EQ(records[1].type, MemOpType::STORE);
    ASSERT_EQ(records[1].dstAddr, DST_ADDR);
    ASSERT_EQ(records[1].memSize, movFpRecord.mSize * 32U * mergeColumn);
}

TEST(AddressSanitizer, parse_mov_fp_record_with_f32_channel_split_expect_get_correct_memory_records)
{
    MovFpRecord movFpRecord{};
    movFpRecord.dst = DST_ADDR;
    movFpRecord.src = SRC0_ADDR;
    movFpRecord.dstStride = 64;
    movFpRecord.srcStride = 64;
    movFpRecord.nSize = 24;
    movFpRecord.mSize = 64;
    movFpRecord.ndNum = 0;
    movFpRecord.dstNdStride = 0;
    movFpRecord.srcNdStride = 0;
    movFpRecord.quantPreBits = 32;
    movFpRecord.int8ChannelMerge = false;
    movFpRecord.int4ChannelMerge = false;
    movFpRecord.channelSplit = true;
    movFpRecord.enNZ2ND = false;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::MOV_FP;
    kernelRecord.payload.movFpRecord = movFpRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ConvertSanEventsToMemOpRecords(events, records);
    uint16_t columnNum = (movFpRecord.nSize + 16U - 1) / 16U;  // src中N维的列数
    uint16_t splitColumn = movFpRecord.nSize / 8U;  // dst中N维的列数
    ASSERT_EQ(records.size(), columnNum);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, movFpRecord.mSize * 64U * columnNum);
    ASSERT_EQ(records[1].type, MemOpType::STORE);
    ASSERT_EQ(records[1].dstAddr, DST_ADDR);
    ASSERT_EQ(records[1].memSize, movFpRecord.mSize * 32U * splitColumn);
}

TEST(AddressSanitizer, parse_mov_fp_record_with_NZ2ND_s32_expect_get_correct_memory_records)
{
    MovFpRecord movFpRecord{};
    movFpRecord.dst = DST_ADDR;
    movFpRecord.src = SRC0_ADDR;
    movFpRecord.dstStride = 48;
    movFpRecord.srcStride = 80;
    movFpRecord.nSize = 24;
    movFpRecord.mSize = 48;
    movFpRecord.ndNum = 2;
    movFpRecord.dstNdStride = 64 * movFpRecord.dstStride;
    movFpRecord.srcNdStride = 15;
    movFpRecord.quantPreBits = 32;
    movFpRecord.int8ChannelMerge = false;
    movFpRecord.int4ChannelMerge = false;
    movFpRecord.channelSplit = false;
    movFpRecord.enNZ2ND = true;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::MOV_FP;
    kernelRecord.payload.movFpRecord = movFpRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), movFpRecord.ndNum * (movFpRecord.mSize * 2 + 1));
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, movFpRecord.mSize * 64U);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + movFpRecord.srcNdStride * 1024U);
    ASSERT_EQ(records[1].memSize, movFpRecord.mSize * 64U);
    ASSERT_EQ(records[2].type, MemOpType::LOAD);
    ASSERT_EQ(records[2].dstAddr, SRC0_ADDR + movFpRecord.srcStride * 64U);
    ASSERT_EQ(records[2].memSize, 32U);
    ASSERT_EQ(records[98].type, MemOpType::STORE);
    ASSERT_EQ(records[98].dstAddr, DST_ADDR);
    ASSERT_EQ(records[98].memSize, movFpRecord.nSize * sizeof(int32_t));
    ASSERT_EQ(records[99].type, MemOpType::STORE);
    ASSERT_EQ(records[99].dstAddr, DST_ADDR + movFpRecord.dstStride * sizeof(int32_t));
    ASSERT_EQ(records[99].memSize, movFpRecord.nSize * sizeof(int32_t));
}

TEST(AddressSanitizer, parse_mov_fp_record_with_NZ2ND_s16_expect_get_correct_memory_records)
{
    MovFpRecord movFpRecord{};
    movFpRecord.dst = DST_ADDR;
    movFpRecord.src = SRC0_ADDR;
    movFpRecord.dstStride = 24;
    movFpRecord.srcStride = 80;
    movFpRecord.nSize = 24;
    movFpRecord.mSize = 48;
    movFpRecord.ndNum = 2;
    movFpRecord.dstNdStride = 64 * movFpRecord.dstStride;
    movFpRecord.srcNdStride = 15;
    movFpRecord.quantPreBits = 16;
    movFpRecord.int8ChannelMerge = false;
    movFpRecord.int4ChannelMerge = false;
    movFpRecord.channelSplit = false;
    movFpRecord.enNZ2ND = true;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::MOV_FP;
    kernelRecord.payload.movFpRecord = movFpRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 100);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, movFpRecord.mSize * 64U);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + movFpRecord.srcNdStride * 1024U);
    ASSERT_EQ(records[1].memSize, movFpRecord.mSize * 64U);
    ASSERT_EQ(records[2].type, MemOpType::LOAD);
    ASSERT_EQ(records[2].dstAddr, SRC0_ADDR + movFpRecord.srcStride * 64U);
    ASSERT_EQ(records[2].memSize, 32U);
    ASSERT_EQ(records[98].type, MemOpType::STORE);
    ASSERT_EQ(records[98].dstAddr, DST_ADDR);
    ASSERT_EQ(records[98].memSize, movFpRecord.nSize * sizeof(int16_t) * 48U);
    ASSERT_EQ(records[99].type, MemOpType::STORE);
    ASSERT_EQ(records[99].dstAddr, DST_ADDR + movFpRecord.dstStride * sizeof(int16_t) * 64U);
    ASSERT_EQ(records[99].memSize, movFpRecord.nSize * sizeof(int16_t) * 48U);
}

TEST(AddressSanitizer, parse_mov_fp_record_with_NZ2ND_s8_expect_get_correct_memory_records)
{
    MovFpRecord movFpRecord{};
    movFpRecord.dst = DST_ADDR;
    movFpRecord.src = SRC0_ADDR;
    movFpRecord.dstStride = 24;
    movFpRecord.srcStride = 80;
    movFpRecord.nSize = 24;
    movFpRecord.mSize = 48;
    movFpRecord.ndNum = 2;
    movFpRecord.dstNdStride = 64 * movFpRecord.dstStride;
    movFpRecord.srcNdStride = 15;
    movFpRecord.quantPreBits = 8;
    movFpRecord.int8ChannelMerge = false;
    movFpRecord.int4ChannelMerge = false;
    movFpRecord.channelSplit = false;
    movFpRecord.enNZ2ND = true;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::MOV_FP;
    kernelRecord.payload.movFpRecord = movFpRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 100);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, movFpRecord.mSize * 64U);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + movFpRecord.srcNdStride * 1024U);
    ASSERT_EQ(records[1].memSize, movFpRecord.mSize * 64U);
    ASSERT_EQ(records[2].type, MemOpType::LOAD);
    ASSERT_EQ(records[2].dstAddr, SRC0_ADDR + movFpRecord.srcStride * 64U);
    ASSERT_EQ(records[2].memSize, 32U);
    ASSERT_EQ(records[98].type, MemOpType::STORE);
    ASSERT_EQ(records[98].dstAddr, DST_ADDR);
    ASSERT_EQ(records[98].memSize, movFpRecord.nSize * sizeof(int8_t) * 48U);
    ASSERT_EQ(records[99].type, MemOpType::STORE);
    ASSERT_EQ(records[99].dstAddr, DST_ADDR + movFpRecord.dstStride * sizeof(int8_t) * 64U);
    ASSERT_EQ(records[99].memSize, movFpRecord.nSize * sizeof(int8_t) * 48U);
}

TEST(AddressSanitizer, parse_mov_fp_record_with_NZ2ND_s4_expect_get_correct_memory_records)
{
    MovFpRecord movFpRecord{};
    movFpRecord.dst = DST_ADDR;
    movFpRecord.src = SRC0_ADDR;
    movFpRecord.dstStride = 24;
    movFpRecord.srcStride = 80;
    movFpRecord.nSize = 24;
    movFpRecord.mSize = 48;
    movFpRecord.ndNum = 2;
    movFpRecord.dstNdStride = 64 * movFpRecord.dstStride;
    movFpRecord.srcNdStride = 15;
    movFpRecord.quantPreBits = 4;
    movFpRecord.int8ChannelMerge = false;
    movFpRecord.int4ChannelMerge = false;
    movFpRecord.channelSplit = false;
    movFpRecord.enNZ2ND = true;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::MOV_FP;
    kernelRecord.payload.movFpRecord = movFpRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 100);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, movFpRecord.mSize * 64U);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, SRC0_ADDR + movFpRecord.srcNdStride * 1024U);
    ASSERT_EQ(records[1].memSize, movFpRecord.mSize * 64U);
    ASSERT_EQ(records[2].type, MemOpType::LOAD);
    ASSERT_EQ(records[2].dstAddr, SRC0_ADDR + movFpRecord.srcStride * 64U);
    ASSERT_EQ(records[2].memSize, 32U);
    ASSERT_EQ(records[98].type, MemOpType::STORE);
    ASSERT_EQ(records[98].dstAddr, DST_ADDR);
    ASSERT_EQ(records[98].memSize, movFpRecord.nSize / 2* 48U);
    ASSERT_EQ(records[99].type, MemOpType::STORE);
    ASSERT_EQ(records[99].dstAddr, DST_ADDR + movFpRecord.dstStride / 2 * 64U);
    ASSERT_EQ(records[99].memSize, movFpRecord.nSize / 2* 48U);
}

TEST(AddressSanitizer, parse_vec_dup_records)
{
    VecDupRecord vecDupRecord{};
    vecDupRecord.dst = DST_ADDR;
    vecDupRecord.repeat = 1;
    vecDupRecord.dstBlockStride = 1;
    vecDupRecord.dstRepeatStride = 1;
    vecDupRecord.maskMode = MaskMode::MASK_NORM;
    vecDupRecord.vectorMask = {static_cast<uint64_t>(-1), static_cast<uint64_t>(-1)};

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::VEC_DUP;
    kernelRecord.payload.vecDupRecord = vecDupRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].type, MemOpType::STORE);
    ASSERT_EQ(records[0].dstAddr, DST_ADDR);
    ASSERT_EQ(records[0].memSize, 32 * 8);
}

TEST(AddressSanitizer, parse_muti_repeat_vec_dup_records)
{
    VecDupRecord vecDupRecord{};
    vecDupRecord.dst = DST_ADDR;
    vecDupRecord.repeat = 3;
    vecDupRecord.dstBlockStride = 1;
    vecDupRecord.dstRepeatStride = 1;
    vecDupRecord.maskMode = MaskMode::MASK_NORM;
    vecDupRecord.vectorMask = {static_cast<uint64_t>(-1), static_cast<uint64_t>(-1)};

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::VEC_DUP;
    kernelRecord.payload.vecDupRecord = vecDupRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].type, MemOpType::STORE);
    ASSERT_EQ(records[0].dstAddr, DST_ADDR);
    ASSERT_EQ(records[0].memSize, 32 * (8 + 2));
}

TEST(AddressSanitizer, parse_count_model_mask0_zero_vec_dup_records)
{
    VecDupRecord vecDupRecord{};
    vecDupRecord.dst = DST_ADDR;
    vecDupRecord.repeat = 3;
    vecDupRecord.dstBlockStride = 1;
    vecDupRecord.dstRepeatStride = 1;
    vecDupRecord.maskMode = MaskMode::MASK_COUNT;
    vecDupRecord.vectorMask = {static_cast<uint64_t>(0), static_cast<uint64_t>(-1)};

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::VEC_DUP;
    kernelRecord.payload.vecDupRecord = vecDupRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
}

static Load2DRecord CreateLoad2DRecordStruct(void)
{
    Load2DRecord record{};
    record.dst = 0x612;
    record.src = 0x13;
    record.baseIdx = 11;
    record.repeat = 33;
    record.addrCalMode = AddrCalMode::DEC;
    record.srcStride = 1;
    record.dstStride = 2;
    record.blockSize = 512;
    record.location.fileNo = 111;
    record.location.lineNo = 222;

    record.srcMemType = MemType::L1;
    record.dstMemType = MemType::L0A;
    record.location.blockId = 10;

    return record;
}

TEST(AddressSanitizer, parse_load_2d_records)
{
    uint64_t load2dTransBytes = 512ULL;
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::LOAD_2D;
    kernelRecord.payload.load2DRecord = CreateLoad2DRecordStruct();
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
    sanitizerRecord.payload.kernelRecord.payload.load2DRecord.repeat = 3;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 4);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, 0x13 + (11 * load2dTransBytes) - (2 * load2dTransBytes * 1));
    ASSERT_EQ(records[0].memSize, load2dTransBytes * 3U);
    ASSERT_EQ(records[1].type, MemOpType::STORE);
    ASSERT_EQ(records[1].dstAddr, 0x612);
    ASSERT_EQ(records[1].memSize, load2dTransBytes);
    ASSERT_EQ(records[2].type, MemOpType::STORE);
    ASSERT_EQ(records[2].dstAddr, 0x612 +
        (1 * load2dTransBytes * 2));
    ASSERT_EQ(records[2].memSize, load2dTransBytes);
    ASSERT_EQ(records[3].type, MemOpType::STORE);
    ASSERT_EQ(records[3].dstAddr, 0x612 +
        (2 * load2dTransBytes * 2));
    ASSERT_EQ(records[3].memSize, load2dTransBytes);
}

static Load2DTransposeRecord CreateLoad2DTransposeRecordStruct(void)
{
    constexpr uint64_t srcStride = 8;
    constexpr uint64_t dstStride = 3;
    Load2DTransposeRecord record{};
    record.dst = 0x612;
    record.src = 0x13;
    record.indexId = 0;
    record.repeat = 0;
    record.addrMode = false;
    record.srcStride = srcStride;
    record.dstStride = dstStride;
    record.dstFracStride = 0;

    record.dataType = DataType::DATA_B4;
    record.srcMemType = MemType::L1;
    record.dstMemType = MemType::L0B;

    return record;
}

TEST(AddressSanitizer, parse_load_2d_transpose_records)
{
    uint64_t load2dTransBytes = 512ULL;
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::LOAD_2D_TRANSPOSE;
    kernelRecord.payload.load2DTransposeRecord = CreateLoad2DTransposeRecordStruct();
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
    sanitizerRecord.payload.kernelRecord.payload.load2DTransposeRecord.repeat = 2;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 3);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, 0x13 + (4 * 8 * load2dTransBytes));
    ASSERT_EQ(records[1].memSize, 4 * load2dTransBytes);
    ASSERT_EQ(records[2].type, MemOpType::STORE);
    ASSERT_EQ(records[2].dstAddr, 0x612);
    ASSERT_EQ(records[2].memSize, 4 * load2dTransBytes * 2);
}

static BroadcastRecord CreateBroadcastRecordStruct(void)
{
    constexpr uint32_t blockNum = 2;
    constexpr uint32_t srcGapNum = 1;
    constexpr uint32_t dstGapNum = 2;
    constexpr uint32_t fileNo = 111;
    constexpr uint32_t lineNo = 222;
    BroadcastRecord record{};
    record.dst = 0x612;
    record.src = 0x13;
    record.nBurst = 0;
    record.lenBurst = blockNum;
    record.enableRepeat = false;
    record.srcGap = srcGapNum;
    record.dstGap = dstGapNum;
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
 
    record.srcDataType = DataType::DATA_B16;
    record.dstDataType = DataType::DATA_B16;
    record.srcMemType = MemType::UB;
    record.dstMemType = MemType::L0C;
 
    return record;
}
 
TEST(AddressSanitizer, parse_broadcast_records)
{
    uint64_t broadcastTransBytes = 512ULL;
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::BROADCAST;
    kernelRecord.payload.broadcastRecord = CreateBroadcastRecordStruct();
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;
 
    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 0);
    sanitizerRecord.payload.kernelRecord.payload.broadcastRecord.nBurst = 2;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 4);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, 0x13 + 32 * 3);
    ASSERT_EQ(records[1].memSize, 32 * 2);
    ASSERT_EQ(records[3].type, MemOpType::STORE);
    ASSERT_EQ(records[3].dstAddr, 0x612 + broadcastTransBytes * 4);
    ASSERT_EQ(records[3].memSize, broadcastTransBytes * 2);
}

static Load3DRecord CreateLoad3DRecordStruct(void)
{
    Load3DRecord record{};
    record.dst = 0;
    record.src = 0;
    record.srcMemType = MemType::L1;
    record.dstMemType = MemType::L0A;
    record.dataType = DataType::DATA_B16;
    record.fMapW = LOAD3D_FMAP_LENGTH;
    record.fMapH = LOAD3D_FMAP_LENGTH;
    record.fMapC = LOAD3D_FMAP_LENGTH;
    record.filterW = LOAD3D_FILTER_LENGTH;
    record.filterH = LOAD3D_FILTER_LENGTH;
    record.filterWStride = LOAD3D_FILTER_STRIDE;
    record.filterHStride = LOAD3D_FILTER_STRIDE;
    record.matrixKPos = LOAD3D_MATRIX_POS;
    record.matrixMPos = LOAD3D_MATRIX_POS;
    record.matrixKStep = LOAD3D_MATRIX_KSTEP;
    record.matrixMStep = LOAD3D_MATRIX_MSTEP;
    record.filterWDilation = LOAD3D_FILTER_DILATION;
    record.filterHDilation = LOAD3D_FILTER_DILATION;
    record.fMapTopPad = LOAD3D_FMAP_PAD;
    record.fMapBottomPad = LOAD3D_FMAP_PAD;
    record.fMapLeftPad = LOAD3D_FMAP_PAD;
    record.fMapRightPad = LOAD3D_FMAP_PAD;
    record.matrixRptMode = LOAD3D_RPT_MODE;
    record.matrixRptTimes = LOAD3D_RPT_TIMES;
    record.matrixRptStride = LOAD3D_RPT_STRIDE;
    return record;
}

TEST(AddressSanitizer, parse_load_3d_records)
{
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::LOAD_3D;
    kernelRecord.payload.load3DRecord = CreateLoad3DRecordStruct();
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2210);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 2210);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, 512);
    ASSERT_EQ(records[1].memSize, 32);
    ASSERT_EQ(records[2209].type, MemOpType::STORE);
    ASSERT_EQ(records[2209].dstAddr, 0);
    ASSERT_EQ(records[2209].memSize, 288);
}

LoadB2Record CreateLoadB2RecordStruct()
{
    constexpr uint16_t dstAddr = 512;
    constexpr uint16_t srcAddr = 32;

    LoadB2Record record{};
        record.location.blockId = 1;
    record.dataType = DataType::DATA_B8;
    record.dst = dstAddr;
    record.src = srcAddr;
        record.dstMemType = MemType::L0B;
    record.srcMemType = MemType::L1;
    record.repeat = 1;
    record.location.fileNo = 1;
    record.location.lineNo = 1;
    return record;
}

TEST(AddressSanitizer, parse_load_b2_records)
{
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::LOAD_B2;
    kernelRecord.payload.loadB2Record = CreateLoadB2RecordStruct();
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);

    ASSERT_EQ(records.size(), 2);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, 32);
    ASSERT_EQ(records[0].memSize, 512);
    ASSERT_EQ(records[1].type, MemOpType::STORE);
    ASSERT_EQ(records[1].dstAddr, 512);
    ASSERT_EQ(records[1].memSize, 512);
}

LoadAWinogradRecord CreateLoadAWinogradRecord()
{
    constexpr int32_t dstAddr = 512;
    constexpr int32_t srcAddr = 32;
    constexpr int32_t fileNo = 1;
    constexpr int32_t lineNo = 1;
    constexpr int32_t fmSizeH = 16;
    constexpr int32_t fmSizeW = 8;
    constexpr int32_t fmSizeCh = 4;
    constexpr int32_t extStepK = 32;
    constexpr int32_t extStepM = 32;

    LoadAWinogradRecord record{};
    record.location.blockId = 1;
    record.dataType = DataType::DATA_B8;
    record.dst = dstAddr;
    record.src = srcAddr;
    record.dstMemType = MemType::L0A;
    record.srcMemType = MemType::L1;
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
    record.fmSizeH = fmSizeH;
    record.fmSizeW = fmSizeW;
    record.fmSizeCh = fmSizeCh;
    record.innerDstGap = 1;
    record.dstStartPointK = 0;
    record.extStepK = extStepK;
    record.dstStartPointM = 0;
    record.extStepM = extStepM;
    return record;
}

TEST(AddressSanitizer, parse_load_A_Winograd_records)
{
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::LOAD_A_WINOGRAD;
    kernelRecord.payload.loadAWinogradRecord = CreateLoadAWinogradRecord();
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 5);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, 32);
    ASSERT_EQ(records[0].memSize, 512);
    ASSERT_EQ(records[1].type, MemOpType::STORE);
    ASSERT_EQ(records[1].dstAddr, 512);
    ASSERT_EQ(records[1].memSize, 1024);
    ASSERT_EQ(records[2].type, MemOpType::STORE);
    ASSERT_EQ(records[2].dstAddr, 512 + 1536);
    ASSERT_EQ(records[2].memSize, 1024);
    ASSERT_EQ(records[3].type, MemOpType::STORE);
    ASSERT_EQ(records[3].dstAddr, 512 + 1536*2);
    ASSERT_EQ(records[3].memSize, 1024);
    ASSERT_EQ(records[4].type, MemOpType::STORE);
    ASSERT_EQ(records[4].dstAddr, 512 + 1536*3);
    ASSERT_EQ(records[4].memSize, 1024);
}

LoadBWinogradRecord CreateLoadBWinogradRecord()
{
    constexpr int32_t dstAddr = 512;
    constexpr int32_t srcAddr = 32;
    constexpr int32_t repeatTime = 2;
    constexpr int32_t srcRptStride = 10;
    constexpr int32_t dstRptStride = 10;

    LoadBWinogradRecord record{};
    record.location.blockId = 1;
    record.dst = dstAddr;
    record.src = srcAddr;
    record.dstMemType = MemType::L0B;
    record.srcMemType = MemType::L1;
    record.repeat = repeatTime;
    record.location.fileNo = 1;
    record.location.lineNo = 1;
    record.srcRptStride = srcRptStride;
    record.dstRptStride = dstRptStride;
    record.innerDstStride = 1;
    return record;
}

TEST(AddressSanitizer, parse_load_B_Winograd_records)
{
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::LOAD_B_WINOGRAD;
    kernelRecord.payload.loadBWinogradRecord = CreateLoadBWinogradRecord();
    auto &record = kernelRecord.payload.loadBWinogradRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 4);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, 32);
    ASSERT_EQ(records[0].memSize, 512 * 9);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
    ASSERT_EQ(records[1].dstAddr, 32 +  record.srcRptStride * 512);
    ASSERT_EQ(records[1].memSize, 512 * 9);

    ASSERT_EQ(records[2].type, MemOpType::STORE);
    ASSERT_EQ(records[2].dstAddr, 512);
    ASSERT_EQ(records[2].memSize, 512 * 4);
    ASSERT_EQ(records[3].type, MemOpType::STORE);
    ASSERT_EQ(records[3].dstAddr, 512 + record.dstRptStride * 512);
    ASSERT_EQ(records[3].memSize, 512 * 4);
}

TEST(AddressSanitizer, parse_set_l0a_2d_records)
{
    Set2DRecord set2DRecord{};
    set2DRecord.dst = DST_ADDR;
    set2DRecord.repeat = 1;
    set2DRecord.dstMemType = MemType::L0A;
    set2DRecord.dstBlockNum = 1;
    set2DRecord.dstBlockSize = 512;
    set2DRecord.repeatGap = 0;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::SET_2D;
    kernelRecord.payload.set2DRecord = set2DRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].type, MemOpType::STORE);
    ASSERT_EQ(records[0].dstAddr, DST_ADDR);
    ASSERT_EQ(records[0].memSize, 512);
}

TEST(AddressSanitizer, parse_set_l1_2d_records)
{
    Set2DRecord set2DRecord{};
    set2DRecord.dst = DST_ADDR;
    set2DRecord.repeat = 1;
    set2DRecord.dstMemType = MemType::L1;
    set2DRecord.dstBlockNum = 1;
    set2DRecord.dstBlockSize = 32;
    set2DRecord.repeatGap = 0;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::SET_2D;
    kernelRecord.payload.set2DRecord = set2DRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].type, MemOpType::STORE);
    ASSERT_EQ(records[0].dstAddr, DST_ADDR);
    ASSERT_EQ(records[0].memSize, 32);
}

TEST(AddressSanitizer, parse_load_image_8_records)
{
    LoadImageRecord loadImageRecord{};
    loadImageRecord.dst = DST_ADDR;
    loadImageRecord.dstMemType = MemType::L1;
    loadImageRecord.dataType = DataType::DATA_B8;
    loadImageRecord.horSize = 80;
    loadImageRecord.verSize = 80;
    loadImageRecord.lPadSize = 10;
    loadImageRecord.rPadSize = 10;
    loadImageRecord.topPadSize = 10;
    loadImageRecord.botPadSize = 10;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::LOAD_IMAGE;
    kernelRecord.payload.loadImageRecord = loadImageRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].type, MemOpType::STORE);
    ASSERT_EQ(records[0].dstAddr, DST_ADDR);
    ASSERT_EQ(records[0].dstSpace, AddressSpace::L1);
    ASSERT_EQ(records[0].memSize, 320000);
}

LoadSmaskRecord GetLoadSmaskRecord(MemType srcMemType)
{
    LoadSmaskRecord record = {
        .location = {12, 34, 0, 111},
        .src = SRC0_ADDR,
        .dst = DST_ADDR,
        .smaskSize = 32,
        .srcMemType = srcMemType
    };
    return record;
}

TEST(AddressSanitizer, parse_load_smask_gm_records)
{
    LoadSmaskRecord loadSmaskRecord_gm = GetLoadSmaskRecord(MemType::GM);
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::LOAD_SMASK;
    kernelRecord.payload.loadSmaskRecord = loadSmaskRecord_gm;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].dstSpace, AddressSpace::GM);
    ASSERT_EQ(records[0].memSize, 32);
}

TEST(AddressSanitizer, parse_load_smask_ub_records)
{
    LoadSmaskRecord loadSmaskRecord_ub = GetLoadSmaskRecord(MemType::UB);
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::LOAD_SMASK;
    kernelRecord.payload.loadSmaskRecord = loadSmaskRecord_ub;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 1);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].dstSpace, AddressSpace::UB);
    ASSERT_EQ(records[0].memSize, 32);
}

TEST(AddressSanitizer, parse_unary_op_record_expect_get_correct_memory_records)
{
    UnaryOpRecord unaryOpRecord{};
    unaryOpRecord.dst = DST_ADDR;
    unaryOpRecord.src = SRC0_ADDR;
    unaryOpRecord.dstBlockStride = DST_BLOCK_STRIDE;
    unaryOpRecord.srcBlockStride = SRC0_BLOCK_STRIDE;
    unaryOpRecord.dstRepeatStride = DST_REPEAT_STRIDE;
    unaryOpRecord.srcRepeatStride = SRC0_REPEAT_STRIDE;
    unaryOpRecord.repeat = REPEAT;
    unaryOpRecord.dstBlockNum = DST_BLOCK_NUM;
    unaryOpRecord.srcBlockNum = SRC0_BLOCK_NUM;
    unaryOpRecord.dstBlockSize = DST_BLOCK_SIZE;
    unaryOpRecord.srcBlockSize = SRC0_BLOCK_SIZE;
    unaryOpRecord.dstDataBits = 8U;
    unaryOpRecord.srcDataBits = 8U;
    unaryOpRecord.vectorMask = { ~0UL, ~0UL };
    unaryOpRecord.maskMode = MaskMode::MASK_NORM;

    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::UNARY_OP;
    kernelRecord.payload.unaryOpRecord = unaryOpRecord;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 8);
    ASSERT_EQ(records[2].type, MemOpType::LOAD);
    ASSERT_EQ(records[2].dstAddr, SRC0_ADDR + SRC0_REPEAT_STRIDE * SRC0_BLOCK_SIZE);
    ASSERT_EQ(records[2].memSize, SRC0_BLOCK_SIZE);
    ASSERT_EQ(records[3].type, MemOpType::LOAD);
    ASSERT_EQ(records[3].dstAddr, SRC0_ADDR + (SRC0_REPEAT_STRIDE + SRC0_BLOCK_STRIDE) * SRC0_BLOCK_SIZE);
    ASSERT_EQ(records[3].memSize, SRC0_BLOCK_SIZE);
    ASSERT_EQ(records[6].type, MemOpType::STORE);
    ASSERT_EQ(records[6].dstAddr, DST_ADDR + DST_REPEAT_STRIDE * DST_BLOCK_SIZE);
    ASSERT_EQ(records[6].memSize, DST_BLOCK_SIZE);
    ASSERT_EQ(records[7].type, MemOpType::STORE);
    ASSERT_EQ(records[7].dstAddr, DST_ADDR + (DST_REPEAT_STRIDE + DST_BLOCK_STRIDE) * DST_BLOCK_SIZE);
    ASSERT_EQ(records[7].memSize, DST_BLOCK_SIZE);
}

static BinaryOpRecord CreateBinaryOpRecord(void)
{
    BinaryOpRecord binaryOpRecord{};
    binaryOpRecord.dst = DST_ADDR;
    binaryOpRecord.src0 = SRC0_ADDR;
    binaryOpRecord.src1 = SRC1_ADDR;
    binaryOpRecord.dstBlockStride = DST_BLOCK_STRIDE;
    binaryOpRecord.src0BlockStride = SRC0_BLOCK_STRIDE;
    binaryOpRecord.src1BlockStride = SRC1_BLOCK_STRIDE;
    binaryOpRecord.dstRepeatStride = DST_REPEAT_STRIDE;
    binaryOpRecord.src0RepeatStride = SRC0_REPEAT_STRIDE;
    binaryOpRecord.src1RepeatStride = SRC1_REPEAT_STRIDE;
    binaryOpRecord.repeat = REPEAT;
    binaryOpRecord.dstBlockNum = DST_BLOCK_NUM;
    binaryOpRecord.src0BlockNum = SRC0_BLOCK_NUM;
    binaryOpRecord.src1BlockNum = SRC1_BLOCK_NUM;
    binaryOpRecord.dstBlockSize = DST_BLOCK_SIZE;
    binaryOpRecord.src0BlockSize = SRC0_BLOCK_SIZE;
    binaryOpRecord.src1BlockSize = SRC1_BLOCK_SIZE;
    binaryOpRecord.maskMode = MaskMode::MASK_NORM;
    binaryOpRecord.vectorMask = {static_cast<uint64_t>(-1), static_cast<uint64_t>(-1)};
    return binaryOpRecord;
}

TEST(AddressSanitizer, parse_ternary_op_record_expect_get_correct_memory_records)
{
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::TERNARY_OP;
    kernelRecord.payload.binaryOpRecord = CreateBinaryOpRecord();
    kernelRecord.blockType = BlockType::AIVEC;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 3);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 12);
    ASSERT_EQ(records[3].type, MemOpType::LOAD);
    ASSERT_EQ(records[3].dstAddr, SRC0_ADDR + (SRC0_REPEAT_STRIDE + SRC0_BLOCK_STRIDE) * SRC0_BLOCK_SIZE);
    ASSERT_EQ(records[3].memSize, SRC0_BLOCK_SIZE);
    ASSERT_EQ(records[7].type, MemOpType::LOAD);
    ASSERT_EQ(records[7].dstAddr, SRC1_ADDR + (SRC1_REPEAT_STRIDE + SRC1_BLOCK_STRIDE) * SRC1_BLOCK_SIZE);
    ASSERT_EQ(records[7].memSize, SRC1_BLOCK_SIZE);
    ASSERT_EQ(records[11].type, MemOpType::MEMCPY_BLOCKS);
    ASSERT_EQ(records[11].srcAddr, DST_ADDR + (DST_REPEAT_STRIDE + DST_BLOCK_STRIDE) * DST_BLOCK_SIZE);
    ASSERT_EQ(records[11].dstAddr, DST_ADDR + (DST_REPEAT_STRIDE + DST_BLOCK_STRIDE) * DST_BLOCK_SIZE);
    ASSERT_EQ(records[11].memSize, DST_BLOCK_SIZE);
}

static ReduceOpRecord CreateReduceOpRecord(RecordType recordType)
{
    ReduceOpRecord reduceOpRecord{};
    reduceOpRecord.dst = DST_ADDR;
    reduceOpRecord.src = SRC0_ADDR;
    reduceOpRecord.srcBlockStride = SRC0_BLOCK_STRIDE;
    reduceOpRecord.dstRepeatStride = DST_REPEAT_STRIDE;
    reduceOpRecord.srcRepeatStride = SRC0_REPEAT_STRIDE;
    reduceOpRecord.dstRepeatLength = REDUCE_REPEAT_LENGTH;
    reduceOpRecord.repeat = REPEAT;
    reduceOpRecord.dstBlockNum = REDUCE_DST_BLOCK_NUM;
    reduceOpRecord.srcBlockNum = SRC0_BLOCK_NUM;
    reduceOpRecord.dstBlockSize = reduceOpRecord.dstRepeatLength;
    reduceOpRecord.srcBlockSize = SRC0_BLOCK_SIZE;
    reduceOpRecord.dstDataBits = 8U;
    reduceOpRecord.srcDataBits = 8U;
    reduceOpRecord.vectorMask = { ~0UL, ~0UL };
    return reduceOpRecord;
}

TEST(AddressSanitizer, parse_reduce_op_record_expect_get_correct_memory_records)
{
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::REDUCE_OP;
    kernelRecord.payload.reduceOpRecord = CreateReduceOpRecord(kernelRecord.recordType);
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 6);
    ASSERT_EQ(records[2].type, MemOpType::LOAD);
    ASSERT_EQ(records[2].dstAddr, SRC0_ADDR + SRC0_REPEAT_STRIDE * SRC0_BLOCK_SIZE);
    ASSERT_EQ(records[2].memSize, SRC0_BLOCK_SIZE);
    ASSERT_EQ(records[3].type, MemOpType::LOAD);
    ASSERT_EQ(records[3].dstAddr, SRC0_ADDR + (SRC0_REPEAT_STRIDE + SRC0_BLOCK_STRIDE) * SRC0_BLOCK_SIZE);
    ASSERT_EQ(records[3].memSize, SRC0_BLOCK_SIZE);
    ASSERT_EQ(records[5].type, MemOpType::STORE);
    ASSERT_EQ(records[5].dstAddr, DST_ADDR + DST_REPEAT_STRIDE * REDUCE_REPEAT_LENGTH);
    ASSERT_EQ(records[5].memSize, REDUCE_REPEAT_LENGTH);
}

static MarixMulOpRecord CreateMatrixMulOpRecord(void)
{
    MarixMulOpRecord matrixMulOpRecord{};
    matrixMulOpRecord.dst = DST_ADDR;
    matrixMulOpRecord.src0 = SRC0_ADDR;
    matrixMulOpRecord.src1 = SRC1_ADDR;
    matrixMulOpRecord.src0RepeatStride = SRC0_REPEAT_STRIDE;
    matrixMulOpRecord.src0Repeat = REPEAT;
    matrixMulOpRecord.dstBlockNum = DST_BLOCK_NUM;
    matrixMulOpRecord.src0BlockNum = SRC0_BLOCK_NUM;
    matrixMulOpRecord.src1BlockNum = SRC1_BLOCK_NUM;
    matrixMulOpRecord.dstBlockSize = DST_BLOCK_SIZE;
    matrixMulOpRecord.src0BlockSize = SRC0_BLOCK_SIZE;
    matrixMulOpRecord.src1BlockSize = SRC1_BLOCK_SIZE;
    matrixMulOpRecord.cmatrixInitVal = 0;
    matrixMulOpRecord.cmatrixSource = 0;
    matrixMulOpRecord.enUnitFlag = true;
    return matrixMulOpRecord;
}

TEST(AddressSanitizer, parse_matrix_mul_l0c_op_record_expect_get_correct_memory_records)
{
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::MATRIX_MUL_OP;
    kernelRecord.payload.matrixMulOpRecord = CreateMatrixMulOpRecord();
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;

    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 5);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 4);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[0].dstSpace, AddressSpace::L0A);
    ASSERT_EQ(records[0].dstAddr, SRC0_ADDR);
    ASSERT_EQ(records[0].memSize, SRC0_BLOCK_SIZE * SRC0_BLOCK_NUM);
    ASSERT_EQ(records[2].type, MemOpType::LOAD);
    ASSERT_EQ(records[2].dstSpace, AddressSpace::L0B);
    ASSERT_EQ(records[2].dstAddr, SRC1_ADDR);
    ASSERT_EQ(records[2].memSize, SRC1_BLOCK_SIZE * SRC1_BLOCK_NUM);
    ASSERT_EQ(records[3].type, MemOpType::MEMCPY_BLOCKS);
    ASSERT_EQ(records[3].dstSpace, AddressSpace::L0C);
    ASSERT_EQ(records[3].srcAddr, DST_ADDR);
    ASSERT_EQ(records[3].dstAddr, DST_ADDR);
    ASSERT_EQ(records[3].memSize, DST_BLOCK_SIZE * DST_BLOCK_NUM);
}

TEST(AddressSanitizer, parse_vconcat_record)
{
    KernelRecord record;
    constexpr uint64_t src = 0x100;
    constexpr uint64_t dst = 0x500;
    constexpr uint8_t coreID = 7;
    constexpr uint8_t regionRange = 0;
    constexpr uint8_t repeat = 3;
    constexpr uint8_t ppsNum = 16;
    constexpr DataType dataType = DataType::DATA_B16;
    constexpr uint8_t eleByte = (dataType == DataType::DATA_B16 ? 2 : 4);

    record.recordType = RecordType::VEC_REGPROPCOOR_OP;
    record.payload.vecRegPropCoordOpRecord.location.blockId = coreID;
    record.payload.vecRegPropCoordOpRecord.dst = dst;
    record.payload.vecRegPropCoordOpRecord.src = src;
    record.payload.vecRegPropCoordOpRecord.regionRange = regionRange;
    record.payload.vecRegPropCoordOpRecord.repeat = repeat;
    record.payload.vecRegPropCoordOpRecord.dataType = dataType;
    record.payload.vecRegPropCoordOpRecord.isExtract = false;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 48 + 1);

    uint8_t idx = 0;
    ASSERT_EQ(records[idx].coreId, coreID);
    ASSERT_EQ(records[idx].type, MemOpType::LOAD);
    ASSERT_EQ(records[idx].dstAddr, src);
    ASSERT_EQ(records[idx].dstSpace, AddressSpace::UB);
    ASSERT_EQ(records[idx].memSize, eleByte * ppsNum * 3);
    ASSERT_EQ(records[idx+1].coreId, coreID);
    ASSERT_EQ(records[idx+1].type, MemOpType::STORE);
    ASSERT_EQ(records[idx+1].dstAddr, dst);
    ASSERT_EQ(records[idx+1].dstSpace, AddressSpace::UB);
    ASSERT_EQ(records[idx+1].memSize, eleByte);

    record.payload.vecRegPropCoordOpRecord.isExtract = true;
    sanitizerRecord.payload.kernelRecord = record;
    records.clear();
    events.clear();
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 48 + 1);
    ASSERT_EQ(records[0].type, MemOpType::STORE);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
}

TEST(AddressSanitizer, parse_vconcat_record_core_id_equal_0)
{
    KernelRecord record;
    constexpr uint64_t src = 0x100;
    constexpr uint64_t dst = 0x500;
    constexpr uint8_t coreID = 0;
    constexpr uint8_t regionRange = 0;
    constexpr uint8_t repeat = 3;
    constexpr uint8_t ppsNum = 16;
    constexpr DataType dataType = DataType::DATA_B16;
    constexpr uint8_t eleByte = (dataType == DataType::DATA_B16 ? 2 : 4);

    record.recordType = RecordType::VEC_REGPROPCOOR_OP;
    record.payload.vecRegPropCoordOpRecord.location.blockId = coreID;
    record.payload.vecRegPropCoordOpRecord.dst = dst;
    record.payload.vecRegPropCoordOpRecord.src = src;
    record.payload.vecRegPropCoordOpRecord.regionRange = regionRange;
    record.payload.vecRegPropCoordOpRecord.repeat = repeat;
    record.payload.vecRegPropCoordOpRecord.dataType = dataType;
    record.payload.vecRegPropCoordOpRecord.isExtract = false;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = record;
    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 48 + 1);

    uint8_t idx = 0;
    ASSERT_EQ(records[idx].coreId, coreID);
    ASSERT_EQ(records[idx].type, MemOpType::LOAD);
    ASSERT_EQ(records[idx].dstAddr, src);
    ASSERT_EQ(records[idx].dstSpace, AddressSpace::UB);
    ASSERT_EQ(records[idx].memSize, eleByte * ppsNum * 3);
    ASSERT_EQ(records[idx+1].coreId, coreID);
    ASSERT_EQ(records[idx+1].type, MemOpType::STORE);
    ASSERT_EQ(records[idx+1].dstAddr, dst);
    ASSERT_EQ(records[idx+1].dstSpace, AddressSpace::UB);
    ASSERT_EQ(records[idx+1].memSize, eleByte);

    record.payload.vecRegPropCoordOpRecord.isExtract = true;
    sanitizerRecord.payload.kernelRecord = record;
    records.clear();
    events.clear();
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 2);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 48 + 1);
    ASSERT_EQ(records[0].type, MemOpType::STORE);
    ASSERT_EQ(records[1].type, MemOpType::LOAD);
}

TEST(AddressSanitizer, parse_mstx_wait_cross_records_with_two_times_expect_get_correct_records)
{
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::MSTX_STUB;

    MstxCrossRecord crossRecord = {
        .addr = 0x300,
        .flagId = 2,
        .pipe = PipeType::PIPE_V,
        .isMore = true,
        .isMerge = false,
    };

    MstxRecord record = {
        .interfaceType      = InterfaceType::MSTX_WAIT_CROSS_SYNC,
        .bufferLens         = sizeof(MstxCrossRecord),
        .location           = {10, 10, 0x200},
        .error              = false,
    };
    record.interface.mstxCrossRecord = crossRecord;

    kernelRecord.payload.mstxRecord = record;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;
    std::vector<SanEvent> events;

    /// 模拟多个wait
    int randm = RandInt(1, 20);
    for (int i = 0; i < randm; ++i) {
        RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
        sanitizerRecord.payload.kernelRecord.recordType = RecordType::LOAD;
        RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
        sanitizerRecord.payload.kernelRecord.recordType = RecordType::MSTX_STUB;
    }
    
    sanitizerRecord.payload.kernelRecord.payload.mstxRecord.interface.mstxCrossRecord.addr = 0x400;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    sanitizerRecord.payload.kernelRecord.recordType = RecordType::BLOCK_FINISH;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);

    /// 重置buffer和合并信息，防止对其他UT产生干扰；
    RecordPreProcess::GetInstance().ClearBuffer();
    RecordPreProcess::GetInstance().ResetWaitMergeInfo();
    /// buffer中事件为：wait1-1 -> load -> wait1-2 -> load ...... -> wait1-3 -> load -> wait2-1 -> BLOCK_FINISH
    /// 处理之后的记录为： wait1-1 -> load  ... load -> wait2-1
    ASSERT_EQ(events.size(), 2 + randm); // 2表示前后的wait

    ASSERT_EQ(events[0].eventInfo.mstxCrossInfo.addr, crossRecord.addr);
    ASSERT_EQ(events[0].eventInfo.mstxCrossInfo.flagId, crossRecord.flagId);
    ASSERT_EQ(events[0].eventInfo.mstxCrossInfo.isMore, crossRecord.isMore);
    ASSERT_EQ(events[0].eventInfo.mstxCrossInfo.isMerge, crossRecord.isMerge);
    ASSERT_EQ(events[0].eventInfo.mstxCrossInfo.pipe, crossRecord.pipe);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[events.size() - 1].eventInfo.mstxCrossInfo.addr, 0x400);
}

TEST(AddressSanitizer, parse_mstx_wait_cross_records_with_more_times_expect_get_correct_records)
{
    MstxCrossRecord crossRecord{};
    crossRecord.isMore = true;

    MstxRecord record = {
        .interfaceType      = InterfaceType::MSTX_WAIT_CROSS_SYNC,
        .bufferLens         = sizeof(MstxCrossRecord),
        .location           = {10, 10, 0x200},
        .error              = false,
    };
    record.interface.mstxCrossRecord = crossRecord;
    KernelRecord kernelRecord{};
    kernelRecord.payload.mstxRecord = record;
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;
    std::vector<SanEvent> events;

    /// 模拟多个wait，wait1_1 -> load -> wait2_1 -> load ->
    ///              wait1_2 -> load -> wait2_2 -> load ->
    ///              wait1_3 -> load -> wait2_3 -> load ->
    ///              ......
    int randm = RandInt(1, 20);
    for (int i = 0; i < randm; ++i) {
        sanitizerRecord.payload.kernelRecord.recordType = RecordType::MSTX_STUB;
        sanitizerRecord.payload.kernelRecord.payload.mstxRecord.interface.mstxCrossRecord.addr = 0x300;
        RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
        sanitizerRecord.payload.kernelRecord.recordType = RecordType::LOAD;
        RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
        sanitizerRecord.payload.kernelRecord.recordType = RecordType::MSTX_STUB;
        sanitizerRecord.payload.kernelRecord.payload.mstxRecord.interface.mstxCrossRecord.addr = 0x400;
        RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
        sanitizerRecord.payload.kernelRecord.recordType = RecordType::LOAD;
        RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    }

    sanitizerRecord.payload.kernelRecord.recordType = RecordType::MSTX_STUB;
    sanitizerRecord.payload.kernelRecord.payload.mstxRecord.interface.mstxCrossRecord.addr = 0x500;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    sanitizerRecord.payload.kernelRecord.recordType = RecordType::KERNEL_FINISH;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);

    /// 重置buffer和合并信息，防止对其他UT产生干扰；
    RecordPreProcess::GetInstance().ClearBuffer();
    RecordPreProcess::GetInstance().ResetWaitMergeInfo();
    /// buffer中事件为：wait1-1 -> load -> wait1-2 -> load ...... -> wait1-3 -> load -> wait2-1 -> BLOCK_FINISH
    /// 处理之后的记录为： wait1-1 -> load  ... load -> wait2-1
    uint8_t loadSize = randm * 2;
    ASSERT_EQ(events.size(), loadSize + 4); // 4表示有3个wait和一个FINISH

    ASSERT_EQ(events[0].eventInfo.mstxCrossInfo.addr, 0x300);
    ASSERT_EQ(events[1].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[2].eventInfo.mstxCrossInfo.addr, 0x400);
    ASSERT_EQ(events[events.size() - 2].eventInfo.mstxCrossInfo.addr, 0x500);
    ASSERT_EQ(events[events.size() - 1].type, EventType::SANITIZER_CONTROL_EVENT);
    ASSERT_EQ(events[events.size() - 1].eventInfo.sanitizerControlInfo.type, SanitizerControlType::KERNEL_FINISH);
}

TEST(AddressSanitizer, parse_ib_set_records_with_two_times_and_expect_get_correct_records)
{
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::IB_SET_STUB;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;
    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;

     /// load、IB_SET_STUB指令先入队
    sanitizerRecord.payload.kernelRecord.recordType = RecordType::LOAD;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    sanitizerRecord.payload.kernelRecord.recordType = RecordType::IB_SET_STUB;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);

    for (int i = 0; i < RandInt(1, 50); ++i) {
        sanitizerRecord.payload.kernelRecord.recordType = RecordType::LOAD;
        RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    }

    sanitizerRecord.payload.kernelRecord.recordType = RecordType::PIPE_BARRIER;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    /// 模拟第二个IB_SET_STUB
    sanitizerRecord.payload.kernelRecord.recordType = RecordType::IB_SET_STUB;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    sanitizerRecord.payload.kernelRecord.recordType = RecordType::LOAD;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);

    /// 事件序列为：load -> IB_SET -> load -> ...... -> load -> pipe_barrier -> IB_SET -> load
    /// 合并之后的记录为： load-> pipe_barrier -> IB_SET -> load
    ASSERT_EQ(events.size(), 4);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].type, EventType::SYNC_EVENT);
    ASSERT_EQ(events[2].type, EventType::CROSS_CORE_SOFT_SYNC_EVENT);
    ASSERT_EQ(events[3].type, EventType::MEM_EVENT);
}

TEST(AddressSanitizer, parse_ib_wait_records_with_two_times_and_expect_get_correct_records)
{
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::IB_WAIT_STUB;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;
    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;

     /// load、IB_WAIT_STUB指令先入队
    sanitizerRecord.payload.kernelRecord.recordType = RecordType::LOAD;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    sanitizerRecord.payload.kernelRecord.recordType = RecordType::IB_WAIT_STUB;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);

    for (int i = 0; i < RandInt(1, 50); ++i) {
        sanitizerRecord.payload.kernelRecord.recordType = RecordType::LOAD;
        RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    }

    sanitizerRecord.payload.kernelRecord.recordType = RecordType::PIPE_BARRIER;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    /// 模拟第二个IB_WAIT_STUB
    sanitizerRecord.payload.kernelRecord.recordType = RecordType::IB_WAIT_STUB;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    sanitizerRecord.payload.kernelRecord.recordType = RecordType::LOAD;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);

    /// 事件序列为：load -> IB_WAIT -> load -> ...... -> load -> pipe_barrier -> IB_WAIT -> load
    /// 合并之后的记录为： load-> pipe_barrier -> IB_WAIT -> load
    ASSERT_EQ(events.size(), 4);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].type, EventType::SYNC_EVENT);
    ASSERT_EQ(events[2].type, EventType::CROSS_CORE_SOFT_SYNC_EVENT);
    ASSERT_EQ(events[3].type, EventType::MEM_EVENT);
}

TEST(AddressSanitizer, parse_sync_all_records_with_two_times_expect_get_correct_records)
{
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::SYNC_ALL_STUB;

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;
    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;

     /// load、SYNC_ALL_STUB指令先入队
    sanitizerRecord.payload.kernelRecord.recordType = RecordType::LOAD;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    sanitizerRecord.payload.kernelRecord.recordType = RecordType::SYNC_ALL_STUB;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);

    for (int i = 0; i < RandInt(1, 50); ++i) {
        sanitizerRecord.payload.kernelRecord.recordType = RecordType::LOAD;
        RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    }

    sanitizerRecord.payload.kernelRecord.recordType = RecordType::PIPE_BARRIER;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    /// 模拟第二个SYNC_ALL_STUB
    sanitizerRecord.payload.kernelRecord.recordType = RecordType::SYNC_ALL_STUB;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    sanitizerRecord.payload.kernelRecord.recordType = RecordType::LOAD;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);

    /// 事件序列为：load -> SYNC_ALL_STUB -> load -> ...... -> load -> SYNC_ALL_STUB -> load
    /// 合并之后的记录为： load-> SYNC_ALL_STUB -> load
    ASSERT_EQ(events.size(), 5);
    ASSERT_EQ(events[0].type, EventType::MEM_EVENT);
    ASSERT_EQ(events[1].type, EventType::SYNC_EVENT);
    ASSERT_EQ(events[2].type, EventType::CROSS_CORE_SYNC_EVENT);
    ASSERT_EQ(events[3].type, EventType::CROSS_CORE_SYNC_EVENT);
    ASSERT_EQ(events[4].type, EventType::MEM_EVENT);
}

TEST(AddressSanitizer, parse_blockstride_equal_0_record_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 1;
    opInfo.blockStride = 0;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    AddressSanitizer::ConvertSanEventToMemOpRecords(event, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
}

TEST(AddressSanitizer, parse_blockstride_equal_0_blockSize_equal_0_record_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 0;
    opInfo.repeatTimes = 1;
    opInfo.blockStride = 0;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    AddressSanitizer::ConvertSanEventToMemOpRecords(event, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, 0);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
}

TEST(AddressSanitizer,
    parse_blockstride_equal_0_repeatStride_greater_than_blockNum_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 5;
    opInfo.blockStride = 0;
    opInfo.repeatStride = 20;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    AddressSanitizer::ConvertSanEventToMemOpRecords(event, records);
    ASSERT_EQ(records.size(), 5);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
    ASSERT_EQ(records[1].dstAddr, 0x200 + opInfo.repeatStride * opInfo.blockSize);
    ASSERT_EQ(records[1].memSize, opInfo.blockSize);
    ASSERT_FALSE(records[1].srcAddr == records[1].dstAddr);
    ASSERT_EQ(records[2].dstAddr, 0x200 + opInfo.repeatStride * opInfo.blockSize * 2);
    ASSERT_EQ(records[2].memSize, opInfo.blockSize);
    ASSERT_FALSE(records[2].srcAddr == records[2].dstAddr);
    ASSERT_EQ(records[3].dstAddr, 0x200 + opInfo.repeatStride * opInfo.blockSize * 3);
    ASSERT_EQ(records[3].memSize, opInfo.blockSize);
    ASSERT_FALSE(records[3].srcAddr == records[3].dstAddr);
    ASSERT_EQ(records[4].dstAddr, 0x200 + opInfo.repeatStride * opInfo.blockSize * 4);
    ASSERT_EQ(records[4].memSize, opInfo.blockSize);
    ASSERT_FALSE(records[4].srcAddr == records[4].dstAddr);
}

TEST(AddressSanitizer,
    parse_blockstride_equal_0_repeatStride_greater_than_blockStride_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 5;
    opInfo.blockStride = 0;
    opInfo.repeatStride = 2;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    AddressSanitizer::ConvertSanEventToMemOpRecords(event, records);
    ASSERT_EQ(records.size(), 5);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize);
    ASSERT_EQ(records[1].dstAddr, 0x200 + 32 * 2);
    ASSERT_EQ(records[1].memSize, opInfo.blockSize);
    ASSERT_EQ(records[2].dstAddr, 0x200 + 32 * 4);
    ASSERT_EQ(records[2].memSize, opInfo.blockSize);
    ASSERT_EQ(records[3].dstAddr, 0x200 + 32 * 6);
    ASSERT_EQ(records[3].memSize, opInfo.blockSize);
    ASSERT_EQ(records[4].dstAddr, 0x200 + 32 * 8);
    ASSERT_EQ(records[4].memSize, opInfo.blockSize);
}

TEST(AddressSanitizer, parse_blockstride_equal_0_repeatStride_equal_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 5;
    opInfo.blockStride = 0;
    opInfo.repeatStride = opInfo.blockNum;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    AddressSanitizer::ConvertSanEventToMemOpRecords(event, records);
    ASSERT_EQ(records.size(), 5);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize);
    ASSERT_EQ(records[1].dstAddr, 0x200 + 32 * 8);
    ASSERT_EQ(records[1].memSize, opInfo.blockSize);
    ASSERT_EQ(records[2].dstAddr, 0x200 + 32 * 16);
    ASSERT_EQ(records[2].memSize, opInfo.blockSize);
    ASSERT_EQ(records[3].dstAddr, 0x200 + 32 * 24);
    ASSERT_EQ(records[3].memSize, opInfo.blockSize);
    ASSERT_EQ(records[4].dstAddr, 0x200 + 32 * 32);
    ASSERT_EQ(records[4].memSize, opInfo.blockSize);
}

TEST(AddressSanitizer, parse_blockstride_equal_0_repeatStride_equal_0_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 3;
    opInfo.blockStride = 0;
    opInfo.repeatStride = 0;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    AddressSanitizer::ConvertSanEventToMemOpRecords(event, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
}

TEST(AddressSanitizer, parse_blockstride_equal_1_record_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 1;
    opInfo.blockStride = 1;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    AddressSanitizer::ConvertSanEventToMemOpRecords(event, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize * opInfo.blockNum);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
}

TEST(AddressSanitizer, parse_blockstride_equal_1_blockSize_equal_0_record_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 0;
    opInfo.repeatTimes = 1;
    opInfo.blockStride = 1;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    AddressSanitizer::ConvertSanEventToMemOpRecords(event, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, 0);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
}

TEST(AddressSanitizer,
    parse_blockstride_equal_1_repeatStride_greater_than_blockNum_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 5;
    opInfo.blockStride = 1;
    opInfo.repeatStride = 20;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    AddressSanitizer::ConvertSanEventToMemOpRecords(event, records);
    ASSERT_EQ(records.size(), 5);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize * opInfo.blockNum);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
    ASSERT_EQ(records[1].dstAddr, 0x200 + opInfo.repeatStride * opInfo.blockSize);
    ASSERT_EQ(records[1].memSize, opInfo.blockSize * opInfo.blockNum);
    ASSERT_FALSE(records[1].srcAddr == records[1].dstAddr);
    ASSERT_EQ(records[2].dstAddr, 0x200 + opInfo.repeatStride * opInfo.blockSize * 2);
    ASSERT_EQ(records[2].memSize, opInfo.blockSize * opInfo.blockNum);
    ASSERT_FALSE(records[2].srcAddr == records[2].dstAddr);
    ASSERT_EQ(records[3].dstAddr, 0x200 + opInfo.repeatStride * opInfo.blockSize * 3);
    ASSERT_EQ(records[3].memSize, opInfo.blockSize * opInfo.blockNum);
    ASSERT_FALSE(records[3].srcAddr == records[3].dstAddr);
    ASSERT_EQ(records[4].dstAddr, 0x200 + opInfo.repeatStride * opInfo.blockSize * 4);
    ASSERT_EQ(records[4].memSize, opInfo.blockSize * opInfo.blockNum);
    ASSERT_FALSE(records[4].srcAddr == records[4].dstAddr);
}

TEST(AddressSanitizer, parse_blockstride_equal_1_repeatStride_smaller_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 3;
    opInfo.blockStride = 1;
    opInfo.repeatStride = 2;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    AddressSanitizer::ConvertSanEventToMemOpRecords(event, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize *
        (opInfo.blockNum + (opInfo.repeatTimes-1) * opInfo.repeatStride));
}

TEST(AddressSanitizer, parse_blockstride_equal_1_repeatStride_equal_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 3;
    opInfo.blockStride = 1;
    opInfo.repeatStride = opInfo.blockNum;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    AddressSanitizer::ConvertSanEventToMemOpRecords(event, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize * opInfo.blockNum * opInfo.repeatTimes);
}

TEST(AddressSanitizer, parse_blockstride_equal_1_repeatStride_equal_0_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 3;
    opInfo.blockStride = 1;
    opInfo.repeatStride = 0;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    AddressSanitizer::ConvertSanEventToMemOpRecords(event, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize * opInfo.blockNum);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
}

TEST(AddressSanitizer, parse_repeatStride_equal_1_blockStride_equal_0_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 3;
    opInfo.blockStride = 0;
    opInfo.repeatStride = 0;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    AddressSanitizer::ConvertSanEventToMemOpRecords(event, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
}

TEST(AddressSanitizer, parse_repeatStride_equal_0_blockStride_equal_1_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 3;
    opInfo.blockStride = 1;
    opInfo.repeatStride = 0;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    AddressSanitizer::ConvertSanEventToMemOpRecords(event, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize * opInfo.blockNum);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
}

TEST(AddressSanitizer, parse_repeatStride_equal_1_blockStride_equal_1_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 3;
    opInfo.blockStride = 1;
    opInfo.repeatStride = 1;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    AddressSanitizer::ConvertSanEventToMemOpRecords(event, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize * opInfo.blockNum + opInfo.blockSize * 2);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
}

TEST(AddressSanitizer, convert_single_record_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 1;
    opInfo.blockStride = 0;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;

    MemOpRecord record;
    SetBasicMemInfo(record, event);

    ConvertSingleRecordRepeats(event, record, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
}

TEST(AddressSanitizer, convert_single_record_repeatStride_greater_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 5;
    opInfo.blockStride = 0;
    opInfo.repeatStride = 20;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    MemOpRecord record;
    SetBasicMemInfo(record, event);

    ConvertSingleRecordRepeats(event, record, records);
    ASSERT_EQ(records.size(), 5);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
    ASSERT_EQ(records[1].dstAddr, 0x200 + opInfo.repeatStride * opInfo.blockSize);
    ASSERT_EQ(records[1].memSize, opInfo.blockSize);
    ASSERT_FALSE(records[1].srcAddr == records[1].dstAddr);
    ASSERT_EQ(records[2].dstAddr, 0x200 + opInfo.repeatStride * opInfo.blockSize * 2);
    ASSERT_EQ(records[2].memSize, opInfo.blockSize);
    ASSERT_FALSE(records[2].srcAddr == records[2].dstAddr);
    ASSERT_EQ(records[3].dstAddr, 0x200 + opInfo.repeatStride * opInfo.blockSize * 3);
    ASSERT_EQ(records[3].memSize, opInfo.blockSize);
    ASSERT_FALSE(records[3].srcAddr == records[3].dstAddr);
    ASSERT_EQ(records[4].dstAddr, 0x200 + opInfo.repeatStride * opInfo.blockSize * 4);
    ASSERT_EQ(records[4].memSize, opInfo.blockSize);
    ASSERT_FALSE(records[4].srcAddr == records[4].dstAddr);
}

TEST(AddressSanitizer,
    convert_single_record_repeatStride_greater_than_blockStride_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 5;
    opInfo.blockStride = 0;
    opInfo.repeatStride = 2;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    MemOpRecord record;
    SetBasicMemInfo(record, event);

    ConvertSingleRecordRepeats(event, record, records);
    ASSERT_EQ(records.size(), 5);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize);
    ASSERT_EQ(records[1].dstAddr, 0x200 + 32 * 2);
    ASSERT_EQ(records[1].memSize, opInfo.blockSize);
    ASSERT_EQ(records[2].dstAddr, 0x200 + 32 * 4);
    ASSERT_EQ(records[2].memSize, opInfo.blockSize);
    ASSERT_EQ(records[3].dstAddr, 0x200 + 32 * 6);
    ASSERT_EQ(records[3].memSize, opInfo.blockSize);
    ASSERT_EQ(records[4].dstAddr, 0x200 + 32 * 8);
    ASSERT_EQ(records[4].memSize, opInfo.blockSize);
}

TEST(AddressSanitizer, convert_single_record_repeatStride_equal_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 5;
    opInfo.blockStride = 0;
    opInfo.repeatStride = opInfo.blockNum;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    MemOpRecord record;
    SetBasicMemInfo(record, event);

    ConvertSingleRecordRepeats(event, record, records);
    ASSERT_EQ(records.size(), 5);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize);
    ASSERT_EQ(records[1].dstAddr, 0x200 + 32 * 8);
    ASSERT_EQ(records[1].memSize, opInfo.blockSize);
    ASSERT_EQ(records[2].dstAddr, 0x200 + 32 * 16);
    ASSERT_EQ(records[2].memSize, opInfo.blockSize);
    ASSERT_EQ(records[3].dstAddr, 0x200 + 32 * 24);
    ASSERT_EQ(records[3].memSize, opInfo.blockSize);
    ASSERT_EQ(records[4].dstAddr, 0x200 + 32 * 32);
    ASSERT_EQ(records[4].memSize, opInfo.blockSize);
}

TEST(AddressSanitizer, convert_single_record_repeatStride_equal_0_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 3;
    opInfo.blockStride = 0;
    opInfo.repeatStride = 0;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    MemOpRecord record;
    SetBasicMemInfo(record, event);

    ConvertSingleRecordRepeats(event, record, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
}

TEST(AddressSanitizer, convert_single_record_record_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 1;
    opInfo.blockStride = 1;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    MemOpRecord record;
    SetBasicMemInfo(record, event);

    ConvertSingleRecordRepeats(event, record, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize * opInfo.blockNum);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
}

TEST(AddressSanitizer, convert_single_record_blockSize_equal_0_record_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 0;
    opInfo.repeatTimes = 1;
    opInfo.blockStride = 1;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    MemOpRecord record;
    SetBasicMemInfo(record, event);

    ConvertSingleRecordRepeats(event, record, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, 0);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
}

TEST(AddressSanitizer, convert_single_record_s_eq1_repeatStride_greater_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 5;
    opInfo.blockStride = 1;
    opInfo.repeatStride = 20;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    MemOpRecord record;
    SetBasicMemInfo(record, event);

    ConvertSingleRecordRepeats(event, record, records);
    ASSERT_EQ(records.size(), 5);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize * opInfo.blockNum);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
    ASSERT_EQ(records[1].dstAddr, 0x200 + opInfo.repeatStride * opInfo.blockSize);
    ASSERT_EQ(records[1].memSize, opInfo.blockSize * opInfo.blockNum);
    ASSERT_FALSE(records[1].srcAddr == records[1].dstAddr);
    ASSERT_EQ(records[2].dstAddr, 0x200 + opInfo.repeatStride * opInfo.blockSize * 2);
    ASSERT_EQ(records[2].memSize, opInfo.blockSize * opInfo.blockNum);
    ASSERT_FALSE(records[2].srcAddr == records[2].dstAddr);
    ASSERT_EQ(records[3].dstAddr, 0x200 + opInfo.repeatStride * opInfo.blockSize * 3);
    ASSERT_EQ(records[3].memSize, opInfo.blockSize * opInfo.blockNum);
    ASSERT_FALSE(records[3].srcAddr == records[3].dstAddr);
    ASSERT_EQ(records[4].dstAddr, 0x200 + opInfo.repeatStride * opInfo.blockSize * 4);
    ASSERT_EQ(records[4].memSize, opInfo.blockSize * opInfo.blockNum);
    ASSERT_FALSE(records[4].srcAddr == records[4].dstAddr);
}

TEST(AddressSanitizer, convert_single_record_s_eq1_repeatStride_smaller_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 3;
    opInfo.blockStride = 1;
    opInfo.repeatStride = 2;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    MemOpRecord record;
    SetBasicMemInfo(record, event);

    ConvertSingleRecordRepeats(event, record, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize *
        (opInfo.blockNum + (opInfo.repeatTimes-1) * opInfo.repeatStride));
}

TEST(AddressSanitizer, convert_single_record_s_eq1_repeatStride_equal_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 3;
    opInfo.blockStride = 1;
    opInfo.repeatStride = opInfo.blockNum;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    MemOpRecord record;
    SetBasicMemInfo(record, event);

    ConvertSingleRecordRepeats(event, record, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize * opInfo.blockNum * opInfo.repeatTimes);
}

TEST(AddressSanitizer, convert_single_record_s_eq1_repeatStride_equal_0_expect_get_correct_memory_records)
{
    MemOpInfo opInfo{};
    opInfo.addr = 0x200;
    opInfo.blockNum = 8;
    opInfo.blockSize = 32;
    opInfo.repeatTimes = 3;
    opInfo.blockStride = 1;
    opInfo.repeatStride = 0;
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo = opInfo;
    std::vector<MemOpRecord> records;
    MemOpRecord record;
    SetBasicMemInfo(record, event);

    ConvertSingleRecordRepeats(event, record, records);
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].dstAddr, 0x200);
    ASSERT_EQ(records[0].memSize, opInfo.blockSize * opInfo.blockNum);
    ASSERT_FALSE(records[0].srcAddr == records[0].dstAddr);
}

TEST(AddressSanitizer, registered_heap_but_not_registered_regions_with_memcheck_expect_get_error)
{
    Config config {
        .defaultCheck = true,
        .memCheck = true,
    };
    DeviceInfoSummary deviceInfoSummary {};
    auto asan = SanitizerFactory::GetInstance().Create(ToolType::MEMCHECK);
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    ASSERT_TRUE(asan->SetDeviceInfo(deviceInfoSummary, config));
    std::string msg;
    asan->RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    SanitizerRecord record;
    record.version = RecordVersion::MEMORY_RECORD;
    MemOpRecord opRecord{};
    uint64_t addr = RandUint(0, GLOBAL_MEM_MASK/2);
    uint64_t size = RandUint(1, 1024);
    opRecord.type = MemOpType::MALLOC;
    opRecord.dstAddr = addr;
    opRecord.dstSpace = AddressSpace::GM;
    opRecord.memSize = size;
    opRecord.infoSrc = MemInfoSrc::MSTX_HEAP;
    record.payload.memoryRecord = opRecord;
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));

    opRecord.type = MemOpType::LOAD;
    opRecord.infoSrc = MemInfoSrc::BYPASS;
    record.payload.memoryRecord = opRecord;
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("illegal read of size " + std::to_string(size)) != std::string::npos);
}

TEST(AddressSanitizer, registered_heap_and_regions_with_memcheck_expect_get_error)
{
    Config config {
        .defaultCheck = true,
        .memCheck = true,
    };
    DeviceInfoSummary deviceInfoSummary {};
    auto asan = SanitizerFactory::GetInstance().Create(ToolType::MEMCHECK);
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    ASSERT_TRUE(asan->SetDeviceInfo(deviceInfoSummary, config));
    std::string msg;
    asan->RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    SanitizerRecord record;
    record.version = RecordVersion::MEMORY_RECORD;
    MemOpRecord opRecord{};
    uint64_t addr = RandUint(0, GLOBAL_MEM_MASK/2);
    uint64_t size = RandUint(1, 1024 * 1024 * 1024UL);
    opRecord.type = MemOpType::MALLOC;
    opRecord.dstAddr = addr;
    opRecord.dstSpace = AddressSpace::GM;
    opRecord.memSize = size;
    opRecord.infoSrc = MemInfoSrc::RT;
    record.payload.memoryRecord = opRecord;
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));

    opRecord.infoSrc = MemInfoSrc::MSTX_HEAP;
    record.payload.memoryRecord = opRecord;
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));

    opRecord.memSize = size + RandUint(1, 1024 * 1024);
    opRecord.infoSrc = MemInfoSrc::MSTX_REGION;
    opRecord.rootAddr = addr;
    record.payload.memoryRecord = opRecord;
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));

    opRecord.type = MemOpType::LOAD;
    opRecord.infoSrc = MemInfoSrc::BYPASS;
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("illegal read of size") == std::string::npos);

    uint64_t illegalSize = RandUint(1, 1024);
    opRecord.memSize += illegalSize;
    record.payload.memoryRecord = opRecord;
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("illegal read of size " + std::to_string(illegalSize)) != std::string::npos);
}

TEST(AddressSanitizer, registered_heap_then_unregistered_twice_expect_get_error)
{
    Config config {
        .defaultCheck = true,
        .memCheck = true,
    };
    DeviceInfoSummary deviceInfoSummary {};
    auto asan = SanitizerFactory::GetInstance().Create(ToolType::MEMCHECK);
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    ASSERT_TRUE(asan->SetDeviceInfo(deviceInfoSummary, config));
    std::string msg;
    asan->RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    SanitizerRecord record;
    record.version = RecordVersion::MEMORY_RECORD;
    MemOpRecord opRecord{};
    uint64_t addr = 0x22;
    uint64_t size = 100;
    opRecord.type = MemOpType::MALLOC;
    opRecord.dstAddr = addr;
    opRecord.dstSpace = AddressSpace::GM;
    opRecord.memSize = size;
    opRecord.infoSrc = MemInfoSrc::MSTX_HEAP;
    opRecord.rootAddr = addr;
    record.payload.memoryRecord = opRecord;
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));

    opRecord.infoSrc = MemInfoSrc::MSTX_REGION;
    opRecord.memSize = 10;
    opRecord.dstAddr = addr + 10;
    record.payload.memoryRecord = opRecord;
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));

    opRecord.dstAddr += 10;
    record.payload.memoryRecord = opRecord;
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));

    opRecord.type = MemOpType::FREE;
    record.payload.memoryRecord = opRecord;
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("illegal free") == std::string::npos);

    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("illegal free") != std::string::npos);
    msg.clear();

    opRecord.infoSrc = MemInfoSrc::MSTX_HEAP;
    opRecord.dstAddr = addr;
    record.payload.memoryRecord = opRecord;
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("illegal free") != std::string::npos);
}

static ScatterVnchwconvRecord CreateScatterVnchwconvRecordStruct()
{
    constexpr uint32_t repeat = 1;
    constexpr uint32_t srcStride = 1;
    constexpr uint32_t dstStride = 2;
    constexpr uint32_t fileNo = 111;
    constexpr uint32_t lineNo = 222;
    ScatterVnchwconvRecord record{};
    record.dst0.l64 = 0x612;
    record.dst0.h64 = 0x612;
    record.src0.l64 = 0x13;
    record.src0.h64 = 0x13;
    record.dst1.l64 = 0x612;
    record.dst1.h64 = 0x612;
    record.src1.l64 = 0x13;
    record.src1.h64 = 0x13;
    record.repeat = repeat;
 
    record.dstHighHalf = false;
    record.srcHighHalf = false;
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
    record.dataType = DataType::DATA_B16;
    record.srcStride = srcStride;
    record.dstStride = dstStride;
 
    return record;
}
 
TEST(AddressSanitizer, parse_scattervnchwconv_records)
{
    KernelRecord kernelRecord;
    kernelRecord.recordType = RecordType::SCATTERVNCHWCONV;
    kernelRecord.payload.scatterVnchwconvRecord = CreateScatterVnchwconvRecordStruct();
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    sanitizerRecord.payload.kernelRecord = kernelRecord;
 
    std::vector<MemOpRecord> records;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(sanitizerRecord, events);
    ASSERT_EQ(events.size(), 32);
    ConvertSanEventsToMemOpRecords(events, records);
    ASSERT_EQ(records.size(), 32);
    ASSERT_EQ(records[0].type, MemOpType::LOAD);
    ASSERT_EQ(records[2].type, MemOpType::LOAD);
    ASSERT_EQ(records[4].type, MemOpType::LOAD);
    ASSERT_EQ(records[6].type, MemOpType::LOAD);
    ASSERT_EQ(records[16].type, MemOpType::STORE);
    ASSERT_EQ(records[18].type, MemOpType::STORE);
    ASSERT_EQ(records[20].type, MemOpType::STORE);
    ASSERT_EQ(records[22].type, MemOpType::STORE);
}

TEST(AddressSanitizer, malloc_rt_bypass_extra_bypass_memory_expect_get_error)
{
    Config config{};
    config.memCheck = true;
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    auto asan = SanitizerFactory::GetInstance().Create(ToolType::MEMCHECK);
    ASSERT_TRUE(asan->SetDeviceInfo(deviceInfoSummary, config));
    std::string msg;
    asan->RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanitizerRecord record{};
    CreateSanitizeRecord(MemOpType::MALLOC, MemInfoSrc::RT, 0x100, record);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    CreateSanitizeRecord(MemOpType::MALLOC, MemInfoSrc::BYPASS, 0x200, record);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    CreateSanitizeRecord(MemOpType::STORE, MemInfoSrc::BYPASS, 0x200, record);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    CreateSanitizeRecord(MemOpType::LOAD, MemInfoSrc::BYPASS, 0x200, record);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    CreateSanitizeRecord(MemOpType::MALLOC, MemInfoSrc::EXTRA, 0x300, record);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    CreateSanitizeRecord(MemOpType::STORE, MemInfoSrc::EXTRA, 0x300, record);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    CreateSanitizeRecord(MemOpType::MALLOC, MemInfoSrc::BYPASS, 0x500, record);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    CreateSanitizeRecord(MemOpType::STORE, MemInfoSrc::BYPASS, 0x500, record);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_FALSE(msg.find("illegal write") != std::string::npos);
    CreateSanitizeRecord(MemOpType::LOAD, MemInfoSrc::RT, 0x300, record);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("illegal read") != std::string::npos);
    msg.clear();
    CreateSanitizeRecord(MemOpType::LOAD, MemInfoSrc::EXTRA, 0x300, record);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_FALSE(msg.find("illegal read") != std::string::npos);
    CreateSanitizeRecord(MemOpType::LOAD, MemInfoSrc::BYPASS, 0x200, record);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_FALSE(msg.find("illegal read") != std::string::npos);
    CreateSanitizeRecord(MemOpType::FREE, MemInfoSrc::BYPASS, 0x200, record);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_FALSE(msg.find("illegal free") != std::string::npos);
    CreateSanitizeRecord(MemOpType::LOAD, MemInfoSrc::BYPASS, 0x200, record);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("illegal read") != std::string::npos);
    msg.clear();
    CreateSanitizeRecord(MemOpType::LOAD, MemInfoSrc::RT, 0x100, record);
    CreateSanitizeRecord(MemOpType::LOAD, MemInfoSrc::BYPASS, 0x200, record);
    ASSERT_FALSE(asan->CheckRecordBeforeProcess(record));
    ASSERT_TRUE(msg.find("illegal read") != std::string::npos);
}

}
