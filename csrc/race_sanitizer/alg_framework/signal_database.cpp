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

#include <algorithm>

#include "core/framework/utility/type_traits.h"
#include "sanitizer_report.h"

#include "signal_database.h"

namespace Sanitizer {

void SignalDatabase::Set(MstxSignalSet const &event, VectorTime const &vt)
{
    database_[event.addr].value = event.value;
    database_[event.addr].vt = vt;
}

void SignalDatabase::Accumulate(MstxSignalSet const &event, VectorTime const &vt)
{
    auto it = database_.find(event.addr);
    if (it == database_.end()) {
        Value entry{};
        entry.value = event.value;
        entry.vt = vt;
        database_.emplace(event.addr, std::move(entry));
        return;
    }

    /// 计数型发布：同一地址由 N 个生产者分别置位、M 个消费者轮询其总额（如各核把
    /// 自己负责的行数/循环数累加到同一个 GM 计数器）。此时若覆盖，则只保留最后一个
    /// 生产者的时钟与值：等待方拿不到其余生产者的 happens-before 关系；且回放顺序与
    /// 执行顺序不一致时 value 可能被改小，导致等待永远无法满足。故合并向量时钟、
    /// value 取最大值（单调不减），使任意 WAIT(GE/GT n) 最终都能被满足。
    VectorClock::UpdateVectorTime(vt, it->second.vt);
    it->second.value = std::max(it->second.value, event.value);
}

bool SignalDatabase::Wait(MstxSignalWait const &event, VectorTime &vt)
{
    auto it = Sanitizer::as_const(database_).find(event.addr);
    if (it == database_.cend()) {
        return false;
    }

    Value const &value = it->second;
    vt = value.vt;
    switch (event.cmpOp) {
    case CompareOp::EQ:
        return value.value == event.cmpValue;
    case CompareOp::NE:
        return value.value != event.cmpValue;
    case CompareOp::GT:
        return value.value > event.cmpValue;
    case CompareOp::GE:
        return value.value >= event.cmpValue;
    case CompareOp::LT:
        return value.value < event.cmpValue;
    case CompareOp::LE:
        return value.value <= event.cmpValue;
    default:
        return false;
    }
}

} // namespace Sanitizer
