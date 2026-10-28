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
#include <array>
#include <cstdio>
#include <functional>
#include <mutex>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#define private public
#include "address_sanitizer.h"
#include "checker.h"
#undef private
#include "race_sanitizer.h"
#include "config.h"
#include "platform_config.h"
#include "securec.h"
#include "utility/log.h"

using namespace Sanitizer;

TEST(Checker, set_invalid_device_type_expect_run_checker_success)
{
    Config config {
        .defaultCheck = true,
        .memCheck = true,
    };
    Checker checker(config, false);
    DeviceInfoSummary deviceInfo = {};
    deviceInfo.device = DeviceType::INVALID;
    checker.SetDeviceInfo(deviceInfo);
}

TEST(Checker, set_loglv_and_path_expect_run_checker_success)
{
    Config config {
        .defaultCheck = true,
        .memCheck = true,
    };
    std::vector<DetectionInfo> detectionInfo;
    Checker checker(config, false);
    char const* logFile = "/tmp/test.log";
    checker.SetDetectionInfo(LogLv::INFO, std::cout);
    remove(logFile);
    checker.Finish();
}

SanitizerRecord CreateKernelSanRecord()
{
    SanitizerRecord sanRecord;
    sanRecord.version = RecordVersion::KERNEL_RECORD;
    BinaryOpRecord record {
        .dst = 0x23b00,
        .src0 = 0x1fc00,
        .src1 = 0x2b900,
        .location = {1, 0, 21},
        .vectorMask = {0xfc0, 0x0},
        .dstBlockStride = 1,
        .src0BlockStride = 1,
        .src1BlockStride = 1,
        .dstRepeatStride = 8,
        .src0RepeatStride = 8,
        .src1RepeatStride = 1,
        .dstBlockSize = 32,
        .src0BlockSize = 32,
        .src1BlockSize = 8,
        .repeat = 1,
        .dstBlockNum = 8,
        .src0BlockNum = 8,
        .src1BlockNum = 1,
        .dstDataBits = 32,
        .src0DataBits = 32,
        .src1DataBits = 32,
        .maskMode = MaskMode::MASK_COUNT,
        .instrName = InstrName::NONE
    };
    sanRecord.payload.kernelRecord.recordType = RecordType::BINARY_OP;
    sanRecord.payload.kernelRecord.blockType = BlockType::AIVEC;
    sanRecord.payload.kernelRecord.payload.binaryOpRecord = record;
    return sanRecord;
}

int GetDetectInfoNum(const std::string &str, const std::string &pattern = "serialNo") {
    size_t count = 0;
    size_t pos = 0;
    while ((pos = str.find(pattern, pos)) != std::string::npos) {
        ++count;
        pos += pattern.length(); // 避免重叠匹配
    }
    return count;
}

void SetCheckerContext(Checker &checker, DeviceType device, KernelType kernelType, uint32_t deviceId = 0) {
    DeviceInfoSummary deviceInfo{};
    deviceInfo.device = device;
    deviceInfo.deviceId = deviceId;
    checker.SetDeviceInfo(deviceInfo);
    KernelSummary kernelInfo{};
    kernelInfo.kernelType = kernelType;
    kernelInfo.blockDim = 1;
    checker.SetKernelInfo(kernelInfo);
}

std::shared_ptr<Checker> MakeRealChecker(Config config, std::stringstream &ss, KernelType kernelType) {
    auto checker = std::make_shared<Checker>(config, false);
    checker->SetDetectionInfo(LogLv::WARN, ss);
    SetCheckerContext(*checker, DeviceType::ASCEND_910B1, kernelType);
    return checker;
}

std::shared_ptr<Checker> GetNewChecker(std::stringstream &ss)
{
    Config config{};
    config.defaultCheck = true;
    config.memCheck = true;
    config.raceCheck = true;

    auto checker = std::make_shared<Checker>(config, false);
    // 重置 notify func，防止因生命周期导致 core dump
    checker->SetDetectionInfo(LogLv::WARN, ss);
    DeviceInfoSummary deviceInfo = {};
    deviceInfo.device = DeviceType::ASCEND_910B1;
    checker->SetDeviceInfo(deviceInfo);
    KernelSummary kernelInfo = {};
    kernelInfo.blockDim = 1;
    kernelInfo.pcStartAddr = 0x1000;
    kernelInfo.kernelType = KernelType::AIVEC;
    checker->SetKernelInfo(kernelInfo);
    return checker;
}

class RecordingSanitizer final : public SanitizerBase {
public:
    void DoEvents(const SanitizerRecord &record, EventRange events) override {
        rangePointers.push_back(events.begin());
        std::vector<SanEvent> copy;
        if (!events.empty()) {
            copy.assign(events.begin(), events.end());
        }
        Do(record, copy);
    }
    bool SetDeviceInfo(DeviceInfoSummary const &, Config const &) override { return true; }
    bool SetKernelInfo(KernelSummary const &) override { return true; }
    void ParseOnlineError(const KernelErrorRecord &, BlockType, uint64_t) override {}
    void RegisterNotifyFunc(const MSG_FUNC &) override {}
    void Exit() override {}

    void Do(const SanitizerRecord &record, const std::vector<SanEvent> &events) override {
        records.push_back(record);
        consumedEvents.emplace_back(events);
    }

    bool CheckRecordBeforeProcess(const SanitizerRecord &) override {
        if (beforeRecord) {
            beforeRecord();
        }
        ++checkRecordCount;
        return allowProcess;
    }

    std::vector<SanitizerRecord> records;
    std::vector<std::vector<SanEvent>> consumedEvents;
    size_t checkRecordCount = 0;
    bool allowProcess = true;
    std::vector<const SanEvent *> rangePointers;
    std::function<void()> beforeRecord;
};

std::array<std::shared_ptr<RecordingSanitizer>, 3> MakeRecordingSanitizers() {
    return {std::make_shared<RecordingSanitizer>(), std::make_shared<RecordingSanitizer>(),
        std::make_shared<RecordingSanitizer>()};
}

SanitizerRecord MakeScalarRecord(
    RecordType recordType, BlockType blockType, AddressSpace space, uint64_t serialNo, uint64_t addr) {
    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = recordType;
    record.payload.kernelRecord.blockType = blockType;
    record.payload.kernelRecord.serialNo = serialNo;
    auto &loadStoreRecord = record.payload.kernelRecord.payload.loadStoreRecord;
    loadStoreRecord.addr = addr;
    loadStoreRecord.size = 1;
    loadStoreRecord.space = space;
    loadStoreRecord.alignSize = 1;
    return record;
}

