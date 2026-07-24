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


#include <iostream>
#include <cstdint>
#include <functional>
#include <fstream>

#include "constant.h"
#include "record_defs.h"
#include "record_format.h"
#include "runtime_context.h"
#include "utility/cpp_future.h"
#include "utility/log.h"

#include "kernel_block.h"

namespace {

using namespace Sanitizer;

template <typename Record>
void CalRecordPc(Record &record)
{
    uint64_t pcStartAddr = RuntimeContext::Instance().kernelSummary_.pcStartAddr;
    if (pcStartAddr == 0x00) {
        record.location.pc =  INVALID_PC_OFFSET;
    } else if (record.location.pc >= pcStartAddr) {
        // update pc offset with pc start addr
        record.location.pc -= pcStartAddr;
    }
}

template <typename Record>
inline bool ParseRecordByType(uint8_t const *ptr, Record &record, uint64_t &recordSize)
{
    record = *static_cast<Record const *>(static_cast<void const *>(ptr));
    recordSize = sizeof(Record);
    CalRecordPc(record);
    return true;
}

inline bool ParseMemErrorType(uint8_t const *ptr, KernelErrorRecord &errorRecord, uint64_t &recordSize)
{
    errorRecord = *static_cast<KernelErrorRecord const *>(static_cast<void const *>(ptr));
    recordSize = sizeof(KernelErrorRecord) + errorRecord.recordSize +
        errorRecord.errorNum * sizeof(KernelErrorDesc);
    /// 更新record中的指针成员
    errorRecord.record = reinterpret_cast<const void *>(ptr + sizeof(KernelErrorRecord));
    errorRecord.kernelErrorDesc = reinterpret_cast<const KernelErrorDesc *>(ptr + sizeof(KernelErrorRecord) +
        errorRecord.recordSize);
    return true;
}

const std::map<RecordType, std::function<bool(uint8_t const *, KernelRecord &, uint64_t &)>> RECORD_TYPE_FUNC_MAP = {
    {RecordType::LOAD, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::STORE, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::LD, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::LD_IO, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::ST, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::ST_IO, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::STP, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::STI, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::STI_IO, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::LDP, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::ST_ATOMIC, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::STI_ATOMIC, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::ST_DEV, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::LD_DEV, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::DMA_MOV, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.dmaMovRecord, offset);
    }},
    {RecordType::DMA_MOV_CONV_RELU, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.dmaMovConvReluRecord, offset);
    }},
    {RecordType::DMA_MOV_DEPTH_WISE, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.dmaMovConvReluRecord, offset);
    }},
    {RecordType::DMA_MOV_ND2NZ, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.dmaMovNd2nzRecord, offset);
    }},
    {RecordType::DMA_MOV_ND2NZ_D, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.dmaMovNd2nzDavRecord, offset);
    }},
    {RecordType::DMA_MOV_DN2NZ_D, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.dmaMovNd2nzDavRecord, offset);
    }},
    {RecordType::MOV_ALIGN, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.movAlignRecord, offset);
    }},
    {RecordType::MOV_ALIGN_V2, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.movAlignRecordV2, offset);
    }},
    {RecordType::ND_DMA_OUT_TO_UB, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.ndDMAOut2UbRecord, offset);
    }},
    {RecordType::MOV_BT, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.movBtRecord, offset);
    }},
    {RecordType::MOV_FP, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.movFpRecord, offset);
    }},
    {RecordType::FIX_L0C_TO_L1, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.movFpRecord, offset);
    }},
    {RecordType::FIX_L0C_TO_UB, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.movFpRecord, offset);
    }},
    {RecordType::MOV_L1_TO_UB, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.movL1UbRecord, offset);
    }},
    {RecordType::VEC_DUP, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.vecDupRecord, offset);
    }},
    {RecordType::LOAD_2D, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.load2DRecord, offset);
    }},
    {RecordType::LOAD_L1_2D, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadL12DRecord, offset);
    }},
    {RecordType::LOAD_L1_MX_2D, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadL1Mx2DRecord, offset);
    }},
    {RecordType::LOAD_L1_2D_TRANSPOSE, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.load2DTransposeRecord, offset);
    }},
    {RecordType::LOAD_2D_SPARSE, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.load2DSparseRecord, offset);
    }},
    {RecordType::LOAD_2D_TRANSPOSE, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.load2DTransposeRecord, offset);
    }},
    {RecordType::DECOMPRESS_HEADER, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.decompressHeaderRecord, offset);
    }},
    {RecordType::LOAD_3D, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.load3DRecord, offset);
    }},
    {RecordType::LOAD_3D_V2, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.load3DV2Record, offset);
    }},
    {RecordType::LOAD_IMAGE, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadImageRecord, offset);
    }},
    {RecordType::LOAD_SMASK, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadSmaskRecord, offset);
    }},
    {RecordType::BROADCAST, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.broadcastRecord, offset);
    }},
    {RecordType::DC_PRELOAD, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.dcPreloadRecord, offset);
    }},
    {RecordType::SCATTERVNCHWCONV, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.scatterVnchwconvRecord, offset);
    }},
    {RecordType::SCATTERVNCHWCONV_A5, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.scatterVnchwconvRecord, offset);
    }},
    {RecordType::VBS32_A5, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.vbs32Record, offset);
    }},
    {RecordType::SET_2D, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.set2DRecord, offset);
    }},
    {RecordType::LOAD_B2, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadB2Record, offset);
    }},
    {RecordType::LOAD_A_WINOGRAD, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadAWinogradRecord, offset);
    }},
    {RecordType::LOAD_B_WINOGRAD, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadBWinogradRecord, offset);
    }},
    {RecordType::UNARY_OP, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.unaryOpRecord, offset);
    }},
    {RecordType::VGATHER, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.vgatherRecord, offset);
    }},
    {RecordType::ELEMENT, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.elementRecord, offset);
    }},
    {RecordType::VCONV_DST_S4_OP, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.unaryOpRecord, offset);
    }},
    {RecordType::VCONV_SRC_S4_OP, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.unaryOpRecord, offset);
    }},
    {RecordType::VCOPY_OP, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.unaryOpRecord, offset);
    }},
    {RecordType::VREDUCEV2_UNARY, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.unaryOpRecord, offset);
    }},
    {RecordType::VREDUCEV2_BINARY, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.binaryOpRecord, offset);
    }},
    {RecordType::VREDUCEV2, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.reduceV2Record, offset);
    }},
    {RecordType::VMRGSORT4_OP_C220, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.unaryOpRecord, offset);
    }},
    {RecordType::VMRGSORT4_OP_M200, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.unaryOpRecord, offset);
    }},
    {RecordType::VMRGSORT4_OP_C310, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.vms4V2RecordA5, offset);
    }},
    {RecordType::BINARY_OP, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.binaryOpRecord, offset);
    }},
    {RecordType::VSEL_OP, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.binaryOpRecord, offset);
    }},
    {RecordType::TERNARY_OP, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.binaryOpRecord, offset);
    }},
    {RecordType::MATRIX_MUL_OP, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.matrixMulOpRecord, offset);
    }},
    {RecordType::MMAD_A5, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.mmadA5Record, offset);
    }},
    {RecordType::VEC_REGPROPCOOR_OP, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.vecRegPropCoordOpRecord, offset);
    }},
    {RecordType::MSTX_STUB, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.mstxRecord, offset);
    }},
    {RecordType::REDUCE_OP, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.reduceOpRecord, offset);
    }},
    {RecordType::CMPMASK_OP, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.cmpMaskRecord, offset);
    }},
    {RecordType::SET_FLAG, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.syncRecord, offset);
    }},
    {RecordType::SET_FLAGI, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.syncRecord, offset);
    }},
    {RecordType::WAIT_FLAG, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.syncRecord, offset);
    }},
    {RecordType::WAIT_FLAGI, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.syncRecord, offset);
    }},
    {RecordType::GET_BUF, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.bufRecord, offset);
    }},
    {RecordType::GET_BUFI, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.bufRecord, offset);
    }},
    {RecordType::RLS_BUF, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.bufRecord, offset);
    }},
    {RecordType::RLS_BUFI, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.bufRecord, offset);
    }},
    {RecordType::GET_BUF_V, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.bufRecord, offset);
    }},
    {RecordType::GET_BUFI_V, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.bufRecord, offset);
    }},
    {RecordType::RLS_BUF_V, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.bufRecord, offset);
    }},
    {RecordType::RLS_BUFI_V, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.bufRecord, offset);
    }},
    {RecordType::FFTS_SYNC, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.fftsSyncRecord, offset);
    }},
    {RecordType::WAIT_FLAG_DEV, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.waitFlagDevRecord, offset);
    }},
    {RecordType::WAIT_FLAG_DEV_PIPE, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.waitFlagDevPipeRecord, offset);
    }},
    {RecordType::WAIT_FLAG_DEVI_PIPE, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.waitFlagDevPipeRecord, offset);
    }},
    {RecordType::SET_INTRA_BLOCK, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.intraBlockSyncRecord, offset);
    }},
    {RecordType::WAIT_INTRA_BLOCK, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.intraBlockSyncRecord, offset);
    }},
    {RecordType::SET_INTRA_BLOCKI, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.intraBlockSyncRecord, offset);
    }},
    {RecordType::WAIT_INTRA_BLOCKI, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.intraBlockSyncRecord, offset);
    }},
    {RecordType::FFTS_SYNC_V, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.fftsSyncRecord, offset);
    }},
    {RecordType::WAIT_FLAG_DEV_PIPE_V, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.waitFlagDevPipeRecord, offset);
    }},
    {RecordType::WAIT_FLAG_DEVI_PIPE_V, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.waitFlagDevPipeRecord, offset);
    }},
    {RecordType::SET_INTRA_BLOCK_V, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.intraBlockSyncRecord, offset);
    }},
    {RecordType::SET_INTRA_BLOCKI_V, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.intraBlockSyncRecord, offset);
    }},
    {RecordType::WAIT_INTRA_BLOCK_V, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.intraBlockSyncRecord, offset);
    }},
    {RecordType::WAIT_INTRA_BLOCKI_V, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.intraBlockSyncRecord, offset);
    }},
    {RecordType::SET_FLAG_V, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.syncRecord, offset);
    }},
    {RecordType::SET_FLAGI_V, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.syncRecord, offset);
    }},
    {RecordType::WAIT_FLAG_V, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.syncRecord, offset);
    }},
    {RecordType::WAIT_FLAGI_V, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.syncRecord, offset);
    }},
    {RecordType::HSET_FLAG, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.hardSyncRecord, offset);
    }},
    {RecordType::HSET_FLAGI, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.hardSyncRecord, offset);
    }},
    {RecordType::HWAIT_FLAG, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.hardSyncRecord, offset);
    }},
    {RecordType::HWAIT_FLAGI, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.hardSyncRecord, offset);
    }},
    {RecordType::PIPE_BARRIER, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.pipeBarrierRecord, offset);
    }},
    {RecordType::SET_ATOMIC, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.atomicModeRecord, offset);
    }},
    {RecordType::IB_SET_STUB, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.softSyncRecord, offset);
    }},
    {RecordType::IB_WAIT_STUB, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.softSyncRecord, offset);
    }},
    {RecordType::SYNC_ALL_STUB, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.softSyncRecord, offset);
    }},
    {RecordType::SIMT_LDG, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.simtLoadStoreRecord, offset);
    }},
    {RecordType::SIMT_STG, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.simtLoadStoreRecord, offset);
    }},
    {RecordType::SIMT_LDS, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.simtLoadStoreRecord, offset);
    }},
    {RecordType::SIMT_STS, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.simtLoadStoreRecord, offset);
    }},
    {RecordType::SIMT_LDK, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.simtLoadStoreRecord, offset);
    }},
    {RecordType::SIMT_STK, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.simtLoadStoreRecord, offset);
    }},
    {RecordType::SIMT_LD, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.simtLoadStoreRecord, offset);
    }},
    {RecordType::SIMT_ST, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.simtLoadStoreRecord, offset);
    }},
    {RecordType::SIMT_ATOM, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.simtAtomRecord, offset);
    }},
    {RecordType::SIMT_RED, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.simtAtomRecord, offset);
    }},
    {RecordType::ONLINE_ERROR, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseMemErrorType(record, kernelRecord.payload.kernelErrorRecord, offset);
    }},
    {RecordType::SCALAR_RED, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::SCALAR_ATOM, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::LDVA, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.loadStoreRecord, offset);
    }},
    {RecordType::SET_L1_2D, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.setL12DRecord, offset);
    }},
    {RecordType::MOV_UB_TO_L1, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.movL1UbRecord, offset);
    }},
    {RecordType::MOV_UB_TO_UB, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.movL1UbRecord, offset);
    }},
    {RecordType::MOV_CBUF_TO_BT, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.movL1BtRecord, offset);
    }},
    {RecordType::MOV_CBUF_TO_FB, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.movL1FbRecord, offset);
    }},
    {RecordType::SET_VECTOR_MASK_0, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.registerSetRecord, offset);
    }},
    {RecordType::SET_VECTOR_MASK_1, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.registerSetRecord, offset);
    }},
    {RecordType::SET_CTRL, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.registerSetRecord, offset);
    }},
    {RecordType::SET_FFTS_BASE_ADDR, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.registerSetRecord, offset);
    }},
    {RecordType::SET_FPC, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.registerSetRecord, offset);
    }},
    {RecordType::SET_QUANT_PRE, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.registerSetRecord, offset);
    }},
    {RecordType::SET_QUANT_POST, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.registerSetRecord, offset);
    }},
    {RecordType::SET_LRELU_ALPHA, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.registerSetRecord, offset);
    }},
    {RecordType::THREAD_BLOCK_BARRIER, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.simtEmptyRecord, offset);
    }},
    {RecordType::SIMT_START, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.simtEmptyRecord, offset);
    }},
    {RecordType::SIMT_END, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.simtEmptyRecord, offset);
    }},
    {RecordType::SIMT_CALL, [](uint8_t const *record, KernelRecord &kernelRecord, uint64_t &offset) {
        return ParseRecordByType(record, kernelRecord.payload.mainScalarEmptyRecord, offset);
    }}
};

