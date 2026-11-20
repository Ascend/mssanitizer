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

#include "parse_pipeline.h"

#include <memory>
#include <utility>

namespace Sanitizer {

OrderedParsePipeline::OrderedParsePipeline(uint32_t workers, uint32_t inflightCap, ParseFn parse, CommitFn commit)
    : parse_(std::move(parse)), commit_(std::move(commit)), inflightCap_(inflightCap == 0 ? 1 : inflightCap) {
    workers_.reserve(workers);
    for (uint32_t i = 0; i < workers; ++i) {
        workers_.emplace_back([this] { WorkerLoop(); });
    }
    committer_ = std::thread([this] { CommitLoop(); });
}

OrderedParsePipeline::~OrderedParsePipeline() { Shutdown(); }

void OrderedParsePipeline::Submit(ParseTask &&task) {
    std::shared_ptr<ParseTask> taskPtr = std::make_shared<ParseTask>(std::move(task));
    Job job([this, taskPtr]() { return parse_(std::move(*taskPtr)); });
    std::future<ParseOut> fut = job.get_future();
    {
        std::unique_lock<std::mutex> lk(mtx_);
        backpressureCv_.wait(lk, [this] { return stop_ || ordered_.size() < inflightCap_; });
        if (stop_) {
            return;
        }
        ordered_.push_back(std::move(fut));   // 提交顺序 == 队列顺序，保序的关键
        jobs_.push(std::move(job));
        ++submitted_;
    }
    jobCv_.notify_one();
    commitCv_.notify_one();
}

void OrderedParsePipeline::Drain() {
    std::unique_lock<std::mutex> lk(mtx_);
    drainCv_.wait(lk, [this] { return committed_ == submitted_; });
    if (firstError_ != nullptr) {
        std::rethrow_exception(firstError_);
    }
}

void OrderedParsePipeline::Shutdown() {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (stop_) {
            return;
        }
        stop_ = true;
    }
    jobCv_.notify_all();
    backpressureCv_.notify_all();
    commitCv_.notify_all();
    for (auto &w : workers_) {
        if (w.joinable()) {
            w.join();
        }
    }
    if (committer_.joinable()) {
        committer_.join();
    }
    workers_.clear();
}

void OrderedParsePipeline::WorkerLoop() {
    for (;;) {
        Job job;
        {
            std::unique_lock<std::mutex> lk(mtx_);
            jobCv_.wait(lk, [this] { return stop_ || !jobs_.empty(); });
            if (jobs_.empty()) {
                if (stop_) {
                    return;
                }
                continue;
            }
            job = std::move(jobs_.front());
            jobs_.pop();
        }
        job();   // 结果或异常都进 future，worker 不会因业务异常退出
    }
}

void OrderedParsePipeline::CommitLoop() {
    for (;;) {
        std::future<ParseOut> fut;
        {
            std::unique_lock<std::mutex> lk(mtx_);
            commitCv_.wait(lk, [this] { return stop_ || !ordered_.empty(); });
            if (ordered_.empty()) {
                if (stop_) {
                    return;
                }
                continue;
            }
            fut = std::move(ordered_.front());
            ordered_.pop_front();
        }
        try {
            ParseOut out = fut.get();   // 阻塞到本任务完成，天然保序；解析异常在此抛出
            commit_(std::move(out));    // 不持 mtx_：消费端阻塞不再拖住 worker
        } catch (...) {
            // 记录首个异常但不中断：剩余任务继续解析/提交，Drain 在全部结束后统一重抛
            std::lock_guard<std::mutex> lk(mtx_);
            if (firstError_ == nullptr) {
                firstError_ = std::current_exception();
            }
        }
        {
            std::lock_guard<std::mutex> lk(mtx_);
            ++committed_;
        }
        drainCv_.notify_all();
        backpressureCv_.notify_all();
    }
}

} // namespace Sanitizer
