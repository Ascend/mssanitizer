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

#include "kernel_binary_manager.h"
#include <mutex>

namespace Sanitizer {

void KernelBinaryManager::Set(std::string const &kernelName, std::vector<char> const &binary)
{
    std::unique_lock<std::mutex> guard(mtx_);
    binaryMap_[kernelName] = binary;
}

bool KernelBinaryManager::Get(std::string const &kernelName, std::vector<char> &binary) const
{
    std::unique_lock<std::mutex> guard(mtx_);
    auto it = binaryMap_.find(kernelName);
    if (it == binaryMap_.cend()) {
        return false;
    }
    binary = it->second;
    return true;
}

} // namespace Sanitizer