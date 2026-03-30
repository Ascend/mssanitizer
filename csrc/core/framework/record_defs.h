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

#ifndef CORE_FRAMEWORK_RECORD_DEFS_H
#define CORE_FRAMEWORK_RECORD_DEFS_H

#include "arch_def.h"
#include "utility/struct.h"
#include "sanitizer_report.h"

/*************************************************************************************
 * This header file is included by both c/c++ and cce kernel. Never include standard *
 * libraries here.                                                                   *
 *************************************************************************************/

/* 此头文件包含 kernel 侧向 GM 上记录的信息以及 HD 通信相关的枚举类型和结构体定义
 */

#if !(defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1)
#include <cstdint>
#include <cstddef>
#endif

namespace Sanitizer {

constexpr uint16_t MAX_RECORD_BUF_SIZE_EACH_BLOCK = 8 * 1024;    // 单核内存最大8G，单位为M
constexpr uint16_t MAX_RECORD_BUF_SIZE_ALL_BLOCK = 24 * 1024;    // 多核内存最大24G，单位为M
constexpr uint64_t MB_TO_BYTES = 1024 * 1024UL;                  // MB转换到字节换算单位
constexpr uint64_t MAX_BLOCKDIM_NUMS = 100;
constexpr uint16_t MATRIX_FRACTAL_SIZE = 512;
constexpr uint16_t MATRIX_FRACTAL_ROW_SIZE = 32;
// mix算子运行时，blockDim = 1对应1个cube + 2个vec
/// 因某些硬件的特殊架构，一个 AICore 中包含若干个 AIVEC 核和 AICUBE 核（c220架构 为 1 个 AICUBE 核和 2 个 AIVEC 核）
/// 当算子以 MIX 模式运行时，blockDim 代表参与运算的 AICore 逻辑核数，因此实际参与运算的 subBlockDim 分别为
/// blockDim * C220_VEC_SUB_BLOCKDIM 和 blockDim * C220_CUBE_SUB_BLOCKDIM。
// 最终申请的GM内存为：cache_size * blockDim * (C220_VEC_SUB_BLOCKDIM + C220_CUBE_SUB_BLOCKDIM)
// 排布顺序为：vec/vec/cube/vec/vec/cube ......
constexpr uint8_t C220_VEC_SUB_BLOCKDIM = 2;
constexpr uint8_t C220_CUBE_SUB_BLOCKDIM = 1;
constexpr uint8_t C220_MIX_SUB_BLOCKDIM = C220_VEC_SUB_BLOCKDIM + C220_CUBE_SUB_BLOCKDIM;
// 单核检测时不检测的核需要分配的内存大小，单位为M
constexpr uint16_t SINGLE_CHECK_OTHER_BLOCK_CACHE_SIZE = 1;
/// c220芯片架构，
/// A2芯片： vec核对应的物理核编号范围：[25, 74]，cube核编号范围：[0, 24]
/// A3芯片： 偶数卡：和A2芯片一致;奇数卡: vec核对应的物理核编号范围:[32793,32842],cube核编号范围:[32768,32792]
constexpr int64_t C220_A2_A3_MAXCORE_NUM = 75;
constexpr int64_t C220_A2_OR_A3_EVEN_DEVICE_VEC_PHYS_CORE_START_IDS = 25;
constexpr int64_t C220_A2_OR_A3_EVEN_DEVICE_VEC_PHYS_CORE_END_IDS = 74;
constexpr int64_t C220_A3_ODD_DEVICE_VEC_PHYS_CORE_START_IDS = 32793;
constexpr int64_t C220_A3_ODD_DEVICE_VEC_PHYS_CORE_END_IDS = 32842;
constexpr int64_t C220_A3_ODD_DEVICE_VEC_CUBE_CORE_START_IDS = 32768;

/// c310架构A5芯片：vec核对应的物理核编号范围:[18, 51]和>= 72
constexpr int64_t C310_A5_DEVICE_VEC_PHYS_SMALL_BOUND_CORE_START_IDS = 18;
constexpr int64_t C310_A5_DEVICE_VEC_PHYS_SMALL_BOUND_CORE_END_IDS = 51;
constexpr int64_t C310_A5_DEVICE_VEC_PHYS_GREAT_BOUND_CORE_START_IDS = 72;

// GM地址写入时，优先写入RecordType，再写入Record；RecordType为32bit目的是为了后续写入GM时的地址对齐
// 这里将不同种类的记录分成若干区间，之后新增记录时，只从每类的最后添加，这样可以保证枚举的底层值
// 不变，从而避免device/host/工具侧版本不同造成的解析错误

/// 头部安全校检值，对应二进制：0b101101001011010010110100101101001011010010110100101101001011010
/// 尽量保证高比特位包含足够多的1，减小碰撞机率
constexpr uint64_t RECORD_HEAD_SECURITY_VALUE = 0x5a5a5a5a5a5a5a5a;
const uint64_t VEC_MASK_MAX_VALUE = static_cast<uint64_t>(-1);

/// 算子 kernel name 的最大长度
constexpr uint16_t KERNEL_NAME_MAX = 2048;

constexpr uint16_t DUMP_PATH_MAX = 256;

constexpr int16_t CHECK_ALL_BLOCK = -1;

constexpr uint32_t MOV_LOCAL_BLOCK_SIZE = 32UL;

// 用作在block record缓存区域末尾留出余量，避免后一个block修改blockHead时篡改前一个cache的末尾数据
constexpr uint64_t CACHE_LINE_SIZE = 64ULL;

// simt最大线程数
constexpr uint16_t SIMT_THREAD_MAX_SIZE = 2048;

// simt记录占总cache-size比例
constexpr float SIMT_CACHE_SIZE_RATIO  = 0.1;

// shadow memory占总cache-size比例
constexpr float SHADOW_MEM_CACHE_SIZE_RATIO = 0.5;

// shadow memory能正常运行所需GM的最小size,12MB
constexpr uint64_t SHADOW_MEM_MIN_BYTE_SIZE = 12 * 1024 * 1024;

// 非法的地址信息
constexpr uint64_t ILLEGAL_ADDR = 0xFFFFFFFFFFFFFFFFULL;

namespace OnlineShadowMemory {
// gm建模地址范围0 ~ 0xFFFF FFFF FFFF (48 bits)
constexpr uint64_t ONLINE_GLOBAL_MEM_MASK = 0xFFFFFFFFFFFFULL;
// 片上内存建模地址范围0 ~ 0xF FFFF FFFF (32 bits)
constexpr uint64_t ONLINE_LOCAL_MEM_MASK = 0xFFFFFFFFULL;
// 用于标记GM上定义的数据来源于host
constexpr uint64_t ONLINE_ONE_SM_STAND_FOR_BYTE = 0xFFFFULL + 1; // 64KB

enum MemoryByteStatus : uint8_t {
    DEFAULT = 0,
    READ,
    GLOBAL_READ,
    WRITE,
    RACE,
};

enum class OnlineMemoryType : uint8_t {
    GM = 0,
    UB,
};

/*
协议设计如下：
    [63:32]: pc
    [31:31]: sync threads state
    [30:30]: memoryType 当前内存表示ub还是gm，默认为gm
    [11:14]: memory status表示当前内存上的状态，分为DEFAULT/READ/GLOBAL_READ/WRITE/RACE ...
    [10:0]: threadId
*/
constexpr uint64_t PC_START_BIT = 32;
constexpr uint64_t PC_MASK = 0xFFFFU;
constexpr uint64_t SYNC_STATE_START_BIT = 31;
constexpr uint64_t SYNC_STATE_MASK = 0x1U;
constexpr uint64_t MEMORY_TYPE_START_BIT = 30;
constexpr uint64_t MEMORY_TYPE_MASK = 0x1U;
constexpr uint64_t MEMORY_STATUS_START_BIT = 11;
constexpr uint64_t MEMORY_STATUS_MASK = 0xFU;
constexpr uint64_t THREAD_ID_MASK = 0x7FFU;
} // namespace OnlineShadowMemory

// 在线shadow memory的单字节状态
enum class OnlineSmAddrStatus : uint64_t {
    LOCKED_BY_OTHER_THREADS = 1U,
    UNALLOCATABLE = UINT64_MAX, // 内存不足，无法再分配
};

enum class RecordType : uint32_t {
    /// load_store
    LOAD = 0,
    STORE,
    LD,
    LD_IO,
    ST,
    ST_IO,
    STP,
    STI,
    STI_IO,
    LDP,
    ST_ATOMIC,
    STI_ATOMIC,
    ST_DEV,
    LD_DEV,

