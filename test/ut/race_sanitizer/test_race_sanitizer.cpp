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
#include "core/framework/config_manager.h"
#include "core/framework/event_def.h"
#include "record_pre_process.h"
#include "alg_framework/race_alg_impl.h"
#include "alg_framework/single_pipe_race_alg_impl.h"
#include "rand_data.h"
#include "sanitizer_report.h"
#include "race_info_display.h"
#define private public
#include "race_sanitizer.h"
#undef private

namespace {
using namespace Sanitizer;

auto g_fillUnaryOpRecord = [](SanitizerRecord& record, uint64_t addr, uint64_t blockNum,
    uint64_t blockSize, int64_t blockStride, uint64_t mask0, MaskMode maskMode) {
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.blockType = BlockType::AIVEC;
    record.payload.kernelRecord.recordType = RecordType::UNARY_OP;
    auto& unaryOpRecord = record.payload.kernelRecord.payload.unaryOpRecord;
    unaryOpRecord.location.blockId = 0;
    unaryOpRecord.repeat = 1;
    unaryOpRecord.dst = addr;
    unaryOpRecord.dstBlockNum = blockNum;
    unaryOpRecord.srcBlockNum = 0;
    unaryOpRecord.dstBlockSize = blockSize;
    unaryOpRecord.srcBlockSize = 0;
    unaryOpRecord.dstBlockStride = blockStride;
    unaryOpRecord.srcBlockStride = 1;
    unaryOpRecord.dstRepeatStride = 1;
    unaryOpRecord.srcRepeatStride = 1;
    unaryOpRecord.dstDataBits = 8;
    unaryOpRecord.srcDataBits = 8;
    unaryOpRecord.vectorMask.mask0 = mask0;
    unaryOpRecord.maskMode = maskMode;
    unaryOpRecord.instrName = InstrName::NONE;
};

TEST(RaceSanitizer, race_sanitizer_detect_cntmask_overlapping_mem_race_events_expect_success)
{
    // 无mask时，两条UnaryOpRecord的内存示意图
    // |____|         |____|         |____|
    //                |_________|         |_________|
    SinglePipeRaceAlgImpl alg(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    std::vector<SanEvent> events;

    // mask = 33时，两条UnaryOpRecord的内存示意图
    // |____|         |_|
    //                |_________|         |_________|
    SanitizerRecord record;
    constexpr uint64_t unitMemLen = 32;
    constexpr uint64_t addr = 0x010000;
    g_fillUnaryOpRecord(record, addr, 3U, unitMemLen, 3U, 33, MaskMode::MASK_COUNT);
    record.payload.kernelRecord.serialNo = 1;
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillUnaryOpRecord(record, addr + 3 * unitMemLen, 2U, unitMemLen * 2, 2U, 15, MaskMode::MASK_NORM);
    record.payload.kernelRecord.serialNo = 2;
    RecordPreProcess::GetInstance().Process(record, events);
    for (const auto &event : events) {
        alg.Do(event);
    }
    SanEvent event;
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 1U);

    // mask = 32时，两条UnaryOpRecord的内存示意图
    // |____|
    //                |_________|         |_________|
    SinglePipeRaceAlgImpl alg1(KernelType::AIVEC, DeviceType::ASCEND_910B1, 2);
    events.clear();
    g_fillUnaryOpRecord(record, addr, 3U, unitMemLen, 3U, 32, MaskMode::MASK_COUNT);
    record.payload.kernelRecord.serialNo = 1;
    RecordPreProcess::GetInstance().Process(record, events);
    g_fillUnaryOpRecord(record, addr + 3 * unitMemLen, 2U, unitMemLen * 2, 2U, 15, MaskMode::MASK_NORM);
    record.payload.kernelRecord.serialNo = 2;
    RecordPreProcess::GetInstance().Process(record, events);
    for (const auto &event : events) {
        alg1.Do(event);
    }
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg1.Do(event);
    alg1.WaitFinished();
    ASSERT_EQ(alg1.GetRaceCount(), 0U);
}

TEST(RaceSanitizer, race_sanitizer_detect_race_events_mask_count_expect_success)
{
    // 无mask时，两条UnaryOpRecord的内存示意图
    //                     |____|____|____|
    // |____|              |____|
    RaceAlgImpl alg(KernelType::AICUBE, DeviceType::ASCEND_910B1, 2);
    std::vector<SanEvent> events;
    SanEvent event;
    uint8_t unitSize = 16U;
    event.type = EventType::MEM_EVENT;
    event.pipe = PipeType::PIPE_M;
    event.eventInfo.memInfo = {MemType::UB, AccessType::WRITE, Sanitizer::VectorMask{0, 0},
        Sanitizer::MaskMode::MASK_NORM, InstrName::NONE, 8,
        static_cast<uint32_t>(0x50) + unitSize * 4, 3U, unitSize, 1U, 1U, 1U, 32};
    events.emplace_back(event);

    // mask = 16时，两条UnaryOpRecord的内存示意图
    //                     |____|____|____|
    // |____|
    SanitizerRecord record;
    g_fillUnaryOpRecord(record, 0x50, 2U, unitSize, 4U, 16, MaskMode::MASK_COUNT);
    record.payload.kernelRecord.serialNo = 1;
    RecordPreProcess::GetInstance().Process(record, events);
    for (const auto &event : events) {
        alg.Do(event);
    }
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg.Do(event);
    alg.WaitFinished();
    ASSERT_EQ(alg.GetRaceCount(), 0U);
    ASSERT_EQ(alg.IsFinished(), true);

    // mask = 17时，两条UnaryOpRecord的内存示意图
    //                     |____|____|____|
    // |____|              |_|
    RaceAlgImpl alg1(KernelType::AICUBE, DeviceType::ASCEND_910B1, 2);
    g_fillUnaryOpRecord(record, 0x50, 2U, unitSize, 4U, 17, MaskMode::MASK_COUNT);
    record.payload.kernelRecord.serialNo = 2;
    RecordPreProcess::GetInstance().Process(record, events);
    for (const auto &event : events) {
        alg1.Do(event);
    }
    event.type = EventType::SANITIZER_CONTROL_EVENT;
    event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
    alg1.Do(event);
    alg1.WaitFinished();
    ASSERT_EQ(alg1.GetRaceCount(), 1U);
    ASSERT_EQ(alg1.IsFinished(), true);
}

auto g_createBinaryOpRecord = [](BinaryOpRecord& binaryOpRecord) {
    binaryOpRecord.location.blockId = 0;
    binaryOpRecord.repeat = 1;
    binaryOpRecord.dst = 0x200;
    binaryOpRecord.src0 = 0x100;
    binaryOpRecord.src1 = 0x300;
    binaryOpRecord.src0BlockNum = 1;
    binaryOpRecord.src1BlockNum = 1;
    binaryOpRecord.dstBlockNum = 1;
    binaryOpRecord.src0BlockSize = 128;
    binaryOpRecord.src1BlockSize = 128;
    binaryOpRecord.dstBlockSize = 64;
    binaryOpRecord.src0DataBits = 8;
    binaryOpRecord.src1DataBits = 8;
    binaryOpRecord.dstDataBits = 8;
    binaryOpRecord.instrName = InstrName::NONE;
};

TEST(RaceSanitizer, race_sanitizer_can_detect_aiv_pipe_race_events_expect_success)
{
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    KernelSummary kernelSummary {};
    kernelSummary.blockDim = 2;
    kernelSummary.kernelType = KernelType::AIVEC;
    RaceSanitizer alg;
    alg.SetDeviceInfo(deviceInfoSummary, Config{});
    alg.SetKernelInfo(kernelSummary);
    std::string msg;
    alg.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanitizerRecord record{};
    record.version = RecordVersion::MEMORY_RECORD;
    ASSERT_FALSE(alg.CheckRecordBeforeProcess(record));
    record.version = RecordVersion::KERNEL_RECORD;
    auto &kernelRecord = record.payload.kernelRecord;
    kernelRecord.recordType = RecordType::UNARY_OP;
    kernelRecord.blockType = BlockType::AIVEC;
    UnaryOpRecord unaryOpRecord{};
    unaryOpRecord.location.blockId = 0;
    unaryOpRecord.repeat = 1;
    unaryOpRecord.dst = 0x100;
    unaryOpRecord.src = 0x200;
    unaryOpRecord.srcBlockNum = 1;
    unaryOpRecord.dstBlockNum = 1;
    unaryOpRecord.srcBlockSize = 32;
    unaryOpRecord.dstBlockSize = 32;
    unaryOpRecord.srcDataBits = 16;
    unaryOpRecord.dstDataBits = 16;
    kernelRecord.payload.unaryOpRecord = unaryOpRecord;
    kernelRecord.serialNo++;
    ASSERT_TRUE(alg.CheckRecordBeforeProcess(record));
    std::vector<SanEvent> events = {};
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 2);
    BinaryOpRecord binaryOpRecord{};
    g_createBinaryOpRecord(binaryOpRecord);
    kernelRecord.recordType = RecordType::BINARY_OP;
    kernelRecord.payload.binaryOpRecord = binaryOpRecord;
    kernelRecord.serialNo++;
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 5);
    kernelRecord.recordType = RecordType::KERNEL_FINISH;
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 6);
    alg.Do(record, events);
    alg.raceAlgs_[2]->WaitFinished();
    ASSERT_EQ((*alg.raceAlgs_[2]->GetResult()).size(), 2);
}

