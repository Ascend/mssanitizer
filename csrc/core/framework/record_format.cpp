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


#include "record_format.h"
#include "file_mapping.h"
#include "record_defs.h"

#include <functional>
#include <ios>

namespace {

struct NonEmptyString {
    NonEmptyString(std::string const &str, std::string const &placeholder = "<unknown>")
        : str{str}, placeholder{placeholder} { }
    std::string const &str;
    std::string const &placeholder;
};

std::ostream &operator<<(std::ostream &os, NonEmptyString const &nonEmptyString)
{
    return os << (nonEmptyString.str.empty() ? nonEmptyString.placeholder : nonEmptyString.str);
}

} // namespace [Dummy]

namespace Sanitizer {

static const std::map<RecordType, std::string> RECORD_TYPE_MAP = {
    {RecordType::LOAD,                        "LOAD"},
    {RecordType::STORE,                       "STORE"},
    {RecordType::LD,                          "LD"},
    {RecordType::LD_IO,                       "LD_IO"},
    {RecordType::ST,                          "ST"},
    {RecordType::ST_IO,                       "ST_IO"},
    {RecordType::STP,                         "STP"},
    {RecordType::STI,                         "STI"},
    {RecordType::STI_IO,                      "STI_IO"},
    {RecordType::LDP,                         "LDP"},
    {RecordType::ST_ATOMIC,                   "ST_ATOMIC"},
    {RecordType::STI_ATOMIC,                  "STI_ATOMIC"},
    {RecordType::ST_DEV,                      "ST_DEV"},
    {RecordType::LD_DEV,                      "LD_DEV"},
    {RecordType::DMA_MOV,                     "DMA_MOV"},
    {RecordType::DMA_MOV_CONV_RELU,           "DMA_MOV_CONV_RELU"},
    {RecordType::DMA_MOV_DEPTH_WISE,          "DMA_MOV_DEPTH_WISE"},
    {RecordType::DMA_MOV_ND2NZ,               "DMA_MOV_ND2NZ"},
    {RecordType::DMA_MOV_ND2NZ_D,             "DMA_MOV_ND2NZ_D"},
    {RecordType::DMA_MOV_DN2NZ_D,             "DMA_MOV_DN2NZ_D"},
    {RecordType::MOV_ALIGN,                   "MOV_ALIGN"},
    {RecordType::MOV_ALIGN_V2,                "MOV_ALIGN_V2"},
    {RecordType::ND_DMA_OUT_TO_UB,            "ND_DMA_OUT_TO_UB"},
    {RecordType::MOV_BT,                      "MOV_BT"},
    {RecordType::MOV_FP,                      "MOV_FP"},
    {RecordType::FIX_L0C_TO_L1,               "FIX_L0C_TO_L1"},
    {RecordType::FIX_L0C_TO_UB,               "FIX_L0C_TO_UB"},
    {RecordType::VEC_DUP,                     "VEC_DUP"},
    {RecordType::LOAD_2D,                     "LOAD_2D"},
    {RecordType::LOAD_L1_2D,                  "LOAD_L1_2D"},
    {RecordType::LOAD_L1_MX_2D,               "LOAD_L1_MX_2D"},
    {RecordType::LOAD_L1_2D_TRANSPOSE,        "LOAD_L1_2D_TRANSPOSE"},
    {RecordType::LOAD_2D_SPARSE,              "LOAD_2D_SPARSE"},
    {RecordType::LOAD_2D_TRANSPOSE,           "LOAD_2D_TRANSPOSE"},
    {RecordType::DECOMPRESS_HEADER,           "DECOMPRESS_HEADER"},
    {RecordType::BROADCAST,                   "BROADCAST"},
    {RecordType::DC_PRELOAD,                  "DC_PRELOAD"},
    {RecordType::SCATTERVNCHWCONV,            "SCATTERVNCHWCONV"},
    {RecordType::SCATTERVNCHWCONV_A5,         "SCATTERVNCHWCONV"},
    {RecordType::LOAD_B2,                     "LOAD_B2"},
    {RecordType::LOAD_A_WINOGRAD,             "LOAD_A_WINOGRAD"},
    {RecordType::LOAD_B_WINOGRAD,             "LOAD_B_WINOGRAD"},
    {RecordType::LOAD_3D,                     "LOAD_3D"},
    {RecordType::LOAD_3D_V2,                  "LOAD_3D_V2"},
    {RecordType::SET_2D,                      "SET_2D"},
    {RecordType::LOAD_IMAGE,                  "LOAD_IMAGE"},
    {RecordType::LOAD_SMASK,                  "LOAD_SMASK"},
    {RecordType::UNARY_OP,                    "UNARY_OP"},
    {RecordType::VGATHER,                     "VGATHER"},
    {RecordType::ELEMENT,                     "ELEMENT"},
    {RecordType::VCOPY_OP,                    "VCOPY_OP"},
    {RecordType::VREDUCEV2_UNARY,             "VREDUCEV2"},
    {RecordType::VREDUCEV2_BINARY,            "VREDUCEV2"},
    {RecordType::VREDUCEV2,                   "VREDUCEV2"},
    {RecordType::VCONV_DST_S4_OP,             "VCONV_OP"},
    {RecordType::VCONV_SRC_S4_OP,             "VCONV_OP"},
    {RecordType::VSEL_OP,                     "VSEL_OP"},
    {RecordType::VMRGSORT4_OP_C220,           "VMRGSORT4_OP"},
    {RecordType::VMRGSORT4_OP_M200,           "VMRGSORT4_OP"},
    {RecordType::VMRGSORT4_OP_C310,           "VMRGSORT4_OP"},
    {RecordType::VBS32_A5,                    "VBS32"},
    {RecordType::MSTX_STUB,                   "MSTX_STUB"},
    {RecordType::BINARY_OP,                   "BINARY_OP"},
    {RecordType::TERNARY_OP,                  "TERNARY_OP"},
    {RecordType::REDUCE_OP,                   "REDUCE_OP"},
    {RecordType::CMPMASK_OP,                  "CMPMASK_OP"},
    {RecordType::MATRIX_MUL_OP,               "MATRIX_MUL_OP"},
    {RecordType::MMAD_A5,                     "MMAD"},
    {RecordType::VEC_REGPROPCOOR_OP,          "VEC_REGPROPCOOR_OP"},
    {RecordType::SET_FLAG,                    "SET_FLAG"},
    {RecordType::SET_FLAGI,                   "SET_FLAGI"},
    {RecordType::WAIT_FLAG,                   "WAIT_FLAG"},
    {RecordType::WAIT_FLAGI,                  "WAIT_FLAGI"},
    {RecordType::GET_BUF,                    "GET_BUF"},
    {RecordType::GET_BUFI,                    "GET_BUFI"},
    {RecordType::RLS_BUF,                    "RLS_BUF"},
    {RecordType::RLS_BUFI,                    "RLS_BUFI"},
    {RecordType::GET_BUF_V,                    "GET_BUF_V"},
    {RecordType::GET_BUFI_V,                    "GET_BUFI_V"},
    {RecordType::RLS_BUF_V,                    "RLS_BUF_V"},
    {RecordType::RLS_BUFI_V,                    "RLS_BUFI_V"},
    {RecordType::HSET_FLAG,                   "HSET_FLAG"},
    {RecordType::HWAIT_FLAG,                  "HWAIT_FLAG"},
    {RecordType::HSET_FLAGI,                  "HSET_FLAGI"},
    {RecordType::HWAIT_FLAGI,                 "HWAIT_FLAGI"},
    {RecordType::PIPE_BARRIER,                "PIPE_BARRIER"},
    {RecordType::FFTS_SYNC,                   "FFTS_SYNC"},
    {RecordType::WAIT_FLAG_DEV,               "WAIT_FLAG_DEV"},
    {RecordType::WAIT_FLAG_DEV_PIPE,          "WAIT_FLAG_DEV"},
    {RecordType::WAIT_FLAG_DEVI_PIPE,         "WAIT_FLAG_DEVI"},
    {RecordType::SET_INTRA_BLOCK,             "SET_INTRA_BLOCK"},
    {RecordType::WAIT_INTRA_BLOCK,            "WAIT_INTRA_BLOCK"},
    {RecordType::SET_INTRA_BLOCKI,            "SET_INTRA_BLOCKI"},
    {RecordType::WAIT_INTRA_BLOCKI,           "WAIT_INTRA_BLOCKI"},
    {RecordType::SET_FLAG_V,                  "SET_FLAG_V"},
    {RecordType::SET_FLAGI_V,                 "SET_FLAGI_V"},
    {RecordType::WAIT_FLAG_V,                 "WAIT_FLAG_V"},
    {RecordType::WAIT_FLAGI_V,                "WAIT_FLAGI_V"},
    {RecordType::FFTS_SYNC_V,                 "FFTS_SYNC_V"},
    {RecordType::WAIT_FLAG_DEV_PIPE_V,        "WAIT_FLAG_DEV_V"},
    {RecordType::WAIT_FLAG_DEVI_PIPE_V,       "WAIT_FLAG_DEVI_V"},
    {RecordType::SET_INTRA_BLOCK_V,           "SET_INTRA_BLOCK_V"},
    {RecordType::SET_INTRA_BLOCKI_V,          "SET_INTRA_BLOCKI_V"},
    {RecordType::WAIT_INTRA_BLOCK_V,          "WAIT_INTRA_BLOCK_V"},
    {RecordType::WAIT_INTRA_BLOCKI_V,         "WAIT_INTRA_BLOCKI_V"},
    {RecordType::SET_ATOMIC,                  "SET_ATOMIC"},
    {RecordType::IB_SET_STUB,                 "IB_SET_STUB"},
    {RecordType::IB_WAIT_STUB,                "IB_WAIT_STUB"},
    {RecordType::SYNC_ALL_STUB,               "SYNC_ALL_STUB"},
    {RecordType::SIMT_LDG,                    "SIMT_LDG"},
    {RecordType::SIMT_STG,                    "SIMT_STG"},
    {RecordType::SIMT_LDS,                    "SIMT_LDS"},
    {RecordType::SIMT_STS,                    "SIMT_STS"},
    {RecordType::SIMT_LDK,                    "SIMT_LDK"},
    {RecordType::SIMT_STK,                    "SIMT_STK"},
    {RecordType::SIMT_LD,                     "SIMT_LD"},
    {RecordType::SIMT_ST,                     "SIMT_ST"},
    {RecordType::SIMT_ATOM,                   "SIMT_ATOM"},
    {RecordType::SIMT_RED,                    "SIMT_RED"},
    {RecordType::ONLINE_ERROR,                "ONLINE_ERROR"},
    {RecordType::SCALAR_RED,                  "SCALAR_RED"},
    {RecordType::SCALAR_ATOM,                 "SCALAR_ATOM"},
    {RecordType::LDVA,                        "LDVA"},
    {RecordType::FINISH,                      "FINISH"},
    {RecordType::KERNEL_FINISH,               "KERNEL_FINISH"},
    {RecordType::BLOCK_FINISH,                "BLOCK_FINISH"},
    {RecordType::SET_L1_2D,                   "SET_L1_2D"},
    {RecordType::MOV_L1_TO_UB,                "MOV_L1_TO_UB"},
    {RecordType::MOV_UB_TO_L1,                "MOV_UB_TO_L1"},
    {RecordType::MOV_UB_TO_UB,                "MOV_UB_TO_UB"},
    {RecordType::MOV_CBUF_TO_BT,              "MOV_CBUF_TO_BT"},
    {RecordType::MOV_CBUF_TO_FB,              "MOV_CBUF_TO_FB"},
    {RecordType::SIMT_ENTRY,                  "SIMT_ENTRY"},
    {RecordType::DYNAMIC_OP,                  "DYNAMIC_OP"},
    {RecordType::SET_VECTOR_MASK_0,           "SET_VECTOR_MASK_0"},
    {RecordType::SET_VECTOR_MASK_1,           "SET_VECTOR_MASK_1"},
    {RecordType::SET_CTRL,                    "SET_CTRL"},
    {RecordType::SET_FFTS_BASE_ADDR,          "SET_FFTS_BASE_ADDR"},
    {RecordType::SET_FPC,                     "SET_FPC"},
    {RecordType::SET_QUANT_PRE,               "SET_QUANT_PRE"},
    {RecordType::SET_QUANT_POST,              "SET_QUANT_POST"},
    {RecordType::SET_LRELU_ALPHA,             "SET_LRELU_ALPHA"},
    {RecordType::THREAD_BLOCK_BARRIER,        "THREAD_BLOCK_BARRIER"},
    {RecordType::SIMT_START,                  "SIMT_START"},
    {RecordType::SIMT_END,                    "SIMT_END"},
    {RecordType::SIMT_CALL,                   "SIMT_CALL"},
};

std::ostream &operator<<(std::ostream &os, RecordType recordType)
{
    return FormatEnum(os, RECORD_TYPE_MAP, recordType, "RecordType");
}

static const std::map<KernelErrorType, std::string> ERROR_TYPE_MAP = {
    {KernelErrorType::ILLEGAL_ADDR_WRITE, "ILLEGAL_ADDR_WRITE"},
    {KernelErrorType::ILLEGAL_ADDR_READ, "ILLEGAL_ADDR_READ"},
    {KernelErrorType::MISALIGNED_ACCESS, "MISALIGNED_ACCESS"},
    {KernelErrorType::THREAD_OVERLAP, "THREAD_OVERLAP"},
    {KernelErrorType::THREAD_RW_RACE, "THREAD_RW_RACE"},
    {KernelErrorType::THREAD_WR_RACE, "THREAD_WR_RACE"},
    {KernelErrorType::THREAD_WW_RACE, "THREAD_WW_RACE"},
    {KernelErrorType::THREADS_ASYNC_IN_BLOCK, "THREADS_ASYNC_IN_BLOCK"},
    {KernelErrorType::SYNC_THREADS_RECORD_LOSS, "SYNC_THREADS_RECORD_LOSS"},
};

std::ostream &operator<<(std::ostream &os, KernelErrorType errorType) {
    return FormatEnum(os, ERROR_TYPE_MAP, errorType, "KernelErrorType");
}

std::ostream &operator<<(std::ostream &os, InterfaceType interfaceType)
{
    static const std::map<InterfaceType, std::string> INTERFACE_TYPE_MAP = {
        {InterfaceType::MSTX_SET_CROSS_SYNC,       "SET_CROSS"},
        {InterfaceType::MSTX_WAIT_CROSS_SYNC,      "WAIT_CROSS"},
        {InterfaceType::MSTX_HCCL,                 "HCCL"},
        {InterfaceType::MSTX_HCCLV,                "HCCLV"},
        {InterfaceType::MSTX_CROSS_CORE_BARRIER,   "CROSS_CORE_BARRIER"},
        {InterfaceType::MSTX_CROSS_CORE_SET_FLAG,  "CROSS_CORE_SET_FLAG"},
        {InterfaceType::MSTX_CROSS_CORE_WAIT_FLAG, "CROSS_CORE_WAIT_FLAG"},
        {InterfaceType::MSTX_SIGNAL_SET,           "SIGNAL_SET"},
        {InterfaceType::MSTX_SIGNAL_WAIT,          "SIGNAL_WAIT"},
        {InterfaceType::MSTX_CROSS_NPU_BARRIER,    "CROSS_NPU_BARRIER"},
        {InterfaceType::MSTX_VEC_UNARY_OP,         "VEC_UNARY"},
        {InterfaceType::MSTX_VEC_BINARY_OP,        "VEC_BINARY"},
        {InterfaceType::MSTX_DATA_COPY,            "DATA_COPY"},
        {InterfaceType::MSTX_DATA_COPY_PAD,        "DATA_COPY_PAD"},
    };

    return FormatEnum(os, INTERFACE_TYPE_MAP, interfaceType, "InterfaceType");
}

std::ostream &operator<<(std::ostream &os, DataType dataType)
{
    static const std::map<DataType, std::string> DATA_TYPE_MAP = {
        {DataType::DATA_B4,  "B4"},
        {DataType::DATA_B8,  "B8"},
        {DataType::DATA_B16, "B16"},
        {DataType::DATA_B32, "B32"},
    };

    return FormatEnum(os, DATA_TYPE_MAP, dataType, "DataType");
}

std::ostream &operator<<(std::ostream &os, PadMode padMode)
{
    static const std::map<PadMode, std::string> PAD_MODE_MAP = {
        {PadMode::PAD_NONE,  "NONE"},
        {PadMode::PAD_MODE1, "MODE1"},
        {PadMode::PAD_MODE2, "MODE2"},
        {PadMode::PAD_MODE3, "MODE3"},
        {PadMode::PAD_MODE4, "MODE4"},
        {PadMode::PAD_MODE5, "MODE5"},
        {PadMode::PAD_MODE6, "MODE6"},
        {PadMode::PAD_MODE7, "MODE7"},
        {PadMode::PAD_MODE8, "MODE8"},
    };

    return FormatEnum(os, PAD_MODE_MAP, padMode, "PadType");
}

std::ostream &operator<<(std::ostream &os, ConvRelu convRelu)
{
    static const std::map<ConvRelu, std::string> CONV_RELU_MAP = {
        {ConvRelu::CRMODE_NONE,  "CRMODE_NONE"},
        {ConvRelu::CRMODE_F32toF16_NONE, "CRMODE_F32toF16_NONE"},
        {ConvRelu::CRMODE_F32toF16_RELU, "CRMODE_F32toF16_RELU"},
        {ConvRelu::CRMODE_S32toF16_NONE, "CRMODE_S32toF16_NONE"},
        {ConvRelu::CRMODE_F16toF32_NONE, "CRMODE_F16toF32_NONE"},
        {ConvRelu::CRMODE_NONE_RELU, "CRMODE_NONE_RELU"},
        {ConvRelu::CRMODE_F16_MUL, "CRMODE_F16_MUL"},
        {ConvRelu::CRMODE_S32toF16_DEQSCALE_SPR, "CRMODE_S32toF16_DEQSCALE_SPR"},
        {ConvRelu::CRMODE_DEQSCALE_VDEQ8, "CRMODE_DEQSCALE_VDEQ8"},
        {ConvRelu::CRMODE_DEQSCALE_DEQ8, "CRMODE_DEQSCALE_DEQ8"},
        {ConvRelu::CRMODE_DEQSCALE_VDEQ16, "CRMODE_DEQSCALE_VDEQ16"},
        {ConvRelu::CRMODE_DEQSCALE_DEQ16, "CRMODE_DEQSCALE_DEQ16"},
        {ConvRelu::CRMODE_DEQSCALE_VDEQS16, "CRMODE_DEQSCALE_VDEQS16"},
        {ConvRelu::CRMODE_DEQSCALE_DEQS16, "CRMODE_DEQSCALE_DEQS16"},
    };

    return FormatEnum(os, CONV_RELU_MAP, convRelu, "ConvReluType");
}

std::ostream &operator<<(std::ostream &os, MemType memType)
{
    static const std::map<MemType, std::string> MEM_TYPE_MAP = {
        {MemType::L1,  "L1"},
        {MemType::L0A, "L0A"},
        {MemType::L0B, "L0B"},
        {MemType::L0C, "L0C"},
        {MemType::UB,  "UB"},
        {MemType::BT,  "BT"},
        {MemType::FB,  "FB"},
        {MemType::GM,  "GM"},
    };

    return FormatEnum(os, MEM_TYPE_MAP, memType, "MemType");
}

std::ostream &operator<<(std::ostream &os, PipeType pipeType)
{
    static const std::map<PipeType, std::string> PIPE_TYPE_MAP = {
        {PipeType::PIPE_S,    "PIPE_S"},
        {PipeType::PIPE_V,    "PIPE_V"},
        {PipeType::PIPE_M,    "PIPE_M"},
        {PipeType::PIPE_MTE1, "PIPE_MTE1"},
        {PipeType::PIPE_MTE2, "PIPE_MTE2"},
        {PipeType::PIPE_MTE3, "PIPE_MTE3"},
        {PipeType::PIPE_ALL,  "PIPE_ALL"},
        {PipeType::PIPE_MTE4, "PIPE_MTE4"},
        {PipeType::PIPE_MTE5, "PIPE_MTE5"},
        {PipeType::PIPE_V2,   "PIPE_V2"},
        {PipeType::PIPE_FIX,  "PIPE_FIX"},
        {PipeType::PIPE_S_CAL, "PIPE_S"},
    };

    return FormatEnum(os, PIPE_TYPE_MAP, pipeType, "PipeType");
}

std::ostream &operator<<(std::ostream &os, EventID evenID)
{
    return os << "EVENT_ID" << static_cast<uint32_t>(evenID);
}

std::ostream &operator<<(std::ostream &os, ByteMode byteMode)
{
    static const std::map<ByteMode, std::string> BYTE_MODE_MAP = {
        {ByteMode::BM_DISABLE, "DISABLE"},
        {ByteMode::BM_ENABLE,  "ENABLE"},
    };

    return FormatEnum(os, BYTE_MODE_MAP, byteMode, "ByteMode");
}

std::ostream &operator<<(std::ostream &os, AtomicMode atomicMode)
{
    static const std::map<AtomicMode, std::string> ATOMIC_MODE_MAP = {
        {AtomicMode::NONE, "NONE"},
        {AtomicMode::F32,  "F32"},
        {AtomicMode::F16,  "F16"},
        {AtomicMode::S16,  "S16"},
        {AtomicMode::S32,  "S32"},
        {AtomicMode::S8,   "S8"},
        {AtomicMode::BF16, "BF16"},
        {AtomicMode::SUM,  "SUM"},
        {AtomicMode::MAX,  "MAX"},
        {AtomicMode::MIN,  "MIN"},
    };

    return FormatEnum(os, ATOMIC_MODE_MAP, atomicMode, "AtomicMode");
}

std::ostream &operator<<(std::ostream &os, MemOpType memOpType)
{
    static const std::map<MemOpType, std::string> MEM_OP_TYPE_MAP = {
        {MemOpType::MALLOC,        "MALLOC"},
        {MemOpType::FREE,          "FREE"},
        {MemOpType::MEMCPY_BLOCKS, "MEMCPY_BLOCKS"},
        {MemOpType::LOAD,          "LOAD"},
        {MemOpType::STORE,         "STORE"},
        {MemOpType::GM_ADDR_OUT_OF_BOUND,   "GM_ADDR_OUT_OF_BOUND"},
    };

    return FormatEnum(os, MEM_OP_TYPE_MAP, memOpType, "MemOpType");
}

std::ostream &operator<<(std::ostream &os, AddressSpace space)
{
    static const std::map<AddressSpace, std::string> ADDR_SPACE_MAP = {
        {AddressSpace::PRIVATE,  "PRIVATE"},
        {AddressSpace::GM,  "GM"},
        {AddressSpace::L1,  "L1"},
        {AddressSpace::L0A, "L0A"},
        {AddressSpace::L0B, "L0B"},
        {AddressSpace::L0C, "L0C"},
        {AddressSpace::UB,  "UB"},
        {AddressSpace::BT,  "BT"},
        {AddressSpace::FB,  "FB"},
        {AddressSpace::INVALID,  "INVALID"},
    };

    return FormatEnum(os, ADDR_SPACE_MAP, space, "AddressSpace");
}

std::ostream &operator<<(std::ostream &os, BlockType blockType)
{
    static const std::map<BlockType, std::string> BLOCK_TYPE_MAP = {
        {BlockType::AIVEC,  "aiv"},
        {BlockType::AICUBE,  "aic"},
        {BlockType::AICORE,  "aicore"},
    };

    return FormatEnum(os, BLOCK_TYPE_MAP, blockType, "BlockType");
}

std::ostream &operator<<(std::ostream &os, AccessType accessType)
{
    static const std::map<AccessType, std::string> ACCESS_TYPE_MAP = {
        {AccessType::READ,          "READ"},
        {AccessType::WRITE,         "WRITE"},
        {AccessType::MEMCPY_BLOCKS, "MEMCPY_BLOCKS"},
    };

    return FormatEnum(os, ACCESS_TYPE_MAP, accessType, "AccessType");
}

std::ostream &operator<<(std::ostream &os, MaskMode maskMode)
{
    static const std::map<MaskMode, std::string> MASK_MODE_MAP = {
        {MaskMode::MASK_NORM,  "NORM"},
        {MaskMode::MASK_COUNT,  "COUNT"},
    };

    return FormatEnum(os, MASK_MODE_MAP, maskMode, "MaskMode");
}

std::ostream &operator<<(std::ostream &os, CompareOp cmpOp)
{
    static const std::map<CompareOp, std::string> CMP_OP_MAP = {
        {CompareOp::EQ, "EQ"},
        {CompareOp::NE, "NE"},
        {CompareOp::GT, "GT"},
        {CompareOp::GE, "GE"},
        {CompareOp::LT, "LT"},
        {CompareOp::LE, "LE"},
    };

    return FormatEnum(os, CMP_OP_MAP, cmpOp, "CompareOp");
}

std::ostream &operator<<(std::ostream &os, DeviceInfoSummary const &summary)
{
    return os << "[summary] device:" << static_cast<uint32_t>(summary.device);
}

std::ostream &operator<<(std::ostream &os, Location const &location)
{
    auto fileIdx = FileMapping::Instance().Query(location.fileNo).fileIdx;
    return os << location.blockId << ", " << "loc-" << fileIdx << "-" << location.lineNo
              << ", " << "0x" << std::hex << location.pc << std::dec;
}

std::ostream &operator<<(std::ostream &os, VectorMask const &vectorMask)
{
    return os <<"(" << ConvertVecMaskValue(vectorMask.mask1)
              <<"," << ConvertVecMaskValue(vectorMask.mask0) << ")";
}

std::ostream &operator<<(std::ostream &os, CompareMask const &compareMask)
{
    return os <<"(" << ConvertVecMaskValue(compareMask.mask1)
              <<"," << ConvertVecMaskValue(compareMask.mask0) << ")";
}

std::ostream &operator<<(std::ostream &os, VaRegister const &vaRegister)
{
    return os << std::hex << "(" << "0x" << vaRegister.h64 << ", " <<
        "0x" << vaRegister.l64 << ")" << std::dec;
}

std::ostream &operator<<(std::ostream &os, LoadStoreRecord const &record)
{
    return os << record.location
              << ", " << "type:" << record.space
              << ";" << "addr:0x" << std::hex << record.addr << std::dec
              << ";" << "size:" << record.size
              << ";" << "datatype:" << record.dataType;
}

std::ostream &operator<<(std::ostream &os, MstxCrossRecord const &record)
{
    return os << ", " << "addr:0x" << std::hex << record.addr << std::dec
              << ", " << "flagid:" << record.flagId
              << ";" << "pipe:" << record.pipe
              << ";" << "ismore:" << record.isMore
              << ";" << "ismerge:" << record.isMerge;
}

std::ostream &operator<<(std::ostream &os, MstxHcclRecord const &record)
{
    return os << ", " << "src:0x" << std::hex << record.src << std::dec
              << ", " << "dst:0x" << std::hex << record.dst << std::dec
              << ", " << "srcCount:" << record.srcCount
              << ", " << "dstCount:" << record.dstCount
              << ", " << "srcStride:" << record.srcStride
              << ", " << "dstStride:" << record.dstStride
              << ", " << "srcRepeatStride:" << record.srcRepeatStride
              << ", " << "dstRepeatStride:" << record.dstRepeatStride
              << ", " << "srcDataTypeSize:" << record.srcDataTypeSize
              << ", " << "dstDataTypeSize:" << record.dstDataTypeSize
              << ", " << "repeat:" << record.repeat
              << ", " << "rankDim:" << record.rankDim
              << ", " << "flagId:" << record.flagId;
}

std::ostream &operator<<(std::ostream &os, MstxHcclCoreRecord const &record)
{
    return os << ", " << "src:0x" << std::hex << record.src << std::dec
              << ", " << "dst:0x" << std::hex << record.dst << std::dec
              << ", " << "srcLen:" << record.srclenBurst
              << ", " << "dstLen:" << record.dstlenBurst
              << ", " << "repeat:" << record.repeat;
}

std::ostream &operator<<(std::ostream &os, MstxCrossCoreBarrier const &record)
{
    return os << ", " << "usedCoreNum:" << record.usedCoreNum
              << ", " << "usedCoreId:" << record.usedCoreId
              << ", " << "isAIVOnly:" << std::boolalpha << record.isAIVOnly
              << ", " << "pipeBarrierAll:" << std::boolalpha << record.pipeBarrierAll;
}

std::ostream &operator<<(std::ostream &os, MstxCrossCoreSetFlag const &record)
{
    return os << ", " << "eventId:" << record.eventId
              << ", " << "peerCoreId:" << record.peerCoreId
              << ", " << "pipeBarrierAll:" << std::boolalpha << record.pipeBarrierAll;
}

std::ostream &operator<<(std::ostream &os, MstxCrossCoreWaitFlag const &record)
{
    return os << ", " << "eventId:" << record.eventId
              << ", " << "peerCoreId:" << record.peerCoreId
              << ", " << "pipeBarrierAll:" << std::boolalpha << record.pipeBarrierAll;
}

std::ostream &operator<<(std::ostream &os, MstxSignalSet const &record)
{
    return os << ", " << "addr:0x" << std::hex << record.addr << std::dec
              << ", " << "value:" << record.value;
}

std::ostream &operator<<(std::ostream &os, MstxSignalWait const &record)
{
    return os << ", " << "addr:0x" << std::hex << record.addr << std::dec
              << ", " << "cmpValue:" << record.cmpValue
              << ", " << "cmpOp:" << record.cmpOp;
}

std::ostream &operator<<(std::ostream &os, MstxCrossNpuBarrier const &record)
{
    return os << ", " << "usedDeviceNum:" << record.usedDeviceNum
              << ", " << "usedDeviceId:" << record.usedDeviceId
              << ", " << "usedCoreNum:" << record.usedCoreNum
              << ", " << "usedCoreId:" << record.usedCoreId
              << ", " << "isAIVOnly:" << std::boolalpha << record.isAIVOnly;
}

std::ostream &operator<<(std::ostream &os, MstxTensorDesc const &tensor)
{
  return os << "(addr:0x" << std::hex << tensor.addr << std::dec
            << ",size:" << tensor.size
            << ",space:" << tensor.space
            << ",dataBits:" << static_cast<uint32_t>(tensor.dataBits) << ")";
}

std::ostream &operator<<(std::ostream &os, MstxVecWrapper const &record)
{
    return os << "maskMode:" << record.maskMode
              << ", " << "mask:" << record.mask
              << ", " << "reserveBufSize:" << record.reserveBufSize
              << ", " << "useMask:" << std::boolalpha << record.useMask;
}

std::ostream &operator<<(std::ostream &os, MstxVecUnaryDesc const &record)
{
    return os << ", " << "dst:" << record.dst
              << ", " << "src:" << record.src
              << ", " << record.wrapper
              << ", " << "blockNum:" << record.blockNum
              << ", " << "dstBlockStride:" << static_cast<uint32_t>(record.dstBlockStride)
              << ", " << "srcBlockStride:" << static_cast<uint32_t>(record.srcBlockStride)
              << ", " << "repeatTimes:" << static_cast<uint32_t>(record.repeatTimes)
              << ", " << "dstRepeatStride:" << static_cast<uint32_t>(record.dstRepeatStride)
              << ", " << "srcRepeatStride:" << static_cast<uint32_t>(record.srcRepeatStride)
              << ", " << "name:" << NonEmptyString{record.name};
}

std::ostream &operator<<(std::ostream &os, MstxVecBinaryDesc const &record)
{
    return os << ", " << "dst:" << record.dst
              << ", " << "src0:" << record.src0
              << ", " << "src1:" << record.src1
              << ", " << record.wrapper
              << ", " << "blockNum:" << record.blockNum
              << ", " << "dstBlockStride:" << static_cast<uint32_t>(record.dstBlockStride)
              << ", " << "src0BlockStride:" << static_cast<uint32_t>(record.src0BlockStride)
              << ", " << "src1BlockStride:" << static_cast<uint32_t>(record.src1BlockStride)
              << ", " << "repeatTimes:" << static_cast<uint32_t>(record.repeatTimes)
              << ", " << "dstRepeatStride:" << static_cast<uint32_t>(record.dstRepeatStride)
              << ", " << "src0RepeatStride:" << static_cast<uint32_t>(record.src0RepeatStride)
              << ", " << "src1RepeatStride:" << static_cast<uint32_t>(record.src1RepeatStride)
              << ", " << "name:" << NonEmptyString{record.name};
}

std::ostream &operator<<(std::ostream &os, MstxDataCopyDesc const &record)
{
    return os << ", " << "dst:" << record.dst
              << ", " << "src:" << record.src
              << ", " << "lenBurst:" << record.lenBurst
              << ", " << "nBurst:" << record.nBurst
              << ", " << "srcGap:" << record.srcGap
              << ", " << "dstGap:" << record.dstGap
              << ", " << "name:" << NonEmptyString{record.name};
}

std::ostream &operator<<(std::ostream &os, MstxDataCopyPadDesc const &record)
{
    return os << ", " << "dst:" << record.dst
              << ", " << "src:" << record.src
              << ", " << "lenBurst:" << record.lenBurst
              << ", " << "nBurst:" << record.nBurst
              << ", " << "srcGap:" << record.srcGap
              << ", " << "dstGap:" << record.dstGap
              << ", " << "leftPad:" << record.leftPad
              << ", " << "rightPad:" << record.rightPad
              << ", " << "name:" << NonEmptyString{record.name};
}

std::ostream &operator<<(std::ostream &os, MstxRecord const &record)
{
    os << record.location
       << ", " << "interfaceType:" << record.interfaceType
       << ";" << "bufferlens:" << record.bufferLens
       << ";" << "error:" << record.error;

    using MstxRecordStreamFunc = std::function<void(std::ostream &, MstxRecord const &)>;
    static const std::map<InterfaceType, MstxRecordStreamFunc> MSTX_RECORD_FORMAT_MAP = {
        {InterfaceType::MSTX_SET_CROSS_SYNC,
            [](std::ostream &os, MstxRecord const &r) { os << r.interface.mstxCrossRecord; }},
        {InterfaceType::MSTX_WAIT_CROSS_SYNC,
            [](std::ostream &os, MstxRecord const &r) { os << r.interface.mstxCrossRecord; }},
        {InterfaceType::MSTX_HCCL,
            [](std::ostream &os, MstxRecord const &r) { os << r.interface.mstxHcclRecord; }},
        {InterfaceType::MSTX_HCCLV,
            [](std::ostream &os, MstxRecord const &r) { os << r.interface.mstxHcclCoreRecord; }},
        {InterfaceType::MSTX_CROSS_CORE_BARRIER,
            [](std::ostream &os, MstxRecord const &r) { os << r.interface.mstxCrossCoreBarrier; }},
        {InterfaceType::MSTX_CROSS_CORE_SET_FLAG,
            [](std::ostream &os, MstxRecord const &r) { os << r.interface.mstxCrossCoreSetFlag; }},
        {InterfaceType::MSTX_CROSS_CORE_WAIT_FLAG,
            [](std::ostream &os, MstxRecord const &r) { os << r.interface.mstxCrossCoreWaitFlag; }},
        {InterfaceType::MSTX_SIGNAL_SET,
            [](std::ostream &os, MstxRecord const &r) { os << r.interface.mstxSignalSet; }},
        {InterfaceType::MSTX_SIGNAL_WAIT,
            [](std::ostream &os, MstxRecord const &r) { os << r.interface.mstxSignalWait; }},
        {InterfaceType::MSTX_CROSS_NPU_BARRIER,
            [](std::ostream &os, MstxRecord const &r) { os << r.interface.mstxCrossNpuBarrier; }},
        {InterfaceType::MSTX_VEC_UNARY_OP,
            [](std::ostream &os, MstxRecord const &r) { os << r.interface.mstxVecUnaryDesc; }},
        {InterfaceType::MSTX_VEC_BINARY_OP,
            [](std::ostream &os, MstxRecord const &r) { os << r.interface.mstxVecBinaryDesc; }},
        {InterfaceType::MSTX_DATA_COPY,
            [](std::ostream &os, MstxRecord const &r) { os << r.interface.mstxDataCopyDesc; }},
        {InterfaceType::MSTX_DATA_COPY_PAD,
            [](std::ostream &os, MstxRecord const &r) { os << r.interface.mstxDataCopyPadDesc; }},
    };

    typename decltype(MSTX_RECORD_FORMAT_MAP)::const_iterator it = MSTX_RECORD_FORMAT_MAP.find(record.interfaceType);
    if (it == MSTX_RECORD_FORMAT_MAP.cend()) {
        return os;
    }

    it->second(os, record);
    return os;
}

std::ostream &operator<<(std::ostream &os, DmaMovRecord const &record)
{
    return os << record.location
              << ", dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ");" << "nburst:" << record.nBurst
              << ";" << "lenburst:" << record.lenBurst
              << ";" << "padmode:" << record.padMode
              << ";" << "bytemode:" << record.byteMode;
}

