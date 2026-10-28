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
#include <sstream>
#include <atomic>
#include <thread>

#include "core/framework/cross_npu_checker.h"
#include "core/framework/device_manager.h"
#include "core/framework/kernel_manager.h"
#include "core/framework/record_defs.h"

using namespace Sanitizer;

namespace SanitizerTest {

TEST(CrossNpuCollection, disabled_modes_do_not_create_device_cache)
{
    for (bool race : {false, true}) {
        for (bool cross : {false, true}) {
            if (race && cross) {
                continue;
            }
            Config config{};
            config.initCheck = true;
            config.raceCheck = race;
            config.checkCrossNpuRaces = cross;
            CrossNpuChecker checker(config);
            EXPECT_EQ(checker.GetRecordArrayForCollection(3), nullptr);
            EXPECT_TRUE(checker.GetRecordMap().empty());
        }
    }
}

TEST(CrossNpuCollection, enabled_mode_retains_kernel_boundaries_and_record_order)
{
    Config config{};
    config.raceCheck = true;
    config.checkCrossNpuRaces = true;
    CrossNpuChecker checker(config);
    auto *records = checker.GetRecordArrayForCollection(3);
    ASSERT_NE(records, nullptr);
    for (uint64_t kernel = 0; kernel < 2; ++kernel) {
        records->CreateNewKernel();
        for (auto type : {RecordType::LOAD, RecordType::BLOCK_FINISH, RecordType::KERNEL_FINISH}) {
            SanitizerRecord record{};
            record.version = RecordVersion::KERNEL_RECORD;
            record.payload.kernelRecord.recordType = type;
            record.payload.kernelRecord.serialNo = kernel;
            records->Push(record);
        }
    }
    ASSERT_EQ(records->GetRecords().size(), 2U);
    for (size_t kernel = 0; kernel < 2; ++kernel) {
        const auto &batch = records->GetRecords()[kernel];
        ASSERT_EQ(batch.size(), 3U);
        EXPECT_EQ(batch[0].payload.kernelRecord.recordType, RecordType::LOAD);
        EXPECT_EQ(batch[1].payload.kernelRecord.recordType, RecordType::BLOCK_FINISH);
        EXPECT_EQ(batch[2].payload.kernelRecord.recordType, RecordType::KERNEL_FINISH);
        EXPECT_EQ(batch[0].payload.kernelRecord.serialNo, kernel);
    }
}

TEST(CrossNpuCollection, concurrent_device_lookup_preserves_cache_identity)
{
    Config config{};
    config.raceCheck = true;
    config.checkCrossNpuRaces = true;
    CrossNpuChecker checker(config);
    constexpr size_t THREAD_COUNT = 8;
    constexpr size_t DEVICES_PER_THREAD = 64;
    std::vector<CrossNpuChecker::RecordArray *> caches(THREAD_COUNT * DEVICES_PER_THREAD);
    std::atomic<size_t> ready{0};
    std::atomic<bool> start{false};
    std::vector<std::thread> threads;
    for (size_t t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&, t]() {
            ++ready;
            while (!start.load()) {
                std::this_thread::yield();
            }
            for (size_t i = 0; i < DEVICES_PER_THREAD; ++i) {
                const auto id = t * DEVICES_PER_THREAD + i;
                caches[id] = checker.GetRecordArrayForCollection(id);
            }
        });
    }
    while (ready.load() != THREAD_COUNT) {
        std::this_thread::yield();
    }
    start = true;
    for (auto &thread : threads) {
        thread.join();
    }
    ASSERT_EQ(checker.GetRecordMap().size(), caches.size());
    for (size_t id = 0; id < caches.size(); ++id) {
        EXPECT_EQ(caches[id], checker.GetRecordArrayForCollection(id));
    }
}

class TestCrossNpuChecker : public testing::Test {
public:
    void SetUp() override
    {
        DeviceManager::Instance().Clear();
        KernelManager::Instance().Clear();

        DeviceInfoSummary deviceInfo{};
        deviceInfo.deviceId = 0;
        deviceInfo.device = DeviceType::ASCEND_910B1;
        DeviceManager::Instance().Set(deviceInfo.deviceId, deviceInfo);
        deviceInfo.deviceId = 1;
        DeviceManager::Instance().Set(deviceInfo.deviceId, deviceInfo);

        KernelSummary kernelSummary{};
        kernelSummary.blockDim = 8;
        kernelSummary.kernelType = KernelType::MIX;
        KernelManager::Instance().Add(0, kernelSummary);
        KernelManager::Instance().Add(1, kernelSummary);
    }
    void TearDown() override
    {
        DeviceManager::Instance().Clear();
        KernelManager::Instance().Clear();
    }
};

