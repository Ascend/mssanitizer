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
#include <cstring>
#include <tuple>

#include "ccec_defs.h"
#include "data_process.h"
#include "plugin/ccec/mstx_instructions.cpp"

using namespace Sanitizer;

namespace SanitizerTest {

TEST(MstxInstructions, dump_cross_records_expect_get_correct_records)
{
    MstxCrossRecord crossRecord1 = {
        .addr = 0x300,
        .flagId = 2,
        .pipe = PipeType::PIPE_V,
        .isMore = false,
        .isMerge = true,
    };

    MstxRecord record = {
        .interfaceType      = InterfaceType::MSTX_SET_CROSS_SYNC,
        .bufferLens         = sizeof(MstxCrossRecord),
        .location           = {10, 10, 0x200},
        .error              = false,
    };

    record.interface.mstxCrossRecord = crossRecord1;
    std::vector<uint8_t> memInfo(UT_RECORD_BUF_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report___mstx_dfx_report_stub(memInfo.data(), record.location.fileNo, record.location.lineNo,
        record.location.pc, static_cast<uint32_t>(record.interfaceType), record.bufferLens, &crossRecord1);

    MstxCrossRecord crossRecord2 = {
        .addr = 0x500,
        .flagId = 6,
        .pipe = PipeType::PIPE_MTE2,
        .isMore = true,
        .isMerge = false,
    };

    __sanitizer_report___mstx_dfx_report_stub(memInfo.data(), record.location.fileNo, record.location.lineNo,
        record.location.pc, static_cast<uint32_t>(record.interfaceType), record.bufferLens, &crossRecord2);

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::MSTX_STUB>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(MstxRecord);
    record.interface.mstxCrossRecord = crossRecord2;
    ASSERT_TRUE(CheckRecordEqual<RecordType::MSTX_STUB>(ptr, record));
}

TEST(MstxInstructions, dump_cross_record_with_error_buffer_size_expect_get_error)
{
    MstxCrossRecord crossRecord = {
        .addr = 0x300,
        .flagId = 2,
        .pipe = PipeType::PIPE_V,
        .isMore = false,
        .isMerge = true,
    };

    MstxRecord record = {
        .interfaceType      = InterfaceType::MSTX_WAIT_CROSS_SYNC,
        .bufferLens         = sizeof(MstxCrossRecord),
        .location           = {10, 10, 0x200},
        .error              = false,
    };
    record.interface.mstxCrossRecord = crossRecord;
    std::vector<uint8_t> memInfo(UT_RECORD_BUF_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report___mstx_dfx_report_stub(memInfo.data(), record.location.fileNo, record.location.lineNo,
        record.location.pc, static_cast<uint32_t>(record.interfaceType), record.bufferLens + 1, &crossRecord);

    const MstxRecord mstxRecord = *reinterpret_cast<const MstxRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType));
    ASSERT_TRUE(mstxRecord.error);
}

TEST(MstxInstructions, dump_cross_record_with_nullptr_expect_get_error)
{
    MstxRecord record = {
        .interfaceType      = InterfaceType::MSTX_WAIT_CROSS_SYNC,
        .bufferLens         = sizeof(MstxCrossRecord),
        .location           = {10, 10, 0x200},
        .error              = false,
    };
    std::vector<uint8_t> memInfo(UT_RECORD_BUF_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report___mstx_dfx_report_stub(memInfo.data(), record.location.fileNo, record.location.lineNo,
        record.location.pc, static_cast<uint32_t>(record.interfaceType), record.bufferLens, nullptr);

    const MstxRecord mstxRecord = *reinterpret_cast<const MstxRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType));
    ASSERT_TRUE(mstxRecord.error);
}

