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

namespace {
std::vector<uint8_t> CreateStackDesignSimtInfo()
{
    auto info = CreateMemInfo();
    auto head = reinterpret_cast<RecordGlobalHead *>(info.data());
    head->checkParms.memcheck = true;
    head->checkParms.initcheck = false;
    head->checkParms.racecheck = false;
    head->checkParms.outOfBoundCheck = false;
    head->supportSimt = true;
    head->offsetInfo.simtErrorInfo.offset = 1024;
    head->offsetInfo.simtErrorInfo.size = 16 * 1024;
    return info;
}

SimtRecordBlockHead *StackDesignSimtHead(std::vector<uint8_t> &info)
{
    return reinterpret_cast<SimtRecordBlockHead *>(info.data() + sizeof(RecordGlobalHead) + 1024);
}

const KernelErrorDesc *StackDesignFirstError(std::vector<uint8_t> &info)
{
    auto bytes = reinterpret_cast<uint8_t *>(StackDesignSimtHead(info) + 1);
    return reinterpret_cast<const KernelErrorDesc *>(
        bytes + sizeof(RecordType) + sizeof(KernelErrorRecord) + sizeof(SimtLoadStoreRecord));
}
struct PrivateProbePair {
    decltype(&__sanitizer_report_simt_ldk_u8) stackProbe;
    decltype(&__sanitizer_report_simt_ld_u8) genericProbe;
    uint64_t width;
    KernelErrorType errorType;
};

const PrivateProbePair PRIVATE_PROBES[] = {
    {__sanitizer_report_simt_ldk_u8, __sanitizer_report_simt_ld_u8, 1, KernelErrorType::ILLEGAL_ADDR_READ},
    {__sanitizer_report_simt_ldk_s8, __sanitizer_report_simt_ld_s8, 1, KernelErrorType::ILLEGAL_ADDR_READ},
    {__sanitizer_report_simt_ldk_u16, __sanitizer_report_simt_ld_u16, 2, KernelErrorType::ILLEGAL_ADDR_READ},
    {__sanitizer_report_simt_ldk_s16, __sanitizer_report_simt_ld_s16, 2, KernelErrorType::ILLEGAL_ADDR_READ},
    {__sanitizer_report_simt_ldk_b32, __sanitizer_report_simt_ld_b32, 4, KernelErrorType::ILLEGAL_ADDR_READ},
    {__sanitizer_report_simt_stk_b8, __sanitizer_report_simt_st_b8, 1, KernelErrorType::ILLEGAL_ADDR_WRITE},
    {__sanitizer_report_simt_stk_b16, __sanitizer_report_simt_st_b16, 2, KernelErrorType::ILLEGAL_ADDR_WRITE},
    {__sanitizer_report_simt_stk_b32, __sanitizer_report_simt_st_b32, 4, KernelErrorType::ILLEGAL_ADDR_WRITE},
};
} // namespace

TEST(SimtLoadStoreInstructions, legal_private_stack_operands_and_generic_pointers_produce_no_errors)
{
    for (const auto &probe : PRIVATE_PROBES) {
        for (uint64_t target : {uint64_t{0}, uint64_t{0x100}, uint64_t{0x8000} - probe.width}) {
            SCOPED_TRACE(testing::Message() << "target=" << target << " width=" << probe.width);
            auto info = CreateStackDesignSimtInfo();
            // LDK/STK 的地址操作数是线程栈内偏移；普通 LD/ST 接收完整指针，先扣除虚拟栈窗口基址。
            probe.stackProbe(info.data(), 0x1000, 0, target, 0);
            probe.genericProbe(info.data(), 0x1000, 0, GetSysVaBase() + 0x100000 + target, 0);
            EXPECT_EQ(StackDesignSimtHead(info)->recordWriteCount, 0U);
            EXPECT_EQ(StackDesignSimtHead(info)->recordCount, 0U);
        }
    }
}

TEST(SimtLoadStoreInstructions, private_stack_operands_and_generic_pointers_agree_at_thread_limit)
{
    for (const auto &probe : PRIVATE_PROBES) {
        for (bool generic : {false, true}) {
            auto info = CreateStackDesignSimtInfo();
            const uint64_t addr = generic ? GetSysVaBase() + 0x108000 : 0x8000;
            auto report = generic ? probe.genericProbe : probe.stackProbe;
            report(info.data(), 0x1000, 0, addr, 0);
            ASSERT_EQ(StackDesignSimtHead(info)->recordWriteCount, 1U);
            ASSERT_EQ(StackDesignSimtHead(info)->recordCount, 1U);
            const auto error = StackDesignFirstError(info);
            EXPECT_EQ(error->space, AddressSpace::PRIVATE);
            EXPECT_EQ(error->errorType, probe.errorType);
            EXPECT_EQ(error->payload.illegalDesc.addr, 0x8000U);
            EXPECT_EQ(error->payload.illegalDesc.illegalSize, probe.width);
        }
    }
}

