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


#ifndef CORE_FRAMEWORK_PLATFORM_CONFIG_H
#define CORE_FRAMEWORK_PLATFORM_CONFIG_H

#include <cstdint>
#include <iostream>
#include <unordered_map>

#include "arch_def.h"

namespace Sanitizer {
constexpr uint32_t UB_ALIGN_SIZE = 32U;
constexpr uint32_t KB_TO_BYTES = 1024;
constexpr uint32_t AI_CORE_STACK_SIZE = 32 * KB_TO_BYTES; // 默认每个aicore的stack size
// 根据ASCEND C口径：栈内存按75、32、18（分别对应910B、910A、310P）个核计算，一个核按最大32KB计算（算子编译时可按16/32K配置）
constexpr uint32_t ASCEND_910B_PRIVATE_SIZE = 75 * AI_CORE_STACK_SIZE;
constexpr uint32_t ASCEND_910A_PRIVATE_SIZE = 32 * AI_CORE_STACK_SIZE;
constexpr uint32_t ASCEND_310P_PRIVATE_SIZE = 18 * AI_CORE_STACK_SIZE;
constexpr uint32_t ASCEND_950_PRIVATE_SIZE = 108 * AI_CORE_STACK_SIZE;
constexpr uint32_t GB_TO_BYTES = 1024 * 1024 * 1024;

struct ChipInfo {
    uint32_t aiCoreCnt;
    uint32_t l0aSize;
    uint32_t l0bSize;
    uint32_t l0cSize;
    uint32_t l1Size;
    uint32_t ubSize;
    uint32_t ubBlockSize;
    uint32_t privateSize;
    uint64_t hbmSize;
};

inline bool HasSubBlocks(DeviceType deviceType)
{
    if (deviceType > DeviceType::ASCEND_910B_START && deviceType < DeviceType::ASCEND_910B_END) {
        return true;
    } else if (deviceType > DeviceType::ASCEND_950_START && deviceType < DeviceType::ASCEND_950_END) {
        return true;
    }
    return false;
}

inline bool IsAscend95(DeviceType deviceType)
{
    return deviceType > DeviceType::ASCEND_950_START && deviceType < DeviceType::ASCEND_950_END;
}

/// 芯片内存信息，通过 ${ASCEND_HOME_PATH}/compiler/data/platform_config 路径获取
const std::unordered_map<DeviceType, ChipInfo> CHIP_INFO_MAP = {
    {DeviceType::ASCEND_910_PREMIUM_A,
        {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 1024 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_910A_PRIVATE_SIZE, 32 * GB_TO_BYTES}},
    {DeviceType::ASCEND_910B1,
        {24U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 128 * KB_TO_BYTES, 512 * KB_TO_BYTES, 192 * KB_TO_BYTES, 32U,
            ASCEND_910B_PRIVATE_SIZE, 64 * GB_TO_BYTES}},
    {DeviceType::ASCEND_910B2,
        {24U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 128 * KB_TO_BYTES, 512 * KB_TO_BYTES, 192 * KB_TO_BYTES, 32U,
            ASCEND_910B_PRIVATE_SIZE, 64 * GB_TO_BYTES}},
    {DeviceType::ASCEND_910B3,
        {20U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 128 * KB_TO_BYTES, 512 * KB_TO_BYTES, 192 * KB_TO_BYTES, 32U,
            ASCEND_910B_PRIVATE_SIZE, 64 * GB_TO_BYTES}},
    {DeviceType::ASCEND_910B4,
        {20U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 128 * KB_TO_BYTES, 512 * KB_TO_BYTES, 192 * KB_TO_BYTES, 32U,
            ASCEND_910B_PRIVATE_SIZE, 64 * GB_TO_BYTES}},
    {DeviceType::ASCEND_310P,
        {8U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 1024 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_310P_PRIVATE_SIZE, 32 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_950x,
        {8U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 24 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_950y,
        {8U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 16 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950PR_950z,
        {8U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 24 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9571,
        {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 144 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9572,
        {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 96 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9573,
        {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 126 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9574,
        {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 84 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9575,
        {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 96 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9576,
        {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 64 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9577,
        {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 84 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9578,
        {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 56 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950PR_9579,
        {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 128 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950PR_957b,
        {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 112 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950PR_957c,
        {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 84 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950PR_957d,
        {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 96 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9581,
        {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 144 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9582,
        {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 96 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9583,
        {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 126 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9584,
        {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 84 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9585,
        {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 96 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9586,
        {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 64 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9587,
        {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 84 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9588,
        {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 56 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950PR_9589,
        {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 128 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950PR_958b,
        {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 112 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9591,
        {36U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 144 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9592,
        {36U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 96 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9595,
        {36U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 96 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9596,
        {36U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 64 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950PR_9599,
        {36U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 128 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_95A1,
        {36U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 128 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_95A2,
        {36U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 96 * GB_TO_BYTES}},
    // 前缀兜底采用同系列代表规格，不表示未知型号的实际核数或 HBM 容量。
    {DeviceType::ASCEND_910B_DEFAULT,
        {24U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 128 * KB_TO_BYTES, 512 * KB_TO_BYTES, 192 * KB_TO_BYTES, 32U,
            ASCEND_910B_PRIVATE_SIZE, 64ULL * GB_TO_BYTES}},
    {DeviceType::ASCEND_950_DEFAULT,
        {36U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES, 256 * KB_TO_BYTES, 32U,
            ASCEND_950_PRIVATE_SIZE, 128ULL * GB_TO_BYTES}},
};

const std::unordered_map<std::string, uint16_t> ALIGN_MAP = {
    {"UB_NORMAL_ALIGN_SIZE", 32U},
    {"L0A_L0B_L0C_NORMAL_ALIGN_SIZE",  512U},
    {"L1_NORMAL_ALIGN_SIZE", 32U},
    {"VMRGSORET4_ALIGN_SIZE", 8U},
};
} // namespace Sanitizer

#endif
