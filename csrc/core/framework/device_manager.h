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

#ifndef CORE_FRAMEWORK_DEVICE_MANAGER_H
#define CORE_FRAMEWORK_DEVICE_MANAGER_H

#include <cstdint>
#include <mutex>
#include <unordered_map>

#include "record_defs.h"
#include "utility/singleton.h"

namespace Sanitizer {

/**
 * 记录程序中使用的设备信息，并提供查询能力
 */
class DeviceManager : public Singleton<DeviceManager> {
public:
    friend class Singleton<DeviceManager>;

    using DeviceId = uint32_t;

    /**
     * @brief 记录设备信息
     * @param deviceId [in] 要记录的设备 ID，与 SetDevice 接口中设置的 ID 保持一致
     * @param deviceInfo [in] 要记录的设备信息
     */
    void Set(DeviceId deviceId, DeviceInfoSummary const &deviceInfo);

    /**
     * @brief 获取设备信息
     * @param deviceId [in] 要获取信息的设备 ID
     * @param deviceInfo [out] 返回设备信息
     * @return true 获取成功
     *         false 获取失败
     */
    bool Get(DeviceId deviceId, DeviceInfoSummary &deviceInfo) const;

    /**
     * @brief 获取已记录的设备数量
     * @return 返回已记录的设备数量
     */
    std::size_t GetDeviceCount() const { return deviceMap_.size(); }

    /**
     * @brief 获取已记录的设备 ID 列表
     * @return 返回已记录的设备 ID 列表，并从小到大排序
     */
    std::vector<DeviceId> GetDeviceList() const;

#if defined(__BUILD_TESTS__)
    void Clear()
    {
        deviceMap_.clear();
    }
#endif

private:
    std::unordered_map<DeviceId, DeviceInfoSummary> deviceMap_;
    mutable std::mutex mtx_;
};

} // namespace Sanitizer

#endif // CORE_FRAMEWORK_DEVICE_MANAGER_H