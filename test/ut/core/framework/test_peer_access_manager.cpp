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
#include <string>
#include <future>
#include <chrono>

#include "address_sanitizer/address_sanitizer.h"
#include "peer_access_manager.h"
#include "runtime_context.h"

using namespace Sanitizer;

namespace {
// 用指定设备号初始化真实 AddressSanitizer，供关系查询测试使用。
bool InitDevice(AddressSanitizer &sanitizer, int32_t deviceId) {
    DeviceInfoSummary info{};
    info.device = DeviceType::ASCEND_910B1;
    info.deviceId = deviceId;
    Config config{.defaultCheck = true, .memCheck = true};
    return sanitizer.SetDeviceInfo(info, config);
}

// 向检测器上报一段固定的 GM 分配。
void Allocate(AddressSanitizer &sanitizer) {
    SanitizerRecord record{};
    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord.type = MemOpType::MALLOC;
    record.payload.memoryRecord.infoSrc = MemInfoSrc::ACL;
    record.payload.memoryRecord.dstSpace = AddressSpace::GM;
    record.payload.memoryRecord.dstAddr = 0x1000;
    record.payload.memoryRecord.memSize = 16;
    sanitizer.CheckRecordBeforeProcess(record);
}

// 上报一次 GM 读取并判断是否产生非法读取告警。
bool HasIllegalRead(AddressSanitizer &sanitizer, uint64_t addr, uint64_t size) {
    std::string message;
    sanitizer.RegisterNotifyFunc([&message](LogLv const &, SanitizerBase::MSG_GEN &&gen) { message += gen().message; });
    SanitizerRecord record{};
    record.version = RecordVersion::MEMORY_RECORD;
    record.payload.memoryRecord.type = MemOpType::LOAD;
    record.payload.memoryRecord.infoSrc = MemInfoSrc::ACL;
    record.payload.memoryRecord.dstSpace = AddressSpace::GM;
    record.payload.memoryRecord.dstAddr = addr;
    record.payload.memoryRecord.memSize = size;
    sanitizer.CheckRecordBeforeProcess(record);
    return message.find("illegal read") != std::string::npos;
}
}

TEST(PeerAccessManager, EnableDisableAndReset) {
    auto &manager = PeerAccessManager::Instance();
    manager.Clear();
    RuntimeContext::Instance().pid_ = 101;
    AddressSanitizer source;
    AddressSanitizer target;
    ASSERT_TRUE(InitDevice(source, 1));
    ASSERT_TRUE(InitDevice(target, 2));
    Allocate(target);

    EXPECT_TRUE(HasIllegalRead(source, 0x1000, 16));
    EXPECT_TRUE(manager.Apply(101, {1, 2, PeerAccessOp::ENABLE}));
    EXPECT_FALSE(HasIllegalRead(source, 0x1000, 16));
    EXPECT_TRUE(HasIllegalRead(source, 0x1008, 16));
    EXPECT_TRUE(HasIllegalRead(source, UINT64_MAX, 16));
    EXPECT_TRUE(manager.Apply(101, {1, 2, PeerAccessOp::DISABLE}));
    EXPECT_TRUE(HasIllegalRead(source, 0x1000, 16));
    EXPECT_TRUE(manager.Apply(101, {1, 2, PeerAccessOp::ENABLE}));
    EXPECT_TRUE(manager.Apply(101, {2, -1, PeerAccessOp::RESET}));
    EXPECT_TRUE(HasIllegalRead(source, 0x1000, 16));
    RuntimeContext::Instance().pid_ = -1;
}

TEST(PeerAccessManager, ProcessIsolationAndDeviceLifetime) {
    auto &manager = PeerAccessManager::Instance();
    manager.Clear();
    RuntimeContext::Instance().pid_ = 101;
    AddressSanitizer source;
    AddressSanitizer target;
    ASSERT_TRUE(InitDevice(source, 1));
    ASSERT_TRUE(InitDevice(target, 2));
    Allocate(target);
    ASSERT_TRUE(manager.Apply(101, {1, 2, PeerAccessOp::ENABLE}));
    EXPECT_FALSE(HasIllegalRead(source, 0x1000, 16));

    RuntimeContext::Instance().pid_ = 102;
    AddressSanitizer otherProcessSource;
    ASSERT_TRUE(InitDevice(otherProcessSource, 1));
    EXPECT_TRUE(HasIllegalRead(otherProcessSource, 0x1000, 16));
    manager.UnregisterDevice(&target);
    EXPECT_TRUE(HasIllegalRead(source, 0x1000, 16));
    EXPECT_FALSE(manager.Apply(101, {1, 1, PeerAccessOp::ENABLE}));
    EXPECT_FALSE(manager.Apply(101, {1, 2, static_cast<PeerAccessOp>(100)}));
    RuntimeContext::Instance().pid_ = -1;
}

