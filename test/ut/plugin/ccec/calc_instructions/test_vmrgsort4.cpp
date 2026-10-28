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
#include "plugin/ccec/calc_instructions/vmrgsort4.cpp"

using namespace Sanitizer;

template<RecordType recordType, typename T>
void CheckDstPubParm(UnaryOpRecord *dstRecord, UnaryOpRecord record)
{
    uint16_t srcUnit = recordType == RecordType::VMRGSORT4_OP_C220 ? 8U : sizeof(T) * 8;
    uint64_t addrList0 = ((record.src >> 0) & 0xFFFF) * srcUnit;
    ASSERT_EQ(dstRecord->src, addrList0);
    ASSERT_EQ(dstRecord->dst, record.dst);
    ASSERT_EQ(dstRecord->location.fileNo, record.location.fileNo);
    ASSERT_EQ(dstRecord->location.lineNo, record.location.lineNo);
    ASSERT_EQ(dstRecord->repeat, record.repeat);
    ASSERT_EQ(dstRecord->dstBlockStride, 1);
    ASSERT_EQ(dstRecord->srcBlockStride, 1);
    ASSERT_EQ(dstRecord->dstRepeatStride, 1);
    ASSERT_EQ(dstRecord->srcRepeatStride, 1);
    ASSERT_EQ(dstRecord->dstBlockNum, 1);
    ASSERT_EQ(dstRecord->srcBlockNum, 1);
}

template<RecordType recordType, typename T>
void CheckElementRead(ElementRecord *dstRecord, __ubuf__ T *src, uint16_t *rpArr, uint8_t index)
{
    uint16_t srcUnit = recordType == RecordType::VMRGSORT4_OP_C220 ? 8U : sizeof(T) * 8;
    uint64_t addrList = ((reinterpret_cast<uint64_t>(src) >> (16 * index)) & 0xFFFF) * srcUnit;
    ASSERT_EQ(dstRecord->addr, addrList);
    uint16_t blockSize = srcUnit;
    ASSERT_EQ(dstRecord->blockSize, blockSize);
    ASSERT_EQ(dstRecord->blockNum, rpArr[index]);
    ASSERT_EQ(dstRecord->alignSize, blockSize);
    ASSERT_EQ(dstRecord->dataBits, sizeof(T) * 8);
}

template<RecordType recordType, typename T>
void CheckElementWrite(ElementRecord *dstRecord, __ubuf__ T *dst, uint16_t *rpArr)
{
    ASSERT_EQ(dstRecord->addr, reinterpret_cast<uint64_t>(dst));
    uint16_t blockSize = recordType == RecordType::VMRGSORT4_OP_C220 ? 8U : sizeof(T) * 8;
    ASSERT_EQ(dstRecord->blockSize, blockSize);
    ASSERT_EQ(dstRecord->blockNum, rpArr[0] + rpArr[1] + rpArr[2] + rpArr[3]);
    uint16_t alignSize = recordType == RecordType::VMRGSORT4_OP_C220 ? 32U : sizeof(T) * 8;
    ASSERT_EQ(dstRecord->alignSize, alignSize);
    ASSERT_EQ(dstRecord->dataBits, sizeof(T) * 8);
}

uint64_t MergeVmrgsort4Config(uint8_t repeat, uint16_t *regionProposalArr)
{
    return (repeat & 0xFF) | (uint64_t(regionProposalArr[0] & 0xFFF) << 8) |
           (uint64_t(regionProposalArr[1] & 0xFFF) << 20) | (uint64_t(regionProposalArr[2] & 0xFFF) << 32) |
           (uint64_t(regionProposalArr[3] & 0xFFF) << 44) | (uint64_t(0xF) << 60);
}

TEST(Vmrgsort4CalcInstructions, dump_records_with_repeat_greater_than_1_and_parse_expect_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint16_t regionProposalArr[4] = {2, 3, 4, 5};
    auto record1 = CreateRandomUnaryOpRecord();
    record1.repeat = RandInt(2, 0xFF);
    __sanitizer_report_vmrgsort4(memInfo.data(), record1.location.fileNo, record1.location.lineNo, record1.location.pc,
        reinterpret_cast<__ubuf__ float *>(record1.dst), reinterpret_cast<__ubuf__ float *>(record1.src),
        record1.repeat, regionProposalArr[0], regionProposalArr[1], regionProposalArr[2], regionProposalArr[3], 0, 15);

    auto record = reinterpret_cast<UnaryOpRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
                                                    sizeof(RecordBlockHead) + sizeof(RecordType));
    CheckDstPubParm<RecordType::VMRGSORT4_OP_C220, float>(record, record1);
    ASSERT_EQ(record->srcBlockSize, 8 * regionProposalArr[0] * 4);
    ASSERT_EQ(record->dstBlockSize, 8 * regionProposalArr[0] * 4);
}