bool ParseRecord(RecordType recordType, uint8_t const *record, KernelRecord &kernelRecord, uint64_t &recordSize)
{
    auto it = RECORD_TYPE_FUNC_MAP.find(kernelRecord.recordType);
    if (it != RECORD_TYPE_FUNC_MAP.end()) {
        return it->second(record, kernelRecord, recordSize);
    } else {
        std::cout << "[mssanitizer] Parse record failed, " << kernelRecord.recordType <<
                     " record type does not exist in recordTypeMap" << std::endl;
        return false;
    }
}

inline uint64_t GetAllThreadSize(RecordGlobalHead const &globalHead)
{
    return (globalHead.offsetInfo.simtErrorInfo.size + sizeof(SimtRecordBlockHead)) * SIMT_THREAD_MAX_SIZE;
}


/// 把threadId按三维展开为(x,y,z)
inline void DecomposeThreadId(uint16_t threadId, SimtEntryBlockHead const &entryHead, SimtThreadLocation &threadLoc)
{
    uint16_t threadXDim = entryHead.threadXDim;
    uint16_t threadYDim = entryHead.threadYDim;
    if (threadXDim == 0 || threadYDim == 0) {
        SAN_ERROR_LOG("threadXDim or threadYDim equal 0 error");
        return;
    }
    threadLoc.idX = threadId % threadXDim;
    threadLoc.idY = (threadId % (threadXDim * threadYDim)) / threadXDim;
    threadLoc.idZ = threadId / (threadXDim * threadYDim);
    threadLoc.mainScalarPc = entryHead.mainScalarPc;
}

