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
#include <algorithm>
#define __NPU_ARCH__ 3101
#define SIMT_MODE
#define private public
#include "plugin/online_check.h"
#undef private

using namespace Sanitizer;

namespace {

TEST(OnlineCheck, test_update_sync_thread_pc_num)
{
    uint64_t blockDim = 3;
    uint64_t cacheSize = 10 * MB_TO_BYTES;
    std::vector<uint8_t> memInfo(cacheSize * blockDim, 0);
    RecordGlobalHead head{};
    RecordBlockHead blockHead{};
    std::vector<HostMemoryInfo> hostmems;
    hostmems.push_back({0x100, 100});
    blockHead.hostMemoryInfoPtr = reinterpret_cast<HostMemoryInfo *>(hostmems.data());
    blockHead.hostMemoryNum = hostmems.size();
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    std::copy_n(reinterpret_cast<uint8_t const*>(&blockHead), sizeof(RecordBlockHead),
        memInfo.begin() + sizeof(RecordGlobalHead));
    std::copy_n(reinterpret_cast<uint8_t const*>(hostmems.data()), sizeof(HostMemoryInfo) * blockHead.hostMemoryNum,
        memInfo.begin() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead));
    uint64_t allHeadSize = sizeof(RecordGlobalHead) + sizeof(HostMemoryInfo) * blockHead.hostMemoryNum +
        sizeof(RecordBlockHead);

    OnlineCheck checker = OnlineCheck();
    checker.Init(memInfo.data(), memInfo.data() + allHeadSize, memInfo.data() + sizeof(RecordGlobalHead), 0);

    checker.UpdateSyncThreadPcNum(0x1000);
    checker.UpdateSyncThreadPcNum(0x2000);
    auto simtBlockHead = reinterpret_cast<__gm__ SimtRecordBlockHead *>(memInfo.data() + allHeadSize);

    ASSERT_EQ(simtBlockHead->syncThreadPC[0], 0x1000);
    ASSERT_EQ(simtBlockHead->syncThreadPC[1], 0x2000);
    ASSERT_EQ(simtBlockHead->syncThreadNum[0], 1);
    ASSERT_EQ(simtBlockHead->syncThreadNum[1], 1);

    checker.UpdateSyncThreadPcNum(0x1000);
    ASSERT_EQ(simtBlockHead->syncThreadNum[0], 2);
}

TEST(OnlineCheck, test_sort_sync_thread_pc_num_in_place)
{
    uint64_t blockDim = 3;
    uint64_t cacheSize = 10 * MB_TO_BYTES;
    std::vector<uint8_t> memInfo(cacheSize * blockDim, 0);
    RecordGlobalHead head{};
    RecordBlockHead blockHead{};
    std::vector<HostMemoryInfo> hostmems;
    hostmems.push_back({0x100, 100});
    blockHead.hostMemoryInfoPtr = reinterpret_cast<HostMemoryInfo *>(hostmems.data());
    blockHead.hostMemoryNum = hostmems.size();
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    std::copy_n(reinterpret_cast<uint8_t const*>(&blockHead), sizeof(RecordBlockHead),
        memInfo.begin() + sizeof(RecordGlobalHead));
    std::copy_n(reinterpret_cast<uint8_t const*>(hostmems.data()), sizeof(HostMemoryInfo) * blockHead.hostMemoryNum,
        memInfo.begin() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead));
    uint64_t allHeadSize = sizeof(RecordGlobalHead) + sizeof(HostMemoryInfo) * blockHead.hostMemoryNum +
        sizeof(RecordBlockHead);

    OnlineCheck checker = OnlineCheck();
    checker.Init(memInfo.data(), memInfo.data() + allHeadSize, memInfo.data() + sizeof(RecordGlobalHead), 0);

    uint64_t threadOffset0 = checker.globalHead_->offsetInfo.simtErrorInfo.offset;
    __gm__ uint8_t *simtBlock0 = checker.memInfoSimd_ + threadOffset0;
    __gm__ SimtRecordBlockHead *simtBlockHead0 = reinterpret_cast<__gm__ SimtRecordBlockHead *>(simtBlock0);
    simtBlockHead0->syncThreadPC[0] = 0x1000;
    simtBlockHead0->syncThreadPC[1] = 0;
    simtBlockHead0->syncThreadNum[0] = 1;
    simtBlockHead0->syncThreadNum[1] = 0;

    uint64_t threadOffset1 = checker.globalHead_->offsetInfo.simtErrorInfo.offset +
        1 * (checker.globalHead_->offsetInfo.simtErrorInfo.size + sizeof(SimtRecordBlockHead));
    __gm__ uint8_t *simtBlock1 = checker.memInfoSimd_ + threadOffset1;
    __gm__ SimtRecordBlockHead *simtBlockHead1 = reinterpret_cast<__gm__ SimtRecordBlockHead *>(simtBlock1);
    simtBlockHead1->syncThreadPC[0] = 0x2000;
    simtBlockHead1->syncThreadPC[1] = 0x3000;
    simtBlockHead1->syncThreadNum[0] = 2;
    simtBlockHead1->syncThreadNum[1] = 1;

    uint64_t threadOffset2 = checker.globalHead_->offsetInfo.simtErrorInfo.offset +
        2 * (checker.globalHead_->offsetInfo.simtErrorInfo.size + sizeof(SimtRecordBlockHead));
    __gm__ uint8_t *simtBlock2 = checker.memInfoSimd_ + threadOffset2;
    __gm__ SimtRecordBlockHead *simtBlockHead2 = reinterpret_cast<__gm__ SimtRecordBlockHead *>(simtBlock2);
    simtBlockHead2->syncThreadPC[0] = 0x1000;
    simtBlockHead2->syncThreadPC[1] = 0x3000;
    simtBlockHead2->syncThreadNum[0] = 1;
    simtBlockHead2->syncThreadNum[1] = 2;

    uint16_t validPcNum = 0;
    uint32_t tmpCounts[SIMT_THREAD_MAX_PC_NUM] = {0};
    checker.simdBlockHead_->blockInfo.simtEndLastThread = 3;
    checker.SortSyncThreadPcNumInPlace(simtBlockHead0, validPcNum, tmpCounts);

    ASSERT_EQ(validPcNum, 3);
    ASSERT_EQ(simtBlockHead0->syncThreadPC[0], 0x1000);
    ASSERT_EQ(simtBlockHead0->syncThreadPC[1], 0x2000);
    ASSERT_EQ(simtBlockHead0->syncThreadPC[2], 0x3000);

    ASSERT_EQ(simtBlockHead0->syncThreadNum[0], 1);
    ASSERT_EQ(simtBlockHead0->syncThreadNum[1], 0);
    ASSERT_EQ(simtBlockHead0->syncThreadNum[2], 0);

    ASSERT_EQ(simtBlockHead1->syncThreadNum[0], 0);
    ASSERT_EQ(simtBlockHead1->syncThreadNum[1], 2);
    ASSERT_EQ(simtBlockHead1->syncThreadNum[2], 1);

    ASSERT_EQ(simtBlockHead2->syncThreadNum[0], 1);
    ASSERT_EQ(simtBlockHead2->syncThreadNum[1], 0);
    ASSERT_EQ(simtBlockHead2->syncThreadNum[2], 2);
}

