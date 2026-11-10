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

#include "ordered_packet_scheduler.h"

#include <condition_variable>
#include <future>
#include <iostream>
#include <set>
#include <utility>

#include "runtime_context.h"
#include "utility/log.h"

namespace Sanitizer {

// 汇集一个 Host 进程各连接的完整包，由专属派发线程按编号派发并等待相关检查完成
// 用 shared_future 保存检查完成通知；后续相关包等待该通知就绪后才分发
class OrderedPacketScheduler::ProcessQueue {
public:
    // 一个 Host PID 对应一条派发线程，协调其设备连接之间的顺序和依赖；不同 PID 独立推进
    ProcessQueue(pid_t pid, Dispatch &dispatch, Completion &completion, IpcUpdates &ipcUpdates)
        : pid_(pid), dispatch_(dispatch), completion_(completion), ipcUpdates_(ipcUpdates),
          worker_(&ProcessQueue::Run, this) {}
    // 保证队列销毁前派发线程已退出，不再访问本对象
    ~ProcessQueue() { Finish(); }

    // 收包线程调用，把完整的业务包入队
    void Submit(Job job) {
        std::lock_guard<std::mutex> lock(mutex_);
        // 取出之前登记的包序号
        const uint64_t id = job.packet.GetOrderId();
        // 拒绝非法、重复编号
        if (id == 0 || id < nextOrderId_ || pendingJobMap_.count(id) != 0) {
            Fail("invalid or duplicate orderId");
            return;
        }
        // 入队，并唤醒调度线程处理入队的包，随后返回继续收包
        pendingJobMap_.emplace(id, std::move(job));
        cv_.notify_one();
    }

    // 所有接收线程结束后调用；核对缺号并等待已派发的检查完成
    void Finish() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closing_ = true;
        }
        // 通知派发线程收尾：派发编号连续的剩余包，遇到缺号则报告不完整并退出
        cv_.notify_one();
        // 回收调度线程
        if (worker_.joinable()) {
            worker_.join();
        }
    }