inline bool IsReadStatus(OnlineShadowMemory::MemoryByteStatus memStatus)
{
    return (memStatus == OnlineShadowMemory::MemoryByteStatus::GLOBAL_READ) ||
        (memStatus == OnlineShadowMemory::MemoryByteStatus::READ);
}

} // namespace [Dummy]

namespace Sanitizer {

std::unique_ptr<KernelBlock> KernelBlock::CreateKernelBlock(uint8_t const *memInfo, uint64_t memSize, uint32_t blockIdx)
{
    if (memInfo == nullptr) {
        SAN_ERROR_LOG("MemInfo is nullptr in kernel block from block %u", blockIdx);
        return nullptr;
    }

    constexpr uint64_t HEAD_SIZE = sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    if (memSize < HEAD_SIZE) {
        SAN_ERROR_LOG("MemSize %lu is less than the required record protocol head size %lu in block %u",
                      memSize, HEAD_SIZE, blockIdx);
        return nullptr;
    }

    auto recordGlobalHead = static_cast<RecordGlobalHead const *>(static_cast<void const *>(memInfo));
    auto simdRecordHead = static_cast<RecordBlockHead const *>(static_cast<void const *>(recordGlobalHead + 1));
    // only check security value at blockIdx 0
    if (blockIdx == 0) {
        if (recordGlobalHead->securityVal != RECORD_HEAD_SECURITY_VALUE) {
            std::cout << "[mssanitizer] [error]"
                " The record address has been changed by the user program,"
                " and the detection result is unreliable. "
                      << std::endl;
            return nullptr;
        }
    }

    auto kernelBlock = MakeUnique<KernelBlock>();
    /// 溢出时，计算需要的额外size
    if (simdRecordHead->recordWriteCount < simdRecordHead->recordCount) {
        kernelBlock->extendCacheSize_ += (simdRecordHead->offset - simdRecordHead->writeOffset) / MB_TO_BYTES + 1;
        kernelBlock->extendRecordCount_ += simdRecordHead->recordCount - simdRecordHead->recordWriteCount;
    }
    kernelBlock->recordGlobalHead_ = *recordGlobalHead;
    kernelBlock->simdRecordHead_ = *simdRecordHead;
    kernelBlock->simdRecords_ = memInfo + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);

