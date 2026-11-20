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
#include <cstdint>
#include "core/framework/vector_time.h"

using namespace Sanitizer;

TEST(VectorTime, set_and_get_large_index_uses_heap)
{
    VectorTime vt;
    vt.Set(65536U, 2U);
    vt.Set(65535U, 3U);
    EXPECT_TRUE(vt.heap);
    EXPECT_EQ(vt.Get(65535U), 3U);
    EXPECT_EQ(vt.Get(65536U), 2U);
    EXPECT_EQ(vt.Get(0U), 1U);
}

TEST(VectorTime, from_full_with_large_index_uses_heap)
{
    DenseVectorTime full(65537U, 1U);
    full[65536U] = 5U;
    VectorTime vt = VectorTime::FromFull(full);
    EXPECT_TRUE(vt.heap);
    EXPECT_EQ(vt.Get(65535U), 1U);
    EXPECT_EQ(vt.Get(65536U), 5U);
}

TEST(VectorTime, merge_max_inline)
{
    VectorTime out{3U, 2U, 1U, 1U, 5U};
    VectorTime in{1U, 7U, 1U, 8U};
    out.MergeMax(in);
    EXPECT_FALSE(out.heap);
    EXPECT_EQ(out.Get(0U), 3U);
    EXPECT_EQ(out.Get(1U), 7U);
    EXPECT_EQ(out.Get(2U), 1U);
    EXPECT_EQ(out.Get(3U), 8U);
    EXPECT_EQ(out.Get(4U), 5U);
    EXPECT_EQ(out.Get(5U), 1U);
}

TEST(VectorTime, merge_max_to_heap_and_dense)
{
    VectorTime out;
    VectorTime in;
    for (uint32_t i = 0U; i < 40U; ++i) { out.Set(i, i + 2U); }
    for (uint32_t i = 40U; i < 60U; ++i) { in.Set(i, i + 3U); }
    in.Set(10U, 100U);
    out.MergeMax(in);
    EXPECT_TRUE(out.heap);
    for (uint32_t i = 0U; i < 40U; ++i) {
        EXPECT_EQ(out.Get(i), i == 10U ? 100U : i + 2U);
    }
    for (uint32_t i = 40U; i < 60U; ++i) { EXPECT_EQ(out.Get(i), i + 3U); }

    VectorTime extra;
    extra.Set(10U, 999U);
    extra.Set(60U, 2U);
    out.MergeMax(extra);
    EXPECT_EQ(out.Get(10U), 999U);
    EXPECT_EQ(out.Get(60U), 2U);
    EXPECT_EQ(out.Get(61U), 1U);
}

TEST(VectorTime, merge_max_large_index)
{
    VectorTime out;
    VectorTime in;
    out.Set(65536U, 2U);
    in.Set(65536U, 5U);
    in.Set(70000U, 7U);
    out.MergeMax(in);
    EXPECT_TRUE(out.heap);
    EXPECT_EQ(out.Get(65536U), 5U);
    EXPECT_EQ(out.Get(70000U), 7U);
    EXPECT_EQ(out.Get(0U), 1U);
}
