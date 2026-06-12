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

#ifndef PLUGIN_RECORD_SIMT_INSTRUCTIONS_H
#define PLUGIN_RECORD_SIMT_INSTRUCTIONS_H

#include <utility>
#include "kernel_pub_func.h"
#include "utils.h"
#include "recorder.h"

namespace Sanitizer {

template<RecordType recordType, DetailedDataType detailedDataType>
AICORE_FUNC_HEAD void SimtRecordLoadStoreEvent(EXTRA_PARAMS_DEC, AddressSpace space, uint64_t addr,
    int64_t offset)
{
    if (MemInfoIsInvalid(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    uint8_t dataType = GetDataBits(detailedDataType) / 8;
    SimtLoadStoreRecord record{};
    record.addr = addr + dataType * offset;
    record.size = dataType;
    record.location.blockId = blockIdx;
    record.space = space;
    record.detailedDataType = detailedDataType;

#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    record.threadLoc.idX = GetThreadIdX();
    record.threadLoc.idY = GetThreadIdY();
    record.threadLoc.idZ = GetThreadIdZ();
#endif

#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    Recorder recorder(memInfo, blockIdx);
    recorder.Check<recordType>(record);
}

template<RecordType recordType, DetailedDataType detailedDataType>
AICORE_FUNC_HEAD void SimtRecordAtomEvent(EXTRA_PARAMS_DEC, AddressSpace space, uint64_t addr,
    SimtAtomMode option)
{
    if (MemInfoIsInvalid(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    uint8_t dataType = GetDataBits(detailedDataType) / 8;
    SimtAtomRecord record{};
    record.addr = addr;
    record.size = dataType;
    record.location.blockId = blockIdx;
    record.space = space;
    record.detailedDataType = detailedDataType;
    record.option = option;

#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    record.threadLoc.idX = GetThreadIdX();
    record.threadLoc.idY = GetThreadIdY();
    record.threadLoc.idZ = GetThreadIdZ();
#endif

#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    Recorder recorder(memInfo, blockIdx);
    recorder.Check<recordType>(record);
}

template<RecordType recordType>
AICORE_FUNC_HEAD void SimtRecordSyncEvent(EXTRA_PARAMS_DEC)
{
    if (MemInfoIsInvalid(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    SimtEmptyRecord record{};
    record.location.blockId = blockIdx;

#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    record.threadLoc.idX = GetThreadIdX();
    record.threadLoc.idY = GetThreadIdY();
    record.threadLoc.idZ = GetThreadIdZ();
#endif

#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    Recorder recorder(memInfo, blockIdx);
    // 保留recordType和record的目的是为了指令落盘，便于后续定位问题
    recorder.UpdateSyncThreadCount<recordType>(record);
    recorder.Check<recordType>(record);
}

template<RecordType recordType>
AICORE_FUNC_HEAD void SimtRecordEmptyEvent(EXTRA_PARAMS_DEC)
{
    if (MemInfoIsInvalid(memInfo)) return;
}

template<>
AICORE_FUNC_HEAD void SimtRecordEmptyEvent<RecordType::SIMT_END>(EXTRA_PARAMS_DEC)
{
    if (MemInfoIsInvalid(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    SimtEmptyRecord record{};
    record.location.blockId = blockIdx;

#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
    record.threadLoc.idX = GetThreadIdX();
    record.threadLoc.idY = GetThreadIdY();
    record.threadLoc.idZ = GetThreadIdZ();
#endif

#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    Recorder recorder(memInfo, blockIdx);
    recorder.CopyShadowMemoryToMemInfo(record);
    recorder.Check<RecordType::SIMT_END>(record);
}

}
#endif
