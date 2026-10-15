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

#include <gtest/gtest.h>

#include "utility/recv_fd.h"

using namespace Sanitizer;

namespace {

void DrainRecvFdQueue() {
    while (PopRecvFd() >= 0) {
    }
}

TEST(RecvFd, pop_from_empty_queue_expect_minus_one) {
    DrainRecvFdQueue();
    ASSERT_EQ(PopRecvFd(), -1);
}

TEST(RecvFd, push_then_pop_expect_fifo_order) {
    DrainRecvFdQueue();
    PushRecvFd(11);
    PushRecvFd(22);
    PushRecvFd(33);
    ASSERT_EQ(PopRecvFd(), 11);
    ASSERT_EQ(PopRecvFd(), 22);
    ASSERT_EQ(PopRecvFd(), 33);
    ASSERT_EQ(PopRecvFd(), -1);
}

} // namespace