SanitizerRecord MakeScalarRecord(RecordType recordType, BlockType blockType, AddressSpace space, uint64_t serialNo) {
    return MakeScalarRecord(recordType, blockType, space, serialNo, 0x1000 + serialNo);
}

SanitizerRecord MakeMemoryRecord() {
    SanitizerRecord record{};
    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord.type = MemOpType::MALLOC;
    record.payload.memoryRecord.dstAddr = 0x1000;
    record.payload.memoryRecord.dstSpace = AddressSpace::GM;
    record.payload.memoryRecord.memSize = 64;
    record.payload.memoryRecord.infoSrc = MemInfoSrc::RT;
    return record;
}

SanitizerRecord MakeKernelFinishRecord() {
    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = RecordType::KERNEL_FINISH;
    return record;
}

TEST(Checker, batch_parse_preserves_empty_and_multiple_event_boundaries) {
    Config config{};
    Checker checker(config, false);
    ParseTask task{};
    task.ctx.deviceInfo.device = DeviceType::ASCEND_910B1;
    task.ctx.kernelSummary.kernelType = KernelType::AIVEC;
    task.ctx.rootTid = RuntimeContext::Instance().rootTid_;
    task.records = {MakeMemoryRecord(),
        MakeScalarRecord(RecordType::STORE, BlockType::AIVEC, AddressSpace::UB, 17),
        CreateKernelSanRecord(), MakeKernelFinishRecord()};

    auto parsed = checker.ParseRecords(std::move(task));

    ASSERT_EQ(parsed.records.size(), 4U);
    ASSERT_EQ(parsed.eventOffsets.size(), parsed.records.size() + 1);
    EXPECT_EQ(parsed.eventOffsets[0], 0U);
    EXPECT_EQ(parsed.eventOffsets[1], 0U);
    EXPECT_EQ(parsed.eventOffsets[2], 1U);
    EXPECT_GT(parsed.eventOffsets[3] - parsed.eventOffsets[2], 1U);
    EXPECT_EQ(parsed.eventOffsets[4] - parsed.eventOffsets[3], 1U);
    EXPECT_EQ(parsed.eventOffsets.back(), parsed.events.size());
    EXPECT_EQ(parsed.events[0].serialNo, 17U);
    EXPECT_EQ(parsed.events.back().type, EventType::SANITIZER_CONTROL_EVENT);
}

TEST(Checker, batch_handoff_keeps_records_without_events_for_every_consumer) {
    auto [memChecker, raceChecker, syncChecker] = MakeRecordingSanitizers();
    Config config{.defaultCheck = true, .memCheck = true, .raceCheck = true, .syncCheck = true};
    Checker checker(config, false);
    const std::array<std::shared_ptr<RecordingSanitizer>, 3> consumers{memChecker, raceChecker, syncChecker};
    for (size_t i = 0; i < consumers.size(); ++i) {
        checker.sanitizerArr_[i] = consumers[i];
    }
    SetCheckerContext(checker, DeviceType::ASCEND_910B1, KernelType::AIVEC);
    checker.Do(std::vector<SanitizerRecord>{MakeMemoryRecord(),
        MakeScalarRecord(RecordType::STORE, BlockType::AIVEC, AddressSpace::UB, 23), MakeKernelFinishRecord()});

    for (const auto &consumer : consumers) {
        ASSERT_EQ(consumer->records.size(), 3U);
        ASSERT_EQ(consumer->consumedEvents.size(), 3U);
        EXPECT_EQ(consumer->checkRecordCount, 3U);
        EXPECT_TRUE(consumer->consumedEvents[0].empty());
        ASSERT_EQ(consumer->consumedEvents[1].size(), 1U);
        EXPECT_EQ(consumer->consumedEvents[1][0].serialNo, 23U);
        ASSERT_EQ(consumer->consumedEvents[2].size(), 1U);
        EXPECT_EQ(consumer->consumedEvents[2][0].type, EventType::SANITIZER_CONTROL_EVENT);
    }
}

TEST(Checker, batch_with_only_empty_events_reaches_all_consumers) {
    auto [memChecker, raceChecker, syncChecker] = MakeRecordingSanitizers();
    Config config{.defaultCheck = true, .memCheck = true, .raceCheck = true, .syncCheck = true};
    Checker checker(config, false);
    const std::array<std::shared_ptr<RecordingSanitizer>, 3> consumers{memChecker, raceChecker, syncChecker};
    for (size_t i = 0; i < consumers.size(); ++i) {
        checker.sanitizerArr_[i] = consumers[i];
    }
    SetCheckerContext(checker, DeviceType::ASCEND_910B1, KernelType::AIVEC);
    checker.Do(std::vector<SanitizerRecord>{MakeMemoryRecord(), MakeMemoryRecord()});
    checker.Finish();

    for (const auto &consumer : consumers) {
        ASSERT_EQ(consumer->records.size(), 2U);
        ASSERT_EQ(consumer->consumedEvents.size(), 2U);
        EXPECT_TRUE(consumer->consumedEvents[0].empty());
        EXPECT_TRUE(consumer->consumedEvents[1].empty());
        EXPECT_EQ(consumer->checkRecordCount, 2U);
    }
    ASSERT_EQ(memChecker->rangePointers.size(), 2U);
    EXPECT_EQ(memChecker->rangePointers[0], nullptr);
    EXPECT_EQ(memChecker->rangePointers[1], nullptr);
}

SanitizerRecord MakeMstxTensorRecord() {
    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = RecordType::MSTX_STUB;
    record.payload.kernelRecord.blockType = BlockType::AIVEC;
    auto &mstxRecord = record.payload.kernelRecord.payload.mstxRecord;
    mstxRecord.interfaceType = InterfaceType::MSTX_VEC_UNARY_OP;
    mstxRecord.interface.mstxVecUnaryDesc.src.space = AddressSpace::UB;
    mstxRecord.interface.mstxVecUnaryDesc.src.addr = 0;
    mstxRecord.interface.mstxVecUnaryDesc.src.size = 1;
    mstxRecord.interface.mstxVecUnaryDesc.src.dataBits = 8;
    mstxRecord.interface.mstxVecUnaryDesc.dst = mstxRecord.interface.mstxVecUnaryDesc.src;
    mstxRecord.interface.mstxVecUnaryDesc.wrapper.maskMode = MaskMode::MASK_NORM;
    mstxRecord.interface.mstxVecUnaryDesc.wrapper.mask = {~0ULL, ~0ULL};
    mstxRecord.interface.mstxVecUnaryDesc.blockNum = 1;
    mstxRecord.interface.mstxVecUnaryDesc.dstBlockStride = 1;
    mstxRecord.interface.mstxVecUnaryDesc.srcBlockStride = 1;
    mstxRecord.interface.mstxVecUnaryDesc.repeatTimes = 1;
    mstxRecord.interface.mstxVecUnaryDesc.dstRepeatStride = 1;
    mstxRecord.interface.mstxVecUnaryDesc.srcRepeatStride = 1;
    return record;
}

