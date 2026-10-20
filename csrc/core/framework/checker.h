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

#ifndef CORE_FRAMEWORK_CHECKER_H
#define CORE_FRAMEWORK_CHECKER_H

#include <atomic>
#include <fstream>
#include <memory>
#include <map>
#include <thread>
#include <queue>
#include <condition_variable>
#include <mutex>
#include "sanitizer_base.h"
#include "record_defs.h"
#include "config.h"
#include "runtime_context.h"
#include "platform_config.h"
#include "parse_pipeline.h"
#include "utility/log.h"
#include "protocol.h"

namespace Sanitizer {

constexpr uint8_t TOOL_NUM = static_cast<uint8_t>(Sanitizer::ToolType::SIZE);

enum class BroadcastEvent : uint8_t {
    NO_INFO = 0,
    DEVICE_INFO_UPDATED,
    KERNEL_SUMMARY_UPDATED,
    KERNEL_NAME_UPDATED,
    SANITIZER_RECORD_ARRIVED,
    KERNEL_BLOCK_ARRIVED,
    STOP,
};

struct WorkArgs {
    BroadcastEvent eventType;
    SanitizerRecord record;
    std::vector<SanEvent> event;
    DeviceInfoSummary deviceInfo;
    KernelSummary kernelSummary;
    std::string kernelNameDisplay;
    std::vector<SanitizerRecord> records;
    std::vector<std::vector<SanEvent>> events;
};

// Checker类主要用于将单条解析信息分发给合适的sanitier工具
// 其主要功能是抽象不同工具在框架界面的差异，对框架而言，只要
// 感知某个工具名字，然后将对应的信息一条条导入checker就能达成目标。
// 现在仅支持AddressSanitizer，所以写死为该类型
// 扩展说明：
//   将抽象类移到框架管理
class Checker {
public:
    explicit Checker(Config const &config, bool enableDebugLog);
    ~Checker();

    void Finish();
    void SetDeviceInfo(const DeviceInfoSummary &deviceInfo);
    void SetKernelInfo(const KernelSummary &kernelInfo);
    void SetDetectionInfo(const LogLv &expectLv, std::ostream &detectionOstream);
    void ParseOnlineError(const SanitizerRecord &record);
    // do check
    void Do(const SanitizerRecord& record);
    void Do(std::vector<SanitizerRecord> &&records);
    bool SupportSimt() const;
    bool IsTargetBlock(uint64_t blockIdx) const;

private:
    void ConsumeRecordThread(uint8_t consumeId, const std::thread::id &rootTid);
    void ConsumeRecord(uint8_t consumeId, const SanitizerRecord &record, std::vector<SanEvent> &events,
        uint64_t &removedPathwayEventCount);
    void ConsumeKernelBlock(uint8_t consumeId, WorkArgs &args, uint64_t &removedPathwayEventCount);
    void ReportPathwayFilterSummary(uint8_t consumeId, uint64_t removedPathwayEventCount) const;
    // ---- 解析并行：有界、保序的解析流水线（见 parse_pipeline.h）----
    ParseOut ParseRecords(ParseTask &&task);
    void DispatchParsed(std::vector<SanitizerRecord> &records, std::vector<std::vector<SanEvent>> &events);
    inline void WaitAfterConsumed(uint8_t consumeId);
    inline bool IsNeedFilterDbi(uint8_t toolIdx) const;
    inline bool IsNeedFilterDbi(const SanitizerRecord &record, uint8_t toolIdx) const;
    inline void TryPrintMissDebugLine();
    inline void DisplaySanitizerBegin(Config const &config) const;
    inline void DisplaySanitizerEnd(std::array<uint32_t, TOOL_NUM> const &errorCounts) const;

private:
    Config config_;
    bool enableDebugLog_;
    std::ostream *detectionOstream_{};

    std::array<std::mutex, TOOL_NUM> mtx_{};
    std::condition_variable workerCv_{};
    std::condition_variable producerCv_{};

    std::array<bool, TOOL_NUM> done_{};
    std::array<uint32_t, TOOL_NUM> errorCounts_{};
    std::atomic_bool finishProduce_{};
    std::atomic_bool printMissDebugLine_{};
    std::array<std::queue<WorkArgs>, TOOL_NUM> workerArgs_{};
    std::vector<std::thread> workers_{};
    mutable std::mutex detMutex_{};
    mutable std::mutex doMutex_{};

    // 解析流水线：并行解析 + 按提交顺序分发
    OrderedParsePipeline pipeline_;

    std::array<std::shared_ptr<Sanitizer::SanitizerBase>, TOOL_NUM> sanitizerArr_{};
    std::array<bool, TOOL_NUM> initWithDeviceInfoDone_{};
    std::array<bool, TOOL_NUM> initWithKernelInfoDone_{};
    bool isKernelWithDBI_{false};
    DeviceType deviceType_{DeviceType::INVALID};
    // 所属 client 线程的 rootTid：解析池线程与消费线程都按它解析 ThreadSingleton
    std::thread::id rootTid_{};
}; // class Checker

} // namespace Sanitizer

#endif // CORE_FRAMEWORK_CHECKER_H