     /// data_move
    DMA_MOV = 100,
    MOV_ALIGN,
    MOV_ALIGN_V2,
    VEC_DUP,
    LOAD_2D,
    LOAD_L1_2D,
    LOAD_L1_MX_2D,
    LOAD_L1_2D_TRANSPOSE,
    LOAD_B2,
    LOAD_A_WINOGRAD,
    LOAD_B_WINOGRAD = 110,
    LOAD_3D,
    SET_2D,
    SET_L1_2D,
    DMA_MOV_ND2NZ,
    MOV_BT,
    LOAD_IMAGE,
    MOV_FP,
    LOAD_2D_SPARSE,
    LOAD_2D_TRANSPOSE,
    DECOMPRESS_HEADER = 120,
    DMA_MOV_CONV_RELU,
    LOAD_SMASK,
    BROADCAST,
    DMA_MOV_DEPTH_WISE,
    ND_DMA_OUT_TO_UB,
    DC_PRELOAD,
    DMA_MOV_ND2NZ_D,   // fix name conflict for dav_c310
    DMA_MOV_DN2NZ_D,
    SCALAR_RED,
    SCALAR_ATOM = 130,
    LDVA,
    MOV_L1_TO_UB,
    MOV_UB_TO_L1,
    MOV_UB_TO_UB,
    MOV_CBUF_TO_BT,
    MOV_CBUF_TO_FB,
    VMRGSORT4_OP_C220,
    VMRGSORT4_OP_M200,
    VMRGSORT4_OP_C310,
    FIX_L0C_TO_L1 = 140,
    FIX_L0C_TO_UB,
    LOAD_3D_V2,

    // SIMD范围内的指令适用于310p的"X address mode"数据类型对齐规则
    SIMD_START = 9999,
    /// calc
    UNARY_OP = 10000,
    VGATHER,
    ELEMENT,
    BINARY_OP,
    TERNARY_OP,
    REDUCE_OP,
    MATRIX_MUL_OP,
    MMAD_A5,
    VEC_REGPROPCOOR_OP,
    CMPMASK_OP,
    VCOPY_OP = 10010,
    VREDUCEV2_UNARY,
    VREDUCEV2_BINARY,
    VREDUCEV2,
    VCONV_DST_S4_OP,
    VCONV_SRC_S4_OP,
    VSEL_OP,

    SIMD_END = 19999,

    /// sync
    SET_FLAG = 20000,
    WAIT_FLAG,
    HSET_FLAG,
    HWAIT_FLAG,
    PIPE_BARRIER,
    FFTS_SYNC,
    WAIT_FLAG_DEV,
    SET_ATOMIC,
    IB_SET_STUB,
    IB_WAIT_STUB,
    SYNC_ALL_STUB = 20010,
    WAIT_FLAG_DEV_PIPE,
    WAIT_FLAG_DEVI_PIPE,
    SET_INTRA_BLOCK,
    WAIT_INTRA_BLOCK,
    SET_INTRA_BLOCKI,
    WAIT_INTRA_BLOCKI,
    MSTX_STUB,
    SCATTERVNCHWCONV,
    SCATTERVNCHWCONV_A5,
    VBS32_A5,
    GET_BUF,
    GET_BUFI,
    RLS_BUF,
    RLS_BUFI,
    SET_FLAGI,
    WAIT_FLAGI,
    HSET_FLAGI,
    HWAIT_FLAGI,
    SET_FLAG_V,
    SET_FLAGI_V,
    WAIT_FLAG_V,
    WAIT_FLAGI_V,
    FFTS_SYNC_V,
    WAIT_FLAG_DEV_PIPE_V,
    WAIT_FLAG_DEVI_PIPE_V,
    SET_INTRA_BLOCK_V,
    SET_INTRA_BLOCKI_V,
    WAIT_INTRA_BLOCK_V,
    WAIT_INTRA_BLOCKI_V,
    GET_BUF_V,
    GET_BUFI_V,
    RLS_BUF_V,
    RLS_BUFI_V,
    SIMT_START = 39999,
    SIMT_LDG,
    SIMT_STG,
    SIMT_LDS,
    SIMT_STS,
    SIMT_LDK,
    SIMT_STK,
    SIMT_LD,
    SIMT_ST,
    SIMT_ATOM,
    SIMT_RED,
    SHADOW_MEMORY = 40010,
    THREAD_BLOCK_BARRIER,
    SIMT_END = 50000,

   /// online检测对应的错误类型
    ONLINE_ERROR = 60000,

    // 寄存器
    REGISTER_START = 70000,
    SET_VECTOR_MASK_0,
    SET_VECTOR_MASK_1,
    SET_CTRL,
    SET_FFTS_BASE_ADDR,
    SET_FPC,
    SET_QUANT_PRE,
    SET_QUANT_POST,
    SET_LRELU_ALPHA,

    /// BLOCK_FINISH 类型是虚拟出的记录类型，表明单个逻辑核的记录类型已经上报完毕，
    /// 用来通知检测算法重置片上内存状态
    BLOCK_FINISH = 100000,
    /// FINISH 类型是虚拟出的记录类型，Device 侧并不会写入这种记录类型，而是在 Host 侧当所有记录
    /// 上报结束后手动上报一次 FINISH，通知检测算法记录已全部上报完成
    FINISH,
};

enum class DataType : uint8_t {
    DATA_B4 = 0,
    DATA_B8,
    DATA_B16,
    DATA_B32,
};

// 由于编译器只支持一个桩函数绑定一个api_id，而检测对于bit位数相同的数据处理逻辑常常一致
// 为了避免编译器优化函数导致动态插桩失败的问题，增加一个表示细节数据类型的占位枚举
// 目前仅用于防止编译器优化函数
enum class DetailedDataType : uint8_t {
    Default = 0,
    B4,
    E1M2,
    E2M1,
    B8,
    S8,
    U8,
    E4M3,
    E5M2,
    B16,
    S16,
    U16,
    F16,
    HALF,
    BF16,
    B32,
    S32,
    U32,
    FLOAT,
    B64,
    S64,
    U64,
    B128,
    F16X2,
    BF16X2,
    F32,
};

/// 此枚举定义与编译器内置类型 pad_t 保持一致
enum class PadMode : uint8_t {
    PAD_NONE = 0, // 不启用 Padding 特性
    PAD_MODE1,    // 每 1B 填充 31 个 PadValue，PadValue 大小为 1B，1Block 的源数据会被扩充到 32Block
    PAD_MODE2,    // 每 2B 填充 15 个 PadValue，PadValue 大小为 2B，1Block 的源数据会被扩充到 16Block
    PAD_MODE3,    // 每 4B 填充 14 个 PadValue，PadValue 大小为 2B，1Block 的源数据会被扩充到 8Block
    PAD_MODE4,    // 每 8B 填充 12 个 PadValue，PadValue 大小为 2B，1Block 的源数据会被扩充到 4Block
    PAD_MODE5,    // 每 16B 填充 8 个 PadValue，PadValue 大小为 2B，1Block 的源数据会被扩充到 2Block
    PAD_MODE6,    // 每 32B 移除 14 个 PadValue，PadValue 大小为 2B，32B 的源数据会被缩小到 4B
    PAD_MODE7,    // 每 32B 移除 12 个 PadValue，PadValue 大小为 2B，32B 的源数据会被缩小到 8B
    PAD_MODE8,    // 每 32B 移除 8 个 PadValue，PadValue 大小为 2B，32B 的源数据会被缩小到 16B
};

enum class ConvRelu : uint8_t {
// CONV_RELU is only valid for the data moved between L0C/L0C16/L0C32/L0CSC16/L0CSC32/L0CDPF16/L0CDPF32 and UB
    CRMODE_NONE = 0,
    CRMODE_F32toF16_NONE,
    CRMODE_F32toF16_RELU,
    CRMODE_S32toF16_NONE,
    CRMODE_F16toF32_NONE,
    CRMODE_NONE_RELU,
    CRMODE_F16_MUL,
    CRMODE_S32toF16_DEQSCALE_SPR,
    CRMODE_DEQSCALE_VDEQ8,
    CRMODE_DEQSCALE_DEQ8,
    CRMODE_DEQSCALE_VDEQ16,
    CRMODE_DEQSCALE_DEQ16,
    CRMODE_DEQSCALE_VDEQS16,
    CRMODE_DEQSCALE_DEQS16,
};

/// 此枚举定义与编译器内置类型 event_t 保持一致
enum class EventID : uint8_t {
    EVENT_ID0 = 0,
    EVENT_ID1,
    EVENT_ID2,
    EVENT_ID3,
    EVENT_ID4,
    EVENT_ID5,
    EVENT_ID6,
    EVENT_ID7,
    VALID_EVENT_ID_SIZE,
};

/// 此枚举定义与编译器内置类型 ub_addr8_t 保持一致
// Vector 地址寄存器，每个大小为128bits
enum class VaAddr8 : uint8_t {
    VA0 = 0,
    VA1,
    VA2,
    VA3,
    VA4,
    VA5,
    VA6,
    VA7,
};

/// 此枚举定义与编译器内置类型 vpart_t 保持一致
// vld_ra_reg指令入参，表示从低地址或高地址读取 128 bits 数据
enum class Vpart : uint8_t {
    L128 = 0,
    H128,
};

/// 此枚举定义与编译器内置类型 Order_t 保持一致
// For VCMAX and VCMIN
enum class OrderType : uint8_t {
    VALUE_INDEX = 0,
    INDEX_VALUE,
    ONLY_VALUE,
    ONLY_INDEX,
};

// For VCMAX and VCMIN
enum class OrderWriteMode : uint8_t {
    WRITE_ONCE = 1,
    WRITE_TWICE = 2,
};

enum class ByteMode : uint8_t {
    BM_DISABLE = 0,
    BM_ENABLE
};

enum class UintFlag : int32_t {
    UF_MODE0 = 0,   // unit-flag mechanism disabled.
    RESERVED,      // reserved.
    UF_MODE2,       // unit-flag mechanism enabled, hardware checks the unit-flag but do not set unit-flag
    UF_MODE3,       // unit-flag mechanism enabled, hardware checks the unit-flag and set unit-flag
};

// 用于标识 set_atomic 系列接口的执行类型
enum class AtomicMode : uint8_t {
    NONE = 0,
    F32,
    F16,
    S16,
    S32,
    S8,
    BF16,
    SUM,
    MAX,
    MIN,
};

// 用于标识 simt_atom option的类型
enum class SimtAtomMode : uint8_t {
    NONE = 0,
    MAX,
    MIN,
    ADD,
    EXCH,
    CAS,
};

// 用于标识 set/rls buf指令的模式e
enum class BufMode : uint8_t {
    BLOCK_MODE = 0,
    NONBLOCK_MODE,
};

/// MemOpRecord 使用的内存操作类型枚举
enum class MemOpType : uint32_t { // 待修改为uint8_t
    MALLOC = 0U,
    FREE,
    MEMCPY_BLOCKS,
    LOAD,
    STORE,
    INVALID,
};

enum class AccessType: uint8_t {
    READ = 0U,
    WRITE,
    MEMCPY_BLOCKS,
};

/// 设备信息
struct DeviceInfoSummary {
    DeviceType device;
    uint32_t blockSize;
    uint32_t blockNum;
    int32_t deviceId;
};

struct KernelSummary {
    uint64_t pcStartAddr;
    uint32_t blockDim;
    KernelType kernelType;
    bool isKernelWithDBI;
    bool hasDebugLine;
    char kernelName[KERNEL_NAME_MAX];
};

struct BlockSummary {
    uint64_t reserved[7U];
    uint32_t reserved2;
    uint32_t validSize;
};

struct Location {
    uint64_t fileNo;
    uint64_t lineNo;
    uint64_t pc;
    uint16_t blockId;
    bool operator==(const Location &rhs) const
    {
        return this->fileNo == rhs.fileNo &&
               this->lineNo == rhs.lineNo &&
               this->pc == rhs.pc &&
               this->blockId == rhs.blockId;
    }
};

/// 当前核上记录信息对应的核类型，mix算子在这里已经拆分成VEC/CUBE记录
enum class BlockType : uint8_t {
    AIVEC,
    AICUBE,
    AICORE,
};

struct CompareMask {
    uint64_t mask0;
    uint64_t mask1;
};

struct VaRegister {
    bool operator==(const VaRegister &vaRegister) const
    {
        return l64 == vaRegister.l64 && h64 == vaRegister.h64;
    }

