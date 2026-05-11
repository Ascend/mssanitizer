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


#include <algorithm>
#include <cstdint>
#include <iostream>

#include "core/framework/record_defs.h"
#include "core/framework/utility/cpp_future.h"
#include "core/framework/utility/log.h"
#include "core/framework/format_converter.h"
#include "mem_error_def.h"

#include "bounds_check.h"

namespace {

using namespace Sanitizer;

inline uint64_t OverlapSize(Bounds::Range const &a, Bounds::Range const &b)
{
    uint64_t l = std::max(a.addrL, b.addrL);
    uint64_t r = std::min(a.addrR, b.addrR);
    return r > l ? r - l : 0UL;
}

inline bool HasPermission(uint32_t permission, AccessType accessType)
{
    if (accessType == AccessType::READ) {
        return (permission & MSTX_MEM_PERMISSIONS_REGION_FLAGS_READ) != 0;
    } else if (accessType == AccessType::WRITE) {
        return (permission & MSTX_MEM_PERMISSIONS_REGION_FLAGS_WRITE) != 0;
    } else {
        return
            (permission & MSTX_MEM_PERMISSIONS_REGION_FLAGS_READ) != 0 &&
            (permission & MSTX_MEM_PERMISSIONS_REGION_FLAGS_WRITE) != 0;
    }
}

} // namespace [Dummy]