std::ostream &operator<<(std::ostream &os, DmaMovConvReluRecord const &record)
{
    return os << record.location
              << ", dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ");" << "nburst:" << record.nBurst
              << ";" << "lenburst:" << record.lenBurst
              << ";" << "crMode:" << record.crMode
              << ";" << "srcDataType:" << record.srcDataType
              << ";" << "dstDataType:" << record.dstDataType;
}

std::ostream &operator<<(std::ostream &os, DmaMovNd2nzRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "nzcotride:" << record.dstNzC0Stride
              << ";" << "nznstride:" << record.dstNzNStride
              << ";" << "nzmatrixstride:" << record.dstNzMatrixStride
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "ndmatrixstride:" << record.srcNdMatrixStride
              << ";" << "dvalue:" << record.srcDValue
              << ");" << "n:(dnum:" << record.ndNum
              << ";" << "value:" << record.nValue
              << ");" << "dvalue:" << record.dValue
              << ";" << "datatype:" << record.dataType;
}

std::ostream &operator<<(std::ostream &os, MovAlignRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "gap:" << record.dstGap
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "gap:" << record.srcGap
              << ");" << "nburst:" << record.nBurst
              << ";" << "lenburst:" << record.lenBurst
              << ";" << "dataType:" << record.dataType
              << ";" << "leftpaddingnum:" << static_cast<uint32_t>(record.leftPaddingNum)
              << ";" << "rightpaddingnum:" << static_cast<uint32_t>(record.rightPaddingNum);
}