    uint64_t l64;
    uint64_t h64;
};

struct ParaBaseRegister {
    uint64_t addr = ILLEGAL_ADDR;
    uint64_t size;
};

// 寄存器类型
enum class RegisterType {
    VECTOR_MASK_0,
    VECTOR_MASK_1,
    CTRL,
    FFTS_BASE_ADDR,
    FPC,
    QUANT_PRE,
    QUANT_POST,
    LRELU_ALPHA,
    MAX,
};

enum class RegisterValueType : uint64_t {
    VAL_UINT64 = 0,
    VAL_HALF,
    VAL_FLOAT,
    VAL_INT32,
};

struct RegisterPayload {
    RegisterValueType regValType;
    uint64_t regVal;    // 动态插桩场景无法感知具体类型，统一使用uint64_保存和打印寄存器值，使用时根据 regValType 取值按对应格式解析
    int64_t regIdx;     // 物理核映射的idx，用于GlobalHead内持久化和工具侧回放时索引数组
};

struct RegisterSetRecord {
    Location location;
    RegisterPayload regPayLoad;
};

struct Register {
    uint64_t fmatrix;
    uint64_t fmatrixB;
    uint64_t l3dRpt;
    uint64_t vectorMask0 = ~0ULL;
    uint64_t vectorMask1 = ~0ULL;
    uint64_t ndParaConfig;
    uint64_t cmpMaskAddr = ~0UL;
    MaskMode maskMode;
    VaRegister va0;
    VaRegister va1;
    VaRegister va2;
    VaRegister va3;
    VaRegister va4;
    VaRegister va5;
    VaRegister va6;
    VaRegister va7;
    uint64_t sprLoopSizeUb2Out;
    uint64_t sprLoopSizeOut2Ub;
    uint64_t sprLoop1StrideUb2Out;
    uint64_t sprLoop1StrideOut2Ub;
    uint64_t sprLoop2StrideUb2Out;
    uint64_t sprLoop2StrideOut2Ub;
    uint64_t sprPadCntNdDma;
    uint64_t sprLoop0StrideNdDma;
    uint64_t sprLoop1StrideNdDma;
    uint64_t sprLoop2StrideNdDma;
    uint64_t sprLoop3StrideNdDma;
    uint64_t sprLoop4StrideNdDma;
    uint64_t sprLoopSizeOut2L1;
    uint64_t sprLoop1StrideOut2L1;
    uint64_t sprLoop2StrideOut2L1;
    uint64_t sprMte2NzPara;
    uint64_t sprMTE2SrcPara;
    uint64_t sprLoop3Para;
    uint64_t sprChannelPara;
    uint64_t sprFmatrix;
    uint64_t sprFmatrixB;
    uint64_t sprFmatrixDual0;
    uint64_t sprFmatrixDual1;
    uint64_t sprL3dRpt;
    uint64_t sprL3dRptB;
    uint64_t sprPadding;
    uint64_t sprPaddingB;
    uint64_t ctrl = 0x08ULL;
    uint64_t fftsBaseAddr;
    uint64_t fpc;
    uint64_t quantPre;
    uint64_t quantPost;
    RegisterPayload lreluAlpha;
    uint64_t rsv[5];    // 64字节对齐
};

/// 该结构体主要包含当前kernel包含的信息
struct KernelInfo {
    uint64_t totalBlockDim{};                         // 工具根据业务逻辑计算得到的blockDim
    uint64_t totalCacheSize{};
    uint32_t kernelParamNum{};                       // kernel入参个数
    KernelType kernelType{};                          // 当前算子的kernel类型，保存在0核头部
    uint64_t l2CacheOffset;
};

/// 该结构体主要包含当前block包含的信息，保存在每个核的头部
struct BlockInfo {
    uint64_t simtSyncThreadCount{};                 // 当前核上simt单元多少个线程已经运行了sync_thread指令
    uint16_t blockId{};
    uint16_t threadXDim{};
    uint16_t threadYDim{};
    uint16_t threadZDim{};
    BlockType blockType{};                            // 当前block的类型，代表当前核记录的信息属于VEC还是CUBE
    uint8_t vecSubBlockDim{};                         // 当前算子一个blockDim使用的vec核心数，保存在每个核的头部
};

/// 该结构体主要包含命令行传到kernel侧的参数信息，只保存在0核的头部
struct CheckParmsInfo {
    uint32_t cacheSize = 100;                         // 单个核申请的记录大小，单位为M
    int16_t checkBlockId = CHECK_ALL_BLOCK;           // 检查的blockId, 默认检测所有核的记录
    bool defaultcheck{};                              // 是否开启内存/未初始化/软件栈检测
    bool racecheck{};                                 // 是否开启竞争检测
    bool initcheck{};                                 // 是否开启未初始化检测
    bool synccheck{};                                 // 是否开启同步检测
    bool registerCheck{};                             // 是否开启寄存器检测
};

struct HostMemoryInfo {
    uint64_t addr;
    uint64_t size;
    bool operator<(const HostMemoryInfo& other) const
    {
        if (addr != other.addr) {
            return addr < other.addr;
        }
        return size < other.size;
    }
};

struct SimtInfo {
    uint64_t offset{};                                // simt信息记录的起始偏移
    uint64_t threadByteSize{};                        // 每个thread最多存储多少个字节
    uint64_t shadowMemoryOffset{};                    // shadow memory 起始偏移地址
    uint64_t shadowMemoryByteSize{};                  // shadow memory 最多使用多少字节
    uint32_t ubDynamicSize{};
};

struct SimtRecordBlockHeadImpl {
    uint64_t recordCount{};                 // 记录数量
    uint64_t recordWriteCount{};            // 已经写入的记录数量
    uint64_t offset{};                      // 所有记录对应的offset
    uint64_t writeOffset{};                 // 已经写入的记录对应的offset
};

using SimtRecordBlockHead = StructAlignBy<SimtRecordBlockHeadImpl, 64UL>;
static_assert(sizeof(SimtRecordBlockHead) % 64UL == 0UL, "SimtRecordBlockHead size should aligned by 64 bytes");

struct RecordGlobalHeadImpl {
    uint64_t securityVal = RECORD_HEAD_SECURITY_VALUE;
    CheckParmsInfo checkParms{};
    KernelInfo kernelInfo{};
    SimtInfo simtInfo{};
    bool supportSimt{false};                // 当前芯片类型是否支持simt

