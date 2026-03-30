/* -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include "register_sanitizer.h"
#include "register_info_display.h"
#include "core/framework/utility/log.h"
#include "core/framework/utility/ustring.h"

namespace Sanitizer {

bool RegisterSanitizer::SetDeviceInfo(DeviceInfoSummary const &deviceInfo, Config const &config)
{
    checkBlockId_ = config.checkBlockId;
    return false;
}

bool RegisterSanitizer::SetKernelInfo(KernelSummary const &kernelInfo)
{
    Init();
    if (kernelInfo.kernelType == KernelType::AICPU) {
        return false;
    }
    if (kernelInfo.blockDim >= MAX_BLOCKDIM_NUMS) {
        return false;
    }
    kernelName_ = Utility::ReplaceInvalidChar(std::string(kernelInfo.kernelName));
    return true;
}

bool RegisterSanitizer::Init()
{
    for (int i = 0; i < C220_A2_A3_MAXCORE_NUM; ++i) {
        for (int regIdx = 0; regIdx < static_cast<int>(RegisterType::MAX); ++regIdx) {
            regValActual_[i][regIdx].regVal = g_regInfoTbl[regIdx].regDftVal;
            regValActual_[i][regIdx].blockType = BlockType::AIVEC;
            regValActual_[i][regIdx].blockId = 0;
        }
    }

    kernelName_ = "";

    return true;
}

bool RegisterSanitizer::CheckRecordBeforeProcess(const SanitizerRecord &record)
{
    // 预处理当前仅处理KernelRecord
    if (record.version == RecordVersion::KERNEL_RECORD) {
        return true;
    }
    return false;
}

void RegisterSanitizer::Do(const SanitizerRecord &record, const std::vector<SanEvent> &events)
{
    (void)record;

    if (events.empty()) {
        return;
    }

    for (auto& event : events) {
        // 某个算子的事件记录全部结束后，检查是否所有需要检查的寄存器都还原回默认值
        if (event.isEndFrame) {
            for (int i = 0; i < C220_A2_A3_MAXCORE_NUM; ++i) {
                for (int regIdx = 0; regIdx < static_cast<int>(RegisterType::MAX); ++regIdx) {
                    if ((regValActual_[i][regIdx].regVal != g_regInfoTbl[regIdx].regDftVal) &&
                        g_regInfoTbl[regIdx].check) {
                        auto regInfo = RegisterDispInfo {};
                        regInfo.baseEvent.coreId = regValActual_[i][regIdx].blockId;
                        regInfo.baseEvent.blockType = regValActual_[i][regIdx].blockType;
                        regInfo.kernelName = kernelName_;
                        regInfo.regType = static_cast<RegisterType>(regIdx);
                        regInfo.regExpVal.regVal = g_regInfoTbl[regIdx].regDftVal;
                        regInfo.regActVal.regVal = regValActual_[i][regIdx].regVal;

                        ReportRegisterInfo(regInfo);
                    }
                }
            }
        }

        if (event.type != EventType::REGISTER_EVENT) {
            continue;
        }

        int64_t regIdx = event.eventInfo.regInfo.regPayLoad.regIdx;  // 调用 GetRegisterIdx() 接口获取的regIdx，与kernel侧保持一致
        int32_t regType = static_cast<int>(event.eventInfo.regInfo.regType);
        uint64_t regVal = event.eventInfo.regInfo.regPayLoad.regVal;
        // ctrl寄存器仅检测bit56是否归0
        if (event.eventInfo.regInfo.regType == RegisterType::CTRL) {    
            regVal = (regVal >> 56) & 1;
        }

        regValActual_[regIdx][regType].regVal = regVal;
        regValActual_[regIdx][regType].blockType = event.loc.blockType;
        regValActual_[regIdx][regType].blockId = event.loc.coreId;  // 用于后续输出告警时打印
    }
}

void RegisterSanitizer::ParseOnlineError(const KernelErrorRecord &record, BlockType blockType, uint64_t serialNo)
{
    (void)record;
    (void)blockType;
    (void)serialNo;
}

void RegisterSanitizer::RegisterNotifyFunc(const MSG_FUNC &func)
{
    msgFunc_ = func;
}

void RegisterSanitizer::Exit()
{
}

void RegisterSanitizer::ReportRegisterInfo(RegisterDispInfo &dispInfo)
{
    std::set<uint64_t> pcOffsets;
    pcOffsets.insert(dispInfo.baseEvent.pc);
    CallStack::Instance().CachePcOffsets(pcOffsets);
    msgFunc_(LogLv::WARN, [&dispInfo](void) {
        std::stringstream ss;
        ss << dispInfo << std::endl;
        return DetectionInfo{ToolType::REGISTERCHECK, ss.str()};
    });
}

static std::shared_ptr<SanitizerBase> CreateRegisterSanitizer()
{
    return std::make_shared<RegisterSanitizer>();
}

static RegisteSanitizer g_regRegisterSanitizer(ToolType::REGISTERCHECK, CreateRegisterSanitizer);
}