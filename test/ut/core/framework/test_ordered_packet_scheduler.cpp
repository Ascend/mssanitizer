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

#include <gtest/gtest.h>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <future>
#include <vector>

#include "ordered_packet_scheduler.h"
#include "runtime_context.h"
#include "peer_access_manager.h"
#include "address_sanitizer/address_sanitizer.h"

using namespace Sanitizer;

TEST(OrderedPacketScheduler, ReordersTwoConnectionsWithinOneProcess) {
    Config config{};
    Checker checkerA(config, false);
    Checker checkerB(config, false);
    std::mutex mutex;
    std::vector<uint64_t> dispatched;
    OrderedPacketScheduler scheduler([&](OrderedPacketScheduler::Job &job) {
        if (job.packet.GetType() == PacketType::DEVICE_SUMMARY) {
            RuntimeContext::Instance().deviceSummary_ = job.packet.GetPayload().deviceSummary;
        }
        std::lock_guard<std::mutex> lock(mutex);
        dispatched.push_back(job.packet.GetOrderId());
    }, [](const OrderedPacketScheduler::Job &job) {
        return job.checker->CaptureMemcheckCompletion();
    });
    DeviceInfoSummary summaryA{};
    DeviceInfoSummary summaryB{};
    summaryA.deviceId = 0;
    summaryB.deviceId = 1;
    Packet second(summaryB);
    second.SetOrderId(2);
    Packet first(summaryA);
    first.SetOrderId(1);
    scheduler.Submit({123, std::thread::id{}, &checkerB, std::move(second), {}});
    scheduler.Submit({123, std::this_thread::get_id(), &checkerA, std::move(first), {}});
    scheduler.Finish();
    EXPECT_EQ(dispatched, (std::vector<uint64_t>{1, 2}));
}

TEST(OrderedPacketScheduler, WaitsForRelatedMemcheckCompletion) {
    Config config{};
    Checker checkerA(config, false);
    Checker checkerB(config, false);
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<uint64_t> dispatched;
    std::promise<void> allocationDone;
    std::promise<void> accessDone;
    auto allocationFuture = allocationDone.get_future().share();
    auto accessFuture = accessDone.get_future().share();
    OrderedPacketScheduler scheduler(
        [&](OrderedPacketScheduler::Job &job) {
            if (job.packet.GetType() == PacketType::DEVICE_SUMMARY) {
                RuntimeContext::Instance().deviceSummary_ = job.packet.GetPayload().deviceSummary;
            }
            {
                std::lock_guard<std::mutex> lock(mutex);
                dispatched.push_back(job.packet.GetOrderId());
            }
            cv.notify_all();
        },
        [&](const OrderedPacketScheduler::Job &job) {
            if (job.packet.GetOrderId() == 2) {
                return allocationFuture;
            }
            if (job.packet.GetOrderId() == 5) {
                return accessFuture;
            }
            std::promise<void> ready;
            ready.set_value();
            return ready.get_future().share();
        });

    const auto connectionA = std::thread::id{};
    const auto connectionB = std::this_thread::get_id();
    DeviceInfoSummary summaryA{};
    summaryA.deviceId = 0;
    DeviceInfoSummary summaryB{};
    summaryB.deviceId = 1;
    HostMemRecord allocation{};
    allocation.type = MemOpType::MALLOC;
    HostMemRecord access{};
    access.type = MemOpType::LOAD;
    HostMemRecord release{};
    release.type = MemOpType::FREE;
    auto submit = [&](uint64_t id, std::thread::id connection, Checker &checker, Packet packet) {
        packet.SetOrderId(id);
        scheduler.Submit({456, connection, &checker, std::move(packet), {}});
    };
    submit(1, connectionA, checkerA, Packet(summaryA));
    submit(2, connectionA, checkerA, Packet(allocation));
    submit(3, connectionB, checkerB, Packet(summaryB));
    submit(4, connectionB, checkerB, Packet(PeerAccessRecord{1, 0, PeerAccessOp::ENABLE}));
    submit(5, connectionB, checkerB, Packet(access));
    submit(6, connectionA, checkerA, Packet(release));

    bool reachedFour;
    {
        std::unique_lock<std::mutex> lock(mutex);
        reachedFour = cv.wait_for(lock, std::chrono::seconds(3), [&] { return dispatched.size() >= 4; });
        EXPECT_EQ(dispatched, (std::vector<uint64_t>{1, 2, 3, 4}));
    }
    allocationDone.set_value();
    bool reachedFive;
    {
        std::unique_lock<std::mutex> lock(mutex);
        reachedFive = cv.wait_for(lock, std::chrono::seconds(3), [&] { return dispatched.size() >= 5; });
        EXPECT_EQ(dispatched, (std::vector<uint64_t>{1, 2, 3, 4, 5}));
    }
    accessDone.set_value();
    scheduler.Finish();
    EXPECT_TRUE(reachedFour);
    EXPECT_TRUE(reachedFive);
    EXPECT_EQ(dispatched, (std::vector<uint64_t>{1, 2, 3, 4, 5, 6}));
}