TEST(MstxInstructions, dump_cross_record_with_illegal_interface_type_expect_get_error)
{
    MstxRecord record = {
        .interfaceType      = InterfaceType::MSTX_WAIT_CROSS_SYNC,
        .bufferLens         = sizeof(MstxCrossRecord),
        .location           = {10, 10, 0x200},
        .error              = false,
    };
    std::vector<uint8_t> memInfo(UT_RECORD_BUF_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.racecheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report___mstx_dfx_report_stub(memInfo.data(), record.location.fileNo, record.location.lineNo,
        record.location.pc, -1, record.bufferLens, nullptr);

    const MstxRecord mstxRecord = *reinterpret_cast<const MstxRecord *>(memInfo.data() + sizeof(RecordGlobalHead) +
        sizeof(RecordBlockHead) + sizeof(RecordType));
    ASSERT_TRUE(mstxRecord.error);
}

TEST(MstxInstructions, dump_cross_record_with_initcheck_expect_get_none)
{
    MstxRecord record = {
        .interfaceType      = InterfaceType::MSTX_WAIT_CROSS_SYNC,
        .bufferLens         = sizeof(MstxCrossRecord),
        .location           = {10, 10, 0x200},
        .error              = false,
    };
    std::vector<uint8_t> memInfo(UT_RECORD_BUF_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.initcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report___mstx_dfx_report_stub(memInfo.data(), record.location.fileNo, record.location.lineNo,
        record.location.pc, static_cast<uint32_t>(record.interfaceType), record.bufferLens, nullptr);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);
}

TEST(MstxInstructions, dump_cross_record_with_memcheck_expect_get_none)
{
    MstxRecord record = {
        .interfaceType      = InterfaceType::MSTX_WAIT_CROSS_SYNC,
        .bufferLens         = sizeof(MstxCrossRecord),
        .location           = {10, 10, 0x200},
        .error              = false,
    };
    std::vector<uint8_t> memInfo = CreateMemInfo();
    __sanitizer_report___mstx_dfx_report_stub(memInfo.data(), record.location.fileNo, record.location.lineNo,
        record.location.pc, static_cast<uint32_t>(record.interfaceType), record.bufferLens, nullptr);
    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);
}

}

namespace {

MstxRecord GetMstxRecord(std::vector<uint8_t> const &memInfo, size_t index = 0)
{
    const size_t offset = sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) +
        index * (sizeof(RecordType) + sizeof(MstxRecord));
    RecordType type{};
    std::memcpy(&type, memInfo.data() + offset, sizeof(type));
    EXPECT_EQ(type, RecordType::MSTX_STUB);
    MstxRecord record{};
    std::memcpy(&record, memInfo.data() + offset + sizeof(type), sizeof(record));
    return record;
}

// Compare named fields rather than unspecified padding bytes.
auto Fields(const MstxTensorDesc &r) { return std::make_tuple(r.space, r.addr, r.size, r.dataBits); }
auto Fields(const MstxVecWrapper &r)
{
    return std::make_tuple(r.maskMode, r.mask.mask0, r.mask.mask1, r.reserveBufSize, r.useMask);
}
auto Fields(const MstxVecUnaryDesc &r)
{
    return std::make_tuple(Fields(r.dst), Fields(r.src), Fields(r.wrapper), r.blockNum, r.dstBlockStride,
        r.srcBlockStride, r.repeatTimes, r.dstRepeatStride, r.srcRepeatStride, std::string(r.name, sizeof(r.name)));
}
auto Fields(const MstxVecBinaryDesc &r)
{
    return std::make_tuple(Fields(r.dst), Fields(r.src0), Fields(r.src1), Fields(r.wrapper), r.blockNum,
        r.dstBlockStride, r.src0BlockStride, r.src1BlockStride, r.repeatTimes, r.dstRepeatStride,
        r.src0RepeatStride, r.src1RepeatStride, std::string(r.name, sizeof(r.name)));
}
template <typename RecordT>
auto CopyFields(const RecordT &r)
{
    return std::make_tuple(Fields(r.dst), Fields(r.src), r.lenBurst, r.nBurst, r.srcGap, r.dstGap,
        std::string(r.name, sizeof(r.name)));
}
auto Fields(const MstxDataCopyDesc &r) { return CopyFields(r); }
auto Fields(const MstxDataCopyPadDesc &r)
{
    return std::make_tuple(CopyFields(r), r.leftPad, r.rightPad);
}
auto Fields(const MstxDataCopyPadV2Desc &r)
{
    return std::make_tuple(CopyFields(r), r.padMode, r.leftPad, r.rightPad);
}
auto Fields(const MstxCrossCoreBarrier &r)
{
    return std::make_tuple(r.usedCoreNum, r.usedCoreId, r.isAIVOnly, r.pipeBarrierAll);
}
auto Fields(const MstxCrossCoreSetFlag &r) { return std::make_tuple(r.eventId, r.peerCoreId, r.pipeBarrierAll); }
auto Fields(const MstxCrossCoreWaitFlag &r) { return std::make_tuple(r.eventId, r.peerCoreId, r.pipeBarrierAll); }
auto Fields(const MstxSignalSet &r) { return std::make_tuple(r.addr, r.value); }
auto Fields(const MstxSignalWait &r) { return std::make_tuple(r.addr, r.cmpValue, r.cmpOp); }
auto Fields(const MstxCrossNpuBarrier &r)
{
    return std::make_tuple(r.usedDeviceNum, r.usedDeviceId, r.usedCoreNum, r.usedCoreId, r.isAIVOnly, r.pipeBarrierAll);
}

