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


#include "protocol.h"

#include <cstdint>

#include "record_defs.h"
#include "record_format.h"
#include "utility/cpp_future.h"
#include "utility/log.h"
#include "utility/serializer.h"

namespace Sanitizer {

class MemCheckProtocol::Extractor {
public:
    Extractor() = default;
    inline Extractor(Extractor const &rhs);
    ~Extractor() = default;
    inline Extractor &operator=(Extractor rhs);
    inline uint64_t Size(void) const;
    template<typename T>
    inline bool Read(T &val);
    inline bool Read(uint64_t size, std::string &buffer);
    inline void Feed(const std::string &msg);
    inline void Swap(Extractor &rhs);

private:
    static constexpr uint64_t BYTES_DROP_THRESHOLD = 1024UL;
    static constexpr uint64_t MAX_STREAM_LEN = 100UL * 1024UL * 1024UL * 1024UL;
    inline void DropUsedBytes(void);

private:
    std::string bytes_;
    std::string::size_type offset_{0UL};
    std::mutex mutex_;
};

MemCheckProtocol::Extractor::Extractor(Extractor const &rhs)
    : bytes_(rhs.bytes_), offset_{rhs.offset_} { }

MemCheckProtocol::Extractor &MemCheckProtocol::Extractor::operator=(Extractor rhs)
{
    Swap(rhs);
    return *this;
}

uint64_t MemCheckProtocol::Extractor::Size(void) const
{
    return bytes_.size() - offset_;
}

template <typename T>
bool MemCheckProtocol::Extractor::Read(T &val)
{
    std::string buffer;
    if (!Read(sizeof(T), buffer)) {
        return false;
    }
    return Deserialize<T>(buffer, val);
}

bool MemCheckProtocol::Extractor::Read(uint64_t size, std::string &buffer)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (offset_ + size > bytes_.size()) {
        return false;
    }
    buffer = bytes_.substr(offset_, size);
    offset_ += size;

    if (offset_ > BYTES_DROP_THRESHOLD) {
        DropUsedBytes();
    }
    return true;
}

void MemCheckProtocol::Extractor::Feed(const std::string &msg)
{
    std::lock_guard<std::mutex> guard(mutex_);
    // 限制协议解析流的最大长度
    if (bytes_.size() + msg.size() < MAX_STREAM_LEN) {
        bytes_ += msg;
    }
}

void MemCheckProtocol::Extractor::Swap(Extractor &rhs)
{
    std::lock_guard<std::mutex> guard(mutex_);
    std::swap(bytes_, rhs.bytes_);
    std::swap(offset_, rhs.offset_);
}

void MemCheckProtocol::Extractor::DropUsedBytes(void)
{
    offset_ = std::min(offset_, bytes_.size());
    bytes_ = bytes_.substr(offset_);
    offset_ = 0UL;
}

MemCheckProtocol::MemCheckProtocol()
{
    extractor_ = std::make_shared<Extractor>();
}

template <typename TriviallyCopyableT>
Packet MemCheckProtocol::GetTriviallyCopyable(void)
{
    TriviallyCopyableT value;
    if (!extractor_->Read(value)) {
        return Packet();
    }
    return Packet(value);
}

template <typename BinaryDataT>
Packet MemCheckProtocol::GetBinaryDataPacket(void)
{
    std::string buffer;
    return GetBinaryData(buffer) ? Packet(BinaryDataT{buffer}) : Packet();
}

bool MemCheckProtocol::GetPacketHead(PacketHead &head) const
{
    return extractor_->Read(head);
}

bool MemCheckProtocol::GetBinaryData(std::string &data)
{
    thread_local static uint64_t size{};
    if (size == 0UL) {
        if (!extractor_->Read(size)) {
            return false;
        }
    }
    if (extractor_->Size() < size) {
        return false;
    }
    if (!extractor_->Read(size, data)) {
        return false;
    }
    size = 0UL;
    return true;
}

Packet MemCheckProtocol::GetPayLoad(PacketHead head)
{
    // 根据包头类型解包
    switch (head.type) {
        case PacketType::DEVICE_SUMMARY:
            return GetTriviallyCopyable<DeviceInfoSummary>();
        case PacketType::KERNEL_SUMMARY:
            return GetTriviallyCopyable<KernelSummary>();
        case PacketType::MEMORY_RECORD:
            return GetTriviallyCopyable<HostMemRecord>();
        case PacketType::KERNEL_RECORD:
            return GetBinaryDataPacket<Packet::KernelRecord>();
        case PacketType::IPC_RECORD:
            return GetTriviallyCopyable<IPCMemRecord>();
        case PacketType::SANITIZER_RECORD:
            return GetTriviallyCopyable<SanitizerRecord>();
        case PacketType::MEM_REGION_PERMISSION:
            return GetTriviallyCopyable<MemRegionPermissionDesc>();
        case PacketType::KERNEL_BINARY:
            return GetBinaryDataPacket<Packet::KernelBinary>();
        case PacketType::LOG_STRING:
            return GetBinaryDataPacket<Packet::LogString>();
        case PacketType::GM_ADDR_OUT_OF_BOUND_RECORD:
            return GetTriviallyCopyable<GMAddrOutOfBoundRecord>();
        case PacketType::INVALID:
        default:
            return Packet{};
    }
}

void MemCheckProtocol::Feed(std::string const &msg)
{
    if (msg.size() != 0U) {
        extractor_->Feed(msg);
    }
}

Packet MemCheckProtocol::GetPacket(void)
{
    // 接收包头，并根据包头把RawBytes转换为对应的结构体
    thread_local static PacketHead head{PacketType::INVALID};
    if (head.type == PacketType::INVALID) {
        if (!GetPacketHead(head)) {
            return Packet{};
        }
    }

    Packet packet = GetPayLoad(head);
    // 如果 Packet 解析成功则重置 head，下一次接收到数据后解析包头
    if (packet.GetType() != PacketType::INVALID) {
        head.type = PacketType::INVALID;
    }

    return packet;
}

} // namespace Sanitizer
