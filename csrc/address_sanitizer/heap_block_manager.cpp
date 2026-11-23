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


#include "address_sanitizer/mem_error_def.h"
#include "core/framework/record_defs.h"
#include "core/framework/utility/log.h"

#include "heap_block_manager.h"

namespace Sanitizer {

inline bool IsAddrInRange(const HeapBlock &block, uint64_t thresholdAddr, uint64_t thresholdSize)
{
    uint64_t addr = block.addr;
    uint64_t size = block.len;
    if (addr >= thresholdAddr && thresholdSize >= size && addr - thresholdAddr <= thresholdSize - size) {
        return true;
    }
    return false;
}

HeapBlockManager::HeapBlockManager()
{
    heapAddMap_[HeapType::RT] = &HeapBlockManager::AddRtDfxBlock;
    heapAddMap_[HeapType::DFX] = &HeapBlockManager::AddRtDfxBlock;
    heapAddMap_[HeapType::BYPASS] = &HeapBlockManager::AddByPassBlock;
    heapAddMap_[HeapType::MSTX_HEAP] = &HeapBlockManager::AddMstxHeapBlock;
    heapAddMap_[HeapType::MSTX_RIGION] = &HeapBlockManager::AddMstxRegionBlock;
    heapFreeMap_[HeapType::RT] = &HeapBlockManager::FreeRtDfxBlock;
    heapFreeMap_[HeapType::DFX] = &HeapBlockManager::FreeRtDfxBlock;
    heapFreeMap_[HeapType::BYPASS] = &HeapBlockManager::FreeByPassBlock;
    heapFreeMap_[HeapType::MSTX_HEAP] = &HeapBlockManager::FreeMstxHeapBlock;
    heapFreeMap_[HeapType::MSTX_RIGION] = &HeapBlockManager::FreeMstxRegionBlock;
}

HeapBlockManager::HeapBlocks &HeapBlockManager::GetHeapBlocks(MemInfoSrc infoSrc)
{
    return infoSrc == MemInfoSrc::EXTRA || infoSrc == MemInfoSrc::MANUAL ||
        infoSrc == MemInfoSrc::BYPASS ? heapBlocksForDfx_ : heapBlocks_;
}

HeapType HeapBlockManager::GetHeapType(const MemOpRecord &record)
{
    switch (record.infoSrc) {
        case MemInfoSrc::HAL:
        case MemInfoSrc::RT:
        case MemInfoSrc::ACL:
            return HeapType::RT;
        case MemInfoSrc::EXTRA:
        case MemInfoSrc::MANUAL:
            return HeapType::DFX;
        case MemInfoSrc::BYPASS:
            return HeapType::BYPASS;
        case MemInfoSrc::MSTX_HEAP:
            return HeapType::MSTX_HEAP;
        case MemInfoSrc::MSTX_REGION:
            return HeapType::MSTX_RIGION;
        default:
            SAN_ERROR_LOG("Failed to get heapType. memInfoSrc: %u", static_cast<uint32_t>(record.infoSrc));
            return HeapType::INVALID;
    }
}

bool HeapBlockManager::AddHeapBlock(const MemOpRecord &record)
{
    HeapType heapType = GetHeapType(record);
    if (heapType == HeapType::INVALID) {
        return false;
    }
    auto it = heapAddMap_.find(heapType);
    if (it == heapAddMap_.end()) {
        SAN_ERROR_LOG("HeapType %u is not found in heapAddMap", static_cast<uint32_t>(heapType));
        return false;
    }
    return (this->*(it->second))(record);
}

bool HeapBlockManager::AddRtDfxBlock(const MemOpRecord &record)
{
    uint64_t addr = record.dstAddr;
    uint64_t size = record.memSize;
    MemInfoSrc infoSrc = record.infoSrc;
    HeapBlock block(record);
    // EXTRA 级别的信息是由 adump 上报的 dfx 信息，dfx 信息可能会与 runtime 信息冲突，
    // 因此采用单独的 heap block 表进行保存
    HeapBlocks &heapBlocks = this->GetHeapBlocks(record.infoSrc);
    if (heapBlocks.find(addr) == heapBlocks.cend()) {
        heapBlocks[addr] = block;
        SAN_INFO_LOG("Add heap block. serialNo:%lu, memInfoSrc:%s, addr:0x%lx, size:%lu",
            record.serialNo, MemInfoSrcToString(infoSrc).c_str(), addr, size);
    } else {
        SAN_INFO_LOG("Double malloc. serialNo:%lu, memInfoSrc:%s, addr:0x%lx, size:%lu",
            record.serialNo, MemInfoSrcToString(infoSrc).c_str(), addr, size);
    }
    return true;
}

bool HeapBlockManager::AddByPassBlock(const MemOpRecord &record)
{
    uint64_t addr = record.dstAddr;
    uint64_t size = record.memSize;
    MemInfoSrc infoSrc = record.infoSrc;
    HeapBlock block(record);

    auto processHeapBlock = [=](HeapBlocks& heapMap) {
        if (heapMap.find(addr) == heapMap.cend()) {
            heapMap[addr] = block;
            SAN_INFO_LOG("Add bypass heap block. serialNo:%lu, memInfoSrc:%s, addr:0x%lx, size:%lu",
                record.serialNo, MemInfoSrcToString(infoSrc).c_str(), addr, size);
        } else {
            SAN_INFO_LOG("Add bypass heap double malloc. serialNo:%lu, memInfoSrc:%s, addr:0x%lx, size:%lu",
                record.serialNo, MemInfoSrcToString(infoSrc).c_str(), addr, size);
        }
    };
    processHeapBlock(heapBlocks_);
    processHeapBlock(heapBlocksForDfx_);
    return true;
}

bool HeapBlockManager::AddMstxHeapBlock(const MemOpRecord &record)
{
    uint64_t addr = record.dstAddr;
    uint64_t size = record.memSize;
    auto it = mstxHeapBlocks_.find(addr);
    if (it != mstxHeapBlocks_.end()) {
        SAN_INFO_LOG("Heap double malloc. serialNo:%lu, addr:0x%lx, size:%lu", record.serialNo, addr, size);
        return false;
    }
    HeapBlock block(record);
    for (const auto &bk : heapBlocks_) {
        if (IsAddrInRange(block, bk.second.addr, bk.second.len)) {
            mstxHeapBlocks_[addr] = block;
            return true;
        }
    }
    block.isInvalid = true;
    mstxHeapBlocks_[addr] = block;
    std::cout << "[mssanitizer] ERROR: heap is not in malloc range, heap addr: 0x" << std::hex << addr
        << std::dec << " size: " << size << std::endl;
    return false;
}

bool HeapBlockManager::AddMstxRegionBlock(const MemOpRecord &record)
{
    uint64_t rootAddr = record.rootAddr;
    auto it = mstxHeapBlocks_.find(rootAddr);
    if (rootAddr != 0UL && it == mstxHeapBlocks_.cend()) {
        std::cout << "[mssanitizer] ERROR: add regions error, heap is not registered, addr:" << std::hex << rootAddr
                  << std::dec << " size: " << record.memSize << std::endl;
        return false;
    }
    SAN_INFO_LOG("Add region block, serialNo:%lu, memInfoSrc:%u, addr:0x%lx, size:%lu",
        record.serialNo, static_cast<uint32_t>(record.infoSrc), record.dstAddr, record.memSize);
    HeapBlock block(record);
    mstxRegionBlocks_[rootAddr].push_back(block);
    return rootAddr == 0UL || !it->second.isInvalid;
}

ErrorMsg HeapBlockManager::FreeHeapBlock(const MemOpRecord &record, uint64_t &size)
{
    HeapType heapType = GetHeapType(record);
    if (heapType == HeapType::INVALID) {
        return {};
    }
    auto it = heapFreeMap_.find(heapType);
    if (it == heapFreeMap_.end()) {
        SAN_ERROR_LOG("HeapType %u is not found in heapFreeMap", static_cast<uint32_t>(heapType));
        return {};
    }
    return (this->*(it->second))(record, size);
}

bool IsRealMalloc(MemInfoSrc infoSrc)
{
    if (infoSrc == MemInfoSrc::HAL || infoSrc == MemInfoSrc::ACL || infoSrc == MemInfoSrc::RT) {
        return true;
    }
    return false;
}

void HeapBlockManager::DeleteMstxInfo(const MemOpRecord &record, uint64_t freeSize)
{
    if (!IsRealMalloc(record.infoSrc)) {
        return;
    }
    uint64_t addr = record.dstAddr;
    for (auto it = mstxHeapBlocks_.begin(); it != mstxHeapBlocks_.end();) {
        /// 内存来源为真实内存时，释放时需要删除和真实内存关联的mstx heap和regions信息，防止漏报
        if (IsAddrInRange(it->second, addr, freeSize)) {
            mstxRegionBlocks_.erase(it->first);
            mstxHeapBlocks_.erase(it);
            return;
        } else {
            ++it;
        }
    }
}

ErrorMsg HeapBlockManager::FreeRtDfxBlock(const MemOpRecord &record, uint64_t &size)
{
    ErrorMsg msg;
    MemInfoSrc infoSrc = record.infoSrc;
    uint64_t addr = record.dstAddr;
    HeapBlocks &heapBlocks = this->GetHeapBlocks(infoSrc);
    auto it = heapBlocks.find(addr);
    if (it == heapBlocks.cend()) {
        SAN_ERROR_LOG("Illegal free. serialNo:%lu, memInfoSrc:%s, addr:0x%lx",
            record.serialNo, MemInfoSrcToString(infoSrc).c_str(), addr);
        msg.SetType(MemErrorType::ILLEGAL_FREE, AddressSpace::GM, addr);
        return msg;
    }
    size = it->second.len;
    DeleteMstxInfo(record, size);
    SAN_INFO_LOG("Free heap block. serialNo:%lu, memInfoSrc:%s, addr:0x%lx, size:%lu",
        record.serialNo, MemInfoSrcToString(infoSrc).c_str(), addr, it->second.len);
    heapBlocks.erase(it);
    return msg;
}

ErrorMsg HeapBlockManager::FreeByPassBlock(const MemOpRecord &record, uint64_t &size)
{
    ErrorMsg msg;
    MemInfoSrc infoSrc = record.infoSrc;
    uint64_t addr = record.dstAddr;

    auto rtIt = heapBlocks_.find(addr);
    auto dfxIt = heapBlocksForDfx_.find(addr);
    if (rtIt == heapBlocks_.end() || dfxIt == heapBlocksForDfx_.end()) {
        SAN_ERROR_LOG("Illegal free. serialNo:%lu, memInfoSrc:%s, addr:0x%lx",
            record.serialNo, MemInfoSrcToString(infoSrc).c_str(), addr);
        msg.SetType(MemErrorType::ILLEGAL_FREE, AddressSpace::GM, addr);
        return msg;
    }
    size = rtIt->second.len;
    SAN_INFO_LOG("Free heap block. serialNo:%lu, memInfoSrc:%s, addr:0x%lx, rtSize:%lu, dfxSize:%lu",
        record.serialNo, MemInfoSrcToString(infoSrc).c_str(), addr, rtIt->second.len, dfxIt->second.len);
    heapBlocks_.erase(rtIt);
    heapBlocksForDfx_.erase(dfxIt);
    return msg;
}

ErrorMsg HeapBlockManager::FreeMstxHeapBlock(const MemOpRecord &record, uint64_t &size)
{
    ErrorMsg msg;
    uint64_t addr = record.dstAddr;
    auto it = mstxHeapBlocks_.find(addr);
    if (it == mstxHeapBlocks_.cend()) {
        SAN_ERROR_LOG("Mstx heap error in free, heap is not registered, serialNo:%lu, addr: 0x%lx",
            record.serialNo, addr);
        msg.SetType(MemErrorType::ILLEGAL_FREE, AddressSpace::GM, addr);
        return msg;
    }
    SAN_INFO_LOG("Mstx free heap block. addr: 0x%lx", addr);
    size = it->second.len;
    /// heap如果提前被register, 则对应的region要删除
    mstxHeapBlocks_.erase(it);
    mstxRegionBlocks_.erase(addr);
    return msg;
}

ErrorMsg HeapBlockManager::FreeMstxRegionBlock(const MemOpRecord &record, uint64_t &size)
{
    ErrorMsg msg;
    uint64_t addr = record.dstAddr;
    HeapBlock block(record);
    for (auto &pair : mstxRegionBlocks_) {
        auto &regionsVec = pair.second;
        for (auto it = regionsVec.rbegin(); it != regionsVec.rend();) {
            if (it->addr == addr) {
                size = it->len;
                SAN_INFO_LOG("Mstx free region block. serialNo:%lu, addr:0x%lx", record.serialNo, addr);
                regionsVec.erase(std::next(it).base());
                return msg;
            } else {
                ++it;
            }
        }
    }
    SAN_ERROR_LOG("Mstx illegal free. serialNo:%lu, memInfoSrc:%u, addr:0x%lx",
        record.serialNo, static_cast<uint32_t>(record.infoSrc), addr);
    msg.SetType(MemErrorType::ILLEGAL_FREE, AddressSpace::GM, addr);
    return msg;
}

ErrorMsgList HeapBlockManager::DoLeakCheck(void) const
{
    // 不需要检测 dfx heap block 的泄漏
    ErrorMsgList msgs;
    for (auto const &block : heapBlocks_) {
        ErrorMsg msg;
        msg.SetType(MemErrorType::MEM_LEAK, AddressSpace::GM, block.second.addr);
        msg.auxData.nBadBytes = block.second.len;
        msg.auxData.moduleId = block.second.moduleId;
        msg.auxData.fileName = std::string(block.second.fileName);
        msg.auxData.lineNo = block.second.lineNo;
        msg.auxData.serialNo = block.second.serialNo;
        msgs.emplace_back(std::move(msg));
    }
    return msgs;
}

uint64_t HeapBlockManager::GetHeapBlockSize(const MemOpRecord &record) const
{
    {
        auto it = heapBlocks_.find(record.dstAddr);
        if (it != heapBlocks_.cend()) {
            return it->second.len;
        }
    }
    {
        auto it = heapBlocksForDfx_.find(record.dstAddr);
        if (it != heapBlocksForDfx_.cend()) {
            return it->second.len;
        }
    }
    return 0U;
}

} // namespace Sanitizer
