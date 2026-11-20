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
#include "core/framework/config_manager.h"

namespace {
using namespace Sanitizer;

// RAII 固定全局 checkDcci：析构恢复原值。gtest 同进程顺序执行，若某用例在
// ASSERT 失败后未复位该全局量，后续用例会静默改走 ScanGeneric（仍能通过断言，
// 但声明的被测路径零覆盖）。所有依赖分派路径的用例都经此 guard 固定开关。
class CheckDcciGuard {
public:
    explicit CheckDcciGuard(bool value) : origin_(ConfigManager::Instance().Get().checkDcci)
    {
        ConfigManager::Instance().Get().checkDcci = value;
    }
    ~CheckDcciGuard()
    {
        ConfigManager::Instance().Get().checkDcci = origin_;
    }
private:
    bool origin_;
};

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

    DenseVectorTime t;
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

    DenseVectorTime t;
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

    DenseVectorTime t1;
    t1.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    t1[static_cast<uint8_t>(PipeType::PIPE_MTE1)]++;
    e1.vt = t1;
    e1.pipe = PipeType::PIPE_MTE1;

    DenseVectorTime t2;
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

    DenseVectorTime t1;
    t1.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    t1[static_cast<uint8_t>(PipeType::PIPE_MTE1)]++;
    e1.vt = t1;
    e1.pipe = PipeType::PIPE_MTE1;

    DenseVectorTime t2;
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

    DenseVectorTime t1;
    t1.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    t1[static_cast<uint8_t>(PipeType::PIPE_MTE1)]++;
    e1.vt = t1;
    e1.pipe = PipeType::PIPE_MTE1;
    e1.memInfo.opType = AccessType::READ;

    DenseVectorTime t2;
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

    DenseVectorTime t1;
    t1.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    t1[static_cast<uint8_t>(PipeType::PIPE_MTE1)]++;
    e1.vt = t1;
    e1.pipe = PipeType::PIPE_MTE1;
    e1.memInfo.opType = AccessType::WRITE;

    DenseVectorTime t2;
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
    DenseVectorTime t1;
    e1.loc.coreId = 0;
    t1.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1);
    t1[static_cast<uint8_t>(PipeType::PIPE_MTE1) + e1.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    e1.vt = t1;
    e1.pipe = PipeType::PIPE_MTE1;
    e1.memInfo.opType = AccessType::READ;

    DenseVectorTime t2;
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
    DenseVectorTime t1;
    e1.loc.coreId = 0;
    t1.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1);
    t1[static_cast<uint8_t>(PipeType::PIPE_MTE1) + e1.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    e1.vt = t1;
    e1.pipe = PipeType::PIPE_MTE1;

    DenseVectorTime t2;
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
    DenseVectorTime t1;
    e1.loc.coreId = 0;
    t1.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1);
    t1[static_cast<uint8_t>(PipeType::PIPE_MTE1) + e1.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    e1.vt = t1;
    e1.pipe = PipeType::PIPE_MTE1;
    e1.memInfo.opType = AccessType::WRITE;

    DenseVectorTime t2;
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

    DenseVectorTime t1;
    e1.loc.coreId = 0;
    t1.resize(static_cast<uint8_t>(PipeType::SIZE), 1);
    t1[static_cast<uint8_t>(PipeType::PIPE_MTE1) + e1.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    e1.vt = t1;
    e1.pipe = PipeType::PIPE_MTE1;
    e1.memInfo.addr = addr;

    DenseVectorTime t2;
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
    DenseVectorTime t1;
    DenseVectorTime t2;

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

    DenseVectorTime t;
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

TEST(MemEventChecker, scalar_read_with_multi_cross_core_write_missing_dcci_expect_all_detected)
{
    const CheckDcciGuard dcciGuard(true); // 本用例测试 DCCI 缺失告警路径

    constexpr uint64_t addr = 0x1000UL;
    SanEvent event;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = addr;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 4U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.eventInfo.memInfo.dcciDistance = 0U;
    MemEvent eWrite1(event);
    MemEvent eWrite2(event);
    MemEvent eRead(event);

    uint8_t blockDim = 10U;
    DenseVectorTime tWrite1;
    eWrite1.loc.coreId = 1U;
    tWrite1.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1U);
    tWrite1[static_cast<uint8_t>(PipeType::PIPE_S_CAL) + eWrite1.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    eWrite1.vt = tWrite1;
    eWrite1.pipe = PipeType::PIPE_S_CAL;
    eWrite1.memInfo.opType = AccessType::WRITE;

    DenseVectorTime tWrite2;
    eWrite2.loc.coreId = 2U;
    tWrite2.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1U);
    tWrite2[static_cast<uint8_t>(PipeType::PIPE_S_CAL) + eWrite2.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    eWrite2.vt = tWrite2;
    eWrite2.pipe = PipeType::PIPE_S_CAL;
    eWrite2.memInfo.opType = AccessType::WRITE;

    DenseVectorTime tRead;
    eRead.loc.coreId = 0U;
    tRead.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1U);
    tRead[static_cast<uint8_t>(PipeType::PIPE_S_CAL) + eRead.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)]++;
    eRead.vt = tRead;
    eRead.pipe = PipeType::PIPE_S_CAL;
    eRead.memInfo.opType = AccessType::READ;

    MemEventChecker checker;
    checker.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, RaceCheckType::CROSS_BLOCK_CHECK);
    checker.PushEvent(eWrite1);
    checker.PushEvent(eWrite2);
    checker.PushEvent(eRead);
    MemEventChecker::RaceMemEventsIdx raceMemEvents;
    MemEventChecker::RaceMemEventsIdx missDcciEventsIdx;
    checker.ScanlineAlgorithm(raceMemEvents, missDcciEventsIdx);
    ASSERT_EQ(raceMemEvents.size(), 0U);
    ASSERT_EQ(missDcciEventsIdx.size(), 2U);
}