SanitizerRecord MakeUnaryRaceRecord(uint64_t serialNo, uint64_t addr) {
    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    auto &kernelRecord = record.payload.kernelRecord;
    kernelRecord.recordType = RecordType::UNARY_OP;
    kernelRecord.blockType = BlockType::AIVEC;
    kernelRecord.serialNo = serialNo;
    auto &unaryOpRecord = kernelRecord.payload.unaryOpRecord;
    unaryOpRecord.location.blockId = 0;
    unaryOpRecord.vectorMask = {~0ULL, ~0ULL};
    unaryOpRecord.maskMode = MaskMode::MASK_NORM;
    unaryOpRecord.dst = addr;
    unaryOpRecord.dstBlockNum = 1;
    unaryOpRecord.dstBlockSize = 32;
    unaryOpRecord.dstBlockStride = 1;
    unaryOpRecord.dstRepeatStride = 1;
    unaryOpRecord.dstDataBits = 8;
    unaryOpRecord.repeat = 1;
    return record;
}

std::function<void()> MakeFinishProduceBarrier(const std::weak_ptr<Checker> &checker) {
    return [checker]() {
        while (auto current = checker.lock()) {
            if (current->finishProduce_) {
                return;
            }
            std::this_thread::yield();
        }
    };
}

std::shared_ptr<Checker> MakeRecordingChecker(const std::shared_ptr<RecordingSanitizer> &memChecker,
    const std::shared_ptr<RecordingSanitizer> &raceChecker, const std::shared_ptr<RecordingSanitizer> &syncChecker,
    const std::shared_ptr<RecordingSanitizer> &registerChecker = nullptr) {
    Config config{.defaultCheck = true, .memCheck = true, .raceCheck = true, .syncCheck = true};
    config.registerCheck = registerChecker != nullptr;
    auto checker = std::make_shared<Checker>(config, false);
    checker->sanitizerArr_[static_cast<size_t>(ToolType::MEMCHECK)] = memChecker;
    checker->sanitizerArr_[static_cast<size_t>(ToolType::RACECHECK)] = raceChecker;
    checker->sanitizerArr_[static_cast<size_t>(ToolType::SYNCCHECK)] = syncChecker;
    if (registerChecker != nullptr) {
        checker->sanitizerArr_[static_cast<size_t>(ToolType::REGISTERCHECK)] = registerChecker;
    }

    const auto beforeRecord = MakeFinishProduceBarrier(checker);
    for (const auto &consumer : {memChecker, raceChecker, syncChecker}) {
        consumer->beforeRecord = beforeRecord;
    }
    if (registerChecker != nullptr) {
        registerChecker->beforeRecord = beforeRecord;
    }
    SetCheckerContext(*checker, DeviceType::ASCEND_910B1, KernelType::AICUBE, 3);
    return checker;
}

TEST(Checker, filters_only_mem_and_race_consumers_on_single_record) {
    auto [memChecker, raceChecker, syncChecker] = MakeRecordingSanitizers();
    auto checker = MakeRecordingChecker(memChecker, raceChecker, syncChecker);

    checker->Do(MakeScalarRecord(RecordType::STORE, BlockType::AICUBE, AddressSpace::UB, 1));
    checker->Do(MakeKernelFinishRecord());

    for (const auto &consumer : {memChecker, raceChecker, syncChecker}) {
        ASSERT_EQ(consumer->consumedEvents.size(), 2U);
    }
    for (const auto &consumer : {memChecker, raceChecker}) {
        EXPECT_TRUE(consumer->consumedEvents[0].empty());
        ASSERT_EQ(consumer->consumedEvents[1].size(), 1U);
        EXPECT_EQ(consumer->consumedEvents[1][0].type, EventType::SANITIZER_CONTROL_EVENT);
    }
    ASSERT_EQ(syncChecker->consumedEvents[0].size(), 1U);
    EXPECT_EQ(syncChecker->consumedEvents[0][0].type, EventType::MEM_EVENT);
    ASSERT_EQ(syncChecker->consumedEvents[1].size(), 1U);
    EXPECT_EQ(syncChecker->consumedEvents[1][0].type, EventType::SANITIZER_CONTROL_EVENT);
    EXPECT_EQ(syncChecker->checkRecordCount, 2U);
}

TEST(Checker, filters_private_events_in_batch_and_preserves_order_for_other_consumers) {
    auto [memChecker, raceChecker, syncChecker] = MakeRecordingSanitizers();
    auto checker = MakeRecordingChecker(memChecker, raceChecker, syncChecker);

    checker->Do(std::vector<SanitizerRecord>{
        MakeScalarRecord(RecordType::STORE, BlockType::AIVEC, AddressSpace::UB, 1),
        MakeScalarRecord(RecordType::STORE, BlockType::AICUBE, AddressSpace::UB, 2),
        MakeKernelFinishRecord(),
    });

    for (const auto &consumer : {memChecker, raceChecker, syncChecker}) {
        ASSERT_EQ(consumer->consumedEvents.size(), 3U);
    }
    for (const auto &consumer : {memChecker, raceChecker}) {
        ASSERT_EQ(consumer->consumedEvents[0].size(), 1U);
        EXPECT_EQ(consumer->consumedEvents[0][0].serialNo, 1U);
        EXPECT_TRUE(consumer->consumedEvents[1].empty());
    }
    ASSERT_EQ(syncChecker->consumedEvents[0].size(), 1U);
    ASSERT_EQ(syncChecker->consumedEvents[1].size(), 1U);
    EXPECT_EQ(syncChecker->consumedEvents[0][0].serialNo, 1U);
    EXPECT_EQ(syncChecker->consumedEvents[1][0].serialNo, 2U);
    for (const auto &consumer : {memChecker, raceChecker, syncChecker}) {
        ASSERT_EQ(consumer->consumedEvents[2].size(), 1U);
        EXPECT_EQ(consumer->checkRecordCount, 3U);
    }
}

