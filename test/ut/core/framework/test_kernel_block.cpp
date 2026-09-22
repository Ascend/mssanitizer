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
#include <vector>
#include <cstring>

#define private public
#include "core/framework/kernel_block.h"
#include "core/framework/record_defs.h"
#undef private

using namespace Sanitizer;

class TestKernelBlock : public testing::Test {
protected:
    void SetUp() override
    {
        KernelBlock::ResetAll();
    }

    void TearDown() override
    {
        KernelBlock::ResetAll();
    }

    // 创建有效的内存缓冲区，包含有效的 RecordGlobalHead 和 RecordBlockHead
    std::vector<uint8_t> CreateValidMemBuffer(
        uint64_t &outMemSize, uint32_t blockIdx = 0, uint32_t entryRecordCount = 0) {
        // 计算最小所需大小：全局头 + 块头 + 一些记录空间
        uint64_t simdRecordsSize = 1024; // 1KB for simd records
        // GetAllThreadSize 计算的是 (simtErrorInfo.size + sizeof(SimtRecordBlockHead)) * SIMT_THREAD_MAX_SIZE
        uint64_t simtErrorInfoSize = sizeof(SimtRecordBlockHead);
        uint64_t simtSize = (simtErrorInfoSize + sizeof(SimtRecordBlockHead)) * SIMT_THREAD_MAX_SIZE;
        uint64_t shadowMemHeadSize = sizeof(ShadowMemoryRecordHead);
        uint64_t shadowMemRecordsSize = 10 * sizeof(ShadowMemoryRecord); // 10 records
        uint64_t simtEntryHeadSize = sizeof(SimtEntryBlockHead);
        // SimtEntryRecord 记录位于 SimtEntryBlockHead 之后，ParseSimtEntryRecord 用例需预留记录空间
        simtEntryHeadSize += static_cast<uint64_t>(entryRecordCount) * sizeof(OnlineShadowMemory::SimtEntryRecord);

        uint64_t totalSize = sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) +
                             simdRecordsSize + simtSize + shadowMemHeadSize +
                             shadowMemRecordsSize + simtEntryHeadSize;

        std::vector<uint8_t> buffer(totalSize, 0);

        // 设置 RecordGlobalHead
        auto *globalHead = reinterpret_cast<RecordGlobalHead *>(buffer.data());
        globalHead->securityVal = RECORD_HEAD_SECURITY_VALUE;
        globalHead->kernelInfo.totalBlockDim = 1;
        globalHead->offsetInfo.simtErrorInfo.offset = simdRecordsSize;
        globalHead->offsetInfo.simtErrorInfo.size = simtErrorInfoSize;
        globalHead->supportSimt = true;

        // 设置 RecordBlockHead
        auto *blockHead = reinterpret_cast<RecordBlockHead *>(buffer.data() + sizeof(RecordGlobalHead));
        blockHead->recordCount = 10;
        blockHead->recordWriteCount = 5;
        blockHead->offset = simdRecordsSize;
        blockHead->writeOffset = simdRecordsSize; // writeOffset 通常等于 offset，表示 SIMT 记录区域的起始位置
        blockHead->blockInfo.blockId = blockIdx;
        blockHead->blockInfo.blockType = BlockType::AIVEC;
        blockHead->blockInfo.vecSubBlockDim = 2;

        // 设置 SimtRecordBlockHead（在 writeOffset 处）
        auto *simtHead = reinterpret_cast<SimtRecordBlockHead *>(
            buffer.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) + blockHead->writeOffset);
        simtHead->recordCount = 0;
        simtHead->recordWriteCount = 0;
        simtHead->offset = 0;
        simtHead->writeOffset = 0;

        // 设置 ShadowMemoryRecordHead
        uint64_t shadowMemOffset = sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) +
                                   blockHead->writeOffset + simtSize;
        auto *shadowHead = reinterpret_cast<ShadowMemoryRecordHead *>(buffer.data() + shadowMemOffset);
        shadowHead->type = static_cast<uint32_t>(RecordType::SIMT_ENTRY);
        shadowHead->recordCount = 10;

        // 设置 SimtEntryBlockHead
        uint64_t simtEntryOffset = shadowMemOffset + sizeof(ShadowMemoryRecordHead) + shadowMemRecordsSize;
        auto *entryHead = reinterpret_cast<SimtEntryBlockHead *>(buffer.data() + simtEntryOffset);
        entryHead->recordCount = 0;
        entryHead->recordWriteCount = 0;
        entryHead->exceedSize = 0;

        outMemSize = totalSize;
        return buffer;
    }
};

