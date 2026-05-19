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

#ifndef PLUGIN_PARSE_RECORD_H
#define PLUGIN_PARSE_RECORD_H

#include "kernel_pub_func.h"

namespace Sanitizer {

struct AddrInfo {
    Location location;
    SimtThreadLocation threadLoc;
    uint64_t addr;
    uint64_t size;
    AddressSpace space;
    AccessType opType;
    uint32_t alignSize;
    SimtAtomMode option;
};

template<RecordType recordType>
AICORE_FUNC_HEAD AddrInfo ParseRecord(SimtLoadStoreRecord const &record)
{
    static_assert(recordType == RecordType::SIMT_LDG || recordType == RecordType::SIMT_STG
    || recordType == RecordType::SIMT_LDS || recordType == RecordType::SIMT_STS
    || recordType == RecordType::SIMT_LDK || recordType == RecordType::SIMT_STK
    || recordType == RecordType::SIMT_LD || recordType == RecordType::SIMT_ST
    || recordType == RecordType::SIMT_RED,
        "unsupport recordType in Parse SimtLoadStoreRecord");
    AddrInfo addrInfo{};
    addrInfo.location = record.location;
    addrInfo.threadLoc = record.threadLoc;
    addrInfo.addr = record.addr;
    addrInfo.size = record.size;
    addrInfo.space = record.space;
    if (recordType == RecordType::SIMT_LDG || recordType == RecordType::SIMT_LDS || recordType == RecordType::SIMT_LDK || recordType == RecordType::SIMT_LD) {
        addrInfo.opType = AccessType::READ;
    } else if (recordType == RecordType::SIMT_RED) {
        addrInfo.opType = AccessType::MEMCPY_BLOCKS;
    } else {
        addrInfo.opType = AccessType::WRITE;
    }
    addrInfo.alignSize = GetAlignSizeByDataType(record.detailedDataType);
    return addrInfo;
}

template<RecordType recordType>
AICORE_FUNC_HEAD AddrInfo ParseRecord(SimtAtomRecord const &record)
{
    static_assert(recordType == RecordType::SIMT_ATOM,
        "unsupport recordType in Parse SimtAtomRecord");
    AddrInfo addrInfo{};
    addrInfo.location = record.location;
    addrInfo.threadLoc = record.threadLoc;
    addrInfo.addr = record.addr;
    addrInfo.size = record.size;
    addrInfo.space = record.space;
    addrInfo.opType = AccessType::MEMCPY_BLOCKS;
    addrInfo.option = record.option;
    addrInfo.alignSize = GetAlignSizeByDataType(record.detailedDataType);
    return addrInfo;
}

template<RecordType recordType>
AICORE_FUNC_HEAD AddrInfo ParseRecord(SimtEmptyRecord const &record)
{
    AddrInfo addrInfo{};
    addrInfo.location = record.location;
    addrInfo.threadLoc = record.threadLoc;
    return addrInfo;
}

}  // namespace Sanitizer

#endif // PLUGIN_PARSE_RECORD_H
