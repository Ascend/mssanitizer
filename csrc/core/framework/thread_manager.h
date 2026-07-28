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


#ifndef CORE_FRAMEWORK_THREADMANAGER_H
#define CORE_FRAMEWORK_THREADMANAGER_H

#include <string>
#include <memory>
#include <unordered_map>
#include <thread>
#include "cross_npu_checker.h"
#include "config.h"
#include "utility/log.h"
#include "checker.h"
#include "protocol.h"

namespace Sanitizer {
// ThreadManager类主要是为了管理线程，fork拉起用户进程之后，会根据client数量创建多个线程并行处理
// 在线程开始之前PreLog，用户进程结束之后ThreadFinish和PostLog
// ThreadManager类持有detectionInfo，所有线程检测出的异常信息都按照线程id保存在detectionInfo中
// ThreadManager类持有checkers_和protocols_，按照线程id进行创建和获取checker对象与Protocol指针
class ThreadManager {
public:
    ThreadManager(Config const &config, bool enableDebugLog, const LogLv &lv, const std::string &logFile)
        : config_{config}, enableDebugLog_{enableDebugLog}, loglv_(lv), logFd_(GetLogFd(logFile)),
          crossNpuChecker_(config) {}
    void PostLog() const;
    Checker& GetChecker(std::thread::id threadId);
    Checker& GetChecker();
    CrossNpuChecker &GetCrossNpuChecker() { return crossNpuChecker_; }
    Protocol &GetProtocol();
    void ThreadFinish();
    int32_t GetCheckersNum() const { return checkers_.size(); }
    int32_t GetProtocolsNum() const { return protocols_.size(); }
private:
    std::ostream &GetLogFd(const std::string &filename);
    std::unordered_map<std::thread::id, std::unique_ptr<Checker>> checkers_;
    std::unordered_map<std::thread::id, std::unique_ptr<Protocol>> protocols_;
    // 接收从command类中传入的参数
    Config config_;
    bool enableDebugLog_;
    LogLv loglv_;
    std::ostream& logFd_;
    static std::mutex mutex_;
    CrossNpuChecker crossNpuChecker_;
};

} // namespace Sanitizer
#endif // CORE_FRAMEWORK_THREADMANAGER_H