// 测试 CreateKernelBlock 正常场景
TEST_F(TestKernelBlock, CreateKernelBlock_NormalCase)
{
    uint64_t memSize = 0;
    auto buffer = CreateValidMemBuffer(memSize, 0);

    auto kernelBlock = KernelBlock::CreateKernelBlock(buffer.data(), memSize, 0);

    ASSERT_NE(kernelBlock, nullptr);
    EXPECT_EQ(kernelBlock->blockIdx_, 0);
    EXPECT_EQ(kernelBlock->recordGlobalHead_.securityVal, RECORD_HEAD_SECURITY_VALUE);
    EXPECT_EQ(kernelBlock->simdRecordHead_.blockInfo.blockType, BlockType::AIVEC);
    EXPECT_EQ(KernelBlock::totalBlockDim_, 1);
    EXPECT_EQ(KernelBlock::vecSubBlockDim_, 2);
}

// 测试 CreateKernelBlock 非0 blockIdx
TEST_F(TestKernelBlock, CreateKernelBlock_NonZeroBlockIdx)
{
    uint64_t memSize = 0;
    auto buffer = CreateValidMemBuffer(memSize, 1);

    // 先创建 block 0 来设置 totalBlockDim
    uint64_t memSize0 = 0;
    auto buffer0 = CreateValidMemBuffer(memSize0, 0);
    auto kernelBlock0 = KernelBlock::CreateKernelBlock(buffer0.data(), memSize0, 0);
    ASSERT_NE(kernelBlock0, nullptr);

    auto kernelBlock1 = KernelBlock::CreateKernelBlock(buffer.data(), memSize, 1);

    ASSERT_NE(kernelBlock1, nullptr);
    EXPECT_EQ(kernelBlock1->blockIdx_, 1);
}

// 测试 CreateKernelBlock 传入 nullptr
TEST_F(TestKernelBlock, CreateKernelBlock_NullMemInfo)
{
    auto kernelBlock = KernelBlock::CreateKernelBlock(nullptr, 1024, 0);
    EXPECT_EQ(kernelBlock, nullptr);
}

// 测试 CreateKernelBlock memSize 过小
TEST_F(TestKernelBlock, CreateKernelBlock_SmallMemSize)
{
    std::vector<uint8_t> buffer(10, 0); // 远小于最小要求
    auto kernelBlock = KernelBlock::CreateKernelBlock(buffer.data(), buffer.size(), 0);
    EXPECT_EQ(kernelBlock, nullptr);
}

// 测试 CreateKernelBlock 安全校验值错误（blockIdx 0）
TEST_F(TestKernelBlock, CreateKernelBlock_InvalidSecurityValue)
{
    uint64_t memSize = 0;
    auto buffer = CreateValidMemBuffer(memSize, 0);

    // 修改安全校验值
    auto *globalHead = reinterpret_cast<RecordGlobalHead *>(buffer.data());
    globalHead->securityVal = 0xDEADBEEF;

    auto kernelBlock = KernelBlock::CreateKernelBlock(buffer.data(), memSize, 0);
    EXPECT_EQ(kernelBlock, nullptr);
}

// 测试 CreateKernelBlock writeOffset 超过 memSize
TEST_F(TestKernelBlock, CreateKernelBlock_WriteOffsetExceedsMemSize)
{
    uint64_t memSize = 0;
    auto buffer = CreateValidMemBuffer(memSize, 0);

    // 修改 writeOffset 使其超过 memSize
    auto *blockHead = reinterpret_cast<RecordBlockHead *>(buffer.data() + sizeof(RecordGlobalHead));
    blockHead->writeOffset = memSize - sizeof(RecordGlobalHead) - sizeof(RecordBlockHead) + 100;

    auto kernelBlock = KernelBlock::CreateKernelBlock(buffer.data(), memSize, 0);
    EXPECT_EQ(kernelBlock, nullptr);
}

