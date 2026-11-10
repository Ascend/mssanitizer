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
#include "core/framework/pipeline_replayer.h"

namespace Sanitizer {

// 该类本身功能主要是支持地址和值有效性的检测
// 其本身利用代理的完成实际功能，但是其接口定义了对外的约束
// 调用方需要按照要求格式输入操作内容并明确事情发生后的通知函数
class AddressSanitizer : public SanitizerBase {
public:
    AddressSanitizer();
    // 销毁前撤销跨卡查询对本对象的引用。
    ~AddressSanitizer() override;
    bool SetDeviceInfo(DeviceInfoSummary const &deviceInfo, Config const &config) override;
    bool SetKernelInfo(KernelSummary const &kernelInfo) override;
    void Do(const SanitizerRecord &record, const std::vector<SanEvent> &events) override;
    void DoEvents(const SanitizerRecord &record, EventRange events) override;
    void ParseOnlineError(const KernelErrorRecord &record, BlockType blockType, uint64_t serialNo) override;
    bool CheckRecordBeforeProcess(const SanitizerRecord &record) override;
    void RegisterNotifyFunc(const MSG_FUNC &func) override;
    void Exit() override;
    // 查询本设备当前选用的 GM 内存范围表是否完整允许指定读写访问；调用方负责并发保护
    bool CheckExistingGmRange(uint64_t addr, uint64_t size, AccessType accessType) const;

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
    static void ConvertMemEvent(const SanEvent &event, std::vector<MemOpRecord> &records);
    static void ConvertDynamicMemEvent(const SanEvent &event, std::vector<MemOpRecord> &records);
    void DoMemOpRecord(MemOpRecord const &record, bool reduce);
    size_t GetRecordsNum(EventRange events) const;
    void ReportErrorMsg();
    void ReportAfterKernelFinish();
    void CheckNonDefaultReg(const SanEvent &event);
    bool SwitchToScope(BoundsCheckScope scope);
    void BeforeScopeSwitch(MemOpRecord const &record);
    void AfterScopeSwitch(MemOpRecord const &record);
    ErrorMsgList ScopeDoAction(AsanAction& action);
    BoundsCheckScope MemInfoToScope(MemInfoSrc infoSrc);
    std::string ScopeToString(BoundsCheckScope scope);
    void UpdateHeapInfo(MemOpRecord const &record);
    void DoWithLocalTensor(const MstxRecord &record, const std::vector<MemOpRecord> &records);
    void AlertGMAddrOutOfBoundError(MemOpRecord memoryRecord);
    void SetPermission(MemRegionPermissionDesc const &desc);
    void ConvertEventsToRecords(EventRange events, std::vector<MemOpRecord> &records);
    bool NeedsInitcheckReplay() const;
    bool StreamsPrivateMemory() const;

    std::unique_ptr<ShadowMemory> shadowMemory_;
    MSG_FUNC msgFunc_;
    Config config_;
    ErrorBuffer errorBuffer_;
    BoundsCheckScope boundsCheckScope_;
    BoundsCheckScope historyBoundsCheckScope_;
    BoundsCheck boundsCheckRuntime_;
    BoundsCheck boundsCheckDfx_;
    std::map<uint64_t, uint64_t> mstxHeapCacheMap_;
    PipelineReplayer pipelineReplayer_;
    std::vector<MemOpRecord> replayedRecords_;
    // 共享空间回放时 PRIVATE 事件按记录块内流式检测，复用该缓冲区。
    std::vector<MemOpRecord> streamedPrivateRecords_;
    size_t replayedEventCount_{};
    DeviceType deviceType_ = DeviceType::INVALID;
    uint64_t preSerialNo_{0};
    // 本检测对象所属连接的被测 Host 进程 PID，用于隔离跨设备查询；-1 表示身份未知
    int32_t pid_{-1};
};
}

#endif