    Register registers[C220_A2_A3_MAXCORE_NUM]; // 保存核上寄存器状态，数组下标对应coreID
};

using RecordGlobalHead = StructAlignBy<RecordGlobalHeadImpl, 64UL>;
static_assert(sizeof(Register) % 64UL == 0UL, "Register size should aligned by 64 bytes");
static_assert(sizeof(RecordGlobalHead) % 64UL == 0UL, "RecordGlobalHead size should aligned by 64 bytes");

/// 后续更改该结构体需保证securityVal位于结构体的第一个元素
/// MemInfoIsInvalid函数将该结构体指针头部的8个字节数据和RECORD_HEAD_SECURITY_VALUE做了相等判断
/// 为了保证版本向后兼容，需要通过reserve数组将RecordBlockHead控制在128B，否则会出现record解析错误，
/// 因此在更新完此数据结构时，同时更新reserve字段
struct RecordBlockHeadImpl {
    uint64_t recordCount{};                 // 记录数量
    uint64_t recordWriteCount{};            // 已经写入的记录数量
    uint64_t offset{};                      // 所有记录对应的offset
    uint64_t writeOffset{};                 // 已经写入的记录对应的offset
    BlockInfo blockInfo{};
    uint32_t hostMemoryNum{};               // 算子host侧memory输入个数
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    __gm__ HostMemoryInfo *hostMemoryInfoPtr{nullptr};     // 算子host侧memory输入数据，kernel侧为gm地址
#else
    HostMemoryInfo *hostMemoryInfoPtr{nullptr};            // 算子host侧memory输入数据，host侧为堆地址
#endif
    uint32_t mstxFuseScopeDepth{};          // 融合语义深度
    bool extraWriteSuccess{false};          // extra地址信息是否写入成功
    ParaBaseRegister paraBase;
};

using RecordBlockHead = StructAlignBy<RecordBlockHeadImpl, 64UL>;
static_assert(sizeof(RecordBlockHead) % 64UL == 0UL, "RecordBlockHead size should aligned by 64 bytes");

/**
 * shadow memory动态分配空间管理
*/
struct ShadowMemoryHeapHeadImpl {
    uint64_t startAddr{0U};
    uint64_t size{0U};
    uint64_t current{0U};
    uint64_t lock{0U};
};

using ShadowMemoryHeapHead = StructAlignBy<ShadowMemoryHeapHeadImpl, 64UL>;
static_assert(sizeof(ShadowMemoryHeapHead) % 64UL == 0UL, "ShadowMemoryHeapHead size should aligned by 64 bytes");

struct MstxCrossRecord {
    uint64_t addr;
    uint16_t flagId;
    PipeType pipe;
    bool isMore;
    bool isMerge;
    bool operator == (const MstxCrossRecord &other) const
    {
        return
            addr == other.addr &&
            flagId == other.flagId &&
            pipe == other.pipe &&
            isMore == other.isMore &&
            isMerge == other.isMerge;
    }
};

// 与AscendC约定的hccl定长结构体
struct MstxHcclRecord {
    uint64_t src;
    uint64_t dst;
    uint64_t srcCount;
    uint64_t dstCount;
    uint64_t srcStride;
    uint64_t dstStride;
    uint64_t srcRepeatStride;
    uint64_t dstRepeatStride;
    size_t srcDataTypeSize;
    size_t dstDataTypeSize;
    uint32_t repeat;
    int32_t rankDim;
    int32_t flagId; // flagId代表hccl接口的类型，allReduce表示0，allGather表示1，reduceScatter表示2，alltoall表示3
};

// MstxHcclRecord的flagId是由AscendC传入因此定义为int32_t
enum class HcclFlagId {
    ALLREDUCE = 0,
    ALLGATHER = 1,
    REDUCESCATTER = 2,
    ALLTOALL = 3,
};

struct MstxHcclCoreRecord {
    uint64_t src;
    uint64_t dst;
    uint64_t srclenBurst;
    uint64_t dstlenBurst;
    uint32_t repeat;
};

// 与AscendC约定的hccl变长结构体
struct MstxHcclRecordV {
    uint64_t src;
    uint64_t dst;
    uint64_t *srcCount;
    uint64_t *dstCount;
    uint64_t *srcStride;
    uint64_t *dstStride;
    uint64_t srcRepeatStride;
    uint64_t dstRepeatStride;
    size_t srcDataTypeSize;
    size_t dstDataTypeSize;
    uint32_t repeat;
    int32_t rankDim;
    int32_t flagId; // flagId为0表示AlltoAllV接口
};

struct MstxRecord {
    InterfaceType interfaceType;
    uint32_t bufferLens;
    Location location;
    bool error;
    union Interface {
        MstxCrossRecord mstxCrossRecord;
        MstxHcclRecord mstxHcclRecord;
        MstxHcclCoreRecord mstxHcclCoreRecord; // 一条MstxHcclRecordV会转换为多条MstxHcclCoreRecord
        MstxVecUnaryDesc mstxVecUnaryDesc;
        MstxVecBinaryDesc mstxVecBinaryDesc;
        MstxDataCopyDesc mstxDataCopyDesc;
        MstxDataCopyPadDesc mstxDataCopyPadDesc;
        MstxCrossCoreBarrier mstxCrossCoreBarrier;
        MstxCrossCoreSetFlag mstxCrossCoreSetFlag;
        MstxCrossCoreWaitFlag mstxCrossCoreWaitFlag;
    } interface;
};

struct LoadStoreRecord {
    bool operator==(const LoadStoreRecord &record)
    {
        return addr == record.addr && size == record.size &&
               space == record.space && location == record.location &&
               alignSize == record.alignSize;
    }
    uint64_t addr;
    uint64_t size;
    Location location;
    AddressSpace space;
    DetailedDataType dataType;
    uint8_t alignSize;
};

struct RedRecord {
    uint64_t addr;
    uint64_t size;
    Location location;
    uint8_t isAtom;
    DetailedDataType detailedDataType;
};

struct DmaMovRecord {
    uint64_t dst;
    uint64_t src;
    Location location;
    uint16_t nBurst;
    uint16_t lenBurst;
    uint16_t srcStride;
    uint16_t dstStride;
    MemType dstMemType;
    MemType srcMemType;
    PadMode padMode;
    ByteMode byteMode;
};

struct DmaMovConvReluRecord {
    uint64_t dst;
    uint64_t src;
    Location location;
    uint16_t nBurst;
    uint16_t lenBurst;
    uint16_t srcStride;
    uint16_t dstStride;
    ConvRelu crMode;
    MemType dstMemType;
    MemType srcMemType;
    DataType srcDataType;
    DataType dstDataType;
};

struct DmaMovNd2nzRecord {
    uint64_t dst;
    uint64_t src;
    Location location;

    uint16_t ndNum;
    uint16_t nValue;
    uint16_t dValue;
    uint16_t srcNdMatrixStride;
    uint16_t srcDValue;
    uint16_t dstNzC0Stride;
    uint16_t dstNzNStride;
    uint16_t dstNzMatrixStride;

    MemType srcMemType;
    MemType dstMemType;
    DataType dataType;
};

struct DmaMovNd2nzDavRecord {
    uint64_t dst; // must be 32B aligned, (d//C0)nC0 format and C0 is 32B
    uint64_t src; // byte aligned
    Location location;
    uint64_t loop4SrcStride; // 40 bits src_nd_matrix_stride, in unit of byte
    uint64_t loop1SrcStride; // 40 bits src_D * sizeof(src_type), in unit of byte
    uint32_t dValue;         // 21 bits
    uint16_t nValue;         // 16 bits
    uint16_t ndNum;          // 16 bits
    uint16_t loop2DstStride; // 16 bits dst_nz_n_stride, in unit of C0_size
    uint16_t loop3DstStride; // 16 bits dst_nz_c0_stride, in unit of C0_size
    uint16_t loop4DstStride; // 16 bits, in unit of C0_size, dst_nz_matrix_stride * sizeof(dst_type) / C0_size
    uint8_t smallC0;         // small C0 mode enable bit