namespace Sanitizer {

constexpr uint32_t Bounds::DEFAULT_PERMISSION;

ErrorMsg DiscreteBounds::Add(uint64_t addr, uint64_t size, uint32_t permission)
{
    ErrorMsg msg = CheckOverflow(addr, size);
    if (msg.isError) {
        return msg;
    }

    auto pred = [](uint64_t a, Range const &b) { return a < b.addrL; };
    auto it = std::upper_bound(ranges_.begin(), ranges_.end(), addr, pred);
    // 检查前一个范围是否能和当前范围融合，可以的话将前一个范围扩展到当前范围的
    // 右边界，否则把当前范围插入数组
    if (it != ranges_.cbegin() && std::prev(it)->addrR >= addr) {
        if (std::prev(it)->permission == permission) {
            it = std::prev(it);
            it->addrR = std::max(it->addrR, addr + size);
        } else {
            std::prev(it)->addrR = addr;
            it = ranges_.insert(it, Range{addr, addr + size, permission});
        }
    } else {
        it = ranges_.insert(it, Range{addr, addr + size, permission});
    }

    // 遍历右边的范围，如果与当前范围重叠，那么将当前范围扩展到右边界；否则由于
    // 数组里的范围是有序的，那么后续的范围都不可能重叠，跳出循环
    auto next = std::next(it);
    for (; next < ranges_.cend(); ++next) {
        if (next->addrL > it->addrR) {
            break;
        }
        if (next->addrR > it->addrR && next->permission != permission) {
            next->addrL = it->addrR;
            break;
        }
        it->addrR = std::max(it->addrR, next->addrR);
    }
    // 被遍历的部分都是已被当前范围融合的部分，需要从数组中删除
    ranges_.erase(std::next(it), next);
    return ErrorMsg();
}

ErrorMsg DiscreteBounds::Remove(uint64_t addr, uint64_t size)
{
    ErrorMsg msg = CheckOverflow(addr, size);
    if (msg.isError) {
        return msg;
    }

    auto pred = [](uint64_t a, Range const &b) { return a < b.addrR; };
    auto it = std::upper_bound(ranges_.begin(), ranges_.end(), addr, pred);
    // 删除范围时一个范围可能被分割成两半。如果有左侧一半需要将这个范围插入
    if (it != ranges_.cend() && addr > it->addrL) {
        it = std::next(ranges_.insert(it, Range{it->addrL, addr, it->permission}));
    }

    // 遍历右边的范围，如果与要删除的范围完成不重叠那么跳出循环；如果部分重叠需要
    // 去掉重叠的部分
    uint64_t addrR = addr + size;
    auto next = it;
    for (; next < ranges_.cend(); ++next) {
        if (next->addrL >= addrR) {
            break;
        }
        if (next->addrR > addrR) {
            next->addrL = addrR;
            break;
        }
    }
    // 被遍历的部分都是完全被需要删除的范围覆盖的部分，需要从数组中删除
    ranges_.erase(it, next);
    return ErrorMsg();
}

ErrorMsg DiscreteBounds::SetPermission(uint64_t addr, uint64_t size, uint32_t permission)
{
    ErrorMsg msg = CheckOverflow(addr, size);
    if (msg.isError) {
        return msg;
    }

    auto pred = [](uint64_t a, Range const &b) { return a < b.addrR; };
    auto it = std::upper_bound(ranges_.begin(), ranges_.end(), addr, pred);
    // 如果当前区间有一半在范围内，并且权限属性与设置不相同，则将区间切分为两半
    if (it != ranges_.cend() && addr > it->addrL && permission != it->permission) {
        it = std::next(ranges_.insert(it, Range{it->addrL, addr, it->permission}));
        it->addrL = addr;
    }

    uint64_t addrR = addr + size;
    for (; it != ranges_.cend(); ++it) {
        if (it->addrL >= addrR) {
            break;
        }
        if (it->addrR > addrR) {
            it = std::next(ranges_.insert(it, Range{it->addrL, addrR, permission}));
            it->addrL = addrR;
            break;
        }
        it->permission = permission;
    }
    return ErrorMsg();
}

ErrorMsg DiscreteBounds::Check(uint64_t addr, uint64_t size, AccessType accessType) const
{
    // 当要检查的范围 size 为 0 时，只检查 addr 地址是否在范围内，并且
    // 异常报告中的越界大小显示为 1 以保持格式兼容
    if (size == 0) {
        return CheckAddrOnly(addr);
    }

    auto pred = [](uint64_t a, Range const &b) { return a < b.addrR; };
    auto it = std::upper_bound(ranges_.begin(), ranges_.end(), addr, pred);
    uint64_t badBytes = size;
    Range target {addr, addr + size};
    for (; it < ranges_.cend(); ++it) {
        if (it->addrL >= target.addrR) {
            break;
        }
        if (!HasPermission(it->permission, accessType)) {
            continue;
        }
        badBytes -= OverlapSize(target, *it);
    }

    if (badBytes > 0) {
        ErrorMsg msg;
        msg.isError = true;
        msg.auxData.badAddr.addr = addr;
        msg.auxData.nBadBytes = badBytes;
        return msg;
    }
    return ErrorMsg();
}

ErrorMsg DiscreteBounds::CheckAddrOnly(uint64_t addr) const
{
    auto pred = [](uint64_t a, Range const &b) { return a < b.addrR; };
    auto it = std::upper_bound(ranges_.begin(), ranges_.end(), addr, pred);
    if (it == ranges_.cend() || (addr < it->addrL || addr >= it->addrR)) {
        ErrorMsg msg;
        msg.isError = true;
        msg.auxData.badAddr.addr = addr;
        msg.auxData.nBadBytes = 1;
        return msg;
    }

    return ErrorMsg();
}

ErrorMsg DiscreteBounds::CheckOverflow(uint64_t addr, uint64_t size) const
{
    static constexpr uint64_t globalMemMask = 0xFFFFFFFFFFFFULL;
    uint64_t badBytes {};
    if (addr <= globalMemMask && size > globalMemMask - addr) {
        badBytes = size - (globalMemMask - addr);
        SAN_INFO_LOG("Right bound of memory range overflows. base address: 0x%lx, size: %lu", addr, size);
    } else if (addr > globalMemMask) {
        badBytes = size;
        SAN_INFO_LOG("Left bound of memory range overflows. base address: 0x%lx, size: %lu", addr, size);
    }

    ErrorMsg msg;
    if (badBytes > 0) {
        msg.SetType(MemErrorType::ILLEGAL_ADDR_WRITE, AddressSpace::GM, addr);
        msg.auxData.lineNo = __LINE__;
        msg.auxData.nBadBytes = badBytes;
    }
    return msg;
}

ErrorMsg UnionBounds::Check(uint64_t addr, uint64_t size, AccessType accessType) const
{
    if (size == 0) {
        return CheckAddrOnly(addr);
    }

    uint64_t overlap = OverlapSize(range_, Range{addr, addr + size});
    if (!HasPermission(range_.permission, accessType)) {
        overlap = 0;
    }
    if (overlap < size) {
        ErrorMsg msg;
        msg.isError = true;
        msg.auxData.badAddr.addr = addr;
        msg.auxData.nBadBytes = size - overlap;
        return msg;
    }
    return ErrorMsg();
}

ErrorMsg UnionBounds::CheckAddrOnly(uint64_t addr) const
{
    if (addr < range_.addrL || addr >= range_.addrR) {
        ErrorMsg msg;
        msg.isError = true;
        msg.auxData.badAddr.addr = addr;
        msg.auxData.nBadBytes = 1;
        return msg;
    }

    return ErrorMsg();
}

BoundsCheck::BoundsCheck(bool localMemoryNeedAlloc) : localMemoryNeedAlloc_{localMemoryNeedAlloc}
{
    bounds_[AddressSpace::GM] = MakeUnique<DiscreteBounds>();
    if (localMemoryNeedAlloc_) {
        bounds_[AddressSpace::L1] = MakeUnique<DiscreteBounds>();
        bounds_[AddressSpace::L0A] = MakeUnique<DiscreteBounds>();
        bounds_[AddressSpace::L0B] = MakeUnique<DiscreteBounds>();
        bounds_[AddressSpace::L0C] = MakeUnique<DiscreteBounds>();
        bounds_[AddressSpace::UB] = MakeUnique<DiscreteBounds>();
    }
}

void BoundsCheck::Init(ChipInfo const &chipInfo)
{
    // 卡切换时除 GM 内存外其他内存都重新初始化
    if (!localMemoryNeedAlloc_) {
        bounds_[AddressSpace::L1] = MakeUnique<UnionBounds>(0, chipInfo.l1Size);
        bounds_[AddressSpace::L0A] = MakeUnique<UnionBounds>(0, chipInfo.l0aSize);
        bounds_[AddressSpace::L0B] = MakeUnique<UnionBounds>(0, chipInfo.l0bSize);
        bounds_[AddressSpace::L0C] = MakeUnique<UnionBounds>(0, chipInfo.l0cSize);
        bounds_[AddressSpace::UB] = MakeUnique<UnionBounds>(0, chipInfo.ubSize);
    }
    bounds_[AddressSpace::PRIVATE] = MakeUnique<UnionBounds>(0, chipInfo.privateSize);
}

ErrorMsg BoundsCheck::Add(AddressSpace space, uint64_t addr, uint64_t size, uint32_t permission)
{
    auto it = bounds_.find(space);
    if (it == bounds_.cend()) {
        SAN_INFO_LOG("Get bounds instance by space %s failed", FormatAddressSpace(space).c_str());
        return ErrorMsg();
    }
    return it->second->Add(addr, size, permission);
}

ErrorMsg BoundsCheck::Remove(AddressSpace space, uint64_t addr, uint64_t size)
{
    auto it = bounds_.find(space);
    if (it == bounds_.cend()) {
        SAN_INFO_LOG("Get bounds instance by space %s failed", FormatAddressSpace(space).c_str());
        return ErrorMsg();
    }
    return it->second->Remove(addr, size);
}

ErrorMsg BoundsCheck::SetPermission(AddressSpace space, uint64_t addr, uint64_t size, uint32_t permission)
{
    auto it = bounds_.find(space);
    if (it == bounds_.cend()) {
        SAN_INFO_LOG("Get bounds instance by space %s failed", FormatAddressSpace(space).c_str());
        return ErrorMsg();
    }
    return it->second->SetPermission(addr, size, permission);
}

ErrorMsg BoundsCheck::Check(AddressSpace space, uint64_t addr, uint64_t size, AccessType accessType) const
{
    auto it = bounds_.find(space);
    if (it == bounds_.cend()) {
        SAN_INFO_LOG("Get bounds instance by space %s failed", FormatAddressSpace(space).c_str());
        return ErrorMsg();
    }
    return it->second->Check(addr, size, accessType);
}

} // namespace Sanitizer
