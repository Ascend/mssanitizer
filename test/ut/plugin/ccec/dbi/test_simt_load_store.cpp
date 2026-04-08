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


#include <random>
#include <utility>
#include <algorithm>
#include <gtest/gtest.h>

#include "../data_process.h"
#define BUILD_DYNAMIC_PROBE
#define __NPU_ARCH__ 3101
#define __DAV_VEC__
#include "plugin/ccec/dbi/probes/simt_load_store.cpp"

using namespace Sanitizer;

namespace SanitizerTest {

TEST(SimtLoadStoreInstructions, dump_simt_load_store_with_initcheck_expect_get_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    RecordGlobalHead head{};
    head.checkParms.defaultcheck = true;
    head.supportSimt = true;
    head.simtInfo.offset = 1024;
    head.simtInfo.threadByteSize = 1024 * 10;
    head.simtInfo.shadowMemoryOffset = 1024 * 1024 * 8;
    head.simtInfo.shadowMemoryByteSize = 1024 * 1024 * 8;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    ShadowMemoryHeapHead smHeapHead;
    smHeapHead.startAddr = reinterpret_cast<uint64_t>(memInfo.data()) + sizeof(RecordGlobalHead) +
        head.simtInfo.shadowMemoryOffset + sizeof(ShadowMemoryHeapHead);
    smHeapHead.size = head.simtInfo.shadowMemoryByteSize > sizeof(ShadowMemoryHeapHead) ?
        head.simtInfo.shadowMemoryByteSize - sizeof(ShadowMemoryHeapHead) : 0U;
    smHeapHead.current = smHeapHead.startAddr;
    smHeapHead.lock = 0U;
    std::copy_n(reinterpret_cast<uint8_t const*>(&smHeapHead), sizeof(ShadowMemoryHeapHead), memInfo.data() + sizeof(RecordGlobalHead) + head.simtInfo.shadowMemoryByteSize);

    __sanitizer_report_simt_ldg_u8(memInfo.data(), 0x100, 0, 0x1000, 8);
    __sanitizer_report_simt_ldg_u16(memInfo.data(), 0x200, 0, 0x2000, 7);
    __sanitizer_report_simt_ldg_b32(memInfo.data(), 0x300, 0, 0x3000, 6);
    __sanitizer_report_simt_ldg_b64(memInfo.data(), 0x400, 0, 0x4000, 5);
    __sanitizer_report_simt_ldg_b128(memInfo.data(), 0x500, 0, 0x5000, 4);
    __sanitizer_report_simt_stg_b8(memInfo.data(), 0x600, 0, 0x6000, 3);
    __sanitizer_report_simt_stg_b16(memInfo.data(), 0x700, 0, 0x7000, 2);
    __sanitizer_report_simt_stg_b32(memInfo.data(), 0x800, 0, 0x8000, 1);
    __sanitizer_report_simt_stg_b64(memInfo.data(), 0x900, 0, 0x9000, 10);
    __sanitizer_report_simt_stg_b128(memInfo.data(), 0x1000, 0, 0x10000, 11);
    // 注意ld st桩的addr需要合理设定否则会导致AddressSpace::INVALID无记录
    __sanitizer_report_simt_ld_u8(memInfo.data(), 0x1000, 0, 0x1000000, 1);
    __sanitizer_report_simt_ld_s8(memInfo.data(), 0x1000, 0, 0x1000000, 2);
    __sanitizer_report_simt_ld_u16(memInfo.data(), 0x1000, 0, 0x1000000, 3);
    __sanitizer_report_simt_ld_s16(memInfo.data(), 0x1000, 0, 0x1000000, 4);
    __sanitizer_report_simt_ld_b32(memInfo.data(), 0x1000, 0, 0x1000000, 5);
    __sanitizer_report_simt_st_b8(memInfo.data(), 0x1000, 0, 0x1000000, 11);
    __sanitizer_report_simt_st_b16(memInfo.data(), 0x1000, 0, 0x1000000, 11);
    __sanitizer_report_simt_st_b32(memInfo.data(), 0x900, 0, 0x1000000, 10);
    __sanitizer_report_simt_lds_u8(memInfo.data(), 0x1000, 0, 0x10000, 11);
    __sanitizer_report_simt_lds_s8(memInfo.data(), 0x1000, 0, 0x10000, 11);
    __sanitizer_report_simt_lds_u16(memInfo.data(), 0x1000, 0, 0x10000, 11);
    __sanitizer_report_simt_lds_s16(memInfo.data(), 0x1000, 0, 0x10000, 11);
    __sanitizer_report_simt_lds_b32(memInfo.data(), 0x1000, 0, 0x10000, 11);
    __sanitizer_report_simt_lds_b64(memInfo.data(), 0x1000, 0, 0x10000, 11);
    __sanitizer_report_simt_lds_b128(memInfo.data(), 0x1000, 0, 0x10000, 11);
    __sanitizer_report_simt_sts_b8(memInfo.data(), 0x1000, 0, 0x10000, 11);
    __sanitizer_report_simt_sts_b16(memInfo.data(), 0x1000, 0, 0x10000, 11);
    __sanitizer_report_simt_sts_b32(memInfo.data(), 0x1000, 0, 0x10000, 11);
    __sanitizer_report_simt_sts_b64(memInfo.data(), 0x1000, 0, 0x10000, 11);
    __sanitizer_report_simt_sts_b128(memInfo.data(), 0x1000, 0, 0x10000, 11);
    __sanitizer_report_simt_ldk_u8(memInfo.data(), 0x1000, 0, 0x800000, 11);
    __sanitizer_report_simt_ldk_s8(memInfo.data(), 0x1000, 0, 0x800000, 11);
    __sanitizer_report_simt_ldk_u16(memInfo.data(), 0x1000, 0, 0x800000, 11);
    __sanitizer_report_simt_ldk_s16(memInfo.data(), 0x1000, 0, 0x800000, 11);
    __sanitizer_report_simt_ldk_b32(memInfo.data(), 0x1000, 0, 0x800000, 11);
    __sanitizer_report_simt_stk_b8(memInfo.data(), 0x1000, 0, 0x800000, 11);
    __sanitizer_report_simt_stk_b16(memInfo.data(), 0x1000, 0, 0x800000, 11);
    __sanitizer_report_simt_stk_b32(memInfo.data(), 0x1000, 0, 0x800000, 11);
    auto blockHead = reinterpret_cast<SimtRecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead) +
        head.simtInfo.offset);
    ASSERT_EQ(blockHead->recordWriteCount, 30);
    ASSERT_EQ(blockHead->recordCount, 30);
}

