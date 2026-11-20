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

#include <atomic>
#include <stdexcept>

#include "core/framework/parse_pipeline.h"

using namespace Sanitizer;

namespace {
ParseTask MakeTask(uint32_t kernelIdx)
{
    ParseTask task;
    task.ctx.kernelIdx = kernelIdx;
    return task;
}
} // namespace

TEST(OrderedParsePipeline, parse_exception_is_rethrown_by_drain_and_remaining_tasks_continue)
{
    std::atomic<int> committed{0};
    OrderedParsePipeline pipeline(2, 4,
        [](ParseTask &&task) -> ParseOut {
            if (task.ctx.kernelIdx == 1) {
                throw std::runtime_error("parse boom");
            }
            ParseOut out;
            out.ctx = task.ctx;
            return out;
        },
        [&committed](ParseOut &&) { ++committed; });

    for (uint32_t i = 0; i < 3; ++i) {
        pipeline.Submit(MakeTask(i));
    }

    EXPECT_THROW(pipeline.Drain(), std::runtime_error);
    EXPECT_EQ(committed.load(), 2);   // 失败任务无结果不提交，其余任务继续提交
}

TEST(OrderedParsePipeline, commit_exception_is_rethrown_and_remaining_tasks_still_commit)
{
    std::atomic<int> attempts{0};
    OrderedParsePipeline pipeline(2, 4,
        [](ParseTask &&task) {
            ParseOut out;
            out.ctx = task.ctx;
            return out;
        },
        [&attempts](ParseOut &&out) {
            ++attempts;
            if (out.ctx.kernelIdx == 1) {
                throw std::runtime_error("commit boom");
            }
        });

    for (uint32_t i = 0; i < 3; ++i) {
        pipeline.Submit(MakeTask(i));
    }

    EXPECT_THROW(pipeline.Drain(), std::runtime_error);
    EXPECT_EQ(attempts.load(), 3);   // 异常后不跳过，后续任务仍提交
}