TEST(MemEventChecker, ub_events_should_not_trigger_missing_dcci)
{
    const CheckDcciGuard dcciGuard(true); // 本用例测试 DCCI 缺失告警路径

    constexpr uint64_t addr = 0x1000UL;
    SanEvent event;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = addr;
    event.eventInfo.memInfo.blockNum = 1U;
    event.eventInfo.memInfo.blockSize = 4U;
    event.eventInfo.memInfo.blockStride = 1U;
    event.eventInfo.memInfo.repeatTimes = 1U;
    event.eventInfo.memInfo.repeatStride = 1U;
    event.eventInfo.memInfo.dcciDistance = 0U;
    MemEvent eWrite(event);
    MemEvent eRead(event);

    uint8_t blockDim = 10U;
    DenseVectorTime tWrite;
    eWrite.loc.coreId = 1U;
    tWrite.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1U);
    tWrite[static_cast<uint8_t>(PipeType::PIPE_S_CAL) + eWrite.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)];
    eWrite.vt = tWrite;
    eWrite.pipe = PipeType::PIPE_S_CAL;
    eWrite.memInfo.opType = AccessType::WRITE;

    DenseVectorTime tRead;
    eRead.loc.coreId = 0U;
    tRead.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1U);
    tRead[static_cast<uint8_t>(PipeType::PIPE_S_CAL) + eRead.loc.coreId * static_cast<uint8_t>(PipeType::SIZE)];
    eRead.vt = tRead;
    eRead.pipe = PipeType::PIPE_S_CAL;
    eRead.memInfo.opType = AccessType::READ;

    MemEventChecker checker;
    checker.Init(KernelType::AIVEC, DeviceType::ASCEND_910B1, RaceCheckType::CROSS_BLOCK_CHECK);
    checker.PushEvent(eWrite);
    checker.PushEvent(eRead);
    MemEventChecker::RaceMemEventsIdx raceMemEvents;
    MemEventChecker::RaceMemEventsIdx missDcciEventsIdx;
    checker.ScanlineAlgorithm(raceMemEvents, missDcciEventsIdx);
    // UB 上的跨核标量读写不需要 DCCI，不应产生 missing DCCI 告警
    ASSERT_EQ(missDcciEventsIdx.size(), 0U);
}

