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

#ifndef CORE_FRAMEWORK_CROSS_NPU_CHECKER_H
#define CORE_FRAMEWORK_CROSS_NPU_CHECKER_H

#include <mutex>
#include <unordered_map>
#include <vector>

#include "config.h"
#include "event_def.h"
#include "race_sanitizer/cross_npu_race_sanitizer.h"
#include "record_defs.h"

namespace Sanitizer {

class CrossNpuChecker {
public:
    /**
     * @brief 用于缓存每个 device 上每个 kernel 的指令记录队列
     */
    class RecordArray {
    public:
        friend class CrossNpuChecker;

        /**
         * @brief 创建一个新 kernel 指令队列
         */
        void CreateNewKernel();

        /**
         * @brief 向当前 kernel 追加一条指令记录
         */
        void Push(SanitizerRecord const &record);

#if defined(__BUILD_TESTS__)
        std::vector<std::vector<SanitizerRecord>> &GetRecords() { return records_; }
#endif

    private:
        std::vector<std::vector<SanitizerRecord>> records_;
    };

    using RecordMap = std::unordered_map<uint32_t, RecordArray>;

public:
    explicit CrossNpuChecker(Config const &config);

    static void UpdateLoc(std::vector<SanEvent> &events, uint32_t deviceIdx, uint32_t kernelIdx, uint32_t deviceId);

    /**
     * @brief 获取指定 deviceId 的指令缓存队列
     */
    RecordArray &GetRecordArray(uint32_t deviceId);

    /**
     * @brief 设置异常报告输出回调
     */
    void SetDetectionInfo(const LogLv &expectLv, std::ostream &detectionOstream);

    /**
     * @brief 启动检测
     */
    bool Run();

#if defined(__BUILD_TESTS__)
    RecordMap &GetRecordMap() { return recordMap_; }
#endif

private:
    inline void DisplaySanitizerBegin(Config const &config) const;
    inline void DisplaySanitizerEnd() const;

private:
    mutable std::mutex mtx_;
    RecordMap recordMap_;
    Config config_;
    std::ostream *detectionOstream_{};
    CrossNpuRaceSanitizer sanitizer_;
    uint32_t errorCount_{};
};

} // namespace Sanitizer

#endif // CORE_FRAMEWORK_CROSS_NPU_CHECKER_H