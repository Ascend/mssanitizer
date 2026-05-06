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
#include <algorithm>
#include "ccec_defs.h"
#include "plugin/online_check.h"

using namespace Sanitizer;

TEST(OnlineCheck, check_illegal_simt_record_expect_one_error)
{
    uint64_t blockDim = 3;
    uint64_t cacheSize = 10 * MB_TO_BYTES;
    std::vector<uint8_t> memInfo(cacheSize * blockDim, 0);
    RecordGlobalHead head{};
    RecordBlockHead blockHead{};
    std::vector<HostMemoryInfo> hostmems;
    hostmems.push_back({0x100, 100});
    hostmems.push_back({0x300, 50});
    blockHead.hostMemoryInfoPtr = reinterpret_cast<HostMemoryInfo *>(hostmems.data());
    blockHead.hostMemoryNum = hostmems.size();
    head.offsetInfo.simtErrorInfo.size = 500;
    head.checkParms.defaultcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    std::copy_n(reinterpret_cast<uint8_t const*>(&blockHead), sizeof(RecordBlockHead),
        memInfo.begin() + sizeof(RecordGlobalHead));
    std::copy_n(reinterpret_cast<uint8_t const*>(hostmems.data()), sizeof(HostMemoryInfo) * blockHead.hostMemoryNum,
        memInfo.begin() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead));
    uint64_t allHeadSize = sizeof(RecordGlobalHead) + sizeof(HostMemoryInfo) * blockHead.hostMemoryNum +
        sizeof(RecordBlockHead);
    OnlineCheck checker = OnlineCheck();
    checker.Init(memInfo.data(), memInfo.data() + allHeadSize, memInfo.data() + sizeof(RecordGlobalHead), 0);
    SimtLoadStoreRecord record = {
        .addr = 0x100,
        .size = 101,
        .location = {10, 10, 0x10, 1},
        .threadLoc = {10, 20, 5},
        .space = AddressSpace::GM,
    };
    checker.Process<RecordType::SIMT_LDG>(record);
    
    auto recordTypePtr = reinterpret_cast<RecordType const*>(memInfo.data() + allHeadSize +
        sizeof(SimtRecordBlockHead));
    ASSERT_EQ(*recordTypePtr, RecordType::ONLINE_ERROR);
    recordTypePtr++;
    auto errorRecord = reinterpret_cast<KernelErrorRecord const*>(recordTypePtr);
    ASSERT_EQ(errorRecord->errorNum, 1);
    ASSERT_EQ(errorRecord->recordSize, sizeof(SimtLoadStoreRecord));
    ASSERT_EQ(errorRecord->recordType, RecordType::SIMT_LDG);

    errorRecord++;
    auto simtRecord = reinterpret_cast<SimtLoadStoreRecord const*>(errorRecord);
    ASSERT_EQ(simtRecord->addr, 0x100);
    ASSERT_EQ(simtRecord->space, AddressSpace::GM);
    ASSERT_EQ(simtRecord->threadLoc.idX, 10);
    ASSERT_EQ(simtRecord->threadLoc.idY, 20);
    ASSERT_EQ(simtRecord->threadLoc.idZ, 5);
    ASSERT_EQ(simtRecord->size, 101);
    simtRecord++;
    auto errorDesc = reinterpret_cast<KernelErrorDesc const*>(simtRecord);
    ASSERT_EQ(errorDesc->errorType, KernelErrorType::ILLEGAL_ADDR_READ);
    ASSERT_EQ(errorDesc->payload.illegalDesc.illegalSize, 1);
    ASSERT_EQ(errorDesc->payload.illegalDesc.addr, 0x100);
    ASSERT_EQ(errorDesc->space, AddressSpace::GM);
    ASSERT_EQ(errorDesc->location.blockId, 1);
}

