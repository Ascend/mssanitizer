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


#ifndef CORE_FRAMEWORK_COMMAND_H
#define CORE_FRAMEWORK_COMMAND_H

#include <map>
#include <vector>
#include <fstream>
#include <string>
#include <thread>
#include "config.h"
#include "process.h"
#include "checker.h"
#include "thread_manager.h"

namespace Sanitizer {

using MemNameType = std::string;
using GMType = uint64_t;
struct SharerMemInfo {
    GMType addr;
    GMType size;
};
struct ShareeMemInfo {
    GMType addr;
    GMType size;
};

// 按共享内存名称保存提供方范围及各使用方映射；保存实际 Checker，供销毁时向使用方投递 FREE
using SharedMemInfoMpType = std::map<MemNameType, std::pair<SharerMemInfo, std::map<Checker *, ShareeMemInfo>>>;
// 按原设备连接和映射地址查找共享内存名称，避免同一派发线程内不同设备的相同地址混淆
using ShareeMemMpType = std::map<std::pair<std::thread::id, GMType>, MemNameType>;

// Command类主要针对解析后的命令进行处理
// 其本身是串接流程的主要类
class Command {
public:
    Command(Config const &config, bool enableDebugLog, const LogLv &lv, const std::string &logFile);
    using ParamList = std::vector<std::string>;
    ProgramStatus Exec(const ParamList &execParams);
    static SharedMemInfoMpType sharedMemInfoMp;
    // 每个 Host PID 的派发线程持有一份索引，键中的原接收线程 ID 进一步区分设备连接
    thread_local static ShareeMemMpType shareeMemInfoMp;

private:
    Config config_;
    bool enableDebugLog_;
    LogLv loglv_;
    std::string logFile_;
};

void HandleIpcMemRecord(Sanitizer::Checker &checker, Sanitizer::IPCMemRecord const &record,
    Sanitizer::ThreadManager &threadManager, Process::MsgRspFunc &msgRspFunc);

bool DetectDumpProject(Command &cmd, const std::string &dumpPath);
} // namespace Sanitizer


#endif // CORE_FRAMEWORK_COMMAND_H