private:
    // 保存本设备访问、状态更新及全部工作的检查完成点，供后续相关包等待
    // 完成标记被该设备的内存检查消费线程处理后通知就绪；派发函数返回不代表检查完成
    struct DeviceWorkCheckPoints {
        // IPC 销毁可能把释放记录交给另一设备，用对象及原连接定位它的完成点
        Checker *checker{};
        std::thread::id connection;
        std::shared_future<void> lastAccess; // 最近一次可能查询远端地址的检查完成通知
        std::shared_future<void> lastUpdate; // 最近一次改变本设备可查询状态的检查完成通知
        std::shared_future<void> lastWork; // 最近一次派发工作的完成通知，用于结束时排空检查
    };

    // 调度线程工作循环；按同 PID 的连续编号取包；缺号一直等到接收侧结束再报告不完整
    void Run() {
        while (true) {
            Job job{};
            {
                std::unique_lock<std::mutex> lock(mutex_);
                // 只有期望编号已在队列中，或已开始收尾，才结束等待；其它编号到达不会放行
                cv_.wait(lock, [this] { return closing_ || pendingJobMap_.count(nextOrderId_) != 0; });
                // 正常运行时这里必有期望编号；只有收尾仍缺该编号时才结束循环
                auto it = pendingJobMap_.find(nextOrderId_);
                if (it == pendingJobMap_.end()) {
                    break;
                }
                job = std::move(it->second);
                pendingJobMap_.erase(it);
                // 序号前移
                ++nextOrderId_;
            }
            try {
                // 此处 Job 已按期望编号取出；等待相关检查并执行分发
                Execute(job);
            } catch (const std::exception &error) {
                Fail(error.what());
            }
        }
        // 收尾前等待各设备此前已派发的内存检查完成
        for (const auto &item : devices_) {
            try {
                Wait(item.second.lastWork);
            } catch (const std::exception &error) {
                // 异常完成点同样说明检查不完整，不能让异常逃出派发线程终止工具
                Fail(error.what());
            }
        }
        if (!pendingJobMap_.empty()) {
            Fail("missing orderId or unconsumed packet");
        }
    }

    // 同一 Host PID 内的业务依赖规则，以下 A -> B 表示设备 A 获准访问设备 B
    // 1. A 访问 B：等 B 的 lastUpdate，确保此前的范围、权限或内存范围表选择更新已完成
    // 2. B 更新状态：等所有获准访问 B 的设备的 lastAccess，避免改变仍被旧访问查询的状态
    //    更新包括 MALLOC/FREE、权限变更、IPC 映射变更，以及非 BYPASS Host 访问引起的表选择变化
    // 3. A 开启授权：授权实际变化时等 A 的 lastAccess，防止新授权放行此前的非法访问
    // 4. A 撤销授权：授权实际变化时等 A 的 lastAccess，防止旧的合法访问因提前撤权而误报
    // 5. RESET：等本设备此前的远端访问和其它设备此前对本设备的访问，再撤销两个方向的授权
    // 6. IPC DESTROY：对实际接收 FREE 的目标设备执行规则 2，并更新这些目标的完成通知
    // 建立依赖的核心是：访问之前，等目标此前的状态更新完成；更新之前，等此前查询这个目标的访问完成
    // 等待对象均来自已派发的旧工作；完成标记被检查消费线程处理后才就绪，而非分发返回即完成
    // 缺号等待由 Run 负责，收尾等待各设备 lastWork；二者不属于本方法的跨设备业务依赖
    void Execute(Job &job) {
        // 恢复 RuntimeContext
        auto &context = rtContexts_[job.connection];
        context.rootTid_ = job.connection;
        context.pid_ = pid_;
        RuntimeContext::Instance() = context;

        const Packet &packet = job.packet;
        const PacketType type = packet.GetType();
        // 日志不参与设备依赖，直接沿用原业务分发
        if (type == PacketType::LOG_STRING) {
            dispatch_(job);
            context = RuntimeContext::Instance();
            return;
        }

        // 从设备摘要中获取本设备 ID；其它包从 RuntimeContext 中获取
        const int32_t device =
            type == PacketType::DEVICE_SUMMARY ? packet.GetPayload().deviceSummary.deviceId : context.GetDeviceId();
        if (device < 0) {
            Fail("invalid deviceId");
            return;
        }

        // 保存本设备的 Checker 和原连接身份，供后续 IPC DESTROY 的 FREE 等待
        auto &ownCheckPoints = devices_[device];
        ownCheckPoints.checker = job.checker;
        ownCheckPoints.connection = job.connection;

        // 标记当前包是否会查询远端状态、是否会改变本设备可查询状态；两者可同时成立
        bool access = false;
        bool update = false;
        Classify(packet, access, update);

        // IPC DESTROY 的 FREE 发往映射使用方，不能归到发包设备的完成点上
        std::set<int32_t> ipcTargets;
        if (type == PacketType::IPC_RECORD && packet.GetPayload().ipcMemRecord.type == IPCOperationType::DESTROY_INFO &&
            ipcUpdates_) {
            for (Checker *checker : ipcUpdates_(job)) {
                for (const auto &item : devices_) {
                    if (item.second.checker == checker) {
                        ipcTargets.insert(item.first);
                    }
                }
            }
        }

        // 精确映射会改变访问方的判定，并查询所有方分配；等待真正消费完成再修改
        if (type == PacketType::P2P_MAPPING_RECORD) {
            const auto &record = packet.GetPayload().p2pMappingRecord;
            if (record.deviceId != device || record.ownerDeviceId < 0 || record.ownerDeviceId == device ||
                record.addr == 0 || record.size == 0 || record.addr > UINT64_MAX - record.size ||
                record.ownerAddr > record.addr) {
                Fail("invalid P2P mapping record");
                return;
            }
            Wait(ownCheckPoints.lastWork);
            Wait(devices_[record.ownerDeviceId].lastUpdate);
            // ApplyMapping 在 dispatch_ 中同步完成；后续所有方更新派发时登记已结束
        } else if (type == PacketType::PEER_ACCESS_RECORD) {
            // 处理peeraccess报文，并更新本设备的访问关系
            const auto &record = packet.GetPayload().peerAccessRecord;
            const PeerAccessOp edgeOp = record.operation;
            // 合法性检查，记录设备id须匹配当前连接所属的设备；ENABLE/DISABLE 指向另一有效设备，RESET 的目标须为 -1
            if (record.deviceId != device ||
                ((edgeOp == PeerAccessOp::ENABLE || edgeOp == PeerAccessOp::DISABLE) &&
                    (record.peerDeviceId < 0 || record.peerDeviceId == device)) ||
                (edgeOp == PeerAccessOp::RESET && record.peerDeviceId != -1) ||
                (edgeOp != PeerAccessOp::ENABLE && edgeOp != PeerAccessOp::DISABLE && edgeOp != PeerAccessOp::RESET)) {
                Fail("invalid peer access record");
                return;
            }
            if (edgeOp == PeerAccessOp::RESET) {
                // RESET 撤销双向授权前，等待本设备此前的远端访问和其它获授权设备此前对本设备的访问检查完成
                if (!relations_[device].empty() || !mappedPeers_[device].empty()) {
                    Wait(ownCheckPoints.lastAccess);
                }
                WaitForReaders(device);
            } else {
                const bool enabled = relations_[device].count(record.peerDeviceId) != 0;
                // 授权实际变化时，等待本设备此前的访问检查完成，避免新授权影响旧访问；重复授权或撤权不等待
                if ((edgeOp == PeerAccessOp::ENABLE && !enabled) ||
                    (edgeOp == PeerAccessOp::DISABLE && (enabled || mappedPeers_[device].count(record.peerDeviceId)))) {
                    Wait(ownCheckPoints.lastAccess);
                }
            }
        } else {
            // 其余报文按访问和更新类型等待相关检查完成
            if (access) {
                for (int32_t target : relations_[device]) {
                    Wait(devices_[target].lastUpdate);
                }
                for (int32_t target : mappedPeers_[device]) {
                    Wait(devices_[target].lastUpdate);
                }
            }
            if (update) {
                WaitForReaders(device);
            }
            for (int32_t target : ipcTargets) {
                WaitForReaders(target);
            }
        }
        // 当前包依赖的此前检查已完成，调用原业务分发；检查工作仍可能异步入队
        SAN_INFO_LOG("Ordered packet dispatch: pid=%d device=%d orderId=%lu type=%u",
            pid_, device, packet.GetOrderId(), static_cast<uint32_t>(type));
        dispatch_(job);
        // 保存分发后变化的连接上下文，供下次处理该连接的包时恢复
        context = RuntimeContext::Instance();

        // 更新调度器的授权关系，供后续包识别依赖；真正的地址查询使用 PeerAccessManager
        if (type == PacketType::P2P_MAPPING_RECORD) {
            mappedPeers_[device].insert(packet.GetPayload().p2pMappingRecord.ownerDeviceId);
        } else if (type == PacketType::PEER_ACCESS_RECORD) {
            const auto &record = packet.GetPayload().peerAccessRecord;
            if (record.operation == PeerAccessOp::ENABLE) {
                relations_[device].insert(record.peerDeviceId);
            } else if (record.operation == PeerAccessOp::DISABLE) {
                relations_[device].erase(record.peerDeviceId);
                mappedPeers_[device].erase(record.peerDeviceId);
            } else if (record.operation == PeerAccessOp::RESET) {
                mappedPeers_.erase(device);
                for (auto &relation : mappedPeers_) {
                    relation.second.erase(device);
                }
                relations_.erase(device);
                for (auto &relation : relations_) {
                    relation.second.erase(device);
                }
            }
        }

        // 保存本包的检查完成通知，供后续相关包等待；IPC DESTROY 的 FREE 也要更新目标设备的完成点
        // 消费线程处理到完成标记时通知才就绪，表示标记前的检查已完成
        auto done = completion_(job);
        ownCheckPoints.lastWork = done;
        if (access) {
            ownCheckPoints.lastAccess = done;
        }
        if (update) {
            ownCheckPoints.lastUpdate = done;
        }
        for (int32_t target : ipcTargets) {
            auto &work = devices_[target];
            // 在实际收到 FREE 的队列尾部取完成点，供随后访问该映射的设备等待
            auto targetJob = job;
            targetJob.checker = work.checker;
            targetJob.connection = work.connection;
            work.lastUpdate = target == device ? done : completion_(targetJob);
            work.lastWork = work.lastUpdate;
        }
    }

    // 标记访问与状态更新：既考虑查询远端范围，也考虑切换后续远端查询使用的本地内存范围表
    static void ClassifyMemory(MemOpType type, MemInfoSrc source, bool &access, bool &update) {
        // load、store、memcpy_block 都可能查询远端地址
        access = type == MemOpType::LOAD || type == MemOpType::STORE || type == MemOpType::MEMCPY_BLOCKS;
        // MALLOC/FREE 更新范围；非 BYPASS 的访问可能切换 Runtime/DFX 表，影响后续远端查询
        update = type == MemOpType::MALLOC || type == MemOpType::FREE || (access && source != MemInfoSrc::BYPASS);
    }

    // 只分类会补查远端或改变 GM 查询结果的记录
    static void Classify(const Packet &packet, bool &access, bool &update) {
        switch (packet.GetType()) {
        case PacketType::MEMORY_RECORD: {
            const auto &record = packet.GetPayload().hostMemRecord;
            ClassifyMemory(record.type, record.infoSrc, access, update);
            break;
        }
        case PacketType::KERNEL_RECORD:
            // 原始 block 尚未解析，按可能包含远端访问的一批记录保护，不标记为状态更新
            access = true;
            break;
        case PacketType::SANITIZER_RECORD: {
            const auto &record = packet.GetPayload().sanitizerRecord;
            if (record.version == RecordVersion::MEMORY_RECORD) {
                ClassifyMemory(record.payload.memoryRecord.type, record.payload.memoryRecord.infoSrc, access, update);
            } else {
                access = record.version == RecordVersion::KERNEL_RECORD &&
                    record.payload.kernelRecord.recordType != RecordType::ONLINE_ERROR &&
                    record.payload.kernelRecord.recordType != RecordType::BLOCK_FINISH &&
                    record.payload.kernelRecord.recordType != RecordType::KERNEL_FINISH;
                update = record.version == RecordVersion::REGION_PERMISSION;
            }
            break;
        }
        case PacketType::DEVICE_SUMMARY:
        case PacketType::MEM_REGION_PERMISSION:
            update = true;
            break;
        case PacketType::IPC_RECORD:
            update = packet.GetPayload().ipcMemRecord.type == IPCOperationType::MAP_INFO ||
                packet.GetPayload().ipcMemRecord.type == IPCOperationType::UNMAP_INFO;
            break;
        default:
            break;
        }
    }

    // 等待某个 Checker 内部 FIFO 标记真正被消费，而不是等待 Do 入队返回
    static void Wait(const std::shared_future<void> &future) {
        if (future.valid()) {
            future.get(); // 等待消费线程通过 promise 通知完成，保证标记前的检查已完成
        }
    }

    // 更新目标设备状态前，等待所有获准访问它的来源设备此前的访问检查完成
    void WaitForReaders(int32_t device) {
        for (const auto &relation : mappedPeers_) {
            if (relation.second.count(device) != 0) {
                Wait(devices_[relation.first].lastAccess);
            }
        }
        for (const auto &relation : relations_) {
            if (relation.second.count(device) != 0) {
                Wait(devices_[relation.first].lastAccess);
            }
        }
    }

    // 已建立过映射的设备对保守保留依赖直到撤权/reset；只影响等待，不授予地址权限
    std::map<int32_t, std::set<int32_t>> mappedPeers_;

    // 同时写工具日志与可见的检测不完整提示
    void Fail(const char *reason) {
        SAN_ERROR_LOG("Incomplete ordered detection for PID %d: %s.", pid_, reason);
        std::cerr << "[mssanitizer] ERROR: Detection incomplete for PID " << pid_ << ": " << reason << std::endl;
    }

    pid_t pid_; // 本队列对应的 Host PID；devices_ 和 relations_ 只记录该 PID 的设备连接及授权

    // 各进程队列共享外层的三项回调，通过 Job 区分连接和 Checker；外层在所有队列结束后才销毁
    Dispatch &dispatch_; // 调用业务分发
    Completion &completion_; // 取得 Checker 的异步检查完成通知
    IpcUpdates &ipcUpdates_; // 查询 IPC 销毁影响的其它设备队列

    std::mutex mutex_; // 保护接收线程与派发线程共享的 pendingJobMap_、nextOrderId_ 和 closing_
    std::map<uint64_t, Job> pendingJobMap_; // 队列，保存已收齐但尚未派发的包，以进程内 orderId 为key
    uint64_t nextOrderId_{1}; // 下一个必须派发的编号；较大编号先到时暂存到 pendingJobMap_ 里等待
    bool closing_{}; // 不再有新输入，派发连续包后检查缺号并退出

    std::condition_variable cv_; // 条件等待变量，下一个编号到达或开始收尾时唤醒派发线程

    std::map<std::thread::id, RuntimeContext> rtContexts_; // 派发线程会轮流处理多个连接，按原连接保存各自的 RuntimeContext
    std::map<int32_t, DeviceWorkCheckPoints> devices_; // 各设备的 Checker、连接和检查完成通知
    std::map<int32_t, std::set<int32_t>> relations_; // 有向授权访存关系：源设备 ID -> 获准访问的目标设备 ID
    // rtContexts_、devices_ 和 relations_ 只由该派发线程读写

    // 线程成员须最后初始化，避免 Run 在编号和关闭标志初始化前读取它们
    std::thread worker_; // 首次收到本 PID 的包时创建，Finish 时 join
};

