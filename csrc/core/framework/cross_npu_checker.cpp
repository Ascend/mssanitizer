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

#include <iostream>
#include <mutex>

#include "core/framework/platform_config.h"
#include "core/framework/utility/type_traits.h"
#include "device_manager.h"
#include "event_def.h"
#include "kernel_manager.h"
#include "record_defs.h"
#include "record_format.h"
#include "record_pre_process.h"
#include "sanitizer_base.h"

#include "cross_npu_checker.h"

namespace Sanitizer {

void CrossNpuChecker::RecordArray::CreateNewKernel()
{
    records_.resize(records_.size() + 1);
}

void CrossNpuChecker::RecordArray::Push(SanitizerRecord const &record)
{
    // 只处理 kernel 侧记录
    if (record.version != RecordVersion::KERNEL_RECORD) {
        return;
    }

    records_.back().emplace_back(record);
}

CrossNpuChecker::CrossNpuChecker(Config const &config) : config_{config}
{
}

CrossNpuChecker::RecordArray &CrossNpuChecker::GetRecordArray(uint32_t deviceId)
{
    std::unique_lock<std::mutex> guard;
    return recordMap_[deviceId];
}

void CrossNpuChecker::SetDetectionInfo(const LogLv &expectLv, std::ostream &detectionOstream)
{
    detectionOstream_ = &detectionOstream;

    auto func = [expectLv, &detectionOstream, this](const LogLv &lv, SanitizerBase::MSG_GEN &&gen) {
        if (lv >= expectLv) {
            ++errorCount_;
            detectionOstream << gen().message << std::flush;
        }
    };
    sanitizer_.RegisterNotifyFunc(func);
}

bool CrossNpuChecker::Run()
{
    DisplaySanitizerBegin(config_);

    sanitizer_.Init();

    // 按每个 device 上的每个 kernel 回放所有缓存的指令记录
    auto deviceList = DeviceManager::Instance().GetDeviceList();
    for (std::size_t deviceIdx = 0; deviceIdx < deviceList.size(); ++deviceIdx) {
        uint32_t deviceId = deviceList[deviceIdx];
        auto it = Sanitizer::as_const(recordMap_).find(deviceId);
        if (it == recordMap_.cend()) {
            continue;
        }
        auto const &kernels = it->second.records_;
        DeviceInfoSummary deviceInfo{};
        if (!DeviceManager::Instance().Get(deviceId, deviceInfo)) {
            SAN_ERROR_LOG("Get device info from device manager failed. deviceId: %u", deviceId);
            return false;
        }
        sanitizer_.SetDeviceInfo(deviceInfo, config_);
        for (std::size_t kernelIdx = 0; kernelIdx < kernels.size(); ++kernelIdx) {
            auto const &kernel = kernels[kernelIdx];
            KernelSummary kernelSummary{};
            if (!KernelManager::Instance().Get(deviceId, kernelIdx, kernelSummary)) {
                SAN_ERROR_LOG("Get kernel summary from kernel manager failed. deviceId: %u, kernelIdx: %zu",
                              deviceId, kernelIdx);
                return false;
            }
            sanitizer_.SetKernelInfo(kernelSummary);
            if (!CheckFilter(deviceInfo, kernelSummary)) {
                continue;
            }
            for (auto const & r : kernel) {
                std::vector<SanEvent> events;
                RecordPreProcess::GetInstance().Process(r, events);
                UpdateLoc(events, deviceIdx, kernelIdx, deviceId);
                sanitizer_.Do(r, events);
            }
        }
    }

    SanitizerRecord record;
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = RecordType::FINISH;
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(record, events);
    sanitizer_.Do(record, events);

    DisplaySanitizerEnd();
    return true;
}

void CrossNpuChecker::DisplaySanitizerBegin(Config const &config) const
{
    if (detectionOstream_ == nullptr) {
        return;
    }

    std::ostream &os = *detectionOstream_;
    os << "[mssanitizer] Start cross npu racecheck." << std::endl;
}

void CrossNpuChecker::DisplaySanitizerEnd() const
{
    if (detectionOstream_ == nullptr) {
        return;
    }

    std::ostream &os = *detectionOstream_;
    if (errorCount_ > 0) {
        os << "[mssanitizer] Cross npu racecheck finished. See all detected errors above." << std::endl;
    } else {
        os << "[mssanitizer] Cross npu racecheck finished. No error detected." << std::endl;
    }
}

void CrossNpuChecker::UpdateLoc(std::vector<SanEvent> &events, uint32_t deviceIdx, uint32_t kernelIdx, uint32_t deviceId)
{
    for (auto &event : events) {
        event.loc.deviceIdx = deviceIdx;
        event.loc.kernelIdx = kernelIdx;
        event.loc.deviceId = deviceId;
    }
}

bool CrossNpuChecker::CheckFilter(DeviceInfoSummary const &deviceInfo, KernelSummary const &kernelSummary) const
{
    // 静态插桩默认处理
    if (!kernelSummary.isKernelWithDBI) {
        return true;
    }

    // Ascend95 已全部改为动态插桩，都要处理
    if (IsAscend95(deviceInfo.device)) {
        return true;
    }

    return false;
}

} // namespace Sanitizer
