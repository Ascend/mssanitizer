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


#include "command.h"
#include <string>
#include <functional>
#include <memory>
#include <sstream>
#include <experimental/filesystem>

#include "core/framework/config_manager.h"
#include "core/framework/device_manager.h"
#include "core/framework/kernel_binary_manager.h"
#include "core/framework/kernel_manager.h"
#include "cross_npu_checker.h"
#include "utility/umask_guard.h"
#include "utility/ustring.h"
#include "utility/serializer.h"
#include "utility/log.h"
#include "utility/path.h"
#include "utility/elf_loader.h"
#include "utility/file_system.h"
#include "checker.h"
#include "config.h"
#include "communication.h"
#include "protocol.h"
#include "thread_manager.h"
#include "platform_config.h"
#include "file_mapping.h"
#include "call_stack.h"
#include "record_defs.h"
#include "kernel_block.h"
#include "runtime_context.h"
#include "peer_access_manager.h"
#include "platform_config.h"
#include "record_format.h"
#include "ordered_packet_scheduler.h"

namespace Sanitizer {
SharedMemInfoMpType Command::sharedMemInfoMp{};
thread_local ShareeMemMpType Command::shareeMemInfoMp{};
}
namespace {
using namespace Sanitizer;
using namespace Utility;
namespace fs = std::experimental::filesystem;
constexpr uint32_t MAX_FILE_MAPPING_BUFF_SIZE = 1024U * 1024 * 1024; // 1GB


SanitizerRecord MemoryRecordToSanitizerRecord(HostMemRecord const &hostMemRecord)
{
    RuntimeContext &ctx = RuntimeContext::Instance();
    MemOpRecord memOpRecord(hostMemRecord);
    memOpRecord.serialNo = ++ctx.serialNo_;
    memOpRecord.coreId = -1;
    memOpRecord.moduleId = -1;
    memOpRecord.srcSpace = AddressSpace::GM;
    memOpRecord.dstSpace = AddressSpace::GM;
    memOpRecord.lineNo = 0;
    memOpRecord.side = MemOpSide::HOST;

    SanitizerRecord sanitizerRecord {};
    sanitizerRecord.version = RecordVersion::MEMORY_RECORD;
    sanitizerRecord.payload.memoryRecord = memOpRecord;
    return sanitizerRecord;
}

void HandleDeviceInfo(Checker &checker, DeviceInfoSummary const &summary)
{
    SAN_INFO_LOG("Receive device summary. device:%u, blockSize:%u, blockNum:%u, deviceId:%d",
        static_cast<uint32_t>(summary.device), summary.blockSize, summary.blockNum, summary.deviceId);
    DeviceManager::Instance().Set(summary.deviceId, summary);
    RuntimeContext::Instance().deviceSummary_ = summary;
    checker.SetDeviceInfo(summary);
}

std::string GetDisplayKernelName(KernelSummary const &kernelSummary, DemangleMode demangleMode)
{
    std::string kernelName(kernelSummary.kernelName);
    if (kernelName.empty()) {
        kernelName = "unknown";
        return kernelName;
    }

    if (EndWith(kernelName, "_mix_aic") || EndWith(kernelName, "_mix_aiv")) {
        kernelName = kernelName.substr(0, kernelName.length() - 8UL);
    }

    if (demangleMode == DemangleMode::MANGLED_NAME) {
        return kernelName;
    }

    std::string demangled;
    if (!Demangle(kernelName, demangled)) {
        return kernelName;
    }

    std::string simplified;
    if (demangleMode == DemangleMode::SIMPLE_DEMANGLED_NAME) {
        if (SimplifyDemangledName(demangled, simplified)) {
            return simplified;
        }
    }

    // demangle 后的函数名中有空格，两侧增加引号保证显示清晰
    return "\"" + demangled + "\"";
}

void HandleKernelInfo(Checker &checker, CrossNpuChecker &crossNpuChecker,
                      KernelSummary const &kernelSummary, Config const &config)
{
    std::string kernelNameLog = Utility::ReplaceInvalidChar(std::string(kernelSummary.kernelName));
    SAN_INFO_LOG("Receive kernel summary. kernelName: %s, pcStartAddr:0x%lx, "
                 "blockDim:%u, kernelType:%u, isKernelWithDBI:%u",
                 kernelNameLog.c_str(), kernelSummary.pcStartAddr, kernelSummary.blockDim,
                 static_cast<uint32_t>(kernelSummary.kernelType), kernelSummary.isKernelWithDBI);

    RuntimeContext::Instance().kernelIdx_++;
    RuntimeContext::Instance().currentBlockIdx_ = 0;
    // 当前连接的跨block解码状态在收包线程上复位，不影响同线程处理的其他设备，
    // 解析已移出该线程，故 ResetAll 不能依赖解析分发路径调用。
    KernelBlock::ResetAll();
    RuntimeContext::Instance().kernelSummary_ = kernelSummary;
    RuntimeContext::Instance().kernelNameDisplay = GetDisplayKernelName(kernelSummary, config.demangleMode);
    KernelManager::Instance().Add(RuntimeContext::Instance().GetDeviceId(), kernelSummary);
    checker.SetKernelInfo(kernelSummary);
    if (auto *records = crossNpuChecker.GetRecordArrayForCollection(RuntimeContext::Instance().GetDeviceId())) {
        records->CreateNewKernel();
    }
}

void HandleHostMemRecord(Checker &checker, HostMemRecord const &record)
{
    checker.Do(MemoryRecordToSanitizerRecord(record));
}

void HandleGMAddrOutOfBoundRecord(Checker &checker, GMAddrOutOfBoundRecord const &record)
{
    RuntimeContext &ctx = RuntimeContext::Instance();
    MemOpRecord memOpRecord{};
    memOpRecord.type = MemOpType::GM_ADDR_OUT_OF_BOUND;
    memOpRecord.serialNo = ++ctx.serialNo_;
    memOpRecord.coreId = -1;
    memOpRecord.moduleId = -1;
    memOpRecord.srcSpace = AddressSpace::GM;
    memOpRecord.dstSpace = AddressSpace::GM;
    memOpRecord.lineNo = 0;
    memOpRecord.dstAddr = record.outAddr;
    memOpRecord.memSize = record.outSize;
    memOpRecord.gmAddrOutOfBoundsRecord = record;

    SanitizerRecord sanitizerRecord {};
    sanitizerRecord.version = RecordVersion::MEMORY_RECORD;
    sanitizerRecord.payload.memoryRecord = memOpRecord;

    checker.Do(sanitizerRecord);
}

// IPC 注册表在不同 PID 的派发线程间共享，记录处理和调度器查询受影响 Checker 时共用此锁
std::mutex &GetIpcMutex() {
    static std::mutex mutex;
    return mutex;
}

std::string ProcessIPCSetEvent(IPCMemRecord const &record, std::mutex &mux)
{
    std::string setInfoNameLog = Utility::FormatNameForLog(std::string(record.setInfo.name));
    std::lock_guard<std::mutex> lk(mux);
    IPCResponse resp{IPCOperationType::SET_INFO, ResponseStatus::FAIL};
    auto it = Command::sharedMemInfoMp.find(record.setInfo.name);
    if (it != Command::sharedMemInfoMp.end()) {
        SAN_WARN_LOG("Error occurs when adding shared memory to container, as its name (%s) has already existed.",
                     setInfoNameLog.c_str());
    } else {
        Command::sharedMemInfoMp.insert(
            {record.setInfo.name, {SharerMemInfo{record.setInfo.addr, record.setInfo.size}, {}}});
        resp.status = ResponseStatus::SUCCESS;
        SAN_INFO_LOG("Adding shared memory to container successfully. name (%s).", setInfoNameLog.c_str());
    }
    return Serialize(PacketType::IPC_RESPONSE, resp);
}

// 销毁时向所有映射使用方的 Checker 投递 FREE，实际更新的设备可能不同于发送销毁消息的设备
std::string ProcessIPCDestroyEvent(IPCMemRecord const &record, std::mutex &mux) {
    std::lock_guard<std::mutex> lk(mux);
    IPCResponse resp{IPCOperationType::DESTROY_INFO, ResponseStatus::FAIL};
    auto it = Command::sharedMemInfoMp.find(record.destroyInfo.name);
    if (it == Command::sharedMemInfoMp.end()) {
        std::string destroyInfoNameLog = Utility::FormatNameForLog(std::string(record.destroyInfo.name));
        SAN_WARN_LOG("Error occurs when destroying shared memory, as its name (%s) does not existed.",
            destroyInfoNameLog.c_str());
    } else {
        auto &shareeMemInfoList = it->second.second;
        for (auto &p : shareeMemInfoList) {
            Checker *shareeChecker = p.first;
            auto addr = p.second.addr;
            HostMemRecord mallocRecord{MemOpType::FREE, MemInfoSrc::BYPASS, MemInfoDesc::IPC_MEMORY, 0, addr, 0};
            HandleHostMemRecord(*shareeChecker, mallocRecord);
            SAN_INFO_LOG("Freeing sharee GM: addr:0x%lx.", addr);
        }
        resp.status = ResponseStatus::SUCCESS;
        Command::sharedMemInfoMp.erase(it);
    }
    return Serialize(PacketType::IPC_RESPONSE, resp);
}

// 同一派发线程处理多个设备，显式使用原连接的 Checker，不能再按当前线程查找检测对象
std::string ProcessIPCMapEvent(IPCMemRecord const &record, Checker &checker, std::mutex &mux) {
    IPCResponse resp{IPCOperationType::MAP_INFO, ResponseStatus::FAIL};
    // thread_local 只隔离不同派发线程；rootTid_ 保存原接收线程 ID，用于区分同进程的设备连接
    auto key = std::make_pair(RuntimeContext::Instance().rootTid_, record.mapInfo.addr);
    auto it1 = Command::shareeMemInfoMp.find(key);
    if (it1 != Command::shareeMemInfoMp.end()) {
        SAN_WARN_LOG("Error occurs when opening shared memory , as its address (0x%lx) has already existed.",
            record.mapInfo.addr);
    } else {
        std::lock_guard<std::mutex> lk(mux);
        auto it2 = Command::sharedMemInfoMp.find(record.mapInfo.name);
        if (it2 == Command::sharedMemInfoMp.end()) {
            std::string mapInfoNameLog = Utility::FormatNameForLog(std::string(record.mapInfo.name));
            SAN_WARN_LOG(
                "Error occurs when opening shared memory, as its name (%s) does not existed.", mapInfoNameLog.c_str());
        } else {
            GMType size = it2->second.first.size;
            ShareeMemInfo shareeMemInfo{record.mapInfo.addr, size};
            it2->second.second.insert({&checker, std::move(shareeMemInfo)});
            Command::shareeMemInfoMp.insert({key, record.mapInfo.name});
            HostMemRecord mallocRecord{MemOpType::MALLOC, MemInfoSrc::BYPASS, MemInfoDesc::IPC_MEMORY,
                0, record.mapInfo.addr, size};
            HandleHostMemRecord(checker, mallocRecord);
            resp.status = ResponseStatus::SUCCESS;
        }
    }
    return Serialize(PacketType::IPC_RESPONSE, resp);
}

// 通过原连接和地址定位映射，并向该连接的 Checker 投递 FREE，撤销工具记录的合法范围
std::string ProcessIPCUnmapEvent(IPCMemRecord const &record, Checker &checker, std::mutex &mux) {
    IPCResponse resp{IPCOperationType::UNMAP_INFO, ResponseStatus::FAIL};
    auto key = std::make_pair(RuntimeContext::Instance().rootTid_, record.unmapInfo.addr);
    auto it = Command::shareeMemInfoMp.find(key);
    if (it == Command::shareeMemInfoMp.end()) {
        SAN_WARN_LOG("Error occurs when closing shared memory , as its addr (0x%lx) does not exist.",
            record.unmapInfo.addr);
    } else {
        std::lock_guard<std::mutex> lk(mux);
        auto itShared = Command::sharedMemInfoMp.find(it->second);
        if (itShared == Command::sharedMemInfoMp.end()) {
            std::string sharedMemoryLog = Utility::FormatNameForLog(it->second);
            SAN_INFO_LOG("Shared memory (%s) has already been destroyed.", sharedMemoryLog.c_str());
        } else {
            HostMemRecord mallocRecord{MemOpType::FREE, MemInfoSrc::BYPASS, MemInfoDesc::IPC_MEMORY,
                0, record.unmapInfo.addr, 0};
            HandleHostMemRecord(checker, mallocRecord);
            itShared->second.second.erase(&checker);
        }
        Command::shareeMemInfoMp.erase(it);
        resp.status = ResponseStatus::SUCCESS;
    }
    return Serialize(PacketType::IPC_RESPONSE, resp);
}

void HandleKernelBlock(Checker &checker, CrossNpuChecker &crossNpuChecker,
                       Packet::BinaryPayload const &payload, Process::MsgRspFunc &msgRspFunc)
{
    RuntimeContext &runtimeContext = RuntimeContext::Instance();
    auto memInfo = static_cast<uint8_t const *>(static_cast<void const *>(payload.buf));
    auto kernelBlock = KernelBlock::CreateKernelBlock(memInfo, payload.len, runtimeContext.currentBlockIdx_);
    if (kernelBlock == nullptr) {
        ++runtimeContext.currentBlockIdx_;
        return;
    }

    SanitizerRecord sanitizerRecord;
    std::vector<SanitizerRecord> records;
    const size_t recordNum = kernelBlock->GetRecordNum();
    constexpr size_t FINISH_RECORD_RESERVE = 2U;
    // 为 BLOCK_FINISH 和 KERNEL_FINISH 预留位置，避免追加结束标记时触发大批次扩容复制。
    records.reserve(recordNum + FINISH_RECORD_RESERVE);
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    auto *recordArray = crossNpuChecker.GetRecordArrayForCollection(runtimeContext.GetDeviceId());
    while (kernelBlock->NextSimd(sanitizerRecord.payload.kernelRecord)) {
        records.emplace_back(sanitizerRecord);
        if (recordArray != nullptr) {
            recordArray->Push(sanitizerRecord);
        }

        // 如果simd记录为SIMT_CALL，则当前记录后插入simt_entry动态记录，将记录执行序还原为发射序
        if (sanitizerRecord.payload.kernelRecord.recordType == RecordType::SIMT_CALL) {
            if (kernelBlock->GetRecordBlockHead().blockInfo.simtEndCount !=
                kernelBlock->GetRecordBlockHead().blockInfo.simtCallCount) {
                SAN_ERROR_LOG("parse simt entry error, simtEndCount:%u, simtCallCount:%u, serialNo:%lu",
                    kernelBlock->GetRecordBlockHead().blockInfo.simtEndCount,
                    kernelBlock->GetRecordBlockHead().blockInfo.simtCallCount, RuntimeContext::Instance().serialNo_);
            } else {
                std::vector<KernelRecord> simtEntryRecords;
                if (kernelBlock->ParseSimtEntryRecord(simtEntryRecords)) {
                    // simtEntry依赖SIMT_CALL前后的simd指令，也需要参与simd指令排序，因此加入records中
                    records.reserve(records.capacity() + simtEntryRecords.size());
                    for (const auto &record : simtEntryRecords) {
                        sanitizerRecord.payload.kernelRecord = record;
                        records.emplace_back(sanitizerRecord);
                    }
                }
            }
        }
    }

    /// 当device支持simt并且是目标核的情况下才解析simt指令，否则会内存越界
    if (checker.SupportSimt() && checker.IsTargetBlock(runtimeContext.currentBlockIdx_)) {
        std::vector<KernelRecord> kernelRecords;
        kernelBlock->ParseSimtErrorRecord(kernelRecords);
        for (const auto &record : kernelRecords) {
            sanitizerRecord.payload.kernelRecord = record;
            checker.Do(sanitizerRecord);
        }
    }

    // report block finish
    sanitizerRecord.payload.kernelRecord.recordType = RecordType::BLOCK_FINISH;
    sanitizerRecord.payload.kernelRecord.serialNo = ++runtimeContext.serialNo_;
    records.emplace_back(sanitizerRecord);
    if (recordArray != nullptr) {
        recordArray->Push(sanitizerRecord);
    }

    // wait for next block
    ++runtimeContext.currentBlockIdx_;

    SAN_INFO_LOG("Kernel block progress: device=%d block=%u total=%lu records=%lu",
                 runtimeContext.GetDeviceId(), runtimeContext.currentBlockIdx_,
                 kernelBlock->GetTotalBlockDim(), kernelBlock->GetRecordNum());
    // report finish if this is the last block
    if (runtimeContext.currentBlockIdx_ == kernelBlock->GetTotalBlockDim()) {
        KernelRecordResponse resp{runtimeContext.currentBlockIdx_, ResponseStatus::SUCCESS};
        msgRspFunc(Serialize(PacketType::KERNEL_RECORD_RESPONSE, resp));
        sanitizerRecord.payload.kernelRecord.recordType = RecordType::KERNEL_FINISH;
        sanitizerRecord.payload.kernelRecord.serialNo = ++runtimeContext.serialNo_;
        records.emplace_back(sanitizerRecord);
        if (recordArray != nullptr) {
            recordArray->Push(sanitizerRecord);
        }
        SAN_INFO_LOG("Finish processing the last kernel block.");
    }

    checker.Do(std::move(records));
}

inline void HandleKernelBinary(Packet::BinaryPayload const &payload, BinaryBuffer::Ptr buffer)
{
    // fd 路径下 buffer 已由 Packet 持有（mmap zero-copy）；字节流路径下退化为整块构造
    if (buffer == nullptr) {
        buffer = BinaryBuffer::FromVector(std::make_shared<const std::vector<char>>(
            payload.buf, payload.buf + payload.len));
    }
    if (buffer == nullptr) {
        SAN_ERROR_LOG("Empty kernel binary.");
        return;
    }

    KernelBinaryManager::Instance().Set(RuntimeContext::Instance().kernelSummary_.kernelName, buffer);

    auto loader = Sanitizer::ElfLoader();
    if (!loader.FromBuffer(buffer)) {
        SAN_INFO_LOG("Binary section load FAILED");
        return;
    }

    Sanitizer::Elf elf = loader.Load();
    std::vector<char> fileMapping = elf.ReadRawData(".init_array_sanitizer_file_mapping");
    if (fileMapping.empty()) {
        SAN_INFO_LOG("Section .init_array_sanitizer_file_mapping is empty");
        return;
    } else {
        /// 根据日志中是否有该字段判断算子是否已经插桩
        SAN_INFO_LOG("Enable sanitizer");
    }

    if (fileMapping.size() >= MAX_FILE_MAPPING_BUFF_SIZE) {
        SAN_INFO_LOG("File mapping size greater than limit.");
        return;
    }

    FileMapping::Instance().Load(fileMapping);
}

inline void HandleLogString(Packet::BinaryPayload const &payload)
{
    std::string str(payload.buf, payload.buf + payload.len);
    std::string strLog = Utility::ReplaceInvalidChar(str);
    SAN_INFO_LOG("HOOK: %s", strLog.c_str());
}

inline void HandleMemRegionPermission(Checker &checker, MemRegionPermissionDesc const &permission)
{
    SAN_INFO_LOG("MEM_REGION_PERMISSION, addr: 0x%lx, size: %lu, deviceId: %u, flags: %u",
                 permission.addr, permission.size, permission.deviceId, permission.flags);
    // handle shared memory regions
    if (permission.flags & MSTX_MEM_PERMISSIONS_REGION_FLAGS_SHARED) {
        auto &spans = DeviceManager::Instance().GetSharedMemorySpans(permission.deviceId);
        spans.Union({permission.addr, permission.addr + permission.size});
    }
    SanitizerRecord record{};
    record.version = RecordVersion::REGION_PERMISSION;
    record.payload.permissionDesc = permission;
    checker.Do(record);
}

} // namespace [Dummy]

