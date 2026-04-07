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

#ifndef RACE_SANITIZER_ALG_FRAMEWORK_CROSS_NPU_RACE_ALG_IMPL_H
#define RACE_SANITIZER_ALG_FRAMEWORK_CROSS_NPU_RACE_ALG_IMPL_H

#include <vector>

#include "core/framework/config.h"
#include "core/framework/event_def.h"
#include "cross_core_sync_info_container.h"
#include "event_container.h"
#include "mem_event_checker.h"
#include "pipe_line.h"
#include "sync_event_data_base.h"
#include "vector_clock.h"

namespace Sanitizer {

class CrossNpuRaceAlgImpl {
public:
    CrossNpuRaceAlgImpl();

    void Init();
    bool SetDeviceInfo(DeviceInfoSummary const &deviceInfo, Config const &config);
    bool SetKernelInfo(KernelSummary const &kernelInfo);
    void Do(const SanEvent& event);
    std::shared_ptr<std::vector<RaceDispInfo>> GetResult() const;
    bool IsFinished() const;

private:
    ReturnType ProcessEvent(const SanEvent& event);
    ReturnType ProcessMemEvent(const SanEvent& event);
    ReturnType ProcessSyncEvent(const SanEvent& event);
    ReturnType ProcessTimeEvent(const SanEvent& event);
    ReturnType ProcessBlockSoftSyncEvent(const SanEvent& event);
    ReturnType ProcessBlockSyncEvent(const SanEvent& event);
    ReturnType ProcessMstxCrossSyncEvent(const SanEvent& event);
    void CacheMstxCrossSet(const SanEvent& event);

private:
    using MstxSetCrossMap = std::map<std::pair<uint64_t, uint64_t>, uint64_t>;

    bool isFinished_{};
    uint32_t totalBlockNum_{};

    EventContainer eventContainer_;
    MemEventChecker memChecker_;
    // 按最大的 blockDim 数初始化 vc 和 syncDB 数组
    std::vector<VectorTime> vc_;
    std::vector<SyncEventDataBase> syncDB_;
    // 为每张卡上的每个 kernel 创建一份核间检测实例
    std::vector<std::vector<MstxSetCrossMap>> mstxSetCrossMap_;
    std::vector<std::vector<CrossCoreSyncInfoContainer>> crossCoreSyncInfoContainer_;
};

} // namespace Sanitizer

#endif // RACE_SANITIZER_ALG_FRAMEWORK_CROSS_NPU_RACE_ALG_IMPL_H