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


#ifndef RACE_SANITIZER_RACE_SANITIZER_H
#define RACE_SANITIZER_RACE_SANITIZER_H

#include <array>
#include <deque>
#include <memory>
#include <queue>

#include "core/framework/sanitizer_base.h"
#include "alg_framework/race_alg_base.h"

namespace Sanitizer {

// 该类是整个竞争检测的总入口，向上对接mssanitizer框架，向下管理协调具体的检测算法实例。
// 实现的功能：
// 完成桩函数record信息预处理
// 根据输入信息/模式加载对应的算法实例，并负责销毁
// 将预处理后数据分发给对应的算法实例
// 检测结果呈现展示
class RaceSanitizer : public SanitizerBase {
public:
    bool SetDeviceInfo(DeviceInfoSummary const &deviceInfo, Config const &config) override;
    bool SetKernelInfo(KernelSummary const &kernelInfo) override;
    void Do(const SanitizerRecord &record, const std::vector<SanEvent> &events) override;
    void ParseOnlineError(const KernelErrorRecord &record, BlockType blockType, uint64_t serialNo) override;
    bool CheckRecordBeforeProcess(const SanitizerRecord &record) override;
    void RegisterNotifyFunc(const MSG_FUNC &func) override;
    void Exit() override;

private:
    // 核内竞争检测
    void SingleBlockRaceCheck(const std::vector<SanEvent> &events);
    // 核内单一流水竞争检测
    void SinglePipeRaceCheck(const std::vector<SanEvent> &events);
    // 核间竞争检测
    void AllBlockRaceCheck(const std::vector<SanEvent> &events);
    void RaceSanitizerRecord(std::shared_ptr<std::vector<RaceDispInfo>> p);
    // 检查记录是否是指定的核
    inline bool IsTargetBlockId(uint32_t blockId);
    // 检查记录是否是需要的指令记录
    bool IsTargetEvent(const SanEvent &event, BlockType targetBlockType);

    void MergeSimtErrors();
    void DoImpl(const SanitizerRecord &record, const std::vector<SanEvent> &events);
    void PopWindowRecord(std::deque<KernelRecord> &aheadDcci, std::deque<KernelRecord> &afterDccis);
    uint32_t NearestDcciDistance(KernelRecord const &kernelRecord, SanEvent const &event,
        std::deque<KernelRecord> &aheadDcci, std::deque<KernelRecord> &afterDccis) const;
    bool DoesDcciHaveEffect(SanEvent const &event, DcciRecord const &dcciRecord) const;

    // 最多支持检测开启5个检测算法
    static constexpr uint32_t MAX_RACE_ALG_NUM = 5U;
    // recordWindow_ 的大小
    static constexpr std::size_t MAX_WINDOW_SIZE = 4;
    std::array<std::shared_ptr<RaceAlgBase>, MAX_RACE_ALG_NUM> raceAlgs_;
    int16_t checkBlockId_ = CHECK_ALL_BLOCK;
    const uint8_t defaultCheckBlockId_ = 0U;
    MSG_FUNC msgFunc_;
    DeviceType deviceType_ = DeviceType::INVALID;
    std::shared_ptr<std::vector<RaceDispInfo>> simtErrors_;
    std::queue<std::pair<SanitizerRecord, std::vector<SanEvent>>> recordWindow_;
};
}
#endif
