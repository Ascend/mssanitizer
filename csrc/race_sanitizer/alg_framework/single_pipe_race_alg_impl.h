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


#ifndef RACE_SANITIZER_ALG_FRAMEWORK_SINGLE_PIPE_RACE_ALG_IMPL_H
#define RACE_SANITIZER_ALG_FRAMEWORK_SINGLE_PIPE_RACE_ALG_IMPL_H

#include <functional>
#include "race_alg_base.h"

namespace Sanitizer {

/// 适用场景：单算子核内单一流水数据竞争检测
/// 约束：    Scalar流水之间的同步由硬件自动保证，调用PipeBarrier<PIPE_S>()
///          会引发硬件错误，因此检测流水不包括PIPE_S
/// 检测原理：流水内只保证按顺序发指令，不负责指令的结束，若两条指令同时访问
///          同一地址会产生竞争，但是如果在指令间插入pipe_barrier，则后序
///          指令会等待前序指令完成后再开始，不产生竞争。因此需要结合时间和
///          空间综合判断。
/// 实现方案：空间上的地址重叠复用原有算法，时间上的并发则无需使用向量时钟
///          算法，因为流水内的指令是按顺序开始，只要两条指令在同一流水中
///          相邻，即认为有竞争风险。此外还需要检测指令之间是否有插入同步，
///          方案是添加barrierNo参数，记录事件之前出现的pipe_barrier次数，
///          两个事件做竞争判断时，如果barrierNo不相等，说明中间有同步指令，
//           认为没有竞争风险。
class SinglePipeRaceAlgImpl : public RaceAlgBase {
public:
    explicit SinglePipeRaceAlgImpl(KernelType kernelType, DeviceType deviceType, uint32_t blockDim);
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
    ReturnType ProcessBufSyncEvent(const SanEvent &event);

private:
    // Scalar流水之间的同步由硬件自动保证，调用PipeBarrier<PIPE_S>()会引发硬件错误，因此PIPE_S除外
    static constexpr uint8_t MAX_PIPE_NUM_FOR_BARRIER = static_cast<uint8_t>(PipeType::SIZE) - 1;
    // 记录每条流水线上出现pipe_barrier指令的个数，除了PIPE_S
    std::array<uint32_t, MAX_PIPE_NUM_FOR_BARRIER> pipeBarrierNo_;
    // 对每条流水的serialNo单独排序，用于检测同一流水上的两条指令是否相邻
    // 2维数组，外层是pipe，里层是{最新的serialNo, 累加的pipeSerialNo}
    std::array<std::array<uint64_t, 2>, MAX_PIPE_NUM_FOR_BARRIER> pipeSerialNo_;
};

}  // namespace Sanitizer
#endif
