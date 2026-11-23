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

#include "heap_block_manager.h"

using namespace Sanitizer;

TEST(HeapBlockManager, bypass_free_requires_both_tables_and_preserves_size_on_failure)
{
    for (auto source : {MemInfoSrc::ACL, MemInfoSrc::EXTRA}) {
        SCOPED_TRACE(static_cast<int>(source));
        HeapBlockManager manager;
        MemOpRecord record{};
        record.dstAddr = 0x1000;
        record.memSize = 16;
        record.infoSrc = source;
        ASSERT_TRUE(manager.AddHeapBlock(record));

        record.infoSrc = MemInfoSrc::BYPASS;
        uint64_t size = 99;
        const auto error = manager.FreeHeapBlock(record, size);
        EXPECT_TRUE(error.isError);
        EXPECT_EQ(error.type, MemErrorType::ILLEGAL_FREE);
        EXPECT_EQ(error.auxData.badAddr.addr, record.dstAddr);
        EXPECT_EQ(size, 99U);
        EXPECT_EQ(manager.GetHeapBlockSize(record), record.memSize);

        // 失败后，原来存在的那张表仍可通过对应来源正常释放。
        record.infoSrc = source;
        EXPECT_FALSE(manager.FreeHeapBlock(record, size).isError);
        EXPECT_EQ(size, record.memSize);
        EXPECT_EQ(manager.GetHeapBlockSize(record), 0U);
    }
}

TEST(HeapBlockManager, bypass_free_removes_both_tables_and_returns_runtime_size)
{
    HeapBlockManager manager;
    MemOpRecord record{};
    record.dstAddr = 0x1000;
    record.memSize = 16;
    record.infoSrc = MemInfoSrc::ACL;
    ASSERT_TRUE(manager.AddHeapBlock(record));
    record.infoSrc = MemInfoSrc::EXTRA;
    record.memSize = 32;
    ASSERT_TRUE(manager.AddHeapBlock(record));

    record.infoSrc = MemInfoSrc::BYPASS;
    uint64_t size = 99;
    EXPECT_FALSE(manager.FreeHeapBlock(record, size).isError);
    EXPECT_EQ(size, 16U);
    EXPECT_TRUE(manager.GetHeapBlocks().empty());
    EXPECT_EQ(manager.GetHeapBlockSize(record), 0U);

    size = 99;
    const auto error = manager.FreeHeapBlock(record, size);
    EXPECT_TRUE(error.isError);
    EXPECT_EQ(error.type, MemErrorType::ILLEGAL_FREE);
    EXPECT_EQ(size, 99U);
}