TEST(Checker, event_range_borrows_work_buffer_and_preserves_empty_records_and_boundaries) {
    auto [memChecker, raceChecker, syncChecker] = MakeRecordingSanitizers();
    auto checker = MakeRecordingChecker(memChecker, raceChecker, syncChecker);
    memChecker->beforeRecord = {};
    WorkArgs args{};
    args.records = {MakeMemoryRecord(),
        MakeScalarRecord(RecordType::STORE, BlockType::AIVEC, AddressSpace::UB, 17),
        MakeKernelFinishRecord()};
    args.events.resize(3);
    args.eventOffsets = {0, 0, 2, 3};
    auto &first = args.events[0];
    first.type = EventType::MEM_EVENT;
    first.loc.blockType = BlockType::AIVEC;
    first.pipe = PipeType::PIPE_S_CAL;
    first.eventInfo.memInfo.memType = MemType::UB;
    first.eventInfo.memInfo.opType = AccessType::WRITE;
    first.serialNo = 17;
    args.events[1] = first;
    args.events[1].serialNo = 18;
    args.events[2].type = EventType::SANITIZER_CONTROL_EVENT;
    uint64_t removed = 0;

    for (const auto &[tool, consumer] :
        std::array<std::pair<ToolType, std::shared_ptr<RecordingSanitizer>>, 3>{{
            {ToolType::MEMCHECK, memChecker}, {ToolType::RACECHECK, raceChecker},
            {ToolType::SYNCCHECK, syncChecker}}}) {
        consumer->beforeRecord = {};
        checker->ConsumeKernelBlock(static_cast<uint8_t>(tool), args, removed);

        ASSERT_EQ(consumer->consumedEvents.size(), 3U);
        EXPECT_TRUE(consumer->consumedEvents[0].empty());
        ASSERT_EQ(consumer->consumedEvents[1].size(), 2U);
        EXPECT_EQ(consumer->consumedEvents[1][0].serialNo, 17U);
        EXPECT_EQ(consumer->consumedEvents[1][1].serialNo, 18U);
        ASSERT_EQ(consumer->rangePointers.size(), 3U);
        EXPECT_EQ(consumer->rangePointers[0], args.events.data());
        EXPECT_EQ(consumer->rangePointers[1], args.events.data());
        EXPECT_EQ(consumer->rangePointers[2], args.events.data() + 2);
        EXPECT_EQ(consumer->checkRecordCount, 3U);
    }
}

TEST(Checker, address_consumer_filters_before_init_pipeline_replay) {
    Config config{.defaultCheck = true, .memCheck = true, .raceCheck = false, .initCheck = true};
    Checker checker(config, false);

    SetCheckerContext(checker, DeviceType::ASCEND_910B1, KernelType::AICUBE);

    checker.Do(MakeScalarRecord(RecordType::STORE, BlockType::AICUBE, AddressSpace::UB, 1));
    checker.Do(MakeScalarRecord(RecordType::STORE, BlockType::AIVEC, AddressSpace::UB, 2));
    checker.Do(MakeKernelFinishRecord());

    auto addressSanitizer =
        std::dynamic_pointer_cast<AddressSanitizer>(checker.sanitizerArr_[static_cast<size_t>(ToolType::MEMCHECK)]);
    ASSERT_NE(addressSanitizer, nullptr);
    ASSERT_EQ(addressSanitizer->replayedRecords_.size(), 1U);
    EXPECT_EQ(addressSanitizer->replayedRecords_[0].serialNo, 2U);
    EXPECT_EQ(addressSanitizer->replayedEventCount_, 1U);
}

TEST(Checker, init_replay_merges_in_callback_order_and_clears_between_kernels) {
    Config config{.defaultCheck = true, .memCheck = true, .raceCheck = false, .initCheck = true};
    Checker checker(config, false);
    SetCheckerContext(checker, DeviceType::ASCEND_910B1, KernelType::AIVEC);
    auto asan = std::dynamic_pointer_cast<AddressSanitizer>(
        checker.sanitizerArr_[static_cast<size_t>(ToolType::MEMCHECK)]);
    ASSERT_NE(asan, nullptr);

    checker.Do(MakeScalarRecord(RecordType::STORE, BlockType::AIVEC, AddressSpace::UB, 11, 0x1000));
    checker.Do(MakeScalarRecord(RecordType::STORE, BlockType::AIVEC, AddressSpace::UB, 12, 0x1001));
    checker.Do(MakeKernelFinishRecord());
    ASSERT_EQ(asan->replayedRecords_.size(), 1U);
    EXPECT_EQ(asan->replayedRecords_[0].dstAddr, 0x1000U);
    EXPECT_EQ(asan->replayedRecords_[0].memSize, 2U);
    EXPECT_EQ(asan->replayedRecords_[0].serialNo, 11U);
    EXPECT_EQ(asan->replayedEventCount_, 2U);

    KernelSummary next{};
    next.kernelType = KernelType::AIVEC;
    next.blockDim = 1;
    checker.SetKernelInfo(next);
    EXPECT_TRUE(asan->replayedRecords_.empty());
    EXPECT_EQ(asan->replayedEventCount_, 0U);
    checker.Do(MakeScalarRecord(RecordType::STORE, BlockType::AIVEC, AddressSpace::UB, 21, 0x1002));
    checker.Do(MakeKernelFinishRecord());
    ASSERT_EQ(asan->replayedRecords_.size(), 1U);
    EXPECT_EQ(asan->replayedRecords_[0].memSize, 1U);
    EXPECT_EQ(asan->replayedRecords_[0].serialNo, 21U);
    EXPECT_EQ(asan->replayedEventCount_, 1U);
}

TEST(Checker, keeps_mstx_tensor_events_for_original_consumer_processing) {
    auto [memChecker, raceChecker, syncChecker] = MakeRecordingSanitizers();
    auto checker = MakeRecordingChecker(memChecker, raceChecker, syncChecker);

    checker->Do(std::vector<SanitizerRecord>{MakeMstxTensorRecord(), MakeKernelFinishRecord()});

    for (const auto &consumer : {memChecker, raceChecker}) {
        ASSERT_EQ(consumer->consumedEvents.size(), 2U);
        ASSERT_EQ(consumer->consumedEvents[0].size(), 2U);
        EXPECT_EQ(consumer->consumedEvents[0][0].type, EventType::MEM_EVENT);
        EXPECT_EQ(consumer->consumedEvents[0][1].type, EventType::MEM_EVENT);
    }
}

