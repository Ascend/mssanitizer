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


#ifndef ADDRESS_SANITIZER_SHADOW_MEMORY_H
#define ADDRESS_SANITIZER_SHADOW_MEMORY_H

#include <map>
#include <memory>
#include <vector>

#include "heap_block_manager.h"
#include "pm.h"
#include "mem_error_def.h"
#include "core/framework/platform_config.h"
#include "core/framework/record_defs.h"

namespace Sanitizer {
enum class MemStatus : uint8_t {
    NOACCESS = 0, // unaddressable
    UNDEFINED,    // addressable but uninitialized
    DEFINED,      // addressable and initialized
    ERROR,        // 多核写GM内存，内存被踩后置为该状态，该状态不参与action操作
};

// shadowMemory内存操作需要用到的内存记录
struct MemOpRecordForShadow {
    AddressSpace dstSpace;
    uint64_t dstAddr;
    uint64_t memSize;
    uint16_t coreId;
    uint64_t serialNo;

    MemOpRecordForShadow(AddressSpace dstSpace, uint64_t dstAddr,
        uint64_t memSize, uint8_t coreId)
        : dstSpace{dstSpace}, dstAddr{dstAddr}, memSize{memSize},
        coreId{coreId}, serialNo(0L)
        {}

    explicit MemOpRecordForShadow(const MemOpRecord &memOpRecord)
        : dstSpace{memOpRecord.dstSpace}, dstAddr{memOpRecord.dstAddr}, memSize{memOpRecord.memSize},
        coreId{static_cast<uint8_t>(memOpRecord.coreId)}, serialNo(memOpRecord.serialNo)
    {
    }
};

// 统计分配未使用内存
struct UnusedHeap {
    uint64_t bytesNotUse;
    uint64_t blockNum;
};

// GM内存中，前6位代表该处内存值的核id，后2位标记MemStatus中的4种状态
// UB中，每2位标识一个block的数据，即一个字节标识4个block
// L0A,L0B,L0C,L1中，每2位标识一个字节的数据
class ShadowMemory {
public:
    ShadowMemory();

    bool Init(ChipInfo deviceChipInfo);
    bool IsReady() const;
    void ResetChipMemory() noexcept;
    void ResetPrivateMemory() noexcept;

    // for SMs n-bytewise-operation
    ErrorMsgList LoadNBytes(MemOpRecordForShadow memOpRecordForShadow, bool initCheck);
    ErrorMsgList StoreNBytes(MemOpRecordForShadow memOpRecordForShadow, bool memCheck);

    // for SMs range setting
    void MakeMemUndefined(uint64_t addr, uint64_t size) const;
    void ClearBlockId(uint64_t addr, uint64_t size) const;

    // 未使用内存统计
    ErrorMsg CheckUnusedMem(uint64_t addr, uint64_t size);
    // 内存泄漏且未使用统计
    ErrorMsgList CheckUnusedHeap();
    UnusedHeap GetUnusedHeap();
    // for heap blocks
    bool AddHeapBlock(const MemOpRecord &record);
    ErrorMsg FreeHeapBlock(const MemOpRecord &record, uint64_t &size);

    ErrorMsgList DoLeakCheck();

    uint64_t GetHeapBlockSize(const MemOpRecord &record) const;
    uint64_t GetHeapBlockBypassSize(const MemOpRecord &record) const;

    /* 设置 shadow memory 原子写状态
     * @param[in] enabled 是否开启原子写
     */
    void SetAtomic(bool enabled) { this->atomicEnabled_ = enabled; }

private:
    // NOTE: these private methods assume that all input params are VALID for performance.
    /** @brief 保证地址和长度在建模范围内，调用者需保证 space 合法
     */
    void ValidateRange(AddressSpace space, uint64_t &addr, uint64_t &size) const;
    inline uint64_t GetGmAddrOffset(uint64_t addr);
    PM* GetMemMap(AddressSpace space) const;
    bool SkipSpace(AddressSpace space);
    ErrorMsg MakeBadBytesMsg(MemErrorType error, AddressSpace space, uint64_t addr, uint64_t nbytes) const;

    void StoreNBytesInRange(Range1D &range, AddressSpace space, uint8_t coreId, uint64_t &nBadBytesForOverlap);

    std::unique_ptr<PM> l0a_;
    std::unique_ptr<PM> l0b_;
    std::unique_ptr<PM> l0c_;
    std::unique_ptr<PM> l1_;
    std::unique_ptr<PM> ub_;
    std::unique_ptr<PM> gm_;
    std::unique_ptr<PM> private_;
    HeapBlockManager heapBlockManager_;
    HeapBlockManager heapBlockManagerBypass_;
    UnusedHeap unusedHeap_{};
    ChipInfo chipInfo_{};
    bool atomicEnabled_{false};
};
}

#endif
