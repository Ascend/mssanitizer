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


#include "event_container.h"
#include "core/framework/utility/log.h"

namespace Sanitizer {

void EventContainer::Init(uint32_t blockNum, uint32_t deviceNum)
{
    deviceNum_ = deviceNum;
    maxBlockNum_ = blockNum;
    ques_.resize(deviceNum * blockNum * (static_cast<uint32_t>(PipeType::SIZE)));
}

// 将事件保存到对应block的PIPE队列中
void EventContainer::Push(const SanEvent &e, PipeType pipe, uint32_t blockIdx, uint32_t deviceIdx)
{
    if (blockIdx >= maxBlockNum_) {
        SAN_ERROR_LOG("Invalid blockIdx %u, serialNo %lu", blockIdx, e.serialNo);
        return;
    }
    if (deviceIdx >= deviceNum_) {
        SAN_ERROR_LOG("Invalid deviceIdx %u, serialNo %lu", deviceIdx, e.serialNo);
        return;
    }

    uint32_t pipeIndex = FlattenPipeIdx(static_cast<uint32_t>(pipe), blockIdx, deviceIdx);
    if (e.type == EventType::MEM_EVENT && e.eventInfo.memInfo.opType == AccessType::MEMCPY_BLOCKS) {
        SanEvent event = e;
        event.eventInfo.memInfo.opType = AccessType::READ;
        ques_[pipeIndex].push(event);
        event.eventInfo.memInfo.opType = AccessType::WRITE;
        ques_[pipeIndex].push(event);
        return;
    }
    ques_[pipeIndex].push(e);
}

// 获取当前PIPE的头部事件
SanEvent EventContainer::Front() const
{
    // Front前先判空, 判空由调用方做
    return ques_[FlattenPipeIdx(pipeIndex_, blockIndex_, deviceIndex_)].front();
}

void EventContainer::Pop()
{
    ++blockPopCount_;
    ++devicePopCount_;
    ques_[FlattenPipeIdx(pipeIndex_, blockIndex_, deviceIndex_)].pop();
}

bool EventContainer::IsCurQueEmpty() const
{
    return ques_[FlattenPipeIdx(pipeIndex_, blockIndex_, deviceIndex_)].empty();
}

bool EventContainer::IsCurBlockEmpty() const
{
    uint32_t startIdx = FlattenPipeIdx(0, blockIndex_, deviceIndex_);
    uint32_t endIdx = startIdx + static_cast<uint32_t>(PipeType::SIZE);
    for (auto i = startIdx; i < endIdx; ++i) {
        if (!ques_[i].empty()) {
            return false;
        }
    }

    return true;
}

bool EventContainer::IsCurDeviceEmpty() const
{
    uint32_t startIdx = FlattenPipeIdx(0, 0, deviceIndex_);
    uint32_t endIdx = startIdx + maxBlockNum_ * static_cast<uint32_t>(PipeType::SIZE);
    for (uint32_t i = startIdx; i < endIdx; ++i) {
        if (!ques_[i].empty()) {
            return false;
        }
    }
    return true;
}

bool EventContainer::IsNeedSwitchNextBlock() const
{
    if (isBlockStuck_) {
        return true;
    }
    return IsCurBlockEmpty();
}

bool EventContainer::IsNeedSwitchNextDevice() const
{
    return isDeviceStuck_ || IsCurDeviceEmpty();
}

bool EventContainer::IsEmpty() const
{
    for (const auto &it : ques_) {
        if (!it.empty()) {
            return false;
        }
    }

    return true;
}

void EventContainer::CheckCurDeviceStuck()
{
    if (isDeviceStuck_) {
        ++stuckDeviceNum_;
    } else {
        // 当前 device 有处理事件，则重新计数
        stuckDeviceNum_ = 0;
    }
}

bool EventContainer::IsAllDeviceStuck() const
{
    return stuckDeviceNum_ == deviceNum_;
}

void EventContainer::PrintStuckSerialNo() const
{
    for (size_t i = 0; i < ques_.size(); i++) {
        if (ques_[i].empty()) {
            continue;
        }
        SAN_INFO_LOG("Stuck pipe index %lu , serialNo %lu, remaining event count %lu",
            i, ques_[i].front().serialNo, ques_[i].size());
    }
}

void EventContainer::SwitchToNextDevice()
{
    deviceIndex_ = (deviceIndex_ + 1U) % deviceNum_;
    blockIndex_ = 0U;
    pipeIndex_ = 0U;
    blockPopCount_ = 0U;
    devicePopCount_ = 0U;
    isBlockStuck_ = false;
    isDeviceStuck_ = false;
}

void EventContainer::SwitchToNextBlock()
{
    blockIndex_ = (blockIndex_ + 1U) % maxBlockNum_;
    pipeIndex_ = 0U;
    blockPopCount_ = 0U;
    isBlockStuck_ = false;
    // 遍历完一轮 block 后，检查当前 device 是否阻塞
    if (blockIndex_ == 0U) {
        isDeviceStuck_ = devicePopCount_ == 0U;
        // 当前 device 有处理过事件认为未阻塞，阻塞 device 数量重新计数
        if (!isDeviceStuck_) {
            stuckDeviceNum_ = 0;
        }
        devicePopCount_ = 0U;
    }
}

void EventContainer::SwitchToNextPipe()
{
    pipeIndex_ = (pipeIndex_ + 1U) % static_cast<uint32_t>(PipeType::SIZE);
    // 遍历完一轮 pipe 后，检查当前 block 是否阻塞
    if (pipeIndex_ == static_cast<uint32_t>(PipeType::PIPE_S)) {
        isBlockStuck_ = blockPopCount_ == 0U;
        blockPopCount_ = 0U;
    }
}

PipeType EventContainer::GetPipeIndex() const
{
    return static_cast<PipeType>(pipeIndex_);
}

uint32_t EventContainer::GetQueIndex() const
{
    return FlattenPipeIdx(pipeIndex_, blockIndex_, deviceIndex_);
}

uint32_t EventContainer::GetBlockIndex() const
{
    return blockIndex_;
}

uint32_t EventContainer::GetCurQueSize() const
{
    return ques_[(blockIndex_ * (static_cast<uint32_t>(PipeType::SIZE))) + pipeIndex_].size();
}

uint32_t EventContainer::GetAllQueSize() const
{
    uint32_t sum = 0U;
    for (const auto &it: ques_) {
        sum += it.size();
    }

    return sum;
}

void EventContainer::SetQueIndex(PipeType pipe)
{
    pipeIndex_ = static_cast<uint32_t>(pipe);
}

uint32_t EventContainer::FlattenPipeIdx(uint32_t pipeIdx, uint32_t blockIdx, uint32_t deviceIdx) const
{
    return deviceIdx * maxBlockNum_ * static_cast<uint32_t>(PipeType::SIZE) +
        blockIdx * static_cast<uint32_t>(PipeType::SIZE) + pipeIdx;
}

}