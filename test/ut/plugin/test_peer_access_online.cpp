/* Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved. */
#include <algorithm>
#include <cstring>
#include <vector>
#include <gtest/gtest.h>
#include "address_sanitizer/address_sanitizer.h"
#include "ccec_defs.h"
#include "plugin/online_check.h"
#include "plugin/recorder.h"
#include "peer_access_manager.h"
#include "record_pre_process.h"
#include <sstream>

using namespace Sanitizer;
namespace {
constexpr uint32_t R = MSTX_MEM_PERMISSIONS_REGION_FLAGS_READ;
constexpr uint32_t W = MSTX_MEM_PERMISSIONS_REGION_FLAGS_WRITE;
constexpr uint32_t RW = R | W;

class PeerOnlineDevice : public testing::Test {
protected:
    std::vector<uint8_t> bytes =
        std::vector<uint8_t>(sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) + 64 * 1024, 0);
    std::vector<PeerMemoryInfo> peers;
    std::vector<HostMemoryInfo> locals;
    std::vector<KernelErrorDesc> errors;
    OnlineCheck checker;
    uint8_t *simt = nullptr;

    void Prepare() {
        std::fill(bytes.begin(), bytes.end(), 0);
        errors.clear();
        auto *global = reinterpret_cast<RecordGlobalHead *>(bytes.data());
        global->checkParms.defaultcheck = true;
        global->checkParms.memcheck = true;
        global->offsetInfo.simtErrorInfo.size = 8192;
        auto *block = reinterpret_cast<RecordBlockHead *>(global + 1);
        block->hostMemoryNum = locals.size();
        block->hostMemoryInfoPtr = locals.data();
        block->peerMemoryNum = peers.size();
        block->peerMemoryInfoPtr = peers.data();
        simt = reinterpret_cast<uint8_t *>(block + 1);
        checker.Init(bytes.data(), simt, reinterpret_cast<uint8_t *>(block), 0);
    }
    template <RecordType kind>
    void Access(uint64_t addr, uint64_t size, AddressSpace space = AddressSpace::GM,
        DetailedDataType data = DetailedDataType::B8) {
        Prepare();
        SimtLoadStoreRecord record{};
        record.addr = addr;
        record.size = size;
        record.space = space;
        record.detailedDataType = data;
        checker.Process<kind>(record);
        ReadErrors();
    }
    template <RecordType kind> void Atomic(uint32_t permission) {
        peers = {{0x1000, 64, permission, 1}};
        Prepare();
        SimtAtomRecord record{};
        record.addr = 0x1000;
        record.size = 4;
        record.space = AddressSpace::GM;
        record.detailedDataType = DetailedDataType::B32;
        checker.Process<kind>(record);
        ReadErrors();
    }
    void ReadErrors() {
        auto *head = reinterpret_cast<SimtRecordBlockHead *>(simt);
        const uint8_t *cursor = simt + sizeof(*head);
        const uint8_t *end = cursor + head->writeOffset;
        while (cursor < end) {
            ASSERT_LE(cursor + sizeof(RecordType) + sizeof(KernelErrorRecord), bytes.data() + bytes.size());
            ASSERT_EQ(*reinterpret_cast<const RecordType *>(cursor), RecordType::ONLINE_ERROR);
            const auto *record = reinterpret_cast<const KernelErrorRecord *>(cursor + sizeof(RecordType));
            const auto *descs = reinterpret_cast<const KernelErrorDesc *>(
                reinterpret_cast<const uint8_t *>(record + 1) + record->recordSize);
            ASSERT_LE(reinterpret_cast<const uint8_t *>(descs + record->errorNum), bytes.data() + bytes.size());
            errors.insert(errors.end(), descs, descs + record->errorNum);
            cursor = reinterpret_cast<const uint8_t *>(descs + record->errorNum);
        }
    }
    void ExpectIllegal(uint64_t addr, uint64_t size, KernelErrorType type) {
        ASSERT_EQ(errors.size(), 1);
        EXPECT_EQ(errors[0].errorType, type);
        EXPECT_EQ(errors[0].space, AddressSpace::GM);
        EXPECT_EQ(errors[0].payload.illegalDesc.addr, addr);
        EXPECT_EQ(errors[0].payload.illegalDesc.illegalSize, size);
    }
};