TEST(SimtLoadStoreInstructions, stack_design_online_gm_left_partial_overlap_is_not_silently_accepted)
{
    auto info = CreateStackDesignSimtInfo();
    auto simd = reinterpret_cast<RecordBlockHead *>(info.data() + sizeof(RecordGlobalHead));
    HostMemoryInfo allocation{0x2001004, 32, MSTX_MEM_PERMISSIONS_REGION_FLAGS_DEFAULT};
    simd->hostMemoryNum = 1;
    simd->hostMemoryInfoPtr = &allocation;
    OnlineCheck check;
    check.Init(info.data(), reinterpret_cast<uint8_t *>(StackDesignSimtHead(info)),
        reinterpret_cast<uint8_t *>(simd), 0);
    SimtLoadStoreRecord record{};
    record.addr = 0x2001000;
    record.size = 8;
    record.space = AddressSpace::GM;
    record.detailedDataType = DetailedDataType::B64;
    check.Process<RecordType::SIMT_LDG>(record);
    ASSERT_EQ(StackDesignSimtHead(info)->recordWriteCount, 1);
    const auto error = StackDesignFirstError(info);
    EXPECT_EQ(error->errorType, KernelErrorType::ILLEGAL_ADDR_READ);
    EXPECT_EQ(error->payload.illegalDesc.illegalSize, 4);
}

TEST(SimtLoadStoreInstructions, stack_design_ub_wrapping_width_cannot_pass_bounds_check)
{
    auto info = CreateStackDesignSimtInfo();
    auto simd = info.data() + sizeof(RecordGlobalHead);
    OnlineCheck check;
    check.Init(info.data(), reinterpret_cast<uint8_t *>(StackDesignSimtHead(info)), simd, 0);
    SimtLoadStoreRecord record{};
    record.addr = 8;
    record.size = UINT64_MAX;
    record.space = AddressSpace::UB;
    record.detailedDataType = DetailedDataType::B64;
    check.Process<RecordType::SIMT_LDS>(record);
    ASSERT_EQ(StackDesignSimtHead(info)->recordWriteCount, 1);
    EXPECT_EQ(StackDesignFirstError(info)->payload.illegalDesc.illegalSize, UINT64_MAX - (253952 - 8));
}

TEST(SimtLoadStoreInstructions, dump_simt_load_store_with_initcheck_expect_get_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    RecordGlobalHead head{};
    head.simtInfo.ubDynamicSize = 0x10000; // 模拟动态 UB 配置；检测使用物理上限，下述 UB 访问仍合法。
    head.checkParms.defaultcheck = true;
    head.checkParms.memcheck = true;
    head.supportSimt = true;
    head.offsetInfo.simtErrorInfo.offset = 1024;
    head.offsetInfo.simtErrorInfo.size = 1024 * 10;
    head.offsetInfo.shadowMemoryInfo.offset = 1024 * 1024 * 8;
    head.offsetInfo.shadowMemoryInfo.size = 1024 * 1024 * 8;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    ShadowMemoryHeapHead smHeapHead;
    smHeapHead.startAddr = reinterpret_cast<uint64_t>(memInfo.data()) + sizeof(RecordGlobalHead) +
        head.offsetInfo.shadowMemoryInfo.offset + sizeof(ShadowMemoryHeapHead);
    smHeapHead.size = head.offsetInfo.shadowMemoryInfo.size > sizeof(ShadowMemoryHeapHead) ?
        head.offsetInfo.shadowMemoryInfo.size - sizeof(ShadowMemoryHeapHead) : 0U;
    smHeapHead.current = smHeapHead.startAddr;
    smHeapHead.lock = 0U;
    std::copy_n(reinterpret_cast<uint8_t const*>(&smHeapHead), sizeof(ShadowMemoryHeapHead),
        memInfo.data() + sizeof(RecordGlobalHead) + head.offsetInfo.shadowMemoryInfo.size);

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
    // 注意ld st桩的addr需要合理设定否则会导致AddressSpace::UNKNOWN无记录
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
    auto blockHead = reinterpret_cast<SimtRecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead) +
        head.offsetInfo.simtErrorInfo.offset);
    // 前面的 10 次 LDG/STG 和 8 次普通 LD/ST 均访问未登记的 GM，共产生 18 条错误；12 次 UB 访问合法。
    ASSERT_EQ(blockHead->recordWriteCount, 18U);
    ASSERT_EQ(blockHead->recordCount, 18U);
    // 以下 8 次 LDK/STK 故意使用 8 MiB 的线程栈偏移，超过 32 KiB，新增 8 条 PRIVATE 越界错误。
    __sanitizer_report_simt_ldk_u8(memInfo.data(), 0x1000, 0, 0x800000, 11);
    __sanitizer_report_simt_ldk_s8(memInfo.data(), 0x1000, 0, 0x800000, 11);
    __sanitizer_report_simt_ldk_u16(memInfo.data(), 0x1000, 0, 0x800000, 11);
    __sanitizer_report_simt_ldk_s16(memInfo.data(), 0x1000, 0, 0x800000, 11);
    __sanitizer_report_simt_ldk_b32(memInfo.data(), 0x1000, 0, 0x800000, 11);
    __sanitizer_report_simt_stk_b8(memInfo.data(), 0x1000, 0, 0x800000, 11);
    __sanitizer_report_simt_stk_b16(memInfo.data(), 0x1000, 0, 0x800000, 11);
    __sanitizer_report_simt_stk_b32(memInfo.data(), 0x1000, 0, 0x800000, 11);
    ASSERT_EQ(blockHead->recordWriteCount, 18U + 8U);
    ASSERT_EQ(blockHead->recordCount, 18U + 8U);
}