TEST(PeerAccessManager, LocalKernelChecksDoNotHoldGlobalPeerLock) {
    auto &manager = PeerAccessManager::Instance();
    manager.Clear();
    RuntimeContext::Instance().pid_ = 101;
    AddressSanitizer sanitizer;
    ASSERT_TRUE(InitDevice(sanitizer, 0));
    Allocate(sanitizer);
    SanEvent event{};
    event.type = EventType::MEM_EVENT;
    auto &memory = event.eventInfo.memInfo;
    memory.memType = MemType::GM;
    memory.opType = AccessType::READ;
    memory.addr = 0x1000;
    memory.blockNum = memory.repeatTimes = 1;
    memory.blockSize = 4;
    std::vector<SanEvent> events{event};
    SanitizerRecord record{};
    record.version = RecordVersion::KERNEL_RECORD;
    std::future<void> checked;
    {
        // 调用线程占用共享关系锁；另一个设备的本地 kernel 检查仍应完成
        std::lock_guard<std::recursive_mutex> lock(manager.Mutex());
        checked = std::async(std::launch::async, [&] { sanitizer.Do(record, events); });
        EXPECT_EQ(checked.wait_for(std::chrono::seconds(3)), std::future_status::ready);
    }
    checked.get();
}

// 真实检测器夹具：显式设置两个设备但不启用设备级Peer授权
class P2PMappingManager : public testing::Test {
protected:
    AddressSanitizer access;
    AddressSanitizer owner;
    void SetUp() override {
        PeerAccessManager::Instance().Clear();
        RuntimeContext::Instance().pid_ = 710;
        ASSERT_TRUE(InitDevice(access, 1));
        ASSERT_TRUE(InitDevice(owner, 2));
        Allocate(owner);
    }
    void TearDown() override {
        PeerAccessManager::Instance().Clear();
        RuntimeContext::Instance().pid_ = -1;
    }
    bool Map(uint64_t addr = 0x1004, uint64_t size = 4) {
        return PeerAccessManager::Instance().ApplyMapping(710, {1, 2, addr, size, 0x1000});
    }
    bool HasIllegalWrite(uint64_t addr, uint64_t size) {
        std::string message;
        access.RegisterNotifyFunc([&message](LogLv const &, SanitizerBase::MSG_GEN &&gen) { message += gen().message; });
        SanitizerRecord record{};
        record.version = RecordVersion::MEMORY_RECORD;
        auto &memory = record.payload.memoryRecord;
        memory.type = MemOpType::STORE;
        memory.infoSrc = MemInfoSrc::ACL;
        memory.dstSpace = AddressSpace::GM;
        memory.dstAddr = addr;
        memory.memSize = size;
        access.CheckRecordBeforeProcess(record);
        return message.find("illegal write") != std::string::npos;
    }
    void FreeOwner() {
        SanitizerRecord record{};
        record.version = RecordVersion::MEMORY_RECORD;
        record.payload.memoryRecord.type = MemOpType::FREE;
        record.payload.memoryRecord.infoSrc = MemInfoSrc::ACL;
        record.payload.memoryRecord.dstSpace = AddressSpace::GM;
        record.payload.memoryRecord.dstAddr = 0x1000;
        owner.CheckRecordBeforeProcess(record);
    }
};

// 测试场景：无设备级授权，仅成功映射一个子窗口
// 测试预期：窗口内合法，原分配其余区域仍报非法读
TEST_F(P2PMappingManager, ExactWindowWithoutDeviceWideAuthorization) {
    ASSERT_TRUE(Map());
    EXPECT_FALSE(HasIllegalRead(access, 0x1004, 4));
    EXPECT_TRUE(HasIllegalRead(access, 0x1000, 4));
    EXPECT_TRUE(HasIllegalRead(access, 0x1007, 2));
}