TEST(SimtLoadStoreInstructions, dump_simt_red_with_initcheck_expect_get_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    RecordGlobalHead head{};
    head.checkParms.defaultcheck = true;
    head.supportSimt = true;
    head.simtInfo.offset = 1024;
    head.simtInfo.threadByteSize = 1024 * 10;
    head.simtInfo.shadowMemoryOffset = 1024 * 1024 * 8;
    head.simtInfo.shadowMemoryByteSize = 1024 * 1024 * 8;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    ShadowMemoryHeapHead smHeapHead;
    smHeapHead.startAddr = reinterpret_cast<uint64_t>(memInfo.data()) + sizeof(RecordGlobalHead) +
        head.simtInfo.shadowMemoryOffset + sizeof(ShadowMemoryHeapHead);
    smHeapHead.size = head.simtInfo.shadowMemoryByteSize > sizeof(ShadowMemoryHeapHead) ?
        head.simtInfo.shadowMemoryByteSize - sizeof(ShadowMemoryHeapHead) : 0U;
    smHeapHead.current = smHeapHead.startAddr;
    smHeapHead.lock = 0U;
    std::copy_n(reinterpret_cast<uint8_t const*>(&smHeapHead), sizeof(ShadowMemoryHeapHead),
        memInfo.data() + sizeof(RecordGlobalHead) + head.simtInfo.shadowMemoryByteSize);

    __sanitizer_report_red_g_u32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_red_g_s32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_red_g_fp16(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_red_g_fp32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_red_g_f16x2(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_red_g_bf16(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_red_g_bf16x2(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_red_s_u32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_red_s_s32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_red_s_fp16(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_red_s_fp32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_red_s_bf16(memInfo.data(), 0x1000, 0, 0x10000);
    auto blockHead = reinterpret_cast<SimtRecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead) +
        head.simtInfo.offset);
    ASSERT_EQ(blockHead->recordWriteCount, 12);
    ASSERT_EQ(blockHead->recordCount, 12);
}
 
TEST(SimtLoadStoreInstructions, dump_simt_atom_with_initcheck_expect_get_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    RecordGlobalHead head{};
    head.checkParms.defaultcheck = true;
    head.supportSimt = true;
    head.simtInfo.offset = 1024;
    head.simtInfo.threadByteSize = 1024 * 10;
    head.simtInfo.shadowMemoryOffset = 1024 * 1024 * 8;
    head.simtInfo.shadowMemoryByteSize = 1024 * 1024 * 8;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    ShadowMemoryHeapHead smHeapHead;
    smHeapHead.startAddr = reinterpret_cast<uint64_t>(memInfo.data()) + sizeof(RecordGlobalHead) +
        head.simtInfo.shadowMemoryOffset + sizeof(ShadowMemoryHeapHead);
    smHeapHead.size = head.simtInfo.shadowMemoryByteSize > sizeof(ShadowMemoryHeapHead) ?
        head.simtInfo.shadowMemoryByteSize - sizeof(ShadowMemoryHeapHead) : 0U;
    smHeapHead.current = smHeapHead.startAddr;
    smHeapHead.lock = 0U;
    std::copy_n(reinterpret_cast<uint8_t const*>(&smHeapHead), sizeof(ShadowMemoryHeapHead),
        memInfo.data() + sizeof(RecordGlobalHead) + head.simtInfo.shadowMemoryByteSize);

    __sanitizer_report_atom_max_s_s32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_min_s_s32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_add_s_s32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_add_s_u32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_exch_s_u32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_min_s_u32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_cas_s_u32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_max_s_u32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_exch_g_u64(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_min_g_u64(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_add_g_u64(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_cas_g_u64(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_max_g_u64(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_add_g_s64(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_min_g_s64(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_max_g_s64(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_exch_g_u32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_min_g_u32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_cas_g_u32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_max_g_u32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_add_g_u32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_add_g_s32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_min_g_s32(memInfo.data(), 0x1000, 0, 0x10000);
    __sanitizer_report_atom_max_g_s32(memInfo.data(), 0x1000, 0, 0x10000);
    auto blockHead = reinterpret_cast<SimtRecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead) +
        head.simtInfo.offset);
    ASSERT_EQ(blockHead->recordWriteCount, 24);
    ASSERT_EQ(blockHead->recordCount, 24);
}

}
