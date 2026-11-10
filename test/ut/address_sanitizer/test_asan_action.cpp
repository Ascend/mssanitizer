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
#include <algorithm>

#include "bounds_check.h"
#include "core/framework/config_manager.h"
#include "heap_block_manager.h"
#include "platform_config.h"

#define private public
#include "shadow_memory.h"
#undef private
#include "asan_action.h"

using namespace Sanitizer;

namespace {
class AsanActionTest : public testing::Test {
protected:
    void SetUp() override {
        savedMask_ = ConfigManager::Instance().GetInitcheckAddressSpaceMask();
        ConfigManager::Instance().SetInitcheckAddressSpaceMask(InitcheckAddressSpaceBit(AddressSpace::PRIVATE));
        const auto it = CHIP_INFO_MAP.find(DeviceType::ASCEND_910B1);
        ASSERT_NE(it, CHIP_INFO_MAP.end());
        ASSERT_TRUE(shadow_.Init(it->first, it->second));
        bounds_.Init(it->second);
        config_.initCheck = true;
        record_.type = MemOpType::MALLOC;
        record_.dstSpace = AddressSpace::GM;
        record_.srcSpace = AddressSpace::GM;
        record_.dstAddr = 0x11c100000123ULL;
        record_.memSize = 8;
        record_.infoSrc = MemInfoSrc::ACL;
    }

    void TearDown() override { ConfigManager::Instance().SetInitcheckAddressSpaceMask(savedMask_); }

    ErrorMsgList Run(AsanAction &&action, bool ignoreShadow = false) {
        return action.doAction(shadow_, bounds_, config_, ignoreShadow);
    }

    void ExpectError(const ErrorMsgList &errors, MemErrorType type) {
        ASSERT_EQ(errors.size(), 1U);
        EXPECT_EQ(errors[0].type, type);
    }

    void ExpectNoGmMaps() {
        const auto &maps = static_cast<GmPM *>(shadow_.gm_.get())->pmList_;
        // 无子 PM，也就未分配二级映射。
        EXPECT_TRUE(std::all_of(maps.begin(), maps.end(), [](PM *pm) { return pm == nullptr; }));
    }

    uint32_t savedMask_{};
    ShadowMemory shadow_;
    BoundsCheck bounds_;
    Config config_{};
    MemOpRecord record_{};
};

TEST_F(AsanActionTest, stack_only_malloc_copy_free_do_not_materialize_gm_shadow) {
    config_.checkUnusedMemory = true;
    record_.memSize = 2 * ONE_SM_STAND_FOR_BYTE + 17;
    for (auto source : {MemInfoSrc::ACL, MemInfoSrc::EXTRA, MemInfoSrc::BYPASS}) {
        SCOPED_TRACE(static_cast<int>(source));
        record_.infoSrc = source;
        ExpectNoGmMaps();
        ASSERT_TRUE(Run(AsanMalloc(record_)).empty());
        EXPECT_FALSE(bounds_.Check(AddressSpace::GM, record_.dstAddr, record_.memSize, AccessType::READ).isError);
        ExpectNoGmMaps();

        MemOpRecord copy = record_;
        copy.type = MemOpType::MEMCPY_BLOCKS;
        copy.srcAddr = record_.dstAddr;
        ASSERT_TRUE(Run(AsanMemcpyBlocks(copy)).empty());
        ExpectNoGmMaps();

        record_.type = MemOpType::FREE;
        ASSERT_TRUE(Run(AsanFree(record_)).empty());
        EXPECT_TRUE(bounds_.Check(AddressSpace::GM, record_.dstAddr, record_.memSize, AccessType::READ).isError);
        ExpectNoGmMaps();
        ExpectError(Run(AsanFree(record_)), MemErrorType::ILLEGAL_FREE);
        ExpectNoGmMaps();
        EXPECT_TRUE(shadow_.DoLeakCheck().empty());
        record_.type = MemOpType::MALLOC;
    }
}

TEST_F(AsanActionTest, disabled_initcheck_does_not_clear_gm_even_with_gm_mask) {
    ConfigManager::Instance().SetInitcheckAddressSpaceMask(InitcheckAddressSpaceBit(AddressSpace::GM));
    config_.initCheck = false;
    record_.memSize = 2 * ONE_SM_STAND_FOR_BYTE + 17;
    ASSERT_TRUE(Run(AsanMalloc(record_)).empty());
    ASSERT_TRUE(Run(AsanFree(record_)).empty());
    ExpectNoGmMaps();
    EXPECT_TRUE(bounds_.Check(AddressSpace::GM, record_.dstAddr, record_.memSize, AccessType::READ).isError);
}

TEST_F(AsanActionTest, selected_gm_free_invalidates_initialized_memory) {
    ConfigManager::Instance().SetInitcheckAddressSpaceMask(InitcheckAddressSpaceBit(AddressSpace::GM));
    ASSERT_TRUE(Run(AsanMalloc(record_)).empty());
    ASSERT_TRUE(Run(AsanStore(record_)).empty());
    ASSERT_TRUE(Run(AsanLoad(record_)).empty());
    ASSERT_TRUE(Run(AsanFree(record_)).empty());
    const auto errors = Run(AsanLoad(record_));
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors[0].type, MemErrorType::UNINITIALIZED_READ);
    EXPECT_EQ(errors[0].auxData.nBadBytes, record_.memSize);
    EXPECT_TRUE(bounds_.Check(AddressSpace::GM, record_.dstAddr, record_.memSize, AccessType::READ).isError);
}