// 测试场景：同一所有方设备的两条相邻映射各自覆盖一半访存
// 测试预期：按连续范围合并后可放过一次跨窗口访问，窗口外仍报非法
TEST_F(P2PMappingManager, AdjacentMappingsCombineIntoSingleCoverage) {
    ASSERT_TRUE(Map(0x1000, 4));
    ASSERT_TRUE(Map(0x1004, 4));
    EXPECT_FALSE(HasIllegalRead(access, 0x1002, 4));
    EXPECT_TRUE(HasIllegalRead(access, 0x1008, 4));
}

// 测试场景：相邻窗口分属不同所有方设备
// 测试预期：合并只在同一所有方设备内进行，跨所有方的相邻窗口不拼接
TEST_F(P2PMappingManager, MappingsDoNotCombineAcrossOwners) {
    ASSERT_TRUE(Map(0x1000, 4));
    AddressSanitizer otherOwner;
    ASSERT_TRUE(InitDevice(otherOwner, 3));
    Allocate(otherOwner);
    ASSERT_TRUE(PeerAccessManager::Instance().ApplyMapping(710, {1, 3, 0x1004, 4, 0x1000}));
    EXPECT_TRUE(HasIllegalRead(access, 0x1002, 4));
}

// 测试场景：映射长度超出所有方真实分配
// 测试预期：拒绝记录，不能借映射创建不存在的原内存
TEST_F(P2PMappingManager, RejectsRangeOutsideOwnerAllocation) {
    EXPECT_FALSE(Map(0x1008, 16));
    EXPECT_TRUE(HasIllegalRead(access, 0x1008, 4));
}

// 测试场景：释放所有方后同址重新分配
// 测试预期：旧映射不复活，重新Map后才能访问
TEST_F(P2PMappingManager, FreeAndReuseRequireNewMapping) {
    ASSERT_TRUE(Map());
    FreeOwner();
    Allocate(owner);
    EXPECT_TRUE(HasIllegalRead(access, 0x1004, 4));
    ASSERT_TRUE(Map());
    EXPECT_FALSE(HasIllegalRead(access, 0x1004, 4));
}

// 测试场景：同址新Malloc事件替换原分配生命周期
// 测试预期：即使没有残留FREE事件也不继承旧映射
TEST_F(P2PMappingManager, NewAllocationInvalidatesOldMapping) {
    ASSERT_TRUE(Map());
    Allocate(owner);
    EXPECT_TRUE(HasIllegalRead(access, 0x1004, 4));
}

// 测试场景：所有方成功reset
// 测试预期：撤销该设备全部映射，被访问设备仍存在也不能继续访问
TEST_F(P2PMappingManager, OwnerResetInvalidatesMapping) {
    ASSERT_TRUE(Map());
    ASSERT_TRUE(PeerAccessManager::Instance().Apply(710, {2, -1, PeerAccessOp::RESET}));
    EXPECT_TRUE(HasIllegalRead(access, 0x1004, 4));
}

// 测试场景：使用方成功reset
// 测试预期：清除该设备获得的精确映射
TEST_F(P2PMappingManager, AccessResetInvalidatesMapping) {
    ASSERT_TRUE(Map());
    ASSERT_TRUE(PeerAccessManager::Instance().Apply(710, {1, -1, PeerAccessOp::RESET}));
    EXPECT_TRUE(HasIllegalRead(access, 0x1004, 4));
}

// 测试场景：未曾采集ENABLE但有精确映射，此时采集到撤权
// 测试预期：撤权仍撤销映射，不依赖设备级关系已经存在
TEST_F(P2PMappingManager, DisableInvalidatesMappingWithoutEnableRecord) {
    ASSERT_TRUE(Map());
    ASSERT_TRUE(PeerAccessManager::Instance().Apply(710, {1, 2, PeerAccessOp::DISABLE}));
    EXPECT_TRUE(HasIllegalRead(access, 0x1004, 4));
}

// 测试场景：另一个PID拥有相同使用设备号
// 测试预期：不能使用原PID映射授权
TEST_F(P2PMappingManager, MappingIsIsolatedByProcess) {
    ASSERT_TRUE(Map());
    RuntimeContext::Instance().pid_ = 711;
    AddressSanitizer other;
    ASSERT_TRUE(InitDevice(other, 1));
    EXPECT_TRUE(HasIllegalRead(other, 0x1004, 4));
}

