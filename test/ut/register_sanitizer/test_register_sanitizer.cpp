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
#include "register_sanitizer.h"
#undef private

namespace {
using namespace Sanitizer;

void UT_FillRegRecord(SanitizerRecord& record, RecordType type, uint16_t coreId, RegisterValueType regValType, uint64_t regVal, int64_t regIdx)
{
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = type;
    
    auto& regRecord = record.payload.kernelRecord.payload.registerSetRecord;
    regRecord.location.blockId = coreId;
    regRecord.regPayLoad.regValType = regValType;
    regRecord.regPayLoad.regVal = regVal;
    regRecord.regPayLoad.regIdx = regIdx;
}

int64_t UT_GetRegIdx()
{
    static int64_t regIdx = 0;
    regIdx += 5;
    if (regIdx >= C220_A2_A3_MAXCORE_NUM) {
        regIdx = 0;
    }
    return regIdx;
}

TEST(RegisterSanitizer, set_vector_mask_0_expect_report_exception)
{
    Config config {};
    config.registerCheck = true;
    DeviceInfoSummary deviceInfoSummary {};
    deviceInfoSummary.device = DeviceType::ASCEND_910B1;
    auto regSanitizer = SanitizerFactory::GetInstance().Create(ToolType::REGISTERCHECK);
    ASSERT_FALSE(regSanitizer->SetDeviceInfo(deviceInfoSummary, config));

    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123456;
    size_t regType = static_cast<size_t>(RegisterType::VECTOR_MASK_0);
    uint64_t regvalU64Dft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_VECTOR_MASK_0;
    uint16_t coreId = 0;
    int64_t regIdx = UT_GetRegIdx();
    
    // 未重置为默认值报告警
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 1U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalU64);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") != std::string::npos);
    ASSERT_TRUE(msg.find("current value is (" + std::to_string(regvalU64) + ")") != std::string::npos);
}

TEST(RegisterSanitizer, set_vector_mask_0_expect_not_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123456;
    size_t regType = static_cast<size_t>(RegisterType::VECTOR_MASK_0);
    uint64_t regvalU64Dft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_VECTOR_MASK_0;
    uint16_t coreId = 0;
    int64_t regIdx = UT_GetRegIdx();

    // 重置为默认值不报告警
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64Dft, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 2U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalU64Dft);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") == std::string::npos);
    ASSERT_TRUE(msg.find("current valure is (" + std::to_string(regvalU64) + ")") == std::string::npos);
}

TEST(RegisterSanitizer, set_vector_mask_1_expect_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123457;
    size_t regType = static_cast<size_t>(RegisterType::VECTOR_MASK_1);
    uint64_t regvalU64Dft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_VECTOR_MASK_1;
    uint16_t coreId = 1;
    int64_t regIdx = UT_GetRegIdx();
    
    // 未重置为默认值报告警
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 1U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalU64);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") != std::string::npos);
    ASSERT_TRUE(msg.find("current value is (" + std::to_string(regvalU64) + ")") != std::string::npos);

}

TEST(RegisterSanitizer, set_vector_mask_1_expect_not_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123457;
    size_t regType = static_cast<size_t>(RegisterType::VECTOR_MASK_1);
    uint64_t regvalU64Dft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_VECTOR_MASK_1;
    uint16_t coreId = 1;
    int64_t regIdx = UT_GetRegIdx();

    // 重置为默认值不报告警
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64Dft, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 2U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalU64Dft);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") == std::string::npos);
    ASSERT_TRUE(msg.find("current valure is (" + std::to_string(regvalU64) + ")") == std::string::npos);
}

TEST(RegisterSanitizer, set_ctrl_non_bit56_expect_not_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123458;
    size_t regType = static_cast<size_t>(RegisterType::CTRL);
    uint64_t regvalU64Dft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_CTRL;
    uint16_t coreId = 0;
    int64_t regIdx = UT_GetRegIdx();
    
    // 非bit56位未复位不报告警
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 1U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, 0); // ctrl寄存器只看bit56
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") == std::string::npos);
    ASSERT_TRUE(msg.find("current value is (" + std::to_string(regvalU64) + ")") == std::string::npos);
}

TEST(RegisterSanitizer, set_ctrl_bit56_expect_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 1UL << 56;
    size_t regType = static_cast<size_t>(RegisterType::CTRL);
    uint64_t regvalU64Dft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_CTRL;
    uint16_t coreId = 0;
    int64_t regIdx = UT_GetRegIdx();

    // bit56位未重置为默认值报告警
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 1U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalU64 >> 56);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") != std::string::npos);
    ASSERT_TRUE(msg.find("current value is (" + std::to_string(regvalU64 >> 56) + ")") != std::string::npos);
}

TEST(RegisterSanitizer, set_ctrl_expect_not_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123458;
    size_t regType = static_cast<size_t>(RegisterType::CTRL);
    uint64_t regvalU64Dft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_CTRL;
    uint16_t coreId = 0;
    int64_t regIdx = UT_GetRegIdx();

    // 重置为默认值不报告警
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64Dft, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 2U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalU64Dft);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") == std::string::npos);
    ASSERT_TRUE(msg.find("current valure is (" + std::to_string(regvalU64) + ")") == std::string::npos);
}