TEST_F(PeerOnlineDevice, AllLoadStoreEntrypointsRespectReadWriteAndNonePermissions) {
    for (uint32_t permission : {0U, R, W, RW}) {
        SCOPED_TRACE(permission);
        peers = {{0x1000, 64, permission, 1}};
        Access<RecordType::SIMT_LDG>(0x1000, 4);
        if (permission & R) {
            EXPECT_TRUE(errors.empty());
        } else {
            ExpectIllegal(0x1000, 4, KernelErrorType::ILLEGAL_ADDR_READ);
        }
        Access<RecordType::SIMT_LD>(0x1000, 4);
        if (permission & R) {
            EXPECT_TRUE(errors.empty());
        } else {
            ExpectIllegal(0x1000, 4, KernelErrorType::ILLEGAL_ADDR_READ);
        }
        Access<RecordType::SIMT_STG>(0x1000, 4);
        if (permission & W) {
            EXPECT_TRUE(errors.empty());
        } else {
            ExpectIllegal(0x1000, 4, KernelErrorType::ILLEGAL_ADDR_WRITE);
        }
        Access<RecordType::SIMT_ST>(0x1000, 4);
        if (permission & W) {
            EXPECT_TRUE(errors.empty());
        } else {
            ExpectIllegal(0x1000, 4, KernelErrorType::ILLEGAL_ADDR_WRITE);
        }
    }
}

TEST_F(PeerOnlineDevice, AtomAndRedRequireBothPermissions) {
    for (uint32_t permission : {0U, R, W, RW}) {
        Atomic<RecordType::SIMT_ATOM>(permission);
        if (permission == RW) {
            EXPECT_TRUE(errors.empty());
        } else {
            ASSERT_EQ(errors.size(), 2);
            EXPECT_EQ(errors[0].errorType, KernelErrorType::ILLEGAL_ADDR_READ);
            EXPECT_EQ(errors[1].errorType, KernelErrorType::ILLEGAL_ADDR_WRITE);
            for (const auto &error : errors) {
                EXPECT_EQ(error.payload.illegalDesc.addr, 0x1000);
                EXPECT_EQ(error.payload.illegalDesc.illegalSize, 4);
            }
        }
        Atomic<RecordType::SIMT_RED>(permission);
        EXPECT_EQ(errors.size(), permission == RW ? 0 : 2);
    }
}

TEST_F(PeerOnlineDevice, LocalAndPeerHalfRangesCannotCombineAndLocalPermissionCannotBeBypassed) {
    locals = {{0x1000, 16, RW}};
    peers = {{0x1010, 16, RW, 1}};
    Access<RecordType::SIMT_LDG>(0x1000, 32);
    ExpectIllegal(0x1000, 16, KernelErrorType::ILLEGAL_ADDR_READ);
    peers = {{0x1000, 32, RW, 1}};
    Access<RecordType::SIMT_LDG>(0x1000, 32);
    ExpectIllegal(0x1000, 16, KernelErrorType::ILLEGAL_ADDR_READ);
    locals = {{0x1000, 32, R}};
    peers = {{0x1000, 32, RW, 1}};
    // 需求要求保留本地明确权限拒绝；若当前在线补查放行，本用例应暴露缺口。
    Access<RecordType::SIMT_STG>(0x1000, 4);
    ExpectIllegal(0x1000, 4, KernelErrorType::ILLEGAL_ADDR_WRITE);
}

TEST_F(PeerOnlineDevice, EmptySnapshotRejectsPreviousAddressAndLocalBaselineStillWorks) {
    peers = {{0x1000, 32, RW, 1}};
    Access<RecordType::SIMT_LDG>(0x1000, 4);
    EXPECT_TRUE(errors.empty());
    peers.clear(); // Host 撤权、RESET、FREE 后的新缓冲均没有旧远端项。
    Access<RecordType::SIMT_LDG>(0x1000, 4);
    ExpectIllegal(0x1000, 4, KernelErrorType::ILLEGAL_ADDR_READ);
    locals = {{0x1000, 32, RW}};
    Access<RecordType::SIMT_STG>(0x1000, 4);
    EXPECT_TRUE(errors.empty());
}

} // namespace

