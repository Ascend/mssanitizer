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
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <future>
#include <condition_variable>

#include "communication.h"
#include "config.h"
#include "protocol.h"
#include "utility/serializer.h"

#define private public
#include "hook_report.h"
#undef private
#include "record_defs.h"

namespace Sanitizer {
MemOpRecord CreateRecord(MemOpType type, const ReportAddrInfo &addrInfo);
}

using namespace Sanitizer;

namespace {
// 启动 CommunicationServer，客户端连接时发送 Config；收集客户端写入的消息供断言。
class HookReportServer {
public:
    explicit HookReportServer(const std::string &socketPath) : server_(socketPath) {}

    void Start() {
        server_.RegisterMsgHandler(
            [this](CommunicationServer::ClientId, std::string msg, CommunicationServer::MsgResponseFunc &rsp) {
                std::lock_guard<std::mutex> lock(mtx_);
                receivedMsgs_.push_back(msg);
                cv_.notify_all();
            });
        server_.SetClientConnectHook([this](CommunicationServer::ClientId id) {
            Config config{};
            config.cacheSize = 100;
            config.checkBlockId = -1;
            config.defaultCheck = true;
            config.memCheck = true;
            std::string configMsg(reinterpret_cast<char const *>(&config), sizeof(Config));
            server_.Write(id, configMsg);
        });
        server_.StartListen();
    }

    void Stop() { server_.Close(); }

    bool WaitForMessage(std::string &outMsg) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (!cv_.wait_for(lock, std::chrono::seconds(5), [this] { return !receivedMsgs_.empty(); })) {
            return false;
        }
        outMsg = receivedMsgs_.front();
        receivedMsgs_.erase(receivedMsgs_.begin());
        return true;
    }

private:
    CommunicationServer server_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::vector<std::string> receivedMsgs_;
};

class HookReportTest : public testing::Test {
protected:
    void SetUp() override {
        socketPath_ = "/tmp/hook_report_ut_" + std::to_string(getpid()) + ".sock";
        unlink(socketPath_.c_str());
        server_ = std::make_unique<HookReportServer>(socketPath_);
        server_->Start();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        setenv("MSOP_SOCKET_PATH", socketPath_.c_str(), 1);
        auto &hook = HookReport::Instance();
        hook.sendFailed_ = false;
        hook.client_.reset();
        hook.determined_ = false;
        hook.memInfoSrc_ = MemInfoSrc::RT;
        hook.deviceType_ = DeviceType::INVALID;
        hook.blockDim_ = 0U;
        hook.recordBuffer_.clear();
        hook.kernelInfo_ = KernelInfo{};
        hook.checkParms_ = CheckParmsInfo{};
        hook.serialNo_ = 0;
        ASSERT_TRUE(hook.InitClient());
        ASSERT_NE(hook.client_, nullptr);
    }

    void TearDown() override {
        // 先关闭 client 连接，避免 server 的 Listen 线程阻塞在 recvmsg 上导致 Close 死锁
        auto &hook = HookReport::Instance();
        hook.client_.reset();
        server_->Stop();
        unsetenv("MSOP_SOCKET_PATH");
    }

    std::string socketPath_;
    std::unique_ptr<HookReportServer> server_;
};
} // namespace

// 三个 Report* 接口走同一套成功上报流程，循环覆盖
TEST_F(HookReportTest, ReportDeviceTypeKernelInfoMemOp_expect_success) {
    auto &hook = HookReport::Instance();
    ReportAddrInfo addrInfo(0x1000, 0x100, 10, "test.cpp", MemInfoSrc::RT);
    MemOpRecord record = CreateRecord(MemOpType::MALLOC, addrInfo);

    hook.SetBlockDim(4);
    hook.SetKernelType(KernelType::AIVEC);

    EXPECT_TRUE(hook.ReportDeviceType(DeviceType::ASCEND_910B1));
    EXPECT_EQ(hook.GetDeviceType(), DeviceType::ASCEND_910B1);
    EXPECT_TRUE(hook.ReportKernelInfo());
    EXPECT_EQ(hook.GetKernelInfo().kernelType, KernelType::AIVEC);
    EXPECT_TRUE(hook.ReportMemOp(record));

    std::string received;
    uint64_t previousOrderId = 0;
    int packetCount = 0;
    while (packetCount < 3) {
        std::string msg;
        ASSERT_TRUE(server_->WaitForMessage(msg));
        received += msg;
        while (received.size() >= sizeof(PacketHead) && packetCount < 3) {
            PacketHead head{};
            std::memcpy(&head, received.data(), sizeof(head));
            size_t payloadSize = 0;
            if (head.type == PacketType::DEVICE_SUMMARY) {
                payloadSize = sizeof(DeviceInfoSummary);
            } else if (head.type == PacketType::KERNEL_SUMMARY) {
                payloadSize = sizeof(KernelSummary);
            } else {
                ASSERT_EQ(head.type, PacketType::SANITIZER_RECORD);
                payloadSize = sizeof(SanitizerRecord);
            }
            const size_t packetSize = sizeof(PacketHead) + payloadSize;
            if (received.size() < packetSize) {
                break;
            }
            EXPECT_GT(head.orderId, previousOrderId);
            if (previousOrderId != 0) {
                EXPECT_EQ(head.orderId, previousOrderId + 1);
            }
            previousOrderId = head.orderId;
            received.erase(0, packetSize);
            ++packetCount;
        }
    }
}

