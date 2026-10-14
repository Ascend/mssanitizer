/* -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#ifndef CORE_FRAMEWORK_PATHWAY_FILTER_H
#define CORE_FRAMEWORK_PATHWAY_FILTER_H

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <vector>

#include "event_def.h"

namespace Sanitizer {

template <typename Enum> class EnumMatcher {
public:
    static_assert(std::is_enum<Enum>::value, "EnumMatcher requires an enum type");

    explicit EnumMatcher(Enum value) : any_(false), values_(1, value) {}

    static EnumMatcher OneOf(std::initializer_list<Enum> values) { return EnumMatcher(values); }

    static EnumMatcher Any() { return EnumMatcher(true); }

    bool Matches(Enum value) const {
        return any_ || std::find(values_.cbegin(), values_.cend(), value) != values_.cend();
    }

private:
    EnumMatcher(std::initializer_list<Enum> values) : any_(false), values_(values) {}

    explicit EnumMatcher(bool any) : any_(any) {}

    bool any_;
    std::vector<Enum> values_;
};

struct PathwayFilterRule {
    EnumMatcher<BlockType> blockType;
    EventType eventType;
    EnumMatcher<PipeType> pipeType;
    EnumMatcher<MemType> memType;
    EnumMatcher<AccessType> accessType;
    EnumMatcher<RecordType> recordType;
};

class PathwayFilter {
public:
    using RuleTable = std::vector<PathwayFilterRule>;

    static const RuleTable &GetRules();

    static bool Matches(const PathwayFilterRule &rule, RecordType recordType, const SanEvent &event);

    // 在调用方独占的事件副本上原地移除匹配事件并返回移除数量。
    static size_t FilterInPlace(RecordType recordType, std::vector<SanEvent> &events);
    static size_t FilterInPlace(RecordType recordType, std::vector<SanEvent> &events, const RuleTable &rules);
};

} // namespace Sanitizer

#endif
