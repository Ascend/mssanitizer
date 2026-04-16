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


#ifndef CORE_FRAMEWORK_FORMAT_CONVERTER_H
#define CORE_FRAMEWORK_FORMAT_CONVERTER_H

#include <string>

#include "record_defs.h"
#include "event_def.h"

namespace Sanitizer {
class FormatConverter {
public:
    static PipeType QueryPipeType(MemType src, MemType dst);
    static AddressSpace MemTypeToAddrSpace(MemType memType);
    static MemOpType AccessTypeToMemOpType(AccessType type);
    static uint64_t GetDataSizeByType(DataType dataType);
    static bool GetDataBitSizeByType(DataType dataType, uint16_t &dataBitSize);
    static bool GetDataTypeByDataBits(uint8_t dataBits, DataType &dataType);
    static uint16_t GetAlignSize(
        MemOpInfo &memInfo, AddressSpace space, RecordType recordType, DeviceType deviceType);
    static uint64_t GetPadModeScaleValue(PadMode padMode);
    static bool CheckChannelSize(DataType dataType, uint32_t channel);
    static bool GetAtomicFlag(KernelRecord const &record, bool &enabled);
};

inline std::string FormatAddressSpace(AddressSpace space)
{
    switch (space) {
        case AddressSpace::PRIVATE:
            return "PRIVATE";
        case AddressSpace::GM:
            return "GM";
        case AddressSpace::L1:
            return "L1";
        case AddressSpace::L0A:
            return "L0A";
        case AddressSpace::L0B:
            return "L0B";
        case AddressSpace::L0C:
            return "L0C";
        case AddressSpace::UB:
            return "UB";
        case AddressSpace::BT:
            return "BT";
        case AddressSpace::FB:
            return "FB";
        default:
            return "INVALID";
    }
}

}
#endif
