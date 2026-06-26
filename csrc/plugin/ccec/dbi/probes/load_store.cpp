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
#include "plugin/record_move_instructions.h"
#include "plugin/utils.h"
#include "plugin/addr_process.h"
#include "plugin/record_mstx_instructions.h"

using namespace Sanitizer;

namespace {

AICORE_FUNC_HEAD uint64_t UpdateAddrByOffset(uint64_t addr, int64_t offset, uint64_t post)
{
    return post == 1UL ? addr : addr + offset;
}

} // namespace [Dummy]

SANITIZER_REPORT(__mstx_dfx_report_stub, uint32_t interfaceId, uint32_t bufferLens, void *buffer) {
    RecordMstxEvent(EXTRA_PARAMS, interfaceId, bufferLens, buffer);
}

SANITIZER_REPORT(st_b64_imm, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset, post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::ST_IO, DetailedDataType::B64>(EXTRA_PARAMS, space, addr, sizeof(uint64_t), sizeof(uint64_t));
}

SANITIZER_REPORT(st_b64_reg, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset * sizeof(uint64_t), post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::ST, DetailedDataType::B64>(EXTRA_PARAMS, space, addr, sizeof(uint64_t), sizeof(uint64_t));
}

SANITIZER_REPORT(st_b32_imm, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset, post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::ST_IO, DetailedDataType::B32>(EXTRA_PARAMS, space, addr, sizeof(uint32_t), sizeof(uint32_t));
}

SANITIZER_REPORT(st_b32_reg, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset * sizeof(uint32_t), post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::ST, DetailedDataType::B32>(EXTRA_PARAMS, space, addr, sizeof(uint32_t), sizeof(uint32_t));
}

SANITIZER_REPORT(st_b16_imm, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset, post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::ST_IO, DetailedDataType::B16>(EXTRA_PARAMS, space, addr, sizeof(uint16_t), sizeof(uint16_t));
}

SANITIZER_REPORT(st_b16_reg, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset * sizeof(uint16_t), post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::ST, DetailedDataType::B16>(EXTRA_PARAMS, space, addr, sizeof(uint16_t), sizeof(uint16_t));
}

SANITIZER_REPORT(st_b8_imm, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset, post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::ST_IO, DetailedDataType::B8>(EXTRA_PARAMS, space, addr, sizeof(uint8_t), sizeof(uint8_t));
}

SANITIZER_REPORT(st_b8_reg, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset * sizeof(uint8_t), post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::ST, DetailedDataType::B8>(EXTRA_PARAMS, space, addr, sizeof(uint8_t), sizeof(uint8_t));
}

SANITIZER_REPORT(stp_b64, uint64_t addr, int64_t offset)
{
    addr += offset;
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::STP, DetailedDataType::B64>(EXTRA_PARAMS, space, addr, sizeof(uint64_t) * 2, sizeof(uint64_t));
}

SANITIZER_REPORT(stp_b32, uint64_t addr, int64_t offset)
{
    addr += offset;
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::STP, DetailedDataType::B32>(EXTRA_PARAMS, space, addr, sizeof(uint32_t) * 2, sizeof(uint32_t));
}

SANITIZER_REPORT(stp_b16, uint64_t addr, int64_t offset)
{
    addr += offset;
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::STP, DetailedDataType::B16>(EXTRA_PARAMS, space, addr, sizeof(uint16_t) * 2, sizeof(uint16_t));
}

SANITIZER_REPORT(stp_b8, uint64_t addr, int64_t offset)
{
    addr += offset;
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::STP, DetailedDataType::B8>(EXTRA_PARAMS, space, addr, sizeof(uint8_t) * 2, sizeof(uint8_t));
}

SANITIZER_REPORT(sti_b64_imm, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset, post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::STI_IO, DetailedDataType::B64>(EXTRA_PARAMS, space, addr, sizeof(uint64_t), sizeof(uint64_t));
}