    // 安全检查：验证 writeOffset 本身是否合理（防止过大值导致指针运算溢出）
    if (simdRecordHead->writeOffset > memSize) {
        SAN_ERROR_LOG("writeOffset %lu exceeds memSize %lu in block %u",
                      simdRecordHead->writeOffset, memSize, blockIdx);
        return nullptr;
    }
    // 安全检查：验证 simdRecords_ + writeOffset 是否在 memSize 范围内
    uint64_t simdRecordsStart = sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    if (simdRecordsStart + simdRecordHead->writeOffset > memSize) {
        SAN_ERROR_LOG("writeOffset %lu exceeds limits %lu in block %u",
                      simdRecordHead->writeOffset, memSize - simdRecordsStart, blockIdx);
        return nullptr;
    }

    kernelBlock->simtRecordHead_ = reinterpret_cast<SimtRecordBlockHead const*>(
        kernelBlock->simdRecords_ + simdRecordHead->writeOffset);

    // 安全检查：验证 simt entry 区域是否在 memSize 范围内，部分算子不含simt entry，此时simtEntryHead_置未空指针
    uint64_t simtEntryStart = simdRecordsStart + simdRecordHead->writeOffset + GetAllThreadSize(*recordGlobalHead);
    if (memSize >= sizeof(SimtEntryBlockHead) && memSize - sizeof(SimtEntryBlockHead) >= simtEntryStart) {
        kernelBlock->simtEntryHead_ = reinterpret_cast<SimtEntryBlockHead const*>(
            kernelBlock->simdRecords_ + simdRecordHead->writeOffset + GetAllThreadSize(*recordGlobalHead));
    } else {
        kernelBlock->simtEntryHead_ = nullptr;
    }