TEST(SimtLoadStoreInstructions, dump_simt_red_with_initcheck_expect_get_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    RecordGlobalHead head{};
    head.simtInfo.ubDynamicSize = 0x10000; // 显式指定 UB 大小，保持 UB 访问(addr>=0x10000)越界，符合原期望
    head.checkParms.defaultcheck = true;
    head.checkParms.memcheck = true;
    head.supportSimt = true;
    head.offsetInfo.simtErrorInfo.offset = 1024;
    head.offsetInfo.simtErrorInfo.size = 1024 * 10;
    head.offsetInfo.shadowMemoryInfo.offset = 1024 * 1024 * 8;
    head.offsetInfo.shadowMemoryInfo.size = 1024 * 1024 * 8;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    ShadowMemoryHeapHead smHeapHead;
    smHeapHead.startAddr = reinterpret_cast<uint64_t>(memInfo.data()) + sizeof(RecordGlobalHead) +
        head.offsetInfo.shadowMemoryInfo.offset + sizeof(ShadowMemoryHeapHead);
    smHeapHead.size = head.offsetInfo.shadowMemoryInfo.size > sizeof(ShadowMemoryHeapHead) ?
        head.offsetInfo.shadowMemoryInfo.size - sizeof(ShadowMemoryHeapHead) : 0U;
    smHeapHead.current = smHeapHead.startAddr;
    smHeapHead.lock = 0U;
    std::copy_n(reinterpret_cast<uint8_t const*>(&smHeapHead), sizeof(ShadowMemoryHeapHead),
        memInfo.data() + sizeof(RecordGlobalHead) + head.offsetInfo.shadowMemoryInfo.size);

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
        head.offsetInfo.simtErrorInfo.offset);
    ASSERT_EQ(blockHead->recordWriteCount, 7);
    ASSERT_EQ(blockHead->recordCount, 7);
}

TEST(SimtLoadStoreInstructions, dump_simt_atom_with_initcheck_expect_get_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    RecordGlobalHead head{};
    head.simtInfo.ubDynamicSize = 0x10000; // 显式指定 UB 大小，保持 UB 访问(addr>=0x10000)越界，符合原期望
    head.checkParms.defaultcheck = true;
    head.checkParms.memcheck = true;
    head.supportSimt = true;
    head.offsetInfo.simtErrorInfo.offset = 1024;
    head.offsetInfo.simtErrorInfo.size = 1024 * 10;
    head.offsetInfo.shadowMemoryInfo.offset = 1024 * 1024 * 8;
    head.offsetInfo.shadowMemoryInfo.size = 1024 * 1024 * 8;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    ShadowMemoryHeapHead smHeapHead;
    smHeapHead.startAddr = reinterpret_cast<uint64_t>(memInfo.data()) + sizeof(RecordGlobalHead) +
        head.offsetInfo.shadowMemoryInfo.offset + sizeof(ShadowMemoryHeapHead);
    smHeapHead.size = head.offsetInfo.shadowMemoryInfo.size > sizeof(ShadowMemoryHeapHead) ?
        head.offsetInfo.shadowMemoryInfo.size - sizeof(ShadowMemoryHeapHead) : 0U;
    smHeapHead.current = smHeapHead.startAddr;
    smHeapHead.lock = 0U;
    std::copy_n(reinterpret_cast<uint8_t const*>(&smHeapHead), sizeof(ShadowMemoryHeapHead),
        memInfo.data() + sizeof(RecordGlobalHead) + head.offsetInfo.shadowMemoryInfo.size);

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
        head.offsetInfo.simtErrorInfo.offset);
    ASSERT_EQ(blockHead->recordWriteCount, 16);
    ASSERT_EQ(blockHead->recordCount, 16);
}

}