template <typename RecordT>
void ExpectPlainRecord(InterfaceType interfaceType, RecordT input, RecordT MstxRecord::Interface::*member,
    const RecordT &expected)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    auto head = reinterpret_cast<RecordGlobalHead *>(memInfo.data());
    head->registers[0].maskMode = MaskMode::MASK_COUNT;
    head->registers[0].vectorMask0 = 0x55;
    head->registers[0].vectorMask1 = 0xAA;
    __sanitizer_report___mstx_dfx_report_stub(memInfo.data(), 10, 20, 0x200, static_cast<uint32_t>(interfaceType),
        static_cast<uint32_t>(sizeof(RecordT)), &input);
    const auto record = GetMstxRecord(memInfo);
    EXPECT_FALSE(record.error);
    EXPECT_EQ(record.interfaceType, interfaceType);
    EXPECT_EQ(record.bufferLens, sizeof(RecordT));
    EXPECT_EQ(record.location.fileNo, 10U);
    EXPECT_EQ(record.location.lineNo, 20U);
    EXPECT_EQ(record.location.pc, 0x200U);
    EXPECT_EQ(Fields(record.interface.*member), Fields(expected));
}

template <typename RecordT>
void ExpectPlainRecord(InterfaceType interfaceType, RecordT input, RecordT MstxRecord::Interface::*member)
{
    ExpectPlainRecord(interfaceType, input, member, input);
}

} // namespace

TEST(MstxInstructions, dump_hccl_records_expect_success_and_error_buffer)
{
    MstxHcclRecord hcclRecord{};
    hcclRecord.src = 0x1000;
    hcclRecord.dst = 0x2000;
    hcclRecord.srcCount = 8;
    hcclRecord.dstCount = 16;
    hcclRecord.srcStride = 2;
    hcclRecord.dstStride = 3;
    hcclRecord.srcRepeatStride = 4;
    hcclRecord.dstRepeatStride = 5;
    hcclRecord.srcDataTypeSize = 1;
    hcclRecord.dstDataTypeSize = 2;
    hcclRecord.repeat = 6;
    hcclRecord.rankDim = 2;
    hcclRecord.flagId = static_cast<int32_t>(HcclFlagId::ALLTOALL);

    std::vector<uint8_t> memInfo = CreateMemInfo();
    __sanitizer_report___mstx_dfx_report_stub(memInfo.data(), 10, 20, 0x200,
        static_cast<uint32_t>(InterfaceType::MSTX_HCCL), sizeof(hcclRecord), &hcclRecord);
    const auto record = GetMstxRecord(memInfo);
    EXPECT_FALSE(record.error);
    EXPECT_EQ(record.interfaceType, InterfaceType::MSTX_HCCL);
    EXPECT_EQ(record.interface.mstxHcclRecord.src, hcclRecord.src);
    EXPECT_EQ(record.interface.mstxHcclRecord.dst, hcclRecord.dst);
    EXPECT_EQ(record.interface.mstxHcclRecord.srcCount, hcclRecord.srcCount);
    EXPECT_EQ(record.interface.mstxHcclRecord.dstCount, hcclRecord.dstCount);
    EXPECT_EQ(record.interface.mstxHcclRecord.srcStride, hcclRecord.srcStride);
    EXPECT_EQ(record.interface.mstxHcclRecord.dstStride, hcclRecord.dstStride);
    EXPECT_EQ(record.interface.mstxHcclRecord.srcRepeatStride, hcclRecord.srcRepeatStride);
    EXPECT_EQ(record.interface.mstxHcclRecord.dstRepeatStride, hcclRecord.dstRepeatStride);
    EXPECT_EQ(record.interface.mstxHcclRecord.srcDataTypeSize, hcclRecord.srcDataTypeSize);
    EXPECT_EQ(record.interface.mstxHcclRecord.dstDataTypeSize, hcclRecord.dstDataTypeSize);
    EXPECT_EQ(record.interface.mstxHcclRecord.repeat, hcclRecord.repeat);
    EXPECT_EQ(record.interface.mstxHcclRecord.rankDim, hcclRecord.rankDim);
    EXPECT_EQ(record.interface.mstxHcclRecord.flagId, hcclRecord.flagId);

    std::vector<uint8_t> sizeErrorMemInfo = CreateMemInfo();
    __sanitizer_report___mstx_dfx_report_stub(sizeErrorMemInfo.data(), 10, 20, 0x200,
        static_cast<uint32_t>(InterfaceType::MSTX_HCCL), sizeof(hcclRecord) + 1, &hcclRecord);
    EXPECT_TRUE(GetMstxRecord(sizeErrorMemInfo).error);

    std::vector<uint8_t> nullErrorMemInfo = CreateMemInfo();
    __sanitizer_report___mstx_dfx_report_stub(nullErrorMemInfo.data(), 10, 20, 0x200,
        static_cast<uint32_t>(InterfaceType::MSTX_HCCL), sizeof(hcclRecord), nullptr);
    EXPECT_TRUE(GetMstxRecord(nullErrorMemInfo).error);
}

