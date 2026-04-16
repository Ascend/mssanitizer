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


#include "format_converter.h"

#include <map>
#include <algorithm>

#include "platform_config.h"
#include "constant.h"

namespace Sanitizer {
// 根据搬运的源内存类型查询PIPE类型
PipeType FormatConverter::QueryPipeType(MemType src, MemType dst)
{
    static const std::map<std::pair<MemType, MemType>, PipeType> MAPS = {
        {{MemType::L1, MemType::L0A}, PipeType::PIPE_MTE1},
        {{MemType::L1, MemType::L0B}, PipeType::PIPE_MTE1},
        {{MemType::L1, MemType::L0C}, PipeType::PIPE_MTE1},
        {{MemType::L1, MemType::UB}, PipeType::PIPE_MTE1},
        {{MemType::L1, MemType::BT}, PipeType::PIPE_MTE1},
        {{MemType::L1, MemType::GM}, PipeType::PIPE_MTE3},
        {{MemType::GM, MemType::L0A}, PipeType::PIPE_MTE2},
        {{MemType::GM, MemType::L0B}, PipeType::PIPE_MTE2},
        {{MemType::GM, MemType::L1}, PipeType::PIPE_MTE2},
        {{MemType::GM, MemType::UB}, PipeType::PIPE_MTE2},
        {{MemType::UB, MemType::GM}, PipeType::PIPE_MTE3},
        {{MemType::UB, MemType::L1}, PipeType::PIPE_MTE3},
        {{MemType::UB, MemType::L0C}, PipeType::PIPE_MTE3},
        {{MemType::L0C, MemType::UB}, PipeType::PIPE_V},
        {{MemType::UB, MemType::UB}, PipeType::PIPE_V},
        {{MemType::L0C, MemType::L1}, PipeType::PIPE_FIX},
        {{MemType::L1, MemType::FB}, PipeType::PIPE_FIX},
    };

    auto iter = MAPS.find({ src, dst });
    return iter == MAPS.end() ? PipeType::SIZE : iter->second;
}

AddressSpace FormatConverter::MemTypeToAddrSpace(MemType memType)
{
    static const std::map<MemType, AddressSpace> MEM_TYPE_TO_ADDR_SPACE_MAP = {
        {MemType::L1,      AddressSpace::L1},
        {MemType::L0A,     AddressSpace::L0A},
        {MemType::L0B,     AddressSpace::L0B},
        {MemType::L0C,     AddressSpace::L0C},
        {MemType::UB,      AddressSpace::UB},
        {MemType::BT,      AddressSpace::BT},
        {MemType::FB,      AddressSpace::FB},
        {MemType::GM,      AddressSpace::GM},
        {MemType::INVALID, AddressSpace::INVALID},
        {MemType::PRIVATE, AddressSpace::PRIVATE},
    };
    auto it = MEM_TYPE_TO_ADDR_SPACE_MAP.find(memType);
    return it == MEM_TYPE_TO_ADDR_SPACE_MAP.cend() ? AddressSpace::INVALID : it->second;
}

MemOpType FormatConverter::AccessTypeToMemOpType(AccessType type)
{
    static const std::map<AccessType, MemOpType> ACCESS_TYPE_TO_MEMOP_TYPE_MAP = {
        {AccessType::READ,      MemOpType::LOAD},
        {AccessType::WRITE,     MemOpType::STORE},
        {AccessType::MEMCPY_BLOCKS,     MemOpType::MEMCPY_BLOCKS},
    };
    auto it = ACCESS_TYPE_TO_MEMOP_TYPE_MAP.find(type);
    return it == ACCESS_TYPE_TO_MEMOP_TYPE_MAP.cend() ? MemOpType::INVALID : it->second;
}

uint64_t FormatConverter::GetDataSizeByType(DataType dataType)
{
    static const std::map<DataType, uint64_t> DATA_TYPE_SIZE_MAP = {
        {DataType::DATA_B8,  1U},
        {DataType::DATA_B16, 2U},
        {DataType::DATA_B32, 4U},
    };
    auto it = DATA_TYPE_SIZE_MAP.find(dataType);
    return it == DATA_TYPE_SIZE_MAP.cend() ? 1U : it->second;
}

bool FormatConverter::GetDataBitSizeByType(DataType dataType, uint16_t &dataBitSize)
{
    static const std::map<DataType, uint16_t> DATA_TYPE_BIT_SIZE_MAP = {
        {DataType::DATA_B4,  4U},
        {DataType::DATA_B8,  8U},
        {DataType::DATA_B16, 16U},
        {DataType::DATA_B32, 32U},
    };
    auto it = DATA_TYPE_BIT_SIZE_MAP.find(dataType);
    if (it != DATA_TYPE_BIT_SIZE_MAP.cend()) {
        dataBitSize = it->second;
        return true;
    }
    return false;
}

bool FormatConverter::GetDataTypeByDataBits(uint8_t dataBits, DataType &dataType)
{
    static const std::map<uint8_t, DataType> BIT_SIZE_DATA_TYPE_MAP = {
        {4U,  DataType::DATA_B4},
        {8U,  DataType::DATA_B8},
        {16U, DataType::DATA_B16},
        {32U, DataType::DATA_B32},
    };
    auto it = BIT_SIZE_DATA_TYPE_MAP.find(dataBits);
    if (it != BIT_SIZE_DATA_TYPE_MAP.cend()) {
        dataType = it->second;
        return true;
    }
    return false;
}

uint16_t FindAlignMap(const std::string& key)
{
    auto it = ALIGN_MAP.find(key);
    if (it == ALIGN_MAP.cend()) {
        std::cout << "[mssanitizer] unsupported ALIGN_MAP key: " << key << std::endl;
    } else {
        return it->second;
    }
    return 1;
}

void AlignRecordBySpace(uint16_t& alignSize, AddressSpace space)
{
    switch (space) {
        case AddressSpace::UB:
            alignSize = FindAlignMap("UB_NORMAL_ALIGN_SIZE");
            break;
        case AddressSpace::L0A:
        case AddressSpace::L0B:
        case AddressSpace::L0C:
            alignSize = FindAlignMap("L0A_L0B_L0C_NORMAL_ALIGN_SIZE");
            break;
        case AddressSpace::L1:
            alignSize = FindAlignMap("L1_NORMAL_ALIGN_SIZE");
            break;
        case AddressSpace::GM:
        case AddressSpace::BT: // 为避免影响其他与之相关的指令对齐逻辑，先暂设为1，后续再适配，FB同理
        case AddressSpace::FB:
        case AddressSpace::PRIVATE:
        case AddressSpace::INVALID:
            alignSize = 1;
            break;
        default:
            ;
    }
}

// 310P芯片支持UB上X adddress mode的数据宽度对齐
bool IsElementAlign(uint8_t dataBits, uint16_t &alignSize, RecordType recordType, DeviceType deviceType)
{
    if (deviceType != DeviceType::ASCEND_310P ||
        recordType <= RecordType::SIMD_START || recordType >= RecordType::SIMD_END) {
        return false;
    }
    uint8_t elementWidth = dataBits / BITS_EACH_BYTE;
    alignSize = elementWidth;
    return true;
}

//  如果是特殊的对齐规则则通过alignSize出参获取
bool IsSpecialAlign(MemOpInfo &memInfo, uint16_t &alignSize, RecordType recordType = RecordType::FINISH)
{
    if (recordType == RecordType::VMRGSORT4_OP_M200) {
        // VMRGSORT4_OP指令，在310P芯片取决于数据类型，float类型为32字节，half类型为16字节；
        alignSize = memInfo.dataBits;
    } else if (recordType == RecordType::VMRGSORT4_OP_C220 || recordType == RecordType::VMRGSORT4_OP_C310) {
        // VMRGSORT4_OP指令，在910B/950芯片上读和写不同，读要求8B对齐，写要求32B对齐
        alignSize = memInfo.opType == AccessType::READ ? 8U : 32U;
    } else if (recordType == RecordType::LOAD || recordType == RecordType::STORE ||
               recordType == RecordType::CMPMASK_OP) {
        //  CMPMASK指令，910B和310P芯片都采取16字节对齐
        alignSize = memInfo.blockSize;
    } else if (recordType == RecordType::DECOMPRESS_HEADER) {
        constexpr uint16_t decompressHeaderSrcAddrAlignSize = 32U;
        alignSize = decompressHeaderSrcAddrAlignSize;
    } else if (recordType == RecordType::DMA_MOV_CONV_RELU &&
               memInfo.memType == MemType::L0C && memInfo.blockSize == 64U) {
        constexpr uint64_t dmaMovConvReluAlignSize1024B = 1024U;
        // DMA_MOV指令，310P芯片上L0C32V采取1024字节对齐
        alignSize = dmaMovConvReluAlignSize1024B;
    } else if (recordType == RecordType::LOAD_SMASK && memInfo.memType == MemType::GM) {
        //  LOAD_SMASK指令，310P芯片在GM上要求2B对齐，在UB上要求32B对齐
        constexpr uint16_t loadMaskSrcAddrAlignSize = 2U;
        alignSize = loadMaskSrcAddrAlignSize;
    } else if (memInfo.memType == MemType::L0C && memInfo.blockSize == 1024U &&
               (recordType == RecordType::BROADCAST || recordType == RecordType::DMA_MOV_DEPTH_WISE)) {
        constexpr uint16_t alignSizeFor1024B = 1024;
        alignSize = alignSizeFor1024B;
    } else {
        return false;
    }
    return true;
}

/// 处理逻辑为先按照特殊的recordType去对齐；没有特殊recordType时，按照AddressSpace去对齐
uint16_t FormatConverter::GetAlignSize(
    MemOpInfo &memInfo, AddressSpace space, RecordType recordType, DeviceType deviceType)
{
    uint16_t alignSize = 1;
    if (IsElementAlign(memInfo.dataBits, alignSize, recordType, deviceType)) {
        return alignSize;
    }
    if (!IsSpecialAlign(memInfo, alignSize, recordType)) {
        AlignRecordBySpace(alignSize, space);
    }
    return alignSize;
}

uint64_t FormatConverter::GetPadModeScaleValue(PadMode padMode)
{
    // PAD_MODE1-5 是放大系数， 6-8是缩小系数，作用于DMA_MOV
    static const std::map<PadMode, uint64_t> SCALE_MAP = {
        {PadMode::PAD_MODE1, 32},
        {PadMode::PAD_MODE2, 16},
        {PadMode::PAD_MODE3, 8},
        {PadMode::PAD_MODE4, 4},
        {PadMode::PAD_MODE5, 2},
        {PadMode::PAD_MODE6, 8},
        {PadMode::PAD_MODE7, 4},
        {PadMode::PAD_MODE8, 2},
    };
    auto it = SCALE_MAP.find(padMode);
    return it == SCALE_MAP.cend() ? 1 : it->second;
}

bool FormatConverter::CheckChannelSize(DataType dataType, uint32_t channel)
{
    // DataType::DATA_B32：channel = 0, 4, N*8, N*8 + 4
    static const std::map<DataType, std::pair<uint32_t, std::vector<int>>> CHANNEL_LEGAL_MAP = {
        {DataType::DATA_B4,  {64, {0, 8, 16, 32}}},
        {DataType::DATA_B8,  {32, {0, 4, 8, 16}}},
        {DataType::DATA_B16, {16, {0, 4, 8}}},
        {DataType::DATA_B32, {8,  {0, 4}}},
    };
    auto it = CHANNEL_LEGAL_MAP.find(dataType);
    if (it == CHANNEL_LEGAL_MAP.cend()) {
        return false;
    }
    uint32_t divisor = it->second.first;
    const auto& remainderVec = it->second.second;
    uint32_t remainder = channel % divisor;
    return std::find(remainderVec.cbegin(), remainderVec.cend(), remainder) != remainderVec.cend();
}

bool FormatConverter::GetAtomicFlag(KernelRecord const &record, bool &enabled)
{
    if (record.recordType != RecordType::SET_ATOMIC) {
        return false;
    }

    AtomicModeRecord const &atomicModeRecord = record.payload.atomicModeRecord;
    switch (atomicModeRecord.mode) {
        // 设置模式不影响原子状态
        case AtomicMode::SUM:
        case AtomicMode::MIN:
        case AtomicMode::MAX:
            return false;
        case AtomicMode::F32:
        case AtomicMode::F16:
        case AtomicMode::S16:
        case AtomicMode::S32:
        case AtomicMode::S8:
        case AtomicMode::BF16:
            enabled = true;
            return true;
        case AtomicMode::NONE:
            enabled = false;
            return true;
    }
    return false;
}
}
