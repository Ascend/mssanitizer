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


#ifndef CORE_FRAMEWORK_PROTOCOL_H
#define CORE_FRAMEWORK_PROTOCOL_H

#include <map>
#include <string>
#include <memory>
#include <cstdint>
#include <algorithm>

#include "record_defs.h"
#include "utility/result.h"
#include "utility/types.h"

namespace Sanitizer {

enum class PacketType : uint32_t {
    // 各工具通用的协议
    DEVICE_SUMMARY = 0,  // 设备信息
    KERNEL_SUMMARY,      // 算子运行时信息
    KERNEL_BINARY,       // 算子二进制
    LOG_STRING,          // 子进程日志信息

    // sanitizer 特有的协议
    HOST_RECORD = 1000,  // Host 侧内存操作记录
    KERNEL_RECORD,       // kernel 侧上报的记录块
    IPC_RECORD,          // IPC 类操作记录
    MEM_REGION_PERMISSION, // 内存权限分配
    SANITIZER_RECORD,    // 用于上报 SanitizerRecord 记录

    // 用于server向client发送的消息
    IPC_RESPONSE = 3000,
    KERNEL_RECORD_RESPONSE,

    INVALID = ~0U,
};

// for transmission
struct PacketHead {
    PacketType type;
};

class Packet {
public:
    /// 文件映射信息载荷，用于 Packet 联合体中。因此使用在联合体中因此必须是 POD
    /// 类型，生命周期与 Packet 类相同
    struct BinaryPayload {
        uint64_t len;  // 文件映射信息缓冲区长度
        char *buf;     // 文件映射信息缓冲区指针
    };

    using KernelRecord = TaggedType<std::string, struct KernelRecordTag>;
    using KernelBinary = TaggedType<std::string, struct KernelBinaryTag>;
    using LogString = TaggedType<std::string, struct LogStringTag>;

    union Payload {
        DeviceInfoSummary deviceSummary;
        KernelSummary kernelSummary;
        SanitizerRecord sanitizerRecord;
        HostMemRecord hostMemRecord;
        BinaryPayload binary;
        IPCMemRecord ipcMemRecord;
        MemRegionPermissionDesc memPermission;
    };

public:
    Packet(void) : type_{PacketType::INVALID}, payload_{} { }
    Packet(Packet const &rhs) : type_{rhs.type_}, payload_{rhs.payload_}
    {
        if (IsBinaryPacket(type_)) {
            /// 拷贝构造进行深拷贝
            uint64_t len = rhs.payload_.binary.len;
            payload_.binary.buf = new(std::nothrow) char[len];
            if (payload_.binary.buf != nullptr) {
                std::copy_n(rhs.payload_.binary.buf, len, payload_.binary.buf);
            } else {
                type_ = PacketType::INVALID;
            }
        }
    }
    Packet(Packet &&rhs) : type_{rhs.type_}, payload_{rhs.payload_}
    {
        if (IsBinaryPacket(type_)) {
            /// 所有权转移
            payload_.binary.buf = rhs.payload_.binary.buf;
            rhs.payload_.binary.buf = nullptr;
        }
    }
    explicit Packet(DeviceInfoSummary const &summary) : type_{PacketType::DEVICE_SUMMARY}
    {
        payload_.deviceSummary = summary;
    }
    explicit Packet(KernelSummary const &summary) : type_{PacketType::KERNEL_SUMMARY}
    {
        payload_.kernelSummary = summary;
    }
    explicit Packet(HostMemRecord const &record) : type_{PacketType::HOST_RECORD}
    {
        payload_.hostMemRecord = record;
    }
    explicit Packet(IPCMemRecord const &ipcMemRecord) : type_{PacketType::IPC_RECORD}
    {
        payload_.ipcMemRecord = ipcMemRecord;
    }
    explicit Packet(SanitizerRecord const &record) : type_{PacketType::SANITIZER_RECORD}
    {
        payload_.sanitizerRecord = record;
    }
    explicit Packet(MemRegionPermissionDesc const &desc) : type_{PacketType::MEM_REGION_PERMISSION}
    {
        payload_.memPermission = desc;
    }
    Packet(PacketType type, std::string const &binaryData) : type_{type}
    {
        uint64_t len = binaryData.size();
        payload_.binary.len = len;
        payload_.binary.buf = new char[len];
        binaryData.copy(payload_.binary.buf, len);
    }
    explicit Packet(KernelBinary const &kernelBinary) : Packet(PacketType::KERNEL_BINARY, kernelBinary.value) { }
    explicit Packet(LogString const &logString) : Packet(PacketType::LOG_STRING, logString.value) { }
    explicit Packet(KernelRecord const &kernelRecord) : Packet(PacketType::KERNEL_RECORD, kernelRecord.value) { }
    ~Packet(void)
    {
        if (IsBinaryPacket(type_)) {
            delete[] payload_.binary.buf;
        }
    }