// 测试场景：重复成功映射与目标检测器注销
// 测试预期：重复幂等，注销后旧映射立即不可用
TEST_F(P2PMappingManager, DuplicateAndOwnerLifetime) {
    ASSERT_TRUE(Map());
    ASSERT_TRUE(Map());
    EXPECT_FALSE(HasIllegalRead(access, 0x1004, 4));
    PeerAccessManager::Instance().UnregisterDevice(&owner);
    EXPECT_TRUE(HasIllegalRead(access, 0x1004, 4));
}

// 测试场景：所有方和使用方重复上报相同设备摘要
// 测试预期：保留仍有效的映射与所有方 GM 分配，窗口内访问继续合法
TEST_F(P2PMappingManager, RepeatedDeviceSummaryPreservesMapping) {
    ASSERT_TRUE(Map());
    ASSERT_TRUE(InitDevice(owner, 2));
    ASSERT_TRUE(InitDevice(access, 1));
    EXPECT_FALSE(HasIllegalRead(access, 0x1004, 4));
    EXPECT_TRUE(HasIllegalRead(access, 0x1003, 2));
    FreeOwner();
    Allocate(owner);
    EXPECT_TRUE(HasIllegalRead(access, 0x1004, 4));
}

// 测试场景：使用方切换设备后重新登记原设备
// 测试预期：原设备映射已撤销，重新登记不恢复旧授权
TEST_F(P2PMappingManager, DeviceSwitchRevokesMapping) {
    ASSERT_TRUE(Map());
    ASSERT_TRUE(InitDevice(access, 3));
    ASSERT_TRUE(InitDevice(access, 1));
    EXPECT_TRUE(HasIllegalRead(access, 0x1004, 4));
}

// 测试场景：已有映射的设备收到不支持的设备类型摘要
// 测试预期：初始化失败后撤销原登记及其映射
TEST_F(P2PMappingManager, FailedDeviceSummaryRevokesMapping) {
    ASSERT_TRUE(Map());
    DeviceInfoSummary info{};
    info.device = static_cast<DeviceType>(UINT32_MAX);
    info.deviceId = 2;
    EXPECT_FALSE(owner.SetDeviceInfo(info, {}));
    ASSERT_TRUE(InitDevice(owner, 2));
    EXPECT_TRUE(HasIllegalRead(access, 0x1004, 4));
}

// 测试场景：零长度、溢出和错误目标设备的报文
// 测试预期：拒绝无效报文，不留下可访问范围
TEST_F(P2PMappingManager, InvalidRecordsDoNotGrantAccess) {
    auto &manager = PeerAccessManager::Instance();
    EXPECT_FALSE(manager.ApplyMapping(710, {1, 1, 0x1000, 4, 0x1000}));
    EXPECT_FALSE(manager.ApplyMapping(710, {1, 2, 0x1000, 0, 0x1000}));
    EXPECT_FALSE(manager.ApplyMapping(710, {1, 2, UINT64_MAX, 4, 0x1000}));
    EXPECT_FALSE(manager.ApplyMapping(710, {-1, 2, 0x1000, 4, 0x1000}));
    EXPECT_TRUE(HasIllegalRead(access, 0x1004, 4));
}

// 测试场景：映射所有方被设为不可读写
// 测试预期：映射不覆盖原分配权限，合法地址仍应报权限错误
TEST_F(P2PMappingManager, OwnerPermissionRemainsEffective) {
    ASSERT_TRUE(Map());
    SanitizerRecord permission{};
    permission.version = RecordVersion::REGION_PERMISSION;
    permission.payload.permissionDesc = {0x1000, 16, 2, 0};
    owner.Do(permission, {});
    EXPECT_TRUE(HasIllegalRead(access, 0x1004, 4));
}

// 测试场景：子窗口内写入及头尾越界写入
// 测试预期：合法写通过，不能用所有方更大Malloc补齐越界
TEST_F(P2PMappingManager, WritesRespectBothWindowBoundaries) {
    ASSERT_TRUE(Map());
    EXPECT_FALSE(HasIllegalWrite(0x1004, 4));
    EXPECT_TRUE(HasIllegalWrite(0x1003, 2));
    EXPECT_TRUE(HasIllegalWrite(0x1007, 2));
}