    MemType srcMemType;
    MemType dstMemType;
    DataType dataType;
};

struct MovAlignRecord {
    uint64_t dst;
    uint64_t src;
    Location location;
    uint32_t srcGap;
    uint32_t dstGap;
    uint32_t lenBurst;
    uint16_t nBurst;
    MemType dstMemType;
    MemType srcMemType;
    DataType dataType;
    uint8_t leftPaddingNum;
    uint8_t rightPaddingNum;
};

struct MovAlignRecordV2 {
    uint64_t dst;
    uint64_t src;
    Location location;
    uint64_t loop1DstStride;  // 40 or 21 bits
    uint64_t loop2DstStride;  // 40 or 21 bits
    uint64_t dstStride;       // 40 or 21 bits
    uint64_t srcStride;       // 40 or 21 bits
    uint64_t loop1SrcStride;  // 40 or 21 bits
    uint64_t loop2SrcStride;  // 40 or 21 bits
    uint32_t nBurst;          // 21 bits
    uint32_t lenBurst;        // 21 bits
    uint32_t loop1Size;       // 21 bits
    uint32_t loop2Size;       // 22 bits
    MemType dstMemType;
    MemType srcMemType;
    DataType dataType;
    uint8_t leftPaddingNum;   // 6 bits, data count of left pad data, valid when src is gm
    uint8_t rightPaddingNum;  // 6 bits, data count of right pad data, valid when src is gm
};

// 依据指令手册该章节的算法3/4，src/dst_stride * element_size才是偏移内存
struct LoopInfo {
    bool operator==(const LoopInfo &r) const
    {
        return
            loopSrcStride == r.loopSrcStride &&
            loopDstStride == r.loopDstStride &&
            loopSize == r.loopSize &&
            loopLpSize == r.loopLpSize &&
            loopRpSize == r.loopRpSize;
    }

    uint64_t loopSrcStride; // 40 bits, src stride of each element
    uint32_t loopDstStride; // 20 bits, dst stride of each element, also works for Lp/Rp data
    uint32_t loopSize;      // 20 bits, src element number of this loop(without padding)
    uint8_t loopLpSize;     // 8 bits, left padding data count
    uint8_t loopRpSize;     // 8 bits, right padding data count
};

struct NdDMAOut2UbRecord {
    static constexpr uint64_t LOOP = 5;

    uint64_t dst;      // byte aligned, start address after padding
    uint64_t src;      // data type aligned
    Location location;
    LoopInfo loop[LOOP];
    DataType dataType;
};

struct MovBtRecord {
    uint64_t dst;
    uint64_t src;
    Location location;
    uint16_t srcGap;
    uint16_t dstGap;
    uint16_t lenBurst;
    uint16_t nBurst;
    uint16_t convControl;
    MemType dstMemType;
    MemType srcMemType;
};

struct MovFpRecord {
    uint64_t dst;
    uint64_t src;
    Location location;
    uint32_t dstStride;
    uint32_t dstNdStride;
    uint16_t srcStride;
    uint16_t nSize;
    uint16_t mSize;
    uint16_t ndNum;
    uint16_t srcNdStride;
    uint16_t srcNzC0Stride;
    uint16_t quantPreBits;
    bool enUnitFlag;
    bool int8ChannelMerge;
    bool int4ChannelMerge;
    bool channelSplit;
    bool enNZ2ND;
    bool enNZ2DN;
    bool isC310;
};

struct VecDupRecord {
    uint64_t dst;
    Location location;
    VectorMask vectorMask;
    uint16_t dstBlockStride;
    uint16_t dstRepeatStride;
    uint8_t repeat;
    uint8_t dataBits;
    MaskMode maskMode;
};

struct UnaryOpRecord {
    uint64_t dst;
    uint64_t src;
    Location location;
    VectorMask vectorMask;
    uint64_t dstBlockSize;
    uint64_t srcBlockSize;
    uint16_t dstBlockStride;
    uint16_t srcBlockStride;
    uint16_t dstRepeatStride;
    uint16_t srcRepeatStride;
    uint8_t repeat;
    uint8_t dstBlockNum;
    uint8_t srcBlockNum;
    uint8_t dstDataBits;
    uint8_t srcDataBits;
    MaskMode maskMode;
};

struct VgatherRecord {
    uint64_t dst;
    uint64_t src;
    Location location;
    VectorMask vectorMask;
    uint32_t dstBlockSize;
    uint32_t srcBlockSize;
    uint32_t offsetAddr;
    uint16_t dstRepeatStride;
    uint16_t dstBlockNum;  // repeat * N, count最多有255 * 128 = 32640
    uint16_t srcBlockNum;
    uint16_t dstAlignSize;
    uint8_t dstBlockStride;
    uint8_t dstRepeat;
    uint8_t dstDataBits;
    MaskMode maskMode;
};

struct ElementRecord {
    Location location;
    uint64_t addr;
    uint16_t blockSize;
    uint16_t blockNum;
    uint16_t alignSize;
    uint8_t dataBits;
    AccessType accessType;
};

struct BinaryOpRecord {
    uint64_t dst;
    uint64_t src0;
    uint64_t src1;
    Location location;
    VectorMask vectorMask;
    uint16_t dstBlockStride;
    uint16_t src0BlockStride;
    uint16_t src1BlockStride;
    uint16_t dstRepeatStride;
    uint16_t src0RepeatStride;
    uint16_t src1RepeatStride;
    uint16_t dstBlockSize;
    uint16_t src0BlockSize;
    uint16_t src1BlockSize;
    uint8_t repeat;
    uint8_t dstBlockNum;
    uint8_t src0BlockNum;
    uint8_t src1BlockNum;
    uint8_t dstDataBits;
    uint8_t src0DataBits;
    uint8_t src1DataBits;
    MaskMode maskMode;
};

struct MarixMulOpRecord {
    uint64_t dst;
    uint64_t src0;
    uint64_t src1;
    uint16_t m;
    uint16_t k;
    uint16_t n;
    Location location;
    uint16_t src0RepeatStride;
    uint16_t dstBlockSize;
    uint16_t src0BlockSize;
    uint16_t src1BlockSize;
    uint16_t src0Repeat;
    uint16_t dstBlockNum;
    uint16_t src0BlockNum;
    uint16_t src1BlockNum;
    uint16_t dstAlignSize;
    uint16_t src0AlignSize;
    uint16_t src1AlignSize;
    bool cmatrixSource;
    bool cmatrixInitVal;
    bool enUnitFlag;
};

struct MmadA5Record {
    bool operator==(const MmadA5Record &record) const
    {
        return dst == record.dst &&
            src0 == record.src0 &&
            src1 == record.src1 &&
            location == record.location &&
            m == record.m &&
            k == record.k &&
            n == record.n &&
            dstBlockNum == record.dstBlockNum &&
            src0BlockNum == record.src0BlockNum &&
            src1BlockNum == record.src1BlockNum &&
            src0AlignSize == record.src0AlignSize &&
            src1AlignSize == record.src1AlignSize &&
            src0Dtype == record.src0Dtype &&
            src1Dtype == record.src1Dtype &&
            cmatrixSource == record.cmatrixSource &&
            cmatrixInitVal == record.cmatrixInitVal &&
            enUnitFlag == record.enUnitFlag;
    }