// 独立HookReport入口由自己的发送锁和计数器管理；普通包与binary包必须连续完整。
TEST_F(HookReportTest, ConcurrentNormalAndBinaryUseOneOrderedStream) {
    auto &hook = HookReport::Instance();
    const std::string bytes = "hook-peer-binary";
    const std::string normal =
        Serialize(PacketHead{PacketType::PEER_ACCESS_RECORD, 0, 0}, PeerAccessRecord{0, 1, PeerAccessOp::ENABLE});
    const std::string binary =
        Serialize(PacketHead{PacketType::KERNEL_BINARY, 0, 0}, static_cast<uint64_t>(bytes.size())) + bytes;
    std::promise<void> start;
    auto ready = start.get_future().share();
    std::thread first([&] {
        ready.wait();
        EXPECT_TRUE(hook.WriteOrdered(normal));
    });
    std::thread second([&] {
        ready.wait();
        EXPECT_TRUE(hook.WriteOrdered(binary));
    });
    start.set_value();
    first.join();
    second.join();
    std::string received;
    const size_t total = normal.size() + binary.size();
    while (received.size() < total) {
        std::string part;
        ASSERT_TRUE(server_->WaitForMessage(part));
        received += part;
    }
    ASSERT_EQ(received.size(), total);
    PacketHead firstHead{}, secondHead{};
    std::memcpy(&firstHead, received.data(), sizeof(firstHead));
    const size_t firstSize = firstHead.type == PacketType::KERNEL_BINARY ? binary.size() : normal.size();
    std::memcpy(&secondHead, received.data() + firstSize, sizeof(secondHead));
    EXPECT_EQ(secondHead.orderId, firstHead.orderId + 1);
    EXPECT_NE(firstHead.type, secondHead.type);
    const auto check = [&](size_t offset, const PacketHead &head) {
        if (head.type == PacketType::KERNEL_BINARY) {
            EXPECT_EQ(
                received.substr(offset + sizeof(head), binary.size() - sizeof(head)), binary.substr(sizeof(head)));
        } else {
            EXPECT_EQ(head.type, PacketType::PEER_ACCESS_RECORD);
            EXPECT_EQ(
                received.substr(offset + sizeof(head), normal.size() - sizeof(head)), normal.substr(sizeof(head)));
        }
    };
    check(0, firstHead);
    check(firstSize, secondHead);
}

TEST(HookReport, CreateRecord_valid_filename_expect_copied) {
    ReportAddrInfo addrInfo(0x1000, 0x100, 10, "test_file.cpp", MemInfoSrc::RT);
    MemOpRecord record = CreateRecord(MemOpType::LOAD, addrInfo);
    std::string fileName(record.fileName, sizeof(record.fileName));
    ASSERT_NE(fileName.find("test_file.cpp"), std::string::npos);
}

TEST(HookReport, CreateRecord_long_filename_expect_truncated) {
    std::string longName(1024, 'a');
    ReportAddrInfo addrInfo(0x1000, 0x100, 5, longName.c_str(), MemInfoSrc::BYPASS);
    MemOpRecord record = CreateRecord(MemOpType::STORE, addrInfo);
    std::string fileName(record.fileName, sizeof(record.fileName));
    ASSERT_EQ(fileName.find_first_of('\0'), sizeof(record.fileName) - 1);
}

TEST(HookReport, CreateRecord_with_invalid_chars_filename_expect_replaced) {
    ReportAddrInfo addrInfo(0x1000, 0x100, 20, "test\nfile\rname", MemInfoSrc::RT);
    MemOpRecord record = CreateRecord(MemOpType::MALLOC, addrInfo);
    std::string fileName(record.fileName, sizeof(record.fileName));
    ASSERT_EQ(fileName.find('\n'), std::string::npos);
    ASSERT_EQ(fileName.find('\r'), std::string::npos);
}

TEST(HookReport, CreateRecord_type_and_addr_expect_correct) {
    ReportAddrInfo addrInfo(0x2000, 0x200, 30, "demo.cpp", MemInfoSrc::BYPASS);
    MemOpRecord record = CreateRecord(MemOpType::FREE, addrInfo);
    ASSERT_EQ(record.type, MemOpType::FREE);
    ASSERT_EQ(record.dstAddr, 0x2000);
    ASSERT_EQ(record.memSize, 0x200);
    ASSERT_EQ(record.infoSrc, MemInfoSrc::BYPASS);
    ASSERT_EQ(record.srcAddr, 0x00);
    ASSERT_EQ(record.srcSpace, AddressSpace::GM);
    ASSERT_EQ(record.dstSpace, AddressSpace::GM);
}

TEST_F(HookReportTest, FailedWriteStopsSubsequentPackets) {
    auto &hook = HookReport::Instance();
    // 未连接的真实客户端稳定返回写失败，无需硬件或对端时序配合
    hook.client_.reset(new CommunicationClient(socketPath_));
    const auto nextOrderId = hook.nextOrderId_;
    const auto message = Serialize(PacketHead{PacketType::LOG_STRING}, uint64_t{0});
    testing::internal::CaptureStderr();
    EXPECT_FALSE(hook.WriteOrdered(message));
    EXPECT_FALSE(hook.WriteOrdered(message));
    EXPECT_NE(testing::internal::GetCapturedStderr().find("detection result is incomplete"), std::string::npos);
    EXPECT_TRUE(hook.sendFailed_);
    EXPECT_EQ(hook.nextOrderId_, nextOrderId + 1);
}