    kernelBlock->blockIdx_ = blockIdx;
    if (blockIdx == 0) {
        KernelBlock::totalBlockDim_ = recordGlobalHead->kernelInfo.totalBlockDim;
        if (simdRecordHead->blockInfo.blockType == BlockType::AIVEC) {
            KernelBlock::vecSubBlockDim_ = simdRecordHead->blockInfo.vecSubBlockDim;
        }
    }
    return kernelBlock;
}

bool KernelBlock::ParseSimdRecord(uint8_t const *record, KernelRecord &kernelRecord)
{
    auto recordTypePtr = static_cast<const RecordType *>(static_cast<const void *>(record));

    // 某些场景下认为当前指令与前一条指令同时发生，此时不更新 serialNo
    if (*recordTypePtr == RecordType::ELEMENT) {
        if (lastRecordType_ != RecordType::VGATHER && lastRecordType_ != RecordType::ELEMENT) {
            ++RuntimeContext::Instance().serialNo_;
        }
    } else if (*recordTypePtr == RecordType::VSEL_OP) {
        if (lastRecordType_ != RecordType::CMPMASK_OP) {
            ++RuntimeContext::Instance().serialNo_;
        }
    } else if (*recordTypePtr == RecordType::UNARY_OP || *recordTypePtr == RecordType::BINARY_OP ||
        *recordTypePtr == RecordType::DMA_MOV || *recordTypePtr == RecordType::MOV_ALIGN) {
        if (lastRecordType_ != RecordType::MSTX_STUB) {
            ++RuntimeContext::Instance().serialNo_;
        }
    } else {
        ++RuntimeContext::Instance().serialNo_;
    }
    lastRecordType_ = *recordTypePtr;

    kernelRecord.recordType = *recordTypePtr;
    kernelRecord.blockType = this->simdRecordHead_.blockInfo.blockType;
    kernelRecord.serialNo = RuntimeContext::Instance().serialNo_;
    record += sizeof(RecordType);

    uint64_t recordSize {};
    if (!ParseRecord(*recordTypePtr, record, kernelRecord, recordSize)) {
        return false;
    }
    /// update record offset with recordSize
    this->simdOffset_ += sizeof(RecordType) + recordSize;
    ++this->recordIdx_;

    /// 后续字段赋值，需要在ParseRecord之后赋值，ParseRecord会重新解析record，在此之前赋值会导致信息丢失；
    if (*recordTypePtr == RecordType::FFTS_SYNC || *recordTypePtr == RecordType::FFTS_SYNC_V) {
        kernelRecord.payload.fftsSyncRecord.vecSubBlockDim = vecSubBlockDim_;
    } else if (*recordTypePtr == RecordType::SET_INTRA_BLOCK || *recordTypePtr == RecordType::SET_INTRA_BLOCKI ||
        *recordTypePtr == RecordType::SET_INTRA_BLOCK_V || *recordTypePtr == RecordType::SET_INTRA_BLOCKI_V) {
        kernelRecord.payload.intraBlockSyncRecord.vecSubBlockDim = vecSubBlockDim_;
    }
    return true;
}

