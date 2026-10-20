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

#ifndef CORE_FRAMEWORK_PARSE_PIPELINE_H
#define CORE_FRAMEWORK_PARSE_PIPELINE_H

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "record_defs.h"
#include "event_def.h"

namespace Sanitizer {

// 解析一个任务所需的上下文（device/kernel 信息），随任务一起提交
struct ParseContext {
    DeviceInfoSummary deviceInfo;
    KernelSummary kernelSummary;
    uint32_t kernelIdx{};
    std::string kernelNameDisplay;
    // 池线程通过该 rootTid 解析 ThreadSingleton（AlignChecker/FileMapping 等），
    // 保证与所属 Checker/client 线程使用同一实例
    std::thread::id rootTid{};
};

struct ParseTask {
    std::vector<SanitizerRecord> records;
    ParseContext ctx;
};

struct ParseOut {
    std::vector<SanitizerRecord> records;
    std::vector<std::vector<SanEvent>> events;
    // 随解析结果带回 committer 线程：收尾(DisplaySanitizerEnd 等)需用同一份 device/kernel 上下文
    ParseContext ctx;
};

// 有界、保序的解析流水线：
//   1. 并行：workers 个线程执行 packaged_task（调用 parse_ 做纯计算）
//   2. 保序：唯一的 committer 线程按“提交顺序”future.get() 后回调 commit_
//   3. 背压：Submit 阻塞，直到未提交数(ordered_.size()) < inflightCap
//   4. Drain：等待 committed_ == submitted_，即所有已提交任务解析并 commit 完成
// committer 回调 commit_ 时不再持有内部锁，避免工具消费端的阻塞拖住 worker。
class OrderedParsePipeline {
public:
    using ParseFn = std::function<ParseOut(ParseTask &&)>;
    using CommitFn = std::function<void(ParseOut &&)>;
    using Job = std::packaged_task<ParseOut()>;

    OrderedParsePipeline(uint32_t workers, uint32_t inflightCap, ParseFn parse, CommitFn commit);
    ~OrderedParsePipeline();

    OrderedParsePipeline(const OrderedParsePipeline &) = delete;
    OrderedParsePipeline &operator=(const OrderedParsePipeline &) = delete;

    // 提交一个任务；队列满时阻塞（背压），保证内存有界
    void Submit(ParseTask &&task);
    // 等所有已提交任务被解析并 commit 完成（对应旧 WaitAllParsed）
    void Drain();
    void Shutdown();

private:
    void WorkerLoop();
    void CommitLoop();

    ParseFn parse_;
    CommitFn commit_;
    uint32_t inflightCap_;

    std::mutex mtx_;
    std::condition_variable jobCv_;
    std::condition_variable backpressureCv_;
    std::condition_variable commitCv_;
    std::condition_variable drainCv_;

    std::queue<Job> jobs_;
    std::deque<std::future<ParseOut>> ordered_;
    uint64_t submitted_{0};
    uint64_t committed_{0};
    bool stop_{false};
    // committer 线程捕获的首个解析/提交异常；失败后仍继续消费并提交剩余任务，Drain 统一重抛
    std::exception_ptr firstError_;

    std::vector<std::thread> workers_;
    std::thread committer_;
};

} // namespace Sanitizer

#endif // CORE_FRAMEWORK_PARSE_PIPELINE_H
