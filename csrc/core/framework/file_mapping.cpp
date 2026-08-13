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


#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <sstream>
#include <thread>

#include "utility/serializer.h"
#include "file_mapping.h"

namespace {

using namespace Sanitizer;

using FileMapType = FileMapping::FileMapType;

struct Stream {
    std::vector<char> buffer;
    std::size_t offset;
};

/* 解析直到某个字节通过谓语判断不满足，直到末尾都没有找
 * 到不满足谓语的字符时返回 false
 * @param s 要解析的字节流
 * @param pred 对当前字节进行判断的谓语
 * @param str 解析得到的字符串
 */
template <typename Pred>
bool ParseWhile(Stream &s, Pred pred, std::string &str)
{
    std::size_t beg = s.offset;
    for (; s.offset < s.buffer.size() && pred(s.buffer[s.offset]); ++s.offset) { }
    if (s.offset == s.buffer.size()) {
        return false;
    }
    str.assign(s.buffer.data() + beg, s.offset - beg);
    return true;
}

/* 解析一个任意字符，stream 为空时返回 false
 * @param s 要解析的字节流
 * @param ch 解析得到的字符
 */
bool ParseAnyChar(Stream &s, char &ch)
{
    if (s.offset >= s.buffer.size()) {
        return false;
    }
    ch = s.buffer[s.offset];
    ++s.offset;
    return true;
}

/* 解析一个字符串，并且消耗结尾的 nul 字符，直到末尾都
 * 匹配不到 nul 字符时返回 false
 * @param s 要解析的字节流
 * @param str 解析得到的字符串
 */
bool ParseString(Stream &s, std::string &str)
{
    if (!ParseWhile(s, [](char c) { return c != '\0'; }, str)) {
        return false;
    }
    char c;
    return ParseAnyChar(s, c);
}

/* 解析一个指定类型的变量，流剩余长度不足时返回 false
 * @param s 要解析的字节流
 * @param t 解析得到的变量
 */
template <typename T>
bool ParseType(Stream &s, T &v)
{
    constexpr std::size_t size = sizeof(T);
    if (s.offset + size > s.buffer.size()) {
        return false;
    }
    if (!Deserialize<T>(std::string(s.buffer.data() + s.offset, size), v)) {
        return false;
    }
    s.offset += size;
    return true;
}

bool ParseEachPair(Stream &s, FileMapType &fileMap)
{
    /// 跳过开头的若干个 '\0' 字符
    std::string str;
    if (!ParseWhile(s, [](char c) { return c == '\0'; }, str)) {
        return false;
    }

    /// 解析文件名
    std::string filename;
    if (!ParseString(s, filename)) {
        return false;
    }

    /// 解析 uuid
    uint64_t uuid{};
    if (!ParseType(s, uuid)) {
        return false;
    }

if (fileMap.find(uuid) == fileMap.end()) {
    fileMap[uuid] = {static_cast<int16_t>(fileMap.size()), filename};
}

    return true;
}

}  // namespace Dummy

namespace Sanitizer {

FileInfo FileMapping::Query(uint64_t fileNo) const
{
    typename FileMapType::const_iterator it = fileMap_.find(fileNo);
    return it == fileMap_.cend() ? FileInfo{-1, "<unknown>"} : it->second;
}

void FileMapping::Load(std::vector<char> const& buffer)
{
    Stream stream{buffer, 0UL};
    while (ParseEachPair(stream, fileMap_)) { }
}

} // namespace Sanitizer
