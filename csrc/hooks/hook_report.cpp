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


#include "hook_report.h"

#include <cstdlib>
#include <memory>
#include <sstream>
#include <algorithm>
#include <unistd.h>

#include "config.h"
#include "protocol.h"
#include "record_defs.h"
#include "platform_config.h"
#include "utility/serializer.h"
#include "utility/ustring.h"
#include "vallina_symbol.h"

namespace Sanitizer {

MemOpRecord CreateRecord(MemOpType type, const ReportAddrInfo &addrInfo)
{
    auto record = MemOpRecord {};
    record.coreId = -1; // do not care
    record.moduleId = -1; // do not care
    record.srcAddr = 0x00;
    record.srcSpace = AddressSpace::GM;
    record.dstAddr = addrInfo.addr;
    record.dstSpace = AddressSpace::GM;
    record.type = type;
    record.memSize = addrInfo.size;
    record.lineNo = static_cast<int32_t>(addrInfo.lineNo);
    record.pc = 0x00;
    record.infoSrc = addrInfo.infoSrc;
    auto fileName = addrInfo.fileName;
    if (fileName == nullptr) {
        fileName = "<unknown>";
    }
    std::string validFileName = Utility::ReplaceInvalidChar(fileName);
    /// 为防止文件名过长导致拷贝失败，限制拷贝长度
    std::size_t length = std::min(validFileName.length(), sizeof(record.fileName) - 1);
    validFileName.copy(record.fileName, length);
    record.fileName[length] = '\0';
    return record;
}

inline bool ReadSockPathFromEnv(std::string &socketPath)
{
    char const *env = getenv("MSSANITIZER_PID");
    socketPath = (env == nullptr) ? "" : std::string(env);
    if (socketPath.empty()) {
        return false;
    }
    return true;
}

HookReport &HookReport::Instance(void)
{
    static HookReport ins;
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    if (ins.client_ == nullptr) {
        ins.InitClient();
    }
    return ins;
}

HookReport::HookReport(void)
    : pcStartAddr{}, client_(nullptr), determined_{false}, memInfoSrc_{MemInfoSrc::RT},
    deviceType_{DeviceType::INVALID} { }

bool HookReport::InitClient(void)
{
    if (client_) {
        return true;
    }

    std::string socketPath;
    if (!ReadSockPathFromEnv(socketPath)) {
        std::cout << "Read main process pid FAILED" << std::endl;
        return false;
    }

    client_.reset(new CommunicationClient(socketPath));
    if (client_ == nullptr) {
        return false;
    }
    Result result = client_->ConnectToServer();
    if (result.Fail()) {
        std::cout << "Client connect to server FAILED. "
                  << result.GetDescription() << std::endl;
        client_.reset(nullptr);
        return false;
    }

    /// Recieve enable mode information
    std::string msg;
    constexpr uint32_t maxCount = 10u;
    for (uint32_t i = 0u; i < maxCount; ++i) {
        if (msg.size() >= sizeof(Config)) {
            break;
        }
        std::string buffer;
        result = client_->Read(buffer);
        if (!result.Fail()) {
            msg += buffer;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100U));
    }

    if (msg.size() < sizeof(Config)) {
        std::cout << "Get device info FAILED" << std::endl;
        std::cout << msg << std::endl;
        client_.reset(nullptr);
        return false;
    }
    Config config{};
    std::copy_n(msg.data(), sizeof(Config), static_cast<char *>(static_cast<void *>(&config)));
    checkParms_.cacheSize = config.cacheSize;
    checkParms_.checkBlockId = config.checkBlockId;
    checkParms_.defaultcheck = config.defaultCheck;
    checkParms_.memcheck = config.memCheck;
    checkParms_.racecheck = config.raceCheck;
    checkParms_.initcheck = config.initCheck;
    checkParms_.synccheck = config.syncCheck;
    checkParms_.registerCheck = config.registerCheck;
    checkParms_.gmBufferGuardSize = config.gmBufferGuardSize;
    return true;
}

