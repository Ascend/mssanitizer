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

#include "record_defs.h"

namespace Sanitizer {

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
    DeviceInfoSummary deviceSummary_ {};
    KernelSummary kernelSummary_{};
    uint32_t kernelIdx_{};
    uint32_t currentBlockIdx_ {};
    uint64_t serialNo_ {};
    bool determined_ {};
    MemInfoSrc memInfoSrc_ {};
    std::thread::id rootTid_{std::this_thread::get_id()};  // current thread get resource from which thread id
    std::vector<HostMemRecord> cachedRecords;
    std::string kernelNameDisplay{};
};

} // namespace Sanitizer