TEST(Vmrgsort4CalcInstructions, dump_records_with_repeat_equal_1_and_parse_expect_success_c220)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint16_t regionProposalArr[4] = {2, 3, 4, 5};
    auto record1 = CreateRandomUnaryOpRecord();
    record1.repeat = 1;
    __sanitizer_report_vmrgsort4(memInfo.data(), record1.location.fileNo, record1.location.lineNo, record1.location.pc,
        reinterpret_cast<__ubuf__ float *>(record1.dst), reinterpret_cast<__ubuf__ float *>(record1.src),
        record1.repeat, regionProposalArr[0], regionProposalArr[1], regionProposalArr[2], regionProposalArr[3], 0, 15);

    auto readRecord0 = reinterpret_cast<ElementRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
                                                         sizeof(RecordBlockHead) + sizeof(RecordType));
    CheckElementRead<RecordType::VMRGSORT4_OP_C220>(readRecord0, reinterpret_cast<__ubuf__ float *>(record1.src),
                                                    regionProposalArr, 0);
    ASSERT_EQ(readRecord0->location.fileNo, record1.location.fileNo);
    ASSERT_EQ(readRecord0->location.lineNo, record1.location.lineNo);

    auto readRecord1 = reinterpret_cast<ElementRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType) * 2 + sizeof(ElementRecord));
    CheckElementRead<RecordType::VMRGSORT4_OP_C220>(readRecord1, reinterpret_cast<__ubuf__ float *>(record1.src),
                                                    regionProposalArr, 1);

    auto readRecord2 = reinterpret_cast<ElementRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType) * 3 + sizeof(ElementRecord) * 2);
    CheckElementRead<RecordType::VMRGSORT4_OP_C220>(readRecord2, reinterpret_cast<__ubuf__ float *>(record1.src),
                                                    regionProposalArr, 2);

    auto readRecord3 = reinterpret_cast<ElementRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType) * 4 + sizeof(ElementRecord) * 3);
    CheckElementRead<RecordType::VMRGSORT4_OP_C220>(readRecord3, reinterpret_cast<__ubuf__ float *>(record1.src),
                                                    regionProposalArr, 3);

    auto writeRecord = reinterpret_cast<ElementRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType) * 5 + sizeof(ElementRecord) * 4);
    CheckElementWrite<RecordType::VMRGSORT4_OP_C220>(writeRecord, reinterpret_cast<__ubuf__ float *>(record1.dst),
                                                     regionProposalArr);
}

TEST(Vmrgsort4CalcInstructions, dump_half_type_records_with_repeat_greater_than_1_and_parse_expect_success_m200)
{
    constexpr uint16_t regPropSize = sizeof(half) * 8;
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint16_t regionProposalArr[4] = {2, 2, 2, 2};
    auto record1 = CreateRandomUnaryOpRecord();
    record1.repeat = RandInt(2, 0xFF);
    uint64_t config = MergeVmrgsort4Config(record1.repeat, regionProposalArr);
    __sanitizer_report_vmrgsort4(memInfo.data(), record1.location.fileNo, record1.location.lineNo, record1.location.pc,
        reinterpret_cast<__ubuf__ half *>(record1.dst), reinterpret_cast<__ubuf__ half *>(record1.src), config);

    auto record = reinterpret_cast<UnaryOpRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
                                                    sizeof(RecordBlockHead) + sizeof(RecordType));
    CheckDstPubParm<RecordType::VMRGSORT4_OP_M200, half>(record, record1);
    ASSERT_EQ(record->srcBlockSize, regPropSize * regionProposalArr[0] * 4);
    ASSERT_EQ(record->dstBlockSize, regPropSize * regionProposalArr[0] * 4);
}