namespace Sanitizer {

// IPC 由进程派发线程处理，checker 和回复通道均来自原连接的 Job
void HandleIpcMemRecord(Sanitizer::Checker &checker, Sanitizer::IPCMemRecord const &record, Sanitizer::ThreadManager &,
    Process::MsgRspFunc &msgRspFunc) {
    SAN_INFO_LOG("Processing IpcMemRecord type:%d.", static_cast<int>(record.type));
    auto &mux = GetIpcMutex();

    std::string result{};
    switch (record.type) {
        case IPCOperationType::SET_INFO:
            result = ProcessIPCSetEvent(record, mux);
            break;
        case IPCOperationType::DESTROY_INFO:
            result = ProcessIPCDestroyEvent(record, mux);
            break;
        case IPCOperationType::MAP_INFO:
            result = ProcessIPCMapEvent(record, checker, mux);
            break;
        case IPCOperationType::UNMAP_INFO:
            result = ProcessIPCUnmapEvent(record, checker, mux);
            break;
        default:
            SAN_WARN_LOG("Unknown IPC memory record type.");
            break;
    }
    msgRspFunc(result);
    std::stringstream ss;
    ss << record <<", deviceId:"<<RuntimeContext::Instance().GetDeviceId();
    SAN_INFO_LOG("%s", ss.str().c_str());
}

Command::Command(Config const &config, bool enableDebugLog, const LogLv &lv, const std::string &logFile)
    : config_{config}, enableDebugLog_{enableDebugLog}, loglv_(lv), logFile_(logFile) {
    ConfigManager::Instance().Set(config);
}

ProgramStatus Command::Exec(const ParamList &execParams) {
    Process process(config_);
    auto socketPath = process.CreateSockPath();
    // 线程管理类，负责多线程开始前和结束后的工作
    ThreadManager threadManager(config_, enableDebugLog_, loglv_, logFile_);
    KernelManager::Instance().SetDemangleMode(config_.demangleMode);

    // 原业务分发逻辑仍使用原连接的 Checker；仅改为由同 PID 的排序线程调用。
    OrderedPacketScheduler scheduler(
        [&threadManager, this](OrderedPacketScheduler::Job &job) {
            // 取出 Job 的 Host PID、原连接 Checker、业务包和回复通道；完整上下文已由 Execute 恢复
            RuntimeContext::Instance().pid_ = job.pid;
            Checker &checker = *job.checker;
            auto &packet = job.packet;
            auto &msgRspFunc = job.reply;
            CallStack::Instance().SetIsPrintFullStack(config_.isPrintFullStack);
            CrossNpuChecker &crossNpuChecker = threadManager.GetCrossNpuChecker();
            switch (packet.GetType()) {
            case PacketType::DEVICE_SUMMARY:
                HandleDeviceInfo(checker, packet.GetPayload().deviceSummary);
                break;
            case PacketType::KERNEL_SUMMARY:
                HandleKernelInfo(checker, crossNpuChecker, packet.GetPayload().kernelSummary, config_);
                break;
            case PacketType::MEMORY_RECORD:
                HandleHostMemRecord(checker, packet.GetPayload().hostMemRecord);
                break;
            case PacketType::KERNEL_RECORD:
                HandleKernelBlock(checker, crossNpuChecker, packet.GetPayload().binary, msgRspFunc);
                break;
            case PacketType::IPC_RECORD:
                HandleIpcMemRecord(checker, packet.GetPayload().ipcMemRecord, threadManager, msgRspFunc);
                break;
            case PacketType::PEER_ACCESS_RECORD: {
                const auto &record = packet.GetPayload().peerAccessRecord;
                if (record.deviceId == RuntimeContext::Instance().GetDeviceId() &&
                    PeerAccessManager::Instance().Apply(job.pid, record)) {
                    SAN_INFO_LOG("Peer access: deviceId=%d peerDeviceId=%d operation=%u", record.deviceId,
                        record.peerDeviceId, static_cast<uint32_t>(record.operation));
                } else {
                    SAN_WARN_LOG("Invalid peer access record received");
                }
                break;
            }
            case PacketType::MEM_REGION_PERMISSION:
                HandleMemRegionPermission(checker, packet.GetPayload().memPermission);
                break;
            case PacketType::SANITIZER_RECORD:
                checker.Do(packet.GetPayload().sanitizerRecord);
                break;
            case PacketType::GM_ADDR_OUT_OF_BOUND_RECORD:
                HandleGMAddrOutOfBoundRecord(checker, packet.GetPayload().gmAddrOutOfBoundRecord);
                break;
            case PacketType::KERNEL_BINARY:
                HandleKernelBinary(packet.GetPayload().binary, packet.GetBinaryBuffer());
                break;
            case PacketType::LOG_STRING:
                HandleLogString(packet.GetPayload().binary);
                break;
            default:
                break;
            }
        },
        [](const OrderedPacketScheduler::Job &job) {
            // 完成标记经解析和交付进入检查队列，由消费线程确认此前工作已检查完成
            return job.checker->CaptureMemcheckCompletion();
        },
        [](const OrderedPacketScheduler::Job &job) {
            // IPC 销毁可能向其它设备投递 FREE，先查询实际受影响的 Checker
            // 调度器据此等待此前访问这些设备的相关检查，并在分发后保存这些设备的更新完成通知
            // 此处仅提供目标对象，同 PID 内的依赖等待和完成通知由 ProcessQueue 处理
            std::vector<Checker *> targets;
            std::lock_guard<std::mutex> lock(GetIpcMutex());
            const auto &record = job.packet.GetPayload().ipcMemRecord;
            auto shared = Command::sharedMemInfoMp.find(record.destroyInfo.name);
            if (shared != Command::sharedMemInfoMp.end()) {
                for (const auto &sharee : shared->second.second) {
                    targets.push_back(sharee.first);
                }
            }
            return targets;
        });

    // msgSplitFunc完成收包，并提交给scheduler进行排序、依赖识别和按序分发
    auto msgSplitFunc = [&threadManager, &scheduler](
                            pid_t pid, const std::string &manyMsg, Process::MsgRspFunc &msgRspFunc) {
        RuntimeContext::Instance().pid_ = pid;
        Checker &checker = threadManager.GetChecker();
        auto &protocol = threadManager.GetProtocol();
        protocol.Feed(manyMsg);
        while (true) {
            auto packet = protocol.GetPacket();
            if (packet.GetType() == PacketType::INVALID) {
                return;
            }
            // 提交后接收线程可以继续读 socket 收包
            scheduler.Submit({pid, std::this_thread::get_id(), &checker, std::move(packet), msgRspFunc});
        }
    };

    process.RegisterMsgTrap(msgSplitFunc, socketPath);
    ExecCmd cmd(execParams);
    process.Launch(cmd); // 子进程结束后，Launch函数返回
    scheduler.Finish();
    sharedMemInfoMp.clear();
    threadManager.ThreadFinish();
    PeerAccessManager::Instance().Clear();
    threadManager.PostLog();
    return process.GetProgramStatus();
}

std::vector<std::string> GetSortedProjects(const std::string &dumpPath)
{
    if (!IsDir(dumpPath)) {
        SAN_INFO_LOG("No sink op need to be detected.");
        return {};
    }
    std::vector<std::pair<std::pair<int, int>, std::string>> projects;
    for (const auto &entry : fs::directory_iterator(dumpPath)) {
        std::string entryPath = entry.path();
        if (IsDir(entryPath)) {
            Path configPath{entryPath + "/kernel_config.bin"};
            if (!configPath.Exists()) {
                SAN_INFO_LOG("Invalid project, config file %s does not exist, skip detect",
                    configPath.ToString().c_str());
                continue;
            }
            std::vector<std::string> splits;
            constexpr int expectSize = 3;
            std::string projectName = Path(entryPath).Name();
            Utility::Split(projectName, std::back_inserter(splits), "_");
            std::string projectNameLog = Utility::ReplaceInvalidChar(projectName);
            if (splits.size() != expectSize) {
                SAN_INFO_LOG("Get invalid subdirectory name: %s, skip detect", projectNameLog.c_str());
                continue;
            }
            int launchId;
            int deviceId;
            constexpr int lastSecond = 2;
            if (!StoiConverter(splits.back(), launchId) ||
                !StoiConverter(splits[splits.size() - lastSecond], deviceId)) {
                SAN_INFO_LOG("Get invalid subdirectory name: %s, skip detect", projectNameLog.c_str());
                continue;
            }
            projects.push_back({{deviceId, launchId}, entryPath});
        }
    }
    std::sort(projects.begin(), projects.end());
    std::vector<std::string> pathList;
    pathList.reserve(projects.size());
    for (const auto &par: projects) {
        pathList.emplace_back(par.second);
    }
    return pathList;
}

std::string GetLauncherPath()
{
    Path exePath;
    if (!GetSelfExePath(exePath)) {
        SAN_INFO_LOG("Can not find self exe path, stop detect");
        return "";
    }
    Path launcherPath = exePath.Parent() / Path("kernel-launcher");
    if (!launcherPath.Exists()) {
        SAN_INFO_LOG("Can not find kernel launcher, stop detect");
        return "";
    }
    return launcherPath.ToString();
}

bool DetectDumpProject(Command &cmd, const std::string &dumpPath)
{
    std::shared_ptr<void> defer(nullptr, [&dumpPath](std::nullptr_t&) {
        fs::remove_all(dumpPath);
    });
    auto projectPaths = GetSortedProjects(dumpPath);
    if (projectPaths.empty()) {
        return false;
    }
    auto launcherPath = GetLauncherPath();
    if (launcherPath.empty()) {
        return false;
    }

    SAN_LOG("================= Start detect for %lu kernels. ==============", projectPaths.size());
    for (const auto &path: projectPaths) {
        Path configPath{path + "/kernel_config.bin"};
        auto name = Path(path).Name();
        std::vector<std::string> execCmd{launcherPath, "-c", configPath.ToString()};
        SAN_LOG("================= Start detect for %s project. ==============", name.c_str());
        (void)cmd.Exec(execCmd);
        SAN_LOG("================= End detect for %s project. ==============", name.c_str());
    }
    return true;
}

} // namespace Sanitizer
