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
// 根据ASCEND C口径：栈内存按75、32、18（分别对应910B、910A、310P）个核计算，一个核按最大32KB计算（算子编译时可按16/32K配置）
constexpr uint32_t ASCEND_910B_PRIVATE_SIZE = 75 * 32 * 1024;
constexpr uint32_t ASCEND_910A_PRIVATE_SIZE = 32 * 32 * 1024;
constexpr uint32_t ASCEND_310P_PRIVATE_SIZE = 18 * 32 * 1024;
constexpr uint32_t ASCEND_A5_PRIVATE_SIZE = 96 * 32 * 1024;
constexpr uint32_t KB_TO_BYTES = 1024;
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
    {DeviceType::ASCEND_910_PREMIUM_A, {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 1024 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_910A_PRIVATE_SIZE, 32 * GB_TO_BYTES}},
    {DeviceType::ASCEND_910B1, {24U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 128 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        192 * KB_TO_BYTES, 32U, ASCEND_910B_PRIVATE_SIZE, 64 * GB_TO_BYTES}},
    {DeviceType::ASCEND_910B2, {24U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 128 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        192 * KB_TO_BYTES, 32U, ASCEND_910B_PRIVATE_SIZE, 64 * GB_TO_BYTES}},
    {DeviceType::ASCEND_910B3, {20U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 128 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        192 * KB_TO_BYTES, 32U, ASCEND_910B_PRIVATE_SIZE, 64 * GB_TO_BYTES}},
    {DeviceType::ASCEND_910B4, {20U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 128 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        192 * KB_TO_BYTES, 32U, ASCEND_910B_PRIVATE_SIZE, 64 * GB_TO_BYTES}},
    {DeviceType::ASCEND_310P, {8U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 1024 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_310P_PRIVATE_SIZE, 32 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_950x, {8U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 24 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_950y, {8U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 16 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950PR_950z, {8U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 16 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9571, {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 144 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9572, {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 96 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9573, {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 128 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9574, {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 84 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9575, {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 96 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9576, {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 64 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9577, {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 84 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9578, {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 56 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950PR_9579, {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 128 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950PR_957b, {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 112 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950PR_957c, {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 84 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950PR_957d, {28U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 96 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9581, {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 144 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9582, {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 96 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9583, {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 126 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9584, {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 84 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9585, {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 96 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9586, {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 64 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9587, {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 84 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9588, {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 56 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950PR_9589, {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 128 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950PR_958b, {32U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 112 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9591, {36U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 144 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9592, {36U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 96 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9595, {36U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 96 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_9596, {36U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 64 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950PR_9599, {36U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 128 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_95A1, {36U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 128 * GB_TO_BYTES}},
    {DeviceType::ASCEND_950DT_95A2, {36U, 64 * KB_TO_BYTES, 64 * KB_TO_BYTES, 256 * KB_TO_BYTES, 512 * KB_TO_BYTES,
        256 * KB_TO_BYTES, 32U, ASCEND_A5_PRIVATE_SIZE, 128 * GB_TO_BYTES}},
};

const std::unordered_map<std::string, DeviceType> SOC_VERSION_MAP = {
    {"Ascend910PremiumA", DeviceType::ASCEND_910_PREMIUM_A},
    {"Ascend910B1", DeviceType::ASCEND_910B1},
    {"Ascend910B2", DeviceType::ASCEND_910B2},
    {"Ascend910B2C", DeviceType::ASCEND_910B2},
    {"Ascend910B3", DeviceType::ASCEND_910B3},
    {"Ascend910B4", DeviceType::ASCEND_910B4},
    {"Ascend910B4-1", DeviceType::ASCEND_910B4},
    {"Ascend910_9391", DeviceType::ASCEND_910B1},
    {"Ascend910_9392", DeviceType::ASCEND_910B1},
    {"Ascend910_9381", DeviceType::ASCEND_910B2},
    {"Ascend910_9382", DeviceType::ASCEND_910B2},
    {"Ascend910_9372", DeviceType::ASCEND_910B3},
    {"Ascend910_9362", DeviceType::ASCEND_910B4},
    {"Ascend950DT_950x",  DeviceType::ASCEND_950DT_950x},
    {"Ascend950DT_950y",  DeviceType::ASCEND_950DT_950y},
    {"Ascend950DT_9571",  DeviceType::ASCEND_950DT_9571},
    {"Ascend950DT_9572",  DeviceType::ASCEND_950DT_9572},
    {"Ascend950DT_9573",  DeviceType::ASCEND_950DT_9573},
    {"Ascend950DT_9574",  DeviceType::ASCEND_950DT_9574},
    {"Ascend950DT_9575",  DeviceType::ASCEND_950DT_9575},
    {"Ascend950DT_9576",  DeviceType::ASCEND_950DT_9576},
    {"Ascend950DT_9577",  DeviceType::ASCEND_950DT_9577},
    {"Ascend950DT_9578",  DeviceType::ASCEND_950DT_9578},
    {"Ascend950DT_9581",  DeviceType::ASCEND_950DT_9581},
    {"Ascend950DT_9582",  DeviceType::ASCEND_950DT_9582},
    {"Ascend950DT_9583",  DeviceType::ASCEND_950DT_9583},
    {"Ascend950DT_9584",  DeviceType::ASCEND_950DT_9584},
    {"Ascend950DT_9585",  DeviceType::ASCEND_950DT_9585},
    {"Ascend950DT_9586",  DeviceType::ASCEND_950DT_9586},
    {"Ascend950DT_9587",  DeviceType::ASCEND_950DT_9587},
    {"Ascend950DT_9588",  DeviceType::ASCEND_950DT_9588},
    {"Ascend950DT_9591",  DeviceType::ASCEND_950DT_9591},
    {"Ascend950DT_9592",  DeviceType::ASCEND_950DT_9592},
    {"Ascend950DT_9595",  DeviceType::ASCEND_950DT_9595},
    {"Ascend950DT_9596",  DeviceType::ASCEND_950DT_9596},
    {"Ascend950DT_95A1",  DeviceType::ASCEND_950DT_95A1},
    {"Ascend950DT_95A2",  DeviceType::ASCEND_950DT_95A2},
    {"Ascend950PR_950z",  DeviceType::ASCEND_950PR_950z},
    {"Ascend950PR_9579",  DeviceType::ASCEND_950PR_9579},
    {"Ascend950PR_957b",  DeviceType::ASCEND_950PR_957b},
    {"Ascend950PR_957c",  DeviceType::ASCEND_950PR_957c},
    {"Ascend950PR_957d",  DeviceType::ASCEND_950PR_957d},
    {"Ascend950PR_9589",  DeviceType::ASCEND_950PR_9589},
    {"Ascend950PR_958b",  DeviceType::ASCEND_950PR_958b},
    {"Ascend950PR_9599",  DeviceType::ASCEND_950PR_9599},
    {"Ascend310P1", DeviceType::ASCEND_310P},
    {"Ascend310P3", DeviceType::ASCEND_310P},
    {"Ascend310P5", DeviceType::ASCEND_310P},
    {"Ascend310P7", DeviceType::ASCEND_310P},
};

const std::unordered_map<std::string, uint16_t> ALIGN_MAP = {
    {"UB_NORMAL_ALIGN_SIZE", 32U},
    {"L0A_L0B_L0C_NORMAL_ALIGN_SIZE",  512U},
    {"L1_NORMAL_ALIGN_SIZE", 32U},
    {"VMRGSORET4_ALIGN_SIZE", 8U},
};

struct CacheLineSpec {
    std::size_t cacheLineSize;
};

const std::unordered_map<DeviceType, CacheLineSpec> CACHE_LINE_SPEC_MAP = {
    {DeviceType::ASCEND_910B1, {256}},
    {DeviceType::ASCEND_910B2, {256}},
    {DeviceType::ASCEND_910B3, {256}},
    {DeviceType::ASCEND_910B4, {256}},
};

} // namespace Sanitizer

#endif
