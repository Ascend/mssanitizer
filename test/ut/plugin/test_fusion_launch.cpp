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
#include <array>
#include <cstring>
#include <vector>
#include "ccec_defs.h"
#include "plugin/recorder.h"
#include "fusion_host_args_fixture.h"

using namespace Sanitizer;

struct FusionLayout {
    uint32_t headSize;
    bool inlineTiling;
};
class FusionAicIntegration : public testing::TestWithParam<FusionLayout> {};

TEST_P(FusionAicIntegration, AdaptedArgumentDrivesExistingRecorderAndPreservesKernelParameters) {
    const auto layout = GetParam();
    const uint32_t expectedOffset = (layout.headSize + 7U) / 8U * 8U;
    std::vector<uint8_t> buffer(sizeof(RecordGlobalHead) + sizeof(RecordBlockHead) + 4096, 0);
    RecordGlobalHead global{};
    global.kernelInfo.totalBlockDim = 1;
    global.kernelInfo.kernelParamNum = expectedOffset / sizeof(uint64_t);
    std::memcpy(buffer.data(), &global, sizeof(global));
    std::array<uint8_t, 56> args{};
    uint32_t recordOffset = 0;
    ASSERT_TRUE(
        MakeFusionAicArgs(buffer.data(), args.data(), args.size(), recordOffset, layout.headSize, layout.inlineTiling));
    ASSERT_EQ(recordOffset, expectedOffset);
    std::array<uint64_t, 7> slots{};
    std::memcpy(slots.data(), args.data(), args.size());
    EXPECT_EQ(slots[0], 0x1000);
    EXPECT_EQ(slots[1], 0x2000);
    EXPECT_EQ(slots[2], layout.inlineTiling ? 0xabc000 : 0);
    const auto recordSlot = recordOffset / sizeof(uint64_t);
    ASSERT_EQ(slots[recordSlot], reinterpret_cast<uint64_t>(buffer.data()));
    if (layout.inlineTiling) {
        EXPECT_EQ(slots[recordSlot + 1], 0x1234);
        EXPECT_EQ(slots[recordSlot + 2], 0x5678);
    }
    Recorder recorder(reinterpret_cast<uint8_t *>(slots[recordOffset / sizeof(uint64_t)]), 0);
    UnaryOpRecord operation{};
    operation.dst = 0x1000;
    operation.src = 0x2000;
    operation.location.pc = 0x3456;
    recorder.DumpRecord<RecordType::UNARY_OP>(operation);
    const auto *block = reinterpret_cast<const RecordBlockHead *>(buffer.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(block->recordCount, 1);
    ASSERT_EQ(block->recordWriteCount, 1);
    const auto *written = buffer.data() + sizeof(RecordGlobalHead) + GetRecordHeadSize(0, 0);
    RecordType type{};
    std::memcpy(&type, written, sizeof(type));
    ASSERT_EQ(type, RecordType::UNARY_OP);
    UnaryOpRecord copied{};
    std::memcpy(&copied, written + sizeof(type), sizeof(copied));
    EXPECT_EQ(copied.dst, operation.dst);
    EXPECT_EQ(copied.src, operation.src);
    EXPECT_EQ(copied.location.pc, operation.location.pc);
}

// 覆盖对齐/非对齐参数头，以及无内联 tiling 的参数布局。
INSTANTIATE_TEST_SUITE_P(ArgumentLayouts, FusionAicIntegration,
    testing::Values(FusionLayout{24, true}, FusionLayout{25, true}, FusionLayout{31, false}, FusionLayout{32, false}));
