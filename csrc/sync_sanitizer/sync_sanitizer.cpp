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


#include "sync_sanitizer.h"
#include "sync_info_display.h"
#include "core/framework/event_def.h"
#include "core/framework/utility/log.h"
#include <cstring>

namespace Sanitizer {

// 将 SyncType 枚举值转为字符串（利用 # 宏字符串化）
static const char *SyncTypeToStr(SyncType t) {
    switch (t) {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CASE_SYNC_TYPE(v) \
    case SyncType::v: \
        return #v
        CASE_SYNC_TYPE(SET_FLAG);
        CASE_SYNC_TYPE(WAIT_FLAG);
        CASE_SYNC_TYPE(PIPE_BARRIER);
        CASE_SYNC_TYPE(FFTS_SYNC);
        CASE_SYNC_TYPE(WAIT_FLAG_DEV);
        CASE_SYNC_TYPE(IB_SET);
        CASE_SYNC_TYPE(IB_WAIT);
        CASE_SYNC_TYPE(SYNC_ALL);
        CASE_SYNC_TYPE(MSTX_SET_CROSS);
        CASE_SYNC_TYPE(MSTX_WAIT_CROSS);
        CASE_SYNC_TYPE(GET_BUF);
        CASE_SYNC_TYPE(RLS_BUF);
        CASE_SYNC_TYPE(WAIT_INTRA_BLOCK);
        CASE_SYNC_TYPE(HSET_FLAG);
        CASE_SYNC_TYPE(HWAIT_FLAG);
#undef CASE_SYNC_TYPE
    default:
        return "UNKNOWN_SYNC_TYPE";
    }
}

// 根据 SanEvent 类型提取对应的指令名，填入 ErrorEvent.instructName
static void FillStuckInstructName(ErrorEvent &errEvent, const SanEvent &event) {
    const char *name;
    switch (event.type) {
    case EventType::SYNC_EVENT:
        name = SyncTypeToStr(event.eventInfo.syncInfo.opType);
        break;
    case EventType::CROSS_CORE_SYNC_EVENT:
        name = SyncTypeToStr(event.eventInfo.fftsSyncInfo.opType);
        break;
    case EventType::CROSS_CORE_SOFT_SYNC_EVENT:
        name = SyncTypeToStr(event.eventInfo.softSyncInfo.opType);
        break;
    case EventType::MSTX_CROSS_SYNC_EVENT:
        name = SyncTypeToStr(event.eventInfo.mstxCrossInfo.opType);
        break;
    case EventType::BUF_SYNC_EVENT:
        name = SyncTypeToStr(event.eventInfo.bufSyncInfo.opType);
        break;
    case EventType::H_SYNC_EVENT:
        name = SyncTypeToStr(event.eventInfo.hsyncInfo.opType);
        break;
    case EventType::MSTX_CROSS_CORE_BARRIER: // mstx事件没有opType，直接用事件名，下同
        name = "MSTX_CROSS_CORE_BARRIER";
        break;
    case EventType::MSTX_CROSS_NPU_BARRIER:
        name = "MSTX_CROSS_NPU_BARRIER";
        break;
    default:
        name = "UNKNOWN";
        break;
    }

    std::strncpy(errEvent.instructName, name, ERROR_EVENT_INSTRUCT_NAME_MAX_LEN - 1);
}

bool SyncSanitizer::SetDeviceInfo(DeviceInfoSummary const &deviceInfo, Config const &config)
{
    checkBlockId_ = config.checkBlockId;
    deviceType_ = deviceInfo.device;
    return false;
}

bool SyncSanitizer::SetKernelInfo(KernelSummary const &kernelInfo)
{
    kernelType_ = kernelInfo.kernelType;
    Init(kernelInfo);
    if (kernelInfo.kernelType == KernelType::AICPU) {
        return false;
    }
    if (kernelInfo.blockDim >= MAX_BLOCKDIM_NUMS) {
        return false;
    }
    return true;
}

void SyncSanitizer::Init(KernelSummary const &kernelInfo)
{
    syncEvents_.clear();
    pipeRedundancyEvents_.clear();
    redundancyInfo_.clear();
    stuckEvents_.clear();
    isFinished_ = false;

    pipelineReplayer_.Init(kernelInfo.kernelType, deviceType_, kernelInfo.blockDim);

    // 注册卡死回调：当所有 pipe 均阻塞时，收集各队列首个未处理事件
    pipelineReplayer_.RegisterCallback([this](ReplayerCallbackType type, const SanEvent &event) {
        if (type == ReplayerCallbackType::ALL_DEVICE_STUCK) {
            // 工具预处理时插入的同步事件不做判断
            if (event.type == EventType::SYNC_EVENT && event.eventInfo.syncInfo.isGenerated) {
                return;
            }

            ErrorEvent errEvent;
            errEvent.Init(MemEvent(event));
            FillStuckInstructName(errEvent, event);

            stuckEvents_.push_back(errEvent);
        }
    });
}

bool SyncSanitizer::CheckRecordBeforeProcess(const SanitizerRecord &record)
{
    // 预处理当前仅处理KernelRecord
    if (record.version == RecordVersion::KERNEL_RECORD) {
        return true;
    }
    return false;
}

inline bool SyncSanitizer::IsTargetBlockId(uint32_t blockId)
{
    if (checkBlockId_ == CHECK_ALL_BLOCK || blockId == static_cast<uint32_t>(checkBlockId_)) {
        return true;
    }
    return false;
}

// set_flag匹配检测
void SyncSanitizer::DoMatchCheck(SanEvent const &event)
{
    uint64_t selfSyncID {}; // 当前指令
    uint64_t peerSyncID {}; // 与之配对的指令

    if (event.type == EventType::SYNC_EVENT && event.eventInfo.syncInfo.opType == SyncType::SET_FLAG) {
        selfSyncID = CalcSetFlagSyncID(event);
        peerSyncID = CalcWaitFlagSyncID(event);
    } else if (event.type == EventType::SYNC_EVENT && event.eventInfo.syncInfo.opType == SyncType::WAIT_FLAG) {
        selfSyncID = CalcWaitFlagSyncID(event);
        peerSyncID = CalcSetFlagSyncID(event);
    } else {
        return;
    }

    if (syncEvents_.find(peerSyncID) != syncEvents_.end()) {
        // 如果找到配对指令，就删除配对指令
        if (syncEvents_[peerSyncID].size() > 1) {
            syncEvents_[peerSyncID].pop_back();
        } else {
            syncEvents_.erase(peerSyncID);
        }
    } else {
        // 如果找不到，就插入当前指令
        auto selfSyncInfo = SyncDispInfo {};
        selfSyncInfo.baseEvent.Init(MemEvent(event));
        selfSyncInfo.srcPipe = event.eventInfo.syncInfo.srcPipe;
        selfSyncInfo.dstPipe = event.eventInfo.syncInfo.dstPipe;
        selfSyncInfo.eventId = event.eventInfo.syncInfo.eventId;
        selfSyncInfo.opType = event.eventInfo.syncInfo.opType;
        selfSyncInfo.checkType = SyncCheckType::MATCH_CHECK;
        syncEvents_[selfSyncID].push_back(selfSyncInfo);
    }
}

// 算子卡死检测
void SyncSanitizer::DoStuckCheck(SanEvent const &event) {
    pipelineReplayer_.Do(event);

    if (pipelineReplayer_.IsFinished()) {
        if (!stuckEvents_.empty()) {
            SyncStuckDspInfo stuckInfo;
            stuckInfo.stuckEventList = stuckEvents_;
            ReportSyncStuckInfo(stuckInfo);
        }
    }
}

SyncDispInfo getSyncDispInfoFromEvent(SanEvent const &event)
{
    SyncDispInfo info = SyncDispInfo {};

    info.baseEvent.Init(MemEvent(event));
    info.srcPipe = event.eventInfo.syncInfo.srcPipe;
    info.dstPipe = event.eventInfo.syncInfo.dstPipe;
    info.eventId = event.eventInfo.syncInfo.eventId;
    info.opType = event.eventInfo.syncInfo.opType;
    info.checkType = SyncCheckType::REDUMTAMCY_CHECK;

    return info;
}

// 获取冗余判断时用于分发指令的pipetype
PipeType getPipeTypeFromSanEvent(SanEvent const &event)
{
    switch (event.type) {
        case EventType::MEM_EVENT:
        case EventType::TIME_EVENT:
            return event.pipe;
        case EventType::SYNC_EVENT:
            switch (event.eventInfo.syncInfo.opType) {
                case SyncType::RLS_BUF:     // 冗余检测只涉及A2/A3，A5相关特性直接返回
                    return PipeType::SIZE;
                case SyncType::WAIT_FLAG:   // wait_flag使用srcPipe
                    return event.eventInfo.syncInfo.dstPipe;
                default:                    // set_flag和其它sync事件使用srcPipe
                    return event.eventInfo.syncInfo.srcPipe;
            }
        case EventType::CROSS_CORE_SYNC_EVENT:
            return event.eventInfo.fftsSyncInfo.dstPipe;
        case EventType::CROSS_CORE_SOFT_SYNC_EVENT:
            return PipeType::PIPE_S;    // SOFT_SYNC固定为PIPE_S
        case EventType::MSTX_CROSS_SYNC_EVENT:
            return event.eventInfo.mstxCrossInfo.pipe;
        default:
            return PipeType::SIZE;
    }
}

// set_flag/wait_flag冗余检测
void SyncSanitizer::DoRedundancyCheck(SanEvent const &event)
{
    PipeType pipe = getPipeTypeFromSanEvent(event);
    // 不需要检测的事件直接返回
    if (pipe == PipeType::SIZE) {
        return;
    }

    // 出现非sync事件时，当前pipe保存的上一条指令必定不冗余，清除指令表中该pipe保存的上一条指令
    if (event.type != EventType::SYNC_EVENT) {
        pipeRedundancyEvents_.erase(pipe);
        return;
    }

    // 工具预处理时插入的同步事件不做判断
    if (event.eventInfo.syncInfo.isGenerated) {
        return;
    }

    bool redundantFlag = false;
    SyncDispInfo selfSyncInfo = getSyncDispInfoFromEvent(event);

    switch (event.eventInfo.syncInfo.opType) {
        case SyncType::SET_FLAG:    // 出现 SET_FLAG/WAIT_FLAG 时，与保存的上一条指令比较看是否完全一致
        case SyncType::WAIT_FLAG:
            redundantFlag = selfSyncInfo == pipeRedundancyEvents_[pipe];
            break;
        case SyncType::PIPE_BARRIER: // PIPE_BARRIER 忽略不处理，不记录指令表
            return;
        default:    // 其他的sync事件直接记表
            break;
    }

    // 将冗余的两个事件记录下来，后续输出
    if (redundantFlag) {
        redundancyInfo_.push_back(pipeRedundancyEvents_[pipe]);
        redundancyInfo_.push_back(selfSyncInfo);
    }

    pipeRedundancyEvents_[pipe] = selfSyncInfo;
}

void SyncSanitizer::Do(const SanitizerRecord &record, const std::vector<SanEvent> &events) {
    if (IsMstxRecordWithTensor(record)) {
        return;
    }

    if (events.empty()) {
        return;
    }

    for (auto& event : events) {
        // 卡死检测需要所有 block 的事件，放在过滤之前
        DoStuckCheck(event);

        if (event.type == EventType::SANITIZER_CONTROL_EVENT &&
            event.eventInfo.sanitizerControlInfo.type == SanitizerControlType::KERNEL_FINISH) {
            isFinished_ = true;
            break;
        }
        if (!IsTargetBlockId(event.loc.coreId)) {
            continue;
        }

        DoMatchCheck(event);
        DoRedundancyCheck(event);
    }

    if (isFinished_) {
        if (!syncEvents_.empty()) {
            ReportUnpairedInfo();
        }
        if (!redundancyInfo_.empty()) {
            ReportRedundancyInfo();
        }
        if (!syncThreadsInfo_.empty()) {
            ReportSyncThreadsInfo();
        }
    }
}

void SyncSanitizer::ParseOnlineError(const KernelErrorRecord &record, BlockType blockType, uint64_t serialNo)
{
    if (record.kernelErrorDesc == nullptr) {
        SAN_ERROR_LOG("kernelErrorDesc is nullptr");
        return;
    }

    for (size_t errorIdx = 0; errorIdx < record.errorNum; ++errorIdx) {
        const KernelErrorDesc &kernelErrorDesc = record.kernelErrorDesc[errorIdx];
        if (kernelErrorDesc.errorType >= KernelErrorType::MAX) {
            SAN_ERROR_LOG("Unknown kernel error type: %u", static_cast<uint32_t>(kernelErrorDesc.errorType));
            continue;
        }
        static bool hasWarnedSyncIndexOutOfBounds = false;
        if (!hasWarnedSyncIndexOutOfBounds && kernelErrorDesc.errorType == KernelErrorType::SYNC_THREADS_RECORD_LOSS) {
            hasWarnedSyncIndexOutOfBounds = true;
            std::cout << "[mssanitizer] WARNING: the number of '__syncthreads()' instructions exceeds "
                      << SIMT_THREAD_MAX_PC_NUM
                      << ". Some records were discarded, the detection result maybe unreliable!" << std::endl;
        }
        if (kernelErrorDesc.errorType == KernelErrorType::THREADS_ASYNC_IN_BLOCK) {
            ErrorEvent event{};
            event.serialNo = serialNo;
            event.SetDeviceIdKernelIdx();
            event.coreId = kernelErrorDesc.location.blockId;
            event.pc = kernelErrorDesc.payload.syncDesc.syncLocation.pc;
            event.blockType = blockType;
            event.isSimt = true;
            event.threadLoc = kernelErrorDesc.payload.syncDesc.syncThreadLoc;
            syncThreadsInfo_.emplace_back(event);
        }
    }
}

void SyncSanitizer::ReportUnpairedInfo()
{
    // build pc stack map cache
    std::set<uint64_t> pcOffsets;
    std::vector<SyncDispInfo> dispEvents;
    for (auto &each : syncEvents_) {
        if (!(each.first & 0xF)) { // 为空说明是set_flag，有值则为wait_flag
            auto &syncEvents = each.second;
            for (auto &syncEvent : syncEvents) {
                pcOffsets.insert(syncEvent.baseEvent.pc);
                dispEvents.push_back(syncEvent);
            }
        }
    }

    if (dispEvents.empty()) {
        return;
    }
    CallStack::Instance().CachePcOffsets(RuntimeContext::Instance().kernelSummary_.kernelName, pcOffsets);
    for (SyncDispInfo const &it : dispEvents) {
        msgFunc_(LogLv::WARN, [&it](void) {
            std::stringstream ss;
            ss << it << std::endl;
            return DetectionInfo{ToolType::SYNCCHECK, ss.str()};
        });
    }
}

void SyncSanitizer::ReportRedundancyInfo() const
{
    std::set<uint64_t> pcOffsets;
    for (SyncDispInfo const &info : redundancyInfo_) {
        pcOffsets.insert(info.baseEvent.pc);
    }

    CallStack::Instance().CachePcOffsets(RuntimeContext::Instance().kernelSummary_.kernelName, pcOffsets);
    for (SyncDispInfo const &it : redundancyInfo_) {
        msgFunc_(LogLv::WARN, [&it](void) {
            std::stringstream ss;
            ss << it << std::endl;
            return DetectionInfo{ToolType::SYNCCHECK, ss.str()};
        });
    }
}


void SyncSanitizer::ReportSyncThreadsInfo() const
{
    std::set<uint64_t> pcOffsets;
    for (ErrorEvent const &info : syncThreadsInfo_) {
        pcOffsets.insert(info.pc);
    }

    CallStack::Instance().CachePcOffsets(RuntimeContext::Instance().kernelSummary_.kernelName, pcOffsets);
    for (ErrorEvent const &it : syncThreadsInfo_) {
        msgFunc_(LogLv::ERROR, [&it](void) {
            std::stringstream ss;
            ss << it << std::endl;
            return DetectionInfo{ToolType::SYNCCHECK, ss.str()};
        });
    }
}

void SyncSanitizer::ReportSyncStuckInfo(SyncStuckDspInfo &stuckDspInfo) {
    // build pc stack map cache
    std::set<uint64_t> pcOffsets;
    for (ErrorEvent const &stuckEvent : stuckDspInfo.stuckEventList) {
        pcOffsets.insert(stuckEvent.pc);
    }

    CallStack::Instance().CachePcOffsets(RuntimeContext::Instance().kernelSummary_.kernelName, pcOffsets);

    msgFunc_(LogLv::ERROR, [&stuckDspInfo](void) {
        std::stringstream ss;
        ss << stuckDspInfo << std::endl;
        return DetectionInfo{ToolType::SYNCCHECK, ss.str()};
    });
}

uint64_t SyncSanitizer::CalcSetFlagSyncID(SanEvent const &event)
{
    constexpr uint8_t coreIDShift = 20;
    constexpr uint8_t blockTypeShift = 16;
    constexpr uint8_t srcPipeShift = 12;
    constexpr uint8_t dstPipeShift = 8;
    constexpr uint8_t eventIDShift = 4;

    return ((static_cast<uint64_t>(event.loc.coreId) & 0xFFFF) << coreIDShift) |
           ((static_cast<uint64_t>(event.loc.blockType) & 0xF) << blockTypeShift) |
           ((static_cast<uint64_t>(event.eventInfo.syncInfo.srcPipe) & 0xF) << srcPipeShift) |
           ((static_cast<uint64_t>(event.eventInfo.syncInfo.dstPipe) & 0xF) << dstPipeShift) |
           ((static_cast<uint64_t>(event.eventInfo.syncInfo.eventId) & 0xF) << eventIDShift);
}

uint64_t SyncSanitizer::CalcWaitFlagSyncID(SanEvent const &event)
{
    return (CalcSetFlagSyncID(event) + 1UL);
}

void SyncSanitizer::RegisterNotifyFunc(const MSG_FUNC &func)
{
    msgFunc_ = func;
}


void SyncSanitizer::Exit()
{
}

static std::shared_ptr<SanitizerBase> CreateSyncSanitizer()
{
    return std::make_shared<SyncSanitizer>();
}

static RegisteSanitizer g_regSyncSanitizer(ToolType::SYNCCHECK, CreateSyncSanitizer);
}
