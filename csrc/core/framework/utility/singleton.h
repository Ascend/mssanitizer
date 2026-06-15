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


#pragma once

#include <thread>
#include <map>
#include <mutex>

#include "../runtime_context.h"

namespace Sanitizer {

template <typename T, typename... Args>
class Singleton {
public:
    inline static T &Instance(Args &&... args);
    Singleton(Singleton const &) = delete;
    Singleton &operator=(Singleton const &) = delete;

protected:
    Singleton(void) = default;
    ~Singleton(void) = default;
};

template <typename T, typename... Args>
T &Singleton<T, Args...>::Instance(Args &&... args)
{
    static T instance(std::forward<Args>(args)...);
    return instance;
}

template <typename T>
class ThreadSingleton {
public:
    static T &Instance()
    {
        static std::map<std::thread::id, T> insts;
        auto threadId = RuntimeContext::Instance().rootTid_;
        static std::mutex mtx{};
        std::lock_guard<std::mutex> scopeLock(mtx);
        return insts[threadId];
    }

    ThreadSingleton(ThreadSingleton const &) = delete;
    ThreadSingleton &operator=(ThreadSingleton const &) = delete;

protected:
    ThreadSingleton(void) = default;
    ~ThreadSingleton(void) = default;
};

} // namespace Sanitizer