std::vector<uint8_t> KernelBlock::AllocMemory(size_t size) const
{
    return std::vector<uint8_t>(size);
}

void KernelBlock::ParseSimtErrorRecord(std::vector<KernelRecord> &kernelRecords)
{
    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    auto &kernelRecord = sanitizerRecord.payload.kernelRecord;
    kernelRecord.blockType = this->simdRecordHead_.blockInfo.blockType;
    uint64_t simtExtendBytes{};
    uint64_t simtExtendRecordCount{};
    for (size_t threadId = 0; threadId < SIMT_THREAD_MAX_SIZE; ++threadId) {
        if (threadId > 0) {
            uint8_t const *threadHead = reinterpret_cast<uint8_t const*>(simtRecordHead_ + 1) +
                recordGlobalHead_.offsetInfo.simtErrorInfo.size;
            simtRecordHead_ = reinterpret_cast<SimtRecordBlockHead const*>(threadHead);
        }

        if (simtRecordHead_->recordWriteCount < simtRecordHead_->recordCount) {
            simtExtendBytes = std::max(simtExtendBytes, simtRecordHead_->offset - simtRecordHead_->writeOffset);
            simtExtendRecordCount = std::max(simtExtendRecordCount,
                simtRecordHead_->recordCount - simtRecordHead_->recordWriteCount);
        }
        if (simtRecordHead_->recordWriteCount == 0U) { continue; }

        simtRecords_ = reinterpret_cast<uint8_t const*>(simtRecordHead_ + 1);
        this->simtOffset_ = 0U;
        for (size_t recordCount = 0; recordCount < simtRecordHead_->recordWriteCount; ++recordCount) {
            auto recordTypePtr = static_cast<const RecordType *>(static_cast<const void *>(simtRecords_));
            kernelRecord.recordType = *recordTypePtr;
            kernelRecord.serialNo = ++RuntimeContext::Instance().serialNo_;
            simtRecords_ += sizeof(RecordType);

            uint64_t recordSize{};
            if (!ParseRecord(*recordTypePtr, simtRecords_, kernelRecord, recordSize)) {
                SAN_ERROR_LOG("parse record error threadId %lu", threadId);
                break;
            }
            kernelRecords.push_back(kernelRecord);
            simtRecords_ += recordSize;
            this->simtOffset_ += sizeof(RecordType) + recordSize;
            if (this->simtOffset_ >= simtRecordHead_->offset) { break; }
        }
    }

    /// 溢出时，计算需要的额外size
    if (simtExtendBytes > 0) {
        extendCacheSize_ += (recordGlobalHead_.offsetInfo.simtErrorInfo.size + simtExtendBytes) *
            SIMT_THREAD_MAX_SIZE / SIMT_CACHE_SIZE_RATIO  / MB_TO_BYTES + 1;
        extendRecordCount_ += simtExtendRecordCount;
    }

    if (extendCacheSize_ > 0) {
        uint64_t totalSize = extendCacheSize_ + recordGlobalHead_.checkParms.cacheSize;
        PrintCacheSizeLog(totalSize);
    }
}