TEST(OnlineCheck, check_normal_simt_record_expect_success)
{
    uint64_t blockDim = 1;
    uint64_t cacheSize = 10 * MB_TO_BYTES;
    std::vector<uint8_t> memInfo(cacheSize * blockDim, 0);
    RecordGlobalHead head{};
    head.simtInfo.ubDynamicSize = 253952UL;
    head.checkParms.defaultcheck = true;
    RecordBlockHead blockHead{};
    std::vector<HostMemoryInfo> hostmems;
    hostmems.push_back({0x100, 100});
    hostmems.push_back({0x200, 50});
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
    SimtLoadStoreRecord record = {
        .addr = 0x100,
        .size = 100,
        .location = {10, 10, 0x10, 1},
        .threadLoc = {20, 10, 3},
        .space = AddressSpace::GM,
    };
    checker.Process<RecordType::SIMT_LDG>(record);
    auto recordTypePtr = reinterpret_cast<RecordType const*>(memInfo.data() + allHeadSize +
        sizeof(SimtRecordBlockHead));
    ASSERT_EQ(*recordTypePtr, RecordType{});
    record.space = AddressSpace::UB;
    checker.Process<RecordType::SIMT_LDG>(record);
    ASSERT_EQ(*recordTypePtr, RecordType{});
    record.addr = 0x110;
    record.size = 50;
    checker.Process<RecordType::SIMT_STG>(record);
    ASSERT_EQ(*recordTypePtr, RecordType{});
    record.addr = 0x200;
    checker.Process<RecordType::SIMT_STG>(record);
    ASSERT_EQ(*recordTypePtr, RecordType{});
    record.addr = 0x210;
    record.size = 30;
    checker.Process<RecordType::SIMT_STG>(record);
    ASSERT_EQ(*recordTypePtr, RecordType{});
}

TEST(OnlineCheck, check_normal_simt_record_with_extra_info_and_all_block_check_expect_success)
{
    uint64_t blockDim = 1;
    uint64_t cacheSize = 10 * MB_TO_BYTES;
    std::vector<uint8_t> memInfo(cacheSize * blockDim, 0);
    RecordGlobalHead head{};
    head.kernelInfo.kernelParamNum = 4;
    head.checkParms.defaultcheck = true;
    RecordBlockHead blockHead{};
    std::vector<HostMemoryInfo> hostmems;
    hostmems.push_back({0x100, 100});
    hostmems.push_back({0x200, 50});
    hostmems.push_back({0x0, 1000});
    hostmems.push_back({0x0, 200});
    hostmems.push_back({0x0, 300});
    hostmems.push_back({0x0, 20});
    hostmems.push_back({0x0, 0});
    std::vector<uint64_t> paraBaseAddrs = {0x800, 0x400, 0x100, 0x400};
    blockHead.hostMemoryInfoPtr = reinterpret_cast<HostMemoryInfo *>(hostmems.data());
    blockHead.hostMemoryNum = hostmems.size();
    ParaBaseRegister paraBase{};
    paraBase.size = paraBaseAddrs.size() * sizeof(uintptr_t);
    paraBase.addr = reinterpret_cast<uint64_t>(paraBaseAddrs.data());
    int64_t regIdx = GetRegisterIdx();
    blockHead.paraBase = paraBase;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    std::copy_n(reinterpret_cast<uint8_t const*>(&blockHead), sizeof(RecordBlockHead),
        memInfo.begin() + sizeof(RecordGlobalHead));
    std::copy_n(reinterpret_cast<uint8_t const*>(hostmems.data()), sizeof(HostMemoryInfo) * blockHead.hostMemoryNum,
        memInfo.begin() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead));
    uint64_t allHeadSize = sizeof(RecordGlobalHead) + sizeof(HostMemoryInfo) * blockHead.hostMemoryNum +
        sizeof(RecordBlockHead);
    OnlineCheck checker = OnlineCheck();
    checker.Init(memInfo.data(), memInfo.data() + allHeadSize, memInfo.data() + sizeof(RecordGlobalHead), 1);
    checker.ProcessParaBaseAddr();
    ASSERT_EQ(hostmems[0].addr, 0x100);
    ASSERT_EQ(hostmems[0].size, 306);
    ASSERT_EQ(hostmems[1].addr, 0x400);
    ASSERT_EQ(hostmems[1].size, 200);
    ASSERT_EQ(hostmems[2].addr, 0x800);
    ASSERT_EQ(hostmems[2].size, 1000);
    for (size_t i = 3; i < hostmems.size(); ++i) {
        ASSERT_EQ(hostmems[i].addr, 0x0);
        ASSERT_EQ(hostmems[i].size, 0);
    }
    auto simdHead = reinterpret_cast<const RecordBlockHead*>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_TRUE(simdHead->extraWriteSuccess);
}

