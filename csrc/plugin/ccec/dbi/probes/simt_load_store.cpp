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


#include "core/framework/record_defs.h"

#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
#define SIMT_MODE  // 开启simt模式
#include "plugin/record_simt_instructions.h"
#include "plugin/addr_process.h"
using namespace Sanitizer;

SANITIZER_REPORT(simt_ldg_u8, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDG, DetailedDataType::U8>(EXTRA_PARAMS, AddressSpace::GM,
                                                                         addr, offset);
}

SANITIZER_REPORT(simt_ldg_s8, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDG, DetailedDataType::S8>(EXTRA_PARAMS, AddressSpace::GM,
                                                                         addr, offset);
}

SANITIZER_REPORT(simt_ldg_u16, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDG, DetailedDataType::U16>(EXTRA_PARAMS, AddressSpace::GM,
                                                                          addr, offset);
}

SANITIZER_REPORT(simt_ldg_s16, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDG, DetailedDataType::S16>(EXTRA_PARAMS, AddressSpace::GM,
                                                                          addr, offset);
}

SANITIZER_REPORT(simt_ldg_b32, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDG, DetailedDataType::B32>(EXTRA_PARAMS, AddressSpace::GM,
                                                                          addr, offset);
}

SANITIZER_REPORT(simt_ldg_b64, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDG, DetailedDataType::B64>(EXTRA_PARAMS, AddressSpace::GM,
                                                                          addr, offset);
}

