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

#ifndef CORE_FRAMEWORK_BINARY_BUFFER_H
#define CORE_FRAMEWORK_BINARY_BUFFER_H

#include <cstddef>
#include <memory>
#include <vector>

#include <sys/mman.h>
#include <unistd.h>

namespace Sanitizer {

/**
 * 内核二进制的只读视图，屏蔽底层存储方式：
 * - 传统字节流路径：共享 std::vector<char>
 * - fd 路径（P3）：接收端 mmap 发送方通过 SCM_RIGHTS 传来的 memfd，零拷贝
 * 下游（ElfLoader / KernelBinaryManager / CallStack）统一按 data()/size() 访问。
 */
class BinaryBuffer {
public:
    using Ptr = std::shared_ptr<const BinaryBuffer>;

    BinaryBuffer() = default;
    ~BinaryBuffer()
    {
        if (map_ != MAP_FAILED) {
            munmap(map_, mapLen_);
            map_ = MAP_FAILED;
        }
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
    }
    BinaryBuffer(BinaryBuffer const &) = delete;
    BinaryBuffer &operator=(BinaryBuffer const &) = delete;

    static Ptr FromVector(std::shared_ptr<const std::vector<char>> vec)
    {
        if (vec == nullptr) {
            return nullptr;
        }
        auto self = std::shared_ptr<BinaryBuffer>(new BinaryBuffer());
        self->vec_ = std::move(vec);
        self->data_ = self->vec_->data();
        self->size_ = self->vec_->size();
        return self;
    }

    // 接管 fd 的所有权并 mmap（MAP_PRIVATE 只读）；失败时关闭 fd 返回 nullptr
    static Ptr FromMappedFd(int fd, std::size_t len)
    {
        if (fd < 0 || len == 0) {
            if (fd >= 0) {
                close(fd);
            }
            return nullptr;
        }
        void *m = mmap(nullptr, len, PROT_READ, MAP_PRIVATE, fd, 0);
        if (m == MAP_FAILED) {
            close(fd);
            return nullptr;
        }
        auto self = std::shared_ptr<BinaryBuffer>(new BinaryBuffer());
        self->fd_ = fd;
        self->map_ = m;
        self->mapLen_ = len;
        self->data_ = static_cast<const char *>(m);
        self->size_ = len;
        return self;
    }

    const char *data() const { return data_; }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

private:
    std::shared_ptr<const std::vector<char>> vec_;
    const char *data_ = nullptr;
    std::size_t size_ = 0;
    int fd_ = -1;
    void *map_ = MAP_FAILED;
    std::size_t mapLen_ = 0;
};

} // namespace Sanitizer

#endif // CORE_FRAMEWORK_BINARY_BUFFER_H
