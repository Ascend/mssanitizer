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
#include "ccec_defs.h"
#include "ccec/data_process.h"
#include "core/framework/kernel_block.h"
#include "core/framework/record_parse.h"
#include "plugin/ccec/calc_instructions/vsel.cpp"
#include "plugin/record_sync_instructions.h"
#include "race_sanitizer/alg_framework/single_pipe_race_alg_impl.h"

extern void __sanitizer_report_get_cmpmask(uint8_t *memInfo, uint64_t fileNo, uint64_t lineNo, int64_t pc, void *dst);
extern void __sanitizer_report_set_cmpmask(uint8_t *memInfo, uint64_t fileNo, uint64_t lineNo, int64_t pc, void *src);

TEST(VselCalcInstructions, dump_records_and_parse_each_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint8_t recordNum = 18;
    auto records = CreateRandomBinaryOpRecords(recordNum);
    uint8_t idx = 0;
    std::vector<CheckBinaryOpBlockInfo> blockInfoVec(18);
    CheckBinaryOpBlockInfo initialValue = {8, 8, 8, 32, 32, 32};
    std::fill_n(blockInfoVec.begin(), blockInfoVec.size(), initialValue);

    CallBinaryConfigFunc<half>(memInfo.data(), records, idx++, __sanitizer_report_vsel);
    CallBinaryConfigFunc<half, half, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel);
    CallBinaryFunc<half, half, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel);
    CallBinaryFunc<half, half, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel,
        uint8_t{}, bool{}, bool{});
    CallBinaryFunc<half, half, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel, VSEL_MODE0);
    CallBinaryFunc<half, half, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel, VSEL_MODE2);

    CallBinaryFunc<half>(memInfo.data(), records, idx++, __sanitizer_report_vsel);
    CallBinaryFunc<half>(memInfo.data(), records, idx++, __sanitizer_report_vsel, uint8_t{}, bool{}, bool{});
    CallBinaryFunc<half>(memInfo.data(), records, idx++, __sanitizer_report_vsel, uint8_t{});

    CallBinaryConfigFunc<float>(memInfo.data(), records, idx++, __sanitizer_report_vsel);
    CallBinaryConfigFunc<float, float, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel);
    CallBinaryFunc<float, float, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel);
    CallBinaryFunc<float, float, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel,
        uint8_t{}, bool{}, bool{});
    CallBinaryFunc<float, float, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel, uint8_t{});

    CallBinaryFunc<float>(memInfo.data(), records, idx++, __sanitizer_report_vsel);
    CallBinaryFunc<float>(memInfo.data(), records, idx++, __sanitizer_report_vsel, uint8_t{}, bool{}, bool{});
    CallBinaryFunc<float>(memInfo.data(), records, idx++, __sanitizer_report_vsel, uint8_t{});

    bool isEqual = MultiIsEqual<BinaryOpRecord, CheckBinaryOpBlockInfo>(memInfo.data(), records, blockInfoVec);
    ASSERT_TRUE(isEqual);
}

TEST(VselCalcInstructions, dump_records_with_half_mode1_and_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint8_t recordNum = 1;
    auto records = CreateRandomBinaryOpRecords(recordNum);
    uint8_t idx = 0;
    CallBinaryFunc<half, half, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel, VSEL_MODE1);
    BinaryOpRecord const &actualRecord = *reinterpret_cast<BinaryOpRecord const *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType));
    ASSERT_EQ(actualRecord.src1BlockSize, 16);
    ASSERT_EQ(actualRecord.src1BlockNum, 1);
    ASSERT_EQ(actualRecord.src1BlockStride, 0);
    ASSERT_EQ(actualRecord.src1RepeatStride, 1);
    ASSERT_EQ(actualRecord.src0DataBits, sizeof(half) * BITS_EACH_BYTE);
    ASSERT_EQ(actualRecord.src1DataBits, sizeof(half) * BITS_EACH_BYTE);
    ASSERT_EQ(actualRecord.dstDataBits, sizeof(half) * BITS_EACH_BYTE);
}

TEST(VselCalcInstructions, dump_records_with_float_mode1_and_parse_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint8_t recordNum = 1;
    auto records = CreateRandomBinaryOpRecords(recordNum);
    uint8_t idx = 0;
    CallBinaryFunc<float, float, void>(memInfo.data(), records, idx++, __sanitizer_report_vsel, VSEL_MODE1);
    BinaryOpRecord const actualRecord = *reinterpret_cast<BinaryOpRecord const *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType));
    ASSERT_EQ(actualRecord.src1BlockSize, 8);
    ASSERT_EQ(actualRecord.src1BlockNum, 1);
    ASSERT_EQ(actualRecord.src1BlockStride, 0);
    ASSERT_EQ(actualRecord.src1RepeatStride, 1);
    ASSERT_EQ(actualRecord.src0DataBits, sizeof(float) * BITS_EACH_BYTE);
    ASSERT_EQ(actualRecord.src1DataBits, sizeof(float) * BITS_EACH_BYTE);
    ASSERT_EQ(actualRecord.dstDataBits, sizeof(float) * BITS_EACH_BYTE);
}

