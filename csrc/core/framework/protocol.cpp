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
#include "utility/elf_loader.h"
#include "utility/log.h"
#include "utility/recv_fd.h"
#include "utility/serializer.h"

namespace Sanitizer {

// 校验从 IPC 流中反序列化出的 PacketType 是否为已定义的合法值，
// 防止未识别的类型导致下游逻辑异常。
bool IsKnownPacketType(PacketType type) {
    switch (type) {
    case PacketType::DEVICE_SUMMARY:
    case PacketType::KERNEL_SUMMARY:
    case PacketType::KERNEL_BINARY:
    case PacketType::LOG_STRING:
    case PacketType::MEMORY_RECORD:
    case PacketType::KERNEL_RECORD:
    case PacketType::IPC_RECORD:
    case PacketType::MEM_REGION_PERMISSION:
    case PacketType::SANITIZER_RECORD:
    case PacketType::GM_ADDR_OUT_OF_BOUND_RECORD:
    case PacketType::IPC_RESPONSE:
    case PacketType::KERNEL_RECORD_RESPONSE:
        return true;
    default:
        return false;
    }
}

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
    inline void Reserve(uint64_t payloadSize);
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
    if (offset_ == 0UL && size == bytes_.size()) {
        // 剩余缓冲恰好就是本包载荷，直接 move 交出，避免 substr 的整块拷贝
        buffer = std::move(bytes_);
        bytes_.clear();
        offset_ = 0UL;
        return true;
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

void MemCheckProtocol::Extractor::Reserve(uint64_t payloadSize)
{
    std::lock_guard<std::mutex> guard(mutex_);
    // 已知即将到达的大包长度，一次性预留容量，避免累积期反复扩容（重分配 + 拷贝 + 页清零）
    uint64_t needed = static_cast<uint64_t>(bytes_.size()) + payloadSize;
    if (needed > bytes_.capacity()) {
        bytes_.reserve(needed);
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
    if (!GetBinaryData(buffer)) {
        return Packet();
    }
    // move 接管，避免再对 GB 级二进制做一次拷贝
    return Packet(BinaryDataT{std::move(buffer)});
}

bool MemCheckProtocol::GetPacketHead(PacketHead &head) const
{
    if (!extractor_->Read(head)) {
        return false;
    }
    if (!IsKnownPacketType(head.type)) {
        SAN_WARN_LOG("Unknown packet type [%u] from IPC stream, treated as INVALID.", static_cast<uint32_t>(head.type));
        head.type = PacketType::INVALID;
    }
    return true;
}

// P3：优先消费发送方经 SCM_RIGHTS 传来的 fd，mmap 后零拷贝构造 Packet；
// 若无 fd（旧协议/内存模式/回退）则走原有字节流路径。
Packet MemCheckProtocol::GetKernelBinaryPacket(void)
{
    if (pendingBinaryFd_ < 0) {
        pendingBinaryFd_ = PopRecvFd();
    }
    if (pendingBinaryFd_ < 0) {
        return GetBinaryDataPacket<Packet::KernelBinary>();
    }
    uint64_t length = 0;
    if (!extractor_->Read(length)) {
        // 长度尚未到达：保留 fd，等待后续数据
        return Packet();
    }
    BinaryBuffer::Ptr buffer = BinaryBuffer::FromMappedFd(pendingBinaryFd_, length);
    pendingBinaryFd_ = -1;
    if (buffer == nullptr) {
        SAN_ERROR_LOG("Failed to mmap kernel binary fd (length: %lu).", length);
        return Packet();
    }
    SAN_INFO_LOG("Kernel binary received via fd, mmap %lu bytes (zero-copy).", length);
    return Packet(PacketType::KERNEL_BINARY, std::move(buffer));
}

bool MemCheckProtocol::GetBinaryData(std::string &data)
{
    thread_local static uint64_t size{};
    if (size == 0UL) {
        if (!extractor_->Read(size)) {
            return false;
        }
        // 安全校验：长度字段来源于 IPC 流，可能被恶意篡改或损坏。
        // 上限取 kernel object（ELF 段）允许的最大尺寸，防止超大长度触发超量内存分配。
        if (size > detail::MAX_LEGAL_ELF_SEGMENT_SIZE) {
            SAN_ERROR_LOG("Binary data length (%lu) exceeds the max kernel object size (%lu), drop this packet.",
                size, detail::MAX_LEGAL_ELF_SEGMENT_SIZE);
            size = 0UL;
            return false;
        }
        if (size > 0UL) {
            extractor_->Reserve(size);
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

Packet MemCheckProtocol::GetPayLoad(PacketHead head) {
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
            return GetKernelBinaryPacket();
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