    uint64_t dst;
    uint64_t src0;
    uint64_t src1;
    Location location;
    uint16_t m;
    uint16_t k;
    uint16_t n;
    uint16_t dstBlockNum;
    uint16_t src0BlockNum;
    uint16_t src1BlockNum;
    uint16_t src0AlignSize;
    uint16_t src1AlignSize;
    DetailedDataType src0Dtype;
    DetailedDataType src1Dtype;
    bool cmatrixSource;
    bool cmatrixInitVal;
    bool enUnitFlag;
};

struct VecRegPropCoordOpRecord {
    uint64_t dst;
    uint64_t src;
    Location location;
    DataType dataType;
    uint8_t regionRange;
    uint8_t repeat;
    bool isExtract;
};

struct ReduceOpRecord {
    uint64_t dst;
    uint64_t src;
    Location location;
    VectorMask vectorMask;
    uint16_t srcBlockStride;
    uint16_t dstRepeatStride;
    uint16_t srcRepeatStride;
    uint16_t dstRepeatLength;
    uint16_t dstAlignSize;
    uint8_t repeat;
    uint8_t dstBlockNum;
    uint8_t srcBlockNum;
    uint8_t dstBlockSize;
    uint8_t srcBlockSize;
    uint8_t dstDataBits;
    uint8_t srcDataBits;
    uint8_t dstDataBitsFactor;
    MaskMode maskMode;
};

struct ReduceV2Record {
    uint64_t dst;
    uint64_t src0;
    uint64_t src1;
    Location location;
    VectorMask vectorMask;
    CompareMask compareMask;
    uint16_t repeat;
    uint16_t src0RepeatStride;
    uint8_t src1RepeatStride;
    uint8_t src0BlockStride;
    uint8_t dataBytes;
    uint8_t patternMode;
    MaskMode maskMode;
};

struct SyncRecord {
    Location location;
    PipeType src;
    PipeType dst;
    uint64_t eventID;
    bool isGenerated;  // 是否是生成的非原生指令
};

struct BufRecord {
    Location location;
    PipeType pipe;
    uint64_t bufId;
    BufMode mode;
    bool isPreprocessed;       // 当前buf指令是否已经被预处理过
};

struct HardSyncRecord {
    Location location;
    PipeType src;
    PipeType dst;
    uint64_t eventID;
    MemType memory;
    uint8_t v;
};

struct SoftSyncRecord {
    Location location;
    uint16_t waitCoreID; // 被等的核ID
    int32_t usedCores;
    int32_t eventID;
    bool isAIVOnly;
};

struct PipeBarrierRecord {
    Location location;
    PipeType pipe;
};

struct FftsSyncRecord {
    Location location;
    PipeType dst;
    uint8_t mode;
    uint8_t flagID;
    uint8_t vecSubBlockDim;
};
 
struct WaitFlagDevRecord {
    Location location;
    uint8_t flagID;
};

struct WaitFlagDevPipeRecord {
    Location location;
    uint8_t flagID;
    PipeType pipe;
};

struct IntraBlockSyncRecord {
    Location location;
    PipeType pipe;
    uint64_t syncID;
    uint8_t vecSubBlockDim;
};

enum class AddrCalMode {
    INC = 0,
    DEC = 1,
};

enum class TransposeMode {
    DISABLE = 0,
    ENABLE = 1,
};

struct LoadAWinogradRecord {
    Location location;
    uint64_t dst;
    uint64_t src;
    uint16_t fmSizeW;
    uint16_t fmSizeH;
    uint16_t fmSizeCh;  // C = C1 * C0
    uint16_t extStepK;  // shape of output sub matrix along K direction
    uint16_t extStepM;  // shape of output sub matrix along M direction
    uint16_t dstStartPointK;
    uint16_t dstStartPointM;
    uint8_t innerDstGap;
    DataType dataType;
    MemType dstMemType;
    MemType srcMemType;
};

struct LoadBWinogradRecord {
    Location location;
    uint64_t dst;
    uint64_t src;
    uint16_t srcRptStride;
    uint16_t dstRptStride;
    uint8_t innerDstStride;
    uint8_t repeat;
    MemType dstMemType;
    MemType srcMemType;
};

struct Load2DRecord {
    Location location;
    uint64_t dst;
    uint64_t src;
    uint16_t baseIdx;
    uint16_t srcStride;
    uint16_t dstStride;
    uint16_t blockSize;
    uint16_t repeat;
    MemType dstMemType;
    MemType srcMemType;
    AddrCalMode addrCalMode;
};

struct LoadL12DRecord {
    bool operator == (const LoadL12DRecord &record) const noexcept
    {
        return this->location == record.location &&
               this->dst == record.dst &&
               this->src == record.src &&
               this->mStartPosition == record.mStartPosition &&
               this->kStartPosition == record.kStartPosition &&
               this->mStep == record.mStep &&
               this->kStep == record.kStep &&
               this->srcStride == record.srcStride &&
               this->dstStride == record.dstStride &&
               this->dstMemType == record.dstMemType &&
               this->transposeMode == record.transposeMode &&
               this->detailedDataType == record.detailedDataType;
    }
    Location location;
    uint64_t dst;
    uint64_t src;
    uint16_t mStartPosition;
    uint16_t kStartPosition;
    uint16_t mStep;
    uint16_t kStep;
    uint16_t srcStride;
    uint16_t dstStride;
    MemType dstMemType;
    TransposeMode transposeMode;
    DetailedDataType detailedDataType;
};

struct LoadL12DTransposeRecord {
    bool operator == (const LoadL12DTransposeRecord &record) const noexcept
    {
        return this->location == record.location &&
               this->dst == record.dst &&
               this->src == record.src &&
               this->repeat == record.repeat &&
               this->srcStride == record.srcStride &&
               this->dstStride == record.dstStride &&
               this->srcFracStride == record.srcFracStride &&
               this->dstFracStride == record.dstFracStride &&
               this->detailedDataType == record.detailedDataType;
    }
    Location location;
    uint64_t dst;
    uint64_t src;
    uint16_t repeat;
    uint16_t srcStride;
    uint16_t dstStride;
    uint16_t srcFracStride;
    uint16_t dstFracStride;
    DetailedDataType detailedDataType;
};

struct LoadL1Mx2DRecord {
    bool operator == (const LoadL1Mx2DRecord &record) const noexcept
    {
        return this->location == record.location &&
               this->dst == record.dst &&
               this->src == record.src &&
               this->xStartPosition == record.xStartPosition &&
               this->yStartPosition == record.yStartPosition &&
               this->xStep == record.xStep &&
               this->yStep == record.yStep &&
               this->srcStride == record.srcStride &&
               this->dstStride == record.dstStride &&
               this->dstMemType == record.dstMemType;
    }
    Location location;
    uint64_t dst;
    uint64_t src;
    uint16_t xStartPosition;
    uint16_t yStartPosition;
    uint16_t xStep;
    uint16_t yStep;
    uint16_t srcStride;
    uint16_t dstStride;
    MemType dstMemType;
};

struct Load2DSparseRecord {
    Location location;
    uint64_t dst;
    uint32_t src0;
    uint32_t src1;
    uint16_t startId;
    uint8_t repeat;
    MemType dstMemType;
    MemType srcMemType;
};

struct Load2DTransposeRecord {
    Location location;
    uint64_t dst;
    uint64_t src;
    uint16_t indexId;
    uint16_t srcStride;
    uint16_t dstStride;
    uint16_t dstFracStride;
    uint8_t repeat;
    bool addrMode;
    DataType dataType;
    MemType dstMemType;
    MemType srcMemType;
};

struct DecompressHeaderRecord {
    Location location;
    uint64_t src;
    uint16_t nBlock;
    MemType srcMemType;
};

struct BroadcastRecord {
    Location location;
    uint64_t dst;
    uint64_t src;
    uint8_t nBurst;
    uint8_t lenBurst;
    uint8_t srcGap;
    uint8_t dstGap;
    bool enableRepeat;
    DataType srcDataType;
    DataType dstDataType;
    MemType dstMemType;
    MemType srcMemType;
};

struct ScatterVnchwconvRecord {
    bool operator==(const ScatterVnchwconvRecord &record) const
    {
        return location == record.location &&
               dst0 == record.dst0 &&
               dst1 == record.dst1 &&
               src0 == record.src0 &&
               src1 == record.src1 &&
               dstStride == record.dstStride &&
               srcStride == record.srcStride &&
               repeat == record.repeat &&
               dstHighHalf == record.dstHighHalf &&
               srcHighHalf == record.srcHighHalf &&
               dataType == record.dataType;
    }

    Location location;
    VaRegister dst0;
    VaRegister dst1;
    VaRegister src0;
    VaRegister src1;
    uint16_t dstStride;
    uint16_t srcStride;
    uint8_t repeat;
    bool dstHighHalf;
    bool srcHighHalf;
    DataType dataType;
};

struct Vbs32Record {
    bool operator==(const Vbs32Record &record) const
    {
        return dst == record.dst &&
            src0 == record.src0 &&
            src1 == record.src1 &&
            location == record.location &&
            repeat == record.repeat &&
            dataType == record.dataType;
    }

    uint64_t dst;
    uint64_t src0;
    uint64_t src1;
    Location location;
    uint8_t repeat;
    DataType dataType;
};

struct Vms4v2RecordA5 {
    static constexpr uint32_t ARRAY_NUM = 4;
    static constexpr uint32_t SRC_UNIT = 3; // src的每个地址左移3位才是真实ub地址

    bool operator==(const Vms4v2RecordA5 &record) const
    {
        bool same = true;
        for (uint32_t i = 0; i < ARRAY_NUM; ++i) {
            same &= src[i] == record.src[i] && elementNum[i] == record.elementNum[i];
        }
        return same && dst == record.dst &&
            location == record.location &&
            repeat == record.repeat &&
            validMask == record.validMask &&
            isAllStored == record.isAllStored &&
            detailedDataType == record.detailedDataType;
    }