TEST_F(TestCrossNpuChecker, record_array_create_new_kernel_expect_success)
{
    CrossNpuChecker::RecordArray recordArray;
    recordArray.CreateNewKernel();
    ASSERT_EQ(recordArray.GetRecords().size(), 1);
}

TEST_F(TestCrossNpuChecker, record_array_push_record_expect_success)
{
    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = RecordType::KERNEL_FINISH;

    CrossNpuChecker::RecordArray recordArray;
    recordArray.CreateNewKernel();
    recordArray.Push(record);

    ASSERT_EQ(recordArray.GetRecords().size(), 1);
    ASSERT_EQ(recordArray.GetRecords()[0].size(), 1);
    SanitizerRecord const &recordGet = recordArray.GetRecords()[0][0];
    ASSERT_EQ(recordGet.version, record.version);
    ASSERT_EQ(recordGet.payload.kernelRecord.recordType, record.payload.kernelRecord.recordType);
}

TEST_F(TestCrossNpuChecker, get_record_array_expect_get_correct_record_array)
{
    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = RecordType::KERNEL_FINISH;

    Config config{};
    CrossNpuChecker checker(config);
    checker.GetRecordArray(0).CreateNewKernel();
    checker.GetRecordArray(0).Push(record);
    checker.GetRecordArray(1).CreateNewKernel();
    checker.GetRecordArray(1).Push(record);

    CrossNpuChecker::RecordMap &recordMap = checker.GetRecordMap();
    ASSERT_EQ(recordMap.size(), 2);
    ASSERT_NE(recordMap.find(0), recordMap.cend());
    ASSERT_NE(recordMap.find(1), recordMap.cend());
    ASSERT_EQ(recordMap[0].GetRecords().size(), 1);
    ASSERT_EQ(recordMap[0].GetRecords()[0].size(), 1);
    ASSERT_EQ(recordMap[1].GetRecords().size(), 1);
    ASSERT_EQ(recordMap[1].GetRecords()[0].size(), 1);
}

TEST_F(TestCrossNpuChecker, check_records_on_same_device_expect_report_nothing)
{
    Config config{};
    CrossNpuChecker checker(config);
    std::ostringstream oss;
    checker.SetDetectionInfo(LogLv::ERROR, oss);

    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = RecordType::STORE;
    record.payload.kernelRecord.blockType = BlockType::AIVEC;
    LoadStoreRecord &storeRecord = record.payload.kernelRecord.payload.loadStoreRecord;
    storeRecord.location.blockId = 0;
    storeRecord.space = AddressSpace::GM;
    storeRecord.addr = 0x00;
    storeRecord.size = 8;

    checker.GetRecordArray(0).CreateNewKernel();
    checker.GetRecordArray(0).Push(record);
    checker.GetRecordArray(0).Push(record);
    checker.Run();

    ASSERT_EQ(oss.str().find("ERROR"), std::string::npos);
    ASSERT_NE(oss.str().find("No error detected"), std::string::npos);
}

TEST_F(TestCrossNpuChecker, check_records_on_different_device_expect_report_correct_races)
{
    Config config{};
    CrossNpuChecker checker(config);
    std::ostringstream oss;
    checker.SetDetectionInfo(LogLv::ERROR, oss);

    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = RecordType::STORE;
    record.payload.kernelRecord.blockType = BlockType::AIVEC;
    LoadStoreRecord &storeRecord = record.payload.kernelRecord.payload.loadStoreRecord;
    storeRecord.location.blockId = 0;
    storeRecord.space = AddressSpace::GM;
    storeRecord.addr = 0x00;
    storeRecord.size = 8;

    DeviceManager::Instance().GetSharedMemorySpans(0).Union({storeRecord.addr, storeRecord.addr + storeRecord.size});
    DeviceManager::Instance().GetSharedMemorySpans(1).Union({storeRecord.addr, storeRecord.addr + storeRecord.size});

    checker.GetRecordArray(0).CreateNewKernel();
    checker.GetRecordArray(0).Push(record);
    checker.GetRecordArray(1).CreateNewKernel();
    checker.GetRecordArray(1).Push(record);
    checker.Run();

    ASSERT_NE(oss.str().find("ERROR: Potential WAW hazard detected at GM"), std::string::npos);
    ASSERT_NE(oss.str().find("See all detected errors above"), std::string::npos);
}

} // namespace SanitizerTest
