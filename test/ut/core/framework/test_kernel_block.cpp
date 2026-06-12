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
    std::vector<uint8_t> CreateValidMemBuffer(uint64_t &outMemSize, uint32_t blockIdx = 0)
    {
        // 计算最小所需大小：全局头 + 块头 + 一些记录空间
        uint64_t simdRecordsSize = 1024; // 1KB for simd records
        // GetAllThreadSize 计算的是 (simtErrorInfo.size + sizeof(SimtRecordBlockHead)) * SIMT_THREAD_MAX_SIZE
        uint64_t simtErrorInfoSize = sizeof(SimtRecordBlockHead);
        uint64_t simtSize = (simtErrorInfoSize + sizeof(SimtRecordBlockHead)) * SIMT_THREAD_MAX_SIZE;
        uint64_t shadowMemHeadSize = sizeof(ShadowMemoryRecordHead);
        uint64_t shadowMemRecordsSize = 10 * sizeof(ShadowMemoryRecord); // 10 records
        uint64_t simtEntryHeadSize = sizeof(SimtEntryBlockHead);

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
