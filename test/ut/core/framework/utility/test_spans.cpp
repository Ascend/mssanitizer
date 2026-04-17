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

#include <gtest/gtest.h>

#include "core/framework/utility/spans.h"

using namespace Sanitizer;

TEST(Spans, union_span_fuse_with_sides_expect_return_correct_spans)
{
    using SpansU64 = Spans<uint64_t>;
    SpansU64 spans;
    spans.Union({0, 100});
    spans.Union({200, 300});
    spans.Union({150, 200});
    spans.Union({100, 150});

    ASSERT_EQ(spans.GetSpans().size(), 1);
    ASSERT_EQ(spans.GetSpans()[0].vmin, 0);
    ASSERT_EQ(spans.GetSpans()[0].vmax, 300);
}

TEST(Spans, spans_check_has_intersection_expect_return_correct_value)
{
    using SpansU64 = Spans<uint64_t>;
    SpansU64 spans;
    spans.Union({100, 200});
    spans.Union({300, 400});

    ASSERT_FALSE(spans.HasIntersection({0, 100}));
    ASSERT_FALSE(spans.HasIntersection({200, 300}));
    ASSERT_FALSE(spans.HasIntersection({400, 500}));
    ASSERT_TRUE(spans.HasIntersection({50, 150}));
    ASSERT_TRUE(spans.HasIntersection({250, 350}));
    ASSERT_TRUE(spans.HasIntersection({50, 450}));
}