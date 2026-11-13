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

#include "fusion_host_args_fixture.h"
#include <array>
#include <vector>
#include <cstring>
#include "runtime/inject_helpers/FusionLaunchArgs.h"

testing::AssertionResult MakeFusionAicArgs(
    uint8_t *record, uint8_t *output, uint32_t capacity, uint32_t &recordOffset, uint32_t headSize, bool inlineTiling) {
    if (headSize < 24 || headSize > 32) {
        return testing::AssertionFailure() << "headSize must be in [24, 32], got " << headSize;
    }
    rtLaunchAttribute_t attr{};
    attr.id = RT_LAUNCH_ATTRIBUTE_BLOCKDIM;
    attr.value.blockDim = 1;
    rtLaunchConfig_t config{&attr, 1};
    rtFunsionTaskInfo_t tasks{};
    tasks.subTaskNum = 2;
    tasks.subTask[0].type = RT_FUSION_CCU;
    tasks.subTask[1].type = RT_FUSION_AICORE;
    tasks.subTask[1].task.aicoreInfo = {reinterpret_cast<void *>(1), 7, &config, nullptr};
    const std::array<uint64_t, 3> parameters{0x1000, 0x2000, 0};
    const std::array<uint64_t, 2> tilingData{0x1234, 0x5678};
    std::vector<uint8_t> original(headSize + (inlineTiling ? sizeof(tilingData) : 0), 0);
    std::memcpy(original.data(), parameters.data(), sizeof(parameters));
    if (inlineTiling) {
        std::memcpy(original.data() + headSize, tilingData.data(), sizeof(tilingData));
    }
    const auto before = original;
    rtHostInputInfo_t tiling{16, headSize};
    rtFusionArgsEx_t args{};
    args.args = original.data();
    args.argsSize = original.size();
    args.hostInputInfoPtr = inlineTiling ? &tiling : nullptr;
    args.hostInputInfoNum = inlineTiling ? 1 : 0;
    FusionLaunchArgs launch;
    if (!launch.Init(&tasks, &args)) {
        return testing::AssertionFailure() << "Init rejected the integration fixture";
    }
    if (!launch.SetTiling(inlineTiling ? 3 : 0)) {
        return testing::AssertionFailure() << "SetTiling rejected the integration fixture";
    }
    if (!launch.AppendRecord(record)) {
        return testing::AssertionFailure() << "AppendRecord rejected the record pointer";
    }
    if (launch.Args()->argsSize > capacity) {
        return testing::AssertionFailure()
            << "output capacity " << capacity << " is smaller than adapted argsSize " << launch.Args()->argsSize;
    }
    // 模拟 Runtime 回填设备 tiling 地址，工具视图必须观察到同一份参数缓冲区。
    if (inlineTiling) {
        const uint64_t deviceTiling = 0xabc000;
        if (!launch.KernelArgs()->hasTiling || launch.KernelArgs()->tilingAddrOffset != 16 ||
            launch.KernelArgs()->tilingDataOffset != launch.RecordOffset() + sizeof(uint64_t) ||
            launch.Args()->hostInputInfoPtr[0].dataOffset != launch.KernelArgs()->tilingDataOffset) {
            return testing::AssertionFailure() << "tiling metadata does not match the adapted layout";
        }
        std::memcpy(
            static_cast<uint8_t *>(launch.Args()->args) + tiling.addrOffset, &deviceTiling, sizeof(deviceTiling));
        uint64_t observed = 0;
        std::memcpy(&observed, static_cast<uint8_t *>(launch.KernelArgs()->args) + 16, sizeof(observed));
        if (observed != deviceTiling) {
            return testing::AssertionFailure() << "ordinary argument view did not observe Runtime tiling writeback";
        }
    }
    if (original != before) {
        return testing::AssertionFailure() << "adapter modified the original argument buffer";
    }
    recordOffset = launch.RecordOffset();
    std::memcpy(output, launch.Args()->args, launch.Args()->argsSize);
    return testing::AssertionSuccess();
}
