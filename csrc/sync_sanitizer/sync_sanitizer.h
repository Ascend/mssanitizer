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


#ifndef SYNC_SANITIZER_SYNC_SANITIZER_H
#define SYNC_SANITIZER_SYNC_SANITIZER_H

#include <map>
#include <vector>
#include "core/framework/sanitizer_base.h"
#include "core/framework/pipeline_replayer.h"

namespace Sanitizer {

// 该类实现synccheck检测，当前功能是检测并上报多余的set_flag同步指令
class SyncSanitizer : public SanitizerBase {
public:
    bool SetDeviceInfo(DeviceInfoSummary const &deviceInfo, Config const &config) override;
    bool SetKernelInfo(KernelSummary const &kernelInfo) override;
    void Do(const SanitizerRecord &record, const std::vector<SanEvent> &events) override;
    void ParseOnlineError(const KernelErrorRecord &record, BlockType blockType, uint64_t serialNo) override;
    bool CheckRecordBeforeProcess(const SanitizerRecord &record) override;
    void RegisterNotifyFunc(const MSG_FUNC &func) override;
    void Exit() override;

private:
    void Init(KernelSummary const &kernelInfo);
    inline bool IsTargetBlockId(uint32_t blockId);
    uint64_t CalcSetFlagSyncID(SanEvent const &event);
    uint64_t CalcWaitFlagSyncID(SanEvent const &event);
    void DoMatchCheck(SanEvent const &event);
    void DoRedundancyCheck(SanEvent const &event);
    void DoStuckCheck(SanEvent const &event);
    void ReportUnpairedInfo();
    void ReportRedundancyInfo() const;
    void ReportSyncThreadsInfo() const;
    void ReportSyncStuckInfo(SyncStuckDspInfo &stuckDspInfo);

    KernelType kernelType_{};
    DeviceType deviceType_ = DeviceType::INVALID;
    PipelineReplayer pipelineReplayer_;
    int16_t checkBlockId_ = CHECK_ALL_BLOCK;
    MSG_FUNC msgFunc_;
    std::map<uint64_t, std::vector<SyncDispInfo>> syncEvents_;
    std::map<PipeType, SyncDispInfo> pipeRedundancyEvents_;  // 指令遍历过程中保存上一条指令
    std::vector<SyncDispInfo> redundancyInfo_;  // 保存重复的指令信息
    std::vector<ErrorEvent> syncThreadsInfo_;   // sync_threads使用错误的online_error信息
    std::vector<ErrorEvent> stuckEvents_; // PipelineReplayer 回调收集的卡死事件
    bool isFinished_{false};
};
}

#endif  // SYNC_SANITIZER_SYNC_SANITIZER_H
