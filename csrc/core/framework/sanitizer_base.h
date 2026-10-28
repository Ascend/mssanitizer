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


#ifndef CORE_FRAMEWORK_SANITIZER_BASE_H
#define CORE_FRAMEWORK_SANITIZER_BASE_H

#include <memory>
#include <functional>
#include <unordered_map>
#include "record_defs.h"
#include "platform_config.h"
#include "config.h"
#include "utility/log.h"
#include "utility/types.h"
#include "event_def.h"
#include "utility/event_range.h"

namespace Sanitizer {
// SanitizerBase接口类用于内存检测和竞争检测等接入mssanitizer框架
// 内存检测和竞争检测基于该接口实现具体的sanitizer功能
class SanitizerBase {
public:
    SanitizerBase() = default;
    virtual ~SanitizerBase() = default;
    using MSG_GEN = Generator<DetectionInfo>;
    using MSG_FUNC = std::function<void(const LogLv &lv, MSG_GEN &&gen)>;
    virtual bool SetDeviceInfo(DeviceInfoSummary const &deviceInfo, Config const &config) = 0;
    virtual bool SetKernelInfo(KernelSummary const &kernelInfo) = 0;
    virtual void Do(const SanitizerRecord &record, const std::vector<SanEvent> &events) = 0;
    virtual void DoEvents(const SanitizerRecord &record, EventRange events) {
        std::vector<SanEvent> copy;
        if (!events.empty()) {
            copy.assign(events.begin(), events.end());
        }
        Do(record, copy);
    }
    virtual void ParseOnlineError(const KernelErrorRecord &record, BlockType blockType, uint64_t serialNo) = 0;
    virtual bool CheckRecordBeforeProcess(const SanitizerRecord &record) = 0;
    virtual void RegisterNotifyFunc(const MSG_FUNC &func) = 0;
    virtual void Exit() = 0;
};

// SanitizerFactory类作用是根据输入mode创建对应的sanitizer对象
class SanitizerFactory {
public:
    using SanitizerCreater = std::function<std::shared_ptr<SanitizerBase>()>;
    static SanitizerFactory& GetInstance() noexcept;
    std::shared_ptr<SanitizerBase> Create(const ToolType tool);
    void RegisteCreater(const ToolType tool, const SanitizerCreater& func);
    virtual ~SanitizerFactory() = default;

private:
    SanitizerFactory() = default;
    SanitizerFactory(const SanitizerFactory&) = delete;
    SanitizerFactory& operator=(const SanitizerFactory&)& = delete;
    std::unordered_map<ToolType, SanitizerCreater> funcList_;
};

// RegisteSanitizer类用于向mssanitizer框架注册各种sanitizer
class RegisteSanitizer {
public:
    RegisteSanitizer(ToolType tool, const SanitizerFactory::SanitizerCreater &func)
    {
        SanitizerFactory::GetInstance().RegisteCreater(tool, func);
    }
    virtual ~RegisteSanitizer() = default;

    RegisteSanitizer(const RegisteSanitizer&) = delete;
    RegisteSanitizer& operator=(const RegisteSanitizer&)& = delete;
};

inline bool IsMstxRecordWithTensor(SanitizerRecord const &record)
{
    return
        record.version == RecordVersion::KERNEL_RECORD &&
        record.payload.kernelRecord.recordType == RecordType::MSTX_STUB &&
        record.payload.kernelRecord.payload.mstxRecord.interfaceType >= InterfaceType::MSTX_VEC_UNARY_OP &&
        record.payload.kernelRecord.payload.mstxRecord.interfaceType < InterfaceType::MSTX_WITH_TENSOR;
}

}
#endif