namespace {
// 构造一条标量原子指令（SCALAR_RED/SCALAR_ATOM）解析后的READ+WRITE两个子事件，
// 模拟EventContainer::Push对MEMCPY_BLOCKS事件的拆分
void AppendScalarAtomicSubEvents(std::vector<MemEvent> &events, uint32_t coreId, uint64_t serialNo, uint64_t addr)
{
    SanEvent event;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = addr;
    event.eventInfo.memInfo.blockNum = 1;
    event.eventInfo.memInfo.blockSize = 4;
    event.eventInfo.memInfo.repeatTimes = 1;
    event.eventInfo.memInfo.isScalarAtomic = true;
    event.pipe = PipeType::PIPE_S;
    event.loc.coreId = coreId;
    event.serialNo = serialNo;

    uint8_t blockDim = 10;
    DenseVectorTime t;
    t.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1U);
    t[static_cast<uint8_t>(PipeType::PIPE_S) + coreId * static_cast<uint8_t>(PipeType::SIZE)]++;

    MemEvent readEvent(event);
    readEvent.memInfo.opType = AccessType::READ;
    readEvent.vt = t;
    events.push_back(readEvent);

    MemEvent writeEvent(event);
    writeEvent.memInfo.opType = AccessType::WRITE;
    writeEvent.vt = t;
    events.push_back(writeEvent);
}

MemEvent MakeScalarAtomicSubEvent(uint32_t coreId, uint64_t serialNo, uint64_t addr, AccessType opType)
{
    SanEvent event;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = addr;
    event.eventInfo.memInfo.blockNum = 1;
    event.eventInfo.memInfo.blockSize = 4;
    event.eventInfo.memInfo.repeatTimes = 1;
    event.eventInfo.memInfo.opType = opType;
    event.eventInfo.memInfo.isScalarAtomic = true;
    event.pipe = PipeType::PIPE_S;
    event.loc.coreId = coreId;
    event.serialNo = serialNo;

    uint8_t blockDim = 10;
    DenseVectorTime t;
    t.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1U);
    t[static_cast<uint8_t>(PipeType::PIPE_S) + coreId * static_cast<uint8_t>(PipeType::SIZE)]++;

    MemEvent memEvent(event);
    memEvent.vt = t;
    return memEvent;
}
}

TEST(MemEventChecker, scalar_atomic_instructions_expect_no_race)
{
    std::vector<MemEvent> events;
    AppendScalarAtomicSubEvents(events, 0U, 13U, 0x120000017000);
    AppendScalarAtomicSubEvents(events, 1U, 25U, 0x120000017000);
    AppendScalarAtomicSubEvents(events, 2U, 38U, 0x120000017000);
    RunAllRaceAlgExpectNoRace(events);
}

TEST(MemEventChecker, scalar_atomic_against_normal_write_expect_cross_core_race)
{
    std::vector<MemEvent> events;
    events.push_back(MakeScalarAtomicSubEvent(0U, 13U, 0x120000017000, AccessType::WRITE));
    events.push_back(MakeScalarAtomicSubEvent(1U, 25U, 0x120000017000, AccessType::WRITE));
    events[1].memInfo.isScalarAtomic = false;
    RunAllRaceAlgExpectCrossCoreRace(events);
}

namespace {
// 构造isAtomicMode=true时的GM搬运算子
MemEvent MakeVectorAtomicWrite(uint32_t coreId, uint64_t serialNo, uint64_t addr, PipeType pipe)
{
    SanEvent event;
    event.eventInfo.memInfo.memType = MemType::GM;
    event.eventInfo.memInfo.addr = addr;
    event.eventInfo.memInfo.blockNum = 1;
    event.eventInfo.memInfo.blockSize = 4;
    event.eventInfo.memInfo.repeatTimes = 1;
    event.eventInfo.memInfo.opType = AccessType::WRITE;
    event.pipe = pipe;
    event.loc.coreId = coreId;
    event.serialNo = serialNo;
    event.isAtomicMode = true;

    uint8_t blockDim = 10;
    DenseVectorTime t;
    t.resize(static_cast<uint8_t>(PipeType::SIZE) * blockDim, 1U);
    t[static_cast<uint8_t>(pipe) + coreId * static_cast<uint8_t>(PipeType::SIZE)]++;

    MemEvent memEvent(event);
    memEvent.vt = t;
    return memEvent;
}
}

