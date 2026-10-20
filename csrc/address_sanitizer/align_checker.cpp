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

#include "align_checker.h"
#include "mem_error_def.h"
#include "core/framework/file_mapping.h"
#include "core/framework/format_converter.h"
#include "core/framework/utility/path.h"
#include "core/framework/utility/ustring.h"

namespace Sanitizer {

inline void SetBasicMemInfo(ErrorMsg &msg, const SanEvent &event)
{
    std::string filePath = FileMapping::Instance().Query(event.loc.fileNo).fileName;
    std::string fileName = Utility::ReplaceInvalidChar(Path(filePath).Name());
    msg.SetLocInfo(fileName, event.loc.lineNo, event.loc.pc, event.loc.coreId, event.loc.blockType);
}

AlignChecker::~AlignChecker()
{
    UnregisterNotifyFunc();
}

bool SupportAlign(AddressSpace space)
{
    switch (space) {
        case AddressSpace::PRIVATE:
        case AddressSpace::GM:
        case AddressSpace::L1:
        case AddressSpace::L0A:
        case AddressSpace::L0B:
        case AddressSpace::L0C:
        case AddressSpace::UB:
            return true;
        /// 对齐检测暂不适配BT/FP地址空间，后续适配后放开
        case AddressSpace::BT:
        case AddressSpace::FB:
            return false;
        default:
            return false;
    }
}

void AlignChecker::CheckAlign(SanEvent &event, uint16_t alignSize)
{
    if (msgFunc_ == nullptr) {
        return;
    }
    if (event.type != EventType::MEM_EVENT) {
        return;
    }
    auto &memInfo = event.eventInfo.memInfo;
    event.eventInfo.memInfo.alignSize = alignSize;
    auto space = FormatConverter::MemTypeToAddrSpace(memInfo.memType);
    if (!SupportAlign(space)) { return; }
    uint64_t addr = memInfo.addr;
    uint64_t size = memInfo.blockSize;
    if (alignSize != 0 && (addr % alignSize) != 0) {
        ErrorMsg msg;
        msg.SetType(MemErrorType::MISALIGNED_ACCESS, space, addr);
        msg.auxData.nBadBytes = size;
        SetBasicMemInfo(msg, event);
        msg.auxData.side = MemOpSide::KERNEL;
        msg.auxData.serialNo = event.serialNo;
        std::lock_guard<std::mutex> lock(errMutex_);
        errorBuffer_.Add(msg);
    }
}

void AlignChecker::CheckAlign(SanEvent &event, RecordType recordType)
{
    if (msgFunc_ == nullptr) {
        return;
    }
    if (event.type != EventType::MEM_EVENT) {
        return;
    }
    auto &memInfo = event.eventInfo.memInfo;
    if (memInfo.blockSize == 0U || memInfo.blockNum == 0U || memInfo.repeatTimes == 0U) {
        return;
    }
    auto space = FormatConverter::MemTypeToAddrSpace(memInfo.memType);
    uint16_t alignSize = FormatConverter::GetAlignSize(memInfo, space, recordType, deviceType_);
    CheckAlign(event, alignSize);
}

void AlignChecker::Notify()
{
    if (msgFunc_ == nullptr) {
        return;
    }
    if (deviceType_ >= DeviceType::INVALID) {
        SAN_WARN_LOG("Invalid device type was set for align checker.");
    }
    std::vector<ReducedErrorMsg> errorList;
    {
        std::lock_guard<std::mutex> lock(errMutex_);
        errorList = errorBuffer_.GetBuffer();
        errorBuffer_.Clear();
    }
    for (ReducedErrorMsg const &error : errorList) {
        msgFunc_(LogLv::ERROR, [&error](void) {
            std::stringstream ss;
            ss << error << std::endl;
            return DetectionInfo{ToolType::MEMCHECK, ss.str()};
        });
    }
}

}  // namespace Sanitizer
