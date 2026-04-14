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


#include <cinttypes>
#include <algorithm>
#include <set>

#include "mem_error_def.h"
#include "core/framework/utility/cpp_future.h"
#include "core/framework/utility/log.h"
#include "core/framework/record_defs.h"
#include "core/framework/record_format.h"
#include "core/framework/format_converter.h"
#include "shadow_memory.h"

namespace {
constexpr uint8_t VA_BITS_SHIFT = 2;
constexpr uint8_t STATUS_MASK = 0b11;
constexpr uint8_t COREID_MASK = ~0b11;
constexpr uint8_t DEFAULT_CORE_ID = 0b111111;
constexpr uint8_t GM_DEFAULT_VALUE = static_cast<uint8_t>(Sanitizer::MemStatus::UNDEFINED) |
                                     (DEFAULT_CORE_ID << VA_BITS_SHIFT);
constexpr uint8_t CHIPMEM_DEFAULT_VALUE = static_cast<uint8_t>(Sanitizer::MemStatus::UNDEFINED) |
                                          (DEFAULT_CORE_ID << VA_BITS_SHIFT);
constexpr uint8_t PRIVATE_DEFAULT_VALUE = static_cast<uint8_t>(Sanitizer::MemStatus::UNDEFINED) |
                                          (DEFAULT_CORE_ID << VA_BITS_SHIFT);
}

