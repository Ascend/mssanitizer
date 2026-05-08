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
#include <vector>

#include "core/framework/config.h"
#include "core/framework/cross_npu_checker.h"
#include "core/framework/device_manager.h"
#include "core/framework/event_def.h"
#include "core/framework/kernel_manager.h"
#include "core/framework/record_defs.h"
#include "core/framework/record_parse.h"
#include "core/framework/utility/log.h"
#include "race_sanitizer/cross_npu_race_sanitizer.h"
#include "sanitizer_report.h"

using namespace Sanitizer;

namespace SanitizerTest {

class TestCrossNpuRaceSanitizer : public testing::Test {
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

TEST_F(TestCrossNpuRaceSanitizer, process_mstx_record_with_tensor_expect_report_nothing)
{
    CrossNpuRaceSanitizer sanitizer;
    sanitizer.Init();
    std::vector<DetectionInfo> reports;
    sanitizer.RegisterNotifyFunc([&reports](const LogLv &lv, CrossNpuRaceSanitizer::MSG_GEN &&gen) {
        reports.emplace_back(gen());
    });

    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = RecordType::MSTX_STUB;
    MstxRecord &mstxRecord = record.payload.kernelRecord.payload.mstxRecord;
    mstxRecord.interfaceType = InterfaceType::MSTX_VEC_UNARY_OP;
    std::vector<SanEvent> events;
    RecordParse::Parse(record, events);
    sanitizer.Do(record, events);
    sanitizer.Do(record, events);

    // finish flag
    events.clear();
    record.payload.kernelRecord.recordType = RecordType::FINISH;
    RecordParse::Parse(record, events);
    sanitizer.Do(record, events);

    ASSERT_TRUE(reports.empty());
}

TEST_F(TestCrossNpuRaceSanitizer, process_record_not_on_gm_expect_report_nothing)
{
    CrossNpuRaceSanitizer sanitizer;
    sanitizer.Init();
    std::vector<DetectionInfo> reports;
    sanitizer.RegisterNotifyFunc([&reports](const LogLv &lv, CrossNpuRaceSanitizer::MSG_GEN &&gen) {
        reports.emplace_back(gen());
    });

    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = RecordType::STORE;
    record.payload.kernelRecord.blockType = BlockType::AIVEC;
    LoadStoreRecord &storeRecord = record.payload.kernelRecord.payload.loadStoreRecord;
    storeRecord.location.blockId = 0;
    storeRecord.space = AddressSpace::UB;
    storeRecord.addr = 0x00;
    storeRecord.size = 8;
    std::vector<SanEvent> events;
    RecordParse::Parse(record, events);
    // process events on device 0
    CrossNpuChecker::UpdateLoc(events, 0, 0, 0);
    sanitizer.Do(record, events);
    // process events on device 1
    CrossNpuChecker::UpdateLoc(events, 1, 0, 1);
    sanitizer.Do(record, events);

    // finish flag
    events.clear();
    record.payload.kernelRecord.recordType = RecordType::FINISH;
    RecordParse::Parse(record, events);
    sanitizer.Do(record, events);

    ASSERT_TRUE(reports.empty());
}

TEST_F(TestCrossNpuRaceSanitizer, process_record_on_same_device_expect_report_nothing)
{
    CrossNpuRaceSanitizer sanitizer;
    sanitizer.Init();
    std::vector<DetectionInfo> reports;
    sanitizer.RegisterNotifyFunc([&reports](const LogLv &lv, CrossNpuRaceSanitizer::MSG_GEN &&gen) {
        reports.emplace_back(gen());
    });

    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = RecordType::STORE;
    record.payload.kernelRecord.blockType = BlockType::AIVEC;
    LoadStoreRecord &storeRecord = record.payload.kernelRecord.payload.loadStoreRecord;
    storeRecord.location.blockId = 0;
    storeRecord.space = AddressSpace::GM;
    storeRecord.addr = 0x00;
    storeRecord.size = 8;
    std::vector<SanEvent> events;
    RecordParse::Parse(record, events);
    // process events on device 0
    CrossNpuChecker::UpdateLoc(events, 0, 0, 0);
    sanitizer.Do(record, events);
    // process events on device 0
    CrossNpuChecker::UpdateLoc(events, 0, 0, 0);
    sanitizer.Do(record, events);

    // finish flag
    events.clear();
    record.payload.kernelRecord.recordType = RecordType::FINISH;
    RecordParse::Parse(record, events);
    sanitizer.Do(record, events);

    ASSERT_TRUE(reports.empty());
}

TEST_F(TestCrossNpuRaceSanitizer, process_record_on_different_device_expect_report_correct_races)
{
    CrossNpuRaceSanitizer sanitizer;
    sanitizer.Init();
    std::vector<DetectionInfo> reports;
    sanitizer.RegisterNotifyFunc([&reports](const LogLv &lv, CrossNpuRaceSanitizer::MSG_GEN &&gen) {
        reports.emplace_back(gen());
    });

    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = RecordType::STORE;
    record.payload.kernelRecord.blockType = BlockType::AIVEC;
    LoadStoreRecord &storeRecord = record.payload.kernelRecord.payload.loadStoreRecord;
    storeRecord.location.blockId = 0;
    storeRecord.space = AddressSpace::GM;
    storeRecord.addr = 0x00;
    storeRecord.size = 8;
    std::vector<SanEvent> events;
    RecordParse::Parse(record, events);
    // process events on device 0
    CrossNpuChecker::UpdateLoc(events, 0, 0, 0);
    sanitizer.Do(record, events);
    // process events on device 1
    CrossNpuChecker::UpdateLoc(events, 1, 0, 1);
    sanitizer.Do(record, events);

    // finish flag
    events.clear();
    record.payload.kernelRecord.recordType = RecordType::FINISH;
    RecordParse::Parse(record, events);
    sanitizer.Do(record, events);

    ASSERT_EQ(reports.size(), 1);
    ASSERT_NE(reports[0].message.find("ERROR: Potential WAW hazard detected at GM"), std::string::npos);
}

} // namespace SanitizerTest