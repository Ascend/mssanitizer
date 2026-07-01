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


#include <thread>
#include <iostream>

#include "checker.h"
#include "file_mapping.h"
#include "protocol.h"
#include "utility/ustring.h"
#include "utility/umask_guard.h"
#include "utility/log.h"
#include "utility/cpp_future.h"
#include "thread_manager.h"

namespace Sanitizer {
std::mutex ThreadManager::mutex_;
void PrintFileNames(Sanitizer::FileMapping::FileMapType const &fileMap)
{
    for (auto it = fileMap.cbegin(); it != fileMap.cend(); it++) {
        if (it->first == 0ULL) {
            continue;
        }
        SAN_INFO_LOG("FileIdx:%d FileName:%s", it->second.fileIdx,
                     Utility::ReplaceInvalidChar(it->second.fileName).c_str());
    }
}

Checker &ThreadManager::GetChecker(std::thread::id threadId)
{
    std::lock_guard<std::mutex> guard(mutex_);
    if (checkers_.find(threadId) == checkers_.end()) {
        checkers_[threadId] = MakeUnique<Checker>(config_);
    }
    return *checkers_[threadId];
}

Checker& ThreadManager::GetChecker()
{
    std::lock_guard<std::mutex> guard(mutex_);
    std::thread::id threadId = std::this_thread::get_id();
    if (checkers_.find(threadId) == checkers_.end()) {
        checkers_[threadId] = MakeUnique<Checker>(config_);
        checkers_[threadId]->SetDetectionInfo(loglv_, logFd_);
    }
    return *checkers_[threadId];
}

Protocol &ThreadManager::GetProtocol()
{
    std::lock_guard<std::mutex> guard(mutex_);
    std::thread::id threadId = std::this_thread::get_id();
    if (protocols_.find(threadId) == protocols_.end()) {
        protocols_[threadId] = MakeUnique<MemCheckProtocol>();
    }
    return *protocols_[threadId];
}

std::ostream& ThreadManager::GetLogFd(const std::string &filename)
{
    std::string filenameLog = Utility::ReplaceInvalidChar(filename);
    // 统一父子进程SAN_LOG文件，此处SAN_LOG会将缓冲内容输出并清空，此处不执行会导致缓冲日志重复输出
    SAN_INFO_LOG("Set log level=%u, logfile=%s", static_cast<uint32_t>(loglv_), filenameLog.c_str());
    // 用户指定logFile
    if (!filename.empty()) {
        constexpr mode_t defaultUmaskForLogFile = 0177;
        UmaskGuard guard{defaultUmaskForLogFile};
        static std::ofstream logFd(filename, std::ofstream::out | std::ofstream::trunc);
        // 可写检查
        if (logFd.is_open()) {
            return logFd;
        } else {
            std::cout << "[ERROR] Failed to open the assigned log file. Please check the access." << std::endl;
        }
    }
    return std::cout;
}

void ThreadManager::ThreadFinish()
{
    std::lock_guard<std::mutex> guard(mutex_);
    checkers_.clear();
    protocols_.clear();

    if (config_.raceCheck && config_.checkCrossNpuRaces) {
        crossNpuChecker_.SetDetectionInfo(loglv_, logFd_);
        crossNpuChecker_.Run();
    }
}

void ThreadManager::PostLog() const
{
    PrintFileNames(FileMapping::Instance().GetFileMap());
    SAN_INFO_LOG("Log ended at %s", log.GetRunningTime().c_str());
}
} // namespace Sanitizer