void KernelBlock::CacheDynamicRecord(uint8_t *startPtr, size_t recordCount, KernelRecord &kernelRecord, uint32_t offset) const
{
    auto &dynamicRecord = kernelRecord.payload.dynamicRecord;
    kernelRecord.serialNo = ++RuntimeContext::Instance().serialNo_;
    dynamicRecord.count = recordCount;
    size_t allocSize = recordCount * sizeof(ShadowMemoryRecord);
    std::vector<uint8_t> memoryVec = AllocMemory(allocSize);
    dynamicMemorys_.push_back(memoryVec);
    auto &vecBack = dynamicMemorys_.back();
    vecBack.assign(startPtr + offset, startPtr + offset + allocSize);
    dynamicRecord.buffer = static_cast<void *>(vecBack.data());
    for (size_t i = 0; i < dynamicRecord.count; ++i) {
        CalRecordPc(reinterpret_cast<ShadowMemoryRecord *>(dynamicRecord.buffer)[i]);
    }
}

bool KernelBlock::ParseSimtEntryRecord(std::vector<KernelRecord> &kernelRecords) {
    if (simtEntryHead_ == nullptr) return false;
    using namespace OnlineShadowMemory;
    auto entryRecord = reinterpret_cast<SimtEntryRecord const*>(simtEntryHead_ + 1);
    if (simtEntryHead_->exceedSize > 0) {
        extendCacheSize_ += simtEntryHead_->exceedSize;
        extendRecordCount_++;
    }
    std::vector<ShadowMemoryRecord> records;
    records.reserve(simtEntryHead_->recordWriteCount);
    size_t ubRecordCount = 0;
    for (size_t i = 0; i < simtEntryHead_->recordWriteCount; ++i) {
        ShadowMemoryRecord record{};
        record.addr = entryRecord[i].addr;
        record.size = entryRecord[i].size;
        uint64_t status = entryRecord[i].status;
        auto memType = static_cast<OnlineMemoryType>((status >> MEMORY_TYPE_START_BIT) & MEMORY_TYPE_MASK);
        auto memStatus = static_cast<MemoryByteStatus>((status >> MEMORY_STATUS_START_BIT) & MEMORY_STATUS_MASK);
        record.space = memType == OnlineMemoryType::GM ? AddressSpace::GM : AddressSpace::UB;
        record.accessType = IsReadStatus(memStatus) ? AccessType::READ : AccessType::WRITE;
        if (record.space == AddressSpace::UB) ubRecordCount++;
        uint16_t threadId = status & THREAD_ID_MASK;
        DecomposeThreadId(threadId, *simtEntryHead_, record.threadLoc);
        record.location.pc = (status >> PC_START_BIT) & PC_MASK;
        record.location.blockId = simdRecordHead_.blockInfo.blockId;
        records.emplace_back(record);
    }
    KernelRecord kernelRecord{};
    kernelRecord.blockType = this->simdRecordHead_.blockInfo.blockType;
    kernelRecord.recordType = RecordType::DYNAMIC_OP;
    auto &dynamicRecord = kernelRecord.payload.dynamicRecord;
    dynamicRecord.dynamicType = RecordType::SIMT_ENTRY;
    if (ubRecordCount > 0) {
        CacheDynamicRecord(reinterpret_cast<uint8_t *>(records.data()), ubRecordCount, kernelRecord);
        kernelRecords.push_back(kernelRecord);
    }
    size_t gmRecordCount = simtEntryHead_->recordWriteCount - ubRecordCount;
    if (gmRecordCount > 0) {
        CacheDynamicRecord(reinterpret_cast<uint8_t *>(records.data()), gmRecordCount, kernelRecord,
            ubRecordCount * sizeof(ShadowMemoryRecord));
        kernelRecords.push_back(kernelRecord);
    }
    // 缓存记录后处理当前simt_entry的偏移
    auto nextSimtEntryHead = reinterpret_cast<uint8_t const*>(simtEntryHead_) + sizeof(SimtEntryBlockHead) +
        simtEntryHead_->recordWriteCount * sizeof(SimtEntryRecord);
    simtEntryHead_ = reinterpret_cast<SimtEntryBlockHead const*>(nextSimtEntryHead);
    return true;
}