TEST(MstxInstructions, dump_hccl_v_records_expect_multiple_core_records)
{
    std::vector<uint64_t> srcCount = {10, 20};
    std::vector<uint64_t> dstCount = {30, 40};
    std::vector<uint64_t> srcStride = {1, 2};
    std::vector<uint64_t> dstStride = {3, 4};
    MstxHcclRecordV hcclRecordV{};
    hcclRecordV.src = 0x1000;
    hcclRecordV.dst = 0x2000;
    hcclRecordV.srcCount = srcCount.data();
    hcclRecordV.dstCount = dstCount.data();
    hcclRecordV.srcStride = srcStride.data();
    hcclRecordV.dstStride = dstStride.data();
    hcclRecordV.srcRepeatStride = 5;
    hcclRecordV.dstRepeatStride = 6;
    hcclRecordV.srcDataTypeSize = 4;
    hcclRecordV.dstDataTypeSize = 2;
    hcclRecordV.repeat = 7;
    hcclRecordV.rankDim = 2;
    hcclRecordV.flagId = 0;

    std::vector<uint8_t> memInfo = CreateMemInfo();
    __sanitizer_report___mstx_dfx_report_stub(memInfo.data(), 10, 20, 0x200,
        static_cast<uint32_t>(InterfaceType::MSTX_HCCLV), sizeof(hcclRecordV), &hcclRecordV);
    RecordBlockHead blockHead{};
    std::memcpy(&blockHead, memInfo.data() + sizeof(RecordGlobalHead), sizeof(blockHead));
    ASSERT_EQ(blockHead.recordWriteCount, hcclRecordV.rankDim);
    for (size_t i = 0; i < static_cast<size_t>(hcclRecordV.rankDim); ++i) {
        SCOPED_TRACE(i);
        const auto record = GetMstxRecord(memInfo, i);
        EXPECT_FALSE(record.error);
        EXPECT_EQ(record.interfaceType, InterfaceType::MSTX_HCCLV);
        EXPECT_EQ(record.interface.mstxHcclCoreRecord.src, hcclRecordV.src + srcStride[i] * 4);
        EXPECT_EQ(record.interface.mstxHcclCoreRecord.dst, hcclRecordV.dst + dstStride[i] * 2);
        EXPECT_EQ(record.interface.mstxHcclCoreRecord.srclenBurst, srcCount[i] * 4);
        EXPECT_EQ(record.interface.mstxHcclCoreRecord.dstlenBurst, dstCount[i] * 2);
        EXPECT_EQ(record.interface.mstxHcclCoreRecord.repeat, hcclRecordV.repeat);
    }

    std::vector<uint8_t> errorMemInfo = CreateMemInfo();
    __sanitizer_report___mstx_dfx_report_stub(errorMemInfo.data(), 10, 20, 0x200,
        static_cast<uint32_t>(InterfaceType::MSTX_HCCLV), sizeof(hcclRecordV) - 1, &hcclRecordV);
    EXPECT_TRUE(GetMstxRecord(errorMemInfo).error);
}

