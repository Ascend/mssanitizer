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
#include <algorithm>
#include <set>
#include <sstream>
#include <functional>
#include "core/framework/event_def.h"
#include "core/framework/config.h"
#include "core/framework/utility/types.h"
#include "core/framework/call_stack.h"
#include "core/framework/kernel_manager.h"
#include "core/framework/record_defs.h"
#include "core/framework/record_format.h"
#include "core/framework/file_mapping.h"
#include "core/framework/runtime_context.h"

namespace Sanitizer {

inline std::ostream &PrintClassicLocation(std::ostream &os, uint64_t fileNo, uint64_t lineNo,
    uint64_t serialNo, const std::string &prefix = "")
{
    os << prefix << "at ";
    FileInfo fileInfo = FileMapping::Instance().Query(fileNo);
    if (fileInfo.fileIdx == -1 || fileInfo.fileName.empty()) {
        os << "<unknown>";
    } else {
        os << fileInfo.fileName << ":" << lineNo;
    }
    return os << " (serialNo:" << serialNo << ")" << std::endl;
}

inline std::ostream &PrintLocationInfo(std::ostream &os, ErrorEvent const &event, uint64_t serialNo,
    bool isOnlineError = false, const std::string &prefix = "")
{
    if (event.pc == 0UL) {
        return PrintClassicLocation(os, event.fileNo, event.lineNo, serialNo, prefix);
    }

    CallStack::Stack stack;
    KernelSummary kernelSummary{};
    if (KernelManager::Instance().Get(event.deviceId, event.kernelIdx, kernelSummary)) {
        stack = CallStack::Instance().Query(kernelSummary.kernelName, event.pc);
    }
    if (event.isSimt) {
        CallStack::Stack mainScalarStack = CallStack::Instance().Query(
            kernelSummary.kernelName, event.threadLoc.mainScalarPc);
        stack.reserve(stack.size() + mainScalarStack.size());
        stack.insert(stack.end(), std::make_move_iterator(mainScalarStack.begin()),
            std::make_move_iterator(mainScalarStack.end()));
    }
    if (stack.empty()) {
        return PrintClassicLocation(os, event.fileNo, event.lineNo, serialNo, prefix);
    }
    os << prefix << "at pc current 0x" << std::hex << event.pc << std::dec;
    if (!isOnlineError) os << " (serialNo:" << serialNo << ")";
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

inline void FormatEvent(std::ostream &os, const ErrorEvent &event, std::string const &accessType,
    std::string const &errType, bool isOnlineError = false)
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
    PrintLocationInfo(os, event, event.serialNo, isOnlineError);
}

inline std::ostream &FormatRaceInfo(std::ostream &os, RaceDispInfo const &raceInfo)
{
    ErrorEvent raceEvent1{};
    ErrorEvent raceEvent2{};
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
    FormatEvent(os, raceEvent1, accessType1, errType, raceInfo.isOnlineError);
    FormatEvent(os, raceEvent2, accessType2, errType, raceInfo.isOnlineError);
    return os;
}

inline void FormatEvent(std::ostream &os, const ErrorEvent &event) {
    std::string const &accessType = event.accessType == static_cast<uint8_t>(AccessType::WRITE) ? "Write" : "Read";
    os << "======    " << static_cast<PipeType>(event.pipeType) << " " << accessType << " at 0x" << std::hex
       << event.addr << std::dec << " in block " << event.coreId << " (" << event.blockType << ")" << " on device "
       << event.deviceId << " ";
    PrintLocationInfo(os, event, event.serialNo);
}

inline std::ostream &FormatMissDcciInfo(std::ostream &os, RaceDispInfo raceInfo) {
    // always show write event first
    if (raceInfo.p1.accessType == static_cast<uint8_t>(AccessType::READ)) {
        std::swap(raceInfo.p1, raceInfo.p2);
    }

    os << "====== ERROR: Missing DCCI instructions detected on cross core data dependency at "
       << static_cast<MemType>(raceInfo.p1.memType) << RaceFormatKernelName{raceInfo.p1.deviceId, raceInfo.p1.kernelIdx}
       << ":" << std::endl;
    FormatEvent(os, raceInfo.p1);
    FormatEvent(os, raceInfo.p2);
    return os;
}

inline std::ostream &operator << (std::ostream &os, RaceDispInfo const &raceInfo) {
    if (raceInfo.isMissDcci) {
        return FormatMissDcciInfo(os, raceInfo);
    } else {
        return FormatRaceInfo(os, raceInfo);
    }
}

// mode4 场景 AIV 核 flag_id 参数非法告警打印
inline std::ostream &operator << (std::ostream &os, CrossCoreSyncWarnInfo const &warnInfo) {
    ErrorEvent const &event = warnInfo.baseEvent;
    os << "====== WARNING: Invalid flag_id " << static_cast<uint32_t>(warnInfo.flagId)
       << RaceFormatKernelName{event.deviceId, event.kernelIdx} << ":" << std::endl
       << "======    in block " << event.blockType << "(" << event.coreId << ")"
       << " on device " << event.deviceId << std::endl;
    // 位置/调用栈各行为保证与上一行 "in block ..." 对齐（统一 "======    " 前缀）
    PrintLocationInfo(os, event, event.serialNo, false, "======    ");
    return os;
}

// mode4 场景 flag_id 非法告警统一上报（核内/跨NPU 检测共用）
// 同一 mode4 同步事件可能被多个检测算法处理，先按 (flagId, serialNo, pc, coreId, deviceId) 去重，最后通过 msgFunc 回调打屏输出。
inline void ReportFlagIdWarnInfos(const std::vector<CrossCoreSyncWarnInfo> &warnInfos,
    const std::string &kernelName,
    const std::function<void(const LogLv &lv, Generator<DetectionInfo> &&gen)> &msgFunc)
{
    if (warnInfos.empty()) {
        return;
    }

    std::vector<CrossCoreSyncWarnInfo> uniqueWarnInfos;
    for (const auto &info : warnInfos) {
        if (std::find(uniqueWarnInfos.begin(), uniqueWarnInfos.end(), info) == uniqueWarnInfos.end()) {
            uniqueWarnInfos.push_back(info);
        }
    }
    if (uniqueWarnInfos.empty()) {
        return;
    }

    // build pc stack map cache
    std::set<uint64_t> pcOffsets;
    for (const auto &info : uniqueWarnInfos) {
        pcOffsets.insert(info.baseEvent.pc);
    }
    CallStack::Instance().CachePcOffsets(kernelName, pcOffsets);

    for (const auto &it : uniqueWarnInfos) {
        msgFunc(LogLv::WARN, [&it](void) {
            std::stringstream ss;
            ss << it << std::endl;
            return DetectionInfo{ToolType::RACECHECK, ss.str()};
        });
    }
}
}

#endif
