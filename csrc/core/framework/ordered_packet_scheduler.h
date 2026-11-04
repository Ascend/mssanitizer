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

#ifndef CORE_FRAMEWORK_ORDERED_PACKET_SCHEDULER_H
#define CORE_FRAMEWORK_ORDERED_PACKET_SCHEDULER_H

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "checker.h"
#include "process.h"
#include "protocol.h"

namespace Sanitizer {

/*
工具侧收包后的调度入口：将各连接的完整包按 Host PID 交给对应的进程队列。处理数据类型为 Job 结构体
每次 Command::Exec 创建一个对象，并由其管理每个 Host PID 的 ProcessQueue 和派发线程，业务形态为：

Command::Exec 创建 OrderedPacketScheduler
    │
    ├── PID 100 → 一个 ProcessQueue → 一条派发线程
    │                ├── 设备0的包
    │                └── 设备1的包
    │
    └── PID 200 → 一个 ProcessQueue → 一条派发线程
                     ├── 设备0的包
                     └── 设备1的包
*/
class OrderedPacketScheduler {
public:
    // 保存一个待派发的完整包及其原连接、检测对象和回复通道
    struct Job {
        pid_t pid; // 发包的 Host 进程，不是工具进程
        std::thread::id connection; // 原接收线程 ID 是连接身份，执行线程借此恢复该连接的上下文
        Checker *checker; // 原连接的检测对象；排序只移动包，不重建 Checker
        Packet packet;
        Process::MsgRspFunc reply; // 原连接的回复通道
    };
    // 执行原 msgSplitFunc 有的按包类型分发逻辑，返回时检查可能仍在异步队列中，需要靠完成通知识别任务完成
    using Dispatch = std::function<void(Job &)>;
    // 在指定 Checker 的内存检查队列尾部取得完成通知，供相关后续包等待
    using Completion = std::function<std::shared_future<void>(const Job &)>;
    // IPC 销毁可能更新多个 Checker，由原业务层提供实际受影响的对象
    using IpcUpdates = std::function<std::vector<Checker *>(const Job &)>;

    // 调用方提供分发和检查完成通知回调；各进程队列共用这些回调
    explicit OrderedPacketScheduler(Dispatch dispatch, Completion completion, IpcUpdates ipcUpdates = {});
    // 停止接收新任务并回收各进程派发线程
    ~OrderedPacketScheduler();

    // 从任意连接提交完整设备包；缺较小序号时只在ProcessQueue暂存，不阻塞接收线程
    void Submit(Job job);
    // 所有接收线程结束后调用；核对缺号并等待已派发检查完成
    void Finish();

private:
    // 执行暂存、排序和分发的内部类，每个 Host PID 对应一个队列
    class ProcessQueue;
    Dispatch dispatch_; // 实际业务分发回调，调用 Command 原有的业务分发代码
    Completion completion_; // 获取 Checker 异步检查完成通知的回调
    IpcUpdates ipcUpdates_; // 查询 IPC 销毁实际更新的 Checker
    std::mutex mutex_; // 保护进程队列的创建、查找和关闭
    std::map<pid_t, std::unique_ptr<ProcessQueue>> processesQueueMap_; // Host PID -> 该 PID 的进程队列
    bool closed_{}; // Finish 后拒绝提交新包
};

} // namespace Sanitizer

#endif
