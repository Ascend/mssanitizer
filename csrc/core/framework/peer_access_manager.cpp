/* -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include "peer_access_manager.h"
#include "address_sanitizer/address_sanitizer.h"

#include <limits>
#include <algorithm>
#include <tuple>

namespace Sanitizer {

bool PeerAccessManager::Apply(int32_t pid, const PeerAccessRecord &record) {
    if (pid < 0 || record.deviceId < 0) {
        return false;
    }
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto &relations = processes_[pid].relations;
    switch (record.operation) {
    case PeerAccessOp::ENABLE:
    case PeerAccessOp::DISABLE:
        if (record.peerDeviceId < 0 || record.peerDeviceId == record.deviceId) {
            return false;
        }
        if (record.operation == PeerAccessOp::ENABLE) {
            relations[record.deviceId].insert(record.peerDeviceId);
        } else {
            auto &mappings = processes_[pid].mappings;
            mappings.erase(std::remove_if(mappings.begin(), mappings.end(), [&record](const P2PMappingRecord &m) {
                return m.deviceId == record.deviceId && m.ownerDeviceId == record.peerDeviceId;
            }), mappings.end());
            relations[record.deviceId].erase(record.peerDeviceId);
        }
        return true;
    case PeerAccessOp::RESET:
        if (record.peerDeviceId != -1) {
            return false;
        }
        {
            auto &mappings = processes_[pid].mappings;
            mappings.erase(std::remove_if(mappings.begin(), mappings.end(), [&record](const P2PMappingRecord &m) {
                return m.deviceId == record.deviceId || m.ownerDeviceId == record.deviceId;
            }), mappings.end());
        }
        // 重置设备既不能继续访问其他设备，也不能继续被其他设备访问。
        relations.erase(record.deviceId);
        for (auto &item : relations) {
            item.second.erase(record.deviceId);
        }
        return true;
    default:
        return false;
    }
}

bool PeerAccessManager::ApplyMapping(int32_t pid, const P2PMappingRecord &record)
{
    if (pid < 0 || record.deviceId < 0 || record.ownerDeviceId < 0 || record.deviceId == record.ownerDeviceId ||
        record.addr == 0 || record.size == 0 || record.addr > UINT64_MAX - record.size ||
        record.ownerAddr > record.addr) {
        return false;
    }
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    // 确保映射的内存空间真实存在于被访问设备的已经登记分配内存内
    auto &process = processes_[pid];
    auto owner = process.devices.find(record.ownerDeviceId);
    if (owner == process.devices.end() || !owner->second->ContainsGmRange(record.addr, record.size)) {
        return false;
    }

    // 去重后push_back
    const auto same = [&record](const P2PMappingRecord &m) {
        return m.deviceId == record.deviceId && m.ownerDeviceId == record.ownerDeviceId &&
            m.addr == record.addr && m.size == record.size && m.ownerAddr == record.ownerAddr;
    };
    if (std::find_if(process.mappings.begin(), process.mappings.end(), same) == process.mappings.end()) {
        // 按使用方、所有方、窗口起点排序，访存检查通过一次连续扫描合并窗口
        const auto less = [](const P2PMappingRecord &a, const P2PMappingRecord &b) {
            return std::tie(a.deviceId, a.ownerDeviceId, a.addr) < std::tie(b.deviceId, b.ownerDeviceId, b.addr);
        };
        auto position = std::lower_bound(process.mappings.begin(), process.mappings.end(), record, less);
        process.mappings.insert(position, record);
    }
    return true;
}

void PeerAccessManager::RevokeMappingsOfAllocation(int32_t pid, int32_t ownerDeviceId, uint64_t ownerAddr) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto process = processes_.find(pid);
    if (process == processes_.end()) {
        return;
    }

    // 按"所有方设备 + 原分配起点"删除全部相关映射
    auto &mappings = process->second.mappings;
    mappings.erase(std::remove_if(mappings.begin(), mappings.end(),
                       [ownerDeviceId, ownerAddr](const P2PMappingRecord &m) {
                           return m.ownerDeviceId == ownerDeviceId && m.ownerAddr == ownerAddr;
                       }),
        mappings.end());
}

void PeerAccessManager::RegisterDevice(int32_t pid, int32_t deviceId, AddressSanitizer *sanitizer,
    const BoundsCheck *runtimeBounds, const BoundsCheck *dfxBounds) {
    if (pid < 0 || deviceId < 0 || sanitizer == nullptr || runtimeBounds == nullptr || dfxBounds == nullptr) {
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    processes_[pid].devices[deviceId] = sanitizer;
    sources_[runtimeBounds] = {pid, deviceId, sanitizer};
    sources_[dfxBounds] = {pid, deviceId, sanitizer};
}

void PeerAccessManager::UnregisterDevice(AddressSanitizer *sanitizer) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (auto &process : processes_) {
        for (auto device = process.second.devices.begin(); device != process.second.devices.end();) {
            // 同一设备的新对象不能被旧对象的析构误删。
            if (device->second == sanitizer) {
                const int32_t id = device->first;
                auto &mappings = process.second.mappings;
                mappings.erase(std::remove_if(mappings.begin(), mappings.end(), [id](const P2PMappingRecord &m) {
                    return m.deviceId == id || m.ownerDeviceId == id;
                }), mappings.end());
                device = process.second.devices.erase(device);
            } else {
                ++device;
            }
        }
    }
    for (auto source = sources_.begin(); source != sources_.end();) {
        if (source->second.sanitizer == sanitizer) {
            source = sources_.erase(source);
        } else {
            ++source;
        }
    }
}

bool PeerAccessManager::CheckRemote(
    const BoundsCheck &localBounds, uint64_t addr, uint64_t size, AccessType accessType) {
    if (size == 0 || addr > std::numeric_limits<uint64_t>::max() - size) {
        return false;
    }
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto source = sources_.find(&localBounds);
    if (source == sources_.end()) {
        return false;
    }
    auto process = processes_.find(source->second.pid);
    if (process == processes_.end()) {
        return false;
    }
    // 精确映射独立于设备级授权；同一所有方设备的多个窗口可以合并覆盖本次访问
    const uint64_t end = addr + size;
    const auto &mappings = process->second.mappings;
    auto mapping = std::lower_bound(mappings.begin(), mappings.end(), source->second.deviceId,
        [](const P2PMappingRecord &m, int32_t device) { return m.deviceId < device; });
    int32_t ownerId = -1;
    bool ownerAllows = false;
    uint64_t covered = addr;
    for (; mapping != mappings.end() && mapping->deviceId == source->second.deviceId; ++mapping) {
        if (ownerId != mapping->ownerDeviceId) {
            ownerId = mapping->ownerDeviceId;
            auto owner = process->second.devices.find(ownerId);
            // 每个所有方检查一次当前权限，其窗口随后按起点连续推进
            ownerAllows =
                owner != process->second.devices.end() && owner->second->CheckExistingGmRange(addr, size, accessType);
            covered = addr;
        }
        if (!ownerAllows) {
            continue;
        }
        if (mapping->addr > covered) {
            ownerAllows = false; // 后续窗口起点更大，当前所有方的空洞无法补齐
            continue;
        }
        covered = std::max(covered, mapping->addr + mapping->size);
        if (covered >= end) {
            return true;
        }
    }
    auto relation = process->second.relations.find(source->second.deviceId);
    if (relation == process->second.relations.end()) {
        return false;
    }
    // 一个获授权的目标设备必须独立覆盖完整访问范围；不拼接多个设备的区间。
    for (int32_t peer : relation->second) {
        auto device = process->second.devices.find(peer);
        if (device == process->second.devices.end()) {
            continue;
        }
        if (device->second->CheckExistingGmRange(addr, size, accessType)) {
            return true;
        }
    }
    return false;
}

PeerAccessManager &PeerAccessManager::Instance() {
    static PeerAccessManager manager;
    return manager;
}

void PeerAccessManager::Clear() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    processes_.clear();
    sources_.clear();
}

} // namespace Sanitizer