TEST(PeerOnlineParameters, SortingLocalParametersDoesNotRewritePeerTable) {
    std::vector<uint8_t> bytes(sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) + 64 * 1024, 0);
    auto *global = reinterpret_cast<RecordGlobalHead *>(bytes.data());
    global->kernelInfo.kernelParamNum = 3;
    global->checkParms.checkBlockId = CHECK_ALL_BLOCK;
    global->checkParms.memcheck = true;
    global->offsetInfo.simtErrorInfo.size = 4096;
    // 本地参数使用相同权限，仅验证参数整理不会改写独立的远端表
    std::vector<uint64_t> parameters{0x2000, 0x1000, 0x8000};
    std::vector<HostMemoryInfo> locals{
        {0, 32, MSTX_MEM_PERMISSIONS_REGION_FLAGS_READ}, {0, 32, MSTX_MEM_PERMISSIONS_REGION_FLAGS_READ}, {0, 0, 0}};
    std::vector<PeerMemoryInfo> peers{{0x8000, 64, MSTX_MEM_PERMISSIONS_REGION_FLAGS_READ, 1}};
    auto *block = reinterpret_cast<RecordBlockHead *>(global + 1);
    block->hostMemoryNum = locals.size();
    block->hostMemoryInfoPtr = locals.data();
    block->peerMemoryNum = peers.size();
    block->peerMemoryInfoPtr = peers.data();
    block->paraBase.addr = reinterpret_cast<uint64_t>(parameters.data());
    block->paraBase.size = parameters.size() * sizeof(uint64_t);
    auto *simt = reinterpret_cast<uint8_t *>(block + 1);
    OnlineCheck checker;
    checker.Init(bytes.data(), simt, reinterpret_cast<uint8_t *>(block), 0);
    checker.ProcessParaBaseAddr();
    ASSERT_TRUE(block->extraWriteSuccess);
    ASSERT_EQ(locals[0].addr, 0x1000);
    ASSERT_EQ(locals[1].addr, 0x2000);
    EXPECT_EQ(peers[0].addr, 0x8000);
    EXPECT_EQ(peers[0].size, 64);
    EXPECT_EQ(peers[0].permission, MSTX_MEM_PERMISSIONS_REGION_FLAGS_READ);
    SimtLoadStoreRecord record{};
    record.addr = 0x8000;
    record.size = 4;
    record.space = AddressSpace::GM;
    record.detailedDataType = DetailedDataType::B32;
    checker.Process<RecordType::SIMT_STG>(record);
    EXPECT_GT(reinterpret_cast<SimtRecordBlockHead *>(simt)->writeOffset, 0);
}

// 测试场景：同设备两条相邻的P2P映射窗口
// 测试预期：窗口内可访问，跨两个窗口的访存按连续范围合并后同样可访问，窗口外仍报非法
TEST_F(PeerOnlineDevice, AdjacentMappingsCombineIntoSingleCoverage) {
    peers = {{0x1000, 4, RW, 1}, {0x1004, 4, RW, 1}};
    Access<RecordType::SIMT_LDG>(0x1000, 4);
    EXPECT_TRUE(errors.empty());
    Access<RecordType::SIMT_LDG>(0x1002, 4);
    EXPECT_TRUE(errors.empty());
    Access<RecordType::SIMT_LDG>(0x1006, 4);
    ExpectIllegal(0x1006, 4, KernelErrorType::ILLEGAL_ADDR_READ);
}

// 测试场景：同设备两条相邻窗口的权限不同
// 测试预期：合并只使用两条窗口都具备的权限；读可跨窗口合并，写仍被只读窗口拒绝
TEST_F(PeerOnlineDevice, CombinedMappingsStillRequireEachWindowPermission) {
    peers = {{0x1000, 4, R, 1}, {0x1004, 4, RW, 1}};
    Access<RecordType::SIMT_LDG>(0x1002, 4);
    EXPECT_TRUE(errors.empty());
    Access<RecordType::SIMT_STG>(0x1000, 4);
    ExpectIllegal(0x1000, 4, KernelErrorType::ILLEGAL_ADDR_WRITE);
    Access<RecordType::SIMT_STG>(0x1002, 4);
    ExpectIllegal(0x1002, 4, KernelErrorType::ILLEGAL_ADDR_WRITE);
}

// 测试场景：属于不同所有设备的相邻窗口
// 测试预期：合并只在同一目标设备内进行，跨设备相邻窗口不拼接
TEST_F(PeerOnlineDevice, MappingsDoNotCombineAcrossDevices) {
    peers = {{0x1000, 4, RW, 1}, {0x1004, 4, RW, 2}};
    Access<RecordType::SIMT_LDG>(0x1002, 4);
    ExpectIllegal(0x1002, 4, KernelErrorType::ILLEGAL_ADDR_READ);
}
