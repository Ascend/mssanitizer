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


#ifndef RACE_SANITIZER_RACE_INFO_DISPLAY_H
#define RACE_SANITIZER_RACE_INFO_DISPLAY_H

#include <vector>
#include <memory>
#include <iostream>
#include "core/framework/event_def.h"
#include "core/framework/call_stack.h"
#include "core/framework/kernel_manager.h"
#include "core/framework/record_defs.h"
#include "core/framework/record_format.h"
#include "core/framework/file_mapping.h"
#include "core/framework/runtime_context.h"

namespace Sanitizer {

inline std::ostream &PrintClassicLocation(std::ostream &os, uint64_t fileNo, uint64_t lineNo,
    uint64_t serialNo)
{
    return os << "at " << FileMapping::Instance().Query(fileNo).fileName << ":" << lineNo <<
                 " (serialNo:" << serialNo << ")" << std::endl;
}

inline std::ostream &PrintLocationInfo(std::ostream &os, BaseEvent const & event,
    uint64_t serialNo)
{
    if (event.pc == 0UL) {
        return PrintClassicLocation(os, event.fileNo, event.lineNo, serialNo);
    }

    CallStack::Stack stack;
    KernelSummary kernelSummary{};
    if (KernelManager::Instance().Get(event.deviceId, event.kernelIdx, kernelSummary)) {
        stack = CallStack::Instance().Query(kernelSummary.kernelName, event.pc);
    }
    if (stack.empty()) {
        return PrintClassicLocation(os, event.fileNo, event.lineNo, serialNo);
    }

    os << "at pc current 0x" << std::hex << event.pc << std::dec;
    if (!event.isSimt) {
        os << " (serialNo:" << serialNo << ")";
    };
    os << std::endl;

    return CallStack::Instance().FormatCallStack(os, stack);
}

struct RaceFormatKernelName {
    uint32_t deviceId;
    uint32_t kernelIdx;
};

inline std::ostream &operator<<(std::ostream &os, RaceFormatKernelName const &formatKernelName)
{
    KernelSummary kernelSummary{};
    std::string kernelName = "unknown";
    if (KernelManager::Instance().Get(formatKernelName.deviceId, formatKernelName.kernelIdx, kernelSummary)) {
        kernelName = KernelManager::Instance().GetDisplayKernelName(kernelSummary.kernelName);
    }
    return os << " in " << kernelName;
}

inline std::ostream &operator<<(std::ostream &os, SimtThreadLocation const &threadLoc)
{
    os << " Thread (" << threadLoc.idX << "," <<threadLoc.idY << "," << threadLoc.idZ << ")";
    return os;
}

inline void FormatEvent(std::ostream &os, const BaseEvent &event, std::string const &accessType,
    std::string const &errType)
{
    os << "======    ";
    if (!event.isSimt) {
        os << static_cast<PipeType>(event.pipeType);
    }
    os << accessType;
    if (event.isSimt) {
        os << event.threadLoc;
    }
    os << " at " << errType << "()+0x" << std::hex << event.addr << std::dec << " in block " << event.coreId
       << " (" << event.blockType << ")" << " on device " << event.deviceId << " ";
    PrintLocationInfo(os, event, event.serialNo);
}

inline std::ostream &operator << (std::ostream &os, RaceDispInfo const &raceInfo)
{
    BaseEvent raceEvent1{};
    BaseEvent raceEvent2{};
    if (raceInfo.p1.isSimt && raceInfo.p2.isSimt) {
        // 如果都为simt错误，则默认竞争第一行显示小线程
        raceEvent1 = raceInfo.p1.threadLoc < raceInfo.p2.threadLoc ? raceInfo.p1 : raceInfo.p2;
        raceEvent2 = raceInfo.p1.threadLoc < raceInfo.p2.threadLoc ? raceInfo.p2 : raceInfo.p1;
    } else {
        // 按照指令序号serialNo对事件p1和p2排序，让p1先于p2
        raceEvent1 = raceInfo.p1.serialNo > raceInfo.p2.serialNo ? raceInfo.p2 : raceInfo.p1;
        raceEvent2 = raceInfo.p1.serialNo > raceInfo.p2.serialNo ? raceInfo.p1 : raceInfo.p2;
    }

    std::string const &accessType1 =
        raceEvent1.accessType == static_cast<uint8_t>(AccessType::WRITE) ? " Write" : " Read";
    std::string const &accessType2 =
        raceEvent2.accessType == static_cast<uint8_t>(AccessType::WRITE) ? " Write" : " Read";
    std::string errType = accessType2.substr(1, 1) + "A" + accessType1.substr(1, 1);  // 有空格，取"W"和"R"
    os << "====== ERROR: Potential " << errType << " hazard detected at " << static_cast<MemType>(raceEvent1.memType)
       << RaceFormatKernelName{raceEvent1.deviceId, raceEvent1.kernelIdx} << ":" << std::endl;
    FormatEvent(os, raceEvent1, accessType1, errType);
    FormatEvent(os, raceEvent2, accessType2, errType);
    return os;
}

}

#endif