TEST_F(AsanActionTest, memcheck_retains_overlap_detection_and_resets_shadow_on_free) {
    config_.initCheck = false;
    config_.memCheck = true;
    config_.outOfBoundCheck = true;
    for (uint32_t core : {1U, 3U}) {
        SCOPED_TRACE(core);
        ASSERT_TRUE(Run(AsanMalloc(record_)).empty());
        record_.coreId = core;
        ASSERT_TRUE(Run(AsanStore(record_)).empty());
        record_.coreId = core + 1;
        ExpectError(Run(AsanStore(record_)), MemErrorType::OUT_OF_BOUNDS);
        if (core == 1) {
            ASSERT_TRUE(Run(AsanFree(record_)).empty());
            ExpectError(Run(AsanLoad(record_)), MemErrorType::ILLEGAL_ADDR_READ);
        }
    }
}

TEST_F(AsanActionTest, extra_free_preserves_defined_state_and_clears_core_id_when_gm_is_needed) {
    record_.infoSrc = MemInfoSrc::EXTRA;
    for (bool memCheck : {false, true}) {
        SCOPED_TRACE(memCheck);
        ConfigManager::Instance().SetInitcheckAddressSpaceMask(
            InitcheckAddressSpaceBit(memCheck ? AddressSpace::PRIVATE : AddressSpace::GM));
        config_.initCheck = !memCheck;
        config_.memCheck = memCheck;
        config_.outOfBoundCheck = true;
        record_.coreId = 1;
        ASSERT_TRUE(Run(AsanMalloc(record_)).empty());
        ASSERT_TRUE(Run(AsanStore(record_)).empty());
        ASSERT_TRUE(Run(AsanFree(record_)).empty());
        EXPECT_TRUE(shadow_.LoadNBytes(MemOpRecordForShadow(record_), true).empty());
        record_.coreId = 2;
        EXPECT_TRUE(shadow_.StoreNBytes(MemOpRecordForShadow(record_), true, true).empty());
        shadow_.MakeMemUndefined(record_.dstAddr, record_.memSize);
    }
}

TEST_F(AsanActionTest, ignored_free_preserves_heap_and_selected_gm_shadow_but_updates_bounds) {
    ConfigManager::Instance().SetInitcheckAddressSpaceMask(InitcheckAddressSpaceBit(AddressSpace::GM));
    ASSERT_TRUE(Run(AsanMalloc(record_)).empty());
    ASSERT_TRUE(Run(AsanStore(record_)).empty());
    ASSERT_TRUE(Run(AsanFree(record_), true).empty());
    EXPECT_EQ(shadow_.GetHeapBlockSize(record_), record_.memSize);
    EXPECT_TRUE(Run(AsanLoad(record_)).empty());
    EXPECT_TRUE(bounds_.Check(AddressSpace::GM, record_.dstAddr, record_.memSize, AccessType::READ).isError);
}

TEST_F(AsanActionTest, stack_only_mstx_heap_unregister_restores_bounds_without_gm_shadow) {
    record_.memSize = 2 * ONE_SM_STAND_FOR_BYTE + 17;
    ASSERT_TRUE(Run(AsanMalloc(record_)).empty());
    record_.infoSrc = MemInfoSrc::MSTX_HEAP;
    record_.rootAddr = record_.dstAddr;
    ASSERT_TRUE(Run(AsanMalloc(record_)).empty());
    EXPECT_TRUE(bounds_.Check(AddressSpace::GM, record_.dstAddr, record_.memSize, AccessType::READ).isError);
    ASSERT_TRUE(Run(AsanFree(record_)).empty());
    EXPECT_FALSE(bounds_.Check(AddressSpace::GM, record_.dstAddr, record_.memSize, AccessType::READ).isError);
    ExpectNoGmMaps();
    record_.infoSrc = MemInfoSrc::ACL;
    ASSERT_TRUE(Run(AsanFree(record_)).empty());
    EXPECT_TRUE(shadow_.DoLeakCheck().empty());
    ExpectNoGmMaps();
}
} // namespace