    uint64_t dst;
    Location location;
    uint16_t src[ARRAY_NUM];
    uint16_t elementNum[ARRAY_NUM];
    uint8_t repeat;
    uint8_t validMask;
    bool isAllStored;
    DetailedDataType detailedDataType;
};

struct DcPreloadRecord {
    Location location;
    uint64_t addr;
    int64_t offset;
};

struct LoadB2Record {
    Location location;
    uint64_t dst;
    uint64_t src;
    uint8_t repeat;
    DataType dataType;
    MemType dstMemType;
    MemType srcMemType;
};

struct Load3DRecord {
    Location location;
    uint64_t dst;
    uint64_t src;
    uint16_t fMapW;
    uint16_t fMapH;
    uint16_t fMapC;
    uint16_t filterW;
    uint16_t filterH;
    uint16_t matrixRptStride;
    uint16_t filterWStride;
    uint16_t filterHStride;
    uint16_t matrixKPos;
    uint16_t matrixMPos;
    uint16_t matrixKStep;
    uint16_t matrixMStep;
    uint16_t matrixRptTimes;
    DataType dataType;
    MemType dstMemType;
    MemType srcMemType;
    uint8_t fMapTopPad;
    uint8_t fMapBottomPad;
    uint8_t fMapLeftPad;
    uint8_t fMapRightPad;
    uint8_t filterWDilation;
    uint8_t filterHDilation;
    uint8_t matrixMode;
    uint8_t matrixRptMode;
};

struct Load3DV2Record {
    bool operator == (const Load3DV2Record &record) const noexcept
    {
        return this->location == record.location &&
               this->dst == record.dst &&
               this->src == record.src &&
               this->dstStride == record.dstStride &&
               this->fMapW == record.fMapW &&
               this->fMapH == record.fMapH &&
               this->fMapC == record.fMapC &&
               this->filterW == record.filterW &&
               this->filterH == record.filterH &&
               this->matrixRptStride == record.matrixRptStride &&
               this->filterWStride == record.filterWStride &&
               this->filterHStride == record.filterHStride &&
               this->matrixKPos == record.matrixKPos &&
               this->matrixMPos == record.matrixMPos &&
               this->matrixKStep == record.matrixKStep &&
               this->matrixMStep == record.matrixMStep &&
               this->matrixRptTimes == record.matrixRptTimes &&
               this->outputMPos == record.outputMPos &&
               this->dataType == record.dataType &&
               this->dstMemType == record.dstMemType &&
               this->srcMemType == record.srcMemType &&
               this->fMapTopPad == record.fMapTopPad &&
               this->fMapBottomPad == record.fMapBottomPad &&
               this->fMapLeftPad == record.fMapLeftPad &&
               this->fMapRightPad == record.fMapRightPad &&
               this->filterWDilation == record.filterWDilation &&
               this->filterHDilation == record.filterHDilation &&
               this->matrixMode == record.matrixMode &&
               this->matrixRptMode == record.matrixRptMode &&
               this->transposeMode == record.transposeMode;
    }
    Location location;
    uint64_t dst;
    uint64_t src;
    uint16_t dstStride;
    uint16_t fMapW;
    uint16_t fMapH;
    uint16_t fMapC;
    uint16_t filterW;
    uint16_t filterH;
    uint16_t matrixRptStride;
    uint16_t filterWStride;
    uint16_t filterHStride;
    uint16_t matrixKPos;
    uint16_t matrixMPos;
    uint16_t matrixKStep;
    uint16_t matrixMStep;
    uint16_t matrixRptTimes;
    uint16_t outputMPos;
    DataType dataType;
    MemType dstMemType;
    MemType srcMemType;
    uint8_t fMapTopPad;
    uint8_t fMapBottomPad;
    uint8_t fMapLeftPad;
    uint8_t fMapRightPad;
    uint8_t filterWDilation;
    uint8_t filterHDilation;
    uint8_t matrixMode;
    uint8_t matrixRptMode;
    bool transposeMode;
};

struct AtomicModeRecord {
    Location location;
    AtomicMode mode;
};

struct Set2DRecord {
    Location location;
    uint64_t dst;
    uint32_t dstBlockSize;
    uint16_t repeat;
    uint16_t dstBlockNum;
    uint16_t repeatGap;
    MemType dstMemType;
};

struct SetL12DRecord {
    bool operator == (const SetL12DRecord &record) const noexcept
    {
        return this->location == record.location &&
               this->dst == record.dst &&
               this->repeat == record.repeat &&
               this->dstBlockNum == record.dstBlockNum &&
               this->repeatGap == record.repeatGap &&
               this->detailedDataType == record.detailedDataType;
    }
    Location location;
    uint64_t dst;
    uint16_t repeat;
    uint16_t dstBlockNum;
    uint16_t repeatGap;
    DetailedDataType detailedDataType;
};

struct MovL1UBRecord {
    bool operator == (const MovL1UBRecord &record) const
    {
        return
            (this->location == record.location) &&
            (this->src == record.src) &&
            (this->dst == record.dst) &&
            (this->nBurst == record.nBurst) &&
            (this->lenBurst == record.lenBurst) &&
            (this->srcGap == record.srcGap) &&
            (this->dstGap == record.dstGap) &&
            (this->dstMemType == record.dstMemType) &&
            (this->srcMemType == record.srcMemType);
    }
    uint64_t dst;
    uint64_t src;
    Location location;
    uint16_t nBurst;
    uint16_t lenBurst;
    uint16_t srcGap;
    uint16_t dstGap;
    MemType dstMemType;
    MemType srcMemType;
};

struct LoadImageRecord {
    Location location;
    uint64_t dst;
    uint16_t horSize;
    uint16_t verSize;
    uint16_t horStartP;
    uint16_t verStartP;
    uint16_t sHorRes;
    uint16_t lPadSize;
    uint16_t rPadSize;
    DataType dataType;
    MemType dstMemType;
    uint8_t topPadSize;
    uint8_t botPadSize;
};

struct LoadSmaskRecord {
    Location location;
    uint64_t src;
    uint16_t dst;
    uint16_t smaskSize;
    MemType srcMemType;
};

struct CmpMaskRecord {
    Location location;
    uint64_t addr;
    uint64_t size;
    AccessType accessType;
};

struct MovL1BtRecord {
    Location location;
    uint64_t dst;
    uint64_t src;
    uint16_t nBurst;
    uint16_t lenBurst;
    uint16_t srcGap;
    uint16_t dstGap;
    DetailedDataType dataType;
    uint8_t cvtEnable;
};

struct MovL1FbRecord {
    Location location;
    uint64_t dst;
    uint64_t src;
    uint16_t nBurst;
    uint16_t lenBurst;
    uint16_t srcStride;
    uint16_t dstStride;
    uint8_t dstMemBlock;
};

struct SimtThreadLocation {
    uint16_t idX;
    uint16_t idY;
    uint16_t idZ;

    bool operator == (const SimtThreadLocation &rhs) const
    {
        return this->idX == rhs.idX &&
               this->idY == rhs.idY &&
               this->idZ == rhs.idZ;
    }

    bool operator < (const SimtThreadLocation& rhs) const
    {
        if (idZ != rhs.idZ) return idZ < rhs.idZ;
        if (idY != rhs.idY) return idY < rhs.idY;
        return idX < rhs.idX;
    }
};

struct SimtLoadStoreRecord {
    uint64_t addr;
    uint64_t size;
    Location location;
    SimtThreadLocation threadLoc;
    AddressSpace space;
    DetailedDataType detailedDataType;
};

struct SimtAtomRecord {
    uint64_t addr;
    uint64_t size;
    Location location;
    SimtThreadLocation threadLoc;
    AddressSpace space;
    DetailedDataType detailedDataType;
    SimtAtomMode option;
};

struct SimtSyncRecord {
    Location location;
    SimtThreadLocation threadLoc;
};

struct ShadowMemoryRecordHead {
    uint32_t type = static_cast<uint32_t>(RecordType::SHADOW_MEMORY);
    uint64_t recordCount;
};

struct ShadowMemoryRecord {
    uint64_t addr;
    uint64_t size;
    Location location;
    SimtThreadLocation threadLoc;
    AddressSpace space;
    MemOpType opType;
};

enum class KernelErrorType : uint8_t {
    ILLEGAL_ADDR_WRITE = 0U,
    ILLEGAL_ADDR_READ,
    MISALIGNED_ACCESS,
    THREAD_OVERLAP,                // 线程间踩踏
    THREAD_RW_RACE,                // 线程间读写竞争，先读后写
    THREAD_WR_RACE,                // 线程间写读竞争，先写后读
    THREAD_WW_RACE,                // 线程间写写竞争

