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

#include <algorithm>

#include "device_manager.h"

namespace Sanitizer {

void DeviceManager::Set(DeviceId deviceId, DeviceInfoSummary const &deviceInfo)
{
    deviceMap_[deviceId] = deviceInfo;
}

bool DeviceManager::Get(DeviceId deviceId, DeviceInfoSummary &deviceInfo) const
{
    auto it = deviceMap_.find(deviceId);
    if (it == deviceMap_.cend()) {
        return false;
    }
    deviceInfo = it->second;
    return true;
}

std::vector<DeviceManager::DeviceId> DeviceManager::GetDeviceList() const
{
    std::vector<DeviceId> deviceList;
    deviceList.reserve(deviceMap_.size());
    for (auto const &p : deviceMap_) {
        deviceList.emplace_back(p.first);
    }
    std::sort(deviceList.begin(), deviceList.end());
    return deviceList;
}

} // namespace Sanitizer