void KernelBlock::PrintCacheSizeLog(uint64_t totalSize)
{
    if (totalSize > MAX_RECORD_BUF_SIZE_EACH_BLOCK) {
        std::cout << "[mssanitizer] [error] " << this->extendRecordCount_
                  << " records undetected in block " << this->blockIdx_
                  <<", exceed the max record buffer size for each block: "
                  << MAX_RECORD_BUF_SIZE_EACH_BLOCK
                  << ". Some records were discarded, the detection result maybe unreliable!" << std::endl;
    } else {
        std::cout << "[mssanitizer] [warning] " << this->extendRecordCount_
                  << " records undetected in block " << this->blockIdx_
                  << ", please use --cache-size=" << totalSize << " to run the operator again."
                  << std::endl;
    }
}

bool KernelBlock::NextSimd(KernelRecord &kernelRecord)
{
    if (this->simdRecordHead_.recordWriteCount == 0U || this->simdOffset_ >= this->simdRecordHead_.writeOffset) {
        lastRecordType_ = RecordType::BLOCK_FINISH;
        return false;
    }
    uint8_t const *currentRecord = this->simdRecords_ + this->simdOffset_;
    if (!ParseSimdRecord(currentRecord, kernelRecord)) {
        lastRecordType_ = RecordType::BLOCK_FINISH;
        return false;
    }

    return true;
}

uint64_t KernelBlock::GetTotalBlockDim() const
{
    return totalBlockDim_;
}

thread_local uint8_t KernelBlock::vecSubBlockDim_{};
thread_local uint64_t KernelBlock::totalBlockDim_{};
thread_local std::vector<std::vector<uint8_t>> KernelBlock::dynamicMemorys_{};
thread_local RecordType KernelBlock::lastRecordType_{RecordType::BLOCK_FINISH};

} // namespace Sanitizer