bool HookReport::ReportDeviceType(DeviceType deviceType)
{
    /// Send device type head
    if (!client_) {
        std::cout << "Report device type FAILED due to uninitialized client." << std::endl;
        return false;
    }
    deviceType_ = deviceType;
    PacketHead head {PacketType::DEVICE_SUMMARY};
    auto summary = DeviceInfoSummary {};
    summary.device = deviceType;
    Result result = client_->Write(Serialize(head, summary));
    if (result.Fail()) {
        std::cout << "Report device type FAILED" << std::endl;
    }
    return !result.Fail();
}

bool HookReport::ReportKernelInfo()
{
    if (!client_) {
        std::cout << "Report device info FAILED due to uninitialized client." << std::endl;
        return false;
    }
    PacketHead head {PacketType::KERNEL_SUMMARY};
    auto summary = KernelSummary {};
    summary.blockDim = blockDim_;
    summary.kernelType = kernelInfo_.kernelType;
    Result result = client_->Write(Serialize(head, summary));
    if (result.Fail()) {
        std::cout << "Report kernel info FAILED" << std::endl;
    }
    return !result.Fail();
}

bool HookReport::ReportMalloc(const ReportAddrInfo &addrInfo, int32_t moduleId)
{
    MemOpRecord record = CreateRecord(MemOpType::MALLOC, addrInfo);
    record.moduleId = moduleId;
    return ReportMemOp(record);
}

bool HookReport::ReportFree(const ReportAddrInfo &addrInfo)
{
    MemOpRecord record = CreateRecord(MemOpType::FREE, addrInfo);
    return ReportMemOp(record);
}

bool HookReport::ReportMemset(const ReportAddrInfo &addrInfo)
{
    MemOpRecord record = CreateRecord(MemOpType::STORE, addrInfo);
    return ReportMemOp(record);
}

bool HookReport::ReportMemcpy(const ReportAddrInfo &addrInfo, uint64_t srcAddr)
{
    MemOpRecord record = CreateRecord(MemOpType::MEMCPY_BLOCKS, addrInfo);
    record.srcAddr = srcAddr;
    return ReportMemOp(record);
}

bool HookReport::ReportLoad(const ReportAddrInfo &addrInfo)
{
    MemOpRecord record = CreateRecord(MemOpType::LOAD, addrInfo);
    return ReportMemOp(record);
}

bool HookReport::ReportStore(const ReportAddrInfo &addrInfo)
{
    MemOpRecord record = CreateRecord(MemOpType::STORE, addrInfo);
    return ReportMemOp(record);
}

bool HookReport::ReportMemOp(MemOpRecord const &record)
{
    if (client_ == nullptr) {
        std::cout << "Client is NULL" << std::endl;
        return false;
    }

    PacketHead head {PacketType::SANITIZER_RECORD};
    auto sanitizerRecord = SanitizerRecord {};
    sanitizerRecord.version = RecordVersion::MEMORY_RECORD;
    sanitizerRecord.payload.memoryRecord = record;
    sanitizerRecord.payload.memoryRecord.serialNo = serialNo_++;
    Result result = client_->Write(Serialize(head, sanitizerRecord));
    return !result.Fail();
}

bool HookReport::Report(SanitizerRecord &sanitizerRecord)
{
    if (client_ == nullptr) {
        std::cout << "Client is NULL" << std::endl;
        return false;
    }
    if (sanitizerRecord.version == RecordVersion::KERNEL_RECORD) {
        sanitizerRecord.payload.kernelRecord.serialNo = serialNo_++;
    } else if (sanitizerRecord.version == RecordVersion::MEMORY_RECORD) {
        sanitizerRecord.payload.memoryRecord.serialNo = serialNo_++;
    }
    PacketHead head {PacketType::SANITIZER_RECORD};
    Result result = client_->Write(Serialize(head, sanitizerRecord));
    return !result.Fail();
}

bool HookReport::ReportKernelBinary(std::vector<char> const &binary)
{
    if (client_ == nullptr) {
        std::cout << "Client is NULL" << std::endl;
        return false;
    }

    PacketHead head {PacketType::KERNEL_BINARY};
    std::string buffer = Serialize<PacketHead, uint64_t>(head, binary.size());
    buffer += std::string(binary.data(), binary.size());
    Result result = client_->Write(buffer);
    return !result.Fail();
}

