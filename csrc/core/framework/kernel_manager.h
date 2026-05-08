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

#ifndef CORE_FRAMEWORK_KERNEL_MANAGER_H
#define CORE_FRAMEWORK_KERNEL_MANAGER_H

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "config.h"
#include "record_defs.h"
#include "utility/singleton.h"

namespace Sanitizer {

/**
 * 记录被检测程序中的算子执行序，并提供查询能力
 */
class KernelManager : public Singleton<KernelManager> {
public:
    friend class Singleton<KernelManager>;

    using DeviceId = uint32_t;

    /**
     * @breif 设置函数名 demangle 模式
     * @param demangleMode [in] 函数名 demangle 模式
     */
    void SetDemangleMode(DemangleMode demangleMode);

    /**
     * @brief 向指定 device 追加 kernel 信息记录，并返回序号
     * @param deviceId [in] 记录 kernel 的设备 id
     * @param kernelSummary [in] 记录 kernel 的信息
     * @return 返回当前设备上记录的 kernel 信息序号
     */
    std::size_t Add(DeviceId deviceId, KernelSummary const &kernelSummary);

    /**
     * @brief 获取指定设备上记录的 kernel 数量
     * @param deviceId [in] 要获取的设备 id
     * @param kernelCount [out] 获取到的 kernel 数量
     * @return true 获取成功
     *         false 获取失败
     */
    bool GetKernelCount(DeviceId deviceId, std::size_t &kernelCount) const;

    /**
     * @brief 获取指定 device 指定序号的 kernel 信息
     * @param deviceId [in] 要获取的设备 id
     * @param kernelIdx [in] 要获取的 kernel 序号
     * @param kernelSummary [out] 获取到的 kernel 信息
     * @return true 获取成功
     *         false 获取失败
     */
    bool Get(DeviceId deviceId, std::size_t kernelIdx, KernelSummary &kernelSummary) const;

    /**
     * @brief 获取用于显示的 kernelName
     * @param kernelName [in] mangled kernelName
     * @return 根据 demangleMode 生成的用于显示的 kernelName
     */
    std::string GetDisplayKernelName(std::string kernelName) const;

#if defined(__BUILD_TESTS__)
    void Clear()
    {
        kernelMap_.clear();
    }
#endif

private:
    std::unordered_map<DeviceId, std::vector<KernelSummary>> kernelMap_;
    mutable std::mutex mtx_;
    DemangleMode demangleMode_{};
};

} // namespace Sanitizer

#endif // CORE_FRAMEWORK_KERNEL_MANAGER_H