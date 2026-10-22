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

#include "checker.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <numeric>
#include <ostream>
#include <sstream>
#include <thread>
#include <iostream>
#include <vector>

#include "platform_config.h"
#include "record_defs.h"
#include "record_format.h"
#include "runtime_context.h"
#include "sanitizer_base.h"
#include "record_pre_process.h"
#include "kernel_block.h"
#include "file_mapping.h"
#include "utility/ustring.h"
#include "utility/serializer.h"
#include "utility/umask_guard.h"
#include "utility/log.h"
#include "utility/cpp_future.h"
#include "pathway_filter.h"

namespace {

using namespace Sanitizer;

enum class SanitizerTool : uint8_t {
    MEMCHECK = 0, // 内存检测
    RACECHECK,    // 竞争检测
    INITCHECK,    // 同步检测
    SYNCCHECK,    // 同步检测
    SIZE,
};

std::ostream &operator<<(std::ostream &os, SanitizerTool tool)
{
    static const std::map<SanitizerTool, std::string> SANITIZER_TOOL_MAP = {
        {SanitizerTool::MEMCHECK,  "memcheck"},
        {SanitizerTool::RACECHECK, "racecheck"},
        {SanitizerTool::INITCHECK, "initcheck"},
        {SanitizerTool::SYNCCHECK, "synccheck"},
    };

    return FormatEnum(os, SANITIZER_TOOL_MAP, tool, "SanitizerTool");
}

std::vector<SanitizerTool> GetEnabledSanitizerTools(Config const &config)
{
    std::vector<SanitizerTool> tools;
    tools.reserve(static_cast<std::size_t>(SanitizerTool::SIZE));
    if (config.memCheck) {
        tools.emplace_back(SanitizerTool::MEMCHECK);
    }
    if (config.raceCheck) {
        tools.emplace_back(SanitizerTool::RACECHECK);
    }
    if (config.initCheck) {
        tools.emplace_back(SanitizerTool::INITCHECK);
    }
    if (config.syncCheck) {
        tools.emplace_back(SanitizerTool::SYNCCHECK);
    }
    return tools;
}

bool IsSanitizerToolSupportDBI(DeviceType deviceType, SanitizerTool tool)
{
    if (IsAscend95(deviceType)) {
        return true;
    }
    return tool == SanitizerTool::MEMCHECK;
}

} // namespace [Dummy]

