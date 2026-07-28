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

#include <gtest/gtest.h>
#include "address_sanitizer.h"
#include "checker.h"
#include "config.h"
#include "platform_config.h"
#include "securec.h"
#include "utility/log.h"
#include "thread_manager.h"

using namespace Sanitizer;

TEST(ThreadManager, one_thread_expect_run_checker_and_protocol_success)
{
    Config config {
        .defaultCheck = true,
        .memCheck = true,
    };
    LogLv logLv = LogLv::DEBUG;
    char const* logFile = "/tmp/test.log";
    ThreadManager threadManager(config, false, logLv, logFile);

    Checker& checker1 = threadManager.GetChecker();
    Checker& checker2 = threadManager.GetChecker();
    auto &ptr1 = threadManager.GetProtocol();
    auto &ptr2 = threadManager.GetProtocol();
    EXPECT_EQ(&checker1, &checker2);  // 期望两次返回的引用指向同一个对象，通过比较地址来验证
    EXPECT_EQ(&ptr1, &ptr2);  // 期望两次返回的指针是相同的
}

TEST(ThreadManager, multi_thread_expect_run_checker_and_protocol_success)
{
    Config config {
        .defaultCheck = true,
        .memCheck = true,
    };
    LogLv logLv = LogLv::DEBUG;
    char const* logFile = "/tmp/test.log";
    ThreadManager threadManager(config, false, logLv, logFile);

    std::vector<std::thread> threads;
    const int numThreads = 8;
    for (int i = 0; i < numThreads; ++i) {
        threads.push_back(std::thread([&]() {
            Checker& checker = threadManager.GetChecker();
            auto &protocol = threadManager.GetProtocol();
        }));
    }
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }
    // 检查checkers_容器中元素个数是否与启动的线程数量一致，以此验证不同线程获取到不同Checker对象
    EXPECT_EQ(threadManager.GetCheckersNum(), numThreads);
    EXPECT_EQ(threadManager.GetProtocolsNum(), numThreads);
}
