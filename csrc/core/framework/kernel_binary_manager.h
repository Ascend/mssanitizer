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

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "core/framework/binary_buffer.h"
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

    // 持有二进制视图（vector 或 mmap'd fd），Set/Get 全程零拷贝
    using BinaryPtr = BinaryBuffer::Ptr;

    /**
     * @brief 记录二进制文件
     * @param kernelName [in] 要记录的二进制文件对应的 kernelName
     * @param binary [in] 要记录的二进制文件数据（共享所有权，不拷贝）
     */
    void Set(std::string const &kernelName, BinaryPtr binary);

    /**
     * @brief 获取二进制文件
     * @param kernelName [in] 要查询的二进制文件对应的 kernelName
     * @param binary [out] 返回二进制数据的共享指针，不拷贝
     * @return true 获取成功
     *         false 获取失败
     */
    bool Get(std::string const &kernelName, BinaryPtr &binary) const;

#if defined(__BUILD_TESTS__)
    void Clear()
    {
        binaryMap_.clear();
    }
#endif

private:
    std::unordered_map<std::string, BinaryPtr> binaryMap_;
    mutable std::mutex mtx_;
};

} // namespace Sanitizer

#endif // CORE_FRAMEWORK_KERNEL_MANAGER_H