// 只回归连接上下文与回复归属，不为元数据添加本设备检查完成等待
TEST(OrderedPacketScheduler, ContextAndRepliesStayWithOriginalConnection) {
    Config config{};
    Checker checker(config, false);
    std::vector<std::pair<int32_t, std::string>> replies;
    OrderedPacketScheduler scheduler([&](OrderedPacketScheduler::Job &job) {
        auto &context = RuntimeContext::Instance();
        if (job.packet.GetType() == PacketType::DEVICE_SUMMARY) {
            context.deviceSummary_ = job.packet.GetPayload().deviceSummary;
        }
        EXPECT_EQ(context.pid_, 888);
        EXPECT_EQ(context.rootTid_, job.connection);
        if (job.packet.GetType() == PacketType::KERNEL_BINARY) {
            EXPECT_EQ(
                std::string(job.packet.GetPayload().binary.buf, job.packet.GetPayload().binary.len), "kernel-elf");
        }
        job.reply(std::to_string(context.GetDeviceId()) + ":" + std::to_string(job.packet.GetOrderId()));
    }, [](const OrderedPacketScheduler::Job &job) {
        return job.checker->CaptureMemcheckCompletion();
    });
    auto submit = [&](uint64_t id, int device, Packet packet) {
        packet.SetOrderId(id);
        auto connection = device == 0 ? std::this_thread::get_id() : std::thread::id{};
        scheduler.Submit({888, connection, &checker, std::move(packet), [&](std::string message) {
                              // 被调用时PID分发线程串行执行，无需额外发送线程或测试锁。
                              replies.emplace_back(RuntimeContext::Instance().GetDeviceId(), std::move(message));
                          }});
    };
    DeviceInfoSummary a{}, b{};
    a.deviceId = 0;
    b.deviceId = 1;
    submit(1, 0, Packet(a));
    submit(2, 1, Packet(b));
    HostMemRecord access{};
    access.type = MemOpType::LOAD;
    submit(3, 0, Packet(access));
    submit(4, 0, Packet(PacketType::KERNEL_BINARY, "kernel-elf"));
    submit(5, 1, Packet(KernelSummary{}));
    scheduler.Finish();
    EXPECT_EQ(replies,
        (std::vector<std::pair<int32_t, std::string>>{{0, "0:1"}, {1, "1:2"}, {0, "0:3"}, {0, "0:4"}, {1, "1:5"}}));
}

