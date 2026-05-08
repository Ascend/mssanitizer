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


#include "address_sanitizer.h"

#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <memory>

#include "bounds_check.h"
#include "mem_error_def.h"
#include "securec.h"
#include "shadow_memory.h"
#include "align_checker.h"
#include "asan_action.h"
#include "core/framework/constant.h"
#include "core/framework/record_defs.h"
#include "core/framework/file_mapping.h"
#include "core/framework/format_converter.h"
#include "core/framework/utility/cpp_future.h"
#include "core/framework/utility/path.h"
#include "core/framework/utility/ustring.h"

namespace {
Sanitizer::LogLv GetNotifyLv(const Sanitizer::MemErrorType &type)
{
    using namespace Sanitizer;
    static const std::map<MemErrorType, LogLv> ERROR_NOTIFY_LV_MAP = {
        {MemErrorType::OUT_OF_BOUNDS, LogLv::WARN},
        {MemErrorType::MISALIGNED_ACCESS, LogLv::ERROR},
        {MemErrorType::MEM_LEAK, LogLv::WARN},
        {MemErrorType::MEM_UNUSED, LogLv::WARN},
    };
    auto iter = ERROR_NOTIFY_LV_MAP.find(type);
    if (iter != ERROR_NOTIFY_LV_MAP.end()) {
        return iter->second;
    }
    return LogLv::ERROR;
}
}

