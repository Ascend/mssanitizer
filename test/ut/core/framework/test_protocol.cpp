/* -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
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
#include <thread>

#include "fd_test_util.h"
#include "protocol.h"
#include "utility/elf_loader.h"
#include "utility/recv_fd.h"
#include "utility/serializer.h"

namespace Sanitizer {

inline bool operator==(MemOpRecord const &lhs, MemOpRecord const &rhs)
{
    constexpr std::size_t fileNameLen = sizeof(MemOpRecord::fileName);
    return
        lhs.type == rhs.type &&
        lhs.coreId == rhs.coreId &&
        lhs.moduleId == rhs.moduleId &&
        lhs.srcAddr == rhs.srcAddr &&
        lhs.dstAddr == rhs.dstAddr &&
        lhs.srcSpace == rhs.srcSpace &&
        lhs.dstSpace == rhs.dstSpace &&
        lhs.memSize == rhs.memSize &&
        lhs.lineNo == rhs.lineNo &&
        std::string(lhs.fileName, fileNameLen) == std::string(rhs.fileName, fileNameLen);
}

inline bool operator==(HostMemRecord const &lhs, HostMemRecord const &rhs)
{
    return
        lhs.type == rhs.type &&
        lhs.infoSrc == rhs.infoSrc &&
        lhs.srcAddr == rhs.srcAddr &&
        lhs.dstAddr == rhs.dstAddr &&
        lhs.memSize == rhs.memSize;
}

}  // namespace Sanitizer

using namespace Sanitizer;

TEST(Protocol, peer_access_record_round_trip) {
    PacketHead head{};
    head.type = PacketType::PEER_ACCESS_RECORD;
    PeerAccessRecord record{2, 3, PeerAccessOp::ENABLE};
    MemCheckProtocol protocol;
    protocol.Feed(Serialize(head));
    EXPECT_EQ(protocol.GetPacket().GetType(), PacketType::INVALID);
    protocol.Feed(Serialize(record));
    Packet packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::PEER_ACCESS_RECORD);
    EXPECT_EQ(packet.GetPayload().peerAccessRecord.deviceId, 2);
    EXPECT_EQ(packet.GetPayload().peerAccessRecord.peerDeviceId, 3);
    EXPECT_EQ(packet.GetPayload().peerAccessRecord.operation, PeerAccessOp::ENABLE);
}

TEST(Protocol, ordered_packet_survives_partial_header_and_copy) {
    PacketHead head{PacketType::PEER_ACCESS_RECORD, 0, 42};
    PeerAccessRecord record{2, 3, PeerAccessOp::ENABLE};
    std::string bytes = Serialize(head, record);
    MemCheckProtocol protocol;
    for (size_t i = 0; i + 1 < bytes.size(); ++i) {
        protocol.Feed(bytes.substr(i, 1));
        EXPECT_EQ(protocol.GetPacket().GetType(), PacketType::INVALID);
    }
    protocol.Feed(bytes.substr(bytes.size() - 1));
    Packet packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::PEER_ACCESS_RECORD);
    EXPECT_EQ(packet.GetOrderId(), 42);
    Packet copied = packet;
    EXPECT_EQ(copied.GetOrderId(), 42);
    EXPECT_EQ(copied.GetPayload().peerAccessRecord.peerDeviceId, 3);
}

TEST(Protocol, kernel_record_binary_keeps_order_after_move) {
    MemCheckProtocol protocol;
    const std::string records("abc", 3);
    protocol.Feed(
        Serialize(PacketHead{PacketType::KERNEL_RECORD, 0, 71}, static_cast<uint64_t>(records.size())) + records);
    Packet packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::KERNEL_RECORD);
    Packet moved = std::move(packet);
    EXPECT_EQ(moved.GetOrderId(), 71);
    EXPECT_EQ(std::string(moved.GetPayload().binary.buf, moved.GetPayload().binary.len), records);
}

TEST(Protocol, get_summary_from_protocol_expect_success_and_equal_to_packet)
{
    PacketHead header;
    header.type = PacketType::DEVICE_SUMMARY;
    DeviceInfoSummary summary{
        .device = DeviceType::ASCEND_910_PREMIUM_A,
        .blockSize = 1,
        .blockNum = 2,
    };

    MemCheckProtocol protocol;
    protocol.Feed(Serialize(header));
    Packet packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::INVALID);

    protocol.Feed(Serialize(summary));
    packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::DEVICE_SUMMARY);
    DeviceInfoSummary ret = packet.GetPayload().deviceSummary;
    ASSERT_EQ(summary.device, ret.device);
    ASSERT_EQ(summary.blockSize, ret.blockSize);
    ASSERT_EQ(summary.blockNum, ret.blockNum);
}

TEST(Protocol, get_mem_op_record_from_protocol_expect_success_and_equal_to_packet)
{
    PacketHead summaryHeader;
    summaryHeader.type = PacketType::DEVICE_SUMMARY;
    DeviceInfoSummary summary{
        .device = DeviceType::ASCEND_910_PREMIUM_A,
        .blockSize = 1,
        .blockNum = 2,
    };

    PacketHead recordHeader;
    recordHeader.type = PacketType::SANITIZER_RECORD;
    MemOpRecord record{};
    record.coreId = 1;
    record.srcAddr = 0x1000;
    record.dstAddr = 0x2000;
    record.srcSpace = AddressSpace::GM;
    record.dstSpace = AddressSpace::UB;
    record.memSize = 0x1000;
    record.lineNo = 19;
    strncpy(record.fileName, "test.cpp", sizeof(record.fileName) - 1);

    MemCheckProtocol protocol;
    protocol.Feed(Serialize(summaryHeader));
    Packet packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::INVALID);

    protocol.Feed(Serialize(summary));
    packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::DEVICE_SUMMARY);

    protocol.Feed(Serialize(recordHeader));
    packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::INVALID);

    SanitizerRecord sanitizerRecord;
    sanitizerRecord.version = RecordVersion::MEMORY_RECORD;
    sanitizerRecord.payload.memoryRecord = record;
    protocol.Feed(Serialize(sanitizerRecord));
    packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::SANITIZER_RECORD);
    ASSERT_EQ(packet.GetPayload().sanitizerRecord.version, RecordVersion::MEMORY_RECORD);
    MemOpRecord ret = packet.GetPayload().sanitizerRecord.payload.memoryRecord;
    ASSERT_EQ(record, ret);
}

TEST(Protocol, get_host_mem_record_from_protocol_expect_success_and_equal_to_packet)
{
    PacketHead summaryHeader;
    summaryHeader.type = PacketType::DEVICE_SUMMARY;
    DeviceInfoSummary summary{
        .device = DeviceType::ASCEND_910_PREMIUM_A,
        .blockSize = 1,
        .blockNum = 2,
    };

    PacketHead recordHeader;
    recordHeader.type = PacketType::MEMORY_RECORD;
    HostMemRecord record{
        .type = MemOpType::MALLOC,
        .infoSrc = MemInfoSrc::BYPASS,
        .infoDesc = MemInfoDesc::DEFAULT,
        .srcAddr = 0x1000,
        .dstAddr = 0x2000,
        .memSize = 0x1000,
    };

    MemCheckProtocol protocol;
    protocol.Feed(Serialize(summaryHeader));
    Packet packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::INVALID);

    protocol.Feed(Serialize(summary));
    packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::DEVICE_SUMMARY);

    protocol.Feed(Serialize(recordHeader));
    packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::INVALID);

    protocol.Feed(Serialize(record));
    packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::MEMORY_RECORD);
    HostMemRecord ret = packet.GetPayload().hostMemRecord;
    ASSERT_EQ(record, ret);
}

TEST(Packet, copy_constructor_binary_expect_deep_copy) {
    std::string binaryData = "hello_kernel_binary";
    Packet original(PacketType::KERNEL_BINARY, binaryData);
    Packet copy(original);

    ASSERT_EQ(copy.GetType(), PacketType::KERNEL_BINARY);
    ASSERT_EQ(copy.GetPayload().binary.len, original.GetPayload().binary.len);
    ASSERT_NE(copy.GetPayload().binary.buf, original.GetPayload().binary.buf);
    ASSERT_EQ(std::string(copy.GetPayload().binary.buf, copy.GetPayload().binary.len), binaryData);
}

TEST(Packet, move_constructor_binary_expect_ownership_transfer) {
    std::string binaryData = "hello_kernel_binary";
    Packet original(PacketType::KERNEL_BINARY, binaryData);
    char *originalBuf = original.GetPayload().binary.buf;

    Packet moved(std::move(original));
    ASSERT_EQ(moved.GetType(), PacketType::KERNEL_BINARY);
    ASSERT_EQ(moved.GetPayload().binary.buf, originalBuf);
    ASSERT_EQ(original.GetPayload().binary.buf, nullptr);
}

TEST(Packet, IsBinaryPacket_expect_correct_classification) {
    ASSERT_TRUE(Packet::IsBinaryPacket(PacketType::KERNEL_BINARY));
    ASSERT_TRUE(Packet::IsBinaryPacket(PacketType::LOG_STRING));
    ASSERT_TRUE(Packet::IsBinaryPacket(PacketType::KERNEL_RECORD));
    ASSERT_FALSE(Packet::IsBinaryPacket(PacketType::DEVICE_SUMMARY));
    ASSERT_FALSE(Packet::IsBinaryPacket(PacketType::KERNEL_SUMMARY));
    ASSERT_FALSE(Packet::IsBinaryPacket(PacketType::SANITIZER_RECORD));
    ASSERT_FALSE(Packet::IsBinaryPacket(PacketType::MEMORY_RECORD));
}

TEST(Protocol, get_kernel_summary_from_protocol_expect_success) {
    PacketHead header;
    header.type = PacketType::KERNEL_SUMMARY;
    KernelSummary summary{};
    summary.blockDim = 4;
    summary.kernelType = KernelType::AICPU;
    summary.pcStartAddr = 0x5000;

    MemCheckProtocol protocol;
    protocol.Feed(Serialize(header));
    Packet packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::INVALID);

    protocol.Feed(Serialize(summary));
    packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::KERNEL_SUMMARY);
    KernelSummary ret = packet.GetPayload().kernelSummary;
    ASSERT_EQ(ret.blockDim, summary.blockDim);
    ASSERT_EQ(ret.kernelType, summary.kernelType);
    ASSERT_EQ(ret.pcStartAddr, summary.pcStartAddr);
}

TEST(Protocol, get_kernel_binary_from_protocol_expect_success)
{
    PacketHead header;
    header.type = PacketType::KERNEL_BINARY;
    std::string binaryData = "hello_kernel_binary";

    MemCheckProtocol protocol;
    protocol.Feed(Serialize(header));
    Packet packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::INVALID);

    protocol.Feed(Serialize(static_cast<uint64_t>(binaryData.size())) + binaryData);
    packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::KERNEL_BINARY);
    ASSERT_EQ(packet.GetPayload().binary.len, binaryData.size());
    ASSERT_EQ(std::string(packet.GetPayload().binary.buf, packet.GetPayload().binary.len), binaryData);
}

TEST(Protocol, get_kernel_binary_from_fd_expect_zero_copy_packet)
{
    const std::string payload = "mmap-kernel-binary";
    int fd = TestUtility::CreatePayloadFd(payload);
    ASSERT_GE(fd, 0);
    // P3：模拟发送方经 SCM_RIGHTS 传来的 fd，接收端 mmap 零拷贝构造 Packet
    PushRecvFd(fd);

    PacketHead header;
    header.type = PacketType::KERNEL_BINARY;

    MemCheckProtocol protocol;
    protocol.Feed(Serialize(header, static_cast<uint64_t>(payload.size())));
    Packet packet = protocol.GetPacket();

    ASSERT_EQ(packet.GetType(), PacketType::KERNEL_BINARY);
    ASSERT_NE(packet.GetBinaryBuffer(), nullptr);
    ASSERT_EQ(packet.GetPayload().binary.len, payload.size());
    ASSERT_EQ(std::string(packet.GetPayload().binary.buf, packet.GetPayload().binary.len), payload);
}

TEST(Protocol, get_kernel_binary_with_oversized_length_expect_invalid_packet)
{
    PacketHead header;
    header.type = PacketType::KERNEL_BINARY;
    // 长度字段超过 kernel object 上限，应被安全校验拦截
    uint64_t oversize = detail::MAX_LEGAL_ELF_SEGMENT_SIZE + 1UL;

    MemCheckProtocol protocol;
    protocol.Feed(Serialize(header, oversize));
    Packet packet = protocol.GetPacket();
    ASSERT_EQ(packet.GetType(), PacketType::INVALID);
}

namespace Sanitizer {
// 在每个字节边界暂停，确认半包绝不会提交；随后粘连的普通包也应保持原编号。
TEST(Protocol, ordered_binary_and_normal_packets_at_every_byte_boundary) {
    // 与实际接收模型一致：一条连接在独立接收线程解析，避免其它异常包测试的线程状态。
    std::thread([] {
        const std::string data = "peer-binary";
        const std::string binary =
            Serialize(PacketHead{PacketType::KERNEL_BINARY, 0, 17}, static_cast<uint64_t>(data.size())) + data;
        const PeerAccessRecord relation{0, 1, PeerAccessOp::ENABLE};
        const std::string normal = Serialize(PacketHead{PacketType::PEER_ACCESS_RECORD, 0, 18}, relation);
        for (size_t split = 1; split < binary.size(); ++split) {
            SCOPED_TRACE(split);
            MemCheckProtocol protocol;
            protocol.Feed(binary.substr(0, split));
            EXPECT_EQ(protocol.GetPacket().GetType(), PacketType::INVALID);
            protocol.Feed(binary.substr(split) + normal);
            auto packet = protocol.GetPacket();
            ASSERT_EQ(packet.GetType(), PacketType::KERNEL_BINARY);
            EXPECT_EQ(packet.GetOrderId(), 17);
            EXPECT_EQ(std::string(packet.GetPayload().binary.buf, packet.GetPayload().binary.len), data);
            packet = protocol.GetPacket();
            EXPECT_EQ(packet.GetType(), PacketType::PEER_ACCESS_RECORD);
            EXPECT_EQ(packet.GetOrderId(), 18);
            EXPECT_EQ(protocol.GetPacket().GetType(), PacketType::INVALID);
        }
        MemCheckProtocol protocol;
        for (size_t i = 0; i < normal.size(); ++i) {
            protocol.Feed(normal.substr(i, 1));
            auto packet = protocol.GetPacket();
            EXPECT_EQ(packet.GetType(), i + 1 == normal.size() ? PacketType::PEER_ACCESS_RECORD : PacketType::INVALID);
            if (i + 1 == normal.size()) {
                EXPECT_EQ(packet.GetOrderId(), 18);
            }
        }
    }).join();
}
} // namespace Sanitizer

// 测试场景：P2P映射报文分段接收并复制Packet
// 测试预期：类型、顺序号、所有方身份及精确区间完整保留
TEST(Protocol, P2PMappingRoundTripAndCopy) {
    // 与真实接收模型及已有半包测试一致，一条连接使用独立接收线程
    std::thread([] {
        PacketHead head{PacketType::P2P_MAPPING_RECORD, 0, 51};
        P2PMappingRecord record{1, 2, 0x1008, 8, 0x1000};
        MemCheckProtocol protocol;
        const auto bytes = Serialize(head) + Serialize(record);
        protocol.Feed(bytes.substr(0, bytes.size() - 1));
        EXPECT_EQ(protocol.GetPacket().GetType(), PacketType::INVALID);
        protocol.Feed(bytes.substr(bytes.size() - 1));
        Packet original = protocol.GetPacket();
        Packet packet(original);
        ASSERT_EQ(packet.GetType(), PacketType::P2P_MAPPING_RECORD);
        EXPECT_EQ(packet.GetOrderId(), 51);
        EXPECT_EQ(packet.GetPayload().p2pMappingRecord.deviceId, 1);
        EXPECT_EQ(packet.GetPayload().p2pMappingRecord.ownerDeviceId, 2);
        EXPECT_EQ(packet.GetPayload().p2pMappingRecord.addr, 0x1008);
        EXPECT_EQ(packet.GetPayload().p2pMappingRecord.size, 8);
        EXPECT_EQ(packet.GetPayload().p2pMappingRecord.ownerAddr, 0x1000);
    }).join();
}