TEST(Checker, clears_pathway_count_when_finish_precheck_rejects_record) {
    auto [memChecker, raceChecker, syncChecker] = MakeRecordingSanitizers();
    auto checker = MakeRecordingChecker(memChecker, raceChecker, syncChecker);
    memChecker->allowProcess = false;
    memChecker->beforeRecord = {};

    std::vector<SanEvent> events;
    uint64_t removedPathwayEventCount = 3;
    checker->ConsumeRecord(
        static_cast<uint8_t>(ToolType::MEMCHECK), MakeKernelFinishRecord(), events, removedPathwayEventCount);

    EXPECT_EQ(removedPathwayEventCount, 0U);
    EXPECT_TRUE(memChecker->records.empty());
}

TEST(Checker, memory_record_reaches_consumer_without_pathway_filter) {
    auto [memChecker, raceChecker, syncChecker] = MakeRecordingSanitizers();
    auto checker = MakeRecordingChecker(memChecker, raceChecker, syncChecker);
    memChecker->beforeRecord = {};

    SanEvent retainedEvent{};
    retainedEvent.serialNo = 17;
    retainedEvent.type = EventType::MEM_EVENT;
    retainedEvent.pipe = PipeType::PIPE_S_CAL;
    retainedEvent.eventInfo.memInfo.memType = MemType::UB;
    retainedEvent.eventInfo.memInfo.opType = AccessType::WRITE;
    retainedEvent.timeInfo = {1, 2, 3};
    std::vector<SanEvent> events{retainedEvent};
    constexpr uint64_t removedPathwayEventCount = 9;
    uint64_t removed = removedPathwayEventCount;

    checker->ConsumeRecord(static_cast<uint8_t>(ToolType::MEMCHECK), MakeMemoryRecord(), events, removed);

    EXPECT_EQ(removed, removedPathwayEventCount);
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].serialNo, retainedEvent.serialNo);
    EXPECT_EQ(events[0].type, retainedEvent.type);
    EXPECT_EQ(events[0].pipe, retainedEvent.pipe);
    EXPECT_EQ(events[0].eventInfo.memInfo.memType, retainedEvent.eventInfo.memInfo.memType);
    EXPECT_EQ(events[0].eventInfo.memInfo.opType, retainedEvent.eventInfo.memInfo.opType);
    EXPECT_EQ(events[0].timeInfo, retainedEvent.timeInfo);
    ASSERT_EQ(memChecker->consumedEvents.size(), 1U);
    ASSERT_EQ(memChecker->consumedEvents[0].size(), 1U);
    EXPECT_EQ(memChecker->consumedEvents[0][0].serialNo, retainedEvent.serialNo);
}

TEST(Checker, address_consumer_filters_real_memcheck_path_without_suppressing_other_events) {
    std::stringstream ss;
    auto checker = MakeRealChecker(Config{.defaultCheck = true, .memCheck = true}, ss, KernelType::AICUBE);

    // Both stores target unregistered UB. The AICUBE event is filtered by the
    // pathway rule; the AIVEC event must still reach AddressSanitizer.
    checker->Do(MakeScalarRecord(RecordType::STORE, BlockType::AICUBE, AddressSpace::UB, 1, 0x100000));
    checker->Do(MakeScalarRecord(RecordType::STORE, BlockType::AIVEC, AddressSpace::UB, 2, 0x200000));
    checker->Do(MakeKernelFinishRecord());

    const std::string message = ss.str();
    EXPECT_EQ(message.find("serialNo:1"), std::string::npos);
    EXPECT_NE(message.find("serialNo:2"), std::string::npos);
    ASSERT_NE(message.find("illegal write of size 1"), std::string::npos);
    EXPECT_EQ(message.find("illegal write of size 1"), message.rfind("illegal write of size 1"));
}

TEST(Checker, address_consumer_reports_real_mstx_tensor_bounds_error) {
    std::stringstream ss;
    auto checker = MakeRealChecker(Config{.defaultCheck = true, .memCheck = true}, ss, KernelType::AIVEC);

    // The descriptor advertises one byte while the parsed vector operation is
    // one 32-byte block, so the real local-tensor bounds path must report both
    // the source read and destination write.
    checker->Do(std::vector<SanitizerRecord>{MakeMstxTensorRecord(), MakeKernelFinishRecord()});

    const std::string message = ss.str();
    EXPECT_NE(message.find("illegal read of size 31"), std::string::npos);
    EXPECT_NE(message.find("illegal write of size 31"), std::string::npos);
}

TEST(Checker, race_consumer_keeps_real_unfiltered_aivec_race) {
    std::stringstream ss;
    auto checker = MakeRealChecker(Config{.raceCheck = true}, ss, KernelType::AIVEC);

    checker->Do(MakeUnaryRaceRecord(1, 0x1000));
    checker->Do(MakeUnaryRaceRecord(2, 0x1000));
    checker->Do(MakeKernelFinishRecord());

    EXPECT_NE(ss.str().find("Potential WAW hazard detected"), std::string::npos) << ss.str();
}

TEST(Checker, race_consumer_filters_real_target_pathway_before_race_algorithm) {
    std::stringstream ss;
    auto checker = MakeRealChecker(Config{.raceCheck = true}, ss, KernelType::AICUBE);

    checker->Do(MakeScalarRecord(RecordType::STORE, BlockType::AICUBE, AddressSpace::UB, 1, 0x1000));
    checker->Do(MakeScalarRecord(RecordType::STORE, BlockType::AICUBE, AddressSpace::UB, 2, 0x1000));
    checker->Do(MakeKernelFinishRecord());

    EXPECT_EQ(ss.str().find("Potential"), std::string::npos);
}

