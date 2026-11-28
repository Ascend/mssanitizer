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


#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <thread>
#include <memory>

#include "record_defs.h"

namespace Sanitizer {

// 收包线程轮流解析多个设备连接，解码状态必须随设备连接上下文切换，不能使用thread_local仅按线程保存
// 否则在连接 A 尚未完成时线程开始处理连接 B，B 的初始化可能把 A 的状态清掉，造成 A 的完成计数错误
struct KernelDecodeState {
    uint8_t vecSubBlockDim{};
    uint64_t totalBlockDim{};
    std::vector<std::vector<uint8_t>> dynamicMemorys;
    RecordType lastRecordType{RecordType::BLOCK_FINISH};
};

class RuntimeContext {
public:
    static RuntimeContext &Instance()
    {
        static thread_local RuntimeContext inst;
        return inst;
    }

    int32_t GetDeviceId() const
    {
        return deviceSummary_.deviceId;
    }

public:
    // 使用共享所有权避免调度器保存/恢复上下文时复制动态记录并使事件内的地址失效
    std::shared_ptr<KernelDecodeState> kernelDecodeState_{std::make_shared<KernelDecodeState>()};
    DeviceInfoSummary deviceSummary_ {};
    KernelSummary kernelSummary_{};
    uint32_t kernelIdx_{};
    uint32_t currentBlockIdx_ {};
    uint64_t serialNo_ {};
    // 当前连接所属的被测 Host 进程 PID；-1 表示身份未知
    int32_t pid_{-1};
    bool determined_ {};
    MemInfoSrc memInfoSrc_ {};
    std::thread::id rootTid_{std::this_thread::get_id()};  // current thread get resource from which thread id
    std::vector<HostMemRecord> cachedRecords;
    std::string kernelNameDisplay{};
};

} // namespace Sanitizer
