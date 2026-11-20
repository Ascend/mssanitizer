/* -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
 * MindStudio is licensed under Mulan PSL v2.
 * You may obtain a copy at http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND.
 * ------------------------------------------------------------------------- */

#ifndef CORE_FRAMEWORK_UTILITY_EVENT_RANGE_H
#define CORE_FRAMEWORK_UTILITY_EVENT_RANGE_H

#include <cstddef>
#include <vector>
#include "core/framework/event_def.h"

namespace Sanitizer {

// Borrowed events remain owned by the current work item throughout consumption.
class EventRange {
public:
    EventRange(const SanEvent *data, size_t size) : data_(data), size_(size) {}
    EventRange(const std::vector<SanEvent> &events) : data_(events.data()), size_(events.size()) {}
    const SanEvent *begin() const { return data_; }
    const SanEvent *end() const { return size_ == 0 ? data_ : data_ + size_; }
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

private:
    const SanEvent *data_;
    size_t size_;
};

} // namespace Sanitizer
#endif