TEST(RaceSanitizer, race_sanitizer_with_default_check_and_ffts_set_wait_can_detect_target_event)
{
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    KernelSummary kernelSummary {};
    for (size_t i = 0; i < 20; ++i) {
        kernelSummary.blockDim = RandUint(1, 40);
        kernelSummary.kernelType = static_cast<KernelType>(RandUint(static_cast<uint64_t>(KernelType::AIVEC),
                            static_cast<uint64_t>(KernelType::MIX)));
        RaceSanitizer alg;
        alg.SetDeviceInfo(deviceInfoSummary, Config{});
        alg.SetKernelInfo(kernelSummary);
        SanEvent event{};
        event.type = EventType::MEM_EVENT;
        BlockType blockType = kernelSummary.kernelType == KernelType::AICUBE ? BlockType::AICUBE : BlockType::AIVEC;
        event.loc.blockType = blockType;
        event.loc.coreId = 0;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        event.type = EventType::SYNC_EVENT;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        event.loc.coreId = RandUint(1, 40);
        event.type = EventType::MEM_EVENT;
        ASSERT_FALSE(alg.IsTargetEvent(event, blockType));
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        event.type = EventType::SYNC_EVENT;
        ASSERT_FALSE(alg.IsTargetEvent(event, blockType));
    }
}

