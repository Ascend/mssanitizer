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

#include "core/framework/kernel_manager.h"
#include "core/framework/record_defs.h"
#include "core/framework/sanitizer_base.h"
#include "core/framework/call_stack.h"
#include "race_info_display.h"

#include "cross_npu_race_sanitizer.h"

namespace Sanitizer {

CrossNpuRaceSanitizer::CrossNpuRaceSanitizer()
{
}

void CrossNpuRaceSanitizer::Do(const SanitizerRecord &record, const std::vector<SanEvent> &events)
{
    if (IsMstxRecordWithTensor(record)) {
        return;
    }

    for (auto const &event : events) {
        raceAlg_.Do(event);
    }

    if (raceAlg_.IsFinished()) {
        RaceSanitizerRecord(raceAlg_.GetResult());
    }
}

void CrossNpuRaceSanitizer::RaceSanitizerRecord(std::shared_ptr<std::vector<RaceDispInfo>> p) const
{
    if (p->empty()) {
        return;
    }

    // build pc stack map cache
    std::set<uint64_t> pcOffsets;
    for (RaceDispInfo const &error : *p) {
        pcOffsets.insert(error.p1.pc);
        pcOffsets.insert(error.p2.pc);
    }

    // 当前 NPU 间检测时限定相同的 kernelName 间，因此任意取一个竞争事件用于获取 kernelName
    ErrorEvent const &event = (*p)[0].p1;
    KernelSummary kernelSummary{};
    if (!KernelManager::Instance().Get(event.deviceId, event.kernelIdx, kernelSummary)) {
        SAN_ERROR_LOG("Get kernelSummary failed in RaceSanitizerRecord. deviceId: %u, kernelIdx: %u",
                      event.deviceId, event.kernelIdx);
        return;
    }
    CallStack::Instance().CachePcOffsets(kernelSummary.kernelName, pcOffsets);

    for (const auto &it : *p) {
        msgFunc_(LogLv::ERROR, [&it](void) {
            std::stringstream ss;
            ss << it << std::endl;
            return DetectionInfo{ToolType::RACECHECK, ss.str()};
        });
    }
}

void CrossNpuRaceSanitizer::RegisterNotifyFunc(const MSG_FUNC &func)
{
    msgFunc_ = func;
}

void CrossNpuRaceSanitizer::Init()
{
    raceAlg_.Init();
}

bool CrossNpuRaceSanitizer::SetDeviceInfo(DeviceInfoSummary const &deviceInfo, Config const &config)
{
    raceAlg_.SetDeviceInfo(deviceInfo, config);
    return true;
}

bool CrossNpuRaceSanitizer::SetKernelInfo(KernelSummary const &kernelInfo)
{
    raceAlg_.SetKernelInfo(kernelInfo);
    return true;
}

} // namespace Sanitizer