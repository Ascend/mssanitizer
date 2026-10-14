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

#include <gtest/gtest.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <vector>

#include "pathway_filter.h"
#include "record_parse.h"

namespace allocation_probe {

thread_local bool enabled = false;
thread_local std::size_t allocationCount = 0;

void *Allocate(std::size_t size) {
    void *memory = std::malloc(size == 0 ? 1 : size);
    if (memory == nullptr) {
        throw std::bad_alloc();
    }
    if (enabled) {
        ++allocationCount;
    }
    return memory;
}

void Begin() {
    allocationCount = 0;
    enabled = true;
}

std::size_t End() {
    enabled = false;
    return allocationCount;
}

} // namespace allocation_probe

void *operator new(std::size_t size) { return allocation_probe::Allocate(size); }

void *operator new[](std::size_t size) { return allocation_probe::Allocate(size); }

void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
    try {
        return allocation_probe::Allocate(size);
    } catch (...) {
        return nullptr;
    }
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
    try {
        return allocation_probe::Allocate(size);
    } catch (...) {
        return nullptr;
    }
}

void operator delete(void *memory) noexcept { std::free(memory); }

void operator delete[](void *memory) noexcept { std::free(memory); }

void operator delete(void *memory, std::size_t) noexcept { std::free(memory); }

void operator delete[](void *memory, std::size_t) noexcept { std::free(memory); }