TEST(RaceSanitizer, race_sanitizer_with_default_check_and_ffts_set_or_wait_can_detect_target_event)
{
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    KernelSummary kernelSummary {};
    for (size_t i = 0; i < 20; ++i) {
        kernelSummary.blockDim = RandUint(1, 40);
        kernelSummary.kernelType = static_cast<KernelType>(RandUint(static_cast<uint64_t>(KernelType::AIVEC),
                            static_cast<uint64_t>(KernelType::MIX)));
        RaceSanitizer alg;
        alg.SetDeviceInfo(deviceInfoSummary, Config{});
        alg.SetKernelInfo(kernelSummary);
        SanEvent event{};
        event.type = EventType::MEM_EVENT;
        BlockType blockType = kernelSummary.kernelType == KernelType::AICUBE ? BlockType::AICUBE : BlockType::AIVEC;
        event.loc.blockType = blockType;
        event.loc.coreId = 0;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        if (RandUint(0, 1) == 0U) {
            event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
            ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        }
        if (RandUint(0, 1) == 0U) {
            event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
            ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        } else {
            event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
            ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        }
        event.type = EventType::SYNC_EVENT;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        event.loc.blockType = BlockType::AICORE;
        ASSERT_TRUE(alg.IsTargetEvent(event, BlockType::AICORE));
        event.loc.coreId = RandUint(1, 80);
        event.type = EventType::MEM_EVENT;
        ASSERT_FALSE(alg.IsTargetEvent(event, blockType));
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        event.type = EventType::SYNC_EVENT;
        ASSERT_FALSE(alg.IsTargetEvent(event, blockType));
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
    }
}

