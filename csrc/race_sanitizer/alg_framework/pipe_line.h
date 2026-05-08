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


#ifndef RACE_SANITIZER_ALG_FRAMEWORK_PIPE_LINE_H
#define RACE_SANITIZER_ALG_FRAMEWORK_PIPE_LINE_H

#include <functional>
#include "event_container.h"

namespace Sanitizer {

enum class ReturnType : int8_t {
    PROCESS_OK,
    PROCESS_STALLED,
};

// 定义PIPE流水执行状态机各状态
enum class PipeState : int8_t {
    CHECK_ALL_DEVICE,
    CHECK_CUR_DEVICE,
    CHECK_CUR_BLOCK,
    CHECK_CUR_PIPE,
    PROCESS_EVENT,
    EXIT,
    EXCEPTION,
};
// 这个类用于控制pipe流水执行
class PipeLine {
public:
    using EventFunc = std::function<ReturnType(const SanEvent&)>;
    explicit PipeLine(EventContainer &e) : events_(e) {};
    void RegisterEventFunc(const EventFunc &f);
    void Run();
private:
    PipeState CheckAllDevice();
    PipeState CheckCurDevice();
    PipeState CheckCurBlock();
    PipeState CheckCurPipe();
    PipeState Process();

private:
    EventContainer &events_;
    EventFunc func_;
    PipeState pipeState_ = PipeState::CHECK_ALL_DEVICE;
};
}
#endif