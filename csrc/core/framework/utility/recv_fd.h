/* -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * ------------------------------------------------------------------------- */

#ifndef CORE_UTILITY_RECV_FD_H
#define CORE_UTILITY_RECV_FD_H

#include <deque>

namespace Sanitizer {

// 接收端在本线程内收到的 SCM_RIGHTS fd 队列。
// server 在 Read（recvmsg）时入队，协议层在解析 KERNEL_BINARY 时出队；
// 二者运行在同一线程，故使用 thread_local 无需加锁。
inline std::deque<int> &RecvFdQueue()
{
    static thread_local std::deque<int> queue;
    return queue;
}

inline void PushRecvFd(int fd)
{
    RecvFdQueue().push_back(fd);
}

inline int PopRecvFd()
{
    auto &queue = RecvFdQueue();
    if (queue.empty()) {
        return -1;
    }
    int fd = queue.front();
    queue.pop_front();
    return fd;
}

} // namespace Sanitizer

#endif // CORE_UTILITY_RECV_FD_H
