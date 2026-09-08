#include "core/framework/record_defs.h"
#include "core/framework/arch_def.h"
#include "plugin/utils.h"
#include "plugin/record_sync_instructions.h"
using namespace Sanitizer;

SANITIZER_REPORT(set_flag, pipe_t pipe, pipe_t tpipe, uint64_t eventID)
{
    RecordSyncEvent<RecordType::SET_FLAG>(EXTRA_PARAMS, pipe, tpipe, eventID);
}

SANITIZER_REPORT(set_flagi, pipe_t pipe, pipe_t tpipe, uint64_t eventID)
{
    RecordSyncEvent<RecordType::SET_FLAGI>(EXTRA_PARAMS, pipe, tpipe, eventID);
}

SANITIZER_REPORT(wait_flag, pipe_t pipe, pipe_t tpipe, uint64_t eventID)
{
    RecordSyncEvent<RecordType::WAIT_FLAG>(EXTRA_PARAMS, pipe, tpipe, eventID);
}

SANITIZER_REPORT(wait_flagi, pipe_t pipe, pipe_t tpipe, uint64_t eventID)
{
    RecordSyncEvent<RecordType::WAIT_FLAGI>(EXTRA_PARAMS, pipe, tpipe, eventID);
}

SANITIZER_REPORT(get_buf, pipe_t pipe, uint8_t bufId, bool mode)
{
    RecordBufEvent<RecordType::GET_BUF>(EXTRA_PARAMS, pipe, static_cast<uint64_t>(bufId), mode);
}

SANITIZER_REPORT(get_bufi, pipe_t pipe, uint64_t bufId, bool mode)
{
    RecordBufEvent<RecordType::GET_BUFI>(EXTRA_PARAMS, pipe, bufId, mode);
}

SANITIZER_REPORT(rls_buf, pipe_t pipe, uint8_t bufId, bool mode)
{
    RecordBufEvent<RecordType::RLS_BUF>(EXTRA_PARAMS, pipe, static_cast<uint64_t>(bufId), mode);
}

SANITIZER_REPORT(rls_bufi, pipe_t pipe, uint64_t bufId, bool mode)
{
    RecordBufEvent<RecordType::RLS_BUFI>(EXTRA_PARAMS, pipe, bufId, mode);
}

SANITIZER_REPORT(get_buf_v, uint8_t bufId, bool mode)
{
    RecordBufEvent<RecordType::GET_BUF_V>(EXTRA_PARAMS, static_cast<pipe_t>(PipeType::PIPE_V),
        static_cast<uint64_t>(bufId), mode);
}

SANITIZER_REPORT(get_bufi_v, uint64_t bufId, bool mode)
{
    RecordBufEvent<RecordType::GET_BUFI_V>(EXTRA_PARAMS, static_cast<pipe_t>(PipeType::PIPE_V), bufId, mode);
}

SANITIZER_REPORT(rls_buf_v, uint8_t bufId, bool mode)
{
    RecordBufEvent<RecordType::RLS_BUF_V>(EXTRA_PARAMS, static_cast<pipe_t>(PipeType::PIPE_V),
        static_cast<uint64_t>(bufId), mode);
}

SANITIZER_REPORT(rls_bufi_v, uint64_t bufId, bool mode)
{
    RecordBufEvent<RecordType::RLS_BUFI_V>(EXTRA_PARAMS, static_cast<pipe_t>(PipeType::PIPE_V), bufId, mode);
}

SANITIZER_REPORT(wait_flag_dev_pipe, pipe_t pipe, int64_t xt)
{
    int64_t flagID = xt & 0xF;
    RecordWaitFlagDevEventWithPipe<RecordType::WAIT_FLAG_DEV_PIPE>(EXTRA_PARAMS, pipe, flagID);
}

SANITIZER_REPORT(wait_flag_devi_pipe, pipe_t pipe, uint8_t flagID)
{
    RecordWaitFlagDevEventWithPipe<RecordType::WAIT_FLAG_DEVI_PIPE>(EXTRA_PARAMS, pipe, flagID);
}

SANITIZER_REPORT(set_cross_core, pipe_t pipe, uint64_t config)
{
    RecordFftsSyncEvent<RecordType::FFTS_SYNC>(EXTRA_PARAMS, pipe, config);
}

SANITIZER_REPORT(wait_intra_block, pipe_t pipe, uint64_t syncID)
{
    RecordIntraBlockSyncEvent<RecordType::WAIT_INTRA_BLOCK>(EXTRA_PARAMS, pipe, syncID);
}

SANITIZER_REPORT(set_intra_block, pipe_t pipe, uint64_t syncID)
{
    RecordIntraBlockSyncEvent<RecordType::SET_INTRA_BLOCK>(EXTRA_PARAMS, pipe, syncID);
}

SANITIZER_REPORT(wait_intra_blocki, pipe_t pipe, uint8_t syncID)
{
    RecordIntraBlockSyncEvent<RecordType::WAIT_INTRA_BLOCKI>(EXTRA_PARAMS, pipe, syncID);
}

SANITIZER_REPORT(set_intra_blocki, pipe_t pipe, uint8_t syncID)
{
    RecordIntraBlockSyncEvent<RecordType::SET_INTRA_BLOCKI>(EXTRA_PARAMS, pipe, syncID);
}