TEST(Vmrgsort4CalcInstructions, dump_half_type_records_with_repeat_equal_1_and_parse_expect_success_m200)
{
    constexpr uint16_t regPropSize = sizeof(half) * 8;
    std::vector<uint8_t> memInfo = CreateMemInfo();
    constexpr uint8_t maskSignal = 15;
    uint16_t regionProposalArr[4] = {2, 3, 4, 0};
    auto record1 = CreateRandomUnaryOpRecord();
    record1.repeat = 1;
    uint64_t config = (record1.repeat & 0xFF) | (uint64_t(regionProposalArr[0] & 0xFFF) << 8) |
                      (uint64_t(regionProposalArr[1] & 0xFFF) << 20) | (uint64_t(regionProposalArr[2] & 0xFFF) << 32) |
                      (uint64_t(regionProposalArr[3] & 0xFFF) << 44) | (uint64_t(maskSignal & 0xF) << 60);
    __sanitizer_report_vmrgsort4(memInfo.data(), record1.location.fileNo, record1.location.lineNo, record1.location.pc,
        reinterpret_cast<__ubuf__ half *>(record1.dst), reinterpret_cast<__ubuf__ half *>(record1.src), config);

    auto readRecord0 = reinterpret_cast<ElementRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
                                                         sizeof(RecordBlockHead) + sizeof(RecordType));
    CheckElementRead<RecordType::VMRGSORT4_OP_M200>(readRecord0, reinterpret_cast<__ubuf__ half *>(record1.src),
                                                    regionProposalArr, 0);

    auto readRecord1 = reinterpret_cast<ElementRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType) * 2 + sizeof(ElementRecord));
    CheckElementRead<RecordType::VMRGSORT4_OP_M200>(readRecord1, reinterpret_cast<__ubuf__ half *>(record1.src),
                                                    regionProposalArr, 1);

    auto readRecord2 = reinterpret_cast<ElementRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType) * 3 + sizeof(ElementRecord) * 2);
    CheckElementRead<RecordType::VMRGSORT4_OP_M200>(readRecord2, reinterpret_cast<__ubuf__ half *>(record1.src),
                                                    regionProposalArr, 2);

    auto readRecord3 = reinterpret_cast<ElementRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType) * 4 + sizeof(ElementRecord) * 3);
    CheckElementRead<RecordType::VMRGSORT4_OP_M200>(readRecord3, reinterpret_cast<__ubuf__ half *>(record1.src),
                                                    regionProposalArr, 3);

    auto writeRecord = reinterpret_cast<ElementRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType) * 5 + sizeof(ElementRecord) * 4);
    CheckElementWrite<RecordType::VMRGSORT4_OP_M200>(writeRecord, reinterpret_cast<__ubuf__ half *>(record1.dst),
                                                     regionProposalArr);
}


TEST(Vmrgsort4CalcInstructions, dump_float_type_records_with_repeat_greater_than_1_and_parse_expect_success_m200)
{
    constexpr uint16_t regPropSize = sizeof(float) * 8;
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint16_t regionProposalArr[4] = {2, 2, 2, 2};
    auto record1 = CreateRandomUnaryOpRecord();
    record1.repeat = RandInt(2, 0xFF);
    uint64_t config = MergeVmrgsort4Config(record1.repeat, regionProposalArr);
    __sanitizer_report_vmrgsort4(memInfo.data(), record1.location.fileNo, record1.location.lineNo, record1.location.pc,
        reinterpret_cast<__ubuf__ float *>(record1.dst), reinterpret_cast<__ubuf__ float *>(record1.src), config);

    auto record = reinterpret_cast<UnaryOpRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
                                                    sizeof(RecordBlockHead) + sizeof(RecordType));
    CheckDstPubParm<RecordType::VMRGSORT4_OP_M200, float>(record, record1);
    ASSERT_EQ(record->srcBlockSize, regPropSize * regionProposalArr[0] * 4);
    ASSERT_EQ(record->dstBlockSize, regPropSize * regionProposalArr[0] * 4);
}

