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

#ifndef __TEST_UTILITY_FD_TEST_UTIL_H__
#define __TEST_UTILITY_FD_TEST_UTIL_H__

#include <fcntl.h>
#include <unistd.h>

#include <cstdlib>
#include <string>

#include <sys/types.h>

namespace TestUtility {

// 创建一个内容为 payload、可被 mmap 的只读 fd，用于验证 BinaryBuffer::FromMappedFd 等
// 零拷贝路径；失败返回 -1。使用 mkstemp 以兼容不支持 memfd_create 的旧工具链。
inline int CreatePayloadFd(const std::string &payload) {
    char pathTemplate[] = "/tmp/msan_fd_test_XXXXXX";
    int fd = mkstemp(pathTemplate);
    if (fd < 0) {
        return -1;
    }
    // 立即 unlink：fd 仍然有效，close 后文件自动回收，避免残留临时文件
    unlink(pathTemplate);
    if (ftruncate(fd, static_cast<off_t>(payload.size())) != 0) {
        close(fd);
        return -1;
    }
    if (!payload.empty()) {
        ssize_t written = pwrite(fd, payload.data(), payload.size(), 0);
        if (written != static_cast<ssize_t>(payload.size())) {
            close(fd);
            return -1;
        }
    }
    return fd;
}

} // namespace TestUtility

#endif // __TEST_UTILITY_FD_TEST_UTIL_H__
