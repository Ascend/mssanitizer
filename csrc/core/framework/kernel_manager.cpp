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

#include <mutex>

#include "utility/ustring.h"

#include "kernel_manager.h"

namespace Sanitizer {

using namespace Utility;

void KernelManager::SetDemangleMode(DemangleMode demangleMode)
{
    demangleMode_ = demangleMode;
}

std::size_t KernelManager::Add(DeviceId deviceId, KernelSummary const &kernelSummary)
{
    std::unique_lock<std::mutex> guard(mtx_);
    auto &kernels = kernelMap_[deviceId];
    std::size_t idx = kernels.size();
    kernels.push_back(kernelSummary);
    return idx;
}

bool KernelManager::GetKernelCount(DeviceId deviceId, std::size_t &kernelCount) const
{
    auto it = kernelMap_.find(deviceId);
    if (it == kernelMap_.cend()) {
        return false;
    }
    kernelCount = it->second.size();
    return true;
}

bool KernelManager::Get(DeviceId deviceId, std::size_t kernelIdx, KernelSummary &kernelSummary) const
{
    std::unique_lock<std::mutex> guard(mtx_);
    auto it = kernelMap_.find(deviceId);
    if (it == kernelMap_.cend()) {
        return false;
    }
    auto const &kernels = it->second;
    if (kernelIdx >= kernels.size()) {
        return false;
    }
    kernelSummary = kernels[kernelIdx];
    return true;
}

std::string KernelManager::GetDisplayKernelName(std::string kernelName) const
{
    if (kernelName.empty()) {
        kernelName = "unknown";
        return kernelName;
    }

    if (EndWith(kernelName, "_mix_aic") || EndWith(kernelName, "_mix_aiv")) {
        kernelName = kernelName.substr(0, kernelName.length() - 8UL);
    }

    if (demangleMode_ == DemangleMode::MANGLED_NAME) {
        return kernelName;
    }

    std::string demangled;
    if (!Demangle(kernelName, demangled)) {
        return kernelName;
    }

    std::string simplified;
    if (demangleMode_ == DemangleMode::SIMPLE_DEMANGLED_NAME) {
        if (SimplifyDemangledName(demangled, simplified)) {
            return simplified;
        }
    }

    // demangle 后的函数名中有空格，两侧增加引号保证显示清晰
    return "\"" + demangled + "\"";
}

} // namespace Sanitizer