TEST(Vmrgsort4CalcInstructions, dump_float_type_records_with_repeat_equal_1_and_parse_expect_success_m200)
{
    constexpr uint16_t regPropSize = sizeof(float) * 8;
    std::vector<uint8_t> memInfo = CreateMemInfo();
    uint16_t regionProposalArr[4] = {2, 3, 4, 0};
    auto record1 = CreateRandomUnaryOpRecord();
    record1.repeat = 1;
    uint64_t config = MergeVmrgsort4Config(record1.repeat, regionProposalArr);
    __sanitizer_report_vmrgsort4(memInfo.data(), record1.location.fileNo, record1.location.lineNo, record1.location.pc,
        reinterpret_cast<__ubuf__ float *>(record1.dst), reinterpret_cast<__ubuf__ float *>(record1.src), config);

    auto readRecord0 = reinterpret_cast<ElementRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
                                                         sizeof(RecordBlockHead) + sizeof(RecordType));
    CheckElementRead<RecordType::VMRGSORT4_OP_M200>(readRecord0, reinterpret_cast<__ubuf__ float *>(record1.src),
                                                    regionProposalArr, 0);

    auto readRecord1 = reinterpret_cast<ElementRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType) * 2 + sizeof(ElementRecord));
    CheckElementRead<RecordType::VMRGSORT4_OP_M200>(readRecord1, reinterpret_cast<__ubuf__ float *>(record1.src),
                                                    regionProposalArr, 1);

    auto readRecord2 = reinterpret_cast<ElementRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType) * 3 + sizeof(ElementRecord) * 2);
    CheckElementRead<RecordType::VMRGSORT4_OP_M200>(readRecord2, reinterpret_cast<__ubuf__ float *>(record1.src),
                                                    regionProposalArr, 2);

    auto readRecord3 = reinterpret_cast<ElementRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType) * 4 + sizeof(ElementRecord) * 3);
    CheckElementRead<RecordType::VMRGSORT4_OP_M200>(readRecord3, reinterpret_cast<__ubuf__ float *>(record1.src),
                                                    regionProposalArr, 3);

    auto writeRecord = reinterpret_cast<ElementRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType) * 5 + sizeof(ElementRecord) * 4);
    CheckElementWrite<RecordType::VMRGSORT4_OP_M200>(writeRecord, reinterpret_cast<__ubuf__ float *>(record1.dst),
                                                     regionProposalArr);
}

TEST(Vmrgsort4CalcInstructions, dump_c220_config_and_half_explicit_overloads_expect_success) {
    uint16_t rp[4] = {2, 3, 4, 5};
    uint64_t src1 = (static_cast<uint64_t>(rp[0]) << 0) | (static_cast<uint64_t>(rp[1]) << 16) |
                    (static_cast<uint64_t>(rp[2]) << 32) | (static_cast<uint64_t>(rp[3]) << 48);

    {
        auto record1 = CreateRandomUnaryOpRecord();
        record1.repeat = RandInt(2, 0xFF);
        uint64_t config = (record1.repeat & 0xFF) | (uint64_t(0xF) << 8);
        std::vector<uint8_t> memInfo = CreateMemInfo();
        __sanitizer_report_vmrgsort4(memInfo.data(), record1.location.fileNo, record1.location.lineNo, record1.location.pc,
            reinterpret_cast<__ubuf__ half *>(record1.dst), reinterpret_cast<__ubuf__ half *>(record1.src), src1, config);
        auto record = reinterpret_cast<UnaryOpRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
                                                        sizeof(RecordBlockHead) + sizeof(RecordType));
        CheckDstPubParm<RecordType::VMRGSORT4_OP_C220, half>(record, record1);
    }
    {
        auto record1 = CreateRandomUnaryOpRecord();
        record1.repeat = RandInt(2, 0xFF);
        uint64_t config = (record1.repeat & 0xFF) | (uint64_t(0xF) << 8);
        std::vector<uint8_t> memInfo = CreateMemInfo();
        __sanitizer_report_vmrgsort4(memInfo.data(), record1.location.fileNo, record1.location.lineNo, record1.location.pc,
            reinterpret_cast<__ubuf__ float *>(record1.dst), reinterpret_cast<__ubuf__ float *>(record1.src), src1, config);
        auto record = reinterpret_cast<UnaryOpRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
                                                        sizeof(RecordBlockHead) + sizeof(RecordType));
        CheckDstPubParm<RecordType::VMRGSORT4_OP_C220, float>(record, record1);
    }

    {
        auto record1 = CreateRandomUnaryOpRecord();
        record1.repeat = RandInt(2, 0xFF);
        std::vector<uint8_t> memInfo = CreateMemInfo();
        __sanitizer_report_vmrgsort4(memInfo.data(), record1.location.fileNo, record1.location.lineNo, record1.location.pc,
            reinterpret_cast<__ubuf__ half *>(record1.dst), reinterpret_cast<__ubuf__ half *>(record1.src),
            record1.repeat, rp[0], rp[1], rp[2], rp[3], 0, 15);
        auto record = reinterpret_cast<UnaryOpRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
                                                        sizeof(RecordBlockHead) + sizeof(RecordType));
        CheckDstPubParm<RecordType::VMRGSORT4_OP_C220, half>(record, record1);
    }
}
