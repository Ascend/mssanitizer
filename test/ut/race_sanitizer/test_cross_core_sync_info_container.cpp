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

#include "core/framework/cross_core_sync_info_container.h"

using namespace Sanitizer;

namespace SanitizerTest {

LocInfo BuildAivLoc() {
    LocInfo loc{};
    loc.fileNo = 1;
    loc.lineNo = 10;
    loc.pc = 0x1234;
    loc.deviceIdx = 0;
    loc.kernelIdx = 0;
    loc.deviceId = 0;
    loc.coreId = 25;
    loc.blockType = BlockType::AIVEC;
    return loc;
}

TEST(CrossCoreSyncInfoContainer, set_ffts_mode0_and_wait_flag_dev_expect_success)
{
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(6, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE0, 0, vt);
    std::fill(vt.begin(), vt.end(), 2);
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE0, 1, vt);
    std::fill(vt.begin(), vt.end(), 4);
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE0, 3, vt);
    std::fill(vt.begin(), vt.end(), 3);
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE0, 4, vt);
    std::fill(vt.begin(), vt.end(), 0);
    bool getflag = syncContainer.GetBlockSyncInfo(0, 1, vt);
    ASSERT_TRUE(getflag);
    ASSERT_EQ(vt[0], 4U);
}

TEST(CrossCoreSyncInfoContainer, set_ffts_mode1_and_wait_flag_dev_expect_success)
{
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(6, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE1, 0, vt);
    std::fill(vt.begin(), vt.end(), 3);
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE1, 1, vt);
    std::fill(vt.begin(), vt.end(), 0);
    bool getflag = syncContainer.GetBlockSyncInfo(0, 1, vt);
    ASSERT_TRUE(getflag);
    ASSERT_EQ(vt[0], 3U);
}

TEST(CrossCoreSyncInfoContainer, aiv_set_ffts_mode2_and_wait_flag_dev_expect_success)
{
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(6, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE2, 0, vt);
    std::fill(vt.begin(), vt.end(), 3);
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE2, 1, vt);
    std::fill(vt.begin(), vt.end(), 0);
    bool getflag = syncContainer.GetBlockSyncInfo(0, 2, vt);
    ASSERT_TRUE(getflag);
    ASSERT_EQ(vt[0], 3U);
}

TEST(CrossCoreSyncInfoContainer, aic_set_ffts_mode2_and_wait_flag_dev_expect_success)
{
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(6, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE2, 2, vt);
    std::fill(vt.begin(), vt.end(), 0);
    bool getflag = syncContainer.GetBlockSyncInfo(0, 0, vt);
    ASSERT_TRUE(getflag);
    ASSERT_EQ(vt[0], 1U);
    std::fill(vt.begin(), vt.end(), 0);
    getflag = syncContainer.GetBlockSyncInfo(0, 1, vt);
    ASSERT_TRUE(getflag);
    ASSERT_EQ(vt[0], 1U);
}

