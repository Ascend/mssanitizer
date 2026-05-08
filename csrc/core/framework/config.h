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


#ifndef __CORE_CONFIG_H__
#define __CORE_CONFIG_H__

#include <cstdint>
#include <vector>
#include <string>

#include "record_defs.h"
#include "utility/log.h"

namespace Sanitizer {

/// 检测工具类型
enum class ToolType : uint8_t {
    MEMCHECK = 0,    // 内存检测
    RACECHECK,       // 竞争检测
    SYNCCHECK,       // 同步检测
    REGISTERCHECK,   // 寄存器默认值检测
    SIZE,
};

enum class DemangleMode : uint8_t {
    FULL_DEMANGLED_NAME = 0,
    SIMPLE_DEMANGLED_NAME,
    MANGLED_NAME,
};

// result of all sanitizers
struct DetectionInfo {
    ToolType type;
    std::string message;
};

constexpr uint16_t PLUGIN_PATH_MAX = 256;

/// 检测算法相关配置
/// 检测工具开始运行时，工具端会将此配置发送给桩函数用于使能相关的桩接口
/// 因此需要保证此结构体为 trivial 类型
struct Config {
    bool defaultCheck;
    bool memCheck;
    bool raceCheck;
    bool initCheck;
    bool syncCheck;
    bool registerCheck;
    bool checkDeviceHeap;
    bool checkCannHeap;
    bool leakCheck;
    bool checkUnusedMemory;
    bool checkCrossNpuRaces;
    bool isPrintFullStack{false};
    int16_t checkBlockId = -1;            // -1代表默认检查所有核的记录
    uint32_t cacheSize = 100;             // 默认大小为100M
    DemangleMode demangleMode{DemangleMode::FULL_DEMANGLED_NAME};
    char pluginPath[PLUGIN_PATH_MAX];
    char kernelName[KERNEL_NAME_MAX];
    char dumpPath[DUMP_PATH_MAX];
};

/// 用户命令结构体，用于承载用户命令行参数的解析结果
struct UserCommand {
    bool printHelpInfo{false};
    bool printVersionInfo{false};
    bool logFileAvailable{true};
    LogLv logLv{LogLv::WARN};
    Config config{};
    std::string logFile;
    std::vector<std::string> cmd;
};

}  // namespace Sanitizer

#endif  // __CORE_CONFIG_H__
