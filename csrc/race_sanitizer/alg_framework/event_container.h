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


#ifndef RACE_SANITIZER_ALG_FRAMEWORK_EVENT_CONTAINER_H
#define RACE_SANITIZER_ALG_FRAMEWORK_EVENT_CONTAINER_H

#include <queue>
#include <memory>
#include "core/framework/event_def.h"

namespace Sanitizer {

// 该类使用若干个队列存储事件，记录各PIPE里待执行的事件。一个类对象记录了一个AICore上的所有事件。
class EventContainer {
public:
    // 初始化存储队列
    void Init(uint32_t blockNum, uint32_t deviceNum = 1);
    // 将事件保存到对应的队列中
    void Push(const SanEvent &e, PipeType pipe, uint32_t blockIdx, uint32_t deviceIdx = 0);
    // 获取当前队列头部事件
    SanEvent Front();
    // 将当前队列的头部事件出队，表示事件处理完成
    void Pop();
    // 判断当前队列为空
    bool IsCurQueEmpty() const;
    // 判断是否满足条件切换到下一个block
    bool IsNeedSwitchNextBlock() const;
    // 判断是否满足条件切换到下一个 device
    bool IsNeedSwitchNextDevice() const;
    // 所有队列为空
    bool IsEmpty() const;
    // 检查当前 device 是否被阻塞，并更新一共被阻塞的 device 数量
    void CheckCurDeviceStuck();
    // 检查是否所有 device 都被阻塞
    bool IsAllDeviceStuck() const;
    // 打印卡住的事件serialNo
    void PrintStuckSerialNo() const;
    // 切换到下一个 device
    void SwitchToNextDevice();
    // 切换到下一个block的PIPES队列
    void SwitchToNextBlock();
    // 切换到block内下一个PIPE进行事件出队
    void SwitchToNextPipe();
    // 获取当前block内pipe编号
    PipeType GetPipeIndex() const;
    // 获取当前队列编号
    uint32_t GetQueIndex() const;
    // 获取当前block编号
    uint32_t GetBlockIndex() const;
    // 计算全局 pipe 索引
    uint32_t FlattenPipeIdx(uint32_t pipeIdx, uint32_t blockIdx, uint32_t deviceIdx) const;

    // 测试接口
    uint32_t GetCurQueSize() const;
    uint32_t GetAllQueSize() const;
    void SetQueIndex(PipeType pipe);
private:
    // 当前block内事件队列是否为空
    bool IsCurBlockEmpty() const;
    // 当前 device 内事件队列是否为空
    bool IsCurDeviceEmpty() const;

private:
    std::vector<std::queue<SanEvent>> ques_;
    uint32_t deviceNum_ = 0;
    uint32_t maxBlockNum_ = 0;

    // 用于记录本轮 block 出队事件数
    uint32_t blockPopCount_ = 0U;
    // 用于记录本轮 device 出队事件数
    uint32_t devicePopCount_ = 0U;
    // 当前 block 是否阻塞，用于控制 block 切换
    bool isBlockStuck_ = false;
    // 当前 device 是否阻塞，用于控制 device 切换
    bool isDeviceStuck_ = false;
    // 记录阻塞的 device 数量，用于跳出死循环
    uint32_t stuckDeviceNum_ = 0;

    uint32_t pipeIndex_ = 0U;
    uint32_t blockIndex_ = 0U;
    uint32_t deviceIndex_ = 0U;
};
}

#endif