namespace Sanitizer {

inline void FillFileNameByNo(MemOpRecord& record, uint64_t fileNo)
{
    /// 编译器 filemapping 中获取的路径可能会超过 record.fileName 的长度导致 strncpy_s 接口
    /// 拷贝失败。因此此处截取出文件名，并最长拷贝 sizeof(record.fileName) - 1 个字符
    std::string filePath = FileMapping::Instance().Query(fileNo).fileName;
    std::string fileName = Utility::ReplaceInvalidChar(Path(filePath).Name());

    std::size_t length = std::min(fileName.length(), sizeof(record.fileName) - 1);
    fileName.copy(record.fileName, length);
    record.fileName[length] = '\0';
}

void SetBasicMemInfo(MemOpRecord &record, const SanEvent &event)
{
    record.serialNo = event.serialNo;
    record.type = FormatConverter::AccessTypeToMemOpType(event.eventInfo.memInfo.opType);
    record.coreId = event.loc.coreId;
    record.moduleId = -1;
    // 在kernelRecord转换成sanevent的过程中，srcAddr和dstAddr相等且srcSpace和dstSpace相等
    record.srcSpace = record.dstSpace = FormatConverter::MemTypeToAddrSpace(event.eventInfo.memInfo.memType);
    record.lineNo = event.loc.lineNo;
    FillFileNameByNo(record, event.loc.fileNo);
    record.blockType = event.loc.blockType;
    record.pc = event.loc.pc;
    // 从 SanEvent 转换来的 MemOpRecord 都是 kernel 侧的记录
    record.side = MemOpSide::KERNEL;
}

void ProcessAndStoreMemOp(MemOpRecord &record, std::vector<MemOpRecord> &records)
{
    if (record.type == MemOpType::MEMCPY_BLOCKS) {
        record.srcAddr = record.dstAddr;
    }
    records.emplace_back(record);
}

// MemOpInfo转为MemOpRecord，单次repeat是单条记录
void ConvertSingleRecordRepeats(const SanEvent &event, MemOpRecord &record, std::vector<MemOpRecord> &records)
{
    MemOpInfo memInfo = event.eventInfo.memInfo;
    record.memSize = memInfo.blockStride == 1U ? memInfo.blockSize * memInfo.blockNum : memInfo.blockSize;
    uint64_t repeatStrideSize = memInfo.repeatStride * memInfo.blockSize;
    if (memInfo.repeatStride == 0) {
        record.dstAddr = memInfo.addr;
        ProcessAndStoreMemOp(record, records);
        return;
    }
    
    if (repeatStrideSize <= record.memSize) {
        // repeatStride小于合并后的单条repeat记录长度，则全部合为1条记录
        record.memSize = repeatStrideSize * (memInfo.repeatTimes - 1) + record.memSize;
        record.dstAddr = memInfo.addr;
        ProcessAndStoreMemOp(record, records);
        return;
    }

    if (memInfo.repeatTimes > 0x7FFF) {
        SAN_WARN_LOG("the repeat times exceed the maximum 0x7FFF, event serialNo: %lu", event.serialNo);
        memInfo.repeatTimes = 0x7FFF;
    }

    for (uint64_t repeatIdx = 0; repeatIdx < memInfo.repeatTimes; ++repeatIdx) {
        record.dstAddr = memInfo.addr + repeatIdx * repeatStrideSize;
        ProcessAndStoreMemOp(record, records);
    }
}

// MemOpInfo转为MemOpRecord，单次repeat是多条有间隔的记录
void ConvertMutiRecordRepeats(const SanEvent &event, MemOpRecord &record, std::vector<MemOpRecord> &records)
{
    MemOpInfo memInfo = event.eventInfo.memInfo;
    if (memInfo.repeatStride == 0) {
        for (uint64_t blockIdx = 0; blockIdx < memInfo.blockNum; ++blockIdx) {
            record.dstAddr = memInfo.addr;
            record.memSize = memInfo.blockSize;
            ProcessAndStoreMemOp(record, records);
        }
    } else if (memInfo.repeatStride == 1 && memInfo.repeatTimes >= memInfo.blockStride) {
        record.dstAddr = memInfo.addr;
        record.memSize = (memInfo.blockNum - 1 + memInfo.repeatTimes) * memInfo.blockSize;
        ProcessAndStoreMemOp(record, records);
    } else if (memInfo.repeatStride == 1 && memInfo.repeatTimes < memInfo.blockStride) {
        for (uint64_t blockIdx = 0; blockIdx < memInfo.blockNum; ++blockIdx) {
            record.dstAddr = memInfo.addr + blockIdx * memInfo.blockSize * memInfo.blockStride;
            record.memSize = memInfo.repeatTimes * memInfo.blockSize;
            ProcessAndStoreMemOp(record, records);
        }
    } else if (memInfo.repeatStride == memInfo.blockStride) {
        uint64_t recortCnt = std::max(memInfo.blockNum, memInfo.repeatTimes);
        for (uint64_t blockIdx = 0; blockIdx < recortCnt; ++blockIdx) {
            record.dstAddr = memInfo.addr + blockIdx * memInfo.blockSize * memInfo.blockStride;
            record.memSize = memInfo.blockSize;
            ProcessAndStoreMemOp(record, records);
        }
    } else {
        if (memInfo.repeatTimes > 0x7FFF) {
            SAN_WARN_LOG("the repeat times exceed the maximum 0x7FFF, event serialNo: %lu", event.serialNo);
            memInfo.repeatTimes = 0x7FFF;
        }
        for (uint64_t repeatIdx = 0; repeatIdx < memInfo.repeatTimes; ++repeatIdx) {
            uint64_t repeatAddr = memInfo.addr + repeatIdx * memInfo.blockSize * memInfo.repeatStride;
            for (uint64_t blockIdx = 0; blockIdx < memInfo.blockNum; ++blockIdx) {
                record.dstAddr = repeatAddr + blockIdx * memInfo.blockStride * memInfo.blockSize;
                record.memSize = memInfo.blockSize;
                ProcessAndStoreMemOp(record, records);
            }
        }
    }
}

void AddressSanitizer::ConvertSanEventToMemOpRecords(const SanEvent &event,
    std::vector<MemOpRecord> &records)
{
    if (event.type != EventType::MEM_EVENT) {
        return;
    }
    auto& memInfo = event.eventInfo.memInfo;
    if (memInfo.repeatTimes == 0) {
        return;
    }
    MemOpRecord record;
    record.ignoreIllegalCheck = memInfo.ignoreIllegalCheck;
    SetBasicMemInfo(record, event);
    // 原内存检测解析时是按repeattimes遍历读->写->读->写；
    // 而解析成SanEvent再解析成MemOpRecords后会变成 读读读...->写写写....此处逻辑发生变动
    if (memInfo.blockStride <= 1) {
        ConvertSingleRecordRepeats(event, record, records);
    } else {
        ConvertMutiRecordRepeats(event, record, records);
    }
}

inline bool IsContinuousAddr(const MemOpRecord &lhs, const MemOpRecord &rhs)
{
    return (lhs.dstAddr + lhs.memSize == rhs.dstAddr) && (
            lhs.type != MemOpType::MEMCPY_BLOCKS ||
            lhs.srcAddr + lhs.memSize == rhs.srcAddr);
}

inline bool IsSameOp(const MemOpRecord &lhs, const MemOpRecord &rhs)
{
    return (lhs.type == rhs.type) && (lhs.dstSpace == rhs.dstSpace) && (
            lhs.type != MemOpType::MEMCPY_BLOCKS ||
            lhs.srcSpace == rhs.srcSpace);
}

void MergeRecords(std::vector<MemOpRecord> &records)
{
    size_t last = 0;
    for (size_t i = 1; i < records.size(); i++) {
        if (IsSameOp(records[last], records[i]) && IsContinuousAddr(records[last], records[i])) {
            records[last].memSize += records[i].memSize;
        } else {
            ++last;
            records[last] = std::move(records[i]);
        }
    }
    records.resize(last + 1);
}

AddressSanitizer::AddressSanitizer()
    : shadowMemory_{MakeUnique<ShadowMemory>()},
      msgFunc_{&AddressSanitizer::DefaultMsgFunc},
      config_{}, boundsCheckScope_{BoundsCheckScope::RUNTIME}, historyBoundsCheckScope_{BoundsCheckScope::RUNTIME} { }

// AddressSanitizer的具体实现
bool AddressSanitizer::SetDeviceInfo(DeviceInfoSummary const &deviceInfo, Config const &config)
{
    config_ = config;
    // init shadow memory by chipType
    auto it = CHIP_INFO_MAP.find(deviceInfo.device);
    if (it == CHIP_INFO_MAP.end()) {
        SAN_WARN_LOG("The soc type %u is invalid ", static_cast<uint32_t>(deviceInfo.device));
        return false;
    }

    AlignChecker::Instance().SetDeviceType(deviceInfo.device);
    ChipInfo chipInfo = it->second;
    boundsCheckRuntime_.Init(chipInfo);
    boundsCheckDfx_.Init(chipInfo);
    return shadowMemory_->Init(chipInfo);
}

bool AddressSanitizer::SetKernelInfo(KernelSummary const &kernelInfo)
{
    return false;
}

bool AddressSanitizer::CheckRecordBeforeProcess(const SanitizerRecord &record)
{
    if (shadowMemory_ == nullptr || !shadowMemory_->IsReady()) {
        SAN_WARN_LOG("Shadow memory has NOT been initialized. Device type is missing or unsupported");
        return false;
    }

    if (record.version == RecordVersion::MEMORY_RECORD) {
        this->BeforeScopeSwitch(record.payload.memoryRecord);
        this->DoMemOpRecord(record.payload.memoryRecord, false);
        this->AfterScopeSwitch(record.payload.memoryRecord);
        return false;
    }

    if (record.version == RecordVersion::KERNEL_RECORD) {
        if (record.payload.kernelRecord.recordType == RecordType::BLOCK_FINISH) {
            shadowMemory_->ResetChipMemory();
            shadowMemory_->ResetPrivateMemory();
            return true;
        }
        if (record.payload.kernelRecord.recordType == RecordType::FINISH) {
            this->ReportErrorMsg();
            AlignChecker::Instance().Notify();
            this->errorBuffer_.Clear();
            return true;
        }
        bool atomicEnabled;
        if (FormatConverter::GetAtomicFlag(record.payload.kernelRecord, atomicEnabled)) {
            shadowMemory_->SetAtomic(atomicEnabled);
            return false;
        }
    }
    return true;
}

void AddressSanitizer::ReportErrorMsg()
{
    auto const &errorList = this->errorBuffer_.GetBuffer();

    // build pc stack map cache
    std::set<uint64_t> pcOffsets;
    for (ReducedErrorMsg const &error : errorList) {
        pcOffsets.insert(error.errorMsg.auxData.pc);
    }
    CallStack::Instance().CachePcOffsets(RuntimeContext::Instance().kernelSummary_.kernelName, pcOffsets);

    for (ReducedErrorMsg const &error : errorList) {
        msgFunc_(GetNotifyLv(error.errorMsg.type), [&error](void) {
            std::stringstream ss;
            ss << error << std::endl;
            return DetectionInfo{ToolType::MEMCHECK, ss.str()};
        });
    }
}

bool AddressSanitizer::SwitchToScope(BoundsCheckScope scope)
{
    if (scope == boundsCheckScope_) {
        return false;
    }
    boundsCheckScope_ = scope;
    return true;
}

ErrorMsgList AddressSanitizer::ScopeDoAction(std::shared_ptr<AsanAction> action)
{
    switch (boundsCheckScope_) {
        case AddressSanitizer::BoundsCheckScope::DFX:
            return action->doAction(*shadowMemory_, boundsCheckDfx_, config_);
        case AddressSanitizer::BoundsCheckScope::BYPASS: {
            // bypass第一次处理时先忽略shadowMemory，保证boundsCheck free时能获取到size，以防漏报；
            ErrorMsgList dfxError = action->doAction(*shadowMemory_, boundsCheckDfx_, config_, true);
            ErrorMsgList rtError = action->doAction(*shadowMemory_, boundsCheckRuntime_, config_);
            dfxError.reserve(dfxError.size() + rtError.size());
            dfxError.insert(dfxError.end(), rtError.begin(), rtError.end());
            return dfxError;
        }
        default:
            return action->doAction(*shadowMemory_, boundsCheckRuntime_, config_);
    }
}

AddressSanitizer::BoundsCheckScope AddressSanitizer::MemInfoToScope(MemInfoSrc infoSrc)
{
    switch (infoSrc) {
        case MemInfoSrc::EXTRA:
        case MemInfoSrc::MANUAL:
            return BoundsCheckScope::DFX;
        case MemInfoSrc::BYPASS:
            return BoundsCheckScope::BYPASS;
        default:
            return BoundsCheckScope::RUNTIME;
    }
}

std::string AddressSanitizer::ScopeToString(BoundsCheckScope scope)
{
    switch (scope) {
        case AddressSanitizer::BoundsCheckScope::DFX: return "DFX";
        case AddressSanitizer::BoundsCheckScope::BYPASS: return "BYPASS";
        default: return "RT";
    }
}

void AddressSanitizer::UpdateHeapInfo(MemOpRecord const &record)
{
    if (record.infoSrc != MemInfoSrc::MSTX_HEAP) {
        return;
    }
    if (record.type == MemOpType::MALLOC) {
        mstxHeapCacheMap_[record.dstAddr]++;
    }
    if (record.type == MemOpType::FREE) {
        auto it = mstxHeapCacheMap_.find(record.dstAddr);
        if (it != mstxHeapCacheMap_.cend()) {
            if (it->second <= 1U) {
                mstxHeapCacheMap_.erase(it);
            } else {
                it->second--;
            }
        }
    }
}

void AddressSanitizer::BeforeScopeSwitch(MemOpRecord const &record)
{
    // 根据信息来源切换到对应的上下文
    auto infoSrc = record.infoSrc;
    historyBoundsCheckScope_ = boundsCheckScope_;
    UpdateHeapInfo(record);
    auto scope = MemInfoToScope(infoSrc);
    if (this->SwitchToScope(scope)) {
        SAN_INFO_LOG("Bounds check before switches to scope %s, serialNo:%lu, memInfoSrc:%s",
            ScopeToString(scope).c_str(), record.serialNo, MemInfoSrcToString(record.infoSrc).c_str());
    }
}

void AddressSanitizer::AfterScopeSwitch(MemOpRecord const &record)
{
    // 如果heap没有被unregister，并且当前信息来说是extra，则以RUNTIME的scope为准；
    if (record.infoSrc == MemInfoSrc::EXTRA && !mstxHeapCacheMap_.empty()) {
        if (this->SwitchToScope(BoundsCheckScope::RUNTIME)) {
            SAN_INFO_LOG("Bounds check after switches to scope RT, serialNo:%lu, memInfoSrc:%s",
                         record.serialNo, MemInfoSrcToString(record.infoSrc).c_str());
        }
    }

    // 处理完 DFX 的内存释放信息后，需要退化到 RUNTIME 上下文
    if ((record.infoSrc == MemInfoSrc::EXTRA || record.infoSrc == MemInfoSrc::MANUAL) &&
        record.type == MemOpType::FREE) {
        if (this->SwitchToScope(BoundsCheckScope::RUNTIME)) {
            SAN_INFO_LOG("Bounds check after scope decays to RT after free");
        }
    }
    if (record.infoSrc == MemInfoSrc::BYPASS) {
        if (this->SwitchToScope(historyBoundsCheckScope_)) {
            SAN_INFO_LOG("Bounds check after switches to scope %s, serialNo:%lu, memInfoSrc:%s",
                ScopeToString(historyBoundsCheckScope_).c_str(), record.serialNo,
                MemInfoSrcToString(record.infoSrc).c_str());
        }
    }
}

size_t AddressSanitizer::GetRecordsNum(const std::vector<SanEvent> &events) const
{
    size_t numRecords = 0;
    for (auto& event : events) {
        if (event.type == EventType::MEM_EVENT) {
            const auto &memInfo = event.eventInfo.memInfo;
            if (memInfo.repeatTimes > 0x7FFF) {
                continue;
            }
            if (memInfo.blockStride <= 1) {
                numRecords += memInfo.repeatTimes;
            } else {
                numRecords += memInfo.repeatTimes * memInfo.blockNum;
            }
        }
    }
    return numRecords;
}

void AddressSanitizer::Do(const SanitizerRecord &record, const std::vector<SanEvent> &events)
{
    size_t numRecords = GetRecordsNum(events);
    std::vector<MemOpRecord> records;
    // important, can speed much
    records.reserve(numRecords);
    for (auto& event : events) {
        ConvertSanEventToMemOpRecords(event, records);
    }
    if (records.empty()) {
        return;
    }

    MergeRecords(records);

    if (IsMstxRecordWithTensor(record)) {
        DoWithLocalTensor(record.payload.kernelRecord.payload.mstxRecord, records);
        return;
    }

    for (MemOpRecord const& r : records) {
        this->DoMemOpRecord(r, true);
    }
}

inline void AddLocalTensorBound(BoundsCheck &boundsCheck, MstxTensorDesc const &tensor)
{
    if (tensor.space == AddressSpace::GM) {
        return;
    }
    boundsCheck.Add(tensor.space, tensor.addr, tensor.size * tensor.dataBits / BITS_EACH_BYTE);
}

void AddressSanitizer::DoWithLocalTensor(const MstxRecord &record, const std::vector<MemOpRecord> &records)
{
    BoundsCheck boundsCheckR(true); // 用于检查读 tensor 越界
    BoundsCheck boundsCheckW(true); // 用于检查写 tensor 越界

    if (record.interfaceType == InterfaceType::MSTX_VEC_UNARY_OP) {
        AddLocalTensorBound(boundsCheckR, record.interface.mstxVecUnaryDesc.src);
        AddLocalTensorBound(boundsCheckW, record.interface.mstxVecUnaryDesc.dst);
    } else if (record.interfaceType == InterfaceType::MSTX_VEC_BINARY_OP) {
        AddLocalTensorBound(boundsCheckR, record.interface.mstxVecBinaryDesc.src0);
        AddLocalTensorBound(boundsCheckR, record.interface.mstxVecBinaryDesc.src1);
        AddLocalTensorBound(boundsCheckW, record.interface.mstxVecBinaryDesc.dst);
    } else if (record.interfaceType == InterfaceType::MSTX_DATA_COPY) {
        AddLocalTensorBound(boundsCheckR, record.interface.mstxDataCopyDesc.src);
        AddLocalTensorBound(boundsCheckW, record.interface.mstxDataCopyDesc.dst);
    } else if (record.interfaceType == InterfaceType::MSTX_DATA_COPY_PAD) {
        AddLocalTensorBound(boundsCheckR, record.interface.mstxDataCopyPadDesc.src);
        AddLocalTensorBound(boundsCheckW, record.interface.mstxDataCopyPadDesc.dst);
    }

    for (MemOpRecord const& r : records) {
        if (r.type != MemOpType::LOAD && r.type != MemOpType::STORE) {
            SAN_WARN_LOG("Invalid MemOpType %s in local tensor address sanitizer", ToString(r.type).c_str());
            continue;
        }
        if (r.dstSpace == AddressSpace::GM) {
            // GlobalTensor 当前长度不准确，不做 tensor 越界检测
            continue;
        }

        auto action = AsanActionFactory::CreateAsanAction(r);
        if (action == nullptr) {
            SAN_WARN_LOG("Failed to create action");
            return;
        }
        BoundsCheck &boundsCheck = r.type == MemOpType::LOAD ? boundsCheckR : boundsCheckW;
        ErrorMsgList errors = action->doAction(*shadowMemory_, boundsCheck, config_, true);
        for (auto const &error : errors) {
            this->errorBuffer_.Add(error);
        }
    }
}

void AddressSanitizer::ParseOnlineError(const KernelErrorRecord &record, BlockType blockType, uint64_t serialNo)
{
    if (record.kernelErrorDesc == nullptr) {
        SAN_ERROR_LOG("kernelErrorDesc is nullptr");
        return;
    }
    ErrorMsg error{};
    error.isError = true;
    error.auxData.blockType = blockType;
    error.auxData.serialNo = serialNo;
    error.auxData.side = MemOpSide::KERNEL;
    error.auxData.isSimtError = true;

    for (size_t errorIdx = 0; errorIdx < record.errorNum; ++errorIdx) {
        const KernelErrorDesc &kernelErrorDesc = record.kernelErrorDesc[errorIdx];
        if (kernelErrorDesc.errorType == KernelErrorType::THREAD_OVERLAP) {
            SAN_INFO_LOG("shadow memory thread(%u,%u,%u) l1StartAddr=0x%lx l2StartAddr=0x%lx l2MemStatusAddr=0x%lx(maxAddr)",
                error.auxData.threadLoc.idX, error.auxData.threadLoc.idY, error.auxData.threadLoc.idZ,
                kernelErrorDesc.l1StartAddr, kernelErrorDesc.l2StartAddr, kernelErrorDesc.l2MemStatusAddr);
            if (kernelErrorDesc.l2StartAddr >= kernelErrorDesc.l2MemStatusAddr || kernelErrorDesc.l2StartAddr < kernelErrorDesc.l1StartAddr) {
                SAN_INFO_LOG("====== invalid l2StartAddr=0x%lu", kernelErrorDesc.l2StartAddr);
            }
            if (kernelErrorDesc.l2StartAddr % 8 != 0) {
                SAN_INFO_LOG("====== unaligned l2StartAddr=0x%lu", kernelErrorDesc.l2StartAddr);
            }
 	    }

        if (kernelErrorDesc.errorType >= KernelErrorType::MAX) {
            SAN_ERROR_LOG("Unknown kernel error type: %u", static_cast<uint32_t>(kernelErrorDesc.errorType));
            continue;
        }

        error.auxData.lineNo = kernelErrorDesc.location.lineNo;
        std::string filePath = FileMapping::Instance().Query(kernelErrorDesc.location.fileNo).fileName;
        error.auxData.fileName = Utility::ReplaceInvalidChar(Path(filePath).Name());
        error.auxData.space = kernelErrorDesc.space;
        error.auxData.pc = kernelErrorDesc.location.pc;
        error.auxData.coreId = kernelErrorDesc.location.blockId;
        error.auxData.threadLoc = kernelErrorDesc.threadLoc;
        if (kernelErrorDesc.errorType == KernelErrorType::ILLEGAL_ADDR_READ ||
            kernelErrorDesc.errorType == KernelErrorType::ILLEGAL_ADDR_WRITE) {
            error.type = kernelErrorDesc.errorType == KernelErrorType::ILLEGAL_ADDR_READ ?
                MemErrorType::ILLEGAL_ADDR_READ :  MemErrorType::ILLEGAL_ADDR_WRITE;
            auto &errorDesc = kernelErrorDesc.payload.illegalDesc;
            error.auxData.badAddr.addr = errorDesc.addr;
            error.auxData.nBadBytes = errorDesc.illegalSize;
            this->errorBuffer_.Add(error);
        } else if (kernelErrorDesc.errorType == KernelErrorType::MISALIGNED_ACCESS) {
            error.type = MemErrorType::MISALIGNED_ACCESS;
            auto &errorDesc = kernelErrorDesc.payload.misAlignDesc;
            error.auxData.badAddr.addr = errorDesc.addr;
            error.auxData.nBadBytes = errorDesc.misAlignSize;
            this->errorBuffer_.Add(error);
        } else if (kernelErrorDesc.errorType == KernelErrorType::THREAD_OVERLAP) {
            error.type = MemErrorType::OUT_OF_BOUNDS;
            auto &errorDesc = kernelErrorDesc.payload.overLapDesc;
            error.auxData.badAddr.addr = errorDesc.addr;
            error.auxData.nBadBytes = errorDesc.overLapSize;
            error.auxData.conflictedThreadLoc = errorDesc.conflictedThreadLoc;
            this->errorBuffer_.Add(error);
        }
    }
}

void AddressSanitizer::DoMemOpRecord(MemOpRecord const &record, bool reduce)
{
    auto action = AsanActionFactory::CreateAsanAction(record);
    if (action == nullptr) {
        SAN_WARN_LOG("Failed to create action");
        return;
    }

    msgFunc_(LogLv::INFO, [&action](void) {
        std::stringstream recordStr;
        recordStr << action->record_ << std::endl;
        return DetectionInfo{ToolType::MEMCHECK, recordStr.str()};
    });

    ErrorMsgList errorTypes = ScopeDoAction(action);
    for (ErrorMsg const &error : errorTypes) {
        if (reduce) {
            this->errorBuffer_.Add(error);
            continue;
        }
        msgFunc_(GetNotifyLv(error.type), [&error](void) {
            std::stringstream ss;
            ss << ReducedErrorMsg{error, {}, {}, {}} << std::endl;
            return DetectionInfo{ToolType::MEMCHECK, ss.str()};
        });
    }
}

void AddressSanitizer::RegisterNotifyFunc(const MSG_FUNC& func)
{
    msgFunc_ = func;
    AlignChecker::Instance().RegisterNotifyFunc(func);
    return;
}

void AddressSanitizer::Exit()
{
    auto notifyFuncReclaim = [this](int*) {
        if (this->msgFunc_ != nullptr) {
            AlignChecker::Instance().UnregisterNotifyFunc();
            this->msgFunc_ = nullptr;
        }
    };
    std::shared_ptr<int> notifyFuncReclaimer(nullptr, notifyFuncReclaim);
    if (!shadowMemory_) {
        SAN_INFO_LOG("ShadowMemory is not initialized yet");
        return;
    }

    if (config_.checkUnusedMemory) {
        SummaryUnusedHeapCheck();
    }

    if (config_.leakCheck) {
        SummaryLeakCheck();
    } else {
        // 未启用leak check则直接退出
        SAN_INFO_LOG("Leak check is not enabled");
    }

    return;
}

void AddressSanitizer::SummaryUnusedHeapCheck()
{
    std::stringstream ss1;
    ErrorMsgList errNoUse = shadowMemory_->CheckUnusedHeap();
    for (ErrorMsg error : errNoUse) {
        ss1 << ReducedErrorMsg{error, {}, {}, {}} << std::endl;
    }
    UnusedHeap badHeap = shadowMemory_->GetUnusedHeap();
    if ((badHeap.blockNum > 0U) && (msgFunc_ != nullptr)) {
        ss1 << "====== SUMMARY: " << badHeap.bytesNotUse << "byte(s) unused memory in " <<
            badHeap.blockNum << " allocation(s)" << std::endl << std::endl;
        msgFunc_(LogLv::WARN, [&ss1](void) {
           return DetectionInfo{ToolType::MEMCHECK, ss1.str()};
        });
    }
}

void AddressSanitizer::SummaryLeakCheck()
{
    // 检查内存泄漏情况
    auto msgs = shadowMemory_->DoLeakCheck();
    if (msgs.empty()) {
        return;
    }

    std::stringstream ss;
    ss << "====== ERROR: LeakCheck: detected memory leaks" << std::endl << std::endl;
    uint64_t lenSum = 0U;
    for (const auto &msg : msgs) {
        ss << ReducedErrorMsg{msg, {}, {}, {}} << std::endl;
        lenSum += msg.auxData.nBadBytes;
    }
    ss << "====== SUMMARY: " << lenSum << " byte(s) leaked in " << msgs.size() << " allocation(s)" << std::endl;
    if (msgFunc_ != nullptr) {
        msgFunc_(GetNotifyLv(MemErrorType::MEM_LEAK), [&ss](void) {
            return DetectionInfo{ToolType::MEMCHECK, ss.str()};
        });
        msgFunc_ = nullptr;
    }
}

static std::shared_ptr<SanitizerBase> CreateAddressSanitizer()
{
    return std::make_shared<AddressSanitizer>();
}

static RegisteSanitizer g_regAddressSanitizer(ToolType::MEMCHECK, CreateAddressSanitizer);

}