TEST(RaceSanitizer, race_sanitizer_with_single_block_check_cant_detect_target_event)
{
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    KernelSummary kernelSummary {};
    for (size_t i = 0; i < 20; ++i) {
        kernelSummary.blockDim = RandUint(1, 40);
        kernelSummary.kernelType = static_cast<KernelType>(RandUint(static_cast<uint64_t>(KernelType::AIVEC),
                            static_cast<uint64_t>(KernelType::MIX)));
        RaceSanitizer alg;
        Config config{};
        auto pariCoreId = RandTwoDiffUint(0, 80);
        config.checkBlockId = pariCoreId.first;
        alg.SetDeviceInfo(deviceInfoSummary, config);
        alg.SetKernelInfo(kernelSummary);
        SanEvent event{};
        event.type = EventType::MEM_EVENT;
        BlockType blockType = kernelSummary.kernelType == KernelType::AICUBE ? BlockType::AICUBE : BlockType::AIVEC;
        event.loc.blockType = blockType;
        event.loc.coreId = config.checkBlockId;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        event.type = EventType::SYNC_EVENT;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        event.loc.blockType = event.loc.blockType == BlockType::AICUBE ? BlockType::AIVEC : BlockType::AICUBE;
        event.type = EventType::MEM_EVENT;
        ASSERT_FALSE(alg.IsTargetEvent(event, blockType));
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        event.eventInfo.fftsSyncInfo.opType = SyncType::FFTS_SYNC;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        event.eventInfo.fftsSyncInfo.opType = SyncType::WAIT_FLAG_DEV;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        event.type = EventType::SYNC_EVENT;
        ASSERT_FALSE(alg.IsTargetEvent(event, blockType));
        event.loc.blockType = BlockType::AICORE;
        ASSERT_TRUE(alg.IsTargetEvent(event, BlockType::AICORE));
        event.loc.coreId = pariCoreId.second;
        event.type = EventType::MEM_EVENT;
        ASSERT_FALSE(alg.IsTargetEvent(event, blockType));
        event.type = EventType::CROSS_CORE_SYNC_EVENT;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
        event.type = EventType::SYNC_EVENT;
        ASSERT_FALSE(alg.IsTargetEvent(event, blockType));
        event.type = EventType::SANITIZER_CONTROL_EVENT;
        event.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        ASSERT_TRUE(alg.IsTargetEvent(event, blockType));
    }
}

TEST(RaceSanitizer, dcci_window_owns_events_after_borrowed_input_is_destroyed) {
    auto &config = ConfigManager::Instance().Get();
    const bool originalCheckDcci = config.checkDcci;
    config.checkDcci = true;
    RaceSanitizer sanitizer;
    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = RecordType::STORE;
    {
        std::vector<SanEvent> events(1);
        events[0].serialNo = 42;
        sanitizer.DoEvents(record, EventRange(events));
        events[0].serialNo = 99;
    }
    sanitizer.DoEvents(record, EventRange(nullptr, 0));
    config.checkDcci = originalCheckDcci;

    ASSERT_EQ(sanitizer.recordWindow_.size(), 2U);
    const auto &savedEvents = sanitizer.recordWindow_.front().second;
    ASSERT_EQ(savedEvents.size(), 1U);
    EXPECT_EQ(savedEvents[0].serialNo, 42U);
    EXPECT_TRUE(sanitizer.recordWindow_.back().second.empty());
}

TEST(RaceSanitizer, race_sanitizer_detect_load_store_missing_dcci_expect_get_miss_dcci_error) {
    ConfigManager::Instance().Get().checkDcci = true;

    DeviceInfoSummary deviceInfoSummary{};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    KernelSummary kernelSummary{};
    kernelSummary.blockDim = 2;
    kernelSummary.kernelType = KernelType::AIVEC;
    RaceSanitizer alg;
    alg.SetDeviceInfo(deviceInfoSummary, Config{});
    alg.SetKernelInfo(kernelSummary);
    std::string msg;
    alg.RegisterNotifyFunc([&msg](LogLv const &, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });
    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    auto &kernelRecord = record.payload.kernelRecord;

    kernelRecord.recordType = RecordType::LOAD;
    kernelRecord.blockType = BlockType::AIVEC;
    kernelRecord.payload.loadStoreRecord.location.blockId = 0;
    kernelRecord.payload.loadStoreRecord.addr = 0x1234;
    kernelRecord.payload.loadStoreRecord.size = 4;
    kernelRecord.payload.loadStoreRecord.space = AddressSpace::GM;
    kernelRecord.serialNo++;
    std::vector<SanEvent> events = {};
    RecordPreProcess::GetInstance().Process(record, events);
    alg.Do(record, events);
    events.clear();

    kernelRecord.recordType = RecordType::BLOCK_FINISH;
    kernelRecord.serialNo++;
    RecordPreProcess::GetInstance().Process(record, events);
    alg.Do(record, events);
    events.clear();

    kernelRecord.recordType = RecordType::STORE;
    kernelRecord.blockType = BlockType::AIVEC;
    kernelRecord.payload.loadStoreRecord.location.blockId = 1;
    kernelRecord.payload.loadStoreRecord.addr = 0x1234;
    kernelRecord.payload.loadStoreRecord.size = 4;
    kernelRecord.payload.loadStoreRecord.space = AddressSpace::GM;
    kernelRecord.serialNo++;
    RecordPreProcess::GetInstance().Process(record, events);
    alg.Do(record, events);
    events.clear();

    kernelRecord.recordType = RecordType::BLOCK_FINISH;
    kernelRecord.serialNo++;
    RecordPreProcess::GetInstance().Process(record, events);
    alg.Do(record, events);
    events.clear();

    kernelRecord.recordType = RecordType::KERNEL_FINISH;
    kernelRecord.serialNo++;
    RecordPreProcess::GetInstance().Process(record, events);
    alg.Do(record, events);
    events.clear();

    alg.raceAlgs_[4]->WaitFinished();
    std::vector<RaceDispInfo> const &results = *alg.raceAlgs_[4]->GetResult();
    ASSERT_EQ(results.size(), 1U);
    // miss dcci report
    ASSERT_TRUE(results[0].isMissDcci);
    ASSERT_EQ(results[0].p1.addr, 0x1234);
    ASSERT_EQ(results[0].p2.addr, 0x1234);

    ConfigManager::Instance().Get().checkDcci = false;
}

