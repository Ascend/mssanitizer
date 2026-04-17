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

#ifndef CORE_FRAMEWORK_UTILITY_SPANS_H
#define CORE_FRAMEWORK_UTILITY_SPANS_H

#include <vector>

namespace Sanitizer {

template <typename T>
struct Span {
    T vmin;
    T vmax;
};

template <typename T>
class Spans {
public:
    /** @brief 与区间求并集
     */
    Spans<T> &Union(Span<T> const &span);

    /** @brief 判定是否与区间相交
     */
    bool HasIntersection(Span<T> const &span) const;

#ifdef __BUILD_TESTS__
    std::vector<Span<T>> &GetSpans() { return spans_; }
#endif

private:
    std::vector<Span<T>> spans_;
};

}  // namespace Sanitizer

#endif // CORE_FRAMEWORK_UTILITY_SPANS_H