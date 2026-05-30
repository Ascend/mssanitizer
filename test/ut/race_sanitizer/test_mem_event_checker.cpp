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


#include <gtest/gtest.h>
#include <vector>

#include "alg_framework/mem_event_checker.h"

namespace {
using namespace Sanitizer;

void RunAllRaceAlgExpectNoRace(const std::vector<MemEvent> &events)
{
    MemEventChecker checker;
    checker.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, RaceCheckType::SINGLE_BLOCK_CHECK);
    for (auto &event : events) {
        checker.PushEvent(event);
    }
    MemEventChecker::RaceMemEventsIdx raceMemEventsIdx;
    MemEventChecker::RaceMemEventsIdx missDcciEventsIdx;
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    checker.Init(KernelType::AICUBE, DeviceType::ASCEND_910B1, RaceCheckType::SINGLE_BLOCK_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    checker.Init(KernelType::MIX, DeviceType::ASCEND_910B1, RaceCheckType::SINGLE_BLOCK_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    checker.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, RaceCheckType::SINGLE_PIPE_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    checker.Init(KernelType::AICUBE, DeviceType::ASCEND_910B1, RaceCheckType::SINGLE_PIPE_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    checker.Init(KernelType::MIX, DeviceType::ASCEND_910B1, RaceCheckType::SINGLE_PIPE_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    checker.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, RaceCheckType::CROSS_BLOCK_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    checker.Init(KernelType::AICUBE, DeviceType::ASCEND_910B1, RaceCheckType::CROSS_BLOCK_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    checker.Init(KernelType::MIX, DeviceType::ASCEND_910B1, RaceCheckType::CROSS_BLOCK_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    ASSERT_EQ(raceMemEventsIdx.size(), 0);
}

void RunAllRaceAlgExpectInnerCoreRace(const std::vector<MemEvent> &events)
{
    MemEventChecker checker;
    checker.Init(KernelType::AIVEC, DeviceType::ASCEND_910B2, RaceCheckType::SINGLE_BLOCK_CHECK);
    for (auto &event : events) {
        checker.PushEvent(event);
    }
    MemEventChecker::RaceMemEventsIdx raceMemEventsIdx;
    MemEventChecker::RaceMemEventsIdx missDcciEventsIdx;
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    ASSERT_EQ(raceMemEventsIdx.size(), 1);
    raceMemEventsIdx.clear();
    checker.Init(KernelType::AICUBE, DeviceType::ASCEND_910B2, RaceCheckType::SINGLE_BLOCK_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    ASSERT_EQ(raceMemEventsIdx.size(), 1);
    raceMemEventsIdx.clear();
    checker.Init(KernelType::MIX, DeviceType::ASCEND_910B2, RaceCheckType::SINGLE_BLOCK_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    ASSERT_EQ(raceMemEventsIdx.size(), 1);
    raceMemEventsIdx.clear();
    checker.Init(KernelType::AIVEC, DeviceType::ASCEND_910B3, RaceCheckType::CROSS_BLOCK_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    ASSERT_EQ(raceMemEventsIdx.size(), 0);
}

void RunAllRaceAlgExpectInnerPipeRace(const std::vector<MemEvent> &events)
{
    MemEventChecker checker;
    checker.Init(KernelType::AIVEC, DeviceType::ASCEND_910B4, RaceCheckType::SINGLE_PIPE_CHECK);
    for (auto &event : events) {
        checker.PushEvent(event);
    }
    MemEventChecker::RaceMemEventsIdx raceMemEventsIdx;
    MemEventChecker::RaceMemEventsIdx missDcciEventsIdx;
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    ASSERT_EQ(raceMemEventsIdx.size(), 1);
    raceMemEventsIdx.clear();
    checker.Init(KernelType::AICUBE, DeviceType::ASCEND_910B4, RaceCheckType::SINGLE_PIPE_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    ASSERT_EQ(raceMemEventsIdx.size(), 1);
    raceMemEventsIdx.clear();
    checker.Init(KernelType::MIX, DeviceType::ASCEND_910B4, RaceCheckType::SINGLE_PIPE_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    ASSERT_EQ(raceMemEventsIdx.size(), 1);
    raceMemEventsIdx.clear();
    checker.Init(KernelType::AIVEC, DeviceType::ASCEND_910B4, RaceCheckType::CROSS_BLOCK_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    ASSERT_EQ(raceMemEventsIdx.size(), 0);
}

void RunAllRaceAlgExpectCrossCoreRace(const std::vector<MemEvent> &events)
{
    MemEventChecker checker;
    checker.Init(KernelType::AIVEC, DeviceType::ASCEND_910B4, RaceCheckType::CROSS_BLOCK_CHECK);
    for (auto &event : events) {
        checker.PushEvent(event);
    }
    MemEventChecker::RaceMemEventsIdx raceMemEventsIdx;
    MemEventChecker::RaceMemEventsIdx missDcciEventsIdx;
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    ASSERT_EQ(raceMemEventsIdx.size(), 1);
    raceMemEventsIdx.clear();
    checker.Init(KernelType::AICUBE, DeviceType::ASCEND_910B4, RaceCheckType::CROSS_BLOCK_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    ASSERT_EQ(raceMemEventsIdx.size(), 1);
    raceMemEventsIdx.clear();
    checker.Init(KernelType::MIX, DeviceType::ASCEND_910B4, RaceCheckType::CROSS_BLOCK_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    ASSERT_EQ(raceMemEventsIdx.size(), 1);
    raceMemEventsIdx.clear();
    checker.Init(KernelType::AIVEC, DeviceType::ASCEND_910B4, RaceCheckType::SINGLE_BLOCK_CHECK);
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    ASSERT_EQ(raceMemEventsIdx.size(), 0);
}

TEST(MemEventChecker, the_same_pipe_expect_no_race)
{
    SanEvent event;
    event.pipe = PipeType::PIPE_MTE2;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    MemEvent e1(event);
    MemEvent e2(event);

    VectorTime t;
    t.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    e1.vt = t;
    e1.serialNo = 3;
    e2.vt = t;
    e2.serialNo = 5;
    e2.barrierNo = 4;

    std::vector<MemEvent> events;
    events.push_back(e1);
    events.push_back(e2);
    RunAllRaceAlgExpectNoRace(events);
}

TEST(MemEventChecker, the_same_instruction_expect_no_race)
{
    SanEvent event;
    event.pipe = PipeType::PIPE_MTE2;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    MemEvent e1(event);
    e1.serialNo = 3;
    MemEvent e2 = e1;

    std::vector<MemEvent> events;
    events.push_back(e1);
    events.push_back(e2);
    RunAllRaceAlgExpectNoRace(events);
}

TEST(MemEventChecker, different_memtype_expect_no_race)
{
    SanEvent event;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    MemEvent e1(event);
    MemEvent e2(event);

    VectorTime t;
    t.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    e1.vt = t;
    e1.memInfo.memType = MemType::UB;
    e1.pipe = PipeType::PIPE_MTE2;

    e2.vt = t;
    e1.memInfo.memType = MemType::GM;
    e2.pipe = PipeType::PIPE_MTE1;

    std::vector<MemEvent> events;
    events.push_back(e1);
    events.push_back(e2);
    RunAllRaceAlgExpectNoRace(events);
}

TEST(MemEventChecker, read_after_read_expect_no_race)
{
    SanEvent event;
    event.eventInfo.memInfo.opType = AccessType::READ;
    MemEvent e1(event);
    MemEvent e2(event);

    VectorTime t1;
    t1.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    t1[static_cast<uint8_t>(PipeType::PIPE_MTE1)]++;
    e1.vt = t1;
    e1.pipe = PipeType::PIPE_MTE1;

    VectorTime t2;
    t2.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    t2[static_cast<uint8_t>(PipeType::PIPE_MTE2)]++;
    e2.vt = t2;
    e2.pipe = PipeType::PIPE_MTE2;

    std::vector<MemEvent> events;
    events.push_back(e1);
    events.push_back(e2);
    RunAllRaceAlgExpectNoRace(events);
}

TEST(MemEventChecker, write_after_write_expect_core_inner_race)
{
    SanEvent event;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0;
    event.eventInfo.memInfo.blockNum = 8;
    event.eventInfo.memInfo.blockSize = 32;
    event.eventInfo.memInfo.blockStride = 1;
    event.eventInfo.memInfo.repeatTimes = 1;
    MemEvent e1(event);
    MemEvent e2(event);

    VectorTime t1;
    t1.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    t1[static_cast<uint8_t>(PipeType::PIPE_MTE1)]++;
    e1.vt = t1;
    e1.pipe = PipeType::PIPE_MTE1;

    VectorTime t2;
    t2.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    t2[static_cast<uint8_t>(PipeType::PIPE_MTE2)]++;
    e2.vt = t2;
    e2.pipe = PipeType::PIPE_MTE2;

    std::vector<MemEvent> events;
    events.push_back(e1);
    events.push_back(e2);
    RunAllRaceAlgExpectInnerCoreRace(events);
}

TEST(MemEventChecker, write_after_read_expect_core_inner_race)
{
    SanEvent event;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0;
    event.eventInfo.memInfo.blockNum = 8;
    event.eventInfo.memInfo.blockSize = 32;
    event.eventInfo.memInfo.blockStride = 1;
    event.eventInfo.memInfo.repeatTimes = 1;
    MemEvent e1(event);
    MemEvent e2(event);

    VectorTime t1;
    t1.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    t1[static_cast<uint8_t>(PipeType::PIPE_MTE1)]++;
    e1.vt = t1;
    e1.pipe = PipeType::PIPE_MTE1;
    e1.memInfo.opType = AccessType::READ;

    VectorTime t2;
    t2.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    t2[static_cast<uint8_t>(PipeType::PIPE_MTE2)]++;
    e2.vt = t2;
    e2.pipe = PipeType::PIPE_MTE2;
    e2.memInfo.opType = AccessType::WRITE;

    std::vector<MemEvent> events;
    events.push_back(e1);
    events.push_back(e2);
    RunAllRaceAlgExpectInnerCoreRace(events);
}

TEST(MemEventChecker, read_after_write_expect_vec_core_inner_race)
{
    SanEvent event;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0;
    event.eventInfo.memInfo.blockNum = 8;
    event.eventInfo.memInfo.blockSize = 32;
    event.eventInfo.memInfo.blockStride = 1;
    event.eventInfo.memInfo.repeatTimes = 1;
    MemEvent e1(event);
    MemEvent e2(event);

    VectorTime t1;
    t1.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    t1[static_cast<uint8_t>(PipeType::PIPE_MTE1)]++;
    e1.vt = t1;
    e1.pipe = PipeType::PIPE_MTE1;
    e1.memInfo.opType = AccessType::WRITE;

    VectorTime t2;
    t2.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    t2[static_cast<uint8_t>(PipeType::PIPE_MTE2)]++;
    e2.vt = t2;
    e2.pipe = PipeType::PIPE_MTE2;
    e2.memInfo.opType = AccessType::READ;

    std::vector<MemEvent> events;
    events.push_back(e1);
    events.push_back(e2);
    RunAllRaceAlgExpectInnerCoreRace(events);
}

TEST(MemEventChecker, write_after_write_expect_inner_pipe_race)
{
    SanEvent event;
    event.pipe = PipeType::PIPE_V;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0;
    event.eventInfo.memInfo.blockNum = 8;
    event.eventInfo.memInfo.blockSize = 32;
    event.eventInfo.memInfo.blockStride = 1;
    event.eventInfo.memInfo.repeatTimes = 1;
    MemEvent e1(event);
    e1.serialNo = 1;

    MemEvent e2(event);
    e2.serialNo = 2;

    std::vector<MemEvent> events;
    events.push_back(e1);
    events.push_back(e2);
    RunAllRaceAlgExpectInnerPipeRace(events);
}

TEST(MemEventChecker, write_after_read_expect_inner_pipe_race)
{
    SanEvent event;
    event.pipe = PipeType::PIPE_V;
    event.eventInfo.memInfo.opType = AccessType::READ;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0;
    event.eventInfo.memInfo.blockNum = 8;
    event.eventInfo.memInfo.blockSize = 32;
    event.eventInfo.memInfo.blockStride = 1;
    event.eventInfo.memInfo.repeatTimes = 1;
    MemEvent e1(event);
    e1.serialNo = 1;

    MemEvent e2(event);
    e2.memInfo.opType = AccessType::WRITE;
    e2.serialNo = 2;

    std::vector<MemEvent> events;
    events.push_back(e1);
    events.push_back(e2);
    RunAllRaceAlgExpectInnerPipeRace(events);
}

TEST(MemEventChecker, read_after_write_expect_inner_pipe_race)
{
    SanEvent event;
    event.pipe = PipeType::PIPE_V;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0;
    event.eventInfo.memInfo.blockNum = 8;
    event.eventInfo.memInfo.blockSize = 32;
    event.eventInfo.memInfo.blockStride = 1;
    event.eventInfo.memInfo.repeatTimes = 1;
    MemEvent e1(event);
    e1.serialNo = 1;

    MemEvent e2(event);
    e2.memInfo.opType = AccessType::READ;
    e2.serialNo = 2;

    std::vector<MemEvent> events;
    events.push_back(e1);
    events.push_back(e2);
    RunAllRaceAlgExpectInnerPipeRace(events);
}

TEST(MemEventChecker, write_after_read_expect_cross_core_race)
{
    SanEvent event;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0;
    event.eventInfo.memInfo.blockNum = 8;
    event.eventInfo.memInfo.blockSize = 32;
    event.eventInfo.memInfo.repeatTimes = 1;
    MemEvent e1(event);
    MemEvent e2(event);

    uint8_t blockDim = 10;
    VectorTime t1;
    e1.loc.coreId = 0;
    t1.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1);
    t1[static_cast<uint8_t>(PipeType::PIPE_MTE1) + e1.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    e1.vt = t1;
    e1.pipe = PipeType::PIPE_MTE1;
    e1.memInfo.opType = AccessType::READ;

    VectorTime t2;
    e2.loc.coreId = 1;
    t2.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1);
    t2[static_cast<uint8_t>(PipeType::PIPE_MTE2) + e2.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    e2.vt = t2;
    e2.pipe = PipeType::PIPE_MTE2;
    e2.memInfo.opType = AccessType::WRITE;

    std::vector<MemEvent> events;
    events.push_back(e1);
    events.push_back(e2);
    RunAllRaceAlgExpectCrossCoreRace(events);
}

TEST(MemEventChecker, write_after_write_expect_cross_core_race)
{
    SanEvent event;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0;
    event.eventInfo.memInfo.blockNum = 8;
    event.eventInfo.memInfo.blockSize = 32;
    event.eventInfo.memInfo.repeatTimes = 1;
    MemEvent e1(event);
    MemEvent e2(event);

    uint8_t blockDim = 10;
    VectorTime t1;
    e1.loc.coreId = 0;
    t1.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1);
    t1[static_cast<uint8_t>(PipeType::PIPE_MTE1) + e1.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    e1.vt = t1;
    e1.pipe = PipeType::PIPE_MTE1;

    VectorTime t2;
    e2.loc.coreId = 1;
    t2.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1);
    t2[static_cast<uint8_t>(PipeType::PIPE_MTE2) + e2.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    e2.vt = t2;
    e2.pipe = PipeType::PIPE_MTE2;

    std::vector<MemEvent> events;
    events.push_back(e1);
    events.push_back(e2);
    RunAllRaceAlgExpectCrossCoreRace(events);
}

TEST(MemEventChecker, read_after_write_expect_cross_core_race)
{
    SanEvent event;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = 0;
    event.eventInfo.memInfo.blockNum = 8;
    event.eventInfo.memInfo.blockSize = 32;
    event.eventInfo.memInfo.repeatTimes = 1;
    MemEvent e1(event);
    MemEvent e2(event);

    uint8_t blockDim = 10;
    VectorTime t1;
    e1.loc.coreId = 0;
    t1.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1);
    t1[static_cast<uint8_t>(PipeType::PIPE_MTE1) + e1.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    e1.vt = t1;
    e1.pipe = PipeType::PIPE_MTE1;
    e1.memInfo.opType = AccessType::WRITE;

    VectorTime t2;
    e2.loc.coreId = 1;
    t2.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1);
    t2[static_cast<uint8_t>(PipeType::PIPE_MTE2) + e2.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    e2.vt = t2;
    e2.pipe = PipeType::PIPE_MTE2;
    e2.memInfo.opType = AccessType::READ;

    std::vector<MemEvent> events;
    events.push_back(e1);
    events.push_back(e2);
    RunAllRaceAlgExpectCrossCoreRace(events);
}

/*
    OP1: |- - - - |
    OP2: | - - - -|
*/
TEST(MemEventChecker, write_after_write_expect_no_race)
{
    constexpr uint64_t addr = 0x001000UL;
    constexpr uint64_t blockSize = 32;

    SanEvent event;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.blockNum = 4;
    event.eventInfo.memInfo.blockSize = blockSize;
    event.eventInfo.memInfo.blockStride = 2;
    event.eventInfo.memInfo.repeatTimes = 1;
    MemEvent e1(event);
    MemEvent e2(event);

    VectorTime t1;
    e1.loc.coreId = 0;
    t1.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    t1[static_cast<uint8_t>(PipeType::PIPE_MTE1) + e1.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    e1.vt = t1;
    e1.pipe = PipeType::PIPE_MTE1;
    e1.memInfo.addr = addr;

    VectorTime t2;
    e2.loc.coreId = 0;
    t2.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    t2[static_cast<uint8_t>(PipeType::PIPE_MTE1) + e2.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    e2.vt = t2;
    e2.pipe = PipeType::PIPE_MTE2;
    e2.memInfo.addr = addr + blockSize;

    MemEventChecker checker;
    checker.Init(KernelType::AICUBE, DeviceType::ASCEND_910B4, RaceCheckType::CROSS_BLOCK_CHECK);
    checker.PushEvent(e1);
    checker.PushEvent(e2);
    MemEventChecker::RaceMemEventsIdx raceMemEvents;
    MemEventChecker::RaceMemEventsIdx missDcciEventsIdx;
    checker.ScanlineAlgorithm(raceMemEvents, missDcciEventsIdx);
    ASSERT_EQ(raceMemEvents.size(), 0);
}

TEST(MemEventChecker, write_after_write_expect_race)
{
    constexpr uint64_t addr = 0x001000UL;
    constexpr uint64_t blockSize = 32;

    SanEvent event;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.blockNum = 4;
    event.eventInfo.memInfo.blockSize = blockSize;
    event.eventInfo.memInfo.blockStride = 3;
    event.eventInfo.memInfo.repeatTimes = 3;
    MemEvent e1(event);
    MemEvent e2(event);

    uint8_t blockDim = 2;
    VectorTime t1;
    VectorTime t2;

    t1.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1);
    e1.loc.coreId = 0;
    e1.pipe = PipeType::PIPE_MTE1;
    t1[static_cast<uint8_t>(e1.pipe) + e1.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    e1.memInfo.addr = addr;
    e1.vt = t1;

    e2.loc.coreId = 1;
    t2.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1);
    e2.pipe = PipeType::PIPE_MTE2;
    t2[static_cast<uint8_t>(e2.pipe) + e2.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    e2.memInfo.addr = addr + blockSize;
    e2.memInfo.blockNum = 2;
    e2.memInfo.blockStride = 2;
    e2.vt = t2;

    MemEventChecker checker;
    checker.Init(KernelType::AICUBE, DeviceType::ASCEND_910B4, RaceCheckType::CROSS_BLOCK_CHECK);
    checker.PushEvent(e1);
    checker.PushEvent(e2);
    MemEventChecker::RaceMemEventsIdx raceMemEvents;
    MemEventChecker::RaceMemEventsIdx missDcciEventsIdx;
    checker.ScanlineAlgorithm(raceMemEvents, missDcciEventsIdx);
    ASSERT_EQ(raceMemEvents.size(), 1);
}

TEST(MemEventChecker, write_after_write_expect_no_race_moe_mov_align_case)
{
    constexpr uint64_t addr = 0x12c1c11e8600UL;
    constexpr uint64_t blockSize = 1;

    SanEvent event;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.blockNum = 4;
    event.eventInfo.memInfo.blockSize = blockSize;
    event.eventInfo.memInfo.blockStride = 1;
    event.eventInfo.memInfo.repeatStride = 156 + 4;
    event.eventInfo.memInfo.repeatTimes = 251;
    MemEvent e1(event);
    MemEvent e2(event);

    VectorTime t;
    t.resize(static_cast<uint8_t>(PipeType::SIZE), 1);

    e1.vt = t;
    e1.pipe = PipeType::PIPE_MTE3;
    e1.memInfo.addr = addr;

    e2.vt = t;
    e2.pipe = PipeType::PIPE_MTE2;
    e2.memInfo.addr = addr + blockSize * 4;

    MemEventChecker checker;
    checker.Init(KernelType::AICUBE, DeviceType::ASCEND_910B2, RaceCheckType::CROSS_BLOCK_CHECK);
    checker.PushEvent(e1);
    checker.PushEvent(e2);
    MemEventChecker::RaceMemEventsIdx raceMemEvents;
    MemEventChecker::RaceMemEventsIdx missDcciEventsIdx;
    checker.ScanlineAlgorithm(raceMemEvents, missDcciEventsIdx);
    ASSERT_EQ(raceMemEvents.size(), 0);
}

}