TEST(MemEventChecker, scalar_atomic_and_vector_atomic_expect_no_race)
{
    std::vector<MemEvent> events;
    AppendScalarAtomicSubEvents(events, 0U, 13U, 0x120000017000);
    events.push_back(MakeVectorAtomicWrite(1U, 25U, 0x120000017000, PipeType::PIPE_MTE3));
    RunAllRaceAlgExpectNoRace(events);
}

TEST(MemEventChecker, vector_atomic_writes_expect_no_race)
{
    std::vector<MemEvent> events;
    events.push_back(MakeVectorAtomicWrite(0U, 13U, 0x120000017000, PipeType::PIPE_MTE3));
    events.push_back(MakeVectorAtomicWrite(1U, 25U, 0x120000017000, PipeType::PIPE_MTE3));
    RunAllRaceAlgExpectNoRace(events);
}

namespace {
// 构造 MIX 核 SINGLE_BLOCK 用例的内存事件。MIX 核的向量时钟索引按 blockIdx 展开：
// blockIdx = aicoreIndex*C220_MIX_SUB_BLOCKDIM + 子槽号（向量事件用 coreId%子槽数，
// 立方事件固定用最后一个子槽）。时钟用 DenseVectorTime 构造后按展开索引置分量。
// isCube=false 表示 AIVEC（coreId = aicoreIndex*C220_VEC_SUB_BLOCKDIM，展开子槽
// coreId%2）；isCube=true 表示 AICORE（coreId = aicoreIndex，展开子槽固定 2）。
// 同一 aicoreIndex 下 AIVEC 与 AICORE 的 coreId 不同、blockIdx 也不同——这是生产
// 上 AIV checker（同时接收 AIVEC 与 AICORE）可达的跨 blockIdx 组合；AICUBE 事件
// 由 SingleBlockRaceCheck 分流到 AIC checker，不会与本组事件同 checker。
MemEvent MakeMixBlockEvent(uint32_t aicoreIndex, bool isCube, PipeType pipe, AccessType opType,
    uint64_t addr, uint32_t serialNo, const DenseVectorTime &fullVt)
{
    SanEvent event;
    event.eventInfo.memInfo.memType = MemType::UB;
    event.eventInfo.memInfo.addr = addr;
    event.eventInfo.memInfo.blockNum = 8;
    event.eventInfo.memInfo.blockSize = 32;
    event.eventInfo.memInfo.repeatTimes = 1;
    event.eventInfo.memInfo.opType = opType;
    event.pipe = pipe;
    event.loc.coreId = isCube ? aicoreIndex : aicoreIndex * C220_VEC_SUB_BLOCKDIM;
    event.loc.blockType = isCube ? BlockType::AICORE : BlockType::AIVEC;
    event.serialNo = serialNo;

    MemEvent memEvent(event);
    memEvent.vt = fullVt;
    return memEvent;
}
}

