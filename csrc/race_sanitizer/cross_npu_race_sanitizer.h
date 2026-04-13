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

#ifndef RACE_SANITIZER_CROSS_NPU_RACE_SANITIZER_H
#define RACE_SANITIZER_CROSS_NPU_RACE_SANITIZER_H

#include <vector>

#include "alg_framework/cross_npu_race_alg_impl.h"
#include "core/framework/config.h"
#include "core/framework/event_def.h"
#include "core/framework/utility/types.h"

namespace Sanitizer {

class CrossNpuRaceSanitizer {
public:
    using MSG_GEN = Generator<DetectionInfo>;
    using MSG_FUNC = std::function<void(const LogLv &lv, MSG_GEN &&gen)>;

    CrossNpuRaceSanitizer();
    bool SetDeviceInfo(DeviceInfoSummary const &deviceInfo, Config const &config);
    bool SetKernelInfo(KernelSummary const &kernelInfo);
    void Init();
    void Do(const SanitizerRecord &record, const std::vector<SanEvent> &events);
    void RegisterNotifyFunc(const MSG_FUNC &func);

private:
    void RaceSanitizerRecord(std::shared_ptr<std::vector<RaceDispInfo>> p) const;

private:
    MSG_FUNC msgFunc_;
    CrossNpuRaceAlgImpl raceAlg_;
};

} // namespace Sanitizer

#endif // RACE_SANITIZER_CROSS_NPU_RACE_SANITIZER_H