// 覆盖 SetKernelInfo：AICPU 类型 / blockDim 超限 → false；正常 → true
TEST(RaceSanitizer, SetKernelInfo_AllPaths_expect_correct)
{
    auto setup = []() {
        RaceSanitizer alg;
        DeviceInfoSummary deviceInfoSummary{};
        deviceInfoSummary.device = DeviceType::ASCEND_910B1;
        alg.SetDeviceInfo(deviceInfoSummary, Config{});
        return alg;
    };
    {
        auto alg = setup();
        KernelSummary ks{};
        ks.kernelType = KernelType::AICPU;
        ks.blockDim = 1;
        EXPECT_FALSE(alg.SetKernelInfo(ks));
    }
    {
        auto alg = setup();
        KernelSummary ks{};
        ks.kernelType = KernelType::AIVEC;
        ks.blockDim = MAX_BLOCKDIM_NUMS;
        EXPECT_FALSE(alg.SetKernelInfo(ks));
    }
    {
        auto alg = setup();
        KernelSummary ks{};
        ks.kernelType = KernelType::AIVEC;
        ks.blockDim = 1;
        EXPECT_TRUE(alg.SetKernelInfo(ks));
    }
}

// 覆盖 IsTargetEvent：MSTX_SET_CROSS / MSTX_WAIT_CROSS → true；其他 opType → false
TEST(RaceSanitizer, IsTargetEvent_AllPaths_expect_correct)
{
    RaceSanitizer alg;
    DeviceInfoSummary deviceInfoSummary{};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    alg.SetDeviceInfo(deviceInfoSummary, Config{});
    KernelSummary ks{};
    ks.kernelType = KernelType::AIVEC;
    ks.blockDim = 1;
    alg.SetKernelInfo(ks);

    SanEvent event{};
    event.type = EventType::MSTX_CROSS_SYNC_EVENT;
    event.loc.coreId = 99;
    event.loc.blockType = BlockType::AIVEC;

    event.eventInfo.mstxCrossInfo.opType = SyncType::MSTX_SET_CROSS;
    EXPECT_TRUE(alg.IsTargetEvent(event, BlockType::AIVEC));

    event.eventInfo.mstxCrossInfo.opType = SyncType::MSTX_WAIT_CROSS;
    EXPECT_TRUE(alg.IsTargetEvent(event, BlockType::AIVEC));

    event.eventInfo.mstxCrossInfo.opType = SyncType::SET_FLAG;
    EXPECT_FALSE(alg.IsTargetEvent(event, BlockType::AIVEC));
}

namespace {
RaceSanitizer MakeAlg()
{
    RaceSanitizer alg;
    DeviceInfoSummary info{};
    info.device = DeviceType::ASCEND_910B1;
    alg.SetDeviceInfo(info, Config{});
    return alg;
}
}  // namespace

