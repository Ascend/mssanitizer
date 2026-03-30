/* -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#ifndef REGISTER_SANITIZER_REGISTER_SANITIZER_H
#define REGISTER_SANITIZER_REGISTER_SANITIZER_H

#include <map>
#include <vector>
#include "core/framework/sanitizer_base.h"
#include "core/framework/event_def.h"

namespace Sanitizer {
typedef struct {
    uint64_t regVal;
    BlockType blockType;
    uint32_t blockId;
} RegValStore;

typedef struct {
    RegisterType regType;
    std::string regNameStr;
    uint64_t regDftVal;
    bool check; // 是否检查
} CheckedRegInfo;

// 工具检测的寄存器名称及默认值信息，修改请保证type顺序与枚举RegisterType顺序一致
static const CheckedRegInfo g_regInfoTbl[static_cast<size_t>(RegisterType::MAX)] = {
    {RegisterType::VECTOR_MASK_0,   "VECTOR_MASK_0",    ~0ULL,      true},
    {RegisterType::VECTOR_MASK_1,   "VECTOR_MASK_1",    ~0ULL,      true},
    {RegisterType::CTRL,            "CTRL[56]",         0ULL,       true},  // ctrl寄存器仅检测关心的bit56是否归为0
    {RegisterType::FFTS_BASE_ADDR,  "FFTS_BASE_ADDR",   0ULL,       true},
    {RegisterType::FPC,             "FPC",              0ULL,       true},
    {RegisterType::QUANT_PRE,       "QUANT_PRE",        0ULL,       true},
    {RegisterType::QUANT_POST,      "QUANT_POST",       0ULL,       false},
    {RegisterType::LRELU_ALPHA,     "LRELU_ALPHA",      0ULL,       true},
};

// 实现寄存器未重置状态检测
class RegisterSanitizer : public SanitizerBase {
public:
    bool SetDeviceInfo(DeviceInfoSummary const &deviceInfo, Config const &config) override;
    bool SetKernelInfo(KernelSummary const &kernelInfo) override;
    void Do(const SanitizerRecord &record, const std::vector<SanEvent> &events) override;
    void ParseOnlineError(const KernelErrorRecord &record, BlockType blockType, uint64_t serialNo) override;
    bool CheckRecordBeforeProcess(const SanitizerRecord &record) override;
    void RegisterNotifyFunc(const MSG_FUNC &func) override;
    void Exit() override;

private:
    bool Init();
    bool CheckRegisterVal(uint64_t regVal, uint64_t regDftVal, const CheckedRegInfo &checkInfo);
    void ReportRegisterInfo(RegisterDispInfo &info);

    int16_t checkBlockId_ = CHECK_ALL_BLOCK;
    MSG_FUNC msgFunc_;
    bool isFinished_{false};
    RegValStore regValActual_[C220_A2_A3_MAXCORE_NUM][static_cast<size_t>(RegisterType::MAX)] = {0};

    // 当前正在检测的算子的信息
    std::string kernelName_;
};
}

#endif  // REGISTER_SANITIZER_REGISTER_SANITIZER_H