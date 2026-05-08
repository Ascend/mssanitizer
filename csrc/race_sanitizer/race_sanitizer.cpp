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


#include "race_sanitizer.h"
#include <sstream>
#include "core/framework/runtime_context.h"
#include "core/framework/utility/log.h"
#include "alg_framework/race_alg_factory.h"
#include "core/framework/format_converter.h"
#include "race_info_display.h"

namespace {

uint8_t inline ThreadRaceToAccessType(Sanitizer::KernelErrorType errorType, bool isSecondEvent = false)
{
    if (errorType == Sanitizer::KernelErrorType::THREAD_RW_RACE) {
        if (isSecondEvent) {
            return static_cast<uint8_t>(Sanitizer::AccessType::READ);
        }
        return static_cast<uint8_t>(Sanitizer::AccessType::WRITE);
    } else if (errorType == Sanitizer::KernelErrorType::THREAD_WR_RACE) {
        if (isSecondEvent) {
            return static_cast<uint8_t>(Sanitizer::AccessType::WRITE);
        }
        return static_cast<uint8_t>(Sanitizer::AccessType::READ);
    } else if (errorType == Sanitizer::KernelErrorType::THREAD_WW_RACE) {
        return static_cast<uint8_t>(Sanitizer::AccessType::WRITE);
    }
    return 0U;
}

}