std::ostream &operator<<(std::ostream &os, MovAlignRecordV2 const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ";" << "loop1Stride:" << record.loop1DstStride
              << ";" << "loop2Stride:" << record.loop2DstStride
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ";" << "loop1Stride:" << record.loop1SrcStride
              << ";" << "loop2Stride:" << record.loop2SrcStride
              << ");" << "nburst:" << record.nBurst
              << ";" << "lenburst:" << record.lenBurst
              << ";" << "dataType:" << record.dataType
              << ";" << "loop1Size:" << record.loop1Size
              << ";" << "loop2Size:" << record.loop2Size
              << ";" << "leftpaddingnum:" << static_cast<uint32_t>(record.leftPaddingNum)
              << ";" << "rightpaddingnum:" << static_cast<uint32_t>(record.rightPaddingNum);
}

std::ostream &operator<<(std::ostream &os, LoopInfo const &loopInfo)
{
    return os << "size:" << loopInfo.loopSize
              << ";lpSize:" << static_cast<uint32_t>(loopInfo.loopLpSize)
              << ";rpSize:" << static_cast<uint32_t>(loopInfo.loopRpSize)
              << ";dstStride:" << loopInfo.loopDstStride
              << ";srcStride:" << loopInfo.loopSrcStride;
}

std::ostream &operator<<(std::ostream &os, NdDMAOut2UbRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << MemType::UB
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ");" << "src:(" << "type:" << MemType::GM
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ");" << "loop0Info:(" << record.loop[0]
              << ");" << "loop1Info:(" << record.loop[1]
              << ");" << "loop2Info:(" << record.loop[2]
              << ");" << "loop3Info:(" << record.loop[3]
              << ");" << "loop4Info:(" << record.loop[4]
              << ");" << "dataType:" << record.dataType;
}

