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

#ifndef CORE_FRAMEWORK_RECORD_FORMAT_H
#define CORE_FRAMEWORK_RECORD_FORMAT_H

/* 此头文件中定义所有与 record 相关的格式化输出方法，用于 host 侧打印记录或调试
 */

#include <map>
#include <iostream>
#include <cstdint>
#include <sstream>

#include "record_defs.h"
#include "sanitizer_report.h"

namespace Sanitizer {

/* 此模板用于判断一个枚举类型的底层类型是否为 uint8_t。
 * 在 gcc 中 char 类型与 uint8_t 为相同类型，这就导致如果把一个底层为 uint8_t 类型的枚举值转换
 * 为底层类型，此处通过 << 会被格式化为字符，不符合打印出值的初衷。因此实现此模板用于识别底层
 * 类型是否为 uint8_t
 */
template<typename EnumType>
struct UnderlyingUint8 {
    static constexpr bool VALUE = std::is_same<typename std::underlying_type<EnumType>::type, uint8_t>::value;
};

/* 此模板以及下面的特化用于将一个枚举类型的转化为可打印的数值类型。
 * 分两种情况处理：
 * 1. 枚举底层类型为 uint8_t 时返回 uint32_t 用于格式化
 * 2. 其他情况返回枚举的底层类型
 */
template<typename EnumType, typename Enable = void>
struct NumericFormat {
    using type = typename std::underlying_type<EnumType>::type;
};

template<typename EnumType>
struct NumericFormat<EnumType, typename std::enable_if<UnderlyingUint8<EnumType>::VALUE>::type> {
    using type = uint32_t;
};

/* 根据映射表将枚举映射到格式化字符串
 * 1. 当枚举值存在于映射表中时，格式化为枚举对应的字符串
 * 2. 否则格式化为 Unknown Enum(val) 作为提示
 */
template<typename EnumType>
inline std::ostream &FormatEnum(std::ostream &os, std::map<EnumType, std::string> const &enumMap,
                                EnumType val, std::string const &enumStr = "Enum")
{
    auto it = enumMap.find(val);
    if (it == enumMap.end()) {
        using Underlying = typename NumericFormat<EnumType>::type;
        return os << "Unknown " << enumStr << "(" << static_cast<Underlying>(val) << ")";
    } else {
        return os << it->second;
    }
}

inline std::string ConvertVecMaskValue(const uint64_t &mask)
{
    if (mask == VEC_MASK_MAX_VALUE) { return "-1"; }
    std::stringstream ss;
    ss << std::hex << mask << std::dec;
    return "0x" + ss.str();
}

template<typename T>
inline std::string ToString(T const &t)
{
    std::ostringstream oss;
    oss << t;
    return oss.str();
}

std::ostream &operator<<(std::ostream &os, RecordType recordType);
std::ostream &operator<<(std::ostream &os, DataType dataType);
std::ostream &operator<<(std::ostream &os, PadMode padMode);
std::ostream &operator<<(std::ostream &os, MemType memType);
std::ostream &operator<<(std::ostream &os, PipeType pipeType);
std::ostream &operator<<(std::ostream &os, EventID evenID);
std::ostream &operator<<(std::ostream &os, ByteMode byteMode);
std::ostream &operator<<(std::ostream &os, MemOpType memOpType);
std::ostream &operator<<(std::ostream &os, AddressSpace space);
std::ostream &operator<<(std::ostream &os, BlockType blockType);
std::ostream &operator<<(std::ostream &os, AccessType accessType);
std::ostream &operator<<(std::ostream &os, MaskMode maskMode);
std::ostream &operator<<(std::ostream &os, MemInfoSrc memInfoSrc);
std::ostream &operator<<(std::ostream &os, CompareOp cmpOp);
std::ostream &operator<<(std::ostream &os, DeviceInfoSummary const &summary);
std::ostream &operator<<(std::ostream &os, Location const &location);
std::ostream &operator<<(std::ostream &os, VectorMask const &vectorMask);
std::ostream &operator<<(std::ostream &os, CompareMask const &compareMask);
std::ostream &operator<<(std::ostream &os, VaRegister const &vaRegister);
std::ostream &operator<<(std::ostream &os, LoadStoreRecord const &record);
std::ostream &operator<<(std::ostream &os, DmaMovRecord const &record);
std::ostream &operator<<(std::ostream &os, DmaMovConvReluRecord const &record);
std::ostream &operator<<(std::ostream &os, DmaMovNd2nzRecord const &record);
std::ostream &operator<<(std::ostream &os, MovAlignRecord const &record);
std::ostream &operator<<(std::ostream &os, MovAlignRecordV2 const &record);
std::ostream &operator<<(std::ostream &os, NdDMAOut2UbRecord const &record);
std::ostream &operator<<(std::ostream &os, MovBtRecord const &record);
std::ostream &operator<<(std::ostream &os, MovFpRecord const &record);
std::ostream &operator<<(std::ostream &os, VecDupRecord const &record);
std::ostream &operator<<(std::ostream &os, UnaryOpRecord const &record);
std::ostream &operator<<(std::ostream &os, VgatherRecord const &record);
std::ostream &operator<<(std::ostream &os, ElementRecord const &record);
std::ostream &operator<<(std::ostream &os, BinaryOpRecord const &record);
std::ostream &operator<<(std::ostream &os, MarixMulOpRecord const &record);
std::ostream &operator<<(std::ostream &os, ReduceOpRecord const &record);
std::ostream &operator<<(std::ostream &os, ReduceV2Record const &record);
std::ostream &operator<<(std::ostream &os, SyncRecord const &record);
std::ostream &operator<<(std::ostream &os, HardSyncRecord const &record);
std::ostream &operator<<(std::ostream &os, PipeBarrierRecord const &record);
std::ostream &operator<<(std::ostream &os, FftsSyncRecord const &record);
std::ostream &operator<<(std::ostream &os, WaitFlagDevRecord const &record);
std::ostream &operator<<(std::ostream &os, WaitFlagDevPipeRecord const &record);
std::ostream &operator<<(std::ostream &os, IntraBlockSyncRecord const &record);
std::ostream &operator<<(std::ostream &os, KernelRecord const &record);
std::ostream &operator<<(std::ostream &os, MemOpRecord const &record);
std::ostream &operator<<(std::ostream &os, SanitizerRecord const &record);
std::ostream &operator<<(std::ostream &os, IPCOperationType const &type);
std::ostream &operator<<(std::ostream &os, IPCMemRecord const &record);
std::ostream &operator<<(std::ostream &os, Load2DRecord const &record);
std::ostream &operator<<(std::ostream &os, LoadL12DRecord const &record);
std::ostream &operator<<(std::ostream &os, LoadL1Mx2DRecord const &record);
std::ostream &operator<<(std::ostream &os, LoadL12DTransposeRecord const &record);
std::ostream &operator<<(std::ostream &os, Load2DSparseRecord const &record);
std::ostream &operator<<(std::ostream &os, Load2DTransposeRecord const &record);
std::ostream &operator<<(std::ostream &os, DecompressHeaderRecord const &record);
std::ostream &operator<<(std::ostream &os, BroadcastRecord const &record);
std::ostream &operator<<(std::ostream &os, DcPreloadRecord const &record);
std::ostream &operator<<(std::ostream &os, LoadB2Record const &record);
std::ostream &operator<<(std::ostream &os, LoadAWinogradRecord const &record);
std::ostream &operator<<(std::ostream &os, LoadBWinogradRecord const &record);
std::ostream &operator<<(std::ostream &os, Load3DRecord const &record);
std::ostream &operator<<(std::ostream &os, AtomicModeRecord const &record);
std::ostream &operator<<(std::ostream &os, Set2DRecord const &record);
std::ostream &operator<<(std::ostream &os, LoadImageRecord const &record);
std::ostream &operator<<(std::ostream &os, LoadSmaskRecord const &record);
std::ostream &operator<<(std::ostream &os, VecRegPropCoordOpRecord const &record);
std::ostream &operator<<(std::ostream &os, SoftSyncRecord const& record);
std::ostream &operator<<(std::ostream &os, BufRecord const& record);
std::ostream &operator<<(std::ostream &os, CmpMaskRecord const &record);
std::ostream &operator<<(std::ostream &os, MstxCrossCoreBarrier const &record);
std::ostream &operator<<(std::ostream &os, MstxCrossCoreSetFlag const &record);
std::ostream &operator<<(std::ostream &os, MstxCrossCoreWaitFlag const &record);
std::ostream &operator<<(std::ostream &os, MstxSignalSet const &record);
std::ostream &operator<<(std::ostream &os, MstxSignalWait const &record);
std::ostream &operator<<(std::ostream &os, MstxCrossNpuBarrier const &record);
std::ostream &operator<<(std::ostream &os, MstxTensorDesc const &tensor);
std::ostream &operator<<(std::ostream &os, MstxVecWrapper const &record);
std::ostream &operator<<(std::ostream &os, MstxVecUnaryDesc const &record);
std::ostream &operator<<(std::ostream &os, MstxVecBinaryDesc const &record);
std::ostream &operator<<(std::ostream &os, MstxDataCopyDesc const &record);
std::ostream &operator<<(std::ostream &os, MstxDataCopyPadDesc const &record);
std::ostream &operator<<(std::ostream &os, MstxRecord const &record);
std::ostream &operator<<(std::ostream &os, ScatterVnchwconvRecord const &record);
std::ostream &operator<<(std::ostream &os, SimtLoadStoreRecord const &record);
std::ostream &operator<<(std::ostream &os, SimtAtomMode mode);
std::ostream &operator<<(std::ostream &os, SimtAtomRecord const &record);
std::ostream &operator<<(std::ostream &os, DmaMovNd2nzDavRecord const &record);
std::ostream &operator<<(std::ostream &os, SimtThreadLocation const &threadLoc);
std::ostream &operator<<(std::ostream &os, RedRecord const &record);
std::ostream &operator<<(std::ostream &os, SetL12DRecord const &record);
std::ostream &operator<<(std::ostream &os, MovL1UBRecord const &record);
std::ostream &operator<<(std::ostream &os, Vbs32Record const &record);
std::ostream &operator<<(std::ostream &os, Vms4v2RecordA5 const &record);
std::ostream &operator<<(std::ostream &os, MmadA5Record const &record);
std::ostream &operator<<(std::ostream &os, DetailedDataType type);
std::ostream &operator<<(std::ostream &os, MovL1BtRecord const &record);
std::ostream &operator<<(std::ostream &os, MovL1FbRecord const &record);
std::ostream &operator<<(std::ostream &os, ShadowMemoryRecord const &record);
std::ostream &operator<<(std::ostream &os, RegisterSetRecord const &record);
std::ostream &operator<<(std::ostream &os, SimtSyncRecord const &record);
}  // namespace Sanitizer

#endif  // CORE_FRAMEWORK_RECORD_FORMAT_H
