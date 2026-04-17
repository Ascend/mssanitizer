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

#ifndef CORE_FRAMEWORK_UTILITY_SPANS_HPP
#define CORE_FRAMEWORK_UTILITY_SPANS_HPP

#include <algorithm>
#include <iterator>

#include "spans.h"

namespace Sanitizer {

template <typename T>
Spans<T> &Spans<T>::Union(Span<T> const &span)
{
    auto pred = [](T a, Span<T> const &b) { return a < b.vmin; };
    auto it = std::upper_bound(spans_.begin(), spans_.end(), span.vmin, pred);
    if (it != spans_.cbegin() && std::prev(it)->vmax >= span.vmin) {
        it = std::prev(it);
        it->vmax = std::max(it->vmax, span.vmax);
    } else {
        it = spans_.insert(it, span);
    }

    auto next = std::next(it);
    for (; next < spans_.cend(); ++next) {
        if (next->vmin > it->vmax) {
            break;
        }
        it->vmax = std::max(it->vmax, next->vmax);
    }
    spans_.erase(std::next(it), next);
    return *this;
}

template <typename T>
bool Spans<T>::HasIntersection(Span<T> const &span) const
{
    auto pred = [](T a, Span<T> const &b) { return a < b.vmax; };
    auto it = std::upper_bound(spans_.begin(), spans_.end(), span.vmin, pred);
    for (; it < spans_.cend(); ++it) {
        if (it->vmax > span.vmin && span.vmax > it->vmin) {
            return true;
        }

        if (it->vmin >= span.vmax) {
            return false;
        }
    }
    return false;
}

}  // namespace Sanitizer

#endif // CORE_FRAMEWORK_UTILITY_SPANS_HPP