TEST(Checker, register_consumer_receives_original_pathway_input) {
    auto [memChecker, raceChecker, syncChecker] = MakeRecordingSanitizers();
    auto registerChecker = std::make_shared<RecordingSanitizer>();
    auto checker = MakeRecordingChecker(memChecker, raceChecker, syncChecker, registerChecker);

    checker->Do(std::vector<SanitizerRecord>{
        MakeScalarRecord(RecordType::STORE, BlockType::AICUBE, AddressSpace::UB, 1), MakeKernelFinishRecord()});

    for (const auto &consumer : {memChecker, raceChecker}) {
        ASSERT_EQ(consumer->consumedEvents.size(), 2U);
        EXPECT_TRUE(consumer->consumedEvents[0].empty());
    }
    ASSERT_EQ(registerChecker->consumedEvents.size(), 2U);
    ASSERT_EQ(registerChecker->consumedEvents[0].size(), 1U);
    EXPECT_EQ(registerChecker->consumedEvents[0][0].type, EventType::MEM_EVENT);
    EXPECT_EQ(registerChecker->consumedEvents[0][0].serialNo, 1U);
}

std::string ReadLogFile(FILE *file) {
    if (file == nullptr || std::fflush(file) != 0 || std::fseek(file, 0, SEEK_END) != 0) {
        return {};
    }
    const long size = std::ftell(file);
    if (size <= 0 || std::fseek(file, 0, SEEK_SET) != 0) {
        return {};
    }
    std::string contents(static_cast<size_t>(size), '\0');
    const size_t readSize = std::fread(&contents[0], 1, contents.size(), file);
    contents.resize(readSize);
    return contents;
}

TEST(Checker, pathway_summary_is_independent_per_consumer_and_cleared_per_kernel) {
    Log *log = Log::GetLog();
    FILE *capture = std::tmpfile();
    ASSERT_NE(capture, nullptr);
    FILE *oldFile = log->fp_;
    const std::string oldKernelName = RuntimeContext::Instance().kernelNameDisplay;
    log->fp_ = capture;

    RuntimeContext::Instance().kernelNameDisplay = "pathway-filter-first";
    {
        auto [memChecker, raceChecker, syncChecker] = MakeRecordingSanitizers();
        auto checker = MakeRecordingChecker(memChecker, raceChecker, syncChecker);

        // Give the two consumers different precheck outcomes so a shared
        // counter would be observable in the summary lines. The first
        // kernel has two matching events for AddressSanitizer and none for
        // RaceSanitizer.
        raceChecker->allowProcess = false;
        checker->Do(std::vector<SanitizerRecord>{
            MakeScalarRecord(RecordType::STORE, BlockType::AICUBE, AddressSpace::UB, 1),
            MakeScalarRecord(RecordType::STORE, BlockType::AICUBE, AddressSpace::UB, 2),
            MakeKernelFinishRecord(),
        });

        RuntimeContext::Instance().kernelNameDisplay = "pathway-filter-second";
        KernelSummary kernelInfo{};
        kernelInfo.kernelType = KernelType::AICUBE;
        kernelInfo.blockDim = 1;
        checker->SetKernelInfo(kernelInfo);
        // No pathway event is sent for the second kernel: its finish must
        // still emit a zero summary exactly once for each consumer.
        checker->Do(MakeKernelFinishRecord());
    }

    const std::string logs = ReadLogFile(capture);
    log->fp_ = oldFile;
    std::fclose(capture);
    RuntimeContext::Instance().kernelNameDisplay = oldKernelName;

    for (const auto *consumer : {"AddressSanitizer", "RaceSanitizer"}) {
        EXPECT_EQ(GetDetectInfoNum(logs, "consumer:" + std::string(consumer)), 2U);
    }
    const struct {
        const char *consumer;
        const char *kernel;
        uint64_t removedEvents;
    } expected[] = {
        {"AddressSanitizer", "pathway-filter-first", 2},
        {"RaceSanitizer", "pathway-filter-first", 0},
        {"AddressSanitizer", "pathway-filter-second", 0},
        {"RaceSanitizer", "pathway-filter-second", 0},
    };
    for (const auto &summary : expected) {
        const std::string prefix =
            "consumer:" + std::string(summary.consumer) + ", kernel:" + summary.kernel + ", deviceId:3";
        EXPECT_EQ(GetDetectInfoNum(logs, prefix), 1U);
        EXPECT_NE(logs.find(prefix + ", deviceType:"), std::string::npos);
        EXPECT_NE(logs.find(prefix + ", deviceType:2, removed events:" + std::to_string(summary.removedEvents)),
            std::string::npos);
    }
}

TEST(Checker, set_multi_detect_then_fake_record_run_single_data_expect_normal) {
    std::stringstream ss;
    std::shared_ptr<Checker> checker = GetNewChecker(ss);
    SanitizerRecord kernelSanRecord = CreateKernelSanRecord();
    checker->Do(kernelSanRecord);
    checker->Finish();
    size_t matchCnt = GetDetectInfoNum(ss.str());
    ASSERT_EQ(matchCnt, 0);
    if (auto func = std::dynamic_pointer_cast<AddressSanitizer>(
        checker->sanitizerArr_[static_cast<size_t>(ToolType::MEMCHECK)])) {
        ASSERT_EQ(func->errorBuffer_.GetBuffer().size(), 0);
    }
}

TEST(Checker, set_multi_detect_then_fake_many_records_and_run_parallel_expect_normal)
{
    std::stringstream ss;
    std::shared_ptr<Checker> checker = GetNewChecker(ss);

    SanitizerRecord kernelSanRecord = CreateKernelSanRecord();
    constexpr int repeatTimes = 10000;
    for (int i = 0; i < repeatTimes; i++) {
        checker->Do(kernelSanRecord);
    }
    SanitizerRecord lastSanRecord = CreateKernelSanRecord();
    lastSanRecord.payload.kernelRecord.recordType = RecordType::KERNEL_FINISH;
    checker->Do(lastSanRecord);
    size_t matchCnt = GetDetectInfoNum(ss.str());
    ASSERT_EQ(matchCnt, 0);
    checker->Finish();
    if (auto func = std::dynamic_pointer_cast<AddressSanitizer>(
        checker->sanitizerArr_[static_cast<size_t>(ToolType::MEMCHECK)])) {
        ASSERT_EQ(func->errorBuffer_.GetBuffer().size(), 0);
    }
}

TEST(Checker, set_multi_detect_then_no_finish_record_and_run_parallel_expect_private_queue_empty)
{
    std::stringstream ss;
    std::shared_ptr<Checker> checker = GetNewChecker(ss);
    SanitizerRecord kernelSanRecord = CreateKernelSanRecord();

    constexpr int repeatTimes = 1000;
    for (int i = 0; i < repeatTimes; i++) {
        checker->Do(kernelSanRecord);
    }
    checker->Finish();
    for (uint8_t i = 0; i < TOOL_NUM; ++i) {
        if (checker->sanitizerArr_[i] != nullptr) {
            ASSERT_TRUE(checker->workerArgs_[i].empty());
        }
    }
}

