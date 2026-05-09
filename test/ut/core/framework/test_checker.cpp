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

#define private public
#include "address_sanitizer.h"
#include "checker.h"
#undef private
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
    Checker checker(config);
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
    Checker checker(config);
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
    };
    sanRecord.payload.kernelRecord.recordType = RecordType::BINARY_OP;
    sanRecord.payload.kernelRecord.blockType = BlockType::AIVEC;
    sanRecord.payload.kernelRecord.payload.binaryOpRecord = record;
    return sanRecord;
}

int GetDetectInfoNum(const std::string & str)
{
    size_t count = 0;
    size_t pos = 0;
    const std::string pattern = "serialNo";
    while ((pos = str.find(pattern, pos)) != std::string::npos) {
        ++count;
        pos += pattern.length(); // 避免重叠匹配
    }
    return count;
}

std::shared_ptr<Checker> GetNewChecker(std::stringstream &ss)
{
    Config config{};
    config.defaultCheck = true;
    config.memCheck = true;
    config.raceCheck = true;

    auto checker = std::make_shared<Checker>(config);
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

TEST(Checker, set_multi_detect_then_fake_record_run_single_data_expect_normal)
{
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

    auto checker = std::make_shared<Checker>(config);
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

    auto checker = std::make_shared<Checker>(config);
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

    auto checker = std::make_shared<Checker>(config);
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