// 同一 block 内两条互不知晓的并发 WAW：优化后快速路径应正确判定为核内竞争
TEST(MemEventChecker, mix_single_block_write_after_write_expect_core_inner_race)
{
    const CheckDcciGuard dcciGuard(false); // 固定走分桶快速路径
    const uint32_t blockDim = 10;
    const uint32_t blockIdx = 1 * C220_MIX_SUB_BLOCKDIM;   // aicore=1 的向量槽对应的展开 blockIdx
    const uint32_t nPipe = static_cast<uint32_t>(PipeType::SIZE);

    // e1 在 MTE1、e2 在 MTE2，各自时钟只推进自己流水线、未见过对方 → 两事件并发
    DenseVectorTime t1(static_cast<size_t>(blockDim) * nPipe, 1U);
    t1[blockIdx * nPipe + static_cast<uint32_t>(PipeType::PIPE_MTE1)] = 5U;
    MemEvent e1 = MakeMixBlockEvent(1U, false, PipeType::PIPE_MTE1, AccessType::WRITE, 0U, 3U, t1);

    DenseVectorTime t2(static_cast<size_t>(blockDim) * nPipe, 1U);
    t2[blockIdx * nPipe + static_cast<uint32_t>(PipeType::PIPE_MTE2)] = 7U;
    MemEvent e2 = MakeMixBlockEvent(1U, false, PipeType::PIPE_MTE2, AccessType::WRITE, 0U, 5U, t2);

    MemEventChecker checker;
    checker.Init(KernelType::MIX, DeviceType::ASCEND_910B2, RaceCheckType::SINGLE_BLOCK_CHECK);
    checker.PushEvent(e1);
    checker.PushEvent(e2);
    MemEventChecker::RaceMemEventsIdx raceMemEventsIdx;
    MemEventChecker::RaceMemEventsIdx missDcciEventsIdx;
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    ASSERT_EQ(raceMemEventsIdx.size(), 1U);
}

