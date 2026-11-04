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
            relations[record.deviceId].erase(record.peerDeviceId);
        }
        return true;
    case PeerAccessOp::RESET:
        if (record.peerDeviceId != -1) {
            return false;
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
