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


#ifndef __CORE_FRAMEWORK_UTILITY_USTRING_H__
#define __CORE_FRAMEWORK_UTILITY_USTRING_H__

#include <stdexcept>
#include <string>
#include <stdexcept>

namespace Utility {

template<typename Iterator>
inline std::string Join(Iterator beg, Iterator end, std::string const &sep = " ")
{
    std::string ret;
    if (beg == end) {
        return ret;
    }
    ret = *beg++;
    for (; beg != end; ++beg) {
        ret.append(sep);
        ret.append(*beg);
    }
    return ret;
}

template<typename Iterator, typename ElemFormatter>
inline std::string HumanReadableListFormat(Iterator beg, Iterator end, ElemFormatter const &formatter)
{
    std::string ret;
    if (beg == end) {
        return ret;
    }

    ret = formatter(*beg);
    if (beg + 1 == end) {
        return ret;
    }

    ++beg;
    for (; beg + 1 != end; ++beg) {
        ret.append(", ");
        ret.append(formatter(*beg));
    }
    ret.append(" and ");
    ret.append(formatter(*beg));
    return ret;
}

/* 字符串分割
 * 使用指定的分隔符对字符串进行分割
 * @param str 要分割的字符串
 * @param it 分割后的字符串保存的容器的迭代器，由调用者保证容器的大小满足要求
 * @param seps 指定的分隔符列表
 * @param strict 是否启用严格模式。严格模式下连续出现的分隔符会被依次处理：
 *        - split "abc::def" with ":" -> ["abc", "", "def"]
 *        非严格模式下连续出现的分隔符会作为一个整体
 *        - split "abc::def" with ":" -> ["abc", "def"]
 */
template<typename Iterator>
inline void Split(std::string const &str, Iterator it,
                  std::string const &seps = " ", bool strict = false)
{
    std::string::size_type slow = 0UL;
    std::string::size_type fast = str.find_first_of(seps);
    *it = str.substr(slow, fast - slow);
    for (; fast < str.length(); ++it) {
        if (strict) {
            slow = fast == std::string::npos || fast + 1 >= str.length() ?
                std::string::npos : fast + 1UL;
        } else {
            slow = str.find_first_not_of(seps, fast);
        }
        fast = str.find_first_of(seps, slow);
        if (slow == std::string::npos) {
            *it = std::string();
        } else if (fast == std::string::npos) {
            *it = str.substr(slow);
        } else {
            *it = str.substr(slow, fast - slow);
        }
    }
}

inline std::string Strip(std::string str, std::string const &cs = " ")
{
    std::string::size_type l = str.find_first_not_of(cs);
    std::string::size_type r = str.find_last_not_of(cs);
    return l == std::string::npos || r == std::string::npos ? "" : str.substr(l, r - l + 1);
}

inline std::string RStrip(std::string str, std::string const &cs = " ")
{
    std::string::size_type r = str.find_last_not_of(cs);
    return r == std::string::npos ? "" : str.substr(0UL, r + 1);
}

inline bool EndWith(std::string const &str, std::string const &target)
{
    return
        str.length() >= target.length() &&
        str.substr(str.length() - target.length()) == target;
}

std::string ReplaceInvalidChar(const std::string &input);

// 为日志打印格式化共享内存名称: 直接输出 32 位 FNV-1a 哈希, 形如 hash=5020ab2f,
// 既避免二进制 name 造成的乱码, 也避免长串 \\xHH 转义污染日志; 同一 name 哈希值确定, 便于跨日志行关联
std::string FormatNameForLog(const std::string &input);

inline bool StoiConverter(const std::string &numString, int &num, int radix = 10)
{
    try {
        num = stoi(numString, nullptr, radix);
    } catch (std::invalid_argument&) {
        return false;
    } catch (std::out_of_range&) {
        return false;
    }
    return true;
}

/**
 * @brief Get demangled name by calling builtin function `__cxa_demangle'.
 * @param name - mangled function name
 * @param demangled - output demangled function name if demangling succeeds
 * @return demangling success status
 */
bool Demangle(std::string const &name, std::string &demangled);

/**
 * @brief Get the first part of the full demangled function name without parameter list.
 * @param name - full demangled function name
 * @param simplified - output simplified function name if simplification succeeds
 * @return simplification success status
 */
bool SimplifyDemangledName(std::string const &name, std::string &simplified);

}  // namespace Utility

#endif  // __CORE_FRAMEWORK_UTILITY_USTRING_H__
