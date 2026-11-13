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
#include <functional>
#include "runtime/inject_helpers/FusionLaunchArgs.h"
#include "utils/Protocol.h"

namespace {
// 仅此编译单元引入 Host ABI，避免与设备 recorder 的同名类型混用。
struct FusionInput {
    std::array<uint8_t, 48> bytes{};
    std::array<rtHostInputInfo_t, 2> inputs{{{16, 24}, {8, 32}}};
    std::array<rtLaunchAttribute_t, 2> attrs{};
    rtLaunchConfig_t config{};
    rtFunsionTaskInfo_t tasks{};
    rtFusionArgsEx_t args{};

    FusionInput() {
        attrs[0].id = RT_LAUNCH_ATTRIBUTE_BLOCKDIM;
        attrs[0].value.blockDim = 1;
        attrs[1] = attrs[0];
        config = {attrs.data(), 1};
        tasks.subTaskNum = 2;
        tasks.subTask[0].type = RT_FUSION_CCU;
        tasks.subTask[1].type = RT_FUSION_AICORE;
        tasks.subTask[1].task.aicoreInfo = {reinterpret_cast<void *>(1), 7, &config, nullptr};
        args.args = bytes.data();
        args.argsSize = bytes.size();
        args.hostInputInfoPtr = inputs.data();
        args.hostInputInfoNum = 1;
    }
};

struct RejectedInput {
    const char *name;
    std::function<void(FusionInput &)> change;
};

class FusionArgsRejection : public testing::TestWithParam<RejectedInput> {};

TEST_P(FusionArgsRejection, InitRejectsUnsupportedInputBeforeBuildingCopy) {
    FusionInput input;
    // 每例先证明默认夹具合法，避免因为无关的前置错误而假通过。
    FusionLaunchArgs valid;
    ASSERT_TRUE(valid.Init(&input.tasks, &input.args));
    GetParam().change(input);
    const auto before = input.bytes;
    FusionLaunchArgs launch;
    EXPECT_FALSE(launch.Init(&input.tasks, &input.args));
    EXPECT_EQ(launch.Args()->args, nullptr);
    EXPECT_EQ(input.bytes, before);
}

INSTANTIATE_TEST_SUITE_P(UnsupportedInputs, FusionArgsRejection,
    testing::Values(RejectedInput{"NullBuffer", [](FusionInput &x) { x.args.args = nullptr; }},
        RejectedInput{"EmptyBuffer", [](FusionInput &x) { x.args.argsSize = 0; }},
        RejectedInput{"OversizedBuffer", [](FusionInput &x) { x.args.argsSize = MAX_ALL_PARAM_SIZE + 1; }},
        RejectedInput{"DeviceArguments", [](FusionInput &x) { x.args.isNoNeedH2DCopy = 1; }},
        RejectedInput{"AicpuArguments", [](FusionInput &x) { x.args.aicpuNum = 1; }},
        RejectedInput{"MissingHostInputs", [](FusionInput &x) { x.args.hostInputInfoPtr = nullptr; }},
        RejectedInput{"NoTasks", [](FusionInput &x) { x.tasks.subTaskNum = 0; }},
        RejectedInput{"OneTask", [](FusionInput &x) { x.tasks.subTaskNum = 1; }},
        RejectedInput{"ThreeTasks", [](FusionInput &x) { x.tasks.subTaskNum = 3; }},
        RejectedInput{"TwoAicTasks", [](FusionInput &x) { x.tasks.subTask[0] = x.tasks.subTask[1]; }},
        RejectedInput{"TwoCcuTasks", [](FusionInput &x) { x.tasks.subTask[1] = x.tasks.subTask[0]; }},
        RejectedInput{"OtherTaskType", [](FusionInput &x) { x.tasks.subTask[0].type = RT_FUSION_AICPU; }},
        RejectedInput{"MissingHandle", [](FusionInput &x) { x.tasks.subTask[1].task.aicoreInfo.hdl = nullptr; }},
        RejectedInput{"StubFunction",
            [](FusionInput &x) { x.tasks.subTask[1].task.aicoreInfo.stubFunc = reinterpret_cast<void *>(1); }},
        RejectedInput{"MissingConfig", [](FusionInput &x) { x.tasks.subTask[1].task.aicoreInfo.config = nullptr; }},
        RejectedInput{"MissingAttributes", [](FusionInput &x) { x.config.attrs = nullptr; }},
        RejectedInput{"EmptyAttributes", [](FusionInput &x) { x.config.numAttrs = 0; }},
        RejectedInput{"TooManyAttributes", [](FusionInput &x) { x.config.numAttrs = RT_LAUNCH_ATTRIBUTE_MAX + 1; }},
        RejectedInput{"MissingBlockDim", [](FusionInput &x) { x.attrs[0].id = RT_LAUNCH_ATTRIBUTE_DYN_UBUF_SIZE; }},
        RejectedInput{"ZeroBlockDim", [](FusionInput &x) { x.attrs[0].value.blockDim = 0; }},
        RejectedInput{"DuplicateBlockDim", [](FusionInput &x) { x.config.numAttrs = 2; }},
        RejectedInput{"DataAtBufferEnd", [](FusionInput &x) { x.inputs[0].dataOffset = x.args.argsSize; }},
        RejectedInput{"DataBeyondBufferEnd", [](FusionInput &x) { x.inputs[0].dataOffset = x.args.argsSize + 1; }},
        RejectedInput{"MisalignedSlot17", [](FusionInput &x) { x.inputs[0].addrOffset = 17; }},
        RejectedInput{"MisalignedSlot20", [](FusionInput &x) { x.inputs[0].addrOffset = 20; }},
        RejectedInput{"SlotBeyondHead", [](FusionInput &x) { x.inputs[0].addrOffset = 32; }},
        RejectedInput{"SlotAtHeadEnd", [](FusionInput &x) { x.inputs[0].addrOffset = 24; }},
        // 对齐地址槽仍可能跨越 head，单独看护长度判断，不能只测试不对齐的 20。
        RejectedInput{"AlignedSlotCrossesHead", [](FusionInput &x) { x.inputs[0].dataOffset = 23; }},
        RejectedInput{"DuplicateAddressSlot",
            [](FusionInput &x) {
                x.args.hostInputInfoNum = 2;
                x.inputs[1].addrOffset = x.inputs[0].addrOffset;
            }}),
    [](const testing::TestParamInfo<RejectedInput> &info) { return info.param.name; });

TEST(FusionArgsValidation, NullTopLevelPointersAreRejected) {
    FusionInput input;
    FusionLaunchArgs launch;
    EXPECT_FALSE(launch.Init(nullptr, &input.args));
    EXPECT_FALSE(launch.Init(&input.tasks, nullptr));
}

TEST(FusionArgsValidation, EarliestInlineDataDefinesHeadRegardlessOfDescriptorOrder) {
    FusionInput input;
    input.args.hostInputInfoNum = 2;
    std::swap(input.inputs[0], input.inputs[1]);
    FusionLaunchArgs launch;
    ASSERT_TRUE(launch.Init(&input.tasks, &input.args));
    EXPECT_EQ(launch.HeadSize(), 24);
    EXPECT_EQ(launch.RecordOffset(), 24);
    // 不同槽可以共享内联数据；禁止的是重复槽，而不是重复 dataOffset。
    input.inputs[0].dataOffset = input.inputs[1].dataOffset;
    FusionLaunchArgs shared;
    ASSERT_TRUE(shared.Init(&input.tasks, &input.args));
    EXPECT_EQ(shared.HeadSize(), 24);
}

TEST(FusionArgsValidation, SetTilingRequiresAnExistingCompleteInlineSlot) {
    FusionInput input;
    FusionLaunchArgs launch;
    ASSERT_TRUE(launch.Init(&input.tasks, &input.args));
    EXPECT_TRUE(launch.SetTiling(0));
    EXPECT_EQ(launch.KernelArgs()->hasTiling, 0);
    EXPECT_FALSE(launch.SetTiling(4)); // 超出 24 字节 head。
    EXPECT_FALSE(launch.SetTiling(2)); // 槽完整，但没有对应内联描述。
    ASSERT_TRUE(launch.SetTiling(3));
    EXPECT_EQ(launch.KernelArgs()->hasTiling, 1);
    EXPECT_EQ(launch.KernelArgs()->tilingAddrOffset, 16);
    EXPECT_EQ(launch.KernelArgs()->tilingDataOffset, 24);

    input.args.hostInputInfoNum = 0;
    input.args.hostInputInfoPtr = nullptr;
    FusionLaunchArgs noInline;
    ASSERT_TRUE(noInline.Init(&input.tasks, &input.args));
    EXPECT_TRUE(noInline.SetTiling(0));
    EXPECT_FALSE(noInline.SetTiling(3)); // 序号合法但无内联槽，不能伪造 tiling。
}

TEST(FusionArgsValidation, AppendRecordRequiresInitializedBufferAndOnlySucceedsOnce) {
    std::array<uint8_t, 8> record{};
    FusionLaunchArgs empty;
    EXPECT_FALSE(empty.AppendRecord(record.data()));
    FusionInput input;
    FusionLaunchArgs launch;
    ASSERT_TRUE(launch.Init(&input.tasks, &input.args));
    EXPECT_FALSE(launch.AppendRecord(nullptr));
    ASSERT_TRUE(launch.AppendRecord(record.data()));
    EXPECT_FALSE(launch.AppendRecord(record.data()));
}
} // namespace