// 测试 CreateKernelBlock shadowMemoryHead 在 memSize 范围内
TEST_F(TestKernelBlock, CreateKernelBlock_ShadowMemoryInRange)
{
    uint64_t memSize = 0;
    auto buffer = CreateValidMemBuffer(memSize, 0);

    auto kernelBlock = KernelBlock::CreateKernelBlock(buffer.data(), memSize, 0);

    ASSERT_NE(kernelBlock, nullptr);
}

// 测试 CreateKernelBlock shadowMemoryHead 超出 memSize
TEST_F(TestKernelBlock, CreateKernelBlock_ShadowMemoryOutOfRange)
{
    uint64_t memSize = 0;
    auto buffer = CreateValidMemBuffer(memSize, 0);

    // 修改 offsetInfo 使 shadow memory 超出范围
    auto *globalHead = reinterpret_cast<RecordGlobalHead *>(buffer.data());
    globalHead->offsetInfo.simtErrorInfo.size = memSize; // 设置一个很大的值

    auto kernelBlock = KernelBlock::CreateKernelBlock(buffer.data(), memSize, 0);

    ASSERT_NE(kernelBlock, nullptr);
}

// 测试 CreateKernelBlock simtEntry 在 memSize 范围内
TEST_F(TestKernelBlock, CreateKernelBlock_SimtEntryInRange)
{
    uint64_t memSize = 0;
    auto buffer = CreateValidMemBuffer(memSize, 0);

    auto kernelBlock = KernelBlock::CreateKernelBlock(buffer.data(), memSize, 0);

    ASSERT_NE(kernelBlock, nullptr);
    EXPECT_NE(kernelBlock->simtEntryHead_, nullptr);
}

// 测试 CreateKernelBlock CUBE 类型 block
TEST_F(TestKernelBlock, CreateKernelBlock_CubeBlockType)
{
    uint64_t memSize = 0;
    auto buffer = CreateValidMemBuffer(memSize, 0);

    // 修改 blockType 为 CUBE
    auto *blockHead = reinterpret_cast<RecordBlockHead *>(buffer.data() + sizeof(RecordGlobalHead));
    blockHead->blockInfo.blockType = BlockType::AICUBE;

    auto kernelBlock = KernelBlock::CreateKernelBlock(buffer.data(), memSize, 0);

    ASSERT_NE(kernelBlock, nullptr);
    EXPECT_EQ(kernelBlock->simdRecordHead_.blockInfo.blockType, BlockType::AICUBE);
    // vecSubBlockDim_ 不应该被设置（因为是 CUBE 类型）
    EXPECT_EQ(KernelBlock::vecSubBlockDim_, 0);
}

// 测试 GetTotalBlockDim
TEST_F(TestKernelBlock, GetTotalBlockDim)
{
    uint64_t memSize = 0;
    auto buffer = CreateValidMemBuffer(memSize, 0);

    auto kernelBlock = KernelBlock::CreateKernelBlock(buffer.data(), memSize, 0);

    ASSERT_NE(kernelBlock, nullptr);
    EXPECT_EQ(kernelBlock->GetTotalBlockDim(), 1);
}

// 测试 GetRecordBlockHead
TEST_F(TestKernelBlock, GetRecordBlockHead)
{
    uint64_t memSize = 0;
    auto buffer = CreateValidMemBuffer(memSize, 0);

    auto kernelBlock = KernelBlock::CreateKernelBlock(buffer.data(), memSize, 0);

    ASSERT_NE(kernelBlock, nullptr);
    auto blockHead = kernelBlock->GetRecordBlockHead();
    EXPECT_EQ(blockHead.blockInfo.blockType, BlockType::AIVEC);
    EXPECT_EQ(blockHead.recordCount, 10);
}