TEST(RaceSanitizer, ParseOnlineError_AllRaceTypes_expect_success)
{
    RaceSanitizer alg = MakeAlg();
    KernelSummary ks{};
    ks.kernelType = KernelType::AIVEC;
    ks.blockDim = 1;
    alg.SetKernelInfo(ks);
    auto mk = [](KernelErrorType t) {
        KernelErrorDesc d{};
        d.errorType = t; d.space = AddressSpace::GM;
        d.location = {1, 2, 3};
        d.payload.raceDesc.addr = 0x1000;
        d.payload.raceDesc.conflictedLocation = {4, 5, 6};
        return d;
    };
    KernelErrorDesc a[] = { mk(KernelErrorType::THREAD_RW_RACE),
                            mk(KernelErrorType::THREAD_WR_RACE),
                            mk(KernelErrorType::THREAD_WW_RACE) };
    KernelErrorRecord r{}; r.errorNum = 3; r.kernelErrorDesc = a;
    alg.ParseOnlineError(r, BlockType::AIVEC, 1);
    EXPECT_EQ(alg.simtErrors_->size(), 3u);
    KernelErrorDesc b[] = { mk(KernelErrorType::MAX) };
    KernelErrorRecord r2{}; r2.errorNum = 1; r2.kernelErrorDesc = b;
    alg.ParseOnlineError(r2, BlockType::AIVEC, 2);
    EXPECT_EQ(alg.simtErrors_->size(), 3u);
}

TEST(RaceSanitizer, MergeSimtErrors_expect_dedup_and_null_check)
{
    RaceSanitizer alg = MakeAlg();
    alg.simtErrors_ = nullptr;
    alg.MergeSimtErrors();
    alg.simtErrors_ = std::make_shared<std::vector<RaceDispInfo>>();
    RaceDispInfo e1{}; e1.p1.pc = 0x100; e1.p2.pc = 0x200;
    e1.p1.addr = 0x1000; e1.p2.addr = 0x1000;
    alg.simtErrors_->push_back(e1);
    alg.simtErrors_->push_back(e1);
    alg.MergeSimtErrors();
    EXPECT_LE(alg.simtErrors_->size(), 2u);
}

TEST(RaceSanitizer, IsTargetBlockId_expect_correct)
{
    RaceSanitizer alg = MakeAlg();
    Config cfg{}; cfg.checkBlockId = 5;
    alg.SetDeviceInfo({[] { DeviceInfoSummary i{}; i.device = DeviceType::ASCEND_910B1; return i; }()}, cfg);
    EXPECT_TRUE(alg.IsTargetBlockId(5));
    EXPECT_FALSE(alg.IsTargetBlockId(3));
    Config all{}; all.checkBlockId = CHECK_ALL_BLOCK;
    alg.SetDeviceInfo({[] { DeviceInfoSummary i{}; i.device = DeviceType::ASCEND_910B1; return i; }()}, all);
    EXPECT_TRUE(alg.IsTargetBlockId(alg.defaultCheckBlockId_));
    EXPECT_FALSE(alg.IsTargetBlockId(999));
}

TEST(RaceSanitizer, DcciDistanceAndEffect_expect_correct)
{
    RaceSanitizer alg = MakeAlg();
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    event.eventInfo.memInfo.opType = AccessType::READ;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = 0x1000;
    DcciRecord dcci{};
    dcci.type = DcciDstType::CACHE_LINE_UB; dcci.space = AddressSpace::GM; dcci.addr = 0x1000;
    EXPECT_FALSE(alg.DoesDcciHaveEffect(event, dcci));
    dcci.type = DcciDstType::CACHE_LINE_OUT; dcci.space = AddressSpace::UB;
    EXPECT_FALSE(alg.DoesDcciHaveEffect(event, dcci));
    event.eventInfo.memInfo.memType = MemType::L1; dcci.space = AddressSpace::GM;
    EXPECT_FALSE(alg.DoesDcciHaveEffect(event, dcci));
    event.eventInfo.memInfo.memType = MemType::GM;
    dcci.entire = DcciEntireType::ENTIRE_DATA_CACHE;
    EXPECT_TRUE(alg.DoesDcciHaveEffect(event, dcci));
    dcci.entire = DcciEntireType::SINGLE_CACHE_LINE;
    EXPECT_TRUE(alg.DoesDcciHaveEffect(event, dcci));
    event.eventInfo.memInfo.addr = 0x2000;
    EXPECT_FALSE(alg.DoesDcciHaveEffect(event, dcci));
    event.eventInfo.memInfo.opType = AccessType::MEMCPY_BLOCKS;
    KernelRecord kr{};
    std::deque<KernelRecord> ahead, after;
    EXPECT_EQ(alg.NearestDcciDistance(kr, event, ahead, after), 0u);
}