namespace Sanitizer {

namespace {
constexpr uint32_t PARSE_THREADS = 8;

// 解析 worker 与 committer 都是新建线程，RuntimeContext 是 thread_local 的，
// 需各自按本任务的上下文恢复，否则收尾打印(如 DisplaySanitizerEnd)取不到 kernel 名。
void RestoreParseContext(ParseContext const &ctx) {
    RuntimeContext &rc = RuntimeContext::Instance();
    rc.deviceSummary_ = ctx.deviceInfo;
    rc.kernelSummary_ = ctx.kernelSummary;
    rc.kernelIdx_ = ctx.kernelIdx;
    rc.kernelNameDisplay = ctx.kernelNameDisplay;
    rc.rootTid_ = ctx.rootTid;
}
} // namespace

Checker::Checker(Config const &config, bool enableDebugLog)
    : config_(config), enableDebugLog_{enableDebugLog},
      pipeline_(
          PARSE_THREADS, 16, [this](ParseTask &&task) { return ParseRecords(std::move(task)); },
          [this](ParseOut &&out) {
              RestoreParseContext(out.ctx);
              DispatchParsed(out);
          }) {
    // 后续依据不同的工具选择使能不同的sanitizer功能
    rootTid_ = RuntimeContext::Instance().rootTid_;
    auto rootTid = rootTid_;

    auto createCheckerAndThread = [this, rootTid](ToolType toolType, int index, bool enabled) {
        if (enabled) {
            sanitizerArr_[index] = SanitizerFactory::GetInstance().Create(toolType);
            if (sanitizerArr_[index]) {
                workers_.push_back(std::thread(&Checker::ConsumeRecordThread, this, index, rootTid));
                done_[index] = true;
            }
        }
    };
    createCheckerAndThread(ToolType::MEMCHECK, 0, config.defaultCheck);
    createCheckerAndThread(ToolType::RACECHECK, 1, config.raceCheck);
    createCheckerAndThread(ToolType::SYNCCHECK, 2, config.syncCheck);
    createCheckerAndThread(ToolType::REGISTERCHECK, 3, config.registerCheck);
}

Checker::~Checker()
{
    try {
        this->Finish();
    } catch (const std::exception &ex) {
        SAN_WARN_LOG("Got internel exception %s when checker::finish", ex.what());
    }
}

inline void Checker::WaitAfterConsumed(uint8_t consumeId)
{
    {
        std::lock_guard<std::mutex> lock(mtx_[consumeId]);
        if (workerArgs_[consumeId].empty()) {
            done_[consumeId] = true;
        }
    }
    producerCv_.notify_one();
}

inline bool Checker::IsNeedFilterDbi(uint8_t toolIdx) const {
    // 静态插桩默认处理
    if (!isKernelWithDBI_) {
        return false;
    }

    if (IsAscend95(this->deviceType_)) {
        // 已全部改为动态插桩，都要处理
        return false;
    }

    if ((toolIdx == static_cast<uint8_t>(Sanitizer::ToolType::RACECHECK) && config_.raceCheck) ||
        (toolIdx == static_cast<uint8_t>(Sanitizer::ToolType::SYNCCHECK) && config_.syncCheck)) {
        // A2/A3：动态插桩不处理racecheck和synccheck
        return true;
    }

    return false;
}

inline bool Checker::IsNeedFilterDbi(const SanitizerRecord &record, uint8_t toolIdx) const {
    // host侧内存记录全部默认处理
    if (record.version == RecordVersion::MEMORY_RECORD) {
        return false;
    }

    return IsNeedFilterDbi(toolIdx);
}

inline bool IsTargetBlockIdx(DeviceType deviceType, int16_t checkBlockId, uint64_t blockIdx)
{
    if (HasSubBlocks(deviceType)) {
        uint64_t vecTargetBlockIdx = checkBlockId / C220_VEC_SUB_BLOCKDIM * C220_MIX_SUB_BLOCKDIM +
            checkBlockId % C220_VEC_SUB_BLOCKDIM;
        uint64_t cubeTargetBlockIdx = checkBlockId * C220_MIX_SUB_BLOCKDIM + C220_VEC_SUB_BLOCKDIM;
        return blockIdx == vecTargetBlockIdx || blockIdx == cubeTargetBlockIdx;
    } else {
        return blockIdx == static_cast<uint64_t>(checkBlockId);
    }
}

bool Checker::SupportSimt() const
{
    return this->deviceType_ > DeviceType::ASCEND_950_START && this->deviceType_ < DeviceType::ASCEND_950_END;
}

bool Checker::IsTargetBlock(uint64_t blockIdx) const
{
    return this->config_.checkBlockId == CHECK_ALL_BLOCK ||
        IsTargetBlockIdx(deviceType_, config_.checkBlockId, blockIdx);
}

void Checker::ConsumeRecordThread(uint8_t consumeId, const std::thread::id &rootTid)
{
    RuntimeContext().Instance().rootTid_ = rootTid;
    auto &que = workerArgs_[consumeId];
    uint64_t removedPathwayEventCount = 0;
    while (true) {
        WorkArgs args;
        if (finishProduce_ && !que.empty()) {
            args = std::move(que.front());
            que.pop();
        } else {
            std::unique_lock<std::mutex> lock(mtx_[consumeId]);
            workerCv_.wait(lock, [&que] () { return !que.empty(); });
            args = std::move(que.front());
            que.pop();
        }
        switch (args.eventType) {
        case BroadcastEvent::SANITIZER_RECORD_ARRIVED:
            break;
        case BroadcastEvent::KERNEL_BLOCK_ARRIVED: {
            ConsumeKernelBlock(consumeId, args, removedPathwayEventCount);
            continue;
        }
        case BroadcastEvent::DEVICE_INFO_UPDATED: {
            RuntimeContext::Instance().deviceSummary_ = args.deviceInfo;
            continue;
        }
        case BroadcastEvent::KERNEL_SUMMARY_UPDATED: {
            RuntimeContext::Instance().kernelSummary_ = args.kernelSummary;
            removedPathwayEventCount = 0;
            continue;
        }
        case BroadcastEvent::KERNEL_NAME_UPDATED: {
            RuntimeContext::Instance().kernelNameDisplay = args.kernelNameDisplay;
            continue;
        }
        case BroadcastEvent::STOP:
            // clear queue
            std::queue<WorkArgs>().swap(que);
            return;
        default:
            continue;
        };

        ConsumeRecord(consumeId, args.record, args.event, removedPathwayEventCount);
        if (finishProduce_) {
            WaitAfterConsumed(consumeId);
        }
    };
}

void Checker::ConsumeRecord(uint8_t consumeId, const SanitizerRecord &record, std::vector<SanEvent> &events,
    uint64_t &removedPathwayEventCount) {
    const bool initialized = initWithDeviceInfoDone_[consumeId] || initWithKernelInfoDone_[consumeId];
    if (initialized && sanitizerArr_[consumeId]->CheckRecordBeforeProcess(record)) {
        const bool shouldFilter = consumeId == static_cast<uint8_t>(ToolType::MEMCHECK) ||
            consumeId == static_cast<uint8_t>(ToolType::RACECHECK);
        if (shouldFilter && record.version == RecordVersion::KERNEL_RECORD) {
            removedPathwayEventCount += PathwayFilter::FilterInPlace(record.payload.kernelRecord.recordType, events);
        }
        sanitizerArr_[consumeId]->Do(record, events);
    }

    if (record.version == RecordVersion::KERNEL_RECORD &&
        record.payload.kernelRecord.recordType == RecordType::KERNEL_FINISH) {
        if (consumeId == static_cast<uint8_t>(ToolType::MEMCHECK) ||
            consumeId == static_cast<uint8_t>(ToolType::RACECHECK)) {
            ReportPathwayFilterSummary(consumeId, removedPathwayEventCount);
            removedPathwayEventCount = 0;
        }
    }
}

void Checker::ConsumeKernelBlock(uint8_t consumeId, WorkArgs &args, uint64_t &removedPathwayEventCount) {
    for (std::size_t i = 0; i < args.records.size(); ++i) {
        ConsumeRecord(consumeId, args.records[i], args.events[i], removedPathwayEventCount);
    }

    if (finishProduce_) {
        WaitAfterConsumed(consumeId);
    }
}

void Checker::ReportPathwayFilterSummary(uint8_t consumeId, uint64_t removedPathwayEventCount) const {
    const char *consumer = consumeId == static_cast<uint8_t>(ToolType::MEMCHECK) ? "AddressSanitizer" : "RaceSanitizer";
    const RuntimeContext &runtimeContext = RuntimeContext::Instance();
    SAN_INFO_LOG("Pathway filter consumer:%s, kernel:%s, deviceId:%d, deviceType:%u, removed events:%llu", consumer,
        runtimeContext.kernelNameDisplay.c_str(), runtimeContext.GetDeviceId(),
        static_cast<uint32_t>(runtimeContext.deviceSummary_.device),
        static_cast<unsigned long long>(removedPathwayEventCount));
}

void Checker::SetDeviceInfo(const DeviceInfoSummary &deviceInfo) {
    pipeline_.Drain();
    this->deviceType_ = deviceInfo.device;
    for (uint8_t i = 0; i < TOOL_NUM; ++i) {
        if (sanitizerArr_[i] != nullptr) {
            bool initDone = sanitizerArr_[i]->SetDeviceInfo(deviceInfo, config_);
            initWithDeviceInfoDone_[i] = initDone;
            std::lock_guard<std::mutex> lock(mtx_[i]);
            WorkArgs workargs{};
            workargs.eventType = BroadcastEvent::DEVICE_INFO_UPDATED;
            workargs.deviceInfo = deviceInfo;
            workerArgs_[i].push(std::move(workargs));
        }
    }
    workerCv_.notify_all();
}

void Checker::SetKernelInfo(const KernelSummary &kernelInfo)
{
    // 切换 kernel 上下文前排空解析流水线：确保上一 kernel 已提交的 ParseOut 全部 commit，
    // 不会在新 kernel 的 DisplaySanitizerBegin/KERNEL_SUMMARY_UPDATED 之后才被分发，导致上下文/输出错位。
    // 正常路径下上一 kernel 的 KERNEL_FINISH 已触发 Drain，这里是幂等的安全屏障。
    pipeline_.Drain();
    DisplaySanitizerBegin(config_);

    isKernelWithDBI_ = kernelInfo.isKernelWithDBI;
    printMissDebugLine_ = true;
    for (uint8_t i = 0; i < TOOL_NUM; ++i) {
        if (sanitizerArr_[i] != nullptr) {
            bool initDone = sanitizerArr_[i]->SetKernelInfo(kernelInfo);
            initWithKernelInfoDone_[i] = initDone;
            std::lock_guard<std::mutex> lock(mtx_[i]);
            WorkArgs workargs{};
            workargs.eventType = BroadcastEvent::KERNEL_SUMMARY_UPDATED;
            workargs.kernelSummary = kernelInfo;
            workerArgs_[i].push(std::move(workargs));
            workargs.eventType = BroadcastEvent::KERNEL_NAME_UPDATED;
            workargs.kernelNameDisplay = RuntimeContext::Instance().kernelNameDisplay;
            workerArgs_[i].push(std::move(workargs));
        }
    }
    workerCv_.notify_all();
}

void Checker::TryPrintMissDebugLine()
{
    // 每个 kernel 出现第一条异常，并且缺失 debugline 信息时，打印提示
    const bool printMissDebugLine = printMissDebugLine_.exchange(false);
    if (printMissDebugLine && !RuntimeContext::Instance().kernelSummary_.hasDebugLine) {
        printf("[mssanitizer] WARN: Kernel %s missed debug_line information. Please recompile kernel "
               "with `-g' or `-gline-tables-only' option to enable callstack display.\n",
               RuntimeContext::Instance().kernelNameDisplay.c_str());
    }
}

void Checker::DisplaySanitizerBegin(Config const &config) const
{
    if (detectionOstream_ == nullptr) {
        return;
    }

    std::ostream &os = *detectionOstream_;
    RuntimeContext &runtimeContext = RuntimeContext::Instance();
    std::vector<SanitizerTool> tools = GetEnabledSanitizerTools(config);

    // 按是否支持动态插桩将用户开启的检测选项进行分组
    auto ignoredToolIt = tools.cend();
    if (runtimeContext.kernelSummary_.isKernelWithDBI) {
        ignoredToolIt = std::partition(tools.begin(), tools.end(), [this](SanitizerTool tool) {
            return IsSanitizerToolSupportDBI(deviceType_, tool);
        });
    }

    {
        std::lock_guard<std::mutex> guard(detMutex_);
        if (ignoredToolIt != tools.cbegin()) {
            os << "[mssanitizer] Start "
               << Utility::HumanReadableListFormat(tools.cbegin(), ignoredToolIt, ToString<SanitizerTool>)
               << " sanitizer on kernel " << runtimeContext.kernelNameDisplay << std::endl;
        } else {
            os << "[mssanitizer] No active sanitizer tool on kernel "
               << runtimeContext.kernelNameDisplay << "." << std::endl;
        }
    }

    if (ignoredToolIt != tools.cend()) {
        std::lock_guard<std::mutex> guard(detMutex_);
        os << "[mssanitizer] WARN: "
           << Utility::HumanReadableListFormat(ignoredToolIt, tools.cend(), ToString<SanitizerTool>)
           << " is not supported on the kernel that without "
           << "`--cce-enable-sanitizer' compiler option, thus it's temporarily ignored." << std::endl;
    }
}

void Checker::DisplaySanitizerEnd(std::array<uint32_t, TOOL_NUM> const &errorCounts) const
{
    if (detectionOstream_ == nullptr) {
        return;
    }

    std::ostream &os = *detectionOstream_;
    RuntimeContext &runtimeContext = RuntimeContext::Instance();
    auto totalErrorCounts = std::accumulate(errorCounts.cbegin(), errorCounts.cend(), 0U);

    std::lock_guard<std::mutex> guard(detMutex_);
    if (totalErrorCounts > 0) {
        os << "[mssanitizer] Sanitizer finished on kernel "
           << runtimeContext.kernelNameDisplay
           << ". See all detected errors above." << std::endl;
    } else {
        os << "[mssanitizer] Sanitizer finished on kernel "
           << runtimeContext.kernelNameDisplay
           << ". No error detected." << std::endl;
    }
}

void Checker::SetDetectionInfo(const LogLv &expectLv, std::ostream &detectionOstream)
{
    detectionOstream_ = &detectionOstream;
    for (uint8_t i = 0; i < TOOL_NUM; ++i) {
        if (sanitizerArr_[i] != nullptr) {
            auto func = [expectLv, i, &detectionOstream, this](const LogLv &lv, SanitizerBase::MSG_GEN &&gen) {
                if (lv >= expectLv) {
                    TryPrintMissDebugLine();
                    ++errorCounts_[i];
                    std::lock_guard<std::mutex> guard(detMutex_);
                    detectionOstream << gen().message << std::flush;
                }
            };
            sanitizerArr_[i]->RegisterNotifyFunc(func);
        }
    }
}

void Checker::ParseOnlineError(const SanitizerRecord &record)
{
    const auto &kernelRecord = record.payload.kernelRecord;
    uint8_t memCheckIdx = static_cast<uint8_t>(ToolType::MEMCHECK);
    if (sanitizerArr_[memCheckIdx] != nullptr) {
        sanitizerArr_[memCheckIdx]->ParseOnlineError(kernelRecord.payload.kernelErrorRecord,
            kernelRecord.blockType, kernelRecord.serialNo);
    }
    uint8_t raceCheckIdx = static_cast<uint8_t>(ToolType::RACECHECK);
    if (sanitizerArr_[raceCheckIdx] != nullptr) {
        sanitizerArr_[raceCheckIdx]->ParseOnlineError(kernelRecord.payload.kernelErrorRecord,
            kernelRecord.blockType, kernelRecord.serialNo);
    }
    uint8_t syncCheckIdx = static_cast<uint8_t>(ToolType::SYNCCHECK);
    if (sanitizerArr_[syncCheckIdx] != nullptr) {
        sanitizerArr_[syncCheckIdx]->ParseOnlineError(kernelRecord.payload.kernelErrorRecord,
            kernelRecord.blockType, kernelRecord.serialNo);
    }

    std::stringstream ss;
    ss << record << ", deviceId:" << RuntimeContext::Instance().GetDeviceId();
    SAN_LOG("%s", ss.str().c_str());
}

void Checker::Do(const SanitizerRecord &record)
{
    std::lock_guard<std::mutex> lk(doMutex_);
    pipeline_.Drain();
    /// 解析在线检测的异常结果
    if (record.version == RecordVersion::KERNEL_RECORD &&
        record.payload.kernelRecord.recordType == RecordType::ONLINE_ERROR) {
        ParseOnlineError(record);
        return;
    }
    std::vector<SanEvent> events;
    RecordPreProcess::GetInstance().Process(record, events);
    finishProduce_ = false;
    uint8_t activeToolNum = 0;
    for (uint8_t i = 0; i < TOOL_NUM; ++i) {
        if (!IsNeedFilterDbi(record, i) && sanitizerArr_[i] != nullptr) {
            ++activeToolNum;
        }
    }
    const bool canMoveEvent = (activeToolNum <= 1);
    for (uint8_t i = 0; i < TOOL_NUM; ++i) {
        if (IsNeedFilterDbi(record, i)) {
            continue;
        }
        if (sanitizerArr_[i] != nullptr) {
            std::lock_guard<std::mutex> lock(mtx_[i]);
            done_[i] = false;
            WorkArgs workargs{};
            workargs.eventType = BroadcastEvent::SANITIZER_RECORD_ARRIVED;
            workargs.record = record;
            workargs.event = canMoveEvent ? std::move(events) : events;
            workerArgs_[i].push(std::move(workargs));
        }
    }
    if (record.version != RecordVersion::KERNEL_RECORD ||
        record.payload.kernelRecord.recordType != RecordType::KERNEL_FINISH) {
        workerCv_.notify_all();
    } else {
        finishProduce_ = true;
        workerCv_.notify_all();
        for (uint8_t i = 0; i < TOOL_NUM; ++i) {
            if (sanitizerArr_[i] == nullptr) {
                continue;
            }
            if (IsNeedFilterDbi(record, i)) {
                continue;
            }
            std::unique_lock<std::mutex> lock(mtx_[i]);
            producerCv_.wait(lock, [this, i] () { return done_[i]; });
        }
        printMissDebugLine_ = false;
        auto errorCounts = decltype(errorCounts_){};
        std::swap(errorCounts, errorCounts_);
        DisplaySanitizerEnd(errorCounts);
        KernelBlock::ResetAll();
    }

    if (enableDebugLog_) {
        std::stringstream ss;
        ss << record << ", deviceId:" << RuntimeContext::Instance().GetDeviceId();
        SAN_LOG("%s", ss.str().c_str());
    }
}

void Checker::Do(std::vector<SanitizerRecord> records) {
    if (records.empty()) {
        return;
    }
    // 与本提交前的同步语义保持一致：批量以 KERNEL_FINISH 结尾时，等解析、分发与该 kernel 的
    // 消费全部结束后再返回，调用方可立即观察到检测结果。
    const bool endsWithKernelFinish = records.back().version == RecordVersion::KERNEL_RECORD &&
        records.back().payload.kernelRecord.recordType == RecordType::KERNEL_FINISH;
    RuntimeContext &ctx = RuntimeContext::Instance();
    ParseTask task;
    task.records = std::move(records);
    task.ctx.deviceInfo = ctx.deviceSummary_;
    task.ctx.kernelSummary = ctx.kernelSummary_;
    task.ctx.kernelIdx = ctx.kernelIdx_;
    task.ctx.kernelNameDisplay = ctx.kernelNameDisplay;
    task.ctx.rootTid = rootTid_;
    pipeline_.Submit(std::move(task));

    if (endsWithKernelFinish) {
        pipeline_.Drain();
    }
}

ParseOut Checker::ParseRecords(ParseTask &&task) {
    // 池线程的 RuntimeContext 是 thread_local 的，需先恢复本任务的 device/kernel 上下文，解析输出才正确
    RestoreParseContext(task.ctx);

    ParseOut out;
    out.ctx = std::move(task.ctx);
    out.records = std::move(task.records);
    out.events.resize(out.records.size());
    for (std::size_t i = 0; i < out.records.size(); ++i) {
        RecordPreProcess::GetInstance().Process(out.records[i], out.events[i]);
    }
    return out;
}

void Checker::DispatchParsed(ParseOut &out) {
    auto &records = out.records;
    const bool isKernelFinish = !records.empty() && records.back().version == RecordVersion::KERNEL_RECORD &&
        records.back().payload.kernelRecord.recordType == RecordType::KERNEL_FINISH;
    finishProduce_ = false;
    // 仅一个工具使能时，可把 events 直接 move 进队列，避免整块拷贝（多工具时仍需各自一份）
    uint8_t activeToolNum = 0;
    for (uint8_t i = 0; i < TOOL_NUM; ++i) {
        if (!IsNeedFilterDbi(i) && sanitizerArr_[i] != nullptr) {
            ++activeToolNum;
        }
    }
    const bool canMoveEvents = (activeToolNum <= 1);
    for (uint8_t i = 0; i < TOOL_NUM; ++i) {
        if (IsNeedFilterDbi(i)) {
            continue;
        }
        if (sanitizerArr_[i] != nullptr) {
            std::lock_guard<std::mutex> lock(mtx_[i]);
            done_[i] = false;
            WorkArgs workargs{};
            workargs.eventType = BroadcastEvent::KERNEL_BLOCK_ARRIVED;
            workargs.records = records;
            workargs.events = canMoveEvents ? std::move(out.events) : out.events;
            workerArgs_[i].push(std::move(workargs));
        }
    }

    if (isKernelFinish) {
        finishProduce_ = true;
        workerCv_.notify_all();
        for (uint8_t i = 0; i < TOOL_NUM; ++i) {
            if (sanitizerArr_[i] == nullptr) {
                continue;
            }
            if (IsNeedFilterDbi(i)) {
                continue;
            }
            std::unique_lock<std::mutex> lock(mtx_[i]);
            producerCv_.wait(lock, [this, i]() { return done_[i]; });
        }
        printMissDebugLine_ = false;
        auto errorCounts = decltype(errorCounts_){};
        std::swap(errorCounts, errorCounts_);
        DisplaySanitizerEnd(errorCounts);
    } else {
        workerCv_.notify_all();
    }

    if (enableDebugLog_) {
        for (const auto &record : records) {
            std::stringstream ss;
            ss << record << ", deviceId:" << RuntimeContext::Instance().GetDeviceId();
            SAN_LOG("%s", ss.str().c_str());
        }
    }
}

void Checker::Finish()
{
    // 先等所有已提交的 block 解析并分发完成，再停解析流水线
    pipeline_.Drain();
    pipeline_.Shutdown();
    if (!workers_.empty()) {
        for (uint8_t i = 0; i < TOOL_NUM; ++i) {
            if (sanitizerArr_[i] != nullptr) {
                std::lock_guard<std::mutex> lock(mtx_[i]);
                WorkArgs workargs{};
                workargs.eventType = BroadcastEvent::STOP;
                workerArgs_[i].push(std::move(workargs));
            }
        }
        workerCv_.notify_all();
        for (auto &worker: workers_) {
            worker.join();
        }
        workers_.clear();
    }
    // dump mem leak errors
    for (uint8_t i = 0; i < TOOL_NUM; ++i) {
        if (sanitizerArr_[i] != nullptr) {
            sanitizerArr_[i]->Exit();
        }
    }
}

} // namespace Sanitizer
