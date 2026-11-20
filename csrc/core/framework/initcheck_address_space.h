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

#ifndef CORE_FRAMEWORK_INITCHECK_ADDRESS_SPACE_H
#define CORE_FRAMEWORK_INITCHECK_ADDRESS_SPACE_H

#include "config.h"
#include "event_def.h"
#include "format_converter.h"

namespace Sanitizer {

enum class InitcheckFilterScope {
    SHARED_EVENTS,            // 解析结果供所有检测工具共享。
    ADDRESS_SANITIZER_EVENTS, // 仅供初始化和内存检测消费。
    INITIALIZATION_CHECK,    // 仅选择初始化检测的 shadow memory 操作。
};

// 不同阶段需要保留的事件不同；统一在此决定有效掩码，避免各调用方重复工具门控。
inline uint32_t GetEffectiveInitcheckAddressSpaceMask(
    const Config &config, uint32_t mask, InitcheckFilterScope scope)
{
    if (scope == InitcheckFilterScope::INITIALIZATION_CHECK) {
        return config.initCheck ? mask : 0U;
    }
    if (!config.initCheck || config.memCheck ||
        (scope == InitcheckFilterScope::SHARED_EVENTS &&
            (config.raceCheck || config.syncCheck || config.registerCheck))) {
        return INITCHECK_ALL_ADDRESS_SPACES;
    }
    return mask;
}

inline bool IsInitcheckAddressSpaceEnabled(uint32_t mask, AddressSpace space)
{
    return (mask & InitcheckAddressSpaceBit(space)) != 0;
}

// 掩码由调用入口确定。只过滤内存访问；同步、控制和结束事件必须保留。
inline bool ShouldCollectInitcheckEvent(uint32_t mask, const SanEvent &event)
{
    if (mask == INITCHECK_ALL_ADDRESS_SPACES) {
        return true;
    }
    if (event.type == EventType::MEM_EVENT) {
        return IsInitcheckAddressSpaceEnabled(mask,
            FormatConverter::MemTypeToAddrSpace(event.eventInfo.memInfo.memType));
    }
    if (event.type == EventType::DYNAMIC_MEM_EVENT &&
        event.eventInfo.dynamicOpInfo.dynamicType == RecordType::SIMT_ENTRY) {
        return IsInitcheckAddressSpaceEnabled(mask,
            FormatConverter::MemTypeToAddrSpace(event.eventInfo.dynamicOpInfo.memType));
    }
    return true;
}

} // namespace Sanitizer
#endif
