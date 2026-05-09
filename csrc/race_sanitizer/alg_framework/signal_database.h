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

#ifndef RACE_SANITIZER_ALG_FRAMEWORK_SIGNAL_DATABASE_H
#define RACE_SANITIZER_ALG_FRAMEWORK_SIGNAL_DATABASE_H

#include <cstdint>
#include <unordered_map>

#include "sanitizer_report.h"
#include "vector_clock.h"

namespace Sanitizer {

class SignalDatabase {
public:
    using Key = uint64_t;

    struct Value {
        int64_t value;
        VectorTime vt;
    };

public:
    // signal set 事件，并输出对应的向量时间
    void Set(MstxSignalSet const &event, VectorTime const &vt);
    // signal wait 事件，并获取对应的向量时间
    bool Wait(MstxSignalWait const &event, VectorTime &vt);

private:
    std::unordered_map<Key, Value> database_;
};

} // namespace Sanitizer

#endif // RACE_SANITIZER_ALG_FRAMEWORK_SIGNAL_DATABASE_H