namespace {
Sanitizer::UnaryOpRecord GenUnaryOpRecord()
{
    Sanitizer::UnaryOpRecord unaryOpRecord = {
        .dst = 0x102,
        .src = 0x204,
        .location = {1, 0, 21},
        .vectorMask = {0xffffffffffffffff, 0xffffffffffffffff},
        .dstBlockSize = 4,
        .srcBlockSize = 4,
        .dstBlockStride = 1,
        .srcBlockStride = 1,
        .dstRepeatStride = 8,
        .srcRepeatStride = 8,
        .repeat = 1,
        .dstBlockNum = 8,
        .srcBlockNum = 8,
        .dstDataBits = 16,
        .srcDataBits = 32,
        .maskMode = MaskMode::MASK_NORM,
        .instrName = InstrName::NONE
    };
    return unaryOpRecord;
}
}  // namespace

TEST(Checker, test_element_alignment_for_310p_expect_no_misalignment)
{
    std::stringstream ss;

    Config config{};
    config.defaultCheck = true;
    config.memCheck = true;
    config.raceCheck = false;

    auto checker = std::make_shared<Checker>(config, false);
    checker->SetDetectionInfo(LogLv::WARN, ss);
    DeviceInfoSummary deviceInfo = {};
    deviceInfo.device = DeviceType::ASCEND_310P;
    checker->SetDeviceInfo(deviceInfo);
    SanitizerRecord record;
    record.version = Sanitizer::RecordVersion::KERNEL_RECORD;
    auto &kernelRecord = record.payload.kernelRecord;
    kernelRecord.recordType = Sanitizer::RecordType::UNARY_OP;
    kernelRecord.blockType = Sanitizer::BlockType::AICORE;
    kernelRecord.payload.unaryOpRecord = std::move(GenUnaryOpRecord());
    checker->Do(record);
    kernelRecord.payload.unaryOpRecord.dstDataBits = 8;
    kernelRecord.payload.unaryOpRecord.dst = 0x101;
    kernelRecord.payload.unaryOpRecord.maskMode = MaskMode::MASK_COUNT;
    kernelRecord.payload.unaryOpRecord.vectorMask = {0x10, 0x0};
    checker->Do(record);
    kernelRecord.recordType = RecordType::BLOCK_FINISH;
    checker->Do(record);
    kernelRecord.recordType = RecordType::KERNEL_FINISH;
    checker->Do(record);
    checker->Finish();
    ASSERT_EQ(ss.str().find("misalign"), std::string::npos);
}

TEST(Checker, test_element_alignment_for_910b_expect_misalignment)
{
    std::stringstream ss;

    Config config{};
    config.defaultCheck = true;
    config.memCheck = true;
    config.raceCheck = false;

    auto checker = std::make_shared<Checker>(config, false);
    checker->SetDetectionInfo(LogLv::WARN, ss);
    DeviceInfoSummary deviceInfo = {};
    deviceInfo.device = DeviceType::ASCEND_910B1;
    checker->SetDeviceInfo(deviceInfo);
    SanitizerRecord record;
    record.version = Sanitizer::RecordVersion::KERNEL_RECORD;
    auto &kernelRecord = record.payload.kernelRecord;
    kernelRecord.recordType = Sanitizer::RecordType::UNARY_OP;
    kernelRecord.blockType = Sanitizer::BlockType::AICORE;
    kernelRecord.payload.unaryOpRecord = std::move(GenUnaryOpRecord());
    checker->Do(record);
    kernelRecord.recordType = RecordType::BLOCK_FINISH;
    checker->Do(record);
    kernelRecord.recordType = RecordType::KERNEL_FINISH;
    checker->Do(record);
    checker->Finish();
    ASSERT_NE(ss.str().find("misalign"), std::string::npos);
}

TEST(Checker, test_online_mem_check_expect_got_error)
{
    std::stringstream ss;

    Config config{};
    config.defaultCheck = true;
    config.memCheck = true;
    config.raceCheck = false;

    auto checker = std::make_shared<Checker>(config, false);
    checker->SetDetectionInfo(LogLv::WARN, ss);
    DeviceInfoSummary deviceInfo = {};
    deviceInfo.device = DeviceType::ASCEND_910B1;
    checker->SetDeviceInfo(deviceInfo);
    SanitizerRecord record;
    record.version = Sanitizer::RecordVersion::KERNEL_RECORD;
    auto &kernelRecord = record.payload.kernelRecord;
    kernelRecord.recordType = Sanitizer::RecordType::ONLINE_ERROR;
    kernelRecord.blockType = Sanitizer::BlockType::AICORE;
    Sanitizer::KernelErrorRecord errorRecord{};
    errorRecord.errorNum = 1;
    errorRecord.recordType = RecordType::SIMT_STG;
    errorRecord.recordSize = sizeof(SimtLoadStoreRecord);
    KernelErrorDesc errorDesc{};
    errorDesc.errorType = KernelErrorType::ILLEGAL_ADDR_WRITE;
    errorDesc.location.pc = 0x12;
    errorDesc.location.fileNo = 12;
    errorDesc.location.lineNo = 15;
    errorDesc.payload.illegalDesc.illegalSize = 10;
    errorDesc.payload.illegalDesc.addr = 0x100;
    errorDesc.space = Sanitizer::AddressSpace::GM;
    errorRecord.kernelErrorDesc = &errorDesc;
    SimtLoadStoreRecord simtRecord{};
    errorRecord.record = reinterpret_cast<void *>(&simtRecord);

    kernelRecord.payload.kernelErrorRecord = errorRecord;
    checker->Do(record);
    checker->Finish();
    if (auto func = std::dynamic_pointer_cast<AddressSanitizer>(
        checker->sanitizerArr_[static_cast<size_t>(ToolType::MEMCHECK)])) {
        ASSERT_EQ(func->errorBuffer_.GetBuffer().size(), 1);
        auto errorMsg = func->errorBuffer_.GetBuffer()[0];
        ASSERT_EQ(func->errorBuffer_.GetBuffer().size(), 1);
    }
}

