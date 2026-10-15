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
#include <cstring>
#include <mutex>
#include <sstream>
#include <iterator>
#include <iostream>
#include <unordered_map>
#include <vector>

#include <elf.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "core/framework/constant.h"
#include "core/framework/kernel_binary_manager.h"
#include "core/framework/utility/log.h"
#include "core/framework/utility/type_traits.h"
#include "core/framework/utility/umask_guard.h"
#include "core/framework/utility/ustring.h"
#include "core/framework/utility/file_system.h"
#include "core/framework/utility/elf_loader.h"
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

inline std::string CreateKernelFileName(uint64_t seq)
{
    return "kernel.o." + std::to_string(getpid()) + "_" + std::to_string(pthread_self()) + "_" +
        std::to_string(seq);
}

// 只为 llvm-symbolizer 实际读取的内容构建「稀疏」内核镜像：
//  - 有 .debug* 段：仅落盘 .debug*（+ ELF 头/程序头/节头表/节名字表），其余留空洞；
//  - 无 .debug* 段：落盘除 .symtab/.strtab 外的全部节（全量镜像 - 符号表）；
bool WriteSlimKernelForSymbolizer(std::string const &path, BinaryBuffer const &binary) {
    Sanitizer::detail::ElfSections elf;
    if (!Sanitizer::detail::ParseElf(binary, elf)) {
        return false;
    }
    auto sectionName = [&elf](const Elf64_Shdr &sh) {
        if (sh.sh_name >= elf.shstrtab.size()) {
            return std::string {};
        }
        std::size_t maxLen = static_cast<std::size_t>(elf.shstrtab.size() - sh.sh_name);
        const char *name = elf.shstrtab.data() + sh.sh_name;
        return std::string(name, strnlen(name, maxLen));
    };
    auto isDebug = [](std::string const &name) { return name.compare(0, 6, ".debug") == 0; };
    auto isSymbolTable = [](std::string const &name) { return name == ".symtab" || name == ".strtab"; };

    bool hasDebug = false;
    for (Elf64_Shdr &sh : elf.shdrs) {
        std::string name = sectionName(sh);
        if (isDebug(name)) {
            hasDebug = true;
        } else if (isSymbolTable(name)) {
            sh.sh_type = SHT_NOBITS;
            sh.sh_offset = 0;
            sh.sh_size = 0;
            sh.sh_link = 0;
            sh.sh_info = 0;
            sh.sh_addralign = 0;
            sh.sh_entsize = 0;
        }
    }
    const bool materializeAll = !hasDebug;

    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        return false;
    }
    // 先确定最终大小：未写入的区域即空洞，不分配物理块
    bool ok = ftruncate(fd, static_cast<off_t>(binary.size())) == 0;
    auto writeAt = [&](uint64_t off, const void *buf, uint64_t len) {
        return off + len <= binary.size() && pwrite(fd, buf, len, static_cast<off_t>(off)) == static_cast<ssize_t>(len);
    };
    if (ok) {
        ok = writeAt(0, binary.data(), sizeof(Elf64_Ehdr));
    }
    if (ok && elf.ehdr.e_phnum != 0 && elf.ehdr.e_phentsize == sizeof(Elf64_Phdr)) {
        uint64_t phOff = elf.ehdr.e_phoff;
        uint64_t phSize = static_cast<uint64_t>(elf.ehdr.e_phnum) * elf.ehdr.e_phentsize;
        if (phOff <= binary.size() && phSize <= binary.size() - phOff) {
            ok = writeAt(phOff, binary.data() + phOff, phSize);
        }
    }
    for (uint64_t i = 0; ok && i < elf.shdrs.size(); ++i) {
        const Elf64_Shdr &sh = elf.shdrs[i];
        std::string name = sectionName(sh);
        if (isSymbolTable(name)) {
            continue; // 段头已改写为 NOBITS，不落数据
        }
        const bool needWrite = materializeAll || isDebug(name) || i == elf.ehdr.e_shstrndx;
        if (!needWrite || sh.sh_type == SHT_NOBITS || sh.sh_size == 0) {
            continue;
        }
        if (sh.sh_offset <= binary.size() && sh.sh_size <= binary.size() - sh.sh_offset) {
            ok = writeAt(sh.sh_offset, binary.data() + sh.sh_offset, sh.sh_size);
        }
    }
    if (ok) {
        // 最后写回改写过的节头表（含 symtab/strtab 的 NOBITS 改动）
        ok = writeAt(elf.shTableOff, elf.shdrs.data(), elf.shTableSize);
    }
    if (close(fd) != 0) {
        ok = false;
    }
    return ok;
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

