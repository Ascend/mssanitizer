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

#include "pathway_filter.h"

namespace Sanitizer {

const PathwayFilter::RuleTable &PathwayFilter::GetRules() {
    // ParseScalarOpRecord emits PIPE_S_CAL; PIPE_S is retained for the exact representation in the Issue.
    // Use explicit pipe and record allowlists instead of Any to avoid widening the default filter boundary.
    static const RuleTable rules = {
        {EnumMatcher<BlockType>(BlockType::AICUBE), EventType::MEM_EVENT,
            EnumMatcher<PipeType>::OneOf({PipeType::PIPE_S_CAL, PipeType::PIPE_S}), EnumMatcher<MemType>(MemType::UB),
            EnumMatcher<AccessType>(AccessType::WRITE),
            EnumMatcher<RecordType>::OneOf({RecordType::STORE, RecordType::ST, RecordType::ST_IO, RecordType::STP,
                RecordType::STI, RecordType::STI_IO, RecordType::ST_ATOMIC, RecordType::STI_ATOMIC})},
    };
    return rules;
}

bool PathwayFilter::Matches(const PathwayFilterRule &rule, RecordType recordType, const SanEvent &event) {
    if (event.type != rule.eventType || event.type != EventType::MEM_EVENT) {
        return false;
    }

    // Do not inspect the memory union until the event has been identified as a MEM_EVENT.
    const MemOpInfo &memInfo = event.eventInfo.memInfo;
    return rule.blockType.Matches(event.loc.blockType) && rule.pipeType.Matches(event.pipe) &&
        rule.memType.Matches(memInfo.memType) && rule.accessType.Matches(memInfo.opType) &&
        rule.recordType.Matches(recordType);
}

size_t PathwayFilter::FilterInPlace(RecordType recordType, std::vector<SanEvent> &events) {
    return FilterInPlace(recordType, events, GetRules());
}

size_t PathwayFilter::FilterInPlace(RecordType recordType, std::vector<SanEvent> &events, const RuleTable &rules) {
    // A record type that no rule can match is a common fast path. Keep this check
    // derived from the rule table so adding a rule cannot make it stale.
    if (std::none_of(rules.cbegin(), rules.cend(),
            [recordType](const PathwayFilterRule &rule) { return rule.recordType.Matches(recordType); })) {
        return 0;
    }

    const size_t oldSize = events.size();
    const auto newEnd = std::remove_if(events.begin(), events.end(), [recordType, &rules](const SanEvent &event) {
        return std::any_of(rules.cbegin(), rules.cend(), [recordType, &event](const PathwayFilterRule &rule) {
            return PathwayFilter::Matches(rule, recordType, event);
        });
    });
    events.erase(newEnd, events.end());
    return oldSize - events.size();
}

} // namespace Sanitizer
