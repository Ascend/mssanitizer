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
}