std::string CallStack::Load(std::string const &kernelName, BinaryBuffer const &binary)
{
    // 单飞：memcheck 与 registercheck 两个线程可能同时为同一内核构建调用栈，
    // 串行化「查缓存 + 落盘 + 入缓存」，避免同一内核被并行写两份 GB 级 kernel.o。
    std::lock_guard<std::mutex> loadGuard{loadMtx_};
    Path kernelPath;
    {
        std::lock_guard<std::mutex> guard{mtx_};
        auto cached = kernelPathMap_.find(kernelName);
        if (cached != kernelPathMap_.cend()) {
            // 同一内核二进制只需落盘一次，后续直接复用已写好的路径
            return cached->second;
        }
        kernelPath = Path(OUTPUT_DIR) / Path(CreateKernelFileName(kernelPaths_.size()));
    }
    std::string const &kernelStr = kernelPath.ToString();
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
                   "permission to the working directory.\n",
                kernelDirStr.c_str());
            return "";
        }
    }
    auto logger = [](std::string const &msg) { std::cout << "[mssanitizer] "<< msg << std::endl; };
    if (!IsFilePermSafe(kernelDirStr, DIR_FILE_MODE, logger)) {
        return "";
    }
    if (IsSoftLink(kernelDirStr)) {
        printf("[mssanitizer] WARNING: The kernel binary (%s) is soft link.\n", kernelDirStr.c_str());
    }
    UmaskGuard umaskGuard(REGULAR_MODE_MASK);
    // 构建给 llvm-symbolizer 的稀疏内核镜像（有 .debug* 时仅落盘 .debug*；无 .debug* 时
    // 落全量镜像但去掉符号表），大幅降低 GB 级内核的落盘与页缓存开销；构建失败则符号化不可用。
    if (!WriteSlimKernelForSymbolizer(kernelStr, binary)) {
        printf("[mssanitizer] ERROR: Callstack may be unavailable: Failed to store kernel binary. Please make sure "
               "the user has write permission to the directory (%s).\n",
            kernelDirStr.c_str());
        return "";
    }

    std::lock_guard<std::mutex> guard{mtx_};
    kernelPaths_.emplace_back(kernelStr);
    kernelPathMap_[kernelName] = kernelStr;
    return kernelStr;
}

CallStack::Stack CallStack::Query(std::string const &kernelName, uint64_t pcOffset)
{
    {
        std::lock_guard<std::mutex> guard{this->mtx_};
        auto kernelIt = Sanitizer::as_const(pcStackMap_).find(kernelName);
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
        if (Sanitizer::as_const(stacks).find(*it) != stacks.cend()) {
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

    KernelBinaryManager::BinaryPtr binary;
    if (!KernelBinaryManager::Instance().Get(kernelName, binary) || binary == nullptr) {
        SAN_ERROR_LOG("Get kernel binary from KernelBinaryManager failed. kernelName: %s", kernelName.c_str());
        return;
    }
    std::string kernelPath = this->Load(kernelName, *binary);
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

    // 快路径：全量打印或未设置 ASCEND_HOME_PATH 时无需做任何路径判定
    if (isPrintFullStack_ || !envValid) {
        std::size_t idx = 0;
        for (auto const &frame : stack) {
            os << "======    #" << idx++ << " " << frame.fileName
               << ":" << frame.line << ":" << frame.column << std::endl;
        }
        return os;
    }

    // 记录量大时（g3 达 8 万条警告）会反复格式化同一批源文件。
    // 原实现每帧都构造 Path(frame.fileName) 与 Path(ascendHomePath)，且 Path::IsSubPathOf 内部
    // 对两个路径各做一次 Resolved()（拷贝 route + Absolute()/getcwd），开销巨大。
    // 这里把 ASCEND_HOME_PATH 构造一次，并记忆化「该源文件是否位于 ASCEND_HOME 之下」。
    static thread_local std::unordered_map<std::string, bool> underAscendCache;
    Path ascendHome(ascendHomePath);

    std::size_t idx = 0;
    for (auto const &frame : stack) {
        auto it = underAscendCache.find(frame.fileName);
        bool underAscend;
        if (it != underAscendCache.end()) {
            underAscend = it->second;
        } else {
            underAscend = Path(frame.fileName).IsSubPathOf(ascendHome);
            underAscendCache.emplace(frame.fileName, underAscend);
        }
        if (!underAscend) {
            os << "======    #" << idx++ << " " << frame.fileName
               << ":" << frame.line << ":" << frame.column << std::endl;
        }
    }
    return os;
}

} // namespace Sanitizer