SANITIZER_REPORT(hset_flag, pipe_t pipe, pipe_t tpipe, uint64_t eventID, mem_t memID, bool v)
{
    RecordHardSyncEvent<RecordType::HSET_FLAG>(EXTRA_PARAMS, pipe, tpipe, eventID, memID, v);
}

SANITIZER_REPORT(hwait_flag, pipe_t pipe, pipe_t tpipe, uint64_t eventID, mem_t memID, bool v)
{
    RecordHardSyncEvent<RecordType::HWAIT_FLAG>(EXTRA_PARAMS, pipe, tpipe, eventID, memID, v);
}

SANITIZER_REPORT(hset_flagi, pipe_t pipe, pipe_t tpipe, uint64_t eventID, mem_t memID, bool v)
{
    RecordHardSyncEvent<RecordType::HSET_FLAGI>(EXTRA_PARAMS, pipe, tpipe, eventID, memID, v);
}

SANITIZER_REPORT(hwait_flagi, pipe_t pipe, pipe_t tpipe, uint64_t eventID, mem_t memID, bool v)
{
    RecordHardSyncEvent<RecordType::HWAIT_FLAGI>(EXTRA_PARAMS, pipe, tpipe, eventID, memID, v);
}

SANITIZER_REPORT(set_flag_v, pipe_t tpipe, uint64_t eventID)
{
    pipe_t pipe = static_cast<pipe_t>(PipeType::PIPE_V);
    RecordSyncEvent<RecordType::SET_FLAG_V>(EXTRA_PARAMS, pipe, tpipe, eventID);
}

SANITIZER_REPORT(set_flagi_v, pipe_t tpipe, uint64_t eventID)
{
    pipe_t pipe = static_cast<pipe_t>(PipeType::PIPE_V);
    RecordSyncEvent<RecordType::SET_FLAGI_V>(EXTRA_PARAMS, pipe, tpipe, eventID);
}

SANITIZER_REPORT(wait_flag_v, pipe_t pipe, uint64_t eventID)
{
    pipe_t tpipe = static_cast<pipe_t>(PipeType::PIPE_V);
    RecordSyncEvent<RecordType::WAIT_FLAG_V>(EXTRA_PARAMS, pipe, tpipe, eventID);
}

SANITIZER_REPORT(wait_flagi_v, pipe_t pipe, uint64_t eventID)
{
    pipe_t tpipe = static_cast<pipe_t>(PipeType::PIPE_V);
    RecordSyncEvent<RecordType::WAIT_FLAGI_V>(EXTRA_PARAMS, pipe, tpipe, eventID);
}

SANITIZER_REPORT(pipe_barrier, pipe_t pipe)
{
    RecordPipeBarrierEvent(EXTRA_PARAMS, pipe);
}

SANITIZER_REPORT(dsb, uint64_t dsbType) {
    uint64_t memDomain = dsbType & 0xFF;
    RecordDsbBarrierEvent(EXTRA_PARAMS, static_cast<pipe_t>(PipeType::PIPE_S), memDomain);
}

SANITIZER_REPORT(set_cross_core_v, uint64_t config)
{
    pipe_t pipe = static_cast<pipe_t>(PipeType::PIPE_V);
    RecordFftsSyncEvent<RecordType::FFTS_SYNC_V>(EXTRA_PARAMS, pipe, config);
}

SANITIZER_REPORT(wait_flag_dev_pipe_v, int64_t xt)
{
    int64_t flagID = xt & 0xF;
    pipe_t pipe = static_cast<pipe_t>(PipeType::PIPE_V);
    RecordWaitFlagDevEventWithPipe<RecordType::WAIT_FLAG_DEV_PIPE_V>(EXTRA_PARAMS, pipe, flagID);
}

SANITIZER_REPORT(wait_flag_devi_pipe_v, uint8_t flagID)
{
    pipe_t pipe = static_cast<pipe_t>(PipeType::PIPE_V);
    RecordWaitFlagDevEventWithPipe<RecordType::WAIT_FLAG_DEVI_PIPE_V>(EXTRA_PARAMS, pipe, flagID);
}

SANITIZER_REPORT(set_intra_block_v, uint64_t syncID)
{
    pipe_t pipe = static_cast<pipe_t>(PipeType::PIPE_V);
    RecordIntraBlockSyncEvent<RecordType::SET_INTRA_BLOCK_V>(EXTRA_PARAMS, pipe, syncID);
}

SANITIZER_REPORT(set_intra_blocki_v, uint8_t syncID)
{
    pipe_t pipe = static_cast<pipe_t>(PipeType::PIPE_V);
    RecordIntraBlockSyncEvent<RecordType::SET_INTRA_BLOCKI_V>(EXTRA_PARAMS, pipe, syncID);
}

SANITIZER_REPORT(wait_intra_block_v, uint64_t syncID)
{
    pipe_t pipe = static_cast<pipe_t>(PipeType::PIPE_V);
    RecordIntraBlockSyncEvent<RecordType::WAIT_INTRA_BLOCK_V>(EXTRA_PARAMS, pipe, syncID);
}

SANITIZER_REPORT(wait_intra_blocki_v, uint8_t syncID)
{
    pipe_t pipe = static_cast<pipe_t>(PipeType::PIPE_V);
    RecordIntraBlockSyncEvent<RecordType::WAIT_INTRA_BLOCKI_V>(EXTRA_PARAMS, pipe, syncID);
}