TEST(VselCalcInstructions, config_overloads_do_not_record_cmpmask_ub_read) {
    auto record = CreateRandomBinaryOpRecord();
    for (uint8_t selectMode : {VSEL_MODE0, VSEL_MODE1, VSEL_MODE2}) {
        SCOPED_TRACE(selectMode);
        std::vector<uint8_t> memInfo = CreateMemInfo();
        auto *globalHead = reinterpret_cast<RecordGlobalHead *>(memInfo.data());
        uint64_t config = static_cast<uint64_t>(selectMode) << VSEL_MODESHIFT;
        __sanitizer_report_vsel(memInfo.data(), record.location.fileNo, record.location.lineNo, record.location.pc,
            reinterpret_cast<__ubuf__ half *>(record.dst), reinterpret_cast<__ubuf__ half *>(record.src0),
            reinterpret_cast<__ubuf__ half *>(record.src1), config);
        const auto *recordHead = reinterpret_cast<const RecordBlockHead *>(globalHead + 1);
        ASSERT_EQ(recordHead->recordCount, 1U);
        EXPECT_EQ(*reinterpret_cast<const RecordType *>(recordHead + 1), RecordType::VSEL_OP);
    }
}

namespace {
constexpr uint64_t DESCRIPTOR_ADDR = 0x1000;

class CmpMaskRacecheck : public testing::Test {
protected:
    void *const descriptor = reinterpret_cast<void *>(DESCRIPTOR_ADDR);
    std::vector<uint8_t> memInfo = CreateMemInfo();
    SinglePipeRaceAlgImpl racecheck{KernelType::AIVEC, DeviceType::ASCEND_910B1, 1U};

    void SetUp() override {
        ResetParser();
        auto *globalHead = reinterpret_cast<RecordGlobalHead *>(memInfo.data());
        globalHead->checkParms.racecheck = true;
        globalHead->kernelInfo.totalBlockDim = 1U;
        reinterpret_cast<RecordBlockHead *>(globalHead + 1)->blockInfo.blockType = BlockType::AIVEC;
    }

    void TearDown() override { ResetParser(); }

    void RunRacecheck(uint64_t expectedRecordCount) {
        auto block = KernelBlock::CreateKernelBlock(memInfo.data(), memInfo.size(), 0U);
        ASSERT_NE(block, nullptr);
        SanitizerRecord record{};
        record.version = RecordVersion::KERNEL_RECORD;
        std::vector<SanEvent> events;
        uint64_t parsedCount = 0;
        while (block->NextSimd(record.payload.kernelRecord)) {
            RecordParse::Parse(record, events);
            ASSERT_FALSE(events.empty());
            for (const auto &event : events) {
                racecheck.Do(event);
            }
            events.clear();
            ++parsedCount;
        }
        ASSERT_EQ(parsedCount, expectedRecordCount);
        SanEvent finish{};
        finish.type = EventType::SANITIZER_CONTROL_EVENT;
        finish.eventInfo.sanitizerControlInfo.type = SanitizerControlType::KERNEL_FINISH;
        racecheck.Do(finish);
        racecheck.WaitFinished();
    }

private:
    static void ResetParser() {
        KernelBlock::ResetAll();
        RecordParse::ResetAll();
    }
};

TEST_F(CmpMaskRacecheck, consecutive_mode2_selects_reuse_descriptor_without_war) {
    auto *mem = memInfo.data();
    auto *src0 = reinterpret_cast<float *>(0x2000);
    auto *src1 = reinterpret_cast<float *>(0x3000);
    constexpr auto vectorPipe = static_cast<pipe_t>(PipeType::PIPE_V);
    for (uint64_t select = 0; select < 2U; ++select) {
        const uint64_t writeLine = 1U + select * 5U;
        auto *dst = reinterpret_cast<float *>(0x4000 + select * 0x1000);
        __sanitizer_report_get_cmpmask(mem, 0, writeLine, 0, descriptor);
        RecordPipeBarrierEvent(mem, 0, writeLine + 1U, 0, vectorPipe);
        __sanitizer_report_set_cmpmask(mem, 0, writeLine + 2U, 0, descriptor);
        RecordPipeBarrierEvent(mem, 0, writeLine + 3U, 0, vectorPipe);
        __sanitizer_report_vsel(mem, 0, writeLine + 4U, 0, dst, src0, src1, 1, 1, 1, 1, 8, 8, 8, VSEL_MODE2);
    }
    ASSERT_NO_FATAL_FAILURE(RunRacecheck(10U));
    EXPECT_EQ(racecheck.GetRaceCount(), 0U);
}

TEST_F(CmpMaskRacecheck, unsynchronized_descriptor_overwrite_without_vsel_reports_war) {
    __sanitizer_report_set_cmpmask(memInfo.data(), 0, 1, 0, descriptor);
    __sanitizer_report_get_cmpmask(memInfo.data(), 0, 2, 0, descriptor);
    ASSERT_NO_FATAL_FAILURE(RunRacecheck(2U));
    const auto races = racecheck.GetResult();
    ASSERT_EQ(races->size(), 1U);
    const auto &race = races->front();
    const auto &earlier = race.p1.lineNo < race.p2.lineNo ? race.p1 : race.p2;
    const auto &later = race.p1.lineNo < race.p2.lineNo ? race.p2 : race.p1;
    EXPECT_EQ(earlier.lineNo, 1U);
    EXPECT_EQ(static_cast<AccessType>(earlier.accessType), AccessType::READ);
    EXPECT_EQ(static_cast<MemType>(earlier.memType), MemType::UB);
    EXPECT_EQ(earlier.addr, DESCRIPTOR_ADDR);
    EXPECT_EQ(later.lineNo, 2U);
    EXPECT_EQ(static_cast<AccessType>(later.accessType), AccessType::WRITE);
    EXPECT_EQ(static_cast<MemType>(later.memType), MemType::UB);
    EXPECT_EQ(later.addr, DESCRIPTOR_ADDR);
}
} // namespace
