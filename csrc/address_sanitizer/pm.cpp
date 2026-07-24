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

#include "pm.h"
#include "securec.h"
#include "core/framework/utility/log.h"

namespace Sanitizer {

using namespace std;

struct PM::SM {
    uint8_t vabits8[ONE_SM_STAND_FOR_BYTE];
    explicit SM(uint8_t bits8) noexcept
    {
        if (memset_s(&vabits8[0], ONE_SM_STAND_FOR_BYTE, bits8, ONE_SM_STAND_FOR_BYTE) != EOK) {
            SAN_WARN_LOG("Failed to init SM.");
        }
    }
};

Range1D::Iterator::Iterator(const Range1D &range, uint64_t addr) : range_(range), addr_(addr) { }

Range1D::Iterator &Range1D::Iterator::operator++(void)
{
    ++addr_;
    return *this;
}

Range1D::Iterator &Range1D::Iterator::operator+=(uint64_t n)
{
    this->addr_ += n;
    return *this;
}

uint8_t Range1D::Iterator::GetBits(void) const
{
    return range_.pm_.GetBits(addr_);
}

Range1D::Range1D(PM &pm, uint64_t addr, uint64_t size)
    : pm_(pm), addr_(addr), size_(size) { }

Range1D::Iterator Range1D::Begin(void) const
{
    return Iterator(*this, addr_);
}

Range1D::Iterator Range1D::End(void) const
{
    return Iterator(*this, addr_ + size_);
}

Range1D::Iterator Range1D::At(uint64_t addr) const
{
    if (addr < addr_ || addr > addr_ + size_) {
        return this->End();
    }
    return Iterator(*this, addr);
}

void Range1D::Set(uint8_t bits)
{
    pm_.Set(addr_, size_, bits);
}

uint64_t Range1D::Size(void) const
{
    return size_;
}

uint64_t Range1D::UnifiedSizeAfter(Iterator const &it) const
{
    return pm_.UnifiedSizeAfter(addr_, size_, it.addr_);
}

Range1D Range1D::UnifiedRangeAfter(Iterator const &it) const
{
    return Range1D(pm_, it.addr_, this->UnifiedSizeAfter(it));
}

PM::PM(const uint64_t &byteNum, uint8_t memInitVal) noexcept
    : byteNum_(byteNum), smNum_(0U), blockSize_(ONE_SM_STAND_FOR_BYTE)
{
    smNum_ = (byteNum + blockSize_ - 1U) / blockSize_;
    if (byteNum > 0U) {
        smList_.resize(smNum_, nullptr);
        commonBitsList_.resize(smNum_, memInitVal);
    }
}

void PM::Reset(uint8_t memInitVal) noexcept
{
    for (auto &sm : smList_) { // NOLINT(cppcoreguidelines-owning-memory)
        delete sm; // NOLINT(cppcoreguidelines-owning-memory)
        sm = nullptr;
    }

    commonBitsList_.assign(smNum_, memInitVal);
}

PM::~PM()
{
    for (auto &sm : smList_) { // NOLINT(cppcoreguidelines-owning-memory)
        delete sm; // NOLINT(cppcoreguidelines-owning-memory)
    }
}

Range1D PM::GetRange(uint64_t addr, uint64_t size)
{
    return Range1D(*this, addr, size);
}

uint8_t PM::GetBits(uint64_t addr)
{
    uint64_t blockIndex = GetBlockIdx(addr);
    if (smList_[blockIndex] == nullptr) {
        return commonBitsList_[blockIndex];
    } else {
        return smList_[blockIndex]->vabits8[GetBlockOffset(addr)];
    }
}

void PM::Set(uint64_t addr, uint64_t size, uint8_t bits)
{
    uint64_t blockIndexL = GetBlockIdx(addr);
    /// 不直接相加的目的是addr + size可能直接超过pm二级表的最大值，导致溢出后返回的blockIndexR < blockIndexL
    uint64_t blockIndexR = GetBlockIdx(addr) + GetBlockIdx(size);
    uint64_t remainderSum = GetBlockOffset(addr) + GetBlockOffset(size);
    blockIndexR += GetBlockIdx(remainderSum + blockSize_ - 1);
    uint64_t memsetCount {};
    for (uint64_t blockIndex = blockIndexL; blockIndex < blockIndexR; ++blockIndex) {
        uint64_t blockAddrL = blockSize_ * blockIndex;
        uint64_t blockAddrR = blockAddrL + blockSize_;
        if (blockIndex >= smList_.size()) {
            SAN_ERROR_LOG("SM idx (%lu) exceeds smList size (%lu)", blockIndex, smList_.size());
            break;
        }
        // 整个 block 可以由一个合法的 repeat 覆盖时使用 commonBitsList 保存状态
        if (addr <= blockAddrL && addr + size >= blockAddrR) {
            delete smList_[blockIndex]; // NOLINT(cppcoreguidelines-owning-memory)
            smList_[blockIndex] = nullptr;
            commonBitsList_[blockIndex] = bits;
            continue;
        }

        if (smList_[blockIndex] == nullptr) {
            smList_[blockIndex] = new SM(commonBitsList_[blockIndex]); // NOLINT(cppcoreguidelines-owning-memory)
        }
        PM::SM &sm = *smList_[blockIndex];
        uint64_t addrL = std::max(addr, blockAddrL);
        uint64_t addrR = std::min(addr + size, blockAddrR);
        uint64_t destMaxSm = sizeof(sm.vabits8) + blockAddrL - addrL;
        if (addrL == addrR) {
            continue;
        }
        if (memset_s(&sm.vabits8[addrL - blockAddrL], destMaxSm, bits, addrR - addrL)) {
            ++memsetCount;  // 记录memset失败的次数，超过0则统一打印，避免在for循环中输出相同日志内容
        }
    }
    if (memsetCount > 0) {
        SAN_WARN_LOG("Failed to set one block.");
    }
}

uint64_t PM::UnifiedSizeAfter(uint64_t baseAddr, uint64_t size, uint64_t addr) const
{
    uint64_t blockIndex = GetBlockIdx(addr);
    uint64_t blockOffset = GetBlockOffset(addr);
    uint64_t commonSize = std::min(baseAddr + size - addr, blockSize_ - blockOffset);
    if (smList_[blockIndex] != nullptr) {
        PM::SM const& sm = *smList_[blockIndex];
        uint8_t head = sm.vabits8[blockOffset];
        uint64_t offset = 1;
        for (; offset < commonSize && sm.vabits8[blockOffset + offset] == head; ++offset) { }
        return offset;
    } else {
        return commonSize;
    }
}

inline uint64_t PM::GetBlockIdx(uint64_t addr) const
{
    return (addr & LOCAL_MEM_MASK) / blockSize_;
}

inline uint64_t PM::GetBlockOffset(uint64_t addr) const
{
    return (addr & LOCAL_MEM_MASK) % blockSize_;
}

GmPM::GmPM(uint8_t memInitVal) noexcept
    : PM(LOCAL_MEM_MASK, memInitVal), memInitVal_(memInitVal), blockSize_(LOCAL_MEM_MASK)
{
    pmList_.resize((GLOBAL_MEM_MASK + blockSize_ - 1U) / blockSize_, nullptr);
}

// GM doesn't need this function.
void GmPM::Reset(uint8_t memInitVal) noexcept
{
    for (auto &pm : pmList_) {
        if (pm) {
            pm->Reset(memInitVal);
        }
    }
}

GmPM::~GmPM()
{
    for (auto pm : pmList_) { // NOLINT(cppcoreguidelines-owning-memory)
        if (pm) {
            delete pm; // NOLINT(cppcoreguidelines-owning-memory)
        }
    }
}

uint8_t GmPM::GetBits(uint64_t addr)
{
    auto pm = QueryPM(addr);
    if (!pm) {
        return commonBitsList_[GetBlockIdx(addr)];
    }
    return pm->GetBits(GetBlockOffset(addr));
}

void GmPM::Set(uint64_t addr, uint64_t size, uint8_t bits)
{
    uint64_t blockIndexL = GetBlockIdx(addr);
    uint64_t blockIndexR = GetBlockIdx(addr + size + blockSize_ - 1);
    while (blockIndexL < blockIndexR) {
        PmPtr pm = GetPM(addr);
        if (!pm) {
            return;
        }
        uint64_t pmAddr = GetBlockOffset(addr);
        uint64_t pmSize = std::min(blockSize_ - pmAddr, size);    // 一次遍历处理的pm长度；

        pm->Set(pmAddr, pmSize, bits);
        size -= pmSize;
        addr += pmSize;
        blockIndexL++;
    }
}

uint64_t GmPM::UnifiedSizeAfter(uint64_t baseAddr, uint64_t size, uint64_t addr) const
{
    uint64_t basePmIdx = GetBlockIdx(baseAddr);
    uint64_t pmIdx = GetBlockIdx(addr);
    uint64_t newBaseAddr = baseAddr;
    uint64_t nextPmAddr = (pmIdx + 1) * blockSize_;
    uint64_t curSize = std::min(nextPmAddr - baseAddr, size);
    /// 如果越过了pm，则更新基地址和curSize
    if (pmIdx > basePmIdx) {
        newBaseAddr = pmIdx * blockSize_;
        curSize = std::min(blockSize_, size - (newBaseAddr - baseAddr));
    }

    PmPtr pm = QueryPM(addr);
    if (!pm) {
        // 如果 pm 获取为空说明当前 pm 为一致的 bits 状态，此时返回当前 pm 中一致的 bits size
        return PM::UnifiedSizeAfter(GetBlockOffset(newBaseAddr), curSize, GetBlockOffset(addr));
    }
    return pm->UnifiedSizeAfter(GetBlockOffset(newBaseAddr), curSize, GetBlockOffset(addr));
}

GmPM::PmPtr GmPM::QueryPM(uint64_t addr) const
{
    uint64_t pmIdx = GetBlockIdx(addr);
    if (pmIdx >= pmList_.size()) {
        SAN_ERROR_LOG("QueryPM idx (%lu) exceeds pmList size (%lu)", pmIdx, pmList_.size());
        return nullptr;
    }
    return pmList_[pmIdx];
}

GmPM::PmPtr GmPM::GetPM(uint64_t addr)
{
    uint64_t pmIdx = GetBlockIdx(addr);
    if (pmIdx >= pmList_.size()) {
        SAN_ERROR_LOG("GetPM idx (%lu) exceeds pmList size (%lu)", pmIdx, pmList_.size());
        return nullptr;
    }
    if (pmList_[pmIdx] == nullptr) {
        pmList_[pmIdx] = new PM(byteNum_, memInitVal_); // NOLINT(cppcoreguidelines-owning-memory)
    }
    return pmList_[pmIdx];
}

inline uint64_t GmPM::GetBlockIdx(uint64_t addr) const
{
    uint64_t idx = (addr & GLOBAL_MEM_MASK) / blockSize_;
    return idx;
}

inline uint64_t GmPM::GetBlockOffset(uint64_t addr) const
{
    return (addr & GLOBAL_MEM_MASK) % blockSize_;
}

}  // namespace Sanitizer