    MAX,
    INVALID = 0XFF,
};

struct KernelIllegalErrorDesc {
    uint64_t addr;
    uint64_t illegalSize;
};

struct KernelMisAlignErrorDesc {
    uint64_t addr;
    uint64_t misAlignSize;
};

struct KernelOverLapErrorDesc {
    Location conflictedLocation;
    SimtThreadLocation conflictedThreadLoc;
    uint64_t addr;
    uint64_t overLapSize;
};

struct KernelRaceErrorDesc {
    Location conflictedLocation;
    SimtThreadLocation conflictedThreadLoc;
    uint64_t addr;
};

struct KernelErrorDesc {
    Location location;
    SimtThreadLocation threadLoc;
    AddressSpace space;
    KernelErrorType errorType;
    uint64_t l1StartAddr; // L1StartAddr
    uint64_t l2StartAddr; // L2StartAddr
    uint64_t l2MemStatusAddr; // 字节状态位地址
    union Payload {
       KernelIllegalErrorDesc illegalDesc;
       KernelMisAlignErrorDesc misAlignDesc;
       KernelOverLapErrorDesc overLapDesc;
       KernelRaceErrorDesc raceDesc;
    } payload;
};

struct KernelErrorRecord {
    uint32_t errorNum;                                           // 错误类型个数，同一条记录可能对应多条错误信息
    RecordType recordType;                                       // 错误类型对应的桩记录类型
    uint16_t recordSize;
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    __gm__ const KernelErrorDesc *kernelErrorDesc;               // 存储多条错误类型，kernel侧为gm地址
    __gm__ const void *record;                                   // 存储recordType对应的record
#else
    const KernelErrorDesc *kernelErrorDesc;                      // 存储多条错误类型，host侧为堆地址
    const void *record;
#endif
};

struct KernelRecord {
    RecordType recordType;
    BlockType blockType;
    uint64_t serialNo;
    union Payload {
        LoadStoreRecord loadStoreRecord;
        DmaMovRecord dmaMovRecord;
        DmaMovConvReluRecord dmaMovConvReluRecord;
        DmaMovNd2nzRecord dmaMovNd2nzRecord;
        MovAlignRecord movAlignRecord;
        MovAlignRecordV2 movAlignRecordV2;
        MovBtRecord movBtRecord;
        MovFpRecord movFpRecord;
        NdDMAOut2UbRecord ndDMAOut2UbRecord;
        VecDupRecord vecDupRecord;
        UnaryOpRecord unaryOpRecord;
        VgatherRecord vgatherRecord;
        ElementRecord elementRecord;
        BinaryOpRecord binaryOpRecord;
        ReduceOpRecord reduceOpRecord;
        ReduceV2Record reduceV2Record;
        MarixMulOpRecord matrixMulOpRecord;
        VecRegPropCoordOpRecord vecRegPropCoordOpRecord;
        SyncRecord syncRecord;
        BufRecord bufRecord;
        HardSyncRecord hardSyncRecord;
        SoftSyncRecord softSyncRecord;
        PipeBarrierRecord pipeBarrierRecord;
        Load2DRecord load2DRecord;
        LoadL12DRecord loadL12DRecord;
        LoadL1Mx2DRecord loadL1Mx2DRecord;
        LoadL12DTransposeRecord loadL12DTransposeRecord;
        Load2DSparseRecord load2DSparseRecord;
        Load2DTransposeRecord load2DTransposeRecord;
        DecompressHeaderRecord decompressHeaderRecord;
        BroadcastRecord broadcastRecord;
        ScatterVnchwconvRecord scatterVnchwconvRecord;
        LoadB2Record loadB2Record;
        LoadAWinogradRecord loadAWinogradRecord;
        LoadBWinogradRecord loadBWinogradRecord;
        Load3DRecord load3DRecord;
        Load3DV2Record load3DV2Record;
        LoadImageRecord loadImageRecord;
        LoadSmaskRecord loadSmaskRecord;
        FftsSyncRecord fftsSyncRecord;
        MstxRecord mstxRecord;
        WaitFlagDevRecord waitFlagDevRecord;
        WaitFlagDevPipeRecord waitFlagDevPipeRecord;
        IntraBlockSyncRecord intraBlockSyncRecord;
        AtomicModeRecord atomicModeRecord;
        Set2DRecord set2DRecord;
        CmpMaskRecord cmpMaskRecord;
        SimtLoadStoreRecord simtLoadStoreRecord;
        SimtAtomRecord simtAtomRecord;
        DcPreloadRecord dcPreloadRecord;
        DmaMovNd2nzDavRecord dmaMovNd2nzDavRecord;
        KernelErrorRecord kernelErrorRecord;
        RedRecord redRecord;
        SetL12DRecord setL12DRecord;
        MovL1UBRecord movL1UbRecord;
        MmadA5Record mmadA5Record;
        MovL1BtRecord movL1BtRecord;
        MovL1FbRecord movL1FbRecord;
        Vbs32Record vbs32Record;
        Vms4v2RecordA5 vms4V2RecordA5;
        ShadowMemoryRecord shadowMemoryRecord;
        RegisterSetRecord registerSetRecord;
        SimtSyncRecord simtSyncRecord;
    } payload;
};

/// 内存分配信息来源，优先级为 MANUAL > EXTRA > ACL > RT > HAL
enum class MemInfoSrc : uint8_t {
    BYPASS = 0,     // 一些内存分配信息不参与优先级运算，使用此类型
    HAL,            // 使用来自底层驱动实际的内存分配信息，此信息通过劫持 hal 接口拿到
    RT,             // 使用来自 runtime 实际的内存分配信息，此信息通过劫持 runtime 接口拿到
    ACL,            // 使用来自 acl 实际的内存分配信息，此信息通过劫持 acl 接口拿到
    EXTRA,          // 使用框架上报的额外内存信息，包含各 Tensor 的地址和长度
    MANUAL,         // 使用用户通过 API 手动上报的内存信息对算子进行检测
    MSTX_HEAP,      // 使用MSTX HEAP拓展接口分配的内存信息
    MSTX_REGION,    // 使用MSTX REGION拓展接口分配的内存信息
};

/// 内存信息描述，主要用于描述当前内存是input/tiling/二级指针等
enum class MemInfoDesc : uint8_t {
    DEFAULT = 0,                // 默认值，暂时无用
    INPUT,                      // 算子输入
    TILING,                     // 算子tiling
    DOUBLE_PTR,                 // 二级指针
    HCCL_MC2_CONTEXT,           // 算子的mc2_context内存信息
    SECTION,                    // 算子.o中的section信息
    IPC_MEMORY,                 // IPCg共享内存
    OVERFLOW_ADDR,              // overflow地址信息
    PARA_BASE,                  // para_base地址信息
};

/// 内存操作发生的位置
enum class MemOpSide : uint8_t {
    HOST = 0, // 内存操作发生在 host 侧
    KERNEL    // 内存操作发生在 kernel 侧
};

/// Host 侧内存操作记录 Payload
struct HostMemRecord {
    MemOpType type;
    MemInfoSrc infoSrc;
    MemInfoDesc infoDesc;
    uint64_t srcAddr;
    uint64_t dstAddr;
    uint64_t memSize;
    uint64_t paramsNo; // 第几个入参
    uint64_t rootAddr; // 当前host侧的内存记录对应归属地址，用于关联mstx heap和region
};

/// 原始的内存检测记录结构体，为保证向后兼容保留
struct MemOpRecord {
    uint64_t serialNo;
    MemOpType type;
    int32_t coreId;
    int32_t moduleId;
    uint64_t srcAddr;
    uint64_t dstAddr;
    AddressSpace srcSpace;
    AddressSpace dstSpace;
    uint64_t memSize;
    int32_t lineNo;
    char fileName[64U]; // 后续优化为文件编码
    BlockType blockType;
    uint64_t pc;
    MemInfoSrc infoSrc;
    MemInfoDesc infoDesc;
    MemOpSide side;
    uint64_t paramsNo;
    uint64_t rootAddr; // 当前host侧的内存记录对应归属地址，用于关联mstx heap和region
    bool ignoreIllegalCheck;

    MemOpRecord() = default;

    explicit MemOpRecord(const HostMemRecord &record)
        : serialNo{0}, type{record.type}, coreId{}, moduleId{}, srcAddr{record.srcAddr}, dstAddr{record.dstAddr},
        srcSpace{}, dstSpace{}, memSize(record.memSize), lineNo{}, fileName{}, blockType{}, pc{},
        infoSrc{record.infoSrc}, infoDesc{record.infoDesc}, side{}, paramsNo{record.paramsNo},
        rootAddr{record.rootAddr}, ignoreIllegalCheck{}
    {}
};

enum class RecordVersion {
    MEMORY_RECORD = 0,
    KERNEL_RECORD,
    IPC_RECORD
};

enum class IPCOperationType : uint32_t {
    SET_INFO = 0,
    DESTROY_INFO,
    MAP_INFO,
    UNMAP_INFO
};

struct IPCMemorySetInfo {
    uint64_t addr;  // 共享内存在源设备上的地址
    uint64_t size;  // 共享内存的长度
    char name[64];  // 共享内存被设定的名称
};

struct IPCMemoryDestroyInfo {
    char name[64];  // 共享内存被设定的名称
};

struct IPCMemoryMapInfo {
    uint64_t addr;  // 共享内存在目的设备上打开后的虚拟地址
    char name[64];  // 共享内存被设定的名称
};

/** @brief 共享内存解除映射信息
 * 在 `rtIpcCloseMemory` 被调用后发送
 */
struct IPCMemoryUnmapInfo {
    uint64_t addr;  // 要解除的共享内存虚拟地址
};

struct IPCMemRecord {
    IPCOperationType type;
    union {
        IPCMemorySetInfo setInfo;
        IPCMemoryDestroyInfo destroyInfo;
        IPCMemoryMapInfo mapInfo;
        IPCMemoryUnmapInfo unmapInfo;
    };
};

// 目前检测需要用到的记录需要兼容三个版本，memoryRecord 对应原始的内存检测记录结构体，
// kernelRecord 对应新增的算子 kernel 函数内部的检测信息记录
// IpcRecord 对应共享内存接口的行为
struct SanitizerRecord {
    RecordVersion version;
    union {
        MemOpRecord memoryRecord;
        KernelRecord kernelRecord;
        IPCMemRecord ipcRecord;
    } payload;
};

}  // namespace Sanitizer

#endif  // CORE_FRAMEWORK_RECORD_DEFS_H
