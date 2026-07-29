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

#include "protocol.h"
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
