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
#include "platform_config.h"
#include "record_format.h"

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
    RuntimeContext::Instance().kernelSummary_ = kernelSummary;
    RuntimeContext::Instance().kernelNameDisplay = GetDisplayKernelName(kernelSummary, config.demangleMode);
    KernelManager::Instance().Add(RuntimeContext::Instance().GetDeviceId(), kernelSummary);
    checker.SetKernelInfo(kernelSummary);
    crossNpuChecker.GetRecordArray(RuntimeContext::Instance().GetDeviceId()).CreateNewKernel();
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

std::string ProcessIPCSetEvent(IPCMemRecord const &record, std::mutex &mux)
{
    std::string setInfoNameLog = Utility::ReplaceInvalidChar(std::string(record.setInfo.name));
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

std::string ProcessIPCDestroyEvent(IPCMemRecord const &record, ThreadManager &threadManeger, std::mutex &mux)
{
    std::lock_guard<std::mutex> lk(mux);
    IPCResponse resp{IPCOperationType::DESTROY_INFO, ResponseStatus::FAIL};
    auto it = Command::sharedMemInfoMp.find(record.destroyInfo.name);
    if (it == Command::sharedMemInfoMp.end()) {
        SAN_WARN_LOG("Error occurs when destroying shared memory, as its name (%s) does not existed.",
            record.destroyInfo.name);
    } else {
        auto &shareeMemInfoList = it->second.second;
        for (auto &p : shareeMemInfoList) {
            auto curTid = p.first;
            auto addr = p.second.addr;
            HostMemRecord mallocRecord{MemOpType::FREE, MemInfoSrc::BYPASS, MemInfoDesc::IPC_MEMORY, 0, addr, 0};
            HandleHostMemRecord(threadManeger.GetChecker(curTid), mallocRecord);
            SAN_INFO_LOG("Freeing sharee GM: addr:0x%lx.", addr);
        }
        resp.status = ResponseStatus::SUCCESS;
        Command::sharedMemInfoMp.erase(it);
    }
    return Serialize(PacketType::IPC_RESPONSE, resp);
}

std::string ProcessIPCMapEvent(IPCMemRecord const &record, ThreadManager &threadManeger, std::mutex &mux)
{
    IPCResponse resp{IPCOperationType::MAP_INFO, ResponseStatus::FAIL};
    auto it1 = Command::shareeMemInfoMp.find(record.mapInfo.addr);
    if (it1 != Command::shareeMemInfoMp.end()) {
        SAN_WARN_LOG("Error occurs when opening shared memory , as its address (0x%lx) has already existed.",
            record.mapInfo.addr);
    } else {
        std::lock_guard<std::mutex> lk(mux);
        auto it2 = Command::sharedMemInfoMp.find(record.mapInfo.name);
        if (it2 == Command::sharedMemInfoMp.end()) {
            SAN_WARN_LOG("Error occurs when opening shared memory, as its name (%s) does not existed.",
                record.mapInfo.name);
        } else {
            GMType size = it2->second.first.size;
            ShareeMemInfo shareeMemInfo{record.mapInfo.addr, size};
            it2->second.second.insert({std::this_thread::get_id(), std::move(shareeMemInfo)});
            Command::shareeMemInfoMp.insert({record.mapInfo.addr, record.mapInfo.name});
            HostMemRecord mallocRecord{MemOpType::MALLOC, MemInfoSrc::BYPASS, MemInfoDesc::IPC_MEMORY,
                0, record.mapInfo.addr, size};
            HandleHostMemRecord(threadManeger.GetChecker(), mallocRecord);
            resp.status = ResponseStatus::SUCCESS;
        }
    }
    return Serialize(PacketType::IPC_RESPONSE, resp);
}

std::string ProcessIPCUnmapEvent(IPCMemRecord const &record, ThreadManager &threadManeger, std::mutex &mux)
{
    IPCResponse resp{IPCOperationType::UNMAP_INFO, ResponseStatus::FAIL};
    auto it = Command::shareeMemInfoMp.find(record.unmapInfo.addr);
    if (it == Command::shareeMemInfoMp.end()) {
        SAN_WARN_LOG("Error occurs when closing shared memory , as its addr (0x%lx) does not exist.",
            record.unmapInfo.addr);
    } else {
        std::lock_guard<std::mutex> lk(mux);
        auto itShared = Command::sharedMemInfoMp.find(it->second);
        if (itShared == Command::sharedMemInfoMp.end()) {
            std::string sharedMemoryLog = Utility::ReplaceInvalidChar(it->second);
            SAN_INFO_LOG("Shared memory (%s) has already been destroyed.", sharedMemoryLog.c_str());
        } else {
            auto curTid = std::this_thread::get_id();
            HostMemRecord mallocRecord{MemOpType::FREE, MemInfoSrc::BYPASS, MemInfoDesc::IPC_MEMORY,
                0, record.unmapInfo.addr, 0};
            HandleHostMemRecord(threadManeger.GetChecker(curTid), mallocRecord);
            itShared->second.second.erase(std::this_thread::get_id());
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
    records.reserve(kernelBlock->GetRecordNum());
    sanitizerRecord.version = RecordVersion::KERNEL_RECORD;
    CrossNpuChecker::RecordArray &recordArray = crossNpuChecker.GetRecordArray(runtimeContext.GetDeviceId());
    while (kernelBlock->NextSimd(sanitizerRecord.payload.kernelRecord)) {
        records.emplace_back(sanitizerRecord);
        recordArray.Push(sanitizerRecord);

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
    recordArray.Push(sanitizerRecord);

    // wait for next block
    ++runtimeContext.currentBlockIdx_;

    // report finish if this is the last block
    if (runtimeContext.currentBlockIdx_ == kernelBlock->GetTotalBlockDim()) {
        KernelRecordResponse resp{runtimeContext.currentBlockIdx_, ResponseStatus::SUCCESS};
        msgRspFunc(Serialize(PacketType::KERNEL_RECORD_RESPONSE, resp));
        sanitizerRecord.payload.kernelRecord.recordType = RecordType::KERNEL_FINISH;
        sanitizerRecord.payload.kernelRecord.serialNo = ++runtimeContext.serialNo_;
        records.emplace_back(sanitizerRecord);
        recordArray.Push(sanitizerRecord);
        SAN_INFO_LOG("Finish processing the last kernel block.");
    }

    checker.Do(records);
}

inline void HandleKernelBinary(Packet::BinaryPayload const &payload)
{
    std::vector<char> buffer(payload.buf, payload.buf + payload.len);

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

void HandleIpcMemRecord(Sanitizer::Checker &checker, Sanitizer::IPCMemRecord const &record,
    Sanitizer::ThreadManager &threadManeger, Process::MsgRspFunc &msgRspFunc)
{
    SAN_INFO_LOG("Processing IpcMemRecord type:%d.", static_cast<int>(record.type));
    static std::mutex mux;

    static SharedMemInfoMpType sharedMemInfoMp;
    thread_local static ShareeMemMpType shareeMemInfoMp;
    std::string result{};
    switch (record.type) {
        case IPCOperationType::SET_INFO:
            result = ProcessIPCSetEvent(record, mux);
            break;
        case IPCOperationType::DESTROY_INFO:
            result = ProcessIPCDestroyEvent(record, threadManeger, mux);
            break;
        case IPCOperationType::MAP_INFO:
            result = ProcessIPCMapEvent(record, threadManeger, mux);
            break;
        case IPCOperationType::UNMAP_INFO:
            result = ProcessIPCUnmapEvent(record, threadManeger, mux);
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

void Command::Exec(const ParamList &execParams)
{
    Process process(config_);
    auto socketPath = process.CreateSockPath();
    // 线程管理类，负责多线程开始前和结束后的工作
    ThreadManager threadManager(config_, loglv_, logFile_);
    KernelManager::Instance().SetDemangleMode(config_.demangleMode);

    auto msgSplitFunc = [&threadManager, this](
                            const std::string &manyMsg, Process::MsgRspFunc &msgRspFunc) {
        CallStack::Instance().SetIsPrintFullStack(config_.isPrintFullStack);
        Checker& checker = threadManager.GetChecker();
        CrossNpuChecker &crossNpuChecker = threadManager.GetCrossNpuChecker();
        auto &protocol = threadManager.GetProtocol();
        protocol.Feed(manyMsg);
        while (true) {
            auto packet = protocol.GetPacket();
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
                    HandleKernelBinary(packet.GetPayload().binary);
                    break;
                case PacketType::LOG_STRING:
                    HandleLogString(packet.GetPayload().binary);
                    break;
                case PacketType::INVALID:
                default:
                    return;
            }
        }
    };

    process.RegisterMsgTrap(msgSplitFunc, socketPath);
    ExecCmd cmd(execParams);
    process.Launch(cmd); // 子进程结束后，Launch函数返回
    threadManager.ThreadFinish();
    threadManager.PostLog();
    return;
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
        cmd.Exec(execCmd);
        SAN_LOG("================= End detect for %s project. ==============", name.c_str());
    }
    return true;
}

} // namespace Sanitizer