// 覆盖被移除的等待：元数据、告警、登记、无效关系变化及目标的纯访问均不阻塞旧访问
class OrderedNonConflictTest : public testing::TestWithParam<int> {};
TEST_P(OrderedNonConflictTest, DispatchesWithoutWaitingForUnrelatedWork) {
    Config config{};
    Checker checker(config, false);
    std::promise<void> release, dispatched;
    auto pending = release.get_future().share();
    auto arrived = dispatched.get_future();
    OrderedPacketScheduler scheduler(
        [&](OrderedPacketScheduler::Job &job) {
            if (job.packet.GetType() == PacketType::DEVICE_SUMMARY) {
                RuntimeContext::Instance().deviceSummary_ = job.packet.GetPayload().deviceSummary;
            }
            if (job.packet.GetOrderId() == 5) {
                dispatched.set_value();
            }
        },
        [&](const OrderedPacketScheduler::Job &job) {
            if (job.packet.GetOrderId() == 4) {
                return pending;
            }
            std::promise<void> ready;
            ready.set_value();
            return ready.get_future().share();
        });
    auto submit = [&](uint64_t id, int device, Packet packet) {
        packet.SetOrderId(id);
        scheduler.Submit(
            {999, device == 0 ? std::this_thread::get_id() : std::thread::id{}, &checker, std::move(packet), {}});
    };
    DeviceInfoSummary a{}, b{};
    a.deviceId = 0;
    b.deviceId = 1;
    submit(1, 0, Packet(a));
    submit(2, 1, Packet(b));
    submit(3, 0, Packet(PeerAccessRecord{0, 1, PeerAccessOp::ENABLE}));
    HostMemRecord load{};
    load.type = MemOpType::LOAD;
    submit(4, 0, Packet(load));
    switch (GetParam()) {
    case 0:
        submit(5, 0, Packet(a));
        break;
    case 1:
        submit(5, 0, Packet(KernelSummary{}));
        break;
    case 2:
        submit(5, 0, Packet(PacketType::KERNEL_BINARY, "elf"));
        break;
    case 3:
        submit(5, 0, Packet(GMAddrOutOfBoundRecord{}));
        break;
    case 4: {
        IPCMemRecord record{};
        record.type = IPCOperationType::SET_INFO;
        submit(5, 0, Packet(record));
        break;
    }
    case 5:
        submit(5, 0, Packet(PeerAccessRecord{0, 1, PeerAccessOp::ENABLE}));
        break;
    case 6:
        submit(5, 0, Packet(PeerAccessRecord{0, 2, PeerAccessOp::DISABLE}));
        break;
    case 7:
        submit(5, 1, Packet(PacketType::KERNEL_RECORD, "block"));
        break;
    default: {
        SanitizerRecord record{};
        record.version = GetParam() == 8 ? RecordVersion::KERNEL_RECORD : RecordVersion::MEMORY_RECORD;
        record.payload.kernelRecord.recordType = RecordType::LOAD;
        if (GetParam() == 9) {
            record.payload.memoryRecord.type = MemOpType::GM_ADDR_OUT_OF_BOUND;
        }
        submit(5, 1, Packet(record));
        break;
    }
    }
    const auto result = arrived.wait_for(std::chrono::seconds(3));
    release.set_value();
    scheduler.Finish();
    EXPECT_EQ(result, std::future_status::ready);
}
INSTANTIATE_TEST_CASE_P(NoPeerConflict, OrderedNonConflictTest, testing::Range(0, 10));

// 模拟检查队列异常丢弃完成标记：收尾报告不完整，但不能让 future 异常终止进程
TEST(OrderedPacketScheduler, BrokenCompletionAtShutdownReportsIncomplete) {
    Checker checker(Config{}, false);
    std::shared_future<void> broken;
    {
        std::promise<void> abandoned;
        broken = abandoned.get_future().share();
    }
    testing::internal::CaptureStderr();
    OrderedPacketScheduler scheduler(
        [](OrderedPacketScheduler::Job &job) {
            RuntimeContext::Instance().deviceSummary_ = job.packet.GetPayload().deviceSummary;
        },
        [&](const OrderedPacketScheduler::Job &) { return broken; });
    DeviceInfoSummary info{};
    info.deviceId = 0;
    Packet packet(info);
    packet.SetOrderId(1);
    scheduler.Submit({123, std::this_thread::get_id(), &checker, std::move(packet), {}});
    scheduler.Finish();
    EXPECT_NE(testing::internal::GetCapturedStderr().find("Detection incomplete"), std::string::npos);
}
