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