// 测试 ParseSimtEntryRecord：设备写入顺序不保证"UB 在前、GM 在后"（实测可交错 9 段），
// 按位置切分会把 GM/UB 记录混入同一动态事件，导致事件级 memType 取首条记录 space 而失真。
// stable_partition 按记录真实 space 稳定分区后，切分出的两个动态事件应各自单空间。
TEST_F(TestKernelBlock, ParseSimtEntryRecord_InterleavedSpace_SingleSpaceEvents) {
    constexpr uint32_t kEntryCount = 4;
    uint64_t memSize = 0;
    // 4 条 SimtEntryRecord，GM 在前、UB/GM 交错：旧位置切分会把前 2 条 (GM,UB) 混为"UB事件"、
    // 后 2 条 (GM,UB) 混为"GM事件"，事件级 memType 均失真
    auto buffer = CreateValidMemBuffer(memSize, 0, kEntryCount);
    auto kernelBlock = KernelBlock::CreateKernelBlock(buffer.data(), memSize, 0);
    ASSERT_NE(kernelBlock, nullptr);
    ASSERT_NE(kernelBlock->simtEntryHead_, nullptr);

    // simtEntryHead_ 为 const 指针，但其指向的缓冲区本身可写；CreateKernelBlock 按
    // "simt 错误区之后"定位的真实 simtEntry 区域，才是 ParseSimtEntryRecord 读取的位置
    auto *entryHead = const_cast<SimtEntryBlockHead *>(kernelBlock->simtEntryHead_);
    entryHead->recordCount = kEntryCount;
    entryHead->recordWriteCount = kEntryCount;
    entryHead->exceedSize = 0;
    entryHead->mainScalarPc = 0;
    entryHead->threadXDim = 1024; // DecomposeThreadId 依赖线程维度配置
    entryHead->threadYDim = 1;
    entryHead->threadZDim = 1;

    using OnlineShadowMemory::MEMORY_TYPE_START_BIT;
    using OnlineShadowMemory::SimtEntryRecord;
    auto *entryRecords = reinterpret_cast<SimtEntryRecord *>(entryHead + 1);
    const uint64_t ubStatus = 1ULL << MEMORY_TYPE_START_BIT; // [30:30]=1 → UB
    const uint64_t gmStatus = 0; // [30:30]=0 → GM（协议默认）
    entryRecords[0] = SimtEntryRecord{0x1000, gmStatus, 4}; // GM 记录
    entryRecords[1] = SimtEntryRecord{0x0, ubStatus, 4}; // UB 记录
    entryRecords[2] = SimtEntryRecord{0x2000, gmStatus, 4}; // GM 记录
    entryRecords[3] = SimtEntryRecord{0x4, ubStatus, 4}; // UB 记录

    std::vector<KernelRecord> kernelRecords;
    EXPECT_TRUE(kernelBlock->ParseSimtEntryRecord(kernelRecords));

    // stable_partition 后产生两个单空间事件：UB 事件在前、GM 事件在后
    ASSERT_EQ(kernelRecords.size(), 2U);

    // UB 事件：两条记录均为 UB，首条记录 space=UB（事件级 memType 正确）
    ASSERT_EQ(kernelRecords[0].recordType, RecordType::DYNAMIC_OP);
    ASSERT_EQ(kernelRecords[0].payload.dynamicRecord.dynamicType, RecordType::SIMT_ENTRY);
    ASSERT_EQ(kernelRecords[0].payload.dynamicRecord.count, 2U);
    auto *ubRecords = reinterpret_cast<ShadowMemoryRecord *>(kernelRecords[0].payload.dynamicRecord.buffer);
    ASSERT_NE(ubRecords, nullptr);
    EXPECT_EQ(ubRecords[0].space, AddressSpace::UB);
    EXPECT_EQ(ubRecords[0].addr, 0x0U);
    EXPECT_EQ(ubRecords[1].space, AddressSpace::UB);
    EXPECT_EQ(ubRecords[1].addr, 0x4U);

    // GM 事件：两条记录均为 GM，首条记录 space=GM
    ASSERT_EQ(kernelRecords[1].recordType, RecordType::DYNAMIC_OP);
    ASSERT_EQ(kernelRecords[1].payload.dynamicRecord.dynamicType, RecordType::SIMT_ENTRY);
    ASSERT_EQ(kernelRecords[1].payload.dynamicRecord.count, 2U);
    auto *gmRecords = reinterpret_cast<ShadowMemoryRecord *>(kernelRecords[1].payload.dynamicRecord.buffer);
    ASSERT_NE(gmRecords, nullptr);
    EXPECT_EQ(gmRecords[0].space, AddressSpace::GM);
    EXPECT_EQ(gmRecords[0].addr, 0x1000U);
    EXPECT_EQ(gmRecords[1].space, AddressSpace::GM);
    EXPECT_EQ(gmRecords[1].addr, 0x2000U);
}
