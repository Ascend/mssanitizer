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


#ifndef ADDRESS_SANITIZER_MEM_ERROR_DEF_H
#define ADDRESS_SANITIZER_MEM_ERROR_DEF_H

#include <set>
#include <vector>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>
#include <functional>
#include <algorithm>
#include "core/framework/call_stack.h"
#include "core/framework/record_defs.h"
#include "core/framework/record_format.h"
#include "core/framework/utility/types.h"
#include "core/framework/utility/numeric.h"
#include "core/framework/runtime_context.h"
#include "core/framework/platform_config.h"

namespace Sanitizer {
enum class MemErrorType : uint8_t {
    // 针对GM，识别从UB，L0C搬移到GM的过程，如果发现存在不同core搬到同一个GM地址则告警
    // 运算类型：memcpy_blocks/store
    // 内存空间：UB2GM/L0C2GM
    OUT_OF_BOUNDS = 0U,

    // 针对L1和UB，识别单次搬运量超过变量的容量大小则告警
    // 运算类型：memcpy_block/store
    // 内存空间：L1/UB
    ILLEGAL_ADDR_WRITE,
    ILLEGAL_ADDR_READ,

    // 针对L1和UB，识别目的地址不对齐时告警
    // 运算类型：memcpy_block/store/load
    // 内存空间：L1/UB
    MISALIGNED_ACCESS,

    // 针对GM，识别到申请的内存最终未被释放则告警
    // 运算类型：malloc_device/free
    // 内存空间：GM
    MEM_LEAK,

    // 针对GM，识别到free非法地址或double free时告警
    // 运算类型：malloc_device/free
    // 内存空间：GM
    ILLEGAL_FREE,

    // 针对GM，识别到分配的内存未使用时告警
    // 运算类型：malloc_device/free
    // 内存空间：GM
    MEM_UNUSED,

    // 针对GM、UB、L1、L0{ABC}对应的栈内存，当读取未初始化内存时则告警
    // 运算类型：load/store
    // 内存空间：GM、UB、L1、L0{ABC}对应的栈内存
    UNINITIALIZED_READ,

    INTERNAL_ERROR,