TEST(RegisterSanitizer, set_ffts_base_addr_expect_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123459;
    size_t regType = static_cast<size_t>(RegisterType::FFTS_BASE_ADDR);
    uint64_t regvalU64Dft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_FFTS_BASE_ADDR;
    uint16_t coreId = 1;
    int64_t regIdx = UT_GetRegIdx();
    
    // 未重置为默认值报告警
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 1U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalU64);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") != std::string::npos);
    ASSERT_TRUE(msg.find("current value is (" + std::to_string(regvalU64) + ")") != std::string::npos);
}

TEST(RegisterSanitizer, set_ffts_base_addr_expect_not_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123459;
    size_t regType = static_cast<size_t>(RegisterType::FFTS_BASE_ADDR);
    uint64_t regvalU64Dft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_FFTS_BASE_ADDR;
    uint16_t coreId = 1;
    int64_t regIdx = UT_GetRegIdx();

    // 重置为默认值不报告警
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64Dft, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 2U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalU64Dft);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") == std::string::npos);
    ASSERT_TRUE(msg.find("current valure is (" + std::to_string(regvalU64) + ")") == std::string::npos);
}

TEST(RegisterSanitizer, set_fpc_expect_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123460;
    size_t regType = static_cast<size_t>(RegisterType::FPC);
    uint64_t regvalU64Dft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_FPC;
    uint16_t coreId = 0;
    int64_t regIdx = UT_GetRegIdx();

    // 未重置为默认值报告警
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 1U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalU64);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") != std::string::npos);
    ASSERT_TRUE(msg.find("current valure is (" + std::to_string(regvalU64) + ")") == std::string::npos);
}

TEST(RegisterSanitizer, set_fpc_expect_not_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123460;
    size_t regType = static_cast<size_t>(RegisterType::FPC);
    uint64_t regvalU64Dft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_FPC;
    uint16_t coreId = 0;
    int64_t regIdx = UT_GetRegIdx();

    // 重置为默认值不报告警
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64Dft, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 2U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalU64Dft);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") == std::string::npos);
    ASSERT_TRUE(msg.find("current valure is (" + std::to_string(regvalU64) + ")") == std::string::npos);
}


TEST(RegisterSanitizer, set_quant_pre_expect_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123461;
    size_t regType = static_cast<size_t>(RegisterType::QUANT_PRE);
    uint64_t regvalU64Dft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_QUANT_PRE;
    uint16_t coreId = 1;
    int64_t regIdx = UT_GetRegIdx();
    
    // 未重置为默认值报告警
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 1U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalU64);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") != std::string::npos);
    ASSERT_TRUE(msg.find("current value is (" + std::to_string(regvalU64) + ")") != std::string::npos);
}

TEST(RegisterSanitizer, set_quant_pre_expect_not_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123461;
    size_t regType = static_cast<size_t>(RegisterType::QUANT_PRE);
    uint64_t regvalU64Dft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_QUANT_PRE;
    uint16_t coreId = 1;
    int64_t regIdx = UT_GetRegIdx();

    // 重置为默认值不报告警
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64Dft, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 2U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalU64Dft);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") == std::string::npos);
    ASSERT_TRUE(msg.find("current valure is (" + std::to_string(regvalU64) + ")") == std::string::npos);
}

TEST(RegisterSanitizer, set_quant_post_expect_not_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123462;
    size_t regType = static_cast<size_t>(RegisterType::QUANT_POST);
    uint64_t regvalU64Dft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_QUANT_POST;
    uint16_t coreId = 0;
    int64_t regIdx = UT_GetRegIdx();
    
    // 未重置为默认值不报告警，因为post_expect寄存器不检测
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 1U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalU64);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") == std::string::npos);
    ASSERT_TRUE(msg.find("current value is (" + std::to_string(regvalU64) + ")") == std::string::npos);
}

TEST(RegisterSanitizer, set_quant_post_expect_not_report_exception_2)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123462;
    size_t regType = static_cast<size_t>(RegisterType::QUANT_POST);
    uint64_t regvalU64Dft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_QUANT_POST;
    uint16_t coreId = 0;
    int64_t regIdx = UT_GetRegIdx();

    // 重置为默认值不报告警
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64Dft, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 2U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalU64Dft);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") == std::string::npos);
    ASSERT_TRUE(msg.find("current valure is (" + std::to_string(regvalU64) + ")") == std::string::npos);
}

TEST(RegisterSanitizer, set_lrelu_alpha_uint64_expect_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123463;
    uint16_t regvalHalf = 123464;
    uint32_t regvalFloat = 123465;
    size_t regType = static_cast<size_t>(RegisterType::LRELU_ALPHA);
    uint64_t regvalDft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_LRELU_ALPHA;
    uint16_t coreId = 1;
    int64_t regIdx = UT_GetRegIdx();
    
    // 未重置为默认值报告警 - UINT64
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 1U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalU64);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") != std::string::npos);
    ASSERT_TRUE(msg.find("current value is (" + std::to_string(regvalU64) + ")") != std::string::npos);
}

