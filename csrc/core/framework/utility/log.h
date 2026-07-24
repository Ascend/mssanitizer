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


#ifndef CORE_FRAMEWORK_LOG_H
#define CORE_FRAMEWORK_LOG_H

#include <mutex>
#include <string>
#include <cstdio>
#include <cstdint>
#include <thread>
#include <unistd.h>
#include <sstream>
#include <sys/syscall.h>

#include "securec.h"

namespace Sanitizer {
enum class LogLv : uint8_t {
    DEBUG = 0U,
    INFO,
    WARN,
    ERROR,
};

inline bool operator<(const LogLv &a, const LogLv &b)
{
    return static_cast<uint8_t>(a) < static_cast<uint8_t>(b);
}

constexpr int64_t MAX_LOG_FILE_SIZE = 1024L * 1024L;
constexpr long MAX_LOG_FILE_NUMBER = 10L;
constexpr int16_t LOG_BUF_SIZE = 4096;
constexpr uint64_t MAX_PIPE_OUTPUT_SIZE = 1UL << 33; // 8GB
constexpr uint16_t MAX_FILE_PATH_LEN = 4096U;
constexpr size_t MAX_TIMESTAMP_LENGTH = 256U;
constexpr const char* OUTPUT_DIR = "./mindstudio_sanitizer_log";

class Log {
public:
    static Log *GetLog();

    void AppendBuff(const std::string &text);

    FILE *GetLogFp(void)
    {
        std::lock_guard<std::mutex> guard(mtx_);
        if (LogSize() >= MAX_LOG_FILE_SIZE * maxDebugLogSizeRate_) {
            RotateLogFile();
        }
        std::lock_guard<std::mutex> buffGuard(sbuffMtx_);
        if (!sLogBuff_.empty() && fp_) {
            fprintf(fp_, "%s", sLogBuff_.c_str());
            sLogBuff_.clear();
        }
        return fp_;
    }
    std::string GetTimeStamp(void) const;
    std::string GetRunningTime(void) const;
    static uint64_t GetThreadId(void)
    {
        thread_local uint64_t tid = static_cast<uint64_t>(syscall(SYS_gettid));
        return tid;
    }

    void SetMaxDebugLogSizeRate(long rate);

private:
    Log();
    ~Log();

    Log(Log &) = delete;
    Log(const Log &) = delete;
    Log &operator=(Log &) = delete;
    Log &operator=(const Log &) = delete;
    FILE* OpenLogFile(const std::string &path);
    void RotateLogFile();
    int64_t LogSize() const;

    FILE *fp_ { nullptr };
    std::string logFilePath_;
    long rotateCount_ = 1;
    mutable std::mutex mtx_{};
    std::string sLogBuff_;
    std::mutex sbuffMtx_;

    long maxDebugLogSizeRate_;
};
}

// sanitizer internal log macros

// 日志格式：[时间戳] [线程号] [文件名:行号] 日志信息
// 因为首次调用会生成日志文件，所以在执行用户命令成功后再调用来记录有效的日志。
#define SAN_LOG(format, ...)                                                                 \
    do {                                                                                     \
        Sanitizer::Log &log = *Sanitizer::Log::GetLog();                                     \
        FILE *fp = log.GetLogFp();                                                           \
        if (fp != nullptr) {                                                                 \
            fprintf(fp, "%s [%lu] [%s:%d] " format "\n",                                    \
                    log.GetTimeStamp().c_str(),                                              \
                    Sanitizer::Log::GetThreadId(),                                           \
                    __FILENAME__, __LINE__,                                                       \
                    ##__VA_ARGS__);                                                          \
            fflush(fp);                                                                      \
        }                                                                                    \
    } while (0)

#define SAN_ERROR_LOG(format, ...) SAN_LOG("[E] " format, ##__VA_ARGS__)
#define SAN_WARN_LOG(format, ...) SAN_LOG("[W] " format, ##__VA_ARGS__)
#define SAN_INFO_LOG(format, ...) SAN_LOG("[I] " format, ##__VA_ARGS__)

// 此宏函数用来将内容存放在缓冲中，待下次调用SAN_LOG时将缓冲内容输出并清空。用于
// 缓存执行用户cmd前对参数解析的日志文本。
#define SAN_BUFF_LOG(format, ...)                                                             \
    {                                                                                         \
        char buf[LOG_BUF_SIZE]{0};                                                            \
        int ret = sprintf_s(buf, LOG_BUF_SIZE, "%s [%lu] [%s:%d] " format "\n",              \
                           Sanitizer::Log::GetLog()->GetTimeStamp().c_str(),                  \
                           Sanitizer::Log::GetThreadId(),                                     \
                           __FILENAME__, __LINE__,                                                 \
                           ##__VA_ARGS__);                                                    \
        if (ret != -1) {                                                                      \
            Sanitizer::Log::GetLog()->AppendBuff(buf);                                        \
        }                                                                                     \
    }

#define SAN_BUFF_ERROR_LOG(format, ...) SAN_BUFF_LOG("[E] " format, ##__VA_ARGS__)
#define SAN_BUFF_WARN_LOG(format, ...) SAN_BUFF_LOG("[W] " format, ##__VA_ARGS__)
#define SAN_BUFF_INFO_LOG(format, ...) SAN_BUFF_LOG("[I] " format, ##__VA_ARGS__)

#endif