    INVALID,
};

enum class MemAccessType : uint8_t {
    READ = 0U,
    WRITE,
};

struct Addr {uint64_t addr;};
struct ErrorMsg {
    MemErrorType type;
    bool isError;
    struct AuxData {
        AuxData() : badAddr{}, nBadBytes{0UL}, space{AddressSpace::INVALID},
                    moduleId(-1), fileName{}, lineNo{0UL}, coreId{0UL},
                    blockType{BlockType::AICUBE}, pc{0UL}, serialNo(0L),
                    side{MemOpSide::HOST}, threadLoc{}, conflictedThreadLoc{}, isSimtError{false} { }
        Addr badAddr;
        uint64_t nBadBytes;
        AddressSpace space;
        int32_t moduleId;
        std::string fileName;
        uint64_t lineNo;
        int32_t coreId;
        BlockType blockType;
        uint64_t pc;
        uint64_t serialNo;
        MemOpSide side;
        SimtThreadLocation threadLoc;
        SimtThreadLocation conflictedThreadLoc;
        bool isSimtError;
    } auxData;
    ErrorMsg() : type(MemErrorType::INVALID), isError(false) {}
    bool operator==(const ErrorMsg &rhs) const noexcept
    {
        if (!isError || !rhs.isError) {
            return isError == rhs.isError;
        }
        return
            isError == rhs.isError && type == rhs.type && auxData.badAddr.addr == rhs.auxData.badAddr.addr &&
            auxData.nBadBytes == rhs.auxData.nBadBytes && auxData.space == rhs.auxData.space &&
            auxData.moduleId == rhs.auxData.moduleId && auxData.fileName == rhs.auxData.fileName &&
            auxData.lineNo == rhs.auxData.lineNo && auxData.pc == rhs.auxData.pc &&
            auxData.isSimtError == rhs.auxData.isSimtError && auxData.threadLoc == rhs.auxData.threadLoc;
    }
    uint64_t operator()(const ErrorMsg &error) const
    {
        return
            std::hash<uint8_t>()(static_cast<uint8_t>(error.type)) ^
            std::hash<uint64_t>()(error.auxData.badAddr.addr) ^
            std::hash<uint64_t>()(error.auxData.nBadBytes) ^
            HashEnum(error.auxData.space) ^
            std::hash<int32_t>()(error.auxData.moduleId) ^
            std::hash<std::string>()(error.auxData.fileName) ^
            std::hash<uint64_t>()(error.auxData.lineNo) ^
            std::hash<uint64_t>()(error.auxData.pc) ^
            std::hash<uint64_t>()(error.auxData.isSimtError) ^
            std::hash<uint64_t>()(error.auxData.threadLoc.idX) ^
            std::hash<uint64_t>()(error.auxData.threadLoc.idY) ^
            std::hash<uint64_t>()(error.auxData.threadLoc.idZ);
    }
    bool operator()(const ErrorMsg &lhs, const ErrorMsg &rhs) const noexcept
    {
        return lhs == rhs;
    }
    void SetType(const MemErrorType &t, const AddressSpace &space, const uint64_t addr)
    {
        type = t;
        isError = true;
        auxData.space = space;
        auxData.badAddr.addr = addr;
        auxData.moduleId = -1;
    }
    void SetLocInfo(const std::string &fileName, const uint64_t lineNo, const uint64_t pc,
                    const int32_t coreId, const BlockType blockType)
    {
        auxData.fileName = fileName;
        auxData.lineNo = lineNo;
        auxData.pc = pc;
        auxData.coreId = coreId;
        auxData.blockType = blockType;
    }
};

inline std::ostream &operator << (std::ostream &os, const Addr &addr)
{
    std::ostringstream oss;
    oss << "0x" << std::hex << addr.addr;
    return os << oss.str();
}

using ErrorMsgList = std::vector<ErrorMsg>;

struct ReducedErrorMsg {
    ErrorMsg errorMsg;
    mutable std::set<uint64_t> aivBlocks;
    mutable std::set<uint64_t> aicBlocks;
    mutable std::set<uint64_t> aicoreBlocks;
    bool operator==(const ReducedErrorMsg &rhs) const noexcept
    {
        return errorMsg == rhs.errorMsg;
    }
    uint64_t operator()(const ReducedErrorMsg &error) const
    {
        return errorMsg(error.errorMsg);
    }
    bool operator()(const ReducedErrorMsg &lhs, const ReducedErrorMsg &rhs) const noexcept
    {
        return lhs.errorMsg == rhs.errorMsg;
    }
};

using BlockIdxList = TaggedType<std::vector<uint64_t>, struct BlockIdxListTag>;

inline std::ostream &operator<<(std::ostream &os, BlockIdxList const &blockIdxList)
{
    std::vector<uint64_t> list(blockIdxList.value);
    std::sort(list.begin(), list.end());

    if (list.size() == 0UL) {
        return os;
    }
    if (list.size() == 1UL) {
        return os << list[0UL];
    }

    // push first blockIdx
    std::size_t startIdx = 0UL;
    uint64_t start = list[0UL];
    std::size_t idx = 1UL;
    for (; idx < list.size(); ++idx) {
        if (idx - startIdx == static_cast<std::size_t>(list[idx] - start)) {
            continue;
        }
        if (idx == startIdx + 1) {
            os << start << ",";
        } else {
            os << start << "-" << (start + idx - startIdx - 1) << ",";
        }
        startIdx = idx;
        start = list[idx];
    }
    if (idx == startIdx + 1) {
        return os << start;
    } else {
        return os << start << "-" << (start + idx - startIdx - 1);
    }
}

struct FormatBlockInfo {
    ReducedErrorMsg msg;
    bool hasSubBlocks;
};

inline std::ostream &operator<<(std::ostream &os, FormatBlockInfo const &formatBlockInfo)
{
    ReducedErrorMsg const &msg = formatBlockInfo.msg;
    os <<  "======    ";
    if (msg.errorMsg.auxData.isSimtError) {
        const auto &threadLoc = msg.errorMsg.auxData.threadLoc;
        os <<  "by thread (" << threadLoc.idX << "," << threadLoc.idY << "," << threadLoc.idZ << ") ";
    }
    os <<  "in block ";
    if (formatBlockInfo.hasSubBlocks) {
        if (!msg.aivBlocks.empty()) {
            std::vector<uint64_t> aivBlockList(msg.aivBlocks.size());
            std::copy(msg.aivBlocks.cbegin(), msg.aivBlocks.cend(), aivBlockList.begin());
            os << "aiv(" << BlockIdxList{aivBlockList} << ")";
        }
        if (!msg.aivBlocks.empty() && !msg.aicBlocks.empty()) {
            os << ",";
        }
        if (!msg.aicBlocks.empty()) {
            std::vector<uint64_t> aicBlockList(msg.aicBlocks.size());
            std::copy(msg.aicBlocks.cbegin(), msg.aicBlocks.cend(), aicBlockList.begin());
            os << "aic(" << BlockIdxList{aicBlockList} << ")";
        }
    } else {
        if (!msg.aicoreBlocks.empty()) {
            std::vector<uint64_t> aicoreBlockList(msg.aicoreBlocks.size());
            std::copy(msg.aicoreBlocks.cbegin(), msg.aicoreBlocks.cend(), aicoreBlockList.begin());
            os << "aicore(" << BlockIdxList{aicoreBlockList} << ")";
        }
    }
    return os;
}

inline std::ostream &PrintClassicLocation(std::ostream &os, const ErrorMsg & msg)
{
    return os << "======    code in " << msg.auxData.fileName << ":" << msg.auxData.lineNo <<
                 " (serialNo:" << msg.auxData.serialNo << ")" << std::endl;
}

inline std::ostream &PrintLocationInfo(std::ostream &os, const ErrorMsg &msg)
{
    if (msg.auxData.pc == 0UL) {
        return PrintClassicLocation(os, msg);
    }

    CallStack::Stack stack = CallStack::Instance().Query(RuntimeContext::Instance().kernelSummary_.kernelName,
                                                         msg.auxData.pc);
    if (stack.empty()) {
        return PrintClassicLocation(os, msg);
    }

    os << "======    code in pc current 0x" << std::hex << msg.auxData.pc << std::dec <<
          " (serialNo:" << msg.auxData.serialNo << ")" << std::endl;

    return CallStack::Instance().FormatCallStack(os, stack);
}

struct FormatKernelName {
    ErrorMsg const &msg;
};

inline std::ostream &operator<<(std::ostream &os, FormatKernelName const &formatKernelName)
{
    // 内存异常可能发生在 host 侧也可能发生在 kernel 侧，只有发生在
    // kernel 侧时才打印 kernel name
    if (formatKernelName.msg.auxData.side == MemOpSide::KERNEL) {
        os << " in " << RuntimeContext::Instance().kernelNameDisplay;
    }
    return os;
}

inline std::ostream &PrintOutOfBounds(std::ostream &out, const ReducedErrorMsg &reducedMsg)
{
    ErrorMsg const &msg = reducedMsg.errorMsg;
    DeviceType deviceType = static_cast<DeviceType>(RuntimeContext::Instance().deviceSummary_.device);
    bool hasSubBlocks = HasSubBlocks(deviceType);
    out <<
        "====== WARNING: out of bounds of size " << msg.auxData.nBadBytes << std::endl <<
        "======    at " << msg.auxData.badAddr << " on " << msg.auxData.space << " when writing data" <<
        FormatKernelName{msg} << std::endl <<
        FormatBlockInfo{reducedMsg, hasSubBlocks} <<
        " on device "<< RuntimeContext::Instance().GetDeviceId() << std::endl;
    return PrintLocationInfo(out, msg);
}

inline std::ostream &PrintIllegalAddrWrite(std::ostream &out, const ReducedErrorMsg &reducedMsg)
{
    ErrorMsg const &msg = reducedMsg.errorMsg;
    DeviceType deviceType = static_cast<DeviceType>(RuntimeContext::Instance().deviceSummary_.device);
    bool hasSubBlocks = HasSubBlocks(deviceType);
    out <<
        "====== ERROR: illegal write of size " << msg.auxData.nBadBytes << std::endl <<
        "======    at " << msg.auxData.badAddr << " on " << msg.auxData.space <<
        FormatKernelName{msg} << std::endl <<
        FormatBlockInfo{reducedMsg, hasSubBlocks} <<
        " on device "<< RuntimeContext::Instance().GetDeviceId() << std::endl;
    return PrintLocationInfo(out, msg);
}

inline std::ostream &PrintIllegalAddrRead(std::ostream &out, const ReducedErrorMsg &reducedMsg)
{
    ErrorMsg const &msg = reducedMsg.errorMsg;
    DeviceType deviceType = static_cast<DeviceType>(RuntimeContext::Instance().deviceSummary_.device);
    bool hasSubBlocks = HasSubBlocks(deviceType);
    out <<
        "====== ERROR: illegal read of size " << msg.auxData.nBadBytes << std::endl <<
        "======    at " << msg.auxData.badAddr << " on " << msg.auxData.space <<
        FormatKernelName{msg} << std::endl <<
        FormatBlockInfo{reducedMsg, hasSubBlocks} <<
        " on device " << RuntimeContext::Instance().GetDeviceId() << std::endl;
    return PrintLocationInfo(out, msg);
}

inline std::ostream &PrintMisalignedAccess(std::ostream &out, const ReducedErrorMsg &reducedMsg)
{
    ErrorMsg const &msg = reducedMsg.errorMsg;
    DeviceType deviceType = static_cast<DeviceType>(RuntimeContext::Instance().deviceSummary_.device);
    bool hasSubBlocks = HasSubBlocks(deviceType);
    out <<
        "====== ERROR: misaligned access of size " << msg.auxData.nBadBytes << std::endl <<
        "======    at " << msg.auxData.badAddr << " on " << msg.auxData.space <<
        FormatKernelName{msg} << std::endl <<
        FormatBlockInfo{reducedMsg, hasSubBlocks} <<
        " on device " << RuntimeContext::Instance().GetDeviceId() << std::endl;
    return PrintLocationInfo(out, msg);
}

inline std::ostream &PrintIllegalFree(std::ostream &out, const ReducedErrorMsg &reducedMsg)
{
    ErrorMsg const &msg = reducedMsg.errorMsg;
    out <<
        "====== ERROR: illegal free() " << std::endl <<
        "======    at " << msg.auxData.badAddr << " on GM" << std::endl <<
        "======    code in " << msg.auxData.fileName << ":" << msg.auxData.lineNo <<
        " (serialNo:" << msg.auxData.serialNo << ")" << std::endl;
    return out;
}

inline std::ostream &PrintMemLeak(std::ostream &out, const ReducedErrorMsg &reducedMsg)
{
    ErrorMsg const &msg = reducedMsg.errorMsg;
    out <<
        "======    Direct leak of " << std::dec << msg.auxData.nBadBytes << " byte(s)" << std::endl <<
        "======      at " << msg.auxData.badAddr << " on GM";
    if (msg.auxData.moduleId != -1) {
        out << " by module " << msg.auxData.moduleId;
    }
    out << std::endl <<
        "======      allocated in " << msg.auxData.fileName << ":" << msg.auxData.lineNo <<
        " (serialNo:" << msg.auxData.serialNo << ")" << std::endl;
    return out;
}

inline std::ostream &PrintUnusedMem(std::ostream &out, const ReducedErrorMsg &reducedMsg)
{
    ErrorMsg const &msg = reducedMsg.errorMsg;
    out << "====== WARNING: Unused memory of " << msg.auxData.nBadBytes << " byte(s)" << std::endl <<
        "======    at " << msg.auxData.badAddr << " on GM" << std::endl <<
        "======    code in " << msg.auxData.fileName << ":" << msg.auxData.lineNo <<
        " (serialNo:" << msg.auxData.serialNo << ")" << std::endl;
    return out;
}

inline std::ostream &PrintUninitializedRead(std::ostream &out, const ReducedErrorMsg &reducedMsg)
{
    ErrorMsg const &msg = reducedMsg.errorMsg;
    DeviceType deviceType = static_cast<DeviceType>(RuntimeContext::Instance().deviceSummary_.device);
    bool hasSubBlocks = HasSubBlocks(deviceType);
    out <<
        "====== ERROR: uninitialized read of size " << msg.auxData.nBadBytes << std::endl <<
        "======    at " << msg.auxData.badAddr << " on " << msg.auxData.space <<
        FormatKernelName{msg} << std::endl <<
        FormatBlockInfo{reducedMsg, hasSubBlocks} <<
        " on device " << RuntimeContext::Instance().GetDeviceId() << std::endl;
    return PrintLocationInfo(out, msg);
}

inline std::ostream &PrintInternalError(std::ostream &out, const ReducedErrorMsg &reducedMsg)
{
    ErrorMsg const &msg = reducedMsg.errorMsg;
    out << "====== ERROR: internal errors (serialNo:" << msg.auxData.serialNo << ")" << std::endl;
    return PrintLocationInfo(out, msg);
}

inline std::ostream &operator<<(std::ostream &out, const ReducedErrorMsg &reducedMsg)
{
    using PrintFunc = std::function<std::ostream &(std::ostream &, const ReducedErrorMsg &)>;
    static const std::map<MemErrorType, PrintFunc> PRINT_FUNC_MAP = {
        {MemErrorType::OUT_OF_BOUNDS, PrintOutOfBounds},
        {MemErrorType::ILLEGAL_ADDR_WRITE, PrintIllegalAddrWrite},
        {MemErrorType::ILLEGAL_ADDR_READ, PrintIllegalAddrRead},
        {MemErrorType::MISALIGNED_ACCESS, PrintMisalignedAccess},
        {MemErrorType::ILLEGAL_FREE, PrintIllegalFree},
        {MemErrorType::MEM_LEAK, PrintMemLeak},
        {MemErrorType::MEM_UNUSED, PrintUnusedMem},
        {MemErrorType::UNINITIALIZED_READ, PrintUninitializedRead},
        {MemErrorType::INTERNAL_ERROR, PrintInternalError},
    };
    auto it = PRINT_FUNC_MAP.find(reducedMsg.errorMsg.type);
    if (it != PRINT_FUNC_MAP.end()) {
        return it->second(out, reducedMsg);
    }
    return out;
}
}

#endif