std::ostream &operator<<(std::ostream &os, MovBtRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "gap:" << record.dstGap
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "gap:" << record.srcGap
              << ");" << "nburst:" << record.nBurst
              << ";" << "lenburst:" << record.lenBurst;
}

std::ostream &operator<<(std::ostream &os, MovFpRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ";" << "ndstride:" << record.dstNdStride
              << ");" << "src:(" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ";" << "ndstride:" << record.srcNdStride
              << ";" << "nzC0stride:" << record.srcNzC0Stride
              << ");" << "nsize:" << record.nSize
              << ";" << "msize:" << record.mSize
              << ";" << "ndnum:" << record.ndNum
              << ";" << "uintflag:" << record.enUnitFlag
              << ";" << "quantPreBits:" << record.quantPreBits
              << ";" << "int8channelmerge:" << record.int8ChannelMerge
              << ";" << "int4channelmerge:" << record.int4ChannelMerge
              << ";" << "channelsplit:" << record.channelSplit
              << ";" << "ennz2nd:" << record.enNZ2ND
              << ";" << "ennz2dn:" << record.enNZ2DN
              << ";" << "isC310:" << record.isC310;
}

std::ostream &operator<<(std::ostream &os, VecDupRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "blockstride:" << record.dstBlockStride
              << ";" << "rptstride:" << record.dstRepeatStride
              << ";" << "bits:" << static_cast<uint32_t>(record.dataBits)
              << ");" << "repeat:" << static_cast<uint32_t>(record.repeat)
              << ";"  << "maskmode:" << record.maskMode
              << ";" << "vectormask:" << record.vectorMask;
}

std::ostream &operator<<(std::ostream &os, UnaryOpRecord const &record)
{
    return os << record.location
              << ", dst:(" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "blocksize:" << record.dstBlockSize
              << ";" << "blocknum:" << static_cast<uint32_t>(record.dstBlockNum)
              << ";" << "blockstride:" << record.dstBlockStride
              << ";" << "rptstride:" << record.dstRepeatStride
              << ";" << "bits:" << static_cast<uint32_t>(record.dstDataBits)
              << ");" << "src:(" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "blocksize:" << record.srcBlockSize
              << ";" << "blocknum:" << static_cast<uint32_t>(record.srcBlockNum)
              << ";" << "blockstride:" << record.srcBlockStride
              << ";" << "rptstride:" << record.srcRepeatStride
              << ";" << "bits:" << static_cast<uint32_t>(record.srcDataBits)
              << ");" << "repeat:" << static_cast<uint32_t>(record.repeat)
              << ";"  << "maskmode:" << record.maskMode
              << ";" << "vectormask:" << record.vectorMask;
}

std::ostream &operator<<(std::ostream &os, VgatherRecord const &record)
{
    return os << record.location
              << ", dst:(" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "dstBlockSize:" << record.dstBlockSize
              << ";" << "dstBlockNum:" << static_cast<uint32_t>(record.dstBlockNum)
              << ";" << "dstBlockStride:" << static_cast<uint32_t>(record.dstBlockStride)
              << ";" << "dstRepeatStride:" << static_cast<uint32_t>(record.dstRepeatStride)
              << ";" << "bits:" << static_cast<uint32_t>(record.dstDataBits)
              << ");" << "src:(" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "srcBlockSize:" << record.srcBlockSize
              << ";" << "srcBlockNum:" << static_cast<uint32_t>(record.srcBlockNum)
              << ");" << "offsetAddr:" << record.offsetAddr
              << ";" << "repeat:" << static_cast<uint32_t>(record.dstRepeat)
              << ";"  << "maskmode:" << record.maskMode
              << ";" << "vectormask:" << record.vectorMask;
}

std::ostream &operator<<(std::ostream &os, ElementRecord const &record)
{
    return os << record.location
              << ", " << "addr:0x" << std::hex << record.addr << std::dec
              << ";" << "blockSize:" << record.blockSize
              << ";" << "blockNum:" << record.blockNum
              << ";" << "alignSize:" << record.alignSize
              << ";" << "dataBits:" << static_cast<uint32_t>(record.dataBits)
              << ";" << "accessType:" << record.accessType;
}

std::ostream &operator<<(std::ostream &os, BinaryOpRecord const &record)
{
    return os << record.location
              << ", dst:(" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "blockstride:" << record.dstBlockStride
              << ";" << "rptstride:" << record.dstRepeatStride
              << ";" << "blocksize:" << record.dstBlockSize
              << ";" << "blocknum:" << static_cast<uint32_t>(record.dstBlockNum)
              << ";" << "bits:" << static_cast<uint32_t>(record.dstDataBits)
              << ");" << "src0:(" << "addr:0x" << std::hex << record.src0 << std::dec
              << ";" << "blockstride:" << record.src0BlockStride
              << ";" << "rptstride:" << record.src0RepeatStride
              << ";" << "blocksize:" << record.src0BlockSize
              << ";" << "blocknum:" << static_cast<uint32_t>(record.src0BlockNum)
              << ";" << "bits:" << static_cast<uint32_t>(record.src0DataBits)
              << ");" << "src1:(" << "addr:0x" << std::hex << record.src1 << std::dec
              << ";" << "blockstride:" << record.src1BlockStride
              << ";" << "rptstride:" << record.src1RepeatStride
              << ";" << "blocksize:" << record.src1BlockSize
              << ";" << "blocknum:" << static_cast<uint32_t>(record.src1BlockNum)
              << ";" << "bits:" << static_cast<uint32_t>(record.src1DataBits)
              << ");" << "repeat:" << static_cast<uint32_t>(record.repeat)
              << ";" << "maskmode:" << record.maskMode
              << ";" << "vectormask:" << record.vectorMask;
}

std::ostream &operator<<(std::ostream &os, MarixMulOpRecord const &record)
{
    return os << record.location
              << ", " << "m:" << record.m
              << ";" << "k:" << record.k
              << ";" << "n:" << record.n
              << ";" << "dst:(" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "blocksize:" << record.dstBlockSize
              << ";" << "blocknum:" << static_cast<uint32_t>(record.dstBlockNum)
              << ";" << "align:" << static_cast<uint32_t>(record.dstAlignSize)
              << ");" << "src0:(" << "addr:0x" << std::hex << record.src0 << std::dec
              << ";" << "repeat:" <<static_cast<uint32_t>(record.src0Repeat)
              << ";" << "blocksize:" << record.src0BlockSize
              << ";" << "blocknum:" << static_cast<uint32_t>(record.src0BlockNum)
              << ";" << "rptstride:" << record.src0RepeatStride
              << ";" << "align:" << record.src0AlignSize
              << ");" << "src1:(" << "addr:0x" << std::hex << record.src1 << std::dec
              << ";" << "blocksize:" << record.src1BlockSize
              << ";" << "blocknum:" << static_cast<uint32_t>(record.src1BlockNum)
              << ";" << "align:" << record.src1AlignSize
              << ");" << "bias:(" << "initval:" << static_cast<uint32_t>(record.cmatrixInitVal)
              << ";" << "source:" << static_cast<uint32_t>(record.cmatrixSource)
              << ";" << "uintflag:" << record.enUnitFlag << ")";
}

std::ostream &operator<<(std::ostream &os, VecRegPropCoordOpRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "addr:0x" << std::hex << record.dst << std::dec
              << ");" << "src:(" << "addr:0x" << std::hex << record.src << std::dec
              << ");" << "datatype:" << record.dataType
              << ";" << "repeat:" << static_cast<uint32_t>(record.repeat)
              << ";" << "regionrange:" << static_cast<uint32_t>(record.regionRange);
}