TEST(OnlineCheck, check_sync_thread_misuse_expect_one_error)
{
    uint64_t blockDim = 1;
    uint64_t cacheSize = 10 * MB_TO_BYTES;
    std::vector<uint8_t> memInfo(cacheSize * blockDim, 0);
    RecordGlobalHead head{};
    head.checkParms.synccheck = true;
    RecordBlockHead blockHead{};
    std::vector<HostMemoryInfo> hostmems;
    hostmems.push_back({0x100, 100});
    blockHead.hostMemoryInfoPtr = reinterpret_cast<HostMemoryInfo *>(hostmems.data());
    blockHead.hostMemoryNum = hostmems.size();
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    std::copy_n(reinterpret_cast<uint8_t const*>(&blockHead), sizeof(RecordBlockHead),
        memInfo.begin() + sizeof(RecordGlobalHead));
    std::copy_n(reinterpret_cast<uint8_t const*>(hostmems.data()), sizeof(HostMemoryInfo) * blockHead.hostMemoryNum,
        memInfo.begin() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead));
    uint64_t allHeadSize = sizeof(RecordGlobalHead) + sizeof(HostMemoryInfo) * blockHead.hostMemoryNum +
        sizeof(RecordBlockHead);

    OnlineCheck checker = OnlineCheck();
    checker.Init(memInfo.data(), memInfo.data() + allHeadSize, memInfo.data() + sizeof(RecordGlobalHead), 0);

    uint64_t threadOffset0 = checker.globalHead_->offsetInfo.simtErrorInfo.offset;
    __gm__ uint8_t *simtBlock0 = checker.memInfoSimd_ + threadOffset0;
    __gm__ SimtRecordBlockHead *simtBlockHead0 = reinterpret_cast<__gm__ SimtRecordBlockHead *>(simtBlock0);
    simtBlockHead0->syncThreadPC[0] = 0x1000;
    simtBlockHead0->syncThreadPC[1] = 0;
    simtBlockHead0->syncThreadNum[0] = 1;
    simtBlockHead0->syncThreadNum[1] = 0;

    uint64_t threadOffset1 = checker.globalHead_->offsetInfo.simtErrorInfo.offset +
        1 * (checker.globalHead_->offsetInfo.simtErrorInfo.size + sizeof(SimtRecordBlockHead));
    __gm__ uint8_t *simtBlock1 = checker.memInfoSimd_ + threadOffset1;
    __gm__ SimtRecordBlockHead *simtBlockHead1 = reinterpret_cast<__gm__ SimtRecordBlockHead *>(simtBlock1);
    simtBlockHead1->syncThreadPC[0] = 0x2000;
    simtBlockHead1->syncThreadPC[1] = 0x3000;
    simtBlockHead1->syncThreadNum[0] = 2;
    simtBlockHead1->syncThreadNum[1] = 1;

    checker.simdBlockHead_->blockInfo.simtEndLastThread = 2;
    SimtEmptyRecord record = {
        .location = {10, 10, 0x10, 0},
        .threadLoc = {1, 2, 1},
    };
    AddrInfo addrInfo = {
        .location = record.location,
        .threadLoc = record.threadLoc,
    };

    checker.Do<RecordType::SIMT_END>(addrInfo, record);

    auto recordTypePtr = reinterpret_cast<RecordType const*>(memInfo.data() + allHeadSize +
                                                             sizeof(SimtRecordBlockHead));
    ASSERT_EQ(*recordTypePtr, RecordType::ONLINE_ERROR);
}

}
