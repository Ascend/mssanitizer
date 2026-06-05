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

#include "core/framework/utility/type_traits.h"
#include "sanitizer_report.h"

#include "signal_database.h"

namespace Sanitizer {

void SignalDatabase::Set(MstxSignalSet const &event, VectorTime const &vt)
{
    database_[event.addr].value = event.value;
    database_[event.addr].vt = vt;
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