std::ostream &operator<<(std::ostream &os, ReduceOpRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "rptsride:" << record.dstRepeatStride
              << ";" << "blocksize:" <<  static_cast<uint32_t>(record.dstBlockSize)
              << ";" << "blocknum:" << static_cast<uint32_t>(record.dstBlockNum)
              << ";" << "align:" <<  record.dstAlignSize
              << ";" << "rptlength:" << record.dstRepeatLength
              << ";" << "bits:" << static_cast<uint32_t>(record.dstDataBits)
              << ";" << "bitsfactor:" << static_cast<uint32_t>(record.dstDataBitsFactor)
              << ");" << "src:(" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "blockstride:" << record.srcBlockStride
              << ";" << "rptstride:" << record.srcRepeatStride
              << ";" << "blocksize:" << static_cast<uint32_t>(record.srcBlockSize)
              << ";" << "blocknum:" << static_cast<uint32_t>(record.srcBlockNum)
              << ";" << "bits:" << static_cast<uint32_t>(record.srcDataBits)
              << ");" << "repeat:" << static_cast<uint32_t>(record.repeat)
              << ";" << "maskmode:" << record.maskMode
              << ";" << "vectormask:" << record.vectorMask;
}

std::ostream &operator<<(std::ostream &os, ReduceV2Record const &record)
{
    return os << record.location
              << ", " << "dst:(" << "addr:0x" << std::hex << record.dst << std::dec
              << ");" << "src0:(" << "addr:0x" << std::hex << record.src0 << std::dec
              << ";" << "blockstride:" << static_cast<uint32_t>(record.src0BlockStride)
              << ";" << "rptstride:" << static_cast<uint32_t>(record.src0RepeatStride)
              << ");" << "src1:(" << "addr:0x" << std::hex << record.src1 << std::dec
              << ";" << "rptstride:" << static_cast<uint32_t>(record.src1RepeatStride)
              << ");" << "repeat:" << static_cast<uint32_t>(record.repeat)
              << ";" << "databytes:" << static_cast<uint32_t>(record.dataBytes)
              << ";" << "pattern:" << static_cast<uint32_t>(record.patternMode)
              << ";" << "cmpmask:" << record.compareMask
              << ";" << "maskmode:" << record.maskMode
              << ";" << "vectormask:" << record.vectorMask;
}

std::ostream &operator<<(std::ostream &os, SyncRecord const &record)
{
    return os << record.location
              << ", " << "dst:" << record.dst
              << ";" << "src:" << record.src
              << ";" << "eventid:" << record.eventID;
}

std::ostream &operator<<(std::ostream &os, HardSyncRecord const &record)
{
    return os << record.location
              << ", " << "dst:" << record.dst
              << ";" << "src:" << record.src
              << ";" << "eventid:" << record.eventID
              << ";" << "memory:" << record.memory
              << ";" << "v:" << static_cast<uint32_t>(record.v);
}

std::ostream &operator<<(std::ostream &os, PipeBarrierRecord const &record)
{
    return os << record.location
              << ", " << "pipe:" << record.pipe;
}

std::ostream &operator<<(std::ostream &os, FftsSyncRecord const &record)
{
    return os << record.location
              << ", " << "dst:" << record.dst
              << ";" << "mode:" << static_cast<uint16_t>(record.mode)
              << ";" << "flagid:" << static_cast<uint16_t>(record.flagID);
}

std::ostream &operator<<(std::ostream &os, WaitFlagDevRecord const &record)
{
    return os << record.location
              << ", flagid:" << static_cast<uint16_t>(record.flagID);
}

std::ostream &operator<<(std::ostream &os, WaitFlagDevPipeRecord const &record)
{
    return os << record.location
              << ", PipeType:" << record.pipe
              << ", flagid:" << static_cast<uint16_t>(record.flagID);
}

std::ostream &operator<<(std::ostream &os, IntraBlockSyncRecord const &record)
{
    return os << record.location
              << ", " << "PipeType:" << record.pipe
              << ", " << "syncid:" << record.syncID;
}

std::ostream& operator<<(std::ostream& os, BufRecord const& record)
{
    return os << record.location
              << ", " << "pipe:" << record.pipe
              << ";" << "bufId:" << record.bufId
              << ";" << "mode:" << static_cast<uint16_t>(record.mode);
}

std::ostream &operator<<(std::ostream &os, AddrCalMode addrCalMode)
{
    static const std::map<AddrCalMode, std::string> ADDR_CAL_MODE_MAP = {
        {AddrCalMode::INC, "INC"},
        {AddrCalMode::DEC,  "DEC"},
    };

    return FormatEnum(os, ADDR_CAL_MODE_MAP, addrCalMode, "AddrCalMode");
}

std::ostream &operator<<(std::ostream &os, MemInfoSrc memInfoSrc)
{
    static const std::map<MemInfoSrc, std::string> MEM_INFO_SRC_MAP = {
        {MemInfoSrc::BYPASS,      "BYPASS"},
        {MemInfoSrc::HAL,         "HAL"},
        {MemInfoSrc::RT,          "RT"},
        {MemInfoSrc::ACL,         "ACL"},
        {MemInfoSrc::EXTRA,       "EXTRA"},
        {MemInfoSrc::MANUAL,      "MANUAL"},
        {MemInfoSrc::MSTX_HEAP,   "MSTX_HEAP"},
        {MemInfoSrc::MSTX_REGION, "MSTX_REGION"},
    };

    return FormatEnum(os, MEM_INFO_SRC_MAP, memInfoSrc, "MemInfoSrc");
}

std::ostream &operator<<(std::ostream &os, MemInfoDesc memInfoDesc)
{
    static const std::map<MemInfoDesc, std::string> MEM_INFO_DEC_MAP = {
        {MemInfoDesc::DEFAULT,                 "default"},
        {MemInfoDesc::INPUT,                   "input_"},
        {MemInfoDesc::TILING,                  "tiling"},
        {MemInfoDesc::DOUBLE_PTR,              "double_ptr_"},
        {MemInfoDesc::HCCL_MC2_CONTEXT,        "hccl_mc2_context"},
        {MemInfoDesc::SECTION,                 "section"},
        {MemInfoDesc::IPC_MEMORY,              "ipc_memory"},
        {MemInfoDesc::OVERFLOW_ADDR,           "overflow_addr"},
        {MemInfoDesc::PARA_BASE,               "para_base"},
    };

    return FormatEnum(os, MEM_INFO_DEC_MAP, memInfoDesc, "MemInfoDesc");
}

std::ostream &operator<<(std::ostream &os, Load2DRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ");" << "baseidx:" << record.baseIdx
              << ";" << "repeat:" << static_cast<uint32_t>(record.repeat)
              << ";" << "blocksize:" << record.blockSize
              << ";" << "addrcalmode:" << record.addrCalMode;
}

std::ostream &operator<<(std::ostream &os, LoadL12DRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ");" << "src:(" << "type:" << MemType::L1
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ");" << "mstartposition:" << record.mStartPosition
              << ";" << "kstartposition:" << record.kStartPosition
              << ";" << "mstep:" << record.mStep
              << ";" << "kstep:" << record.kStep;
}

std::ostream &operator<<(std::ostream &os, LoadL1Mx2DRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ");" << "src:(" << "type:" << MemType::L1
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ");" << "xstartposition:" << record.xStartPosition
              << ";" << "ystartposition:" << record.yStartPosition
              << ";" << "xstep:" << record.xStep
              << ";" << "ystep:" << record.yStep;
}

std::ostream &operator<<(std::ostream &os, LoadL12DTransposeRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << MemType::L0B
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ";" << "fracstride:" << record.dstFracStride
              << ");" << "src:(" << "type:" << MemType::L1
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ";" << "fracstride:" << record.srcFracStride
              << ");" << "repeat:" << record.repeat
              << ";" << "detaileddatatype:" << record.detailedDataType;
}

std::ostream &operator<<(std::ostream &os, Load2DSparseRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ");" << "src:(type:" << record.srcMemType
              << ");" << "src0:(addr:0x" << std::hex << record.src0 << std::dec
              << ");" << "src1:(addr:0x" << std::hex << record.src1 << std::dec
              << ");" << "repeat:" << static_cast<uint32_t>(record.repeat)
              << ");" << "startid:" << record.startId;
}

std::ostream &operator<<(std::ostream &os, Load2DTransposeRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ";" << "fracstride:" << record.dstFracStride
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ");" << "indexid:" << record.indexId
              << ";" << "repeat:" << static_cast<uint32_t>(record.repeat)
              << ";" << "addrmode:" << record.addrMode
              << ";" << "datatype:" << record.dataType;
}

std::ostream &operator<<(std::ostream &os, DecompressHeaderRecord const &record)
{
    return os << record.location
              << ", " << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ");" << "nblock:" << record.nBlock;
}

std::ostream &operator<<(std::ostream &os, BroadcastRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << record.dstMemType
              << ";" << "datatype:" << record.dstDataType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "gap:" << static_cast<uint32_t>(record.dstGap)
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "datatype:" << record.srcDataType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "gap:" << static_cast<uint32_t>(record.srcGap)
              << ");" << "nBurst:" << static_cast<uint32_t>(record.nBurst)
              << ";" << "lenBurst:" << static_cast<uint32_t>(record.lenBurst)
              << ";" << "enableRepeat:" << record.enableRepeat;
}

std::ostream &operator<<(std::ostream &os, DcPreloadRecord const &record)
{
    return os << record.location
              << ", " << "addr:0x" << std::hex << record.addr << std::dec
              << ";" << "offset:" << record.offset;
}

std::ostream &operator<<(std::ostream &os, ScatterVnchwconvRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "va0:" << record.dst0 << ";" << "va1:" << record.dst1
              << ";" << "stride:" << record.dstStride
              << ";" << "highHalf:" << record.dstHighHalf
              << ");" << "src:(" << "va0:" << record.src0 << ";" << "va1:" << record.src1
              << ";" << "stride:" << record.srcStride
              << ";" << "highHalf:" << record.srcHighHalf
              << ");" << "repeat:" << static_cast<uint32_t>(record.repeat)
              << ";" << "datatype:" << record.dataType;
}

std::ostream &operator<<(std::ostream &os, LoadB2Record const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ");" << "repeat:" << static_cast<uint32_t>(record.repeat)
              << ";" << "datatype:" << record.dataType;
}

std::ostream &operator<<(std::ostream &os, LoadAWinogradRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "startpointk:" << record.dstStartPointK
              << ";" << "startpointmg:" << record.dstStartPointM
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ");" << "fmsize:(" << "w:" << record.fmSizeW
              << ";" << "h:" << record.fmSizeH
              << ";" << "ch:" << record.fmSizeCh
              << ");" << "extstep:(" << "k:" << record.extStepK
              << ";" << "m:" << record.extStepM
              << ");" << "datatype:" << record.dataType
              << ";" << "innerdstgap:" << static_cast<uint32_t>(record.innerDstGap);
}

std::ostream &operator<<(std::ostream &os, LoadBWinogradRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "rptstride:" << record.dstRptStride
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "rptstride:" << record.srcRptStride
              << ");" << "repeat:" << static_cast<uint32_t>(record.repeat)
              << ";" << "innerdststride:" << static_cast<uint32_t>(record.innerDstStride);
}

std::ostream &operator<<(std::ostream &os, Load3DRecord const &record)
{
    return os << record.location
               << ", " << "dst:(" << "type:" << record.dstMemType
               << ";" << "addr:0x" << std::hex << record.dst << std::dec
               << ");" << "src:(" << "type:" << record.srcMemType
               << ";" << "addr:0x" << std::hex << record.src << std::dec
               << ");" << "filter:(" << "w:" << record.filterW
               << ";" << "wstride:" << record.filterWStride
               << ";" << "wdilation:" << static_cast<uint32_t>(record.filterWDilation)
               << ";" << "h:" << record.filterH
               << ";" << "hstride:" << record.filterHStride
               << ";" << "hdilation:" << static_cast<uint32_t>(record.filterHDilation)
               << ");" << "fmap:(" << "w:" << record.fMapW
               << ";" << "h:" << record.fMapH
               << ";" << "c:" << record.fMapC
               << ";" << "t:" << static_cast<uint32_t>(record.fMapTopPad)
               << ";" << "b:" << static_cast<uint32_t>(record.fMapBottomPad)
               << ";" << "l:" << static_cast<uint32_t>(record.fMapLeftPad)
               << ";" << "r:" << static_cast<uint32_t>(record.fMapRightPad)
               << ");" << "matrix:(" << "mode:" << static_cast<uint32_t>(record.matrixMode)
               << ";" << "rptstride:" <<  record.matrixRptStride
               << ";" << "kpos:" << record.matrixKPos
               << ";" << "mpos:" << record.matrixMPos
               << ";" << "kstep:" << record.matrixKStep
               << ";" << "mstep:" << record.matrixMStep
               << ";" << "rpttimes:" << record.matrixRptTimes
               << ";" << "rptmode:" << static_cast<uint32_t>(record.matrixRptMode)
               << ");" << "datatype:" << record.dataType;
}