    inline static bool IsBinaryPacket(PacketType type)
    {
        return type == PacketType::KERNEL_BINARY ||
            type == PacketType::LOG_STRING || type == PacketType::KERNEL_RECORD;
    }

    Packet &operator=(Packet rhs)
    {
        Swap(rhs);
        return *this;
    }

    void Swap(Packet &rhs) noexcept
    {
        std::swap(type_, rhs.type_);
        std::swap(payload_, rhs.payload_);
    }
    PacketType GetType(void) const
    {
        return type_;
    }
    Payload const &GetPayload(void) const
    {
        return payload_;
    }

private:
    PacketType type_;
    Payload payload_;
};

// Protocol类接收数据，根据协议解包
class Protocol {
public:
    Protocol() = default;
    virtual ~Protocol() = default;
    virtual void Feed(std::string const &msg) = 0;
    virtual Packet GetPacket(void) = 0;
};

/*
    MemCheckProtocol类在使能mem_check时创建，用于在内存检测模式下解析从客户端传回的数据
    +------+                   +------+
    |      | ----------------> |      |
    |server|                   |client|
    |      | <---------------- |      |
    +------+                   +------+
    server与client按照如下顺序交互：
step 1       -------传递信息头----->        告知使能模式
step 2       <------传递信息头------        回传设备信息、桩版本号、记录的blockSize等信息
step 3       <---传递host记录-------        回传host侧记录，作为单独一个block
step 4       <---传递device侧记录---        回传device侧记录，每个核的操作记录存放在一个
                                            block中，server接收到全部block后开始解析记录
step 5       <---传递free记录-------        单条传输host侧的free记录

注：
    1. step3,4中服务端按照如下协议提取记录：
       [block-malloc][记录1][记录2][...][block-core0][记录1][记录2]...[block-core1][记录1]
       [记录2]...[block-core2]...
    2. 若测试程序具有多个核函数，则每个核函数会重复一次step2~5
*/
class MemCheckProtocol : public Protocol {
public:
    MemCheckProtocol();
    ~MemCheckProtocol() override = default;
    void Feed(std::string const &msg) override;
    Packet GetPacket(void) override;

private:
    bool GetPacketHead(PacketHead &head) const;
    bool GetBinaryData(std::string &data);
    Packet GetPayLoad(PacketHead head);

    template <typename TriviallyCopyableT>
    Packet GetTriviallyCopyable(void);
    template <typename BinaryDataT>
    Packet GetBinaryDataPacket(void);

    class Extractor;
    std::shared_ptr<Extractor> extractor_;
};

enum class ResponseStatus : uint32_t { SUCCESS = 0, FAIL = 1000 };

struct IPCResponse {
    IPCOperationType type;
    ResponseStatus status;
};

struct KernelRecordResponse {
    uint32_t blockIdx;
    ResponseStatus status;
};

} // namespace Sanitizer
#endif