// 测试场景：映射所有方只读，使用方尝试读和写
// 测试预期：读仍合法，写不因共享属性放行
TEST_F(P2PMappingManager, ReadOnlyOwnerAllowsReadButRejectsWrite) {
    ASSERT_TRUE(Map());
    SanitizerRecord permission{};
    permission.version = RecordVersion::REGION_PERMISSION;
    permission.payload.permissionDesc = {0x1000, 16, 2, 1};
    owner.Do(permission, {});
    EXPECT_FALSE(HasIllegalRead(access, 0x1004, 4));
    EXPECT_TRUE(HasIllegalWrite(0x1004, 4));
}

// 测试场景：Map 前或 Map 后暂时关闭所有方权限，再逐步恢复读写权限
// 测试预期：映射登记成功且保持有效，访问始终遵循所有方当前权限
TEST_F(P2PMappingManager, MappingRegistrationIsIndependentOfCurrentPermission) {
    auto setPermission = [&](uint32_t flags) {
        SanitizerRecord permission{};
        permission.version = RecordVersion::REGION_PERMISSION;
        permission.payload.permissionDesc = {0x1000, 16, 2, flags};
        owner.Do(permission, {});
    };
    for (bool mapBeforePermission : {false, true}) {
        FreeOwner();
        Allocate(owner);
        if (mapBeforePermission) {
            ASSERT_TRUE(Map());
        }
        setPermission(MSTX_MEM_PERMISSIONS_REGION_FLAGS_NONE);
        if (!mapBeforePermission) {
            ASSERT_TRUE(Map());
        }
        EXPECT_TRUE(HasIllegalRead(access, 0x1004, 4));
        EXPECT_TRUE(HasIllegalWrite(0x1004, 4));
        setPermission(MSTX_MEM_PERMISSIONS_REGION_FLAGS_READ);
        EXPECT_FALSE(HasIllegalRead(access, 0x1004, 4));
        EXPECT_TRUE(HasIllegalWrite(0x1004, 4));
        setPermission(Bounds::DEFAULT_PERMISSION);
        EXPECT_FALSE(HasIllegalWrite(0x1004, 4));
    }
}

// 测试场景：逆序登记 256 个窗口并留出中间空洞，随后补齐并增加重叠窗口
// 测试预期：空洞阻止整段访问，补齐后连续覆盖，尾部越界与释放后访问被拒绝
TEST_F(P2PMappingManager, ManyUnorderedWindowsCoverOnlyContinuousLiveRange) {
    constexpr uint64_t base = 0x2000;
    constexpr uint64_t windowSize = 4;
    constexpr int windowCount = 256;
    SanitizerRecord record{};
    record.version = RecordVersion::MEMORY_RECORD;
    auto &memory = record.payload.memoryRecord;
    memory.type = MemOpType::MALLOC;
    memory.infoSrc = MemInfoSrc::ACL;
    memory.dstSpace = AddressSpace::GM;
    memory.dstAddr = base;
    memory.memSize = windowSize * windowCount;
    owner.CheckRecordBeforeProcess(record);
    auto &manager = PeerAccessManager::Instance();
    for (int i = windowCount - 1; i >= 0; --i) {
        if (i != windowCount / 2) {
            ASSERT_TRUE(manager.ApplyMapping(710, {1, 2, base + i * windowSize, windowSize, base}));
        }
    }
    EXPECT_TRUE(HasIllegalRead(access, base, memory.memSize));
    ASSERT_TRUE(manager.ApplyMapping(710, {1, 2, base + windowCount / 2 * windowSize, windowSize, base}));
    ASSERT_TRUE(manager.ApplyMapping(710, {1, 2, base + 1, 8, base}));
    EXPECT_FALSE(HasIllegalRead(access, base, memory.memSize));
    EXPECT_TRUE(HasIllegalRead(access, base + memory.memSize - 2, 4));
    memory.type = MemOpType::FREE;
    owner.CheckRecordBeforeProcess(record);
    EXPECT_TRUE(HasIllegalRead(access, base, memory.memSize));
}

// 测试场景：同一PID中第三个设备访问仅给device1建立的窗口
// 测试预期：设备隔离，device3仍报非法读
TEST_F(P2PMappingManager, OtherDeviceCannotBorrowMapping) {
    ASSERT_TRUE(Map());
    AddressSanitizer third;
    ASSERT_TRUE(InitDevice(third, 3));
    EXPECT_TRUE(HasIllegalRead(third, 0x1004, 4));
}