std::ostream &operator<<(std::ostream &os, Load3DV2Record const &record)
{
    return os << record.location
               << ", " << "dst:(" << "type:" << record.dstMemType
               << ";" << "addr:0x" << std::hex << record.dst << std::dec
               << ");" << "src:(" << "type:" << record.srcMemType
               << ";" << "addr:0x" << std::hex << record.src << std::dec
               << ");" << "filter:(" << "w:" << record.filterW
               << ";" << "wstride:" << record.filterWStride
               << ";" << "wdilation:" << static_cast<uint32_t>(record.filterWDilation)
               << ";" << "h:" << record.filterH
               << ";" << "hstride:" << record.filterHStride
               << ";" << "hdilation:" << static_cast<uint32_t>(record.filterHDilation)
               << ");" << "fmap:(" << "w:" << record.fMapW
               << ";" << "h:" << record.fMapH
               << ";" << "c:" << record.fMapC
               << ";" << "t:" << static_cast<uint32_t>(record.fMapTopPad)
               << ";" << "b:" << static_cast<uint32_t>(record.fMapBottomPad)
               << ";" << "l:" << static_cast<uint32_t>(record.fMapLeftPad)
               << ";" << "r:" << static_cast<uint32_t>(record.fMapRightPad)
               << ");" << "matrix:(" << "mode:" << static_cast<uint32_t>(record.matrixMode)
               << ";" << "rptstride:" <<  record.matrixRptStride
               << ";" << "kpos:" << record.matrixKPos
               << ";" << "mpos:" << record.matrixMPos
               << ";" << "kstep:" << record.matrixKStep
               << ";" << "mstep:" << record.matrixMStep
               << ";" << "rpttimes:" << record.matrixRptTimes
               << ";" << "rptmode:" << static_cast<uint32_t>(record.matrixRptMode)
               << ");" << "datatype:" << record.dataType;
}

std::ostream &operator<<(std::ostream &os, AtomicModeRecord const &record)
{
    return os << record.location
              << ", " << "mode:" << record.mode;
}

std::ostream &operator<<(std::ostream &os, LoadImageRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ");" << "hor:(" << "size:" << record.horSize
              << ";" << "startp:" << record.horStartP
              << ");" << "ver:(" << "size:" << record.verSize
              << ";" << "startp:" << record.verStartP
              << ");" << "shorres:" << record.sHorRes
              << "; " << "lpadsize:" << record.lPadSize
              << "; " << "rpadsize:" << record.rPadSize
              << "; " << "toppadsize:" << static_cast<uint32_t>(record.topPadSize)
              << "; " << "botpadsize:" << static_cast<uint32_t>(record.botPadSize)
              << "; " << "datatype:" << record.dataType;
}

std::ostream &operator<<(std::ostream &os, LoadSmaskRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "addr:0x" << std::hex << record.dst << std::dec
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ");" << "smaskSize:" << static_cast<int>(record.smaskSize);
}

std::ostream &operator<<(std::ostream &os, Set2DRecord const &record)
{
    return os << record.location
              << ", " << "dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "blocknum:" << record.dstBlockNum
              << ";" << "blocksize:" << record.dstBlockSize
              << ");" << "repeat:" << record.repeat
              << ";" << "rptgap:" << record.repeatGap;
}

std::ostream& operator<<(std::ostream& os, SoftSyncRecord const& record)
{
    return os << record.location
              << ", " << "waitcoreid:" << record.waitCoreID
              << ";" << "usedcores:" << record.usedCores
              << ";" << "eventid:" << record.eventID
              << ";" << "isaivonly:" << record.isAIVOnly;
}

std::ostream &operator<<(std::ostream &os, CmpMaskRecord const &record)
{
    return os << record.location
              << ", " << "addr:0x" << std::hex << record.addr << std::dec
              << ";" << "size:" << record.size
              << ";" << "accessType:" << record.accessType;
}

std::ostream &operator<<(std::ostream &os, SimtThreadLocation const &threadLoc)
{
    return os << "threadId:(" << threadLoc.idX
              << "," << threadLoc.idY
              << "," << threadLoc.idZ << ")";
}

std::ostream &operator<<(std::ostream &os, SimtLoadStoreRecord const &record)
{
    return os << record.location << ", " << record.threadLoc
              << ";" << "space:" << record.space
              << ";" << "addr:0x" << std::hex << record.addr << std::dec
              << ";" << "size:" << record.size;
}

std::ostream& operator<<(std::ostream& os, SimtAtomMode mode)
{
    static const std::map<SimtAtomMode, std::string> SIMT_ATOM_MODE_MAP = {
        {SimtAtomMode::NONE, "NONE"},
        {SimtAtomMode::MAX,  "MAX"},
        {SimtAtomMode::MIN,  "MIN"},
        {SimtAtomMode::ADD,  "ADD"},
        {SimtAtomMode::EXCH, "EXCH"},
        {SimtAtomMode::CAS,  "CAS"},
    };
    return FormatEnum(os, SIMT_ATOM_MODE_MAP, mode, "SimtAtomMode");
}

std::ostream &operator<<(std::ostream &os, SimtAtomRecord const &record)
{
    return os << record.location << ", " << record.threadLoc
              << ";" << "space:" << record.space
              << ";" << "addr:0x" << std::hex << record.addr << std::dec
              << ";" << "size:" << record.size
              << ";" << "option:" << record.option;
}