namespace Sanitizer {

inline uint8_t MakeByte(MemStatus status, uint8_t coreId)
{
    return (static_cast<uint8_t>(status) & STATUS_MASK) |
        ((coreId << VA_BITS_SHIFT) & COREID_MASK);
}

inline MemStatus GetMemStatus(uint8_t byte)
{
    return static_cast<MemStatus>(byte & STATUS_MASK);
}

inline uint8_t GetCoreId(uint8_t byte)
{
    return (byte & COREID_MASK) >> VA_BITS_SHIFT;
}

inline uint8_t SetMemStatus(uint8_t byte, MemStatus status)
{
    return (byte & COREID_MASK) | (static_cast<uint8_t>(status) & STATUS_MASK);
}

inline uint8_t SetCoreId(uint8_t byte, uint8_t coreId)
{
    return (byte & STATUS_MASK) | ((coreId << VA_BITS_SHIFT) & COREID_MASK);
}

ShadowMemory::ShadowMemory(void) : gm_{MakeUnique<GmPM>(GM_DEFAULT_VALUE)} { }

bool ShadowMemory::Init(ChipInfo deviceChipInfo)
{
    chipInfo_ = deviceChipInfo;
    l0a_ = std::unique_ptr<PM>(new PM(chipInfo_.l0aSize, CHIPMEM_DEFAULT_VALUE));
    l0b_ = std::unique_ptr<PM>(new PM(chipInfo_.l0bSize, CHIPMEM_DEFAULT_VALUE));
    l0c_ = std::unique_ptr<PM>(new PM(chipInfo_.l0cSize, CHIPMEM_DEFAULT_VALUE));
    l1_ = std::unique_ptr<PM>(new PM(chipInfo_.l1Size, CHIPMEM_DEFAULT_VALUE));
    ub_ = std::unique_ptr<PM>(new PM(chipInfo_.ubSize, CHIPMEM_DEFAULT_VALUE));
    private_ = std::unique_ptr<PM>(new PM(chipInfo_.privateSize, PRIVATE_DEFAULT_VALUE));
    return IsReady();
}

bool ShadowMemory::IsReady() const
{
    if (!(gm_ && l0a_ && l0b_ && l0c_ && l1_ && ub_ && private_)) {
        SAN_ERROR_LOG("ShadowMemory is not ready");
        return false;
    }
    return true;
}

void ShadowMemory::ResetChipMemory() noexcept
{
    l0a_->Reset(CHIPMEM_DEFAULT_VALUE);
    l0b_->Reset(CHIPMEM_DEFAULT_VALUE);
    l0c_->Reset(CHIPMEM_DEFAULT_VALUE);
    l1_->Reset(CHIPMEM_DEFAULT_VALUE);
    ub_->Reset(CHIPMEM_DEFAULT_VALUE);
}

void ShadowMemory::ResetPrivateMemory() noexcept
{
    private_->Reset(PRIVATE_DEFAULT_VALUE);
}

PM* ShadowMemory::GetMemMap(AddressSpace space) const
{
    switch (space) {
        case AddressSpace::GM:
            return gm_.get();
        case AddressSpace::L0A:
            return l0a_.get();
        case AddressSpace::L0B:
            return l0b_.get();
        case AddressSpace::L0C:
            return l0c_.get();
        case AddressSpace::L1:
            return l1_.get();
        case AddressSpace::UB:
            return ub_.get();
        case AddressSpace::PRIVATE:
            return private_.get();
        case AddressSpace::INVALID:
        default:
            SAN_WARN_LOG("Cannot find corresponding map for %s", FormatAddressSpace(space).c_str());
            return nullptr;
    }
}

bool ShadowMemory::AddHeapBlock(const MemOpRecord &record)
{
    if (record.infoSrc == MemInfoSrc::BYPASS) {
        return heapBlockManagerBypass_.AddHeapBlock(record);
    } else {
        return heapBlockManager_.AddHeapBlock(record);
    }
}

ErrorMsg ShadowMemory::FreeHeapBlock(const MemOpRecord &record, uint64_t &size)
{
    if (record.infoSrc == MemInfoSrc::BYPASS) {
        return heapBlockManagerBypass_.FreeHeapBlock(record, size);
    } else {
        return heapBlockManager_.FreeHeapBlock(record, size);
    }
}

ErrorMsg ShadowMemory::CheckUnusedMem(uint64_t addr, uint64_t size)
{
    ErrorMsg errMsg;
    uint64_t nBadBytesForNoUse = 0U;

    PM *memmap = GetMemMap(AddressSpace::GM);
    if (memmap == nullptr) {
        return errMsg;
    }

    Range1D range = memmap->GetRange(addr, size);
    uint64_t unifiedSize = 1UL;
    for (Range1D::Iterator it = range.Begin(); it < range.End(); it += unifiedSize) {
        Range1D unifiedRange = range.UnifiedRangeAfter(it);
        unifiedSize = unifiedRange.Size();
        uint8_t bits = it.GetBits();
        if (GetMemStatus(bits) == MemStatus::UNDEFINED) {
            nBadBytesForNoUse += unifiedSize;
        }
    }
    if (nBadBytesForNoUse > 0U) {
        unusedHeap_.bytesNotUse += nBadBytesForNoUse;
        unusedHeap_.blockNum++;
        errMsg = MakeBadBytesMsg(MemErrorType::MEM_UNUSED, AddressSpace::GM, addr, nBadBytesForNoUse);
        auto const &heapBlocks = heapBlockManager_.GetHeapBlocks();
        auto blkIt = heapBlocks.find(addr);
        if (blkIt != heapBlocks.end()) {
            errMsg.auxData.fileName = blkIt->second.fileName;
            errMsg.auxData.lineNo = blkIt->second.lineNo;
        }
    }
    return errMsg;
}

ErrorMsgList ShadowMemory::CheckUnusedHeap()
{
    ErrorMsgList errMsgList;
    auto const &heapBlocks = heapBlockManager_.GetHeapBlocks();
    for (const auto &it : heapBlocks) {
        ErrorMsg errMsg = CheckUnusedMem(it.second.addr, it.second.len);
        if (errMsg.isError) {
            errMsg.auxData.serialNo = it.second.serialNo;
            errMsgList.emplace_back(errMsg);
        }
    }
    return errMsgList;
}

UnusedHeap ShadowMemory::GetUnusedHeap()
{
    return unusedHeap_;
}

ErrorMsgList ShadowMemory::DoLeakCheck()
{
    return heapBlockManager_.DoLeakCheck();
}

uint64_t ShadowMemory::GetHeapBlockSize(const MemOpRecord &record) const
{
    return heapBlockManager_.GetHeapBlockSize(record);
}

uint64_t ShadowMemory::GetHeapBlockBypassSize(const MemOpRecord &record) const
{
    return heapBlockManagerBypass_.GetHeapBlockSize(record);
}

ErrorMsgList ShadowMemory::LoadNBytes(MemOpRecordForShadow memOpRecordForShadow, bool initCheck)
{
    ErrorMsgList msgList;
    AddressSpace space = memOpRecordForShadow.dstSpace;
    uint64_t addr = memOpRecordForShadow.dstAddr;
    uint64_t size = memOpRecordForShadow.memSize;
    if (SkipSpace(space)) {
        return msgList;
    }

    ValidateRange(space, addr, size);

    uint64_t nUninitializedReadBytes = 0U;
    PM *memmap = GetMemMap(space);
    if (memmap == nullptr) { return msgList; }

    Range1D range = memmap->GetRange(addr, size);
    uint64_t unifiedSize = 1UL;
    for (Range1D::Iterator it = range.Begin(); it < range.End(); it += unifiedSize) {
        Range1D unifiedRange = range.UnifiedRangeAfter(it);
        unifiedSize = unifiedRange.Size();
        uint8_t bits = it.GetBits();
        if (initCheck && GetMemStatus(bits) == MemStatus::UNDEFINED) {
            nUninitializedReadBytes += unifiedSize;
        }
    }

    if (nUninitializedReadBytes > 0U) {
        msgList.emplace_back(MakeBadBytesMsg(MemErrorType::UNINITIALIZED_READ, space, addr, nUninitializedReadBytes));
    }
    return msgList;
}

ErrorMsgList ShadowMemory::StoreNBytes(MemOpRecordForShadow memOpRecordForShadow, bool memCheck)
{
    ErrorMsgList msgList;
    AddressSpace space = memOpRecordForShadow.dstSpace;
    uint64_t addr = memOpRecordForShadow.dstAddr;
    uint64_t size = memOpRecordForShadow.memSize;
    uint8_t coreId = memOpRecordForShadow.coreId;
    if (SkipSpace(space)) { return msgList; }

    ValidateRange(space, addr, size);

    PM *memmap = GetMemMap(space);
    if (memmap == nullptr) { return msgList; }

    uint64_t nBadBytesForOverlap = 0U;
    Range1D range = memmap->GetRange(addr, size);
    StoreNBytesInRange(range, space, coreId, nBadBytesForOverlap);

    if (!memCheck) { return msgList; }

    if (nBadBytesForOverlap > 0U) {
        msgList.emplace_back(MakeBadBytesMsg(MemErrorType::OUT_OF_BOUNDS, space, addr, nBadBytesForOverlap));
    }

    return msgList;
}

void ShadowMemory::StoreNBytesInRange(Range1D &range, AddressSpace space,
                                      uint8_t coreId,
                                      uint64_t &nBadBytesForOverlap)
{
    uint64_t unifiedSize = 1UL;
    for (Range1D::Iterator it = range.Begin(); it < range.End(); it += unifiedSize) {
        Range1D unifiedRange = range.UnifiedRangeAfter(it);
        unifiedSize = unifiedRange.Size();
        uint8_t bits = it.GetBits();
        if (GetMemStatus(bits) == MemStatus::ERROR) {
            continue;
        } else if (GetMemStatus(bits) == MemStatus::UNDEFINED) {
            bits = SetMemStatus(bits, MemStatus::DEFINED);
            unifiedRange.Set(bits);
        }
        if (space != AddressSpace::GM) {
            continue;
        }
        uint8_t id = GetCoreId(bits);
        if (coreId == 0xFF) {
            bits = SetCoreId(bits, DEFAULT_CORE_ID);
            unifiedRange.Set(bits);
        } else if (id == DEFAULT_CORE_ID) {
            bits = SetCoreId(bits, coreId);
            unifiedRange.Set(bits);
        } else if (id != coreId) {
            if (this->atomicEnabled_) {
                continue;
            }
            nBadBytesForOverlap += unifiedSize;
            bits = SetMemStatus(bits, MemStatus::ERROR);
            unifiedRange.Set(bits);
        }
    }
}

void ShadowMemory::MakeMemUndefined(uint64_t addr, uint64_t size) const
{
    ValidateRange(AddressSpace::GM, addr, size);

    PM *memmap = GetMemMap(AddressSpace::GM);
    if (memmap == nullptr) {
        return;
    }

    memmap->GetRange(addr, size).Set(MakeByte(MemStatus::UNDEFINED, DEFAULT_CORE_ID));
}

void ShadowMemory::ClearBlockId(uint64_t addr, uint64_t size) const
{
    ValidateRange(AddressSpace::GM, addr, size);

    PM *memmap = GetMemMap(AddressSpace::GM);
    if (memmap == nullptr) { return; }

    Range1D range = memmap->GetRange(addr, size);
    uint64_t unifiedSize = 1UL;
    for (Range1D::Iterator it = range.Begin(); it < range.End(); it += unifiedSize) {
        Range1D unifiedRange = range.UnifiedRangeAfter(it);
        unifiedSize = unifiedRange.Size();
        uint8_t bits = it.GetBits();
        bits = SetCoreId(bits, DEFAULT_CORE_ID);
        unifiedRange.Set(bits);
    }
}

void ShadowMemory::ValidateRange(AddressSpace space, uint64_t &addr, uint64_t &size) const
{
    const std::map<AddressSpace, uint64_t> MEM_SIZE_MAP = {
        {AddressSpace::GM, chipInfo_.hbmSize},
        {AddressSpace::L1, chipInfo_.l1Size},
        {AddressSpace::L0A, chipInfo_.l0aSize},
        {AddressSpace::L0B, chipInfo_.l0bSize},
        {AddressSpace::L0C, chipInfo_.l0cSize},
        {AddressSpace::UB, chipInfo_.ubSize},
        {AddressSpace::PRIVATE, chipInfo_.privateSize},
    };

    uint64_t maxSize = space == AddressSpace::GM ? GLOBAL_MEM_MASK : MEM_SIZE_MAP.find(space)->second;
    uint64_t addrR {};
    if (UINT64_MAX - size < addr) {
        addrR = maxSize;
    } else {
        addrR = std::min(maxSize, addr + size);
    }
    addr = std::min(maxSize, addr);
    size = addrR - addr;
}

bool ShadowMemory::SkipSpace(AddressSpace space)
{
    return space == AddressSpace::INVALID;
}

ErrorMsg ShadowMemory::MakeBadBytesMsg(MemErrorType error, AddressSpace space,
                                       uint64_t addr, uint64_t nbytes) const
{
    ErrorMsg msg;
    msg.SetType(error, space, addr);
    msg.auxData.nBadBytes = nbytes;
    return msg;
}

} // namespace Sanitizer