SANITIZER_REPORT(sti_b32_imm, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset, post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::STI_IO, DetailedDataType::B32>(EXTRA_PARAMS, space, addr, sizeof(uint32_t), sizeof(uint32_t));
}

SANITIZER_REPORT(sti_b16_imm, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset, post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::STI_IO, DetailedDataType::B16>(EXTRA_PARAMS, space, addr, sizeof(uint16_t), sizeof(uint16_t));
}
SANITIZER_REPORT(sti_b8_imm, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset, post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::STI_IO, DetailedDataType::B8>(EXTRA_PARAMS, space, addr, sizeof(uint8_t), sizeof(uint8_t));
}

SANITIZER_REPORT(sti_b64_reg, uint64_t addr, int64_t offset, uint64_t post)
{
    size_t typeBytes = sizeof(uint64_t);
    addr = UpdateAddrByOffset(addr, offset * sizeof(uint64_t), post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::STI, DetailedDataType::B64>(EXTRA_PARAMS, space, addr, sizeof(uint64_t), sizeof(uint64_t));
}

SANITIZER_REPORT(sti_b32_reg, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset * sizeof(uint32_t), post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::STI, DetailedDataType::B32>(EXTRA_PARAMS, space, addr, sizeof(uint32_t), sizeof(uint32_t));
}

SANITIZER_REPORT(sti_b16_reg, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset * sizeof(uint16_t), post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::STI, DetailedDataType::B16>(EXTRA_PARAMS, space, addr, sizeof(uint16_t), sizeof(uint16_t));
}
SANITIZER_REPORT(sti_b8_reg, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset * sizeof(uint8_t), post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::STI, DetailedDataType::B8>(EXTRA_PARAMS, space, addr, sizeof(uint8_t), sizeof(uint8_t));
}

SANITIZER_REPORT(ld_b64_imm, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset, post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::LD_IO, DetailedDataType::B64>(EXTRA_PARAMS, space, addr, sizeof(uint64_t), sizeof(uint64_t));
}

SANITIZER_REPORT(ld_b32_imm, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset, post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::LD_IO, DetailedDataType::B32>(EXTRA_PARAMS, space, addr, sizeof(uint32_t), sizeof(uint32_t));
}

SANITIZER_REPORT(ld_b16_imm, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset, post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::LD_IO, DetailedDataType::B16>(EXTRA_PARAMS, space, addr, sizeof(uint16_t), sizeof(uint16_t));
}

SANITIZER_REPORT(ld_b8_imm, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset, post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::LD_IO, DetailedDataType::B8>(EXTRA_PARAMS, space, addr, sizeof(uint8_t), sizeof(uint8_t));
}

SANITIZER_REPORT(ld_b64_reg, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset * sizeof(uint64_t), post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::LD, DetailedDataType::B64>(EXTRA_PARAMS, space, addr, sizeof(uint64_t), sizeof(uint64_t));
}

SANITIZER_REPORT(ld_b32_reg, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset * sizeof(uint32_t), post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::LD, DetailedDataType::B32>(EXTRA_PARAMS, space, addr, sizeof(uint32_t), sizeof(uint32_t));
}

SANITIZER_REPORT(ld_b16_reg, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset * sizeof(uint16_t), post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::LD, DetailedDataType::B16>(EXTRA_PARAMS, space, addr, sizeof(uint16_t), sizeof(uint16_t));
}

SANITIZER_REPORT(ld_b8_reg, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset * sizeof(uint8_t), post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::LD, DetailedDataType::B8>(EXTRA_PARAMS, space, addr, sizeof(uint8_t), sizeof(uint8_t));
}

SANITIZER_REPORT(ldp_b64, uint64_t addr, int64_t offset)
{
    addr += offset;
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::LDP, DetailedDataType::B64>(EXTRA_PARAMS, space, addr, sizeof(uint64_t) * 2, sizeof(uint64_t));
}

SANITIZER_REPORT(ldp_b32, uint64_t addr, int64_t offset)
{
    addr += offset;
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::LDP, DetailedDataType::B32>(EXTRA_PARAMS, space, addr, sizeof(uint32_t) * 2, sizeof(uint32_t));
}

SANITIZER_REPORT(ldp_b16, uint64_t addr, int64_t offset)
{
    addr += offset;
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::LDP, DetailedDataType::B16>(EXTRA_PARAMS, space, addr, sizeof(uint16_t) * 2, sizeof(uint16_t));
}

SANITIZER_REPORT(ldp_b8, uint64_t addr, int64_t offset)
{
    addr += offset;
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::LDP, DetailedDataType::B8>(EXTRA_PARAMS, space, addr, sizeof(uint8_t) * 2, sizeof(uint8_t));
}

SANITIZER_REPORT(st_atomic_b32, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset * sizeof(uint32_t), post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::ST_ATOMIC, DetailedDataType::B32>(EXTRA_PARAMS, space, addr, sizeof(uint32_t), sizeof(uint32_t));
}

SANITIZER_REPORT(st_atomic_b16, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset * sizeof(uint16_t), post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::ST_ATOMIC, DetailedDataType::B16>(EXTRA_PARAMS, space, addr, sizeof(uint16_t), sizeof(uint16_t));
}

SANITIZER_REPORT(st_atomic_b8, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset * sizeof(uint8_t), post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::ST_ATOMIC, DetailedDataType::B8>(EXTRA_PARAMS, space, addr, sizeof(uint8_t), sizeof(uint8_t));
}

SANITIZER_REPORT(sti_atomic_b32, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset, post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::STI_ATOMIC, DetailedDataType::B32>(EXTRA_PARAMS, space, addr, sizeof(uint32_t), sizeof(uint32_t));
}

SANITIZER_REPORT(sti_atomic_b16, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset, post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::STI_ATOMIC, DetailedDataType::B16>(EXTRA_PARAMS, space, addr, sizeof(uint16_t), sizeof(uint16_t));
}

SANITIZER_REPORT(sti_atomic_b8, uint64_t addr, int64_t offset, uint64_t post)
{
    addr = UpdateAddrByOffset(addr, offset, post);
    AddressSpace space = RemapAddress(addr);
    RecordLoadStoreEvent<RecordType::STI_ATOMIC, DetailedDataType::B8>(EXTRA_PARAMS, space, addr, sizeof(uint8_t), sizeof(uint8_t));
}

SANITIZER_REPORT(st_dev_b64, uint64_t addr, int64_t offset)
{
    addr = addr + offset;
    RecordLoadStoreEvent<RecordType::ST_DEV, DetailedDataType::B64>(EXTRA_PARAMS, AddressSpace::GM, addr, sizeof(uint64_t), sizeof(uint64_t));
}

SANITIZER_REPORT(st_dev_b32, uint64_t addr, int64_t offset)
{
    addr = addr + offset;
    RecordLoadStoreEvent<RecordType::ST_DEV, DetailedDataType::B32>(EXTRA_PARAMS, AddressSpace::GM, addr, sizeof(uint32_t), sizeof(uint32_t));
}

SANITIZER_REPORT(st_dev_b16, uint64_t addr, int64_t offset)
{
    addr = addr + offset;
    RecordLoadStoreEvent<RecordType::ST_DEV, DetailedDataType::B16>(EXTRA_PARAMS, AddressSpace::GM, addr, sizeof(uint16_t), sizeof(uint16_t));
}

SANITIZER_REPORT(st_dev_b8, uint64_t addr, int64_t offset)
{
    addr = addr + offset;
    RecordLoadStoreEvent<RecordType::ST_DEV, DetailedDataType::B8>(EXTRA_PARAMS, AddressSpace::GM, addr, sizeof(uint8_t), sizeof(uint8_t));
}

SANITIZER_REPORT(ld_dev_b64, uint64_t addr, int64_t offset)
{
    addr = addr + offset;
    RecordLoadStoreEvent<RecordType::LD_DEV, DetailedDataType::B64>(EXTRA_PARAMS, AddressSpace::GM, addr, sizeof(uint64_t), sizeof(uint64_t));
}

SANITIZER_REPORT(ld_dev_b32, uint64_t addr, int64_t offset)
{
    addr = addr + offset;
    RecordLoadStoreEvent<RecordType::LD_DEV, DetailedDataType::B32>(EXTRA_PARAMS, AddressSpace::GM, addr, sizeof(uint32_t), sizeof(uint32_t));
}

SANITIZER_REPORT(ld_dev_b16, uint64_t addr, int64_t offset)
{
    addr = addr + offset;
    RecordLoadStoreEvent<RecordType::LD_DEV, DetailedDataType::B16>(EXTRA_PARAMS, AddressSpace::GM, addr, sizeof(uint16_t), sizeof(uint16_t));
}

SANITIZER_REPORT(ld_dev_b8, uint64_t addr, int64_t offset)
{
    addr = addr + offset;
    RecordLoadStoreEvent<RecordType::LD_DEV, DetailedDataType::B8>(EXTRA_PARAMS, AddressSpace::GM, addr, sizeof(uint8_t), sizeof(uint8_t));
}

SANITIZER_REPORT(scalar_atom_u32, uint64_t addr)
{
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
    RecordScalarRedAndAtomEvent<RecordType::SCALAR_ATOM, DetailedDataType::U32>(EXTRA_PARAMS, addr, sizeof(uint32_t));
#else
    (void)addr;
#endif
}

SANITIZER_REPORT(scalar_atom_s32, uint64_t addr)
{
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
    RecordScalarRedAndAtomEvent<RecordType::SCALAR_ATOM, DetailedDataType::S32>(EXTRA_PARAMS, addr, sizeof(int32_t));
#else
    (void)addr;
#endif
}

SANITIZER_REPORT(scalar_atom_u64, uint64_t addr)
{
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
    RecordScalarRedAndAtomEvent<RecordType::SCALAR_ATOM, DetailedDataType::U64>(EXTRA_PARAMS, addr, sizeof(uint64_t));
#else
    (void)addr;
#endif
}

SANITIZER_REPORT(scalar_atom_s64, uint64_t addr)
{
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
    RecordScalarRedAndAtomEvent<RecordType::SCALAR_ATOM, DetailedDataType::S64>(EXTRA_PARAMS, addr, sizeof(int64_t));
#else
    (void)addr;
#endif
}

SANITIZER_REPORT(scalar_red_u32, uint64_t addr)
{
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
    RecordScalarRedAndAtomEvent<RecordType::SCALAR_RED, DetailedDataType::U32>(EXTRA_PARAMS, addr, sizeof(uint32_t));
#else
    (void)addr;
#endif
}

SANITIZER_REPORT(scalar_red_s32, uint64_t addr)
{
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
    RecordScalarRedAndAtomEvent<RecordType::SCALAR_RED, DetailedDataType::S32>(EXTRA_PARAMS, addr, sizeof(int32_t));
#else
    (void)addr;
#endif
}

SANITIZER_REPORT(scalar_red_f16, uint64_t addr)
{
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
    RecordScalarRedAndAtomEvent<RecordType::SCALAR_RED, DetailedDataType::F16>(EXTRA_PARAMS, addr, sizeof(half));
#else
    (void)addr;
#endif
}

SANITIZER_REPORT(scalar_red_bf16, uint64_t addr)
{
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
    RecordScalarRedAndAtomEvent<RecordType::SCALAR_RED, DetailedDataType::BF16>(EXTRA_PARAMS, addr, sizeof(__bf16));
#else
    (void)addr;
#endif
}

SANITIZER_REPORT(scalar_red_f32, uint64_t addr)
{
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
    RecordScalarRedAndAtomEvent<RecordType::SCALAR_RED, DetailedDataType::FLOAT>(EXTRA_PARAMS, addr, sizeof(float));
#else
    (void)addr;
#endif
}
