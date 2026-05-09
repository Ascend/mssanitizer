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

#ifndef PLUGIN_RECORD_MSTX_INSTRUCTIONS_H
#define PLUGIN_RECORD_MSTX_INSTRUCTIONS_H

#include "kernel_pub_func.h"
#include "utils.h"
#include "recorder.h"

namespace Sanitizer {

AICORE_FUNC_HEAD void AssignCrossRecord(MstxRecord &rhs, const MstxCrossRecord &lhs)
{
    auto &record = rhs.interface.mstxCrossRecord;
    record.addr = lhs.addr;
    record.flagId = lhs.flagId;
    record.pipe = lhs.pipe;
    record.isMore = lhs.isMore;
    record.isMerge = lhs.isMerge;
}

AICORE_FUNC_HEAD void AssignHcclRecord(MstxRecord &rhs, const MstxHcclRecord &lhs)
{
    auto &record = rhs.interface.mstxHcclRecord;
    record.src = lhs.src;
    record.dst = lhs.dst;
    record.srcCount = lhs.srcCount;
    record.dstCount = lhs.dstCount;
    record.srcStride = lhs.srcStride;
    record.dstStride = lhs.dstStride;
    record.srcRepeatStride = lhs.srcRepeatStride;
    record.dstRepeatStride = lhs.dstRepeatStride;
    record.srcDataTypeSize = lhs.srcDataTypeSize;
    record.dstDataTypeSize = lhs.dstDataTypeSize;
    record.repeat = lhs.repeat;
    record.rankDim = lhs.rankDim;
    record.flagId = lhs.flagId;
}

template<typename RecordT>
AICORE_FUNC_HEAD void RecordMstxPlainRecord(Recorder &recorder, MstxRecord &mstxRecord,
                                             RecordT MstxRecord::Interface::*record,
                                             uint32_t bufferLens, void *buffer)
{
    if (bufferLens != sizeof(RecordT) || buffer == nullptr) {
        mstxRecord.error = true;
        recorder.DumpRecord<RecordType::MSTX_STUB>(mstxRecord);
        return;
    }

    mstxRecord.interface.*record = *static_cast<RecordT *>(buffer);
    recorder.DumpRecord<RecordType::MSTX_STUB>(mstxRecord);
}

AICORE_FUNC_HEAD void RecordMstxCrossRecord(Recorder &recorder, MstxRecord &mstxRecord,
                                             uint32_t bufferLens, void *buffer)
{
    if (bufferLens == sizeof(MstxCrossRecord) && buffer != nullptr) {
        MstxCrossRecord mstxCrossRecord = *reinterpret_cast<MstxCrossRecord*>(buffer);
        AssignCrossRecord(mstxRecord, mstxCrossRecord);
    } else {
        mstxRecord.error = true;
    }
    recorder.DumpRecord<RecordType::MSTX_STUB>(mstxRecord);
}

AICORE_FUNC_HEAD void RecordMstxHcclRecord(Recorder &recorder, MstxRecord &mstxRecord,
                                            uint32_t bufferLens, void *buffer)
{
    if (bufferLens == sizeof(MstxHcclRecord) && buffer != nullptr) {
        // 将4个定长接口的buffer数据按照MstxHcclRecord结构体进行解析
        MstxHcclRecord mstxHcclRecord = *reinterpret_cast<MstxHcclRecord*>(buffer);
        AssignHcclRecord(mstxRecord, mstxHcclRecord);
    } else {
        mstxRecord.error = true;
    }
    recorder.DumpRecord<RecordType::MSTX_STUB>(mstxRecord);
}

AICORE_FUNC_HEAD void RecordMstxHcclVRecord(Recorder &recorder, MstxRecord &mstxRecord,
                                             uint32_t bufferLens, void *buffer)
{
    if (bufferLens == sizeof(MstxHcclRecordV) && buffer != nullptr) {
        // 变长数组需要转换为多个MstxHcclCoreRecord
        MstxHcclRecordV mstxHcclRecordV = *reinterpret_cast<MstxHcclRecordV*>(buffer);
        for (int32_t i = 0; i < mstxHcclRecordV.rankDim; ++i) {
            auto &record = mstxRecord.interface.mstxHcclCoreRecord;
            record.src = mstxHcclRecordV.src + mstxHcclRecordV.srcStride[i] * mstxHcclRecordV.srcDataTypeSize;
            record.dst = mstxHcclRecordV.dst + mstxHcclRecordV.dstStride[i] * mstxHcclRecordV.dstDataTypeSize;
            record.srclenBurst = mstxHcclRecordV.srcCount[i] * mstxHcclRecordV.srcDataTypeSize;
            record.dstlenBurst = mstxHcclRecordV.dstCount[i] * mstxHcclRecordV.dstDataTypeSize;
            record.repeat = mstxHcclRecordV.repeat;
            recorder.DumpRecord<RecordType::MSTX_STUB>(mstxRecord);
        }
    } else {
        mstxRecord.error = true;
        recorder.DumpRecord<RecordType::MSTX_STUB>(mstxRecord);
    }
}

template<typename RecordT>
AICORE_FUNC_HEAD void ProcessMstxVecMask(Recorder &recorder, uint32_t bufferLens, void *buffer)
{
    if (bufferLens != sizeof(RecordT) || buffer == nullptr) {
        return;
    }

    RecordT &record = *static_cast<RecordT *>(buffer);
    if (static_cast<uint8_t>(record.wrapper.maskMode) == 0xFF) {
        recorder.GetRegister(&Register::maskMode, record.wrapper.maskMode);
    }
    if (!record.wrapper.useMask) {
        recorder.GetRegister(&Register::vectorMask0, record.wrapper.mask.mask0);
        recorder.GetRegister(&Register::vectorMask1, record.wrapper.mask.mask1);
    }
}

AICORE_FUNC_HEAD void RecordMstxEvent(EXTRA_PARAMS_DEC, uint32_t interfaceId, uint32_t bufferLens, void *buffer)
{
    if (InvalidMemInfo(memInfo)) { return;}

    uint64_t blockIdx = GetBlockIdx();
    Recorder recorder(memInfo, blockIdx);
    if (interfaceId == static_cast<uint32_t>(InterfaceType::MSTX_FUSE_SCOPE_START)) {
        recorder.SetMstxFuseScope(true);
        return;
    } else if (interfaceId == static_cast<uint32_t>(InterfaceType::MSTX_FUSE_SCOPE_END)) {
        recorder.SetMstxFuseScope(false);
        return;
    }

    auto mstxRecord  = MstxRecord{};
    mstxRecord.location.fileNo = fileNo;
    mstxRecord.location.lineNo = lineNo;
    mstxRecord.location.pc = static_cast<uint64_t>(pc);
    mstxRecord.interfaceType = static_cast<InterfaceType>(interfaceId);
    mstxRecord.location.blockId = blockIdx;
    mstxRecord.bufferLens = bufferLens;

    if (interfaceId == static_cast<uint32_t>(InterfaceType::MSTX_SET_CROSS_SYNC) ||
        interfaceId == static_cast<uint32_t>(InterfaceType::MSTX_WAIT_CROSS_SYNC)) {
        /// 该情况下为同步指令，只有开启竞争检测时，才会记录；
        if (!DoRaceCheck(memInfo)) { return; }
        RecordMstxCrossRecord(recorder, mstxRecord, bufferLens, buffer);
    } else if (interfaceId == static_cast<uint32_t>(InterfaceType::MSTX_HCCL)) {
        RecordMstxHcclRecord(recorder, mstxRecord, bufferLens, buffer);
    } else if (interfaceId == static_cast<uint32_t>(InterfaceType::MSTX_HCCLV)) {
        RecordMstxHcclVRecord(recorder, mstxRecord, bufferLens, buffer);
    } else if (static_cast<InterfaceType>(interfaceId) == InterfaceType::MSTX_VEC_UNARY_OP) {
        ProcessMstxVecMask<MstxVecUnaryDesc>(recorder, bufferLens, buffer);
        RecordMstxPlainRecord(recorder, mstxRecord, &MstxRecord::Interface::mstxVecUnaryDesc, bufferLens, buffer);
    } else if (static_cast<InterfaceType>(interfaceId) == InterfaceType::MSTX_VEC_BINARY_OP) {
        ProcessMstxVecMask<MstxVecBinaryDesc>(recorder, bufferLens, buffer);
        RecordMstxPlainRecord(recorder, mstxRecord, &MstxRecord::Interface::mstxVecBinaryDesc, bufferLens, buffer);
    } else if (static_cast<InterfaceType>(interfaceId) == InterfaceType::MSTX_DATA_COPY) {
        RecordMstxPlainRecord(recorder, mstxRecord, &MstxRecord::Interface::mstxDataCopyDesc, bufferLens, buffer);
    } else if (static_cast<InterfaceType>(interfaceId) == InterfaceType::MSTX_DATA_COPY_PAD) {
        RecordMstxPlainRecord(recorder, mstxRecord, &MstxRecord::Interface::mstxDataCopyPadDesc, bufferLens, buffer);
    } else if (static_cast<InterfaceType>(interfaceId) == InterfaceType::MSTX_CROSS_CORE_BARRIER) {
        RecordMstxPlainRecord(recorder, mstxRecord, &MstxRecord::Interface::mstxCrossCoreBarrier, bufferLens, buffer);
    } else if (static_cast<InterfaceType>(interfaceId) == InterfaceType::MSTX_CROSS_CORE_SET_FLAG) {
        RecordMstxPlainRecord(recorder, mstxRecord, &MstxRecord::Interface::mstxCrossCoreSetFlag, bufferLens, buffer);
    } else if (static_cast<InterfaceType>(interfaceId) == InterfaceType::MSTX_CROSS_CORE_WAIT_FLAG) {
        RecordMstxPlainRecord(recorder, mstxRecord, &MstxRecord::Interface::mstxCrossCoreWaitFlag, bufferLens, buffer);
    } else if (static_cast<InterfaceType>(interfaceId) == InterfaceType::MSTX_SIGNAL_SET) {
        RecordMstxPlainRecord(recorder, mstxRecord, &MstxRecord::Interface::mstxSignalSet, bufferLens, buffer);
    } else if (static_cast<InterfaceType>(interfaceId) == InterfaceType::MSTX_SIGNAL_WAIT) {
        RecordMstxPlainRecord(recorder, mstxRecord, &MstxRecord::Interface::mstxSignalWait, bufferLens, buffer);
    } else if (static_cast<InterfaceType>(interfaceId) == InterfaceType::MSTX_CROSS_NPU_BARRIER) {
        RecordMstxPlainRecord(recorder, mstxRecord, &MstxRecord::Interface::mstxCrossNpuBarrier, bufferLens, buffer);
    } else {
        mstxRecord.error = true;
        recorder.DumpRecord<RecordType::MSTX_STUB>(mstxRecord);
    }
}

}  // namespace Sanitizer

#endif  // PLUGIN_RECORD_MSTX_INSTRUCTIONS_H