TEST(OnlineCheck, check_normal_simt_record_with_extra_info_and_single_check_expect_success)
{
    uint64_t blockDim = 2;
    uint64_t cacheSize = 10 * MB_TO_BYTES;
    std::vector<uint8_t> memInfo(cacheSize * blockDim, 0);
    RecordGlobalHead head{};
    head.kernelInfo.kernelParamNum = 4;
    head.checkParms.checkBlockId = 4;
    head.checkParms.defaultcheck = true;
    RecordBlockHead blockHead{};
    std::vector<HostMemoryInfo> hostmems;
    hostmems.push_back({0x100, 100});
    hostmems.push_back({0x200, 50});
    hostmems.push_back({0x0, 1000});
    hostmems.push_back({0x0, 200});
    hostmems.push_back({0x0, 300});
    hostmems.push_back({0x0, 20});
    hostmems.push_back({0x0, 0});
    std::vector<uint64_t> paraBaseAddrs = {0x800, 0x400, 0x100, 0x400};
    blockHead.hostMemoryInfoPtr = reinterpret_cast<HostMemoryInfo *>(hostmems.data());
    blockHead.hostMemoryNum = hostmems.size();
    ParaBaseRegister paraBase{};
    paraBase.size = paraBaseAddrs.size() * sizeof(uintptr_t);
    paraBase.addr = reinterpret_cast<uint64_t>(paraBaseAddrs.data());
    int64_t regIdx = GetRegisterIdx();
    blockHead.paraBase = paraBase;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    std::copy_n(reinterpret_cast<uint8_t const*>(&blockHead), sizeof(RecordBlockHead),
        memInfo.begin() + sizeof(RecordGlobalHead));
    std::copy_n(reinterpret_cast<uint8_t const*>(hostmems.data()), sizeof(HostMemoryInfo) * blockHead.hostMemoryNum,
        memInfo.begin() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead));
    uint64_t allHeadSize = sizeof(RecordGlobalHead) + sizeof(HostMemoryInfo) * blockHead.hostMemoryNum +
        sizeof(RecordBlockHead);
    OnlineCheck checker = OnlineCheck();
    checker.Init(memInfo.data(), memInfo.data() + allHeadSize, memInfo.data() + sizeof(RecordGlobalHead), 0);
    checker.ProcessParaBaseAddr();
    ASSERT_EQ(hostmems[0].addr, 0x100);
    ASSERT_EQ(hostmems[0].size, 100);
    ASSERT_EQ(hostmems[6].addr, 0x0);
    ASSERT_EQ(hostmems[6].size, 0);
    auto simdHead = reinterpret_cast<const RecordBlockHead*>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_FALSE(simdHead->extraWriteSuccess);
}

TEST(OnlineCheck, check_normal_simt_record_with_zero_extra_info_and_all_block_check_expect_success)
{
    uint64_t blockDim = 1;
    uint64_t cacheSize = 10 * MB_TO_BYTES;
    std::vector<uint8_t> memInfo(cacheSize * blockDim, 0);
    RecordGlobalHead head{};
    head.kernelInfo.kernelParamNum = 4;
    head.checkParms.defaultcheck = true;
    RecordBlockHead blockHead{};
    std::vector<HostMemoryInfo> hostmems;
    hostmems.push_back({0x100, 100});
    hostmems.push_back({0x200, 50});
    hostmems.push_back({0x0, 1000});
    hostmems.push_back({0x0, 0}); // 入队两个空地址
    hostmems.push_back({0x0, 0});
    hostmems.push_back({0x0, 2000});
    hostmems.push_back({0x0, 0});
    std::vector<uint64_t> paraBaseAddrs = {0x800, 0x400, 0x100, 0x400};
    blockHead.hostMemoryInfoPtr = reinterpret_cast<HostMemoryInfo *>(hostmems.data());
    blockHead.hostMemoryNum = hostmems.size();
    ParaBaseRegister paraBase{};
    paraBase.size = paraBaseAddrs.size() * sizeof(uintptr_t);
    paraBase.addr = reinterpret_cast<uint64_t>(paraBaseAddrs.data());
    int64_t regIdx = GetRegisterIdx();
    blockHead.paraBase = paraBase;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    std::copy_n(reinterpret_cast<uint8_t const*>(&blockHead), sizeof(RecordBlockHead),
        memInfo.begin() + sizeof(RecordGlobalHead));
    std::copy_n(reinterpret_cast<uint8_t const*>(hostmems.data()), sizeof(HostMemoryInfo) * blockHead.hostMemoryNum,
        memInfo.begin() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead));
    uint64_t allHeadSize = sizeof(RecordGlobalHead) + sizeof(HostMemoryInfo) * blockHead.hostMemoryNum +
        sizeof(RecordBlockHead);
    OnlineCheck checker = OnlineCheck();
    checker.Init(memInfo.data(), memInfo.data() + allHeadSize, memInfo.data() + sizeof(RecordGlobalHead), 1);
    checker.ProcessParaBaseAddr();
    ASSERT_EQ(hostmems[0].addr, 0x100);
    ASSERT_EQ(hostmems[0].size, 100);
    ASSERT_EQ(hostmems[1].addr, 0x200);
    ASSERT_EQ(hostmems[1].size, 50);
    ASSERT_EQ(hostmems[2].addr, 0x400);
    ASSERT_EQ(hostmems[2].size, 0x800 + 1000 - 0x400);
    for (size_t i = 3; i < hostmems.size(); ++i) {
        ASSERT_EQ(hostmems[i].addr, 0x0);
        ASSERT_EQ(hostmems[i].size, 0);
    }
    auto simdHead = reinterpret_cast<const RecordBlockHead*>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_TRUE(simdHead->extraWriteSuccess);
}