TEST(CrossCoreSyncInfoContainer, aiv_ib_set_and_ib_wait_expect_success)
{
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(6, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    syncContainer.SetBlockSoftSyncInfo(0, 0, vt);
    bool ret = syncContainer.GetBlockSoftSyncInfo(0, 0, vt);
    ASSERT_TRUE(ret);
    ASSERT_EQ(vt[0], 1U);
    std::fill(vt.begin(), vt.end(), 3);
    syncContainer.SetBlockSoftSyncInfo(1, 1, vt);
    std::fill(vt.begin(), vt.end(), 0);
    ret = syncContainer.GetBlockSoftSyncInfo(1, 1, vt);
    ASSERT_TRUE(ret);
    ASSERT_EQ(vt[0], 3U);
}

// 场景：上报方声明"不限制对端核"（peerCoreId < 0），即 MSTX 跨核 set/wait 上报接口的 peerCoreId = -1。
// 预期：wait 按 eventID 在全体核中配对，不会因核号越界而永久等待；配对后 set 被消费。
TEST(CrossCoreSyncInfoContainer, ib_wait_with_unspecified_peer_core_expect_success) {
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(6, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 0);
    // 生产者核 4 上报 set
    std::fill(vt.begin(), vt.end(), 7);
    syncContainer.SetBlockSoftSyncInfo(0x1300002, 4, vt);

    // 消费者携带 peerCoreId = -1，仍应配对成功并取到 set 的向量时钟
    std::fill(vt.begin(), vt.end(), 0);
    ASSERT_TRUE(syncContainer.GetBlockSoftSyncInfo(0x1300002, -1, vt));
    ASSERT_EQ(vt[0], 7U);
    // set 已被消费，重复 wait 不应再成功
    ASSERT_FALSE(syncContainer.GetBlockSoftSyncInfo(0x1300002, -1, vt));
    // 无对应 set 时也不能因 peerCoreId < 0 而放行
    ASSERT_FALSE(syncContainer.GetBlockSoftSyncInfo(0x1300003, -1, vt));

    // 指定核号越界（如旧实现中被截断后的 65535）：即便该 eventID 确有 set，也不得误配对
    std::fill(vt.begin(), vt.end(), 9);
    syncContainer.SetBlockSoftSyncInfo(0x1300004, 4, vt);
    ASSERT_FALSE(syncContainer.GetBlockSoftSyncInfo(0x1300004, 65535, vt));
    // 用正确的核号仍可配对，说明上一步只是被越界拦截、set 未被消费
    std::fill(vt.begin(), vt.end(), 0);
    ASSERT_TRUE(syncContainer.GetBlockSoftSyncInfo(0x1300004, 4, vt));
    ASSERT_EQ(vt[0], 9U);
}

TEST(CrossCoreSyncInfoContainer, aiv_sync_all_expect_success)
{
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(6, KernelType::MIX);
    VectorTime vt0, vt1;
    vt0.resize(66, 1);
    vt1.resize(66, 1);
    bool syncAllRet = syncContainer.SyncAll(0, 2, 0, vt0);
    ASSERT_FALSE(syncAllRet);
    ASSERT_EQ(vt0[0], 2U);
    syncAllRet = syncContainer.SyncAll(1, 2, 1, vt1);
    ASSERT_FALSE(syncAllRet);
    ASSERT_EQ(vt1[1], 2U);
    syncAllRet = syncContainer.SyncAll(0, 2, 0, vt0);
    ASSERT_FALSE(syncAllRet);
    ASSERT_EQ(vt0[0], 2U);
    std::vector<VectorTime> vt = std::vector<VectorTime>{ vt0, vt1 };
    ASSERT_NO_THROW(syncContainer.UpdateSyncAllVectorTime(vt));
}

TEST(CrossCoreSyncInfoContainer, mstx_cross_set_wait_expect_success)
{
    CrossCoreSyncInfoContainer syncContainer;
    MstxCrossInfo crossInfo = {
        .addr = 0x200,
        .flagId = 1,
        .pipe = PipeType::PIPE_MTE2,
        .isMore = false,
        .isMerge = false,
        .opType = SyncType::MSTX_SET_CROSS,
    };
    syncContainer.Init(6, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    syncContainer.SetMstxCrossInfo(crossInfo, vt);
    std::fill(vt.begin(), vt.end(), 10);
    crossInfo.flagId = 1;
    bool ret = syncContainer.GetMstxCrossInfo(crossInfo, vt);
    ASSERT_TRUE(ret);
    ASSERT_EQ(vt[4], 10U);
}

// AIV上syncID超过15时，硬件会截断高bit位，工具需同步截断以避免死锁误报
// MIX内核1组: block0=AIV0, block1=AIV1, block2=AIC, vecSubBlockDim=2
TEST(CrossCoreSyncInfoContainer, aiv0_set_intra_block_with_syncid_over_15_expect_truncated_and_match_aic_wait) {
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(3, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    // AIV0使用syncID=20(>=16)，硬件截断为4，AIC用syncID=4应能匹配
    syncContainer.SetBlockSyncInfo(20, FftsSyncMode::MODE4, 0, vt);
    std::fill(vt.begin(), vt.end(), 0);
    bool ret = syncContainer.GetIntraBlockSyncInfo(4, 2, vt);
    ASSERT_TRUE(ret);
    ASSERT_EQ(vt[0], 1U);
}

TEST(CrossCoreSyncInfoContainer, aiv0_wait_intra_block_with_syncid_over_15_expect_truncated_and_match_aic_set) {
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(3, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    // AIC用syncID=4设置，AIV0用syncID=20(>=16)等待，硬件截断为4应能匹配
    syncContainer.SetBlockSyncInfo(4, FftsSyncMode::MODE4, 2, vt);
    std::fill(vt.begin(), vt.end(), 0);
    bool ret = syncContainer.GetIntraBlockSyncInfo(20, 0, vt);
    ASSERT_TRUE(ret);
    ASSERT_EQ(vt[0], 1U);
}

TEST(CrossCoreSyncInfoContainer, aiv1_set_intra_block_with_syncid_over_15_expect_truncated_and_match_aic_wait) {
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(3, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    // AIV1使用syncID=20(>=16)，硬件截断为4，AIV1映射到AIC的syncID=4+16=20，AIC用syncID=20应能匹配
    syncContainer.SetBlockSyncInfo(20, FftsSyncMode::MODE4, 1, vt);
    std::fill(vt.begin(), vt.end(), 0);
    bool ret = syncContainer.GetIntraBlockSyncInfo(20, 2, vt);
    ASSERT_TRUE(ret);
    ASSERT_EQ(vt[0], 1U);
}

TEST(CrossCoreSyncInfoContainer, aiv1_wait_intra_block_with_syncid_over_15_expect_truncated_and_match_aic_set) {
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(3, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    // AIC用syncID=20设置(映射到AIV1的syncID=4)，AIV1用syncID=20(>=16)等待，硬件截断为4应能匹配
    syncContainer.SetBlockSyncInfo(20, FftsSyncMode::MODE4, 2, vt);
    std::fill(vt.begin(), vt.end(), 0);
    bool ret = syncContainer.GetIntraBlockSyncInfo(20, 1, vt);
    ASSERT_TRUE(ret);
    ASSERT_EQ(vt[0], 1U);
}

// AIV 核 mode4 场景使用非法 flag_id(>=16) 应触发告警，且不影响硬件截断逻辑
TEST(CrossCoreSyncInfoContainer, aiv_set_mode4_with_invalid_flag_id_expect_warn)
{
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(3, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    syncContainer.SetBlockSyncInfo(20, FftsSyncMode::MODE4, 0, vt, 2, BuildAivLoc(), 100);
    const auto &warnInfos = syncContainer.GetFlagIdWarnInfo();
    ASSERT_EQ(warnInfos.size(), 1U);
    ASSERT_EQ(warnInfos[0].flagId, 20U);
    ASSERT_EQ(warnInfos[0].baseEvent.serialNo, 100U);
    ASSERT_EQ(warnInfos[0].baseEvent.deviceId, 0U);
    ASSERT_EQ(warnInfos[0].baseEvent.coreId, 25U);
    ASSERT_EQ(warnInfos[0].baseEvent.blockType, BlockType::AIVEC);
    ASSERT_EQ(warnInfos[0].baseEvent.pc, 0x1234U);
    // 告警收集不影响硬件截断逻辑
    std::fill(vt.begin(), vt.end(), 0);
    bool ret = syncContainer.GetIntraBlockSyncInfo(4, 2, vt);
    ASSERT_TRUE(ret);
}

// AIV 核 mode4 场景使用合法 flag_id(0-15) 不应触发告警
TEST(CrossCoreSyncInfoContainer, aiv_set_mode4_with_valid_flag_id_expect_no_warn)
{
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(3, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    syncContainer.SetBlockSyncInfo(5, FftsSyncMode::MODE4, 0, vt, 2, BuildAivLoc(), 100);
    ASSERT_TRUE(syncContainer.GetFlagIdWarnInfo().empty());
}

// AIC 核 mode4 场景使用合法 flag_id(16-31) 不应触发告警
TEST(CrossCoreSyncInfoContainer, aic_set_mode4_with_valid_flag_id_expect_no_warn)
{
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(3, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    // blockIdx=2 为 AIC，使用 flagId=20(16-31) 合法
    syncContainer.SetBlockSyncInfo(20, FftsSyncMode::MODE4, 2, vt, 2, BuildAivLoc(), 100);
    ASSERT_TRUE(syncContainer.GetFlagIdWarnInfo().empty());
}

// 多条非法 flag_id 应记录多条告警，ClearFlagIdWarnInfo 可清空
TEST(CrossCoreSyncInfoContainer, flag_id_warn_info_clear_expect_empty)
{
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(3, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    syncContainer.SetBlockSyncInfo(20, FftsSyncMode::MODE4, 0, vt, 2, BuildAivLoc(), 100);
    syncContainer.SetBlockSyncInfo(18, FftsSyncMode::MODE4, 1, vt, 2, BuildAivLoc(), 101);
    ASSERT_EQ(syncContainer.GetFlagIdWarnInfo().size(), 2U);
    syncContainer.ClearFlagIdWarnInfo();
    ASSERT_TRUE(syncContainer.GetFlagIdWarnInfo().empty());
}

// 场景：kernel1 凑齐 MODE0 全 AIV 阻塞、产生未消费的 waitVec，随后同一容器 Init 进入 kernel2。
// 预期：Init 清空 blockSyncEvent_，kernel2 的 GetBlockSyncInfo 返回 false（否则 wait 被过期 set 满足 → 漏报）。
TEST(CrossCoreSyncInfoContainer, reinit_shall_clear_ffts_sync_info_across_kernels)
{
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(6, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    // kernel#1：4 个 AIV block（c220 默认 vecSubBlockDim=2 -> AIVCount=4）到齐，产生各 AIV block 的 wait
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE0, 0, vt);
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE0, 1, vt);
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE0, 3, vt);
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE0, 4, vt);

    // 同一对象进入 kernel#2：上一 kernel 残留的 wait 必须已被清空
    syncContainer.Init(6, KernelType::MIX);
    std::fill(vt.begin(), vt.end(), 0);
    ASSERT_FALSE(syncContainer.GetBlockSyncInfo(0, 0, vt));
}

// 场景：kernel1 留下未消费的软同步（IB_SET）信息，随后同一容器 Init 进入 kernel2。
// 预期：Init 清空 blockSoftSyncInfo_，kernel2 的 GetBlockSoftSyncInfo 返回 false。
TEST(CrossCoreSyncInfoContainer, reinit_shall_clear_soft_sync_info_across_kernels)
{
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(6, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    // kernel#1 留下一个未被消费的软同步 set
    syncContainer.SetBlockSoftSyncInfo(0, 0, vt);

    syncContainer.Init(6, KernelType::MIX);
    std::fill(vt.begin(), vt.end(), 0);
    ASSERT_FALSE(syncContainer.GetBlockSoftSyncInfo(0, 0, vt));
}

// 场景：kernel1 留下未消费的 MSTX 跨核 set，随后同一容器 Init 进入 kernel2。
// 预期：Init 清空 mstxCrossSetMap_，kernel2 的 GetMstxCrossInfo 返回 false。
TEST(CrossCoreSyncInfoContainer, reinit_shall_clear_mstx_cross_set_across_kernels)
{
    CrossCoreSyncInfoContainer syncContainer;
    MstxCrossInfo crossInfo = {
        .addr = 0x200,
        .flagId = 1,
        .pipe = PipeType::PIPE_MTE2,
        .isMore = false,
        .isMerge = false,
        .opType = SyncType::MSTX_SET_CROSS,
    };
    syncContainer.Init(6, KernelType::MIX);
    VectorTime vt;
    vt.resize(66, 1);
    // kernel#1 留下一个未被消费的 mstx set
    syncContainer.SetMstxCrossInfo(crossInfo, vt);

    syncContainer.Init(6, KernelType::MIX);
    std::fill(vt.begin(), vt.end(), 0);
    ASSERT_FALSE(syncContainer.GetMstxCrossInfo(crossInfo, vt));
}

// 场景：__mix__(0,1)（vec 子核数=1）、blockDim=2，两个 AIV 核各发一个 MODE0 全 AIV 阻塞 set。
// 预期：全 AIV 阻塞的到齐数按 1 算（=2）即满足、GetBlockSyncInfo 为 true；按固定 2 算需 4 个，误报卡死。
TEST(CrossCoreSyncInfoContainer, mix_with_single_vec_sub_block_dim_need_one_aiv_set_per_core)
{
    CrossCoreSyncInfoContainer syncContainer;
    // blockDim=2 -> maxBlockNum_ = 2 * C220_MIX_SUB_BLOCKDIM = 6，逻辑 AICore 数 = 2
    syncContainer.Init(2U * C220_MIX_SUB_BLOCKDIM, KernelType::MIX);
    syncContainer.SetVecSubBlockDim(1U);
    ASSERT_EQ(syncContainer.GetVecSubBlockDim(), 1U);

    VectorTime vt;
    vt.resize(66, 1);
    // 2 个 AIV 核（展开后 block 0 / block 1）各 set 一次即应到齐
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE0, 0, vt);
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE0, 1, vt);

    std::fill(vt.begin(), vt.end(), 0);
    ASSERT_TRUE(syncContainer.GetBlockSyncInfo(0, 0, vt));
}

// 场景：__mix__(1,2)（vec 子核数=2）、blockDim=2，需 4 个 AIV 核到齐（对照组，防过度修正）。
// 预期：只到齐 2/4 时不满足（false），4/4 才满足（true）。
TEST(CrossCoreSyncInfoContainer, mix_with_two_vec_sub_block_dim_need_two_aiv_sets_per_core)
{
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(2U * C220_MIX_SUB_BLOCKDIM, KernelType::MIX);
    syncContainer.SetVecSubBlockDim(C220_VEC_SUB_BLOCKDIM);

    VectorTime vt;
    vt.resize(66, 1);
    // 只到齐 2/4 个 AIV 核，不能提前放行
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE0, 0, vt);
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE0, 1, vt);
    std::fill(vt.begin(), vt.end(), 0);
    ASSERT_FALSE(syncContainer.GetBlockSyncInfo(0, 0, vt));

    // 其余 2 个 AIV 核到齐后才放行
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE0, 3, vt);
    syncContainer.SetBlockSyncInfo(0, FftsSyncMode::MODE0, 4, vt);
    std::fill(vt.begin(), vt.end(), 0);
    ASSERT_TRUE(syncContainer.GetBlockSyncInfo(0, 0, vt));
}

// 场景：kernel1 上报 vecSubBlockDim=1；kernel2 的记录未携带该值，复用同一容器。
// 预期：Init 把 vecSubBlockDim_ 复位为默认值 2，不残留上一算子的取值。
TEST(CrossCoreSyncInfoContainer, reinit_shall_restore_default_vec_sub_block_dim)
{
    CrossCoreSyncInfoContainer syncContainer;
    syncContainer.Init(2U * C220_MIX_SUB_BLOCKDIM, KernelType::MIX);
    syncContainer.SetVecSubBlockDim(1U);
    ASSERT_EQ(syncContainer.GetVecSubBlockDim(), 1U);

    // 下一个算子的记录未携带 vecSubBlockDim 时必须回到默认值 2
    syncContainer.Init(2U * C220_MIX_SUB_BLOCKDIM, KernelType::MIX);
    ASSERT_EQ(syncContainer.GetVecSubBlockDim(), C220_VEC_SUB_BLOCKDIM);
}
}