// 差分测试：同一对事件的时钟/原子判定在快速路径（全部事件同 block）与回退路径（多 block 混入）
// 下必须一致——IsRaceCandidate 两个模板实例共享同一语义的回归看护。
// fast 侧：e1/e2 同为 aicore=1 的 AIVEC（展开 blockIdx 3），MTE1/MTE2 各自推进自时钟、
//          互不知晓 → 并发对，走 IsRaceCandidate<true>；
// fallback 侧：e1' 与 e1 完全相同；e3 与 e2 除 blockIdx 外完全同构（同为 MTE2、自时钟 7、
//          其余分量 1），但为 aicore=1 的 AICORE（展开 blockIdx 5）→ 与 e1' 不同 block，
//          走 IsRaceCandidate<false>。两组候选对进入判定的四个时钟分量值完全相同
//          （a1=5, a2=1, b1=1, b2=7），判定结果必须相同。
// NDEBUG 保护的计数器断言两侧路径确实被执行，防止事件构造错误导致用例静默空转
TEST(MemEventChecker, mix_same_vs_fallback_path_consistent_judgement)
{
    const CheckDcciGuard dcciGuard(false); // 固定走分桶快速路径，防止全局开关泄漏改走 ScanGeneric
    const uint32_t blockDim = 10;
    const uint32_t nPipe = static_cast<uint32_t>(PipeType::SIZE);
    const uint32_t blockIdx1 = 1 * C220_MIX_SUB_BLOCKDIM;              // AIVEC(1)：展开 blockIdx 3
    const uint32_t blockIdx2 = 1 * C220_MIX_SUB_BLOCKDIM + C220_VEC_SUB_BLOCKDIM; // AICORE(1)：展开 blockIdx 5

    // 互不知晓的并发对：各自时钟只推进自己流水线（分量置在各自展开索引上）
    DenseVectorTime t1(static_cast<size_t>(blockDim) * nPipe, 1U);
    t1[blockIdx1 * nPipe + static_cast<uint32_t>(PipeType::PIPE_MTE1)] = 5U;
    DenseVectorTime t2(static_cast<size_t>(blockDim) * nPipe, 1U);
    t2[blockIdx1 * nPipe + static_cast<uint32_t>(PipeType::PIPE_MTE2)] = 7U;

    MemEvent e1 = MakeMixBlockEvent(1U, false, PipeType::PIPE_MTE1, AccessType::WRITE, 0U, 3U, t1);
    MemEvent e2 = MakeMixBlockEvent(1U, false, PipeType::PIPE_MTE2, AccessType::WRITE, 0U, 5U, t2);
    // e1' 与 e1 相同；e3 与 e2 同构（MTE2、自时钟 7、其余分量 1），但位于 AICORE 槽
    // （blockIdx 5）——其自时钟分量须置在 blockIdx2 的展开索引上，与 e2 的 t2 仅差槽位
    DenseVectorTime t3(static_cast<size_t>(blockDim) * nPipe, 1U);
    t3[blockIdx2 * nPipe + static_cast<uint32_t>(PipeType::PIPE_MTE2)] = 7U;
    MemEvent e1Copy = e1;
    MemEvent e3 = MakeMixBlockEvent(1U, true, PipeType::PIPE_MTE2, AccessType::WRITE, 0U, 5U, t3);

    MemEventChecker fastChecker;
    fastChecker.Init(KernelType::MIX, DeviceType::ASCEND_910B2, RaceCheckType::SINGLE_BLOCK_CHECK);
    fastChecker.PushEvent(e1);
    fastChecker.PushEvent(e2);
    MemEventChecker::RaceMemEventsIdx fastRaces;
    MemEventChecker::RaceMemEventsIdx fastDcci;
    fastChecker.ScanlineAlgorithm(fastRaces, fastDcci);

    MemEventChecker fallbackChecker;
    fallbackChecker.Init(KernelType::MIX, DeviceType::ASCEND_910B2, RaceCheckType::SINGLE_BLOCK_CHECK);
    fallbackChecker.PushEvent(e1Copy);
    fallbackChecker.PushEvent(e3);
    MemEventChecker::RaceMemEventsIdx fallbackRaces;
    MemEventChecker::RaceMemEventsIdx fallbackDcci;
    fallbackChecker.ScanlineAlgorithm(fallbackRaces, fallbackDcci);

#ifndef NDEBUG
    // 路径断言（不经生产代码计数器）：allSameBlock 即 pipeIdx/NPIPE 的一致性校验，
    // 故在 UT 侧按同一 GetPipeIdxByMemEvent 断言 floor 关系——fast 侧两事件 floor
    // 相同 ⟺ checker 走快速路径；fallback 侧 floor 不同 ⟺ 走回退路径
    const uint32_t floorFast1 = GetPipeIdxByMemEvent<RaceCheckType::SINGLE_BLOCK_CHECK>(
        e1, KernelType::MIX, DeviceType::ASCEND_910B2) / nPipe;
    const uint32_t floorFast2 = GetPipeIdxByMemEvent<RaceCheckType::SINGLE_BLOCK_CHECK>(
        e2, KernelType::MIX, DeviceType::ASCEND_910B2) / nPipe;
    const uint32_t floorBack1 = GetPipeIdxByMemEvent<RaceCheckType::SINGLE_BLOCK_CHECK>(
        e1Copy, KernelType::MIX, DeviceType::ASCEND_910B2) / nPipe;
    const uint32_t floorBack3 = GetPipeIdxByMemEvent<RaceCheckType::SINGLE_BLOCK_CHECK>(
        e3, KernelType::MIX, DeviceType::ASCEND_910B2) / nPipe;
    ASSERT_EQ(floorFast1, floorFast2); // fast 侧：同 floor → allSameBlock 恒真 → IsRaceCandidate<true>
    ASSERT_NE(floorBack1, floorBack3); // fallback 侧：异 floor → allSameBlock 假 → IsRaceCandidate<false>
#endif
    // 快速路径：全部事件同 block；回退路径：e1'(AIVEC, blockIdx 3) 与 e3(AICORE, blockIdx 5) 不同 block
    ASSERT_EQ(fastRaces.size(), fallbackRaces.size());
    for (size_t i = 0; i < fastRaces.size(); ++i) {
        ASSERT_EQ(fastRaces[i].first.eventIdx, fallbackRaces[i].first.eventIdx);
        ASSERT_EQ(fastRaces[i].second.eventIdx, fallbackRaces[i].second.eventIdx);
    }
}