namespace Sanitizer {
enum class RaceAlgCheckBlockType: uint8_t {
    RACE_ALG_AIV_CHECK = 0U, // AIV核内检测算法
    RACE_ALG_AIC_CHECK, // AIC核内检测算法
    RACE_ALG_AIV_PIPE_CHECK, // AIV核内单一流水检测算法
    RACE_ALG_AIC_PIPE_CHECK, // AIC核内单一流水检测算法
    RACE_ALG_ALL_BLOCK_CHECK, // 核间检测算法
};

bool RaceSanitizer::SetDeviceInfo(DeviceInfoSummary const &deviceInfo, Config const &config)
{
    checkBlockId_ = config.checkBlockId;
    deviceType_ = deviceInfo.device;
    simtErrors_ = std::make_shared<std::vector<RaceDispInfo>>();
    return false;
}

bool RaceSanitizer::SetKernelInfo(KernelSummary const &kernelInfo)
{
    if (kernelInfo.kernelType == KernelType::AICPU) {
        return false;
    }
    if (kernelInfo.blockDim >= MAX_BLOCKDIM_NUMS) {
        return false;
    }
    static const std::map<RaceAlgCheckBlockType, RaceCheckType> RACE_ALG_MAP = {
        {RaceAlgCheckBlockType::RACE_ALG_AIV_CHECK, RaceCheckType::SINGLE_BLOCK_CHECK},
        {RaceAlgCheckBlockType::RACE_ALG_AIC_CHECK, RaceCheckType::SINGLE_BLOCK_CHECK},
        {RaceAlgCheckBlockType::RACE_ALG_AIV_PIPE_CHECK, RaceCheckType::SINGLE_PIPE_CHECK},
        {RaceAlgCheckBlockType::RACE_ALG_AIC_PIPE_CHECK, RaceCheckType::SINGLE_PIPE_CHECK},
        {RaceAlgCheckBlockType::RACE_ALG_ALL_BLOCK_CHECK, RaceCheckType::CROSS_BLOCK_CHECK},
    };
    for (uint8_t i = 0; i <= static_cast<uint8_t>(RaceAlgCheckBlockType::RACE_ALG_ALL_BLOCK_CHECK); i++) {
        auto it = RACE_ALG_MAP.find(static_cast<RaceAlgCheckBlockType>(i));
        RaceCheckType checkType = it == RACE_ALG_MAP.end() ? RaceCheckType::SINGLE_BLOCK_CHECK : it->second;
        raceAlgs_[i] = RaceAlgFactory::Create(checkType, kernelInfo.kernelType, deviceType_, kernelInfo.blockDim);
    }
    return true;
}

inline bool RaceSanitizer::IsTargetBlockId(uint32_t blockId)
{
    if (blockId == static_cast<uint32_t>(checkBlockId_)) {
        return true;
    }
    if (blockId == defaultCheckBlockId_ && checkBlockId_ == CHECK_ALL_BLOCK) {
        return true;
    }
    return false;
}

bool RaceSanitizer::IsTargetEvent(const SanEvent &event, BlockType targetBlockType)
{
    if (event.isEndFrame) {
        return true;
    }

    uint32_t targetBlockId = checkBlockId_ == CHECK_ALL_BLOCK ? defaultCheckBlockId_ : checkBlockId_;
    if (event.loc.coreId == targetBlockId && event.loc.blockType == targetBlockType) {
        return true;
    }
    /// 则其他核只有硬同步和mstx核间同步事件会入队
    if (event.type == EventType::MSTX_CROSS_SYNC_EVENT &&
        (event.eventInfo.mstxCrossInfo.opType == SyncType::MSTX_SET_CROSS ||
        event.eventInfo.mstxCrossInfo.opType == SyncType::MSTX_WAIT_CROSS)) {
        return true;
    }
    return event.type == EventType::CROSS_CORE_SYNC_EVENT;
}

void RaceSanitizer::MergeSimtErrors()
{
    std::vector<RaceDispInfo> uniqueErrors;
    if (simtErrors_ == nullptr) {
        SAN_ERROR_LOG("simt errors is nullptr");
        return;
    }
    for (const auto &current : *simtErrors_) {
        bool found = false;
        for (auto &exist : uniqueErrors) {
            if (current.IsSameSimt(exist)) {
                found = true;
                exist.UpdateMinThreadLoc(current);
                break;
            }
        }
        if (!found) {
            uniqueErrors.push_back(current);
        }
    }
    simtErrors_->clear();
    simtErrors_->assign(uniqueErrors.begin(), uniqueErrors.end());
}

void RaceSanitizer::RaceSanitizerRecord(std::shared_ptr<std::vector<RaceDispInfo>> p)
{
    // build pc stack map cache
    std::set<uint64_t> pcOffsets;
    for (RaceDispInfo const &error : *p) {
        pcOffsets.insert(error.p1.pc);
        pcOffsets.insert(error.p2.pc);
    }
    CallStack::Instance().CachePcOffsets(RuntimeContext::Instance().kernelSummary_.kernelName, pcOffsets);

    for (const auto &it : *p) {
        msgFunc_(LogLv::ERROR, [&it](void) {
            std::stringstream ss;
            ss << it << std::endl;
            return DetectionInfo{ToolType::RACECHECK, ss.str()};
        });
    }
}

void RaceSanitizer::SingleBlockRaceCheck(const std::vector<SanEvent> &events)
{
    for (const auto &event : events) {
        if (IsTargetEvent(event, BlockType::AIVEC) || IsTargetEvent(event, BlockType::AICORE)) {
            raceAlgs_[static_cast<uint8_t>(RaceAlgCheckBlockType::RACE_ALG_AIV_CHECK)]->Do(event);
        }
    }
    for (const auto &event : events) {
        if (IsTargetEvent(event, BlockType::AICUBE)) {
            raceAlgs_[static_cast<uint8_t>(RaceAlgCheckBlockType::RACE_ALG_AIC_CHECK)]->Do(event);
        }
    }
}

void RaceSanitizer::SinglePipeRaceCheck(const std::vector<SanEvent> &events)
{
    for (const auto &event : events) {
        if ((IsTargetBlockId(event.loc.coreId) && (event.loc.blockType == BlockType::AIVEC ||
            event.loc.blockType == BlockType::AICORE)) || event.isEndFrame) {
            raceAlgs_[static_cast<uint8_t>(RaceAlgCheckBlockType::RACE_ALG_AIV_PIPE_CHECK)]->Do(event);
        }
    }
    for (const auto &event : events) {
        if ((IsTargetBlockId(event.loc.coreId) && event.loc.blockType == BlockType::AICUBE) || event.isEndFrame) {
            raceAlgs_[static_cast<uint8_t>(RaceAlgCheckBlockType::RACE_ALG_AIC_PIPE_CHECK)]->Do(event);
        }
    }
}

void RaceSanitizer::AllBlockRaceCheck(const std::vector<SanEvent> &events)
{
    if (checkBlockId_ != CHECK_ALL_BLOCK) {
        return;
    }
    for (const auto &event : events) {
        raceAlgs_[static_cast<uint8_t>(RaceAlgCheckBlockType::RACE_ALG_ALL_BLOCK_CHECK)]->Do(event);
    }
}

bool RaceSanitizer::CheckRecordBeforeProcess(const SanitizerRecord &record)
{
    // 预处理当前仅处理KernelRecord，并且不处理simt的竞争事件
    if (record.version == RecordVersion::KERNEL_RECORD &&
        record.payload.kernelRecord.recordType != RecordType::SHADOW_MEMORY) {
        return true;
    }
    return false;
}

void RaceSanitizer::Do(const SanitizerRecord &record, const std::vector<SanEvent> &events)
{
    if (IsMstxRecordWithTensor(record)) {
        return;
    }

    if (events.empty()) {
        return;
    }

    // 进行检测：分核内（流水间和流水内）、核间遍历events，处理后进行检测
    SingleBlockRaceCheck(events);
    SinglePipeRaceCheck(events);
    AllBlockRaceCheck(events);

    // 显示竞争结果
    for (const auto &it : raceAlgs_) {
        if (it->IsFinished()) {
            RaceSanitizerRecord(it->GetResult());
        }
    }

    // 显示simt核内线程间的竞争结果
    if (record.version == RecordVersion::KERNEL_RECORD &&
        record.payload.kernelRecord.recordType == RecordType::FINISH) {
        MergeSimtErrors();
        RaceSanitizerRecord(simtErrors_);
        simtErrors_->clear();
    }
}

void RaceSanitizer::ParseOnlineError(const KernelErrorRecord &record, BlockType blockType, uint64_t serialNo)
{
    if (simtErrors_ == nullptr) {
        SAN_ERROR_LOG("simt errors is nullptr");
        return;
    }

    for (size_t errorIdx = 0; errorIdx < record.errorNum; ++errorIdx) {
        const KernelErrorDesc &kernelErrorDesc = record.kernelErrorDesc[errorIdx];
        if (kernelErrorDesc.errorType >= KernelErrorType::MAX) {
            SAN_ERROR_LOG("Unknown kernel error type: %u", static_cast<uint32_t>(kernelErrorDesc.errorType));
            continue;
        }

        if (kernelErrorDesc.errorType == KernelErrorType::THREAD_RW_RACE ||
            kernelErrorDesc.errorType == KernelErrorType::THREAD_WR_RACE ||
            kernelErrorDesc.errorType == KernelErrorType::THREAD_WW_RACE) {
            RaceDispInfo error{};
            BaseEvent event{};
            auto &errorDesc = kernelErrorDesc.payload.raceDesc;
            event.coreId = kernelErrorDesc.location.blockId;
            event.addr = errorDesc.addr;
            event.pc = kernelErrorDesc.location.pc;
            event.fileNo = kernelErrorDesc.location.fileNo;
            event.lineNo = kernelErrorDesc.location.lineNo;
            event.blockType = blockType;
            event.isSimt = true;
            event.accessType = ThreadRaceToAccessType(kernelErrorDesc.errorType);
            event.memType = static_cast<uint8_t>(FormatConverter::AddrSpaceToMemType(kernelErrorDesc.space));
            event.threadLoc = kernelErrorDesc.threadLoc;
            error.p1 = event;
            
            event.accessType = ThreadRaceToAccessType(kernelErrorDesc.errorType, true);
            event.pc = errorDesc.conflictedLocation.pc;
            event.fileNo = errorDesc.conflictedLocation.fileNo;
            event.lineNo = errorDesc.conflictedLocation.lineNo;
            event.threadLoc = errorDesc.conflictedThreadLoc;
            error.p2 = event;
            simtErrors_->emplace_back(error);
        }
    }
}

void RaceSanitizer::RegisterNotifyFunc(const MSG_FUNC &func)
{
    msgFunc_ = func;
    return;
}

void RaceSanitizer::Exit()
{
}

static std::shared_ptr<SanitizerBase> CreateRaceSanitizer()
{
    return std::make_shared<RaceSanitizer>();
}

static RegisteSanitizer g_regRaceSanitizer(ToolType::RACECHECK, CreateRaceSanitizer);
}
