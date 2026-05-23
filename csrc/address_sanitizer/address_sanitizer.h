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


#ifndef ADDRESS_SANITIZER_ADDRESS_SANITIZER_H
#define ADDRESS_SANITIZER_ADDRESS_SANITIZER_H

#include <functional>
#include <memory>
#include <cstdint>
#include <unordered_set>
#include "bounds_check.h"
#include "shadow_memory.h"
#include "mem_error_def.h"
#include "error_buffer.h"
#include "asan_action.h"
#include "core/framework/config.h"
#include "core/framework/record_defs.h"
#include "core/framework/sanitizer_base.h"

namespace Sanitizer {

// 该类本身功能主要是支持地址和值有效性的检测
// 其本身利用代理的完成实际功能，但是其接口定义了对外的约束
// 调用方需要按照要求格式输入操作内容并明确事情发生后的通知函数
class AddressSanitizer : public SanitizerBase {
public:
    AddressSanitizer();
    bool SetDeviceInfo(DeviceInfoSummary const &deviceInfo, Config const &config) override;
    bool SetKernelInfo(KernelSummary const &kernelInfo) override;
    void Do(const SanitizerRecord &record, const std::vector<SanEvent> &events) override;
    void ParseOnlineError(const KernelErrorRecord &record, BlockType blockType, uint64_t serialNo) override;
    bool CheckRecordBeforeProcess(const SanitizerRecord &record) override;
    void RegisterNotifyFunc(const MSG_FUNC &func) override;
    void Exit() override;

private:
    // 标记越界检测时使用的上下文类型
    enum class BoundsCheckScope {
        RUNTIME,        // 使用运行时的内存信息，对应没有上报 EXTRA 信息的情况
        DFX,            // 使用 DFX tensor 信息，对应上报了 EXTRA 信息的情况
        BYPASS,
    };

private:
    void SummaryUnusedHeapCheck();
    void SummaryLeakCheck();

    static void DefaultMsgFunc(LogLv const&, MSG_GEN &&) {};
    static void ConvertSanEventToMemOpRecords(const SanEvent &event, std::vector<MemOpRecord> &records);
    void DoMemOpRecord(MemOpRecord const &record, bool reduce);
    size_t GetRecordsNum(const std::vector<SanEvent> &events) const;
    void ReportErrorMsg();
    bool SwitchToScope(BoundsCheckScope scope);
    void BeforeScopeSwitch(MemOpRecord const &record);
    void AfterScopeSwitch(MemOpRecord const &record);
    ErrorMsgList ScopeDoAction(std::shared_ptr<AsanAction> action);
    BoundsCheckScope MemInfoToScope(MemInfoSrc infoSrc);
    std::string ScopeToString(BoundsCheckScope scope);
    void UpdateHeapInfo(MemOpRecord const &record);
    void DoWithLocalTensor(const MstxRecord &record, const std::vector<MemOpRecord> &records);
    void AlertGMAddrOutOfBoundError(MemOpRecord memoryRecord);

    std::unique_ptr<ShadowMemory> shadowMemory_;
    MSG_FUNC msgFunc_;
    Config config_;
    ErrorBuffer errorBuffer_;
    BoundsCheckScope boundsCheckScope_;
    BoundsCheckScope historyBoundsCheckScope_;
    BoundsCheck boundsCheckRuntime_;
    BoundsCheck boundsCheckDfx_;
    std::map<uint64_t, uint64_t> mstxHeapCacheMap_;
};
}

#endif
