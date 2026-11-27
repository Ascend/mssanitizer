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

#ifndef ADDR_PROCESS
#define ADDR_PROCESS

#include "sanitizer_report.h"
#include "utils.h"

// SSBUF 仅存在于 Ascend950(dav_3510)。Host UT 不定义 __CCE_IS_AICORE__，需要保留分类用例。
#if !(defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1) || \
    (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3510))
#define SANITIZER_ENABLE_SSBUF 1
#else
#define SANITIZER_ENABLE_SSBUF 0
#endif

namespace Sanitizer {

// Ascend950(dav_3510) 的 SSBUF 窗口：[sysVaBase + 0x40000, sysVaBase + 0x80000)。
// 该窗口紧邻 UB 下方，bit19=0、bit18=1；当前分类逻辑只识别 bit19，会把 SSBUF 误判为 UNKNOWN。
AICORE_FUNC_HEAD bool IsSsBufAddress(uint64_t addr, uint64_t sysVaBase)
{
    const uint64_t ssBufBase = sysVaBase + SSBUF_ADDR_OFFSET;
    return addr >= ssBufBase && addr < ssBufBase + SSBUF_ADDR_WINDOW_SIZE;
}

// 与 RemapAddress 的区别是：只做地址分类，不修改地址（不截断）
AICORE_FUNC_HEAD AddressSpace ClassifyScalarAddress(uint64_t addr, uint64_t sysVaBase) {
    if (GetUintFromConf<48, 25>(addr) != GetUintFromConf<48, 25>(sysVaBase) || GetUintFromConf<24>(addr) == 1) {
        return AddressSpace::GM;
    }
    if (GetUintFromConf<23, 20>(addr) != 0) {
        return AddressSpace::PRIVATE;
    }
#if SANITIZER_ENABLE_SSBUF
    if (IsSsBufAddress(addr, sysVaBase)) {
        return AddressSpace::SSBUF;
    }
#endif
    return GetUintFromConf<19>(addr) == 1 ? AddressSpace::UB : AddressSpace::UNKNOWN;
}

// 参考SU FS资料
AICORE_FUNC_HEAD AddressSpace RemapAddress(uint64_t &addr) {
    uint64_t sysVaBase = GetSysVaBase();
    uint64_t stackPhyBase = GetStackPhyBase();

    if (GetUintFromConf<48, 25>(addr) != GetUintFromConf<48, 25>(sysVaBase) ||
        GetUintFromConf<24>(addr) == 1) {
        addr = GetUintFromConf<47, 0>(addr);
        return AddressSpace::GM;
    }

#if SANITIZER_ENABLE_SSBUF
    /// Ssbuf 与 UB 同属 bits[24:20]==0 的片上窗口：bit19=0/bit18=1 为 SSBUF，bit19=1 为 UB。
    if (GetUintFromConf<48, 25>(addr) == GetUintFromConf<48, 25>(sysVaBase) &&
        GetUintFromConf<24, 20>(addr) == 0x00 && IsSsBufAddress(addr, sysVaBase)) {
        addr -= sysVaBase + SSBUF_ADDR_OFFSET;
        return AddressSpace::SSBUF;
    }
#endif

    /// 有同名函数时，mix算子链接时会优先链接cube.o中的函数，会导致mix算子load/store指令越界，这里需保证vec/cube函数逻辑相同
    if (GetUintFromConf<48, 25>(addr) == GetUintFromConf<48, 25>(sysVaBase) && GetUintFromConf<24, 20>(addr) == 0x00 &&
        GetUintFromConf<19>(addr) == 0x01) {
        return AddressSpace::UB;
    }

    if ((GetUintFromConf<48, 25>(addr) == GetUintFromConf<48, 25>(sysVaBase)) && (GetUintFromConf<24>(addr) == 0) &&
        (GetUintFromConf<23, 20>(addr) != 0x0)) {
        if (GetUintFromConf<48, 25>(sysVaBase) == GetUintFromConf<48, 25>(stackPhyBase)) {
            // stack在UB上
            addr = GetUintFromConf<18, 0>(addr);
        } else {
            // stack在GM上
            addr = GetUintFromConf<23, 0>(addr);  // 首先得到GM地址
        }
        return AddressSpace::PRIVATE;
    }

    return AddressSpace::UNKNOWN;
}

/**
 * @brief SIMT LD ST地址重映射函数，根据地址范围决定内存类型并调整地址偏移
 * @param Rn 源地址寄存器
 * @param offset #ofst
 * @param dataType size_of_data_type
 * @return AddressSpace 地址所属的内存空间类型
 */
AICORE_FUNC_HEAD AddressSpace SimtRemapAddress(uint64_t &Rn, int64_t offset, uint8_t dataType) {
    uint64_t addr = Rn + offset * dataType;
    uint64_t sysVaBase = GetSysVaBase();
#if SANITIZER_ENABLE_SSBUF
    constexpr uint64_t SSBUF_MEM_START = SSBUF_ADDR_OFFSET;                    // SSBUF 起始偏移
    constexpr uint64_t SSBUF_MEM_END = SSBUF_ADDR_OFFSET + SSBUF_ADDR_WINDOW_SIZE; // SSBUF 结束偏移
#endif
    constexpr uint64_t SHARED_MEM_START = 0x80000;      // Shared Memory 起始偏移
    constexpr uint64_t SHARED_MEM_END = 0x100000;       // Shared Memory 结束偏移
    constexpr uint64_t STACK_MEM_START = 0x100000;      // Stack Memory 起始偏移
    constexpr uint64_t STACK_MEM_END = 0x1000000;       // Stack Memory 结束偏移

#if SANITIZER_ENABLE_SSBUF
    // 检查是否属于 SSBUF（标量共享缓冲访问）
    if (addr >= (sysVaBase + SSBUF_MEM_START) && addr < (sysVaBase + SSBUF_MEM_END)) {
        Rn -= (sysVaBase + SSBUF_MEM_START);
        return AddressSpace::SSBUF;
    }
#endif

    // 检查是否属于 Shared Memory（LDS STS访问）
    if (addr >= (sysVaBase + SHARED_MEM_START) && addr < (sysVaBase + SHARED_MEM_END)) {
        Rn -= (sysVaBase + SHARED_MEM_START);  // 调整为 LDS 偏移地址
        return AddressSpace::UB;
    }

    // 检查是否属于 Stack Memory（LDK STK访问）
    if (addr >= (sysVaBase + STACK_MEM_START) && addr < (sysVaBase + STACK_MEM_END)) {
        Rn -= (sysVaBase + STACK_MEM_START);   // 调整为 LDK 偏移地址
        return AddressSpace::PRIVATE;
    }

    // 检查是否属于异常访问
#if SANITIZER_ENABLE_SSBUF
    if (addr >= sysVaBase && addr < (sysVaBase + SSBUF_MEM_START)) {
        return AddressSpace::UNKNOWN;
    }
#else
    if (addr >= sysVaBase && addr < (sysVaBase + SHARED_MEM_START)) {
        return AddressSpace::UNKNOWN;
    }
#endif

    // 其余情况属于 Global Memory（LDG STG访问）
    return AddressSpace::GM;
}

constexpr uint64_t TB_TO_B_MULTIPLIER = 1ULL << 40;
constexpr uint64_t GM_OFFSET_RANGE_MIN = 24 * TB_TO_B_MULTIPLIER;
constexpr uint64_t GM_OFFSET_RANGE_MAX = 40 * TB_TO_B_MULTIPLIER;

// 双页表区间gm地址偏移还原
AICORE_FUNC_HEAD uint64_t GmAddrSubOffset(__gm__ uint8_t *memInfo, MemType memType, uint64_t &addr) {
    if (memType != MemType::GM) {
        return addr;
    }

    auto head = reinterpret_cast<__gm__ RecordGlobalHead *>(memInfo);
    if (addr >= GM_OFFSET_RANGE_MIN && addr <= GM_OFFSET_RANGE_MAX) {
        return addr - head->kernelInfo.l2CacheOffset;
    }

    return addr;
}


} // namespace Sanitizer

#undef SANITIZER_ENABLE_SSBUF

#endif // ADDR_PROCESS
