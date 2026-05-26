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

#ifndef PLUGIN_RECORD_TYPE_MAP_H
#define PLUGIN_RECORD_TYPE_MAP_H

#include <type_traits>
#include "core/framework/type_map.h"
#include "core/framework/record_defs.h"

namespace Sanitizer {

// 定义一个方便使用的type_pair别名专门用于 RecordType值 => Record类型 的映射
template<RecordType key, typename ValueType>
using TypePair = type_pair<RecordType, key, ValueType>;

// 定义一个方便使用的type_map别名专门用于 RecordType值 => Record类型 的映射
template<typename... TypePairs>
using TypeMap = type_map<RecordType, TypePairs...>;

// 具体实现映射的类型
// 以后需要添加新的映射只需要在这里继续添加即可
// 注意一下最后那个模板参数后面不能有逗号
using RecordTypeMap = TypeMap<TypePair<RecordType::SIMT_LDG, SimtLoadStoreRecord>,
    TypePair<RecordType::SIMT_STG, SimtLoadStoreRecord>, TypePair<RecordType::SIMT_LDS, SimtLoadStoreRecord>,
    TypePair<RecordType::SIMT_STS, SimtLoadStoreRecord>, TypePair<RecordType::SIMT_LDK, SimtLoadStoreRecord>,
    TypePair<RecordType::SIMT_STK, SimtLoadStoreRecord>, TypePair<RecordType::SIMT_LD, SimtLoadStoreRecord>,
    TypePair<RecordType::SIMT_ST, SimtLoadStoreRecord>, TypePair<RecordType::SIMT_ATOM, SimtAtomRecord>,
    TypePair<RecordType::SIMT_RED, SimtLoadStoreRecord>, TypePair<RecordType::THREAD_BLOCK_BARRIER, SimtSyncRecord>,
    TypePair<RecordType::LOAD, LoadStoreRecord>, TypePair<RecordType::STORE, LoadStoreRecord>,
    TypePair<RecordType::LD, LoadStoreRecord>, TypePair<RecordType::LD_IO, LoadStoreRecord>,
    TypePair<RecordType::ST, LoadStoreRecord>, TypePair<RecordType::ST_IO, LoadStoreRecord>,
    TypePair<RecordType::STP, LoadStoreRecord>, TypePair<RecordType::STI, LoadStoreRecord>,
    TypePair<RecordType::STI_IO, LoadStoreRecord>, TypePair<RecordType::LDP, LoadStoreRecord>,
    TypePair<RecordType::ST_ATOMIC, LoadStoreRecord>, TypePair<RecordType::STI_ATOMIC, LoadStoreRecord>,
    TypePair<RecordType::ST_DEV, LoadStoreRecord>, TypePair<RecordType::LD_DEV, LoadStoreRecord>,
    TypePair<RecordType::DMA_MOV, DmaMovRecord>, TypePair<RecordType::DMA_MOV_CONV_RELU, DmaMovConvReluRecord>,
    TypePair<RecordType::DMA_MOV_DEPTH_WISE, DmaMovConvReluRecord>,
    TypePair<RecordType::DMA_MOV_ND2NZ, DmaMovNd2nzRecord>, TypePair<RecordType::MOV_ALIGN, MovAlignRecord>,
    TypePair<RecordType::MOV_ALIGN_V2, MovAlignRecordV2>, TypePair<RecordType::ND_DMA_OUT_TO_UB, NdDMAOut2UbRecord>,
    TypePair<RecordType::MOV_BT, MovBtRecord>, TypePair<RecordType::MOV_FP, MovFpRecord>,
    TypePair<RecordType::FIX_L0C_TO_L1, MovFpRecord>, TypePair<RecordType::FIX_L0C_TO_UB, MovFpRecord>,
    TypePair<RecordType::VEC_DUP, VecDupRecord>, TypePair<RecordType::LOAD_2D, Load2DRecord>,
    TypePair<RecordType::LOAD_L1_2D, LoadL12DRecord>, TypePair<RecordType::LOAD_L1_MX_2D, LoadL1Mx2DRecord>,
    TypePair<RecordType::LOAD_L1_2D_TRANSPOSE, LoadL12DTransposeRecord>,
    TypePair<RecordType::LOAD_2D_SPARSE, Load2DSparseRecord>,
    TypePair<RecordType::LOAD_2D_TRANSPOSE, Load2DTransposeRecord>,
    TypePair<RecordType::DECOMPRESS_HEADER, DecompressHeaderRecord>, TypePair<RecordType::DC_PRELOAD, DcPreloadRecord>,
    TypePair<RecordType::BROADCAST, BroadcastRecord>, TypePair<RecordType::SCATTERVNCHWCONV, ScatterVnchwconvRecord>,
    TypePair<RecordType::SCATTERVNCHWCONV_A5, ScatterVnchwconvRecord>, TypePair<RecordType::LOAD_B2, LoadB2Record>,
    TypePair<RecordType::LOAD_A_WINOGRAD, LoadAWinogradRecord>,
    TypePair<RecordType::LOAD_B_WINOGRAD, LoadBWinogradRecord>, TypePair<RecordType::LOAD_3D, Load3DRecord>,
    TypePair<RecordType::LOAD_3D_V2, Load3DV2Record>, TypePair<RecordType::SET_2D, Set2DRecord>,
    TypePair<RecordType::LOAD_IMAGE, LoadImageRecord>, TypePair<RecordType::LOAD_SMASK, LoadSmaskRecord>,
    TypePair<RecordType::UNARY_OP, UnaryOpRecord>, TypePair<RecordType::VGATHER, VgatherRecord>,
    TypePair<RecordType::ELEMENT, ElementRecord>, TypePair<RecordType::VCOPY_OP, UnaryOpRecord>,
    TypePair<RecordType::VREDUCEV2_UNARY, UnaryOpRecord>, TypePair<RecordType::VREDUCEV2_BINARY, BinaryOpRecord>,
    TypePair<RecordType::VREDUCEV2, ReduceV2Record>, TypePair<RecordType::VMRGSORT4_OP_C220, UnaryOpRecord>,
    TypePair<RecordType::VMRGSORT4_OP_M200, UnaryOpRecord>, TypePair<RecordType::VMRGSORT4_OP_C310, Vms4v2RecordA5>,
    TypePair<RecordType::VBS32_A5, Vbs32Record>, TypePair<RecordType::VCONV_DST_S4_OP, UnaryOpRecord>,
    TypePair<RecordType::VCONV_SRC_S4_OP, UnaryOpRecord>, TypePair<RecordType::VSEL_OP, BinaryOpRecord>,
    TypePair<RecordType::BINARY_OP, BinaryOpRecord>, TypePair<RecordType::MSTX_STUB, MstxRecord>,
    TypePair<RecordType::TERNARY_OP, BinaryOpRecord>, TypePair<RecordType::REDUCE_OP, ReduceOpRecord>,
    TypePair<RecordType::CMPMASK_OP, CmpMaskRecord>, TypePair<RecordType::MATRIX_MUL_OP, MarixMulOpRecord>,
    TypePair<RecordType::VEC_REGPROPCOOR_OP, VecRegPropCoordOpRecord>, TypePair<RecordType::SET_FLAG, SyncRecord>,
    TypePair<RecordType::SET_FLAGI, SyncRecord>, TypePair<RecordType::WAIT_FLAG, SyncRecord>,
    TypePair<RecordType::WAIT_FLAGI, SyncRecord>, TypePair<RecordType::GET_BUF, BufRecord>,
    TypePair<RecordType::GET_BUFI, BufRecord>, TypePair<RecordType::RLS_BUF, BufRecord>,
    TypePair<RecordType::RLS_BUFI, BufRecord>, TypePair<RecordType::GET_BUF_V, BufRecord>,
    TypePair<RecordType::GET_BUFI_V, BufRecord>, TypePair<RecordType::RLS_BUF_V, BufRecord>,
    TypePair<RecordType::RLS_BUFI_V, BufRecord>, TypePair<RecordType::HSET_FLAG, HardSyncRecord>,
    TypePair<RecordType::HWAIT_FLAG, HardSyncRecord>, TypePair<RecordType::HSET_FLAGI, HardSyncRecord>,
    TypePair<RecordType::HWAIT_FLAGI, HardSyncRecord>, TypePair<RecordType::FFTS_SYNC, FftsSyncRecord>,
    TypePair<RecordType::WAIT_FLAG_DEV, WaitFlagDevRecord>,
    TypePair<RecordType::WAIT_FLAG_DEV_PIPE, WaitFlagDevPipeRecord>,
    TypePair<RecordType::WAIT_FLAG_DEVI_PIPE, WaitFlagDevPipeRecord>,
    TypePair<RecordType::SET_INTRA_BLOCK, IntraBlockSyncRecord>,
    TypePair<RecordType::WAIT_INTRA_BLOCK, IntraBlockSyncRecord>,
    TypePair<RecordType::SET_INTRA_BLOCKI, IntraBlockSyncRecord>,
    TypePair<RecordType::WAIT_INTRA_BLOCKI, IntraBlockSyncRecord>, TypePair<RecordType::SET_FLAG_V, SyncRecord>,
    TypePair<RecordType::SET_FLAGI_V, SyncRecord>, TypePair<RecordType::WAIT_FLAG_V, SyncRecord>,
    TypePair<RecordType::WAIT_FLAGI_V, SyncRecord>, TypePair<RecordType::FFTS_SYNC_V, FftsSyncRecord>,
    TypePair<RecordType::WAIT_FLAG_DEV_PIPE_V, WaitFlagDevPipeRecord>,
    TypePair<RecordType::WAIT_FLAG_DEVI_PIPE_V, WaitFlagDevPipeRecord>,
    TypePair<RecordType::SET_INTRA_BLOCK_V, IntraBlockSyncRecord>,
    TypePair<RecordType::SET_INTRA_BLOCKI_V, IntraBlockSyncRecord>,
    TypePair<RecordType::WAIT_INTRA_BLOCK_V, IntraBlockSyncRecord>,
    TypePair<RecordType::WAIT_INTRA_BLOCKI_V, IntraBlockSyncRecord>,
    TypePair<RecordType::PIPE_BARRIER, PipeBarrierRecord>, TypePair<RecordType::SET_ATOMIC, AtomicModeRecord>,
    TypePair<RecordType::IB_SET_STUB, SoftSyncRecord>, TypePair<RecordType::IB_WAIT_STUB, SoftSyncRecord>,
    TypePair<RecordType::SYNC_ALL_STUB, SoftSyncRecord>, TypePair<RecordType::DMA_MOV_ND2NZ_D, DmaMovNd2nzDavRecord>,
    TypePair<RecordType::DMA_MOV_DN2NZ_D, DmaMovNd2nzDavRecord>, TypePair<RecordType::SCALAR_RED, RedRecord>,
    TypePair<RecordType::SCALAR_ATOM, RedRecord>, TypePair<RecordType::LDVA, LoadStoreRecord>,
    TypePair<RecordType::SET_L1_2D, SetL12DRecord>, TypePair<RecordType::MOV_L1_TO_UB, MovL1UBRecord>,
    TypePair<RecordType::MOV_UB_TO_L1, MovL1UBRecord>, TypePair<RecordType::MOV_UB_TO_UB, MovL1UBRecord>,
    TypePair<RecordType::MMAD_A5, MmadA5Record>, TypePair<RecordType::MOV_CBUF_TO_BT, MovL1BtRecord>,
    TypePair<RecordType::MOV_CBUF_TO_FB, MovL1FbRecord>, TypePair<RecordType::SET_VECTOR_MASK_0, RegisterSetRecord>,
    TypePair<RecordType::SET_VECTOR_MASK_1, RegisterSetRecord>, TypePair<RecordType::SET_CTRL, RegisterSetRecord>,
    TypePair<RecordType::SET_FFTS_BASE_ADDR, RegisterSetRecord>, TypePair<RecordType::SET_FPC, RegisterSetRecord>,
    TypePair<RecordType::SET_QUANT_PRE, RegisterSetRecord>, TypePair<RecordType::SET_QUANT_POST, RegisterSetRecord>,
    TypePair<RecordType::SET_LRELU_ALPHA, RegisterSetRecord>, TypePair<RecordType::DCCI, DcciRecord>>;

template<RecordType recordType>
using RecordMapping = RecordTypeMap::type<recordType>;

// 为了方便使用定义的模板，在编译期检查枚举类型和数据类型是否匹配
// example:
//   is_record_match<RecordType::SET_FLAG, SyncRecord>::value => true
//   is_record_match<RecordType::HSET_FLAG, HardSyncRecord>::value => true
//   is_record_match<RecordType::HSET_FLAG, SyncRecord>::value => false
template <RecordType key, typename Record>
struct is_record_match {
    static constexpr bool value = std::is_same<Record, RecordMapping<key>>::value;
};

// 用于类型检查的开关，用类型而不是直接用bool是为了提高可读性
template<bool enabled = true>
struct record_type_check {
    static constexpr bool value = enabled;
};

}  // namespace Sanitizer

#endif // !PLUGIN_RECORD_TYPE_MAP_H