TEST(RaceSanitizer, Do_with_memory_record_expect_skip)
{
    RaceSanitizer alg = MakeAlg();
    KernelSummary ks{}; ks.kernelType = KernelType::AIVEC; ks.blockDim = 1;
    alg.SetKernelInfo(ks);
    SanitizerRecord record{};
    record.version = RecordVersion::MEMORY_RECORD;
    std::vector<SanEvent> events;
    alg.Do(record, events);
    SUCCEED();
}

TEST(RaceInfoDisplay, format_and_print_functions_expect_success)
{
    namespace S = Sanitizer;
    std::stringstream ss;
    S::PrintClassicLocation(ss, 9999, 10, 1, "");
    EXPECT_FALSE(ss.str().empty());

    S::ErrorEvent ev{};
    ev.pc = 0; ev.fileNo = 9999; ev.lineNo = 5; ev.serialNo = 2;
    S::PrintLocationInfo(ss, ev, 2);
    ev.pc = 0x100; ev.isSimt = true; ev.deviceId = 0; ev.kernelIdx = 0;
    S::PrintLocationInfo(ss, ev, 3, false);

    S::SimtThreadLocation tl{};
    tl.idX = 1; tl.idY = 2; tl.idZ = 3;
    std::stringstream s2;
    s2 << tl;
    EXPECT_FALSE(s2.str().empty());

    ss.str("");
    ss << S::RaceFormatKernelName{0, 0};
    EXPECT_NE(ss.str().find("in"), std::string::npos);

    S::ErrorEvent fe{};
    fe.pc = 0; fe.fileNo = 9999; fe.addr = 0x200; fe.coreId = 1;
    fe.blockType = BlockType::AIVEC;
    S::FormatEvent(ss, fe, " Write", "WA");
    fe.accessType = static_cast<uint8_t>(AccessType::WRITE);
    S::FormatEvent(ss, fe);

    S::RaceDispInfo ri{};
    ri.p1.serialNo = 2; ri.p2.serialNo = 1;
    ri.p1.accessType = static_cast<uint8_t>(AccessType::WRITE);
    ri.p2.accessType = static_cast<uint8_t>(AccessType::READ);
    ri.p1.memType = static_cast<uint8_t>(MemType::GM);
    ri.p1.pc = 0; ri.p2.pc = 0;
    S::FormatRaceInfo(ss, ri);
    ri.p1.isSimt = true; ri.p2.isSimt = true;
    S::FormatRaceInfo(ss, ri);
    ri.isMissDcci = true; ri.p1.isSimt = false; ri.p2.isSimt = false;
    ri.p1.accessType = static_cast<uint8_t>(AccessType::READ);
    ri.p2.accessType = static_cast<uint8_t>(AccessType::WRITE);
    S::FormatMissDcciInfo(ss, ri);
    ss << ri;
    ri.isMissDcci = false;
    ss << ri;

    S::CrossCoreSyncWarnInfo warn{};
    warn.flagId = 3; warn.baseEvent.pc = 0; warn.baseEvent.coreId = 2;
    warn.baseEvent.blockType = BlockType::AIVEC;
    ss << warn;
    SUCCEED();
}

TEST(RaceInfoDisplay, report_flag_id_warn_infos_expect_correct)
{
    namespace S = Sanitizer;
    std::vector<std::string> msgs;
    auto msgFunc = [&msgs](const S::LogLv &, S::Generator<S::DetectionInfo> &&gen) {
        msgs.push_back(gen().message);
    };

    S::ReportFlagIdWarnInfos({}, "kernel", msgFunc);
    EXPECT_TRUE(msgs.empty());

    S::CrossCoreSyncWarnInfo w1{};
    w1.flagId = 1;
    w1.baseEvent.pc = 0;
    w1.baseEvent.serialNo = 1;
    S::CrossCoreSyncWarnInfo w2 = w1;  // 重复项
    std::vector<S::CrossCoreSyncWarnInfo> infos = {w1, w2};
    S::ReportFlagIdWarnInfos(infos, "kernel", msgFunc);
    EXPECT_EQ(msgs.size(), 1u);
}
}
