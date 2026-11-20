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

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <utility>

#include "core/framework/runtime_context.h"
#include "core/framework/utility/singleton.h"

using namespace Sanitizer;

namespace {
class ProbeSingleton : public ThreadSingleton<ProbeSingleton> {
public:
    ProbeSingleton() = default;
    int value{0};
};

// 同时存活两个线程，保证拿到的 thread::id 互不相同（避免线程 id 复用）
std::pair<std::thread::id, std::thread::id> CaptureTwoRootIds()
{
    std::atomic<int> ready{0};
    std::thread::id first;
    std::thread::id second;
    std::thread t1([&first, &ready]() {
        first = std::this_thread::get_id();
        ++ready;
        while (ready.load() < 2) { }
    });
    std::thread t2([&second, &ready]() {
        second = std::this_thread::get_id();
        ++ready;
        while (ready.load() < 2) { }
    });
    t1.join();
    t2.join();
    return {first, second};
}
} // namespace

TEST(ThreadSingleton, same_root_shares_instance_across_threads)
{
    const auto roots = CaptureTwoRootIds();
    const auto saved = RuntimeContext::Instance().rootTid_;
    RuntimeContext::Instance().rootTid_ = roots.first;
    ProbeSingleton &instance = ProbeSingleton::Instance();
    instance.value = 42;

    ProbeSingleton *fromWorker = nullptr;
    std::thread worker([&fromWorker, &roots]() {
        RuntimeContext::Instance().rootTid_ = roots.first;
        fromWorker = &ProbeSingleton::Instance();
    });
    worker.join();

    EXPECT_EQ(fromWorker, &instance);
    EXPECT_EQ(fromWorker->value, 42);
    RuntimeContext::Instance().rootTid_ = saved;
}

TEST(ThreadSingleton, different_roots_are_isolated_and_cache_invalidates)
{
    const auto roots = CaptureTwoRootIds();
    const auto saved = RuntimeContext::Instance().rootTid_;

    RuntimeContext::Instance().rootTid_ = roots.first;
    ProbeSingleton *first = &ProbeSingleton::Instance();
    RuntimeContext::Instance().rootTid_ = roots.second;
    ProbeSingleton *second = &ProbeSingleton::Instance();
    EXPECT_NE(first, second);

    // 同一线程切换 rootTid：缓存必须失效并命中各自实例
    RuntimeContext::Instance().rootTid_ = roots.first;
    EXPECT_EQ(&ProbeSingleton::Instance(), first);
    RuntimeContext::Instance().rootTid_ = roots.second;
    EXPECT_EQ(&ProbeSingleton::Instance(), second);
    RuntimeContext::Instance().rootTid_ = saved;
}
