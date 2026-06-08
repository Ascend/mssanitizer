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

#ifndef PLUGIN_RECORD_SYNC_INSTRUCTIONS_H
#define PLUGIN_RECORD_SYNC_INSTRUCTIONS_H

#include "kernel_pub_func.h"
#include "utils.h"
#include "recorder.h"

namespace Sanitizer {

template<RecordType recordType>
AICORE_FUNC_HEAD void RecordSyncEvent(EXTRA_PARAMS_DEC, pipe_t pipe, pipe_t tpipe, uint64_t eventID)
{
    if (MemInfoIsInvalid(memInfo)) {
        return;
    }

    if (!DoRaceCheck(memInfo) && !DoSyncCheck(memInfo) && !DoInitCheck(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = SyncRecord {};
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.src = static_cast<PipeType>(pipe);
    record.dst = static_cast<PipeType>(tpipe);
    record.eventID = eventID;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<recordType>(record);
}

template<RecordType recordType>
AICORE_FUNC_HEAD void RecordBufEvent(EXTRA_PARAMS_DEC, pipe_t pipe, uint64_t bufId, bool mode)
{
    if (MemInfoIsInvalid(memInfo)) {
        return;
    }

    if (!DoRaceCheck(memInfo) && !DoSyncCheck(memInfo) && !DoInitCheck(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = BufRecord {};
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.pipe = static_cast<PipeType>(pipe);
    record.bufId = bufId;
    record.mode = static_cast<BufMode>(mode);

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<recordType>(record);
}

template<RecordType recordType>
AICORE_FUNC_HEAD void RecordSoftSyncEvent(EXTRA_PARAMS_DEC, int32_t waitBlockIdx, int32_t eventID,
    int32_t usedCores, bool isAIVOnly)
{
    if (MemInfoIsInvalid(memInfo)) {
        return;
    }

    if (!DoRaceCheck(memInfo) && !DoSyncCheck(memInfo) && !DoInitCheck(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = SoftSyncRecord{};
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.waitCoreID = waitBlockIdx;
    record.usedCores = usedCores;
    record.eventID = eventID;
    record.isAIVOnly = isAIVOnly;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<recordType>(record);
}

template<RecordType recordType>
AICORE_FUNC_HEAD void RecordHardSyncEvent(EXTRA_PARAMS_DEC,
                                           pipe_t pipe, pipe_t tpipe, uint64_t eventID, mem_t memory, bool v)
{
    if (MemInfoIsInvalid(memInfo)) {
        return;
    }

    if (!DoRaceCheck(memInfo) && !DoSyncCheck(memInfo) && !DoInitCheck(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = HardSyncRecord {};
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.src = static_cast<PipeType>(pipe);
    record.dst = static_cast<PipeType>(tpipe);
    record.eventID = eventID;
    record.memory = static_cast<MemType>(memory);
    record.v = v;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<recordType>(record);
}

AICORE_FUNC_HEAD void RecordPipeBarrierEvent(EXTRA_PARAMS_DEC, pipe_t pipe)
{
    if (MemInfoIsInvalid(memInfo)) {
        return;
    }

    if (!DoRaceCheck(memInfo) && !DoSyncCheck(memInfo) && !DoInitCheck(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    PipeBarrierRecord record;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.pipe = static_cast<PipeType>(pipe);

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::PIPE_BARRIER>(record);
}

template<RecordType recordType>
AICORE_FUNC_HEAD void RecordFftsSyncEvent(EXTRA_PARAMS_DEC, pipe_t pipe, uint64_t config)
{
    if (MemInfoIsInvalid(memInfo)) {
        return;
    }

    if (!(DoRaceCheck(memInfo)) && !DoSyncCheck(memInfo) && !DoInitCheck(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    FftsSyncRecord record;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.dst = static_cast<PipeType>(pipe);
    constexpr uint64_t modeShift = 4;
    constexpr uint64_t flagIdShift = 8;
    record.mode = (config >> modeShift) & 0x3;
    record.flagID = (config >> flagIdShift) & 0xF;
    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<recordType>(record);
}

AICORE_FUNC_HEAD void RecordWaitFlagDevEvent(EXTRA_PARAMS_DEC, int64_t flagID)
{
    if (MemInfoIsInvalid(memInfo)) {
        return;
    }

    if (!(DoRaceCheck(memInfo)) && !DoSyncCheck(memInfo) && !DoInitCheck(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    WaitFlagDevRecord record;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.flagID = flagID;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::WAIT_FLAG_DEV>(record);
}

template<RecordType recordType>
AICORE_FUNC_HEAD void RecordWaitFlagDevEventWithPipe(EXTRA_PARAMS_DEC, pipe_t pipe, int64_t flagID)
{
    if (MemInfoIsInvalid(memInfo)) {
        return;
    }

    if (!(DoRaceCheck(memInfo)) && !DoSyncCheck(memInfo) && !DoInitCheck(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    WaitFlagDevPipeRecord record;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.flagID = flagID;
    record.pipe = static_cast<PipeType>(pipe);

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<recordType>(record);
}

template<RecordType recordType>
AICORE_FUNC_HEAD void RecordIntraBlockSyncEvent(EXTRA_PARAMS_DEC, pipe_t pipe, uint64_t syncID)
{
    if (MemInfoIsInvalid(memInfo)) {
        return;
    }

    if (!(DoRaceCheck(memInfo)) && !DoSyncCheck(memInfo) && !DoInitCheck(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    IntraBlockSyncRecord record;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.pipe = static_cast<PipeType>(pipe);
    record.syncID = syncID;
    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<recordType>(record);
}

AICORE_FUNC_HEAD void RecordSetAtomicEvent(EXTRA_PARAMS_DEC, AtomicMode mode)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    AtomicModeRecord record;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.mode = mode;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::SET_ATOMIC>(record);
}

}  // namespace Sanitizer

#endif  // PLUGIN_RECORD_SYNC_INSTRUCTIONS_H