std::ostream &operator<<(std::ostream &os, KernelErrorRecord const &errorRecord)
{
    if (errorRecord.record != nullptr) {
        if (errorRecord.recordType == RecordType::SIMT_LDG || errorRecord.recordType == RecordType::SIMT_STG
        || errorRecord.recordType == RecordType::SIMT_LDS || errorRecord.recordType == RecordType::SIMT_STS
        || errorRecord.recordType == RecordType::SIMT_LDK || errorRecord.recordType == RecordType::SIMT_STK
        || errorRecord.recordType == RecordType::SIMT_LD || errorRecord.recordType == RecordType::SIMT_ST) {
            auto record = *reinterpret_cast<const SimtLoadStoreRecord *>(errorRecord.record);
            os << record.location.blockId << ", " << "type:" << errorRecord.recordType << ", "
               << "errType:" << errorRecord.kernelErrorDesc->errorType << ", " << record.threadLoc << ";"
               << "space:" << record.space << ";" << "addr:0x" << std::hex << record.addr << std::dec << ";"
               << "size:" << record.size;
        } else if (errorRecord.recordType == RecordType::SIMT_ATOM || errorRecord.recordType == RecordType::SIMT_RED) {
            auto record = *reinterpret_cast<const SimtAtomRecord *>(errorRecord.record);
            os << record.location.blockId
              << ", " << "type:" << errorRecord.recordType
              << ", " << record.threadLoc
              << ";" << "space:" << record.space
              << ";" << "addr:0x" << std::hex << record.addr << std::dec
              << ";" << "size:" << record.size
              << ";" << "option:" << record.option;
        } else if (errorRecord.recordType == RecordType::SIMT_END) {
            auto record = *reinterpret_cast<const SimtEmptyRecord *>(errorRecord.record);
            os << record.location.blockId
               << ", " << "0x" << std::hex << record.location.pc << std::dec
               << ", " << record.threadLoc
               << ", " << "type:" << errorRecord.recordType;
        }
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, RedRecord const &record)
{
    return os << record.location
              << ";" << "space:" << AddressSpace::GM
              << ";" << "addr:0x" << std::hex << record.addr << std::dec
              << ";" << "size:" << record.size;
}

std::ostream &operator<<(std::ostream &os, DetailedDataType type)
{
    static const std::map<DetailedDataType, std::string> DATA_TYPE_STR = {
        {DetailedDataType::Default, "Default"},
        {DetailedDataType::B4, "B4"},
        {DetailedDataType::E1M2, "E1M2"},
        {DetailedDataType::E2M1, "E2M1"},
        {DetailedDataType::B8, "B8"},
        {DetailedDataType::S8, "S8"},
        {DetailedDataType::U8, "U8"},
        {DetailedDataType::E4M3, "E4M3"},
        {DetailedDataType::E5M2, "E5M2"},
        {DetailedDataType::B16, "B16"},
        {DetailedDataType::S16, "S16"},
        {DetailedDataType::U16, "U16"},
        {DetailedDataType::F16, "F16"},
        {DetailedDataType::HALF, "HALF"},
        {DetailedDataType::BF16, "BF16"},
        {DetailedDataType::B32, "B32"},
        {DetailedDataType::S32, "S32"},
        {DetailedDataType::U32, "U32"},
        {DetailedDataType::FLOAT, "FLOAT"},
        {DetailedDataType::B64, "B64"},
        {DetailedDataType::S64, "S64"},
        {DetailedDataType::U64, "U64"},
        {DetailedDataType::B128, "B128"},
        {DetailedDataType::F16X2, "F16X2"},
        {DetailedDataType::BF16X2, "BF16X2"},
        {DetailedDataType::F32, "F32"},
    };
    auto it = DATA_TYPE_STR.find(type);
    return os << (it == DATA_TYPE_STR.end() ? "Unknown" : it->second);
}

std::ostream &operator<<(std::ostream &os, MmadA5Record const &record)
{
    return os << record.location
              << ", " << "m:" << record.m
              << ";" << "k:" << record.k
              << ";" << "n:" << record.n
              << ";" << "dst:(" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "blocknum:" << static_cast<uint32_t>(record.dstBlockNum)
              << ");" << "src0:(" << "addr:0x" << std::hex << record.src0 << std::dec
              << ";" << "blocknum:" << static_cast<uint32_t>(record.src0BlockNum)
              << ";" << "align:" << record.src0AlignSize
              << ";" << "dtype:" << record.src0Dtype
              << ");" << "src1:(" << "addr:0x" << std::hex << record.src1 << std::dec
              << ";" << "blocknum:" << static_cast<uint32_t>(record.src1BlockNum)
              << ";" << "align:" << record.src1AlignSize
              << ";" << "dtype:" << record.src1Dtype
              << ");" << "bias:(" << "initval:" << static_cast<uint32_t>(record.cmatrixInitVal)
              << ";" << "source:" << static_cast<uint32_t>(record.cmatrixSource)
              << ";" << "uintflag:" << record.enUnitFlag << ")";
}

std::ostream &operator<<(std::ostream &os, Vbs32Record const &record)
{
    return os << record.location
              << ", dst:0x" << std::hex << record.dst
              << ", src0:0x" << record.src0
              << ", src1:0x" << record.src1 << std::dec
              << ", repeat:" << static_cast<uint32_t>(record.repeat)
              << ", datatype:" << record.dataType;
}

std::ostream &operator<<(std::ostream &os, Vms4v2RecordA5 const &record)
{
    os << record.location << ", dst:0x" << std::hex << record.dst;
    for (uint32_t i = 0; i < Vms4v2RecordA5::ARRAY_NUM; ++i) {
        os << ";src" << i << "(addr:0x" << std::hex << (record.src[i]) << std::dec
           << ";elementNum:" << record.elementNum[i] << ")";
    }
    return os << ";repeat:" << static_cast<uint32_t>(record.repeat)
              << ";validMask:" << static_cast<uint32_t>(record.validMask)
              << ";isAllStored:" << static_cast<uint32_t>(record.isAllStored);
}

using KernelRecordStreamFunc = std::function<void(std::ostream &, KernelRecord const &)>;
static const std::map<RecordType, KernelRecordStreamFunc> KERNEL_RECORD_FORMAT_MAP = {
    {RecordType::LOAD,          [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadStoreRecord; }},
    {RecordType::STORE,         [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadStoreRecord; }},
    {RecordType::LD,            [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadStoreRecord; }},
    {RecordType::LD_IO,         [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadStoreRecord; }},
    {RecordType::ST,            [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadStoreRecord; }},
    {RecordType::ST_IO,         [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadStoreRecord; }},
    {RecordType::STP,           [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadStoreRecord; }},
    {RecordType::STI,           [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadStoreRecord; }},
    {RecordType::STI_IO,        [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadStoreRecord; }},
    {RecordType::LDP,           [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadStoreRecord; }},
    {RecordType::ST_ATOMIC,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadStoreRecord; }},
    {RecordType::STI_ATOMIC,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadStoreRecord; }},
    {RecordType::LD_DEV,        [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadStoreRecord; }},
    {RecordType::ST_DEV,        [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadStoreRecord; }},
    {RecordType::DMA_MOV,       [](std::ostream &os, KernelRecord const &r) { os << r.payload.dmaMovRecord; }},
    {RecordType::DMA_MOV_CONV_RELU,
        [](std::ostream &os, KernelRecord const &r) { os << r.payload.dmaMovConvReluRecord; }},
    {RecordType::DMA_MOV_DEPTH_WISE,
        [](std::ostream &os, KernelRecord const &r) { os << r.payload.dmaMovConvReluRecord; }},
    {RecordType::DMA_MOV_ND2NZ, [](std::ostream &os, KernelRecord const &r) { os << r.payload.dmaMovNd2nzRecord; }},
    {RecordType::DMA_MOV_ND2NZ_D,
        [](std::ostream &os, KernelRecord const &r) { os << r.payload.dmaMovNd2nzDavRecord; }},
    {RecordType::DMA_MOV_DN2NZ_D,
        [](std::ostream &os, KernelRecord const &r) { os << r.payload.dmaMovNd2nzDavRecord; }},
    {RecordType::MOV_ALIGN,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.movAlignRecord; }},
    {RecordType::MOV_ALIGN_V2,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.movAlignRecordV2; }},
    {RecordType::ND_DMA_OUT_TO_UB, [](std::ostream &os, KernelRecord const &r) { os << r.payload.ndDMAOut2UbRecord; }},
    {RecordType::MOV_BT,        [](std::ostream &os, KernelRecord const &r) { os << r.payload.movBtRecord; }},
    {RecordType::MOV_FP,        [](std::ostream &os, KernelRecord const &r) { os << r.payload.movFpRecord; }},
    {RecordType::FIX_L0C_TO_L1, [](std::ostream &os, KernelRecord const &r) { os << r.payload.movFpRecord; }},
    {RecordType::FIX_L0C_TO_UB, [](std::ostream &os, KernelRecord const &r) { os << r.payload.movFpRecord; }},
    {RecordType::VEC_DUP,       [](std::ostream &os, KernelRecord const &r) { os << r.payload.vecDupRecord; }},
    {RecordType::LOAD_2D,       [](std::ostream &os, KernelRecord const &r) { os << r.payload.load2DRecord; }},
    {RecordType::LOAD_L1_2D,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadL12DRecord; }},
    {RecordType::LOAD_L1_MX_2D, [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadL1Mx2DRecord; }},
    {RecordType::LOAD_L1_2D_TRANSPOSE,
        [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadL12DTransposeRecord; }},
    {RecordType::LOAD_2D_SPARSE,
        [](std::ostream &os, KernelRecord const &r) { os << r.payload.load2DSparseRecord; }},
    {RecordType::LOAD_2D_TRANSPOSE,
        [](std::ostream &os, KernelRecord const &r) { os << r.payload.load2DTransposeRecord; }},
    {RecordType::DECOMPRESS_HEADER,
        [](std::ostream &os, KernelRecord const &r) { os << r.payload.decompressHeaderRecord; }},
    {RecordType::DC_PRELOAD,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.dcPreloadRecord; }},
    {RecordType::BROADCAST,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.broadcastRecord; }},
    {RecordType::SCATTERVNCHWCONV,
        [](std::ostream &os, KernelRecord const &r) { os << r.payload.scatterVnchwconvRecord; }},
    {RecordType::SCATTERVNCHWCONV_A5,
        [](std::ostream &os, KernelRecord const &r) { os << r.payload.scatterVnchwconvRecord; }},
    {RecordType::LOAD_3D,       [](std::ostream &os, KernelRecord const &r) { os << r.payload.load3DRecord; }},
    {RecordType::LOAD_3D_V2,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.load3DV2Record; }},
    {RecordType::LOAD_B2,       [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadB2Record; }},
    {RecordType::LOAD_A_WINOGRAD,
        [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadAWinogradRecord; }},
    {RecordType::LOAD_B_WINOGRAD,
        [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadBWinogradRecord; }},
    {RecordType::SET_2D,        [](std::ostream &os, KernelRecord const &r) { os << r.payload.set2DRecord; }},
    {RecordType::LOAD_IMAGE,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadImageRecord; }},
    {RecordType::LOAD_SMASK,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadSmaskRecord; }},
    {RecordType::UNARY_OP,      [](std::ostream &os, KernelRecord const &r) { os << r.payload.unaryOpRecord; }},
    {RecordType::VGATHER,       [](std::ostream &os, KernelRecord const &r) { os << r.payload.vgatherRecord; }},
    {RecordType::ELEMENT,       [](std::ostream &os, KernelRecord const &r) { os << r.payload.elementRecord; }},
    {RecordType::VCOPY_OP,      [](std::ostream &os, KernelRecord const &r) { os << r.payload.unaryOpRecord; }},
    {RecordType::VCONV_DST_S4_OP,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.unaryOpRecord; }},
    {RecordType::VCONV_SRC_S4_OP,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.unaryOpRecord; }},
    {RecordType::VREDUCEV2_BINARY,   [](std::ostream &os, KernelRecord const &r) { os << r.payload.binaryOpRecord; }},
    {RecordType::VREDUCEV2_UNARY,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.unaryOpRecord; }},
    {RecordType::VREDUCEV2,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.reduceV2Record; }},
    {RecordType::VMRGSORT4_OP_C220,  [](std::ostream &os, KernelRecord const &r) { os << r.payload.unaryOpRecord;}},
    {RecordType::VMRGSORT4_OP_M200,  [](std::ostream &os, KernelRecord const &r) { os << r.payload.unaryOpRecord;}},
    {RecordType::VMRGSORT4_OP_C310,  [](std::ostream &os, KernelRecord const &r) { os << r.payload.vms4V2RecordA5;}},
    {RecordType::VBS32_A5,      [](std::ostream &os, KernelRecord const &r) { os << r.payload.vbs32Record; }},
    {RecordType::BINARY_OP,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.binaryOpRecord; }},
    {RecordType::TERNARY_OP,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.binaryOpRecord; }},
    {RecordType::VSEL_OP,       [](std::ostream &os, KernelRecord const &r) { os << r.payload.binaryOpRecord; }},
    {RecordType::REDUCE_OP,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.reduceOpRecord; }},
    {RecordType::CMPMASK_OP,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.cmpMaskRecord; }},
    {RecordType::MATRIX_MUL_OP, [](std::ostream &os, KernelRecord const &r) { os << r.payload.matrixMulOpRecord; }},
    {RecordType::MMAD_A5,       [](std::ostream &os, KernelRecord const &r) { os << r.payload.mmadA5Record; }},
    {RecordType::VEC_REGPROPCOOR_OP,
        [](std::ostream &os, KernelRecord const &r) { os << r.payload.vecRegPropCoordOpRecord; }},
    {RecordType::SET_FLAG,      [](std::ostream &os, KernelRecord const &r) { os << r.payload.syncRecord; }},
    {RecordType::SET_FLAGI,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.syncRecord; }},
    {RecordType::WAIT_FLAG,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.syncRecord; }},
    {RecordType::WAIT_FLAGI,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.syncRecord; }},
    {RecordType::GET_BUF,      [](std::ostream &os, KernelRecord const &r) { os << r.payload.bufRecord; }},
    {RecordType::GET_BUFI,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.bufRecord; }},
    {RecordType::RLS_BUF,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.bufRecord; }},
    {RecordType::RLS_BUFI,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.bufRecord; }},
    {RecordType::GET_BUF_V,      [](std::ostream &os, KernelRecord const &r) { os << r.payload.bufRecord; }},
    {RecordType::GET_BUFI_V,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.bufRecord; }},
    {RecordType::RLS_BUF_V,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.bufRecord; }},
    {RecordType::RLS_BUFI_V,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.bufRecord; }},
    {RecordType::HSET_FLAG,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.hardSyncRecord; }},
    {RecordType::HWAIT_FLAG,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.hardSyncRecord; }},
    {RecordType::HSET_FLAGI,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.hardSyncRecord; }},
    {RecordType::HWAIT_FLAGI,   [](std::ostream &os, KernelRecord const &r) { os << r.payload.hardSyncRecord; }},
    {RecordType::PIPE_BARRIER,  [](std::ostream &os, KernelRecord const &r) { os << r.payload.pipeBarrierRecord; }},
    {RecordType::FFTS_SYNC,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.fftsSyncRecord; }},
    {RecordType::WAIT_FLAG_DEV, [](std::ostream &os, KernelRecord const &r) { os << r.payload.waitFlagDevRecord; }},
    {RecordType::WAIT_FLAG_DEV_PIPE, [](std::ostream &os, KernelRecord const &r) { os << r.payload.waitFlagDevPipeRecord; }},
    {RecordType::WAIT_FLAG_DEVI_PIPE, [](std::ostream &os, KernelRecord const &r) { os << r.payload.waitFlagDevPipeRecord; }},
    {RecordType::SET_INTRA_BLOCK, [](std::ostream &os, KernelRecord const &r) { os << r.payload.intraBlockSyncRecord; }},
    {RecordType::WAIT_INTRA_BLOCK, [](std::ostream &os, KernelRecord const &r) { os << r.payload.intraBlockSyncRecord; }},
    {RecordType::SET_INTRA_BLOCKI, [](std::ostream &os, KernelRecord const &r) { os << r.payload.intraBlockSyncRecord; }},
    {RecordType::WAIT_INTRA_BLOCKI, [](std::ostream &os, KernelRecord const &r) { os << r.payload.intraBlockSyncRecord; }},
    {RecordType::SET_FLAG_V,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.syncRecord; }},
    {RecordType::SET_FLAGI_V,   [](std::ostream &os, KernelRecord const &r) { os << r.payload.syncRecord; }},
    {RecordType::WAIT_FLAG_V,   [](std::ostream &os, KernelRecord const &r) { os << r.payload.syncRecord; }},
    {RecordType::WAIT_FLAGI_V,  [](std::ostream &os, KernelRecord const &r) { os << r.payload.syncRecord; }},
    {RecordType::FFTS_SYNC_V,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.fftsSyncRecord; }},
    {RecordType::WAIT_FLAG_DEV_PIPE_V, [](std::ostream &os, KernelRecord const &r) { os << r.payload.waitFlagDevPipeRecord; }},
    {RecordType::WAIT_FLAG_DEVI_PIPE_V, [](std::ostream &os, KernelRecord const &r) { os << r.payload.waitFlagDevPipeRecord; }},
    {RecordType::SET_INTRA_BLOCK_V, [](std::ostream &os, KernelRecord const &r) { os << r.payload.intraBlockSyncRecord; }},
    {RecordType::SET_INTRA_BLOCKI_V, [](std::ostream &os, KernelRecord const &r) { os << r.payload.intraBlockSyncRecord; }},
    {RecordType::WAIT_INTRA_BLOCK_V, [](std::ostream &os, KernelRecord const &r) { os << r.payload.intraBlockSyncRecord; }},
    {RecordType::WAIT_INTRA_BLOCKI_V, [](std::ostream &os, KernelRecord const &r) { os << r.payload.intraBlockSyncRecord; }},
    {RecordType::SET_ATOMIC,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.atomicModeRecord; }},
    {RecordType::IB_SET_STUB,   [](std::ostream& os, KernelRecord const& r) { os << r.payload.softSyncRecord; }},
    {RecordType::IB_WAIT_STUB,  [](std::ostream& os, KernelRecord const& r) { os << r.payload.softSyncRecord; }},
    {RecordType::SYNC_ALL_STUB, [](std::ostream& os, KernelRecord const& r) { os << r.payload.softSyncRecord; }},
    {RecordType::MSTX_STUB,     [](std::ostream& os, KernelRecord const& r) { os << r.payload.mstxRecord; }},
    {RecordType::SIMT_LDG,      [](std::ostream& os, KernelRecord const& r) { os << r.payload.simtLoadStoreRecord; }},
    {RecordType::SIMT_STG,      [](std::ostream& os, KernelRecord const& r) { os << r.payload.simtLoadStoreRecord; }},
    {RecordType::SIMT_LDS,      [](std::ostream& os, KernelRecord const& r) { os << r.payload.simtLoadStoreRecord; }},
    {RecordType::SIMT_STS,      [](std::ostream& os, KernelRecord const& r) { os << r.payload.simtLoadStoreRecord; }},
    {RecordType::SIMT_LDK,      [](std::ostream& os, KernelRecord const& r) { os << r.payload.simtLoadStoreRecord; }},
    {RecordType::SIMT_STK,      [](std::ostream& os, KernelRecord const& r) { os << r.payload.simtLoadStoreRecord; }},
    {RecordType::SIMT_LD,       [](std::ostream& os, KernelRecord const& r) { os << r.payload.simtLoadStoreRecord; }},
    {RecordType::SIMT_ST,       [](std::ostream& os, KernelRecord const& r) { os << r.payload.simtLoadStoreRecord; }},
    {RecordType::SIMT_ATOM,     [](std::ostream& os, KernelRecord const& r) { os << r.payload.simtAtomRecord; }},
    {RecordType::SIMT_RED,      [](std::ostream& os, KernelRecord const& r) { os << r.payload.simtAtomRecord; }},
    {RecordType::ONLINE_ERROR,  [](std::ostream &os, KernelRecord const &r) { os << r.payload.kernelErrorRecord; }},
    {RecordType::SCALAR_RED,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.redRecord; }},
    {RecordType::SCALAR_ATOM,   [](std::ostream &os, KernelRecord const &r) { os << r.payload.redRecord; }},
    {RecordType::LDVA,          [](std::ostream &os, KernelRecord const &r) { os << r.payload.loadStoreRecord; }},
    {RecordType::SET_L1_2D,     [](std::ostream &os, KernelRecord const &r) { os << r.payload.setL12DRecord; }},
    {RecordType::MOV_L1_TO_UB,  [](std::ostream &os, KernelRecord const &r) { os << r.payload.movL1UbRecord; }},
    {RecordType::MOV_UB_TO_L1,  [](std::ostream &os, KernelRecord const &r) { os << r.payload.movL1UbRecord; }},
    {RecordType::MOV_UB_TO_UB,  [](std::ostream &os, KernelRecord const &r) { os << r.payload.movL1UbRecord; }},
    {RecordType::MOV_CBUF_TO_BT,  [](std::ostream &os, KernelRecord const &r) { os << r.payload.movL1BtRecord; }},
    {RecordType::MOV_CBUF_TO_FB,  [](std::ostream &os, KernelRecord const &r) { os << r.payload.movL1FbRecord; }},
    {RecordType::DYNAMIC_OP,   [](std::ostream &os, KernelRecord const &r) { os << r.payload.dynamicRecord; }},
    {RecordType::SET_VECTOR_MASK_0, [](std::ostream &os, KernelRecord const &r) { os << r.payload.registerSetRecord; }},
    {RecordType::SET_VECTOR_MASK_1, [](std::ostream &os, KernelRecord const &r) { os << r.payload.registerSetRecord; }},
    {RecordType::SET_CTRL,        [](std::ostream &os, KernelRecord const &r) { os << r.payload.registerSetRecord; }},
    {RecordType::SET_FFTS_BASE_ADDR,    [](std::ostream &os, KernelRecord const &r) { os << r.payload.registerSetRecord; }},
    {RecordType::SET_FPC,         [](std::ostream &os, KernelRecord const &r) { os << r.payload.registerSetRecord; }},
    {RecordType::SET_QUANT_PRE,   [](std::ostream &os, KernelRecord const &r) { os << r.payload.registerSetRecord; }},
    {RecordType::SET_QUANT_POST,  [](std::ostream &os, KernelRecord const &r) { os << r.payload.registerSetRecord; }},
    {RecordType::SET_LRELU_ALPHA, [](std::ostream &os, KernelRecord const &r) { os << r.payload.registerSetRecord; }},
    {RecordType::THREAD_BLOCK_BARRIER, [](std::ostream &os, KernelRecord const &r) { os << r.payload.simtEmptyRecord; }},
    {RecordType::SIMT_START, [](std::ostream &os, KernelRecord const &r) { os << r.payload.simtEmptyRecord; }},
    {RecordType::SIMT_END, [](std::ostream &os, KernelRecord const &r) { os << r.payload.simtEmptyRecord; }},
    {RecordType::SIMT_CALL, [](std::ostream &os, KernelRecord const &r) { os << r.payload.mainScalarEmptyRecord; }}
};