bool HookReport::ReportLogString(std::string const &logString)
{
    if (client_ == nullptr) {
        std::cout << "Client is NULL" << std::endl;
        return false;
    }

    PacketHead head {PacketType::LOG_STRING};
    std::string buffer = Serialize<PacketHead, uint64_t>(head, logString.size());
    buffer += logString;
    Result result = client_->Write(buffer);
    return !result.Fail();
}

bool HookReport::ReportMalloc(uint64_t addr, uint64_t size, MemInfoSrc memInfoSrc)
{
    ReportAddrInfo addrInfo{addr, size, memInfoSrc};
    /// 如果当前来源已确定，则直接上报
    if (determined_) {
        if (memInfoSrc == memInfoSrc_) {
            return ReportMalloc(addrInfo);
        } else {
            return true;
        }
    }

    /// 根据优先级规则更新当前采用的 MemInfoFrom
    memInfoSrc_ = std::max(memInfoSrc_, memInfoSrc);

    /// 当前来源未确定则将记录缓存
    MemOpRecord record = CreateRecord(MemOpType::MALLOC, addrInfo);
    recordBuffer_.emplace_back(memInfoSrc, record);
    return true;
}

bool HookReport::ReportFree(uint64_t addr, MemInfoSrc memInfoSrc)
{
    ReportAddrInfo addrInfo{addr, memInfoSrc};
    /// 如果当前来源已确定，则直接上报
    if (determined_) {
        if (memInfoSrc == memInfoSrc_) {
            return ReportFree(addrInfo);
        } else {
            return true;
        }
    }

    /// 根据优先级规则更新当前采用的 MemInfoFrom
    memInfoSrc_ = std::max(memInfoSrc_, memInfoSrc);

    /// 当前来源未确定则将记录缓存
    MemOpRecord record = CreateRecord(MemOpType::FREE, addrInfo);
    recordBuffer_.emplace_back(memInfoSrc, record);
    return true;
}

bool HookReport::ReportLoad(uint64_t addr, uint64_t size, MemInfoSrc memInfoSrc)
{
    ReportAddrInfo addrInfo{addr, size, memInfoSrc};
    /// 如果当前来源已确定，则直接上报
    if (determined_) {
        if (memInfoSrc == memInfoSrc_) {
            return ReportLoad(addrInfo);
        } else {
            return true;
        }
    }

    /// 根据优先级规则更新当前采用的 MemInfoFrom
    memInfoSrc_ = std::max(memInfoSrc_, memInfoSrc);

    /// 当前来源未确定则将记录缓存
    MemOpRecord record = CreateRecord(MemOpType::LOAD, addrInfo);
    recordBuffer_.emplace_back(memInfoSrc, record);
    return true;
}

bool HookReport::ReportStore(uint64_t addr, uint64_t size, MemInfoSrc memInfoSrc)
{
    ReportAddrInfo addrInfo{addr, size, memInfoSrc};
    /// 如果当前来源已确定，则直接上报
    if (determined_) {
        if (memInfoSrc == memInfoSrc_) {
            return ReportStore(addrInfo);
        } else {
            return true;
        }
    }

    /// 根据优先级规则更新当前采用的 MemInfoFrom
    memInfoSrc_ = std::max(memInfoSrc_, memInfoSrc);

    /// 当前来源未确定则将记录缓存
    MemOpRecord record = CreateRecord(MemOpType::STORE, addrInfo);
    recordBuffer_.emplace_back(memInfoSrc, record);
    return true;
}

void HookReport::DetermineMemInfoSrc(void)
{
    determined_ = true;

    /// emit all buffered records
    for (auto const &p : recordBuffer_) {
        if (p.first == memInfoSrc_) {
            SanitizerRecord record;
            record.version = RecordVersion::MEMORY_RECORD;
            record.payload.memoryRecord = p.second;
            Report(record);
        }
    }

    recordBuffer_.clear();
}

void HookReport::SetBlockDim(uint32_t blockDim)
{
    blockDim_ = blockDim;
}

DeviceType HookReport::GetDeviceType(void)
{
    return deviceType_;
}

void HookReport::SetKernelType(KernelType kernelType)
{
    kernelInfo_.kernelType = kernelType;
}

KernelInfo HookReport::GetKernelInfo(void)
{
    return kernelInfo_;
}

CheckParmsInfo HookReport::GetCheckParms(void)
{
    return checkParms_;
}

}  // namespace Sanitizer
