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

#ifndef CORE_FRAMEWORK_ERROR_EXITCODE_H
#define CORE_FRAMEWORK_ERROR_EXITCODE_H

#include <atomic>
#include <cstdint>

#include "process.h"
#include "utility/log.h"
#include "utility/singleton.h"

namespace Sanitizer {

// 工具参数/配置错误的退出码
constexpr int32_t SANITIZER_INVALID_ARGUMENT_EXIT_CODE = 255;
// 拉起被检测程序失败的退出码
constexpr int32_t SANITIZER_LAUNCH_ERROR_EXIT_CODE = 1;
// 被信号终止时的退出码基数：128 + signal
constexpr int32_t SIGNAL_EXIT_BASE = 128;

// 本次运行是否出现过 ERROR 级检出（跨线程汇总）
class ErrorDetectionSummary : public Singleton<ErrorDetectionSummary> {
public:
    friend class Singleton<ErrorDetectionSummary>;

    void MarkErrorDetected() { detected_.store(true, std::memory_order_relaxed); }
    bool HasErrorDetection() const { return detected_.load(std::memory_order_relaxed); }

#if defined(__BUILD_TESTS__)
    // 仅供单元测试复位
    void Reset() { detected_.store(false, std::memory_order_relaxed); }
#endif

private:
    ErrorDetectionSummary() = default;
    std::atomic<bool> detected_{false};
};

// ERROR 级检出；WARN/INFO 不计入 --error-exitcode
inline bool IsErrorDetectionLevel(const LogLv &lv) { return lv >= LogLv::ERROR; }

// 计算工具进程最终退出码。
// 优先级：拉起失败 > 信号终止 > 程序退出码 > 配置值 + ERROR 级检出 > 0
inline int32_t ResolveExitCode(int32_t errorExitCode, const ProgramStatus &status) {
    if (status.launchError) {
        return SANITIZER_LAUNCH_ERROR_EXIT_CODE;
    }
    if (!status.exitedNormally) {
        return SIGNAL_EXIT_BASE + status.termSignal;
    }
    if (status.exitCode != 0) {
        return status.exitCode;
    }
    if (errorExitCode == 0 || !ErrorDetectionSummary::Instance().HasErrorDetection()) {
        return 0;
    }
    return errorExitCode;
}

} // namespace Sanitizer

#endif // CORE_FRAMEWORK_ERROR_EXITCODE_H