std::ostream &operator<<(std::ostream &os, KernelRecord const &record)
{
    os << record.recordType << ", " << record.serialNo;

    typename decltype(KERNEL_RECORD_FORMAT_MAP)::const_iterator it = KERNEL_RECORD_FORMAT_MAP.find(record.recordType);
    if (it == KERNEL_RECORD_FORMAT_MAP.cend()) {
        return os;
    }

    os << ", " << record.blockType << "-";
    it->second(os, record);
    return os;
}

std::ostream &operator<<(std::ostream &os, MemOpRecord const &record)
{
    std::string fileName(record.fileName);
    os << "serialNo:" << record.serialNo
       << ", " << "type:" << record.type
       << ", " << "coreId:" << record.coreId
       << ", " << "srcAddr:";

    os << "0x" << std::hex << record.srcAddr;
    os <<  ", " << "dstAddr:";

    os << "0x" << std::hex << record.dstAddr << std::dec;

    if (record.infoSrc == MemInfoSrc::BYPASS || record.infoSrc == MemInfoSrc::EXTRA) {
        os << ", " << "memInfoDesc:" << record.infoDesc;
        if (record.infoDesc == MemInfoDesc::INPUT || record.infoDesc == MemInfoDesc::DOUBLE_PTR) {
            os << record.paramsNo;
        }
    }

    return os << ", " << "memInfoSrc:" << record.infoSrc
              << ", " << "srcSpace:" << record.srcSpace
              << ", " << "dstSpace:" << record.dstSpace
              << ", " << "memSize:" << record.memSize
              << ", " << "moduleId:" << record.moduleId
              << ", " << "lineNo:" << record.lineNo
              << ", " << "fileName:" << fileName;
}

std::ostream &operator<<(std::ostream &os, SanitizerRecord const &record)
{
    if (record.version == RecordVersion::MEMORY_RECORD) {
        return os << "RecordVersion:MEMORY_RECORD, " << record.payload.memoryRecord;
    } else if (record.version == RecordVersion::KERNEL_RECORD) {
        return os << record.payload.kernelRecord;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, IPCOperationType const &type)
{
    switch (type) {
        case IPCOperationType::SET_INFO:
            os << "SET_INFO";
            break;
        case IPCOperationType::DESTROY_INFO:
            os << "DESTROY_INFO";
            break;
        case IPCOperationType::MAP_INFO:
            os << "MAP_INFO";
            break;
        case IPCOperationType::UNMAP_INFO:
            os << "UNMAP_INFO";
            break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, IPCMemRecord const &record)
{
    os << "RecordVersion:IPC_RECORD, type:" << record.type << ", ";
    switch (record.type) {
        case IPCOperationType::SET_INFO:
            os << "addr:0x" << std::hex << record.setInfo.addr << ", "
               << "size:" << record.setInfo.size << ", "
               << "name:" << record.setInfo.name;
            break;
        case IPCOperationType::DESTROY_INFO:
            os << "name:" << std::hex << record.destroyInfo.name;
            break;
        case IPCOperationType::MAP_INFO:
            os << "addr:0x" << std::hex << record.mapInfo.addr << ", "
               << "name:" << record.mapInfo.name;
            break;
        case IPCOperationType::UNMAP_INFO:
            os << "addr:0x" << std::hex << record.unmapInfo.addr;
            break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, DmaMovNd2nzDavRecord const &record)
{
    return os
        << record.location
        << ", " << "dst:(" << "type:" << record.dstMemType
        << ";" << "addr:0x" << std::hex << record.dst << std::dec
        << ";" << "loop2Stride:" << record.loop2DstStride
        << ";" << "loop3Stride:" << record.loop3DstStride
        << ";" << "loop4Stride:" << record.loop4DstStride
        << ");" << "src:(" << "type:" << record.srcMemType
        << ";" << "addr:0x" << std::hex << record.src << std::dec
        << ";" << "loop1Stride:" << record.loop1SrcStride
        << ";" << "loop4Stride:" << record.loop4SrcStride
        << ");" << "dataType:" << record.dataType
        << ";" << "D:" << record.dValue
        << ";" << "N:" << record.nValue
        << ";" << "ndNum:" << record.ndNum
        << ";" << "smallC0:" << static_cast<uint32_t>(record.smallC0);
}

std::ostream &operator<<(std::ostream &os, SetL12DRecord const &record)
{
    return os << record.location
              << ", " << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "blocknum:" << record.dstBlockNum
              << ";" << "repeat:" << record.repeat
              << ";" << "repeatgap:" << record.repeatGap;
}

std::ostream &operator<<(std::ostream &os, MovL1UBRecord const &record)
{
    return os << record.location
              << ", dst:(" << "type:" << record.dstMemType
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "gap:" << record.dstGap
              << ");" << "src:(" << "type:" << record.srcMemType
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "gap:" << record.srcGap
              << ");" << "nburst:" << record.nBurst
              << ";" << "lenburst:" << record.lenBurst;
}

std::ostream &operator<<(std::ostream &os, MovL1BtRecord const &record)
{
    return os << record.location
              << ", dst:(" << "type:" << MemType::BT
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "gap:" << record.dstGap
              << ");" << "src:(" << "type:" << MemType::L1
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "gap:" << record.srcGap
              << ");" << "nburst:" << record.nBurst
              << ";" << "lenburst:" << record.lenBurst
              << ";" << "dataType:" << record.dataType
              << ";" << "cvtenable:" << static_cast<uint32_t>(record.cvtEnable);
}

std::ostream &operator<<(std::ostream &os, MovL1FbRecord const &record)
{
    return os << record.location
              << ", dst:(" << "type:" << MemType::FB
              << ";" << "addr:0x" << std::hex << record.dst << std::dec
              << ";" << "stride:" << record.dstStride
              << ");" << "src:(" << "type:" << MemType::L1
              << ";" << "addr:0x" << std::hex << record.src << std::dec
              << ";" << "stride:" << record.srcStride
              << ");" << "nburst:" << record.nBurst
              << ";" << "lenburst:" << record.lenBurst
              << ";" << "dstmemblock:" << static_cast<uint32_t>(record.dstMemBlock);
}

std::ostream &operator<<(std::ostream &os, DynamicRecord const &dynamicRecord)
{
    if (dynamicRecord.buffer != nullptr && dynamicRecord.count > 0) {
        if (dynamicRecord.dynamicType == RecordType::SIMT_ENTRY) {
            const auto &shRecord = reinterpret_cast<const ShadowMemoryRecord *>(dynamicRecord.buffer);
            return os << shRecord->location << ", type:" << dynamicRecord.dynamicType << ";"
                << "space:" << shRecord->space << ";"
                << "count:" << dynamicRecord.count;
        }
    }
    return os << ", type:" << dynamicRecord.dynamicType << ";count:" << dynamicRecord.count;
}

std::ostream &operator<<(std::ostream &os, RegisterSetRecord const &record)
{
    return os << record.location << ", "
              << "regValType:" << static_cast<uint32_t>(record.regPayLoad.regValType) << ", "
              << "regVal:" << record.regPayLoad.regVal;
}

std::ostream &operator<<(std::ostream &os, SimtEmptyRecord const &record)
{
    return os << record.location << ", " << record.threadLoc;
}

std::ostream &operator<<(std::ostream &os, MainScalarEmptyRecord const &record)
{
    return os << record.location;
}

}  // namespace Sanitizer
