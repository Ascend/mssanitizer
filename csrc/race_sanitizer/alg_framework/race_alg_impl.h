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


#ifndef RACE_SANITIZER_ALG_FRAMEWORK_RACE_ALG_IMPL_H
#define RACE_SANITIZER_ALG_FRAMEWORK_RACE_ALG_IMPL_H

#include <functional>
#include "race_alg_base.h"
#include "sync_event_data_base.h"

namespace Sanitizer {

// 该类代表一种具体的竞争检测算法的实现
// 该算法的能力和适用场景是：单算子核内数据竞争检测
// 一个algController对象负责一个AICore上的竞争检测
class RaceAlgImpl : public RaceAlgBase {
public:
    explicit RaceAlgImpl(KernelType kernelType, DeviceType deviceType, uint32_t blockDim);
    // 输入：处理算法预处理后的"事件"
    void Do(const SanEvent& event) override;
    // 输出：MemEventChecker检测出来的竞争风险信息
    std::shared_ptr<std::vector<RaceDispInfo>> GetResult() const override;
    bool IsFinished() const override;
    // 测试接口
    uint32_t GetRaceCount() const;
private:
    // 事件处理函数，根据不同的事件，处理结果定义为正常处理或阻塞。
    ReturnType ProcessEvent(const SanEvent& event);
    ReturnType ProcessMemEvent(const SanEvent& event);
    ReturnType ProcessSyncEvent(const SanEvent& event);
    ReturnType ProcessTimeEvent(const SanEvent& event);
    ReturnType ProcessDynamicMemEvent(const SanEvent& event);
private:
    SyncEventDataBase syncDB_;
};
}

#endif