TEST(RegisterSanitizer, set_lrelu_alpha_uint64_expect_not_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123463;
    uint16_t regvalHalf = 123464;
    uint32_t regvalFloat = 123465;
    size_t regType = static_cast<size_t>(RegisterType::LRELU_ALPHA);
    uint64_t regvalDft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_LRELU_ALPHA;
    uint16_t coreId = 1;
    int64_t regIdx = UT_GetRegIdx();
    
    // 重置为默认值不报告警 - UINT64
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalU64, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalDft, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 2U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalDft);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") == std::string::npos);
    ASSERT_TRUE(msg.find("current valure is (" + std::to_string(regvalU64) + ")") == std::string::npos);
}

TEST(RegisterSanitizer, set_lrelu_alpha_half_expect_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123463;
    uint16_t regvalHalf = 123464;
    uint32_t regvalFloat = 123465;
    size_t regType = static_cast<size_t>(RegisterType::LRELU_ALPHA);
    uint64_t regvalDft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_LRELU_ALPHA;
    uint16_t coreId = 1;
    int64_t regIdx = UT_GetRegIdx();

    // 未重置为默认值不报告警 - half
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_HALF, regvalHalf, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 1U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalHalf);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") != std::string::npos);
    ASSERT_TRUE(msg.find("current value is (" + std::to_string(regvalHalf) + ")") != std::string::npos);
}

TEST(RegisterSanitizer, set_lrelu_alpha_half_expect_not_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123463;
    uint16_t regvalHalf = 123464;
    uint32_t regvalFloat = 123465;
    size_t regType = static_cast<size_t>(RegisterType::LRELU_ALPHA);
    uint64_t regvalDft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_LRELU_ALPHA;
    uint16_t coreId = 1;
    int64_t regIdx = UT_GetRegIdx();

    // 重置为默认值不报告警 - half
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_HALF, static_cast<uint64_t>(regvalU64), regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_HALF, regvalDft, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 2U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalDft);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") == std::string::npos);
    ASSERT_TRUE(msg.find("current valure is (" + std::to_string(regvalU64) + ")") == std::string::npos);
}

TEST(RegisterSanitizer, set_lrelu_alpha_float_expect_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123463;
    uint16_t regvalHalf = 123464;
    uint32_t regvalFloat = 123465;
    size_t regType = static_cast<size_t>(RegisterType::LRELU_ALPHA);
    uint64_t regvalDft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_LRELU_ALPHA;
    uint16_t coreId = 1;
    int64_t regIdx = UT_GetRegIdx();

    // 未重置为默认值报告警 - float
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_UINT64, regvalFloat, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 1U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalFloat);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") != std::string::npos);
    ASSERT_TRUE(msg.find("current value is (" + std::to_string(regvalFloat) + ")") != std::string::npos);
}

TEST(RegisterSanitizer, set_lrelu_alpha_float_expect_not_report_exception)
{
    RegisterSanitizer regSan {};
    regSan.Init();
    std::string msg {};
    regSan.RegisterNotifyFunc([&msg](LogLv const&, SanitizerBase::MSG_GEN &&gen) { msg += gen().message; });

    std::vector<SanEvent> events {};
    SanEvent endEvent {};
    endEvent.isEndFrame = true;
    SanitizerRecord record {};
    uint64_t regvalU64 = 123463;
    uint16_t regvalHalf = 123464;
    uint32_t regvalFloat = 123465;
    size_t regType = static_cast<size_t>(RegisterType::LRELU_ALPHA);
    uint64_t regvalDft = Sanitizer::g_regInfoTbl[regType].regDftVal;
    RecordType recordType = RecordType::SET_LRELU_ALPHA;
    uint16_t coreId = 1;
    int64_t regIdx = UT_GetRegIdx();

    // 重置为默认值不报告警 - float
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_FLOAT, static_cast<uint64_t>(regvalFloat), regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    UT_FillRegRecord(record, recordType, coreId, RegisterValueType::VAL_FLOAT, regvalDft, regIdx);
    RecordPreProcess::GetInstance().Process(record, events);
    ASSERT_EQ(events.size(), 2U);
    events.emplace_back(endEvent);
    regSan.Do(record, events);

    std::cout << "[UT_DEBUG]msg: \n" << msg << std::endl;
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].regVal, regvalDft);
    ASSERT_EQ(regSan.regValActual_[regIdx][regType].blockId, coreId);
    ASSERT_TRUE(msg.find("Register " + Sanitizer::g_regInfoTbl[regType].regNameStr + " was not reset to default in block aiv("
        + std::to_string(coreId) + ")") == std::string::npos);
    ASSERT_TRUE(msg.find("current valure is (" + std::to_string(regvalFloat) + ")") == std::string::npos);
}

}