namespace {
using namespace Sanitizer;

SanEvent MakeMemoryEvent(BlockType blockType = BlockType::AICUBE, PipeType pipeType = PipeType::PIPE_S_CAL,
    MemType memType = MemType::UB, AccessType accessType = AccessType::WRITE, uint64_t serialNo = 0) {
    SanEvent event{};
    event.serialNo = serialNo;
    event.type = EventType::MEM_EVENT;
    event.pipe = pipeType;
    event.loc.blockType = blockType;
    event.eventInfo.memInfo.memType = memType;
    event.eventInfo.memInfo.opType = accessType;
    event.eventInfo.memInfo.repeatTimes = 1;
    event.eventInfo.memInfo.blockNum = 1;
    event.eventInfo.memInfo.blockSize = 1;
    return event;
}

SanEvent MakeSyncEvent() {
    SanEvent event{};
    event.type = EventType::SYNC_EVENT;
    event.pipe = PipeType::PIPE_S;
    event.eventInfo.syncInfo.opType = SyncType::PIPE_BARRIER;
    return event;
}

SanitizerRecord MakeKernelRecord(RecordType recordType = RecordType::STORE) {
    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    record.payload.kernelRecord.recordType = recordType;
    record.payload.kernelRecord.blockType = BlockType::AICUBE;
    return record;
}

constexpr PipeType kSupportedScalarPipes[] = {PipeType::PIPE_S_CAL, PipeType::PIPE_S};
constexpr RecordType kSupportedScalarRecordTypes[] = {RecordType::STORE, RecordType::ST, RecordType::ST_IO,
    RecordType::STP, RecordType::STI, RecordType::STI_IO, RecordType::ST_ATOMIC, RecordType::STI_ATOMIC};

PathwayFilterRule MakeAicubeUbWriteRule(EnumMatcher<PipeType> pipes, EnumMatcher<RecordType> records) {
    return {EnumMatcher<BlockType>(BlockType::AICUBE), EventType::MEM_EVENT, pipes, EnumMatcher<MemType>(MemType::UB),
        EnumMatcher<AccessType>(AccessType::WRITE), records};
}

TEST(PathwayFilter, default_rules_are_exact_and_cover_supported_store_records) {
    const auto &rules = PathwayFilter::GetRules();
    ASSERT_EQ(rules.size(), 1U);
    EXPECT_EQ(rules.front().eventType, EventType::MEM_EVENT);

    for (const auto pipeType : kSupportedScalarPipes) {
        for (const auto recordType : kSupportedScalarRecordTypes) {
            EXPECT_TRUE(PathwayFilter::Matches(rules.front(), recordType,
                MakeMemoryEvent(BlockType::AICUBE, pipeType, MemType::UB, AccessType::WRITE)));
        }
    }
}

TEST(PathwayFilter, default_rule_keeps_all_six_dimensions_strict) {
    const auto &rule = PathwayFilter::GetRules().front();
    for (const auto &event : {MakeMemoryEvent(BlockType::AIVEC, PipeType::PIPE_S_CAL, MemType::UB, AccessType::WRITE),
             MakeMemoryEvent(BlockType::AICUBE, PipeType::PIPE_V, MemType::UB, AccessType::WRITE),
             MakeMemoryEvent(BlockType::AICUBE, PipeType::PIPE_S_CAL, MemType::GM, AccessType::WRITE),
             MakeMemoryEvent(BlockType::AICUBE, PipeType::PIPE_S_CAL, MemType::UB, AccessType::READ)}) {
        EXPECT_FALSE(PathwayFilter::Matches(rule, RecordType::STORE, event));
    }
    EXPECT_FALSE(PathwayFilter::Matches(rule, RecordType::LOAD,
        MakeMemoryEvent(BlockType::AICUBE, PipeType::PIPE_S_CAL, MemType::UB, AccessType::WRITE)));
    EXPECT_FALSE(PathwayFilter::Matches(rule, RecordType::STORE, MakeSyncEvent()));
}

TEST(PathwayFilter, one_of_and_any_matchers_keep_other_fields_strict) {
    const auto setRule = MakeAicubeUbWriteRule(EnumMatcher<PipeType>::OneOf({PipeType::PIPE_S_CAL, PipeType::PIPE_S}),
        EnumMatcher<RecordType>::OneOf({RecordType::STORE, RecordType::ST}));
    EXPECT_TRUE(
        PathwayFilter::Matches(setRule, RecordType::STORE, MakeMemoryEvent(BlockType::AICUBE, PipeType::PIPE_S_CAL)));
    EXPECT_TRUE(PathwayFilter::Matches(setRule, RecordType::ST, MakeMemoryEvent(BlockType::AICUBE, PipeType::PIPE_S)));
    EXPECT_FALSE(
        PathwayFilter::Matches(setRule, RecordType::ST_IO, MakeMemoryEvent(BlockType::AICUBE, PipeType::PIPE_S)));

    const auto anyRule = MakeAicubeUbWriteRule(EnumMatcher<PipeType>::Any(), EnumMatcher<RecordType>::Any());
    for (uint32_t pipeValue = 0; pipeValue < static_cast<uint32_t>(PipeType::SIZE); ++pipeValue) {
        EXPECT_TRUE(PathwayFilter::Matches(
            anyRule, RecordType::LOAD, MakeMemoryEvent(BlockType::AICUBE, static_cast<PipeType>(pipeValue))));
    }
    EXPECT_FALSE(
        PathwayFilter::Matches(anyRule, RecordType::LOAD, MakeMemoryEvent(BlockType::AIVEC, PipeType::PIPE_S_CAL)));
}

TEST(PathwayFilter, filters_all_supported_scalar_record_types_and_both_pipes) {
    for (const auto recordType : kSupportedScalarRecordTypes) {
        for (const auto pipeType : {PipeType::PIPE_S_CAL, PipeType::PIPE_S}) {
            std::vector<SanEvent> events{MakeMemoryEvent(BlockType::AICUBE, pipeType)};
            EXPECT_EQ(PathwayFilter::FilterInPlace(recordType, events), 1U);
            EXPECT_TRUE(events.empty());
        }
    }
}

TEST(PathwayFilter, parser_uses_pipe_s_cal_for_scalar_ub_store) {
    RecordParse::ResetAll();
    SanitizerRecord record = MakeKernelRecord();
    record.payload.kernelRecord.payload.loadStoreRecord.addr = 0;
    record.payload.kernelRecord.payload.loadStoreRecord.space = AddressSpace::UB;
    record.payload.kernelRecord.payload.loadStoreRecord.size = 4;
    record.payload.kernelRecord.payload.loadStoreRecord.alignSize = 1;

    std::vector<SanEvent> events;
    RecordParse::Parse(record, events);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].loc.blockType, BlockType::AICUBE);
    EXPECT_EQ(events[0].pipe, PipeType::PIPE_S_CAL);
    EXPECT_EQ(PathwayFilter::FilterInPlace(RecordType::STORE, events), 1U);
    EXPECT_TRUE(events.empty());
    RecordParse::ResetAll();
}

TEST(PathwayFilter, early_release_keeps_events_for_non_matching_record_and_empty_rules) {
    std::vector<SanEvent> events{
        MakeMemoryEvent(BlockType::AIVEC, PipeType::PIPE_V, MemType::UB, AccessType::WRITE, 7)};
    const auto capacity = events.capacity();
    EXPECT_EQ(PathwayFilter::FilterInPlace(RecordType::LOAD, events), 0U);
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].serialNo, 7U);
    EXPECT_EQ(events.capacity(), capacity);

    for (const auto pipeType : kSupportedScalarPipes) {
        std::vector<SanEvent> deviceEvents{
            MakeMemoryEvent(BlockType::AICUBE, pipeType, MemType::UB, AccessType::WRITE, 8)};
        EXPECT_EQ(PathwayFilter::FilterInPlace(RecordType::ST_DEV, deviceEvents), 0U);
        ASSERT_EQ(deviceEvents.size(), 1U);
        EXPECT_EQ(deviceEvents[0].serialNo, 8U);
        EXPECT_EQ(deviceEvents[0].pipe, pipeType);
    }

    PathwayFilter::RuleTable emptyRules;
    EXPECT_EQ(PathwayFilter::FilterInPlace(RecordType::STORE, events, emptyRules), 0U);
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].serialNo, 7U);
}

TEST(PathwayFilter, filters_in_place_in_order_for_empty_full_alternating_and_sparse_inputs) {
    std::vector<SanEvent> empty;
    EXPECT_EQ(PathwayFilter::FilterInPlace(RecordType::STORE, empty), 0U);

    std::vector<SanEvent> allMatch{
        MakeMemoryEvent(BlockType::AICUBE, PipeType::PIPE_S_CAL, MemType::UB, AccessType::WRITE, 1),
        MakeMemoryEvent(BlockType::AICUBE, PipeType::PIPE_S, MemType::UB, AccessType::WRITE, 2)};
    const auto allMatchCapacity = allMatch.capacity();
    EXPECT_EQ(PathwayFilter::FilterInPlace(RecordType::STORE, allMatch), 2U);
    EXPECT_TRUE(allMatch.empty());
    EXPECT_EQ(allMatch.capacity(), allMatchCapacity);

    std::vector<SanEvent> events{
        MakeMemoryEvent(BlockType::AICUBE, PipeType::PIPE_S_CAL, MemType::UB, AccessType::WRITE, 1),
        MakeMemoryEvent(BlockType::AIVEC, PipeType::PIPE_V, MemType::UB, AccessType::WRITE, 2),
        MakeMemoryEvent(BlockType::AICUBE, PipeType::PIPE_S, MemType::UB, AccessType::WRITE, 3),
        MakeMemoryEvent(BlockType::AIVEC, PipeType::PIPE_V, MemType::UB, AccessType::WRITE, 4),
        MakeMemoryEvent(BlockType::AICUBE, PipeType::PIPE_S_CAL, MemType::UB, AccessType::WRITE, 5),
    };
    EXPECT_EQ(PathwayFilter::FilterInPlace(RecordType::STORE, events), 3U);
    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events[0].serialNo, 2U);
    EXPECT_EQ(events[1].serialNo, 4U);
}

TEST(PathwayFilter, preserves_retained_event_fields_and_time_info) {
    auto removed = MakeMemoryEvent(BlockType::AICUBE, PipeType::PIPE_S_CAL, MemType::UB, AccessType::WRITE, 1);
    removed.timeInfo = {10, 11, 12};
    auto retained = MakeMemoryEvent(BlockType::AIVEC, PipeType::PIPE_V, MemType::GM, AccessType::READ, 2);
    retained.timeInfo = {20, 21, 22};
    retained.loc.pc = 0x1234;
    retained.eventInfo.memInfo.addr = 0x5678;
    std::vector<SanEvent> events{removed, retained};

    EXPECT_EQ(PathwayFilter::FilterInPlace(RecordType::STORE, events), 1U);
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].serialNo, retained.serialNo);
    EXPECT_EQ(events[0].type, retained.type);
    EXPECT_EQ(events[0].pipe, retained.pipe);
    EXPECT_EQ(events[0].loc.pc, retained.loc.pc);
    EXPECT_EQ(events[0].eventInfo.memInfo.memType, retained.eventInfo.memInfo.memType);
    EXPECT_EQ(events[0].eventInfo.memInfo.opType, retained.eventInfo.memInfo.opType);
    EXPECT_EQ(events[0].eventInfo.memInfo.addr, retained.eventInfo.memInfo.addr);
    EXPECT_EQ(events[0].timeInfo, retained.timeInfo);
}

TEST(PathwayFilter, does_not_read_non_memory_event_union) {
    std::vector<SanEvent> events{MakeSyncEvent()};
    EXPECT_EQ(PathwayFilter::FilterInPlace(RecordType::STORE, events), 0U);
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].type, EventType::SYNC_EVENT);
    EXPECT_EQ(events[0].eventInfo.syncInfo.opType, SyncType::PIPE_BARRIER);
}

TEST(PathwayFilter, new_rule_can_match_after_record_type_fast_path) {
    SanEvent event = MakeMemoryEvent(BlockType::AIVEC, PipeType::PIPE_V, MemType::GM, AccessType::READ, 7);
    PathwayFilter::RuleTable rules = PathwayFilter::GetRules();
    rules.push_back({EnumMatcher<BlockType>(BlockType::AIVEC), EventType::MEM_EVENT,
        EnumMatcher<PipeType>(PipeType::PIPE_V), EnumMatcher<MemType>(MemType::GM),
        EnumMatcher<AccessType>(AccessType::READ), EnumMatcher<RecordType>(RecordType::LOAD)});

    std::vector<SanEvent> events{event};
    EXPECT_EQ(PathwayFilter::FilterInPlace(RecordType::LOAD, events, rules), 1U);
    EXPECT_TRUE(events.empty());
}

TEST(PathwayFilter, overlapping_rules_remove_each_event_once) {
    PathwayFilter::RuleTable rules = PathwayFilter::GetRules();
    rules.push_back(rules.front());
    std::vector<SanEvent> events{MakeMemoryEvent()};
    EXPECT_EQ(PathwayFilter::FilterInPlace(RecordType::STORE, events, rules), 1U);
    EXPECT_TRUE(events.empty());
}
TEST(PathwayFilter, in_place_filter_has_no_allocations_and_preserves_time_info) {
    // Warm the static rule table and parser-independent input shape before
    // enabling the probe. The measured region contains only FilterInPlace.
    (void)PathwayFilter::GetRules();
    std::vector<SanEvent> warmup{MakeMemoryEvent()};
    (void)PathwayFilter::FilterInPlace(RecordType::STORE, warmup);

    allocation_probe::Begin();
    auto *probeBuffer = static_cast<uint8_t *>(::operator new[](64));
    probeBuffer[0] = 0x5A;
    ::operator delete[](probeBuffer);
    const std::size_t positiveAllocationCount = allocation_probe::End();
    EXPECT_GT(positiveAllocationCount, 0U);

    std::vector<SanEvent> events;
    events.reserve(5);
    const auto addEvent = [&events](SanEvent event, VectorTime timeInfo, uint64_t addr = 0) {
        event.timeInfo = timeInfo;
        event.eventInfo.memInfo.addr = addr;
        events.emplace_back(event);
    };
    addEvent(MakeMemoryEvent(BlockType::AICUBE, PipeType::PIPE_S_CAL, MemType::UB, AccessType::WRITE, 1), {10, 11});
    addEvent(MakeMemoryEvent(BlockType::AIVEC, PipeType::PIPE_V, MemType::GM, AccessType::READ, 2), {20, 21}, 0x20);
    addEvent(MakeMemoryEvent(BlockType::AICUBE, PipeType::PIPE_S, MemType::UB, AccessType::WRITE, 3), {30, 31});
    addEvent(MakeMemoryEvent(BlockType::AIVEC, PipeType::PIPE_V, MemType::GM, AccessType::READ, 4), {40, 41}, 0x40);
    addEvent(MakeMemoryEvent(BlockType::AICUBE, PipeType::PIPE_S_CAL, MemType::UB, AccessType::WRITE, 5), {50, 51});

    allocation_probe::Begin();
    const std::size_t removed = PathwayFilter::FilterInPlace(RecordType::STORE, events);
    const std::size_t filterAllocationCount = allocation_probe::End();

    EXPECT_EQ(filterAllocationCount, 0U);
    EXPECT_EQ(removed, 3U);
    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events[0].serialNo, 2U);
    EXPECT_EQ(events[0].timeInfo, (VectorTime{20, 21}));
    EXPECT_EQ(events[0].eventInfo.memInfo.addr, 0x20U);
    EXPECT_EQ(events[1].serialNo, 4U);
    EXPECT_EQ(events[1].timeInfo, (VectorTime{40, 41}));
    EXPECT_EQ(events[1].eventInfo.memInfo.addr, 0x40U);
}
} // namespace
