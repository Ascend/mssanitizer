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
        static std::mutex mtx{};
        // 调用方（如解析池线程）的 rootTid_ 在生命周期内固定，缓存后避免每个事件都加全局锁查 map。
        // std::map 节点地址稳定且从不 erase，缓存裸指针安全。
        thread_local std::thread::id cachedThreadId{};
        thread_local T *cachedInstance = nullptr;
        auto threadId = RuntimeContext::Instance().rootTid_;
        if (cachedInstance != nullptr && cachedThreadId == threadId) {
            return *cachedInstance;
        }
        std::lock_guard<std::mutex> scopeLock(mtx);
        T &instance = insts[threadId];
        cachedThreadId = threadId;
        cachedInstance = &instance;
        return instance;
    }

    ThreadSingleton(ThreadSingleton const &) = delete;
    ThreadSingleton &operator=(ThreadSingleton const &) = delete;

protected:
    ThreadSingleton(void) = default;
    ~ThreadSingleton(void) = default;
};

} // namespace Sanitizer