TEST(MstxInstructions, dump_vec_and_plain_records_expect_success)
{
    MstxVecUnaryDesc unaryDesc{};
    unaryDesc.dst = {AddressSpace::GM, 0x1000, 128, 16};
    unaryDesc.src = {AddressSpace::UB, 0x2000, 128, 16};
    unaryDesc.wrapper.maskMode = static_cast<MaskMode>(0xFF);
    unaryDesc.wrapper.useMask = false;
    unaryDesc.wrapper.mask.mask0 = 0x11;
    unaryDesc.wrapper.mask.mask1 = 0x22;
    auto expectedUnary = unaryDesc;
    expectedUnary.wrapper.maskMode = MaskMode::MASK_COUNT;
    expectedUnary.wrapper.mask = {0x55, 0xAA};
    ExpectPlainRecord(InterfaceType::MSTX_VEC_UNARY_OP, unaryDesc,
        &MstxRecord::Interface::mstxVecUnaryDesc, expectedUnary);

    MstxVecBinaryDesc binaryDesc{};
    binaryDesc.dst = {AddressSpace::GM, 0x3000, 256, 32};
    binaryDesc.src0 = {AddressSpace::UB, 0x4000, 256, 32};
    binaryDesc.src1 = {AddressSpace::UB, 0x5000, 256, 32};
    binaryDesc.wrapper.maskMode = MaskMode::MASK_NORM;
    binaryDesc.wrapper.useMask = true;
    binaryDesc.wrapper.mask = {0x33, 0x44};
    ExpectPlainRecord(InterfaceType::MSTX_VEC_BINARY_OP, binaryDesc, &MstxRecord::Interface::mstxVecBinaryDesc);
    binaryDesc.wrapper.maskMode = static_cast<MaskMode>(0xFF);
    binaryDesc.wrapper.useMask = false;
    auto expectedBinary = binaryDesc;
    expectedBinary.wrapper.maskMode = MaskMode::MASK_COUNT;
    expectedBinary.wrapper.mask = {0x55, 0xAA};
    ExpectPlainRecord(InterfaceType::MSTX_VEC_BINARY_OP, binaryDesc,
        &MstxRecord::Interface::mstxVecBinaryDesc, expectedBinary);

    MstxDataCopyDesc dataCopyDesc{};
    dataCopyDesc.dst = {AddressSpace::GM, 0x6000, 64, 8};
    dataCopyDesc.src = {AddressSpace::UB, 0x7000, 64, 8};
    dataCopyDesc.lenBurst = 8;
    dataCopyDesc.nBurst = 2;
    ExpectPlainRecord(InterfaceType::MSTX_DATA_COPY, dataCopyDesc, &MstxRecord::Interface::mstxDataCopyDesc);

    MstxDataCopyPadDesc dataCopyPadDesc{};
    dataCopyPadDesc.lenBurst = 16;
    dataCopyPadDesc.nBurst = 2;
    dataCopyPadDesc.leftPad = 1;
    dataCopyPadDesc.rightPad = 1;
    ExpectPlainRecord(InterfaceType::MSTX_DATA_COPY_PAD, dataCopyPadDesc, &MstxRecord::Interface::mstxDataCopyPadDesc);

    MstxDataCopyPadV2Desc dataCopyPadV2Desc{};
    dataCopyPadV2Desc.padMode = PaddingMode::COMPACT;
    dataCopyPadV2Desc.lenBurst = 32;
    dataCopyPadV2Desc.nBurst = 4;
    ExpectPlainRecord(InterfaceType::MSTX_DATA_COPY_PAD_V2, dataCopyPadV2Desc,
        &MstxRecord::Interface::mstxDataCopyPadV2Desc);

    MstxCrossCoreBarrier crossCoreBarrier{};
    crossCoreBarrier.usedCoreNum = 2;
    crossCoreBarrier.pipeBarrierAll = true;
    ExpectPlainRecord(InterfaceType::MSTX_CROSS_CORE_BARRIER, crossCoreBarrier,
        &MstxRecord::Interface::mstxCrossCoreBarrier);

    MstxCrossCoreSetFlag crossCoreSetFlag{};
    crossCoreSetFlag.eventId = 1;
    crossCoreSetFlag.peerCoreId = 2;
    ExpectPlainRecord(InterfaceType::MSTX_CROSS_CORE_SET_FLAG, crossCoreSetFlag,
        &MstxRecord::Interface::mstxCrossCoreSetFlag);

    MstxCrossCoreWaitFlag crossCoreWaitFlag{};
    crossCoreWaitFlag.eventId = 1;
    crossCoreWaitFlag.peerCoreId = 2;
    ExpectPlainRecord(InterfaceType::MSTX_CROSS_CORE_WAIT_FLAG, crossCoreWaitFlag,
        &MstxRecord::Interface::mstxCrossCoreWaitFlag);

    MstxSignalSet signalSet{};
    signalSet.addr = 0x8000;
    signalSet.value = 1;
    ExpectPlainRecord(InterfaceType::MSTX_SIGNAL_SET, signalSet, &MstxRecord::Interface::mstxSignalSet);

    MstxSignalWait signalWait{};
    signalWait.addr = 0x8000;
    signalWait.cmpValue = 1;
    signalWait.cmpOp = CompareOp::EQ;
    ExpectPlainRecord(InterfaceType::MSTX_SIGNAL_WAIT, signalWait, &MstxRecord::Interface::mstxSignalWait);

    MstxCrossNpuBarrier crossNpuBarrier{};
    crossNpuBarrier.usedDeviceNum = 2;
    crossNpuBarrier.usedCoreNum = 2;
    crossNpuBarrier.pipeBarrierAll = true;
    ExpectPlainRecord(InterfaceType::MSTX_CROSS_NPU_BARRIER, crossNpuBarrier,
        &MstxRecord::Interface::mstxCrossNpuBarrier);
}