// 保存调用方提供的回调，供该调度器管理的各进程队列共同使用
OrderedPacketScheduler::OrderedPacketScheduler(Dispatch dispatch, Completion completion, IpcUpdates ipcUpdates)
    : dispatch_(std::move(dispatch)), completion_(std::move(completion)), ipcUpdates_(std::move(ipcUpdates)) {}
// 析构时兜底回收所有进程派发线程
OrderedPacketScheduler::~OrderedPacketScheduler() { Finish(); }

// 按 Host PID 找到该进程的 ProcessQueue，保存包及其编号
void OrderedPacketScheduler::Submit(Job job) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        SAN_ERROR_LOG("Packet received after ordered scheduler closed.");
        return;
    }
    auto &queue = processesQueueMap_[job.pid];
    if (!queue) {
        // 首次遇到该 PID 时创建队列和派发线程，再提交包
        queue.reset(new ProcessQueue(job.pid, dispatch_, completion_, ipcUpdates_));
    }
    queue->Submit(std::move(job));
}

// 接收线程结束后调用，关闭所有进程队列，并等待其派发及已提交的检查结束
void OrderedPacketScheduler::Finish() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) {
        return;
    }
    closed_ = true;
    for (auto &item : processesQueueMap_) {
        item.second->Finish();
    }
}

} // namespace Sanitizer
