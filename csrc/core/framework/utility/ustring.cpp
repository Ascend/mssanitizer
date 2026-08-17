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

#include "ustring.h"

#include <cstdint>
#include <cstdlib>
#include <cxxabi.h>
#include <memory>
#include <unordered_map>

namespace Utility {
std::string ReplaceInvalidChar(const std::string &input)
{
    std::string initStr = input;
    size_t invalidPos;
    static const std::unordered_map<std::string, std::string> INVALID_CHAR = {
        {"\n", "\\n"}, {"\f", "\\f"}, {"\r", "\\r"}, {"\b", "\\b"},
        {"\t", "\\t"}, {"\v", "\\v"}, {"\u007F", "\\u007F"}
    };
    for (auto &item: INVALID_CHAR) {
        invalidPos = initStr.find(item.first);
        while (invalidPos != std::string::npos) {
            initStr.replace(invalidPos, item.first.length(), item.second);
            invalidPos = initStr.find(item.first, invalidPos + item.second.length());
        }
    }
    return initStr;
}

// 为日志打印格式化共享内存名称: 直接输出 32 位 FNV-1a 哈希, 形如 hash=5020ab2f,
// 既避免二进制 name 造成的乱码, 也避免长串 \\xHH 转义污染日志; 同一 name 哈希值确定, 便于跨日志行关联
std::string FormatNameForLog(const std::string &input) {
    if (input.empty()) {
        return {};
    }
    static const char HEX_DIGITS[] = "0123456789abcdef";
    uint32_t hash = 0x811c9dc5U; // FNV-1a 32-bit basis
    for (unsigned char c : input) {
        hash ^= c;
        hash *= 0x01000193U; // FNV-1a 32-bit prime
    }
    std::string out = "hash=";
    char hex[9] = {0}; // 8 hex digits + NUL
    for (int i = 7; i >= 0; --i) {
        hex[i] = HEX_DIGITS[hash & 0xFU];
        hash >>= 4U;
    }
    out += hex;
    return out;
}

bool Demangle(std::string const &name, std::string &demangled)
{
    int status{-1};
    std::unique_ptr<char, decltype(&std::free)> res {
        abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status), std::free
    };
    if (status == 0) {
        demangled = res.get();
    }
    return status == 0;
}

std::string::size_type RfindEnclosureChar(std::string const& str, char left, char right, std::string::size_type pos)
{
    int depth = 0;
    for (; pos >= 0UL; --pos) {
        if (str[pos] == right) {
            ++depth;
        } else if (str[pos] == left) {
            --depth;
        }
        if (depth <= 0) {
            break;
        }
        if (pos == 0UL) {
            break;
        }
    }

    return depth > 0 ? std::string::npos : pos;
}

bool SimplifyDemangledName(std::string const &name, std::string &simplified)
{
    // kernel name must end with right parenthesis
    if (name.empty() || name[name.length() - 1] != ')') {
        return false;
    }

    // try to find the paired left parenthesis
    std::string::size_type end = RfindEnclosureChar(name, '(', ')', name.length() - 1);
    if (end == 0UL || end == std::string::npos) {
        return false;
    }

    // drop `<...>' if has template specialization
    if (name[end - 1] == '>') {
        end = RfindEnclosureChar(name, '<', '>', end - 1);
        if (end == 0UL || end == std::string::npos) {
            return false;
        }
    }

    // find the start position of funtion name
    std::string::size_type start = name.find_last_of(" ", end);
    if (start == 0UL || start == std::string::npos) {
        return false;
    }

    ++start;
    if (start >= end) {
        return false;
    }
    simplified = name.substr(start, end - start);
    return true;
}

}
