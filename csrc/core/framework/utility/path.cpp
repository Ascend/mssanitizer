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


#include "path.h"

#include <unistd.h>
#include <sys/stat.h>
#include <linux/limits.h>

#include "ustring.h"

namespace Sanitizer {

using namespace Utility;

constexpr char const *PATH_SEP = "/";
constexpr char const *CURRENT_SEGMENT = ".";
constexpr char const *PARENT_SEGMENT = "..";

Path::Path(void) noexcept : absolute_{false} { }

/* Path 构造函数
 * 构造函数中的核心逻辑为将一个路径字符串解析为一系列由 PATH_SEP 分隔的构件（components）组成，
 * 由 std::vector<std::string> 表示。路径可分为相对路径和绝对路径。相对路径在 Path 中表示为第一
 * 个构件不以分隔符开头，如 `["test", "test.cpp"]`；绝对路径的第一个构件以分隔符开头，如
 * `["/test", "test.cpp"]`。
 */
Path::Path(std::string path) noexcept : absolute_{false}
{
    path = Strip(path);
    /// 空路径表示当前路径 "."，并以空列表表示
    if (path.empty()) {
        return;
    }
    if (path[0] == PATH_SEP[0]) {
        absolute_ = true;
    }

    size_t j = 0;
    for (size_t i = 0; i < path.size(); i = j) {
        j = i + 1;
        if (path[i] == PATH_SEP[0]) {
            continue;
        }
        while (j < path.size() && path[j] != PATH_SEP[0]) {
            j++;
        }
        std::string route = path.substr(i, j - i);
        if (route != CURRENT_SEGMENT) {
            route_.emplace_back(route);
        }
    }
}

std::string Path::ToString(void) const
{
    std::string raw;
    /// 空列表表示当前路径 "."
    if (absolute_) {
        raw = PATH_SEP;
    } else if (route_.empty()) {
        return CURRENT_SEGMENT;
    }

    raw.append(Join(route_.cbegin(), route_.cend(), PATH_SEP));
    return raw;
}

std::string Path::Name(void) const
{
    if (route_.empty()) {
        return "";
    }
    return route_.back();
}

Path Path::Parent(void) const &
{
    return Path(*this).Parent();
}

Path Path::Parent(void) &&
{
    if (!route_.empty()) {
        route_.pop_back();
    }
    return std::move(*this);
}

Path Path::Absolute(void) const &
{
    return Path(*this).Absolute();
}

Path Path::Absolute(void) &&
{
    char buf[PATH_MAX] = {0};
    std::string cwd;
    if (getcwd(buf, sizeof(buf))) {
        cwd = buf;
    }
    return Path(cwd) / std::move(*this);
}

Path Path::Resolved(void) const &
{
    return Path(*this).Resolved();
}

Path Path::Resolved(void) &&
{
    Path path = std::move(*this).Absolute();
    auto fast = path.route_.cbegin();
    auto slow = path.route_.begin();
    for (; fast != path.route_.cend(); ++fast) {
        if (*fast != PARENT_SEGMENT) {
            *slow++ = *fast;
        } else if (slow > path.route_.begin()) {
            --slow;
        }
    }
    path.route_.erase(slow, path.route_.end());
    return path;
}

Path Path::operator/(Path rhs) const &
{
    return Path(*this) / rhs;
}

Path Path::operator/(Path rhs) &&
{
    if (rhs.absolute_) {
        return rhs;
    }

    for (auto &r : rhs.route_) {
        route_.emplace_back(std::move(r));
    }
    return *this;
}

bool Path::GetStat(struct stat &st) const
{
    return stat(this->ToString().c_str(), &st) == 0;
}

bool Path::Exists(void) const
{
    struct stat st{};
    return stat(this->ToString().c_str(), &st) == 0;
}

bool Path::IsSubPathOf(const Path& other) const
{
    Path resolvedThis = this->Resolved();
    Path resolvedOther = other.Resolved();
    if (resolvedThis.absolute_ != resolvedOther.absolute_) {
        return false;
    }

    if (resolvedOther.route_.size() > resolvedThis.route_.size()) {
        return false;
    }

    for (size_t i = 0; i < resolvedOther.route_.size(); ++i) {
        if (resolvedThis.route_.at(i) != resolvedOther.route_.at(i)) {
            return false;
        }
    }

    return true;
}
} // namespace Sanitizer