SANITIZER_REPORT(simt_ldg_b128, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDG, DetailedDataType::B128>(EXTRA_PARAMS, AddressSpace::GM,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_stg_b8, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_STG, DetailedDataType::B8>(EXTRA_PARAMS, AddressSpace::GM,
                                                                         addr, offset);
}

SANITIZER_REPORT(simt_stg_b16, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_STG, DetailedDataType::B16>(EXTRA_PARAMS, AddressSpace::GM,
                                                                          addr, offset);
}

SANITIZER_REPORT(simt_stg_b32, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_STG, DetailedDataType::B32>(EXTRA_PARAMS, AddressSpace::GM,
                                                                          addr, offset);
}

SANITIZER_REPORT(simt_stg_b64, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_STG, DetailedDataType::B64>(EXTRA_PARAMS, AddressSpace::GM,
                                                                          addr, offset);
}

SANITIZER_REPORT(simt_stg_b128, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_STG, DetailedDataType::B128>(EXTRA_PARAMS, AddressSpace::GM,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_ld_u8, uint64_t addr, int64_t offset)
{
    AddressSpace space = SimtRemapAddress(addr, offset, sizeof(uint8_t));
    SimtRecordLoadStoreEvent<RecordType::SIMT_LD, DetailedDataType::U8>(EXTRA_PARAMS, space,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_ld_s8, uint64_t addr, int64_t offset)
{
    AddressSpace space = SimtRemapAddress(addr, offset, sizeof(int8_t));
    SimtRecordLoadStoreEvent<RecordType::SIMT_LD, DetailedDataType::S8>(EXTRA_PARAMS, space,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_ld_u16, uint64_t addr, int64_t offset)
{
    AddressSpace space = SimtRemapAddress(addr, offset, sizeof(uint16_t));
    SimtRecordLoadStoreEvent<RecordType::SIMT_LD, DetailedDataType::U16>(EXTRA_PARAMS, space,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_ld_s16, uint64_t addr, int64_t offset)
{
    AddressSpace space = SimtRemapAddress(addr, offset, sizeof(int16_t));
    SimtRecordLoadStoreEvent<RecordType::SIMT_LD, DetailedDataType::S16>(EXTRA_PARAMS, space,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_ld_b32, uint64_t addr, int64_t offset)
{
    AddressSpace space = SimtRemapAddress(addr, offset, sizeof(int32_t));
    SimtRecordLoadStoreEvent<RecordType::SIMT_LD, DetailedDataType::B32>(EXTRA_PARAMS, space,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_st_b8, uint64_t addr, int64_t offset)
{
    AddressSpace space = SimtRemapAddress(addr, offset, sizeof(int8_t));
    SimtRecordLoadStoreEvent<RecordType::SIMT_ST, DetailedDataType::B8>(EXTRA_PARAMS, space,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_st_b16, uint64_t addr, int64_t offset)
{
    AddressSpace space = SimtRemapAddress(addr, offset, sizeof(int16_t));
    SimtRecordLoadStoreEvent<RecordType::SIMT_ST, DetailedDataType::B16>(EXTRA_PARAMS, space,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_st_b32, uint64_t addr, int64_t offset)
{
    AddressSpace space = SimtRemapAddress(addr, offset, sizeof(int32_t));
    SimtRecordLoadStoreEvent<RecordType::SIMT_ST, DetailedDataType::B32>(EXTRA_PARAMS, space,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_lds_u8, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDS, DetailedDataType::U8>(EXTRA_PARAMS, AddressSpace::UB,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_lds_s8, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDS, DetailedDataType::S8>(EXTRA_PARAMS, AddressSpace::UB,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_lds_u16, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDS, DetailedDataType::U16>(EXTRA_PARAMS, AddressSpace::UB,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_lds_s16, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDS, DetailedDataType::S16>(EXTRA_PARAMS, AddressSpace::UB,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_lds_b32, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDS, DetailedDataType::B32>(EXTRA_PARAMS, AddressSpace::UB,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_lds_b64, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDS, DetailedDataType::B64>(EXTRA_PARAMS, AddressSpace::UB,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_lds_b128, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDS, DetailedDataType::B128>(EXTRA_PARAMS, AddressSpace::UB,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_sts_b8, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_STS, DetailedDataType::B8>(EXTRA_PARAMS, AddressSpace::UB,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_sts_b16, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_STS, DetailedDataType::B16>(EXTRA_PARAMS, AddressSpace::UB,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_sts_b32, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_STS, DetailedDataType::B32>(EXTRA_PARAMS, AddressSpace::UB,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_sts_b64, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_STS, DetailedDataType::B64>(EXTRA_PARAMS, AddressSpace::UB,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_sts_b128, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_STS, DetailedDataType::B128>(EXTRA_PARAMS, AddressSpace::UB,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_ldk_u8, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDK, DetailedDataType::U8>(EXTRA_PARAMS, AddressSpace::PRIVATE,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_ldk_s8, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDK, DetailedDataType::S8>(EXTRA_PARAMS, AddressSpace::PRIVATE,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_ldk_u16, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDK, DetailedDataType::U16>(EXTRA_PARAMS, AddressSpace::PRIVATE,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_ldk_s16, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDK, DetailedDataType::S16>(EXTRA_PARAMS, AddressSpace::PRIVATE,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_ldk_b32, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_LDK, DetailedDataType::B32>(EXTRA_PARAMS, AddressSpace::PRIVATE,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_stk_b8, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_STK, DetailedDataType::B8>(EXTRA_PARAMS, AddressSpace::PRIVATE,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_stk_b16, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_STK, DetailedDataType::B16>(EXTRA_PARAMS, AddressSpace::PRIVATE,
                                                                           addr, offset);
}

SANITIZER_REPORT(simt_stk_b32, uint64_t addr, int64_t offset)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_STK, DetailedDataType::B32>(EXTRA_PARAMS, AddressSpace::PRIVATE,
                                                                           addr, offset);
}

SANITIZER_REPORT(atom_max_s_s32, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::S32>(EXTRA_PARAMS, AddressSpace::UB, addr, SimtAtomMode::MAX);
}
 
SANITIZER_REPORT(atom_min_s_s32, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::S32>(EXTRA_PARAMS, AddressSpace::UB, addr, SimtAtomMode::MIN);
}
 
SANITIZER_REPORT(atom_add_s_s32, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::S32>(EXTRA_PARAMS, AddressSpace::UB, addr, SimtAtomMode::ADD);
}
 
SANITIZER_REPORT(atom_add_s_u32, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::U32>(EXTRA_PARAMS, AddressSpace::UB, addr, SimtAtomMode::ADD);
}
 
SANITIZER_REPORT(atom_exch_s_u32, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::U32>(EXTRA_PARAMS, AddressSpace::UB, addr, SimtAtomMode::EXCH);
}
 
SANITIZER_REPORT(atom_min_s_u32, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::U32>(EXTRA_PARAMS, AddressSpace::UB, addr, SimtAtomMode::MIN);
}
 
SANITIZER_REPORT(atom_cas_s_u32, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::U32>(EXTRA_PARAMS, AddressSpace::UB, addr, SimtAtomMode::CAS);
}
 
SANITIZER_REPORT(atom_max_s_u32, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::U32>(EXTRA_PARAMS, AddressSpace::UB, addr, SimtAtomMode::MAX);
}
 
SANITIZER_REPORT(atom_exch_g_u64, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::U64>(EXTRA_PARAMS, AddressSpace::GM, addr, SimtAtomMode::EXCH);
}
 
SANITIZER_REPORT(atom_min_g_u64, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::U64>(EXTRA_PARAMS, AddressSpace::GM, addr, SimtAtomMode::MIN);
}
 
SANITIZER_REPORT(atom_add_g_u64, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::U64>(EXTRA_PARAMS, AddressSpace::GM, addr, SimtAtomMode::ADD);
}
 
SANITIZER_REPORT(atom_cas_g_u64, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::U64>(EXTRA_PARAMS, AddressSpace::GM, addr, SimtAtomMode::CAS);
}
 
SANITIZER_REPORT(atom_max_g_u64, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::U64>(EXTRA_PARAMS, AddressSpace::GM, addr, SimtAtomMode::MAX);
}
 
SANITIZER_REPORT(atom_add_g_s64, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::S64>(EXTRA_PARAMS, AddressSpace::GM, addr, SimtAtomMode::ADD);
}
 
SANITIZER_REPORT(atom_min_g_s64, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::S64>(EXTRA_PARAMS, AddressSpace::GM, addr, SimtAtomMode::MIN);
}
 
SANITIZER_REPORT(atom_max_g_s64, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::S64>(EXTRA_PARAMS, AddressSpace::GM, addr, SimtAtomMode::MAX);
}
 
SANITIZER_REPORT(atom_exch_g_u32, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::U32>(EXTRA_PARAMS, AddressSpace::GM, addr, SimtAtomMode::EXCH);
}
 
SANITIZER_REPORT(atom_min_g_u32, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::U32>(EXTRA_PARAMS, AddressSpace::GM, addr, SimtAtomMode::MIN);
}
 
SANITIZER_REPORT(atom_cas_g_u32, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::U32>(EXTRA_PARAMS, AddressSpace::GM, addr, SimtAtomMode::CAS);
}
 
SANITIZER_REPORT(atom_max_g_u32, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::U32>(EXTRA_PARAMS, AddressSpace::GM, addr, SimtAtomMode::MAX);
}
 
SANITIZER_REPORT(atom_add_g_u32, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::U32>(EXTRA_PARAMS, AddressSpace::GM, addr, SimtAtomMode::ADD);
}
 
SANITIZER_REPORT(atom_add_g_s32, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::S32>(EXTRA_PARAMS, AddressSpace::GM, addr, SimtAtomMode::ADD);
}
 
SANITIZER_REPORT(atom_min_g_s32, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::S32>(EXTRA_PARAMS, AddressSpace::GM, addr, SimtAtomMode::MIN);
}
 
SANITIZER_REPORT(atom_max_g_s32, uint64_t addr)
{
    SimtRecordAtomEvent<RecordType::SIMT_ATOM, DetailedDataType::S32>(EXTRA_PARAMS, AddressSpace::GM, addr, SimtAtomMode::MAX);
}
 
SANITIZER_REPORT(red_g_u32, uint64_t addr)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_RED, DetailedDataType::U32>(EXTRA_PARAMS, AddressSpace::GM, addr, 0);
}
 
SANITIZER_REPORT(red_g_s32, uint64_t addr)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_RED, DetailedDataType::S32>(EXTRA_PARAMS, AddressSpace::GM, addr, 0);
}
 
SANITIZER_REPORT(red_g_fp16, uint64_t addr)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_RED, DetailedDataType::F16>(EXTRA_PARAMS, AddressSpace::GM, addr, 0);
}
 
SANITIZER_REPORT(red_g_fp32, uint64_t addr)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_RED, DetailedDataType::FLOAT>(EXTRA_PARAMS, AddressSpace::GM, addr, 0);
}
 
SANITIZER_REPORT(red_g_f16x2, uint64_t addr)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_RED, DetailedDataType::F16X2>(EXTRA_PARAMS, AddressSpace::GM, addr, 0);
}
 
SANITIZER_REPORT(red_g_bf16, uint64_t addr)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_RED, DetailedDataType::BF16>(EXTRA_PARAMS, AddressSpace::GM, addr, 0);
}
 
SANITIZER_REPORT(red_g_bf16x2, uint64_t addr)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_RED, DetailedDataType::BF16X2>(EXTRA_PARAMS, AddressSpace::GM, addr, 0);
}
 
SANITIZER_REPORT(red_s_u32, uint64_t addr)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_RED, DetailedDataType::U32>(EXTRA_PARAMS, AddressSpace::UB, addr, 0);
}
 
SANITIZER_REPORT(red_s_s32, uint64_t addr)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_RED, DetailedDataType::S32>(EXTRA_PARAMS, AddressSpace::UB, addr, 0);
}
 
SANITIZER_REPORT(red_s_fp16, uint64_t addr)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_RED, DetailedDataType::F16>(EXTRA_PARAMS, AddressSpace::UB, addr, 0);
}
 
SANITIZER_REPORT(red_s_fp32, uint64_t addr)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_RED, DetailedDataType::F32>(EXTRA_PARAMS, AddressSpace::UB, addr, 0);
}
 
SANITIZER_REPORT(red_s_bf16, uint64_t addr)
{
    SimtRecordLoadStoreEvent<RecordType::SIMT_RED, DetailedDataType::BF16>(EXTRA_PARAMS, AddressSpace::UB, addr, 0);
}

#undef SIMT_MODE
#else  // NOT C310
#include "plugin/utils.h"
using namespace Sanitizer;

SANITIZER_REPORT(simt_ldg_u8, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_ldg_s8, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_ldg_u16, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_ldg_s16, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_ldg_b32, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_ldg_b64, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_ldg_b128, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_stg_b8, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_stg_b16, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_stg_b32, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_stg_b64, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_stg_b128, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_ld_u8, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_ld_s8, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_ld_u16, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_ld_s16, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_ld_b32, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_st_b8, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_st_b16, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_st_b32, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_lds_u8, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_lds_s8, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_lds_u16, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_lds_s16, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_lds_b32, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_lds_b64, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_lds_b128, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_sts_b8, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_sts_b16, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_sts_b32, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_sts_b64, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_sts_b128, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_ldk_u8, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}


SANITIZER_REPORT(simt_ldk_s8, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_ldk_u16, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_ldk_s16, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_ldk_b32, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_stk_b8, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_stk_b16, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(simt_stk_b32, uint64_t addr, int64_t offset)
{
    (void)addr;
    (void)offset;
}

SANITIZER_REPORT(atom_max_s_s32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_min_s_s32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_add_s_s32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_add_s_u32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_exch_s_u32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_min_s_u32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_cas_s_u32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_max_s_u32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_exch_g_u64, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_min_g_u64, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_add_g_u64, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_cas_g_u64, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_max_g_u64, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_add_g_s64, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_min_g_s64, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_max_g_s64, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_exch_g_u32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_min_g_u32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_cas_g_u32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_max_g_u32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_add_g_u32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_add_g_s32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_min_g_s32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(atom_max_g_s32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(red_g_u32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(red_g_s32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(red_g_fp16, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(red_g_fp32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(red_g_f16x2, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(red_g_bf16, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(red_g_bf16x2, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(red_s_u32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(red_s_s32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(red_s_fp16, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(red_s_fp32, uint64_t addr)
{
    (void)addr;
}
 
SANITIZER_REPORT(red_s_bf16, uint64_t addr)
{
    (void)addr;
}
#endif