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

#ifndef CORE_FRAMEWORK_PEER_ACCESS_MANAGER_H
#define CORE_FRAMEWORK_PEER_ACCESS_MANAGER_H

#include <cstdint>
#include <map>
#include <mutex>
#include <set>

#include "record_defs.h"

namespace Sanitizer {

class AddressSanitizer;
class BoundsCheck;

// 保存每个应用进程的设备访问关系，并借用各设备现有的 GM 范围检查能力，不负责跨设备记录的顺序回放
class PeerAccessManager {
public:
    static PeerAccessManager &Instance();
    std::recursive_mutex &Mutex() { return mutex_; }
    // 更新 pid 对应 Host 进程的授权关系：deviceId 为访问方，peerDeviceId 为目标方
    bool Apply(int32_t pid, const PeerAccessRecord &record);
    // 登记同一 Host 进程内的设备检测对象及其两套内存范围表（运行库记录、用户标注）
    void RegisterDevice(int32_t pid, int32_t deviceId, AddressSanitizer *sanitizer, const BoundsCheck *runtimeBounds,
        const BoundsCheck *dfxBounds);
    // 对象销毁或重新初始化前撤销登记，避免查询悬空对象
    void UnregisterDevice(AddressSanitizer *sanitizer);
    // 本地 GM 检查失败且完全不与本地范围相交时，检查任一获授权目标设备是否完整允许该读写访问
    bool CheckRemote(const BoundsCheck &localBounds, uint64_t addr, uint64_t size, AccessType accessType);
    // 清空本次被测程序的设备登记和授权关系
    void Clear();

private:
    // 保存一个 Host 进程的设备授权关系和可查询的内存检测对象
    struct ProcessState {
        // 有向关系：访问方设备号 -> 允许访问的目标设备号
        std::map<int32_t, std::set<int32_t>> relations;
        // 设备号 -> 本设备的内存检测对象，供远端访问查询目标设备的有效 GM 范围和权限
        // 仅保存指针，不负责对象的创建和销毁；对象销毁前必须撤销登记
        std::map<int32_t, AddressSanitizer *> devices;
    };
    // 记录内存范围表所属的 Host 进程、访问设备及内存检测对象
    struct Source {
        int32_t pid;
        int32_t deviceId;
        AddressSanitizer *sanitizer;
    };
    // 同一把锁保护关系、设备对象登记及目标设备的内存范围表，访存检查持锁时可能再次查询本类，因此使用递归锁
    std::recursive_mutex mutex_;
    // Host PID -> 该进程的授权关系和设备检测对象表，隔离不同进程的跨设备查询状态
    std::map<int32_t, ProcessState> processes_;
    // AsanLoad/AsanStore 只有当前内存范围表，借其地址找所属进程和访问方设备
    std::map<const BoundsCheck *, Source> sources_;
};

} // namespace Sanitizer

#endif