TEST(OnlineCheck, check_unaligned_simt_record_expect_one_error)
{
    uint64_t blockDim = 3;
    uint64_t cacheSize = 10 * MB_TO_BYTES;
    std::vector<uint8_t> memInfo(cacheSize * blockDim, 0);
    RecordGlobalHead head{};
    RecordBlockHead blockHead{};
    std::vector<HostMemoryInfo> hostmems;
    hostmems.push_back({0x100, 100});
    hostmems.push_back({0x300, 50});
    blockHead.hostMemoryInfoPtr = reinterpret_cast<HostMemoryInfo *>(hostmems.data());
    blockHead.hostMemoryNum = hostmems.size();
    head.offsetInfo.simtErrorInfo.size = 500;
    head.checkParms.defaultcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    std::copy_n(reinterpret_cast<uint8_t const*>(&blockHead), sizeof(RecordBlockHead),
    memInfo.begin() + sizeof(RecordGlobalHead));
    std::copy_n(reinterpret_cast<uint8_t const*>(hostmems.data()), sizeof(HostMemoryInfo) * blockHead.hostMemoryNum,
    memInfo.begin() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead));
    uint64_t allHeadSize = sizeof(RecordGlobalHead) + sizeof(HostMemoryInfo) * blockHead.hostMemoryNum +
                           sizeof(RecordBlockHead);
    OnlineCheck checker = OnlineCheck();
    checker.Init(memInfo.data(), memInfo.data() + allHeadSize, memInfo.data() + sizeof(RecordGlobalHead), 0);
    SimtLoadStoreRecord record = {
        .addr = 0x101,
        .size = 8,
        .location = {10, 10, 0x10, 1},
        .threadLoc = {10, 20, 5},
        .space = AddressSpace::GM,
        .detailedDataType = DetailedDataType::B64,
    };
    checker.Process<RecordType::SIMT_LDG>(record);

    auto recordTypePtr = reinterpret_cast<RecordType const*>(memInfo.data() + allHeadSize +
                                                             sizeof(SimtRecordBlockHead));
    ASSERT_EQ(*recordTypePtr, RecordType::ONLINE_ERROR);
    recordTypePtr++;
    auto errorRecord = reinterpret_cast<KernelErrorRecord const*>(recordTypePtr);
    ASSERT_EQ(errorRecord->errorNum, 1);
    ASSERT_EQ(errorRecord->recordSize, sizeof(SimtLoadStoreRecord));
    ASSERT_EQ(errorRecord->recordType, RecordType::SIMT_LDG);
    errorRecord++;
    auto simtRecord = reinterpret_cast<SimtLoadStoreRecord const*>(errorRecord);
    ASSERT_EQ(simtRecord->addr, 0x101);
    ASSERT_EQ(simtRecord->space, AddressSpace::GM);
    ASSERT_EQ(simtRecord->threadLoc.idX, 10);
    ASSERT_EQ(simtRecord->threadLoc.idY, 20);
    ASSERT_EQ(simtRecord->threadLoc.idZ, 5);
    ASSERT_EQ(simtRecord->size, 8);
    simtRecord++;
    auto errorDesc = reinterpret_cast<KernelErrorDesc const*>(simtRecord);
    ASSERT_EQ(errorDesc->errorType, KernelErrorType::MISALIGNED_ACCESS);
    ASSERT_EQ(errorDesc->payload.illegalDesc.illegalSize, 8);
    ASSERT_EQ(errorDesc->payload.illegalDesc.addr, 0x101);
    ASSERT_EQ(errorDesc->space, AddressSpace::GM);
    ASSERT_EQ(errorDesc->location.blockId, 1);
}