// 跨 block 的两条并发 WAW："全部事件同 block"前提不成立时自动回退逐对原逻辑，仍应正确报竞争。
// 组合取 AIV checker 生产可达的 AIVEC + AICORE（同 aicore=1，展开 blockIdx 分别为 3 和 5）
TEST(MemEventChecker, mix_multi_block_write_after_write_expect_core_inner_race)
{
    const CheckDcciGuard dcciGuard(false); // 固定走分桶快速路径
    const uint32_t blockDim = 10;
    const uint32_t nPipe = static_cast<uint32_t>(PipeType::SIZE);
    // e1 在 aicore=1 的向量槽（blockIdx 3），e2 在 aicore=1 的立方槽（blockIdx 5）
    const uint32_t blockIdx1 = 1 * C220_MIX_SUB_BLOCKDIM;
    const uint32_t blockIdx2 = 1 * C220_MIX_SUB_BLOCKDIM + C220_VEC_SUB_BLOCKDIM;

    DenseVectorTime t1(static_cast<size_t>(blockDim) * nPipe, 1U);
    t1[blockIdx1 * nPipe + static_cast<uint32_t>(PipeType::PIPE_MTE1)] = 5U;
    MemEvent e1 = MakeMixBlockEvent(1U, false, PipeType::PIPE_MTE1, AccessType::WRITE, 0U, 3U, t1);

    DenseVectorTime t2(static_cast<size_t>(blockDim) * nPipe, 1U);
    t2[blockIdx2 * nPipe + static_cast<uint32_t>(PipeType::PIPE_MTE2)] = 7U;
    MemEvent e2 = MakeMixBlockEvent(1U, true, PipeType::PIPE_MTE2, AccessType::WRITE, 0U, 5U, t2);

    MemEventChecker checker;
    checker.Init(KernelType::MIX, DeviceType::ASCEND_910B2, RaceCheckType::SINGLE_BLOCK_CHECK);
    checker.PushEvent(e1);
    checker.PushEvent(e2);
    MemEventChecker::RaceMemEventsIdx raceMemEventsIdx;
    MemEventChecker::RaceMemEventsIdx missDcciEventsIdx;
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    ASSERT_EQ(raceMemEventsIdx.size(), 1U);
}

// 同一 block 内存在同步链先后顺序的两条 WAW：e2 的时钟已合并 e1 的 MTE1 分量（即 e2 晚于 e1
// 且完全知晓 e1 的进展），前置排除应拒绝该候选对，不误报
TEST(MemEventChecker, mix_single_block_sync_chain_expect_no_race)
{
    const CheckDcciGuard dcciGuard(false); // 固定走分桶快速路径
    const uint32_t blockDim = 10;
    const uint32_t blockIdx = 1 * C220_MIX_SUB_BLOCKDIM;
    const uint32_t nPipe = static_cast<uint32_t>(PipeType::SIZE);
    const uint32_t mte1 = blockIdx * nPipe + static_cast<uint32_t>(PipeType::PIPE_MTE1);
    const uint32_t mte2 = blockIdx * nPipe + static_cast<uint32_t>(PipeType::PIPE_MTE2);

    // e1（MTE1 上时钟 2，未见 MTE2）；e2（MTE1 分量 2 + MTE2 分量 3：已同步过 e1）
    // e2 见过的 MTE1 时钟 >= e1 自己的 MTE1 时钟 → e1 happens-before e2，非并发
    DenseVectorTime t1(static_cast<size_t>(blockDim) * nPipe, 1U);
    t1[mte1] = 2U;
    MemEvent e1 = MakeMixBlockEvent(1U, false, PipeType::PIPE_MTE1, AccessType::WRITE, 0U, 3U, t1);

    DenseVectorTime t2(static_cast<size_t>(blockDim) * nPipe, 1U);
    t2[mte1] = 2U;
    t2[mte2] = 3U;
    MemEvent e2 = MakeMixBlockEvent(1U, false, PipeType::PIPE_MTE2, AccessType::WRITE, 0U, 5U, t2);

    MemEventChecker checker;
    checker.Init(KernelType::MIX, DeviceType::ASCEND_910B2, RaceCheckType::SINGLE_BLOCK_CHECK);
    checker.PushEvent(e1);
    checker.PushEvent(e2);
    MemEventChecker::RaceMemEventsIdx raceMemEventsIdx;
    MemEventChecker::RaceMemEventsIdx missDcciEventsIdx;
    checker.ScanlineAlgorithm(raceMemEventsIdx, missDcciEventsIdx);
    ASSERT_EQ(raceMemEventsIdx.size(), 0U);
}

}
