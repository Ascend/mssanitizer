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
#include "record_pre_process.h"
#include "alg_framework/race_alg_impl.h"
#include "alg_framework/single_pipe_race_alg_impl.h"
#include "rand_data.h"
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
        Sanitizer::MaskMode::MASK_NORM, 8, static_cast<uint32_t>(0x50) + unitSize * 4,
        3U, unitSize, 1U, 1U, 1U, 32};
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

}
