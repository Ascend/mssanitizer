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


#ifndef __CORE_FRAMEWORK_CALL_STACK_H__
#define __CORE_FRAMEWORK_CALL_STACK_H__

#include <set>
#include <mutex>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <ostream>
#include "core/framework/utility/singleton.h"

#include "nlohmann/json.hpp"

namespace Sanitizer {

class CallStack : public Singleton<CallStack> {
public:
    friend class Singleton<CallStack>;

    struct Location {
        std::string funcName;
        std::string fileName;
        uint64_t line;
        uint64_t column;
    };

    using Stack = std::vector<Location>;
    using StackEachKernel = std::unordered_map<uint64_t, Stack>;
    using StackCacheMap = std::unordered_map<std::string, StackEachKernel>;

public:
    /**
     * @brief 缓存指定 kernel 的 pc 对应的调用栈
     * @param kernelName [in] 要缓存的 pc 对应的 kernelName
     * @param pcOffsets [in] 要缓存的 pc 集合
     */
    void CachePcOffsets(std::string const &kernelName, std::set<uint64_t> pcOffsets);

    /**
     * @brief 查询指定 kernel 对应的 pc 的调用栈
     * @param kernelName [in] 要查询的 pc 对应的 kernelName
     * @param pcOffset [in] 要查询的 pcOffset
     */
    Stack Query(std::string const &kernelName, uint64_t pcOffset);

    void SetIsPrintFullStack(bool isPrintFullStack){ isPrintFullStack_ = isPrintFullStack; }

    bool IsPrintFullStack() const { return isPrintFullStack_; }

    /**
     * @brief 格式化调用栈
     */
    std::ostream &FormatCallStack(std::ostream &os, Stack const &stack) const;

private:
    CallStack(void) { }
    ~CallStack(void);

    std::string Load(std::vector<char> const &binary);
    StackEachKernel ParseStacks(std::string const &msg) const;
    void ParseEachStack(nlohmann::json const &stackJson, StackEachKernel &stackCacheMap) const;

    mutable StackCacheMap pcStackMap_;
    mutable std::mutex mtx_;
    bool isPrintFullStack_{false};
    std::vector<std::string> kernelPaths_;
};

} // namespace Sanitizer

#endif  // __CORE_FRAMEWORK_CALL_STACK_H__
