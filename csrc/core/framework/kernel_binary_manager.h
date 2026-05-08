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

#ifndef CORE_FRAMEWORK_KERNEL_BINARY_MANAGER_H
#define CORE_FRAMEWORK_KERNEL_BINARY_MANAGER_H

#include <mutex>
#include <unordered_map>
#include <vector>

#include "utility/singleton.h"

namespace Sanitizer {

/**
 * 管理所有算子二进制文件
 * 限制：需要假定所有二进制中 kernelName 都是唯一，采用 kernelName 到二进制文件的
 * 映射关系对二进制进行管理
 */
class KernelBinaryManager : public Singleton<KernelBinaryManager> {
public:
    friend class Singleton<KernelBinaryManager>;

    /**
     * @brief 记录二进制文件
     * @param kernelName [in] 要记录的二进制文件对应的 kernelName
     * @param binary [in] 要记录的二进制文件数据
     */
    void Set(std::string const &kernelName, std::vector<char> const &binary);

    /**
     * @brief 获取二进制文件
     * @param kernelName [in] 要查询的二进制文件对应的 kernelName
     * @param binary [out] 返回二进制数据
     * @return true 获取成功
     *         false 获取失败
     */
    bool Get(std::string const &kernelName, std::vector<char> &binary) const;

#if defined(__BUILD_TESTS__)
    void Clear()
    {
        binaryMap_.clear();
    }
#endif

private:
    std::unordered_map<std::string, std::vector<char>> binaryMap_;
    mutable std::mutex mtx_;
};

} // namespace Sanitizer

#endif // CORE_FRAMEWORK_KERNEL_MANAGER_H