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


#include "call_stack.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <iterator>
#include <iostream>

#include "core/framework/constant.h"
#include "core/framework/kernel_binary_manager.h"
#include "core/framework/utility/log.h"
#include "core/framework/utility/type_traits.h"
#include "core/framework/utility/umask_guard.h"
#include "core/framework/utility/ustring.h"
#include "core/framework/utility/file_system.h"
#include "core/framework/utility/path.h"
#include "core/framework/utility/cmd.h"
#include "core/framework/runtime_context.h"

namespace {

using namespace Sanitizer;

std::vector<std::string> CreateSymbolizerInput(std::set<uint64_t> const &pcOffsets)
{
    auto transMethod = [](uint64_t pcOffset) {
        std::stringstream ss;
        ss << std::hex << "0x" << pcOffset;
        return ss.str();
    };

    std::vector<std::string> pcStringVector(pcOffsets.size());
    std::transform(pcOffsets.cbegin(), pcOffsets.cend(), pcStringVector.begin(), transMethod);
    return pcStringVector;
}

std::string RunSymbolizer(std::string const &kernelPath, std::set<uint64_t> const &pcOffsets)
{
    Path exePath;
    if (!GetSelfExePath(exePath)) {
        SAN_ERROR_LOG("Get self exe path failed.");
        return {};
    }
    Path symbolizerPath = exePath.Parent() / Path("llvm-symbolizer");
    if (!symbolizerPath.Exists()) {
        SAN_ERROR_LOG("llvm-symbolizer NOT found");
        return {};
    }

    std::vector<std::string> cmd = {
        symbolizerPath.ToString(),
        "--inlines",
        "--functions",
        "--demangle",
        "--output-style=JSON",
        "-e",
        kernelPath,
    };
    std::vector<std::string> pcStringVector = CreateSymbolizerInput(pcOffsets);
    cmd.insert(cmd.end(), pcStringVector.cbegin(), pcStringVector.cend());

    std::string output;
    constexpr std::size_t symbolizerMaxTry = 3;
    for (std::size_t i = 0; i < symbolizerMaxTry; ++i) {
        output.clear();
        if (PipeCall(cmd, output)) {
            return output;
        }
    }

    SAN_ERROR_LOG("symbolizer call FAILED");
    return output;
}

inline uint64_t ParsePcOffset(std::string const &pcOffsetStr)
{
    std::stringstream ss(pcOffsetStr);
    uint64_t pcOffset {};
    ss >> std::hex >> pcOffset;
    return pcOffset;
}

inline std::string CreateKernelFileName(void)
{
    return "kernel.o." + std::to_string(getpid()) + "_" + std::to_string(pthread_self());
}

} // namspace [Dummy]

namespace Sanitizer {

CallStack::~CallStack(void)
{
    for (auto const &kernelPath : kernelPaths_) {
        // try to clean up kernel.o cache
        remove(kernelPath.c_str());
    }
}

std::string CallStack::Load(std::vector<char> const &binary)
{
    Path kernelPath = Path(OUTPUT_DIR) / Path(CreateKernelFileName());
    std::string const &kernelStr = kernelPath.ToString();
    UmaskGuard umaskGuard(REGULAR_MODE_MASK);
    if (kernelPath.Exists()) {
        if (remove(kernelStr.c_str()) != 0) {
            printf("[mssanitizer] WARNING: Failed to remove old kernel binary file (%s). Please remove it to enable "
                   "callstack.\n",
                kernelStr.c_str());
        }
    }
    Path kernelDirPath = kernelPath.Parent();
    auto kernelDirStr = kernelDirPath.ToString();
    if (!kernelDirPath.Exists()) {
        if (mkdir(kernelDirStr.c_str(), Sanitizer::DIR_FILE_MODE) != 0) {
            printf("[mssanitizer] ERROR: Failed to create directory (%s). Please make sure current user has write "
                   "permission to the working directory.\n", kernelDirStr.c_str());
            return "";
        }
    }
    auto logger = [](std::string const &msg) { std::cout << "[mssanitizer] "<< msg << std::endl; };
    if (!IsFilePermSafe(kernelDirStr, DIR_FILE_MODE, logger)) {
        return "";
    }
    if (IsSoftLink(kernelDirStr)) {
        printf("[mssanitizer] ERROR: The kernel binary (%s) is soft link.\n", kernelDirStr.c_str());
        return "";
    }
    if (!WriteBinary(kernelStr, binary.data(), binary.size())) {
        printf("[mssanitizer] ERROR: Callstack may be unavailable: Failed to store kernel binary. Please make sure the "
               "user has write permission to the directory (%s).\n",
            kernelDirStr.c_str());
        return "";
    }

    std::lock_guard<std::mutex> guard{mtx_};
    kernelPaths_.emplace_back(kernelStr);
    return kernelStr;
}

CallStack::Stack CallStack::Query(std::string const &kernelName, uint64_t pcOffset)
{
    {
        std::lock_guard<std::mutex> guard{this->mtx_};
        auto kernelIt = as_const(pcStackMap_).find(kernelName);
        if (kernelIt != this->pcStackMap_.cend()) {
            auto stackIt = kernelIt->second.find(pcOffset);
            if (stackIt != kernelIt->second.cend()) {
                return stackIt->second;
            }
        }
    }

    if (pcOffset == INVALID_PC_OFFSET) {
        SAN_WARN_LOG("Failed to query callstack: query call stack with invalid pc offset");
        return {};
    }

    SAN_INFO_LOG("Pc offset %#lx missed in cahced stack map. Rebuild stack map", pcOffset);
    this->CachePcOffsets(kernelName, { pcOffset });

    std::lock_guard<std::mutex> guard{this->mtx_};
    return this->pcStackMap_[kernelName][pcOffset];
}

void CallStack::CachePcOffsets(std::string const &kernelName, std::set<uint64_t> pcOffsets)
{
    // 过滤 INVALID_PC_OFFSET
    pcOffsets.erase(INVALID_PC_OFFSET);

    StackEachKernel stacks;
    {
        // 过滤已缓存过的 pcOffset
        std::lock_guard<std::mutex> guard{this->mtx_};
        stacks = pcStackMap_[kernelName];
    }
    for (auto it = pcOffsets.begin(); it != pcOffsets.cend();) {
        if (as_const(stacks).find(*it) != stacks.cend()) {
            it = pcOffsets.erase(it);
        } else {
            ++it;
        }
    }

    if (pcOffsets.empty()) {
        // 当 pc offset 列表为空时，llvm-symbolizer 会等待标准输入导致进程卡住，需要直接返回
        SAN_INFO_LOG("PcOffsets list is empty, skip cache build");
        return;
    }

    SAN_INFO_LOG("Build stack map with %zu pc offsets", pcOffsets.size());

    std::vector<char> binary;
    if (!KernelBinaryManager::Instance().Get(kernelName, binary)) {
        SAN_ERROR_LOG("Get kernel binary from KernelBinaryManager failed. kernelName: %s", kernelName.c_str());
        return;
    }
    std::string kernelPath = this->Load(binary);
    if (kernelPath.empty()) {
        return;
    }
    std::string symbolizerOutput = RunSymbolizer(kernelPath, pcOffsets);
    StackEachKernel stackCacheMap = this->ParseStacks(symbolizerOutput);

    std::lock_guard<std::mutex> guard{this->mtx_};
    pcStackMap_[kernelName].insert(stackCacheMap.begin(), stackCacheMap.end());
}

void CallStack::ParseEachStack(nlohmann::json const &stackJson, StackEachKernel &stackCacheMap) const
{
    std::string stackJsonLog = Utility::ReplaceInvalidChar(stackJson.dump());
    if (stackJson.contains("Error")) {
        SAN_ERROR_LOG("Query pc offset failed. message: %s", stackJsonLog.c_str());
        return;
    }

    if (!stackJson.contains("Symbol") || !stackJson["Symbol"].is_array()) {
        SAN_ERROR_LOG("No symbols in stack JSON. JSON data: %s", stackJsonLog.c_str());
        return;
    }

    if (!stackJson.contains("Address")) {
        SAN_ERROR_LOG("No Address in stack JSON. JSON data: %s", stackJsonLog.c_str());
        return;
    }
    Location location;
    Stack stack;
    for (nlohmann::json const &frameJson : stackJson["Symbol"]) {
        location.fileName = frameJson["FileName"].get<std::string>();
        location.funcName = frameJson["FunctionName"].get<std::string>();
        location.line = frameJson["Line"].get<uint64_t>();
        location.column = frameJson["Column"].get<uint64_t>();
        stack.emplace_back(location);
    }

    // 如果第一个 frame 行号列号为 0 则忽略这个 frame
    if (!stack.empty() && stack[0].line == 0 && stack[0].column == 0) {
        stack.erase(stack.begin());
    }

    std::string pcOffsetStr = stackJson["Address"].get<std::string>();
    uint64_t pcOffset = ParsePcOffset(pcOffsetStr);
    stackCacheMap[pcOffset] = std::move(stack);
}

CallStack::StackEachKernel CallStack::ParseStacks(std::string const &msg) const
{
    std::vector<std::string> lines;
    std::string stripped = Utility::RStrip(msg, "\n");
    // 根据输入的多个 pc offset 的行数，会产生若干行结果，每行都是一个 JSON 对象
    Utility::Split(stripped, std::back_inserter(lines), "\n", true);
    std::string msgLog = Utility::ReplaceInvalidChar(msg);

    StackEachKernel stackCacheMap;
    nlohmann::json jsonObject;
    std::string jsonObjectLog = Utility::ReplaceInvalidChar(jsonObject.dump());
    for (std::string const &line : lines) {
        try {
            jsonObject = nlohmann::json::parse(line);
        } catch (nlohmann::json::parse_error &e) {
            SAN_ERROR_LOG("Parse symbolizer output as JSON failed. JSON data: %s", msgLog.c_str());
            continue;
        }

        // 当一行包含多个 pc offset 时，输出的 JSON 为 array 类型
        if (jsonObject.is_array()) {
            for (nlohmann::json const &stackJson : jsonObject) {
                ParseEachStack(stackJson, stackCacheMap);
            }
        } else if (jsonObject.is_object()) {
            ParseEachStack(jsonObject, stackCacheMap);
        } else {
            SAN_ERROR_LOG("Unexpected JSON type. Array or object type expected. JSON data: %s", jsonObjectLog.c_str());
        }
    }
    return stackCacheMap;
}

std::ostream &CallStack::FormatCallStack(std::ostream &os, Stack const &stack) const
{
    char const *ascendHomePath = getenv("ASCEND_HOME_PATH");
    bool envValid = ascendHomePath != nullptr && ascendHomePath[0] != '\0';

    std::size_t idx = 0;
    for (auto const &frame : stack) {
        if (isPrintFullStack_ || !envValid ||
            !Path(frame.fileName).IsSubPathOf(Path(ascendHomePath))) {
            os << "======    #" << idx++ << " " << frame.fileName
               << ":" << frame.line << ":" << frame.column << std::endl;
        }
    }
    return os;
}

} // namespace Sanitizer