TEST(MstxInstructions, fuse_scope_start_and_end_expect_depth_changed)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    auto blockHead = reinterpret_cast<RecordBlockHead *>(memInfo.data() + sizeof(RecordGlobalHead));
    __sanitizer_report___mstx_dfx_report_stub(memInfo.data(), 10, 20, 0x200,
        static_cast<uint32_t>(InterfaceType::MSTX_FUSE_SCOPE_START), 0, nullptr);
    EXPECT_EQ(blockHead->mstxFuseScopeDepth, 1);
    __sanitizer_report___mstx_dfx_report_stub(memInfo.data(), 10, 20, 0x200,
        static_cast<uint32_t>(InterfaceType::MSTX_FUSE_SCOPE_END), 0, nullptr);
    EXPECT_EQ(blockHead->mstxFuseScopeDepth, 0);
    __sanitizer_report___mstx_dfx_report_stub(memInfo.data(), 10, 20, 0x200,
        static_cast<uint32_t>(InterfaceType::MSTX_FUSE_SCOPE_END), 0, nullptr);
    EXPECT_EQ(blockHead->mstxFuseScopeDepth, 0);

    auto globalHead = reinterpret_cast<RecordGlobalHead *>(memInfo.data());
    globalHead->checkParms.dcciCheck = true;
    __sanitizer_report___mstx_dfx_report_stub(memInfo.data(), 10, 20, 0x200,
        static_cast<uint32_t>(InterfaceType::MSTX_FUSE_SCOPE_START), 0, nullptr);
    EXPECT_EQ(blockHead->mstxFuseScopeDepth, 0);
    blockHead->mstxFuseScopeDepth = 2;
    __sanitizer_report___mstx_dfx_report_stub(memInfo.data(), 10, 20, 0x200,
        static_cast<uint32_t>(InterfaceType::MSTX_FUSE_SCOPE_END), 0, nullptr);
    EXPECT_EQ(blockHead->mstxFuseScopeDepth, 2);
}

TEST(MstxInstructions, dump_plain_record_with_invalid_buffer_expect_error)
{
    MstxDataCopyDesc dataCopyDesc{};
    std::vector<uint8_t> sizeErrorMemInfo = CreateMemInfo();
    __sanitizer_report___mstx_dfx_report_stub(sizeErrorMemInfo.data(), 10, 20, 0x200,
        static_cast<uint32_t>(InterfaceType::MSTX_DATA_COPY), sizeof(dataCopyDesc) + 1, &dataCopyDesc);
    EXPECT_TRUE(GetMstxRecord(sizeErrorMemInfo).error);

    std::vector<uint8_t> nullErrorMemInfo = CreateMemInfo();
    __sanitizer_report___mstx_dfx_report_stub(nullErrorMemInfo.data(), 10, 20, 0x200,
        static_cast<uint32_t>(InterfaceType::MSTX_DATA_COPY), sizeof(dataCopyDesc), nullptr);
    EXPECT_TRUE(GetMstxRecord(nullErrorMemInfo).error);

    MstxVecUnaryDesc unaryDesc{};
    std::vector<uint8_t> vecErrorMemInfo = CreateMemInfo();
    __sanitizer_report___mstx_dfx_report_stub(vecErrorMemInfo.data(), 10, 20, 0x200,
        static_cast<uint32_t>(InterfaceType::MSTX_VEC_UNARY_OP), sizeof(unaryDesc) - 1, &unaryDesc);
    EXPECT_TRUE(GetMstxRecord(vecErrorMemInfo).error);
}
