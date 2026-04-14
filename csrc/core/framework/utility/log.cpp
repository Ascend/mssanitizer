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


#include "log.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <unistd.h>
#include "file_system.h"

namespace Sanitizer {
}  // namespace Sanitizer
namespace {

constexpr mode_t DEFAULT_UMASK_FOR_LOG_FILE = 0177;

int64_t GetFirstTimeStamp()
{
    static auto beginUs = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return beginUs;
}

std::string GetLogFilePath()
{
    char buf[Sanitizer::MAX_TIMESTAMP_LENGTH];
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    size_t len = std::strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", std::localtime(&time));
    pid_t pid = getpid();
    return std::string(Sanitizer::OUTPUT_DIR) + "/mssanitizer_" + std::string{buf, len} + "_" + std::to_string(pid) +
           ".log";
}

}

namespace Sanitizer {
Log::Log() : logFilePath_(GetLogFilePath()), maxDebugLogSizeRate_(1024L)
{
    fp_ = OpenLogFile(logFilePath_);
    if (fp_ != nullptr) {
        printf("[mssanitizer] logging to file: %s\n", logFilePath_.c_str());
        fprintf(fp_, "[I] Init log at %s\n", this->GetTimeStamp().c_str());
    } else {
        printf("[mssanitizer] failed to open log file %s, logging disabled.\n", logFilePath_.c_str());
    }
    fflush(stdout);
}

void Log::AppendBuff(const std::string &text)
{
    std::lock_guard<std::mutex> guard(sbuffMtx_);
    sLogBuff_ += text;
}

Log::~Log()
{
    if (fp_ != nullptr) {
        fclose(fp_);
        fp_ = nullptr;
    }
}

Log *Log::GetLog()
{
    static Log instance;
    return &instance;
}

void Log::SetMaxDebugLogSizeRate(long rate)
{
    maxDebugLogSizeRate_ = rate;
}

int64_t Log::LogSize() const
{
    if (fp_ == nullptr) {
        return 0;
    }
    int rt = fseeko(fp_, 0L, SEEK_END);
    if (rt != 0) {
        return -1;
    }
    int64_t size = ftello64(fp_);
    return size;
}

void Log::RotateLogFile()
{
    if (fp_ == nullptr) {
        return;
    }
    fclose(fp_);
    fp_ = nullptr;
    if (rotateCount_ >= MAX_LOG_FILE_NUMBER) {
        printf("[mssanitizer] the number of rotated log files exceeded limit(%ld), remove oldest log.\n",
               MAX_LOG_FILE_NUMBER);
        std::string oldestFile = logFilePath_ + ".old." + std::to_string(MAX_LOG_FILE_NUMBER - 1);
        if (remove(oldestFile.c_str()) != 0) {
            printf("[mssanitizer] failed to remove old log file: %s\n", oldestFile.c_str());
            return;
        }
    }
    for (auto idx = std::min(rotateCount_, MAX_LOG_FILE_NUMBER - 1); idx > 0; idx--) {
        std::string oldPath = idx != 1 ? logFilePath_ + ".old." + std::to_string(idx - 1) : logFilePath_;
        std::string newPath = logFilePath_ + ".old." + std::to_string(idx);
        if (!IsPathExists(oldPath) || IsPathExists(newPath)) {
            continue;
        }
        if (rename(oldPath.c_str(), newPath.c_str()) != 0) {
            printf("[mssanitizer] failed to rotate old log file.\n");
            return;
        }
    }
    fp_ = OpenLogFile(logFilePath_);
    if (fp_ != nullptr) {
        rotateCount_++;
    } else {
        printf("[mssanitizer] failed to open log file: %s\n", logFilePath_.c_str());
    }
}

FILE* Log::OpenLogFile(const std::string &path)
{
    Path logPath(path);
    Path logDirPath = logPath.Parent();
    if (!logDirPath.Exists()) {
        if (mkdir(logDirPath.ToString().c_str(), Sanitizer::DIR_FILE_MODE) != 0) {
            printf("[mssanitizer] ERROR: Failed to create directory (%s). Please make sure current user has write "
                   "permission to the working directory.\n",
                logDirPath.ToString().c_str());
            return nullptr;
        }
    }

    if (!Sanitizer::IsFilePermSafe(logDirPath.ToString(), DIR_FILE_MODE)) {
        return nullptr;
    }
    if (IsSoftLink(logDirPath.ToString())) {
        printf("[mssanitizer] WARNING: The kernel binary (%s) is soft link.\n", logDirPath.ToString().c_str());
    }
    return OpenFileWithUmask(path, "a", DEFAULT_UMASK_FOR_LOG_FILE);
}

std::string Log::GetTimeStamp(void) const
{
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t nowTime = system_clock::to_time_t(now);
    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::gmtime(&nowTime));
    return "[" + std::string(timeBuf) + "]" ;
}

std::string Log::GetRunningTime(void) const
{
    using namespace std::chrono;
    auto start = GetFirstTimeStamp();
    auto now = system_clock::now();
    auto nowUs = duration_cast<microseconds>(now.time_since_epoch()).count();
    auto diffUs = nowUs - start;
    constexpr uint64_t sToUs = 1000000U;
    constexpr uint64_t numOfDigits = 6U;
    std::string fillZero(numOfDigits - std::min(numOfDigits, std::to_string(diffUs % sToUs).size()), '0');
    auto diffUsString = std::to_string(diffUs / sToUs) + "." + fillZero + std::to_string(diffUs % sToUs);

    return "[" + diffUsString + "]";
}
}