TEST(Checker, error_buffer_skip_zero_size_error_at_choke_point) {
    // 兜底：ErrorBuffer::Add 是所有 error 报告的唯一汇聚点，这里统一过滤 nBadBytes==0 的
    // 损坏/未初始化 error（覆盖 onlineError 之外的 DoMemOpRecord/align/bounds 等所有路径），
    // 防止 "illegal write of size 0" 之类的无意义误报。
    std::stringstream ss;
    std::shared_ptr<Checker> checker = GetNewChecker(ss);
    if (auto func = std::dynamic_pointer_cast<AddressSanitizer>(
            checker->sanitizerArr_[static_cast<size_t>(ToolType::MEMCHECK)])) {
        // 1) 损坏/未初始化 error（nBadBytes==0）应被跳过
        ErrorMsg zeroError{};
        zeroError.auxData.blockType = BlockType::AIVEC;
        zeroError.auxData.coreId = 0;
        func->errorBuffer_.Add(zeroError);
        ASSERT_TRUE(func->errorBuffer_.GetBuffer().empty());

        // 2) 有效 error（nBadBytes>0）正常入桶并归并 aiv
        ErrorMsg validError{};
        validError.isError = true;
        validError.type = MemErrorType::ILLEGAL_ADDR_WRITE;
        validError.auxData.blockType = BlockType::AIVEC;
        validError.auxData.coreId = 0;
        validError.auxData.nBadBytes = 4;
        func->errorBuffer_.Add(validError);
        ASSERT_EQ(func->errorBuffer_.GetBuffer().size(), 1);
    }
}

TEST(Checker, error_buffer_keep_illegal_free_error) {
    // ILLEGAL_FREE 无字节大小语义（nBadBytes 保持构造默认值 0），
    // 不能被 nBadBytes==0 的兜底过滤误丢，否则回放/reduce 下 illegal free/double free 会漏报。
    std::stringstream ss;
    std::shared_ptr<Checker> checker = GetNewChecker(ss);
    if (auto func = std::dynamic_pointer_cast<AddressSanitizer>(
            checker->sanitizerArr_[static_cast<size_t>(ToolType::MEMCHECK)])) {
        ErrorMsg illegalFree{};
        illegalFree.isError = true;
        illegalFree.type = MemErrorType::ILLEGAL_FREE;
        illegalFree.auxData.blockType = BlockType::AIVEC;
        illegalFree.auxData.coreId = 0;
        illegalFree.auxData.badAddr.addr = 0x61;
        illegalFree.auxData.space = AddressSpace::GM;
        func->errorBuffer_.Add(illegalFree);
        ASSERT_EQ(func->errorBuffer_.GetBuffer().size(), 1);
    }
}

namespace {
// 构造一条 MEMORY_RECORD，并将其 serialNo 置为 KERNEL_FINISH 的枚举值。
// 由于 SanitizerRecord.payload 是 union，KernelRecord.recordType（offset 0, uint32_t）
// 与 MemOpRecord.serialNo（offset 0, uint64_t）的低 32 位共享同一块内存。
// 若不先校验 record.version 就读取 kernelRecord.recordType，会读到 KERNEL_FINISH，
// 从而误触发 kernel 结束分支（重置 errorCounts_、打印 "Sanitizer finished"）。
SanitizerRecord CreateMemoryRecordMimickingKernelFinish() {
    SanitizerRecord record{};
    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord.serialNo = static_cast<uint64_t>(RecordType::KERNEL_FINISH);
    record.payload.memoryRecord.type = MemOpType::MALLOC;
    record.payload.memoryRecord.dstAddr = 0x1000;
    record.payload.memoryRecord.dstSpace = AddressSpace::GM;
    record.payload.memoryRecord.memSize = 0x100;
    return record;
}
} // namespace

TEST(Checker, non_kernel_record_should_not_trigger_kernel_finish_branch) {
    // 单条：非 KERNEL_RECORD 版本的记录即使其 union 内存恰好等于 KERNEL_FINISH，
    // 也不应进入 kernel 结束分支。
    std::stringstream ss;
    std::shared_ptr<Checker> checker = GetNewChecker(ss);

    constexpr uint32_t seedErrorCount = 7;
    checker->errorCounts_[static_cast<size_t>(ToolType::MEMCHECK)] = seedErrorCount;

    SanitizerRecord record = CreateMemoryRecordMimickingKernelFinish();
    checker->Do(record);

    // 结束分支会通过 swap 将 errorCounts_ 清零；未进入则保持 seedErrorCount。
    ASSERT_EQ(checker->errorCounts_[static_cast<size_t>(ToolType::MEMCHECK)], seedErrorCount);
    // DisplaySanitizerEnd 仅在结束分支调用，不应出现 "Sanitizer finished"。
    ASSERT_EQ(ss.str().find("Sanitizer finished"), std::string::npos);

    checker->Finish();
}

TEST(Checker, non_kernel_record_in_batch_should_not_trigger_kernel_finish_branch) {
    // 批量：以非 KERNEL_RECORD 记录作为 back() 时，同样不应误判为 kernel 结束。
    std::stringstream ss;
    std::shared_ptr<Checker> checker = GetNewChecker(ss);

    constexpr uint32_t seedErrorCount = 7;
    checker->errorCounts_[static_cast<size_t>(ToolType::MEMCHECK)] = seedErrorCount;

    std::vector<SanitizerRecord> records{CreateMemoryRecordMimickingKernelFinish()};
    checker->Do(std::move(records));

    ASSERT_EQ(checker->errorCounts_[static_cast<size_t>(ToolType::MEMCHECK)], seedErrorCount);
    ASSERT_EQ(ss.str().find("Sanitizer finished"), std::string::npos);

    checker->Finish();
}

TEST(Checker, kernel_finish_record_still_resets_error_counts) {
    // KERNEL_RECORD + KERNEL_FINISH 仍应正常进入结束分支并清零 errorCounts_。
    std::stringstream ss;
    std::shared_ptr<Checker> checker = GetNewChecker(ss);

    constexpr uint32_t seedErrorCount = 7;
    checker->errorCounts_[static_cast<size_t>(ToolType::MEMCHECK)] = seedErrorCount;

    SanitizerRecord record = CreateKernelSanRecord();
    record.payload.kernelRecord.recordType = RecordType::KERNEL_FINISH;
    checker->Do(record);

    ASSERT_EQ(checker->errorCounts_[static_cast<size_t>(ToolType::MEMCHECK)], 0U);
    ASSERT_NE(ss.str().find("Sanitizer finished"), std::string::npos);

    checker->Finish();
}
