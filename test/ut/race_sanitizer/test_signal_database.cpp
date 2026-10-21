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

#include "alg_framework/signal_database.h"
#include "core/framework/event_def.h"
#include "sanitizer_report.h"

using namespace Sanitizer;

TEST(SignalDatabase, wait_not_exist_addr_expect_return_false)
{
    SignalDatabase database;
    VectorTime vt;

    MstxSignalWait mstxSignalWait{};
    mstxSignalWait.addr = 0x100;
    ASSERT_FALSE(database.Wait(mstxSignalWait, vt));
}

TEST(SignalDatabase, wait_unsatisified_compare_op_expect_return_false)
{
    SignalDatabase database;
    VectorTime vt;

    MstxSignalSet mstxSignalSet{};
    mstxSignalSet.addr = 0x100;
    mstxSignalSet.value = 100;
    database.Set(mstxSignalSet, vt);

    MstxSignalWait mstxSignalWait{};
    mstxSignalWait.addr = 0x100;

    mstxSignalWait.cmpOp = CompareOp::EQ;
    mstxSignalWait.cmpValue = 99;
    ASSERT_FALSE(database.Wait(mstxSignalWait, vt));

    mstxSignalWait.cmpOp = CompareOp::NE;
    mstxSignalWait.cmpValue = 100;
    ASSERT_FALSE(database.Wait(mstxSignalWait, vt));

    mstxSignalWait.cmpOp = CompareOp::GT;
    mstxSignalWait.cmpValue = 100;
    ASSERT_FALSE(database.Wait(mstxSignalWait, vt));

    mstxSignalWait.cmpOp = CompareOp::GE;
    mstxSignalWait.cmpValue = 101;
    ASSERT_FALSE(database.Wait(mstxSignalWait, vt));

    mstxSignalWait.cmpOp = CompareOp::LT;
    mstxSignalWait.cmpValue = 100;
    ASSERT_FALSE(database.Wait(mstxSignalWait, vt));

    mstxSignalWait.cmpOp = CompareOp::LE;
    mstxSignalWait.cmpValue = 99;
    ASSERT_FALSE(database.Wait(mstxSignalWait, vt));
}

TEST(SignalDatabase, accumulate_first_publish_expect_keep_input_vector_time)
{
    SignalDatabase database;
    VectorTime vt = {3, 5, 7, 9};
    VectorTime vtGet;

    MstxSignalSet mstxSignalSet{};
    mstxSignalSet.addr = 0x100;
    mstxSignalSet.value = 100;
    database.Accumulate(mstxSignalSet, vt);

    MstxSignalWait mstxSignalWait{};
    mstxSignalWait.addr = 0x100;
    mstxSignalWait.cmpOp = CompareOp::GE;
    mstxSignalWait.cmpValue = 100;
    ASSERT_TRUE(database.Wait(mstxSignalWait, vtGet));
    /// 首次发布必须整体存入，向量时间的长度与内容都不允许被截断
    ASSERT_EQ(vtGet, vt);
}

TEST(SignalDatabase, accumulate_multi_producer_expect_merge_vector_time_and_keep_max_value)
{
    SignalDatabase database;
    VectorTime vtGet;

    MstxSignalSet firstSet{};
    firstSet.addr = 0x100;
    firstSet.value = 100;
    database.Accumulate(firstSet, VectorTime{7, 1, 1});

    MstxSignalSet secondSet{};
    secondSet.addr = 0x100;
    secondSet.value = 200;
    database.Accumulate(secondSet, VectorTime{1, 9, 1});

    MstxSignalSet thirdSet{};
    thirdSet.addr = 0x100;
    thirdSet.value = 300;
    database.Accumulate(thirdSet, VectorTime{1, 1, 5});

    MstxSignalWait mstxSignalWait{};
    mstxSignalWait.addr = 0x100;
    mstxSignalWait.cmpOp = CompareOp::GE;
    mstxSignalWait.cmpValue = 300;
    ASSERT_TRUE(database.Wait(mstxSignalWait, vtGet));
    /// N 个生产者的向量时间全部保留（逐分量取大），value 取最大值，等待方一次拿到所有生产者
    ASSERT_EQ(vtGet, (VectorTime{7, 9, 5}));
}

TEST(SignalDatabase, accumulate_late_smaller_value_expect_value_not_decrease)
{
    SignalDatabase database;
    VectorTime vtGet;

    MstxSignalSet largeSet{};
    largeSet.addr = 0x100;
    largeSet.value = 300;
    database.Accumulate(largeSet, VectorTime{1, 1, 1});

    /// 回放顺序与执行顺序不一致：后到的是一个更小的值，覆盖语义会把它改小，
    /// 导致 Wait(GE 300) 永远不满足（工具报 pipe line infinite loop）
    MstxSignalSet smallSet{};
    smallSet.addr = 0x100;
    smallSet.value = 100;
    database.Accumulate(smallSet, VectorTime{2, 2, 2});

    MstxSignalWait mstxSignalWait{};
    mstxSignalWait.addr = 0x100;
    mstxSignalWait.cmpOp = CompareOp::GE;
    mstxSignalWait.cmpValue = 300;
    ASSERT_TRUE(database.Wait(mstxSignalWait, vtGet));
    ASSERT_EQ(vtGet, (VectorTime{2, 2, 2}));
}

TEST(SignalDatabase, accumulate_different_addr_expect_not_affect_each_other)
{
    SignalDatabase database;
    VectorTime vtGet;

    MstxSignalSet firstSet{};
    firstSet.addr = 0x100;
    firstSet.value = 100;
    database.Accumulate(firstSet, VectorTime{1, 1, 1});

    MstxSignalSet secondSet{};
    secondSet.addr = 0x200;
    secondSet.value = 200;
    database.Accumulate(secondSet, VectorTime{2, 2, 2});

    MstxSignalWait mstxSignalWait{};
    mstxSignalWait.addr = 0x100;
    mstxSignalWait.cmpOp = CompareOp::GE;
    mstxSignalWait.cmpValue = 100;
    ASSERT_TRUE(database.Wait(mstxSignalWait, vtGet));
    ASSERT_EQ(vtGet, (VectorTime{1, 1, 1}));
}

TEST(SignalDatabase, wait_satisified_compare_op_expect_return_true_and_get_correct_vector_time)
{
    SignalDatabase database;
    VectorTime vt = {1, 2, 3};
    VectorTime vtGet;

    MstxSignalSet mstxSignalSet{};
    mstxSignalSet.addr = 0x100;
    mstxSignalSet.value = 100;
    database.Set(mstxSignalSet, vt);

    MstxSignalWait mstxSignalWait{};
    mstxSignalWait.addr = 0x100;

    mstxSignalWait.cmpOp = CompareOp::EQ;
    mstxSignalWait.cmpValue = 100;
    ASSERT_TRUE(database.Wait(mstxSignalWait, vtGet));
    ASSERT_EQ(vtGet, vt);

    mstxSignalWait.cmpOp = CompareOp::NE;
    mstxSignalWait.cmpValue = 99;
    ASSERT_TRUE(database.Wait(mstxSignalWait, vtGet));
    ASSERT_EQ(vtGet, vt);

    mstxSignalWait.cmpOp = CompareOp::GT;
    mstxSignalWait.cmpValue = 99;
    ASSERT_TRUE(database.Wait(mstxSignalWait, vtGet));
    ASSERT_EQ(vtGet, vt);

    mstxSignalWait.cmpOp = CompareOp::GE;
    mstxSignalWait.cmpValue = 100;
    ASSERT_TRUE(database.Wait(mstxSignalWait, vtGet));
    ASSERT_EQ(vtGet, vt);

    mstxSignalWait.cmpOp = CompareOp::LT;
    mstxSignalWait.cmpValue = 101;
    ASSERT_TRUE(database.Wait(mstxSignalWait, vtGet));
    ASSERT_EQ(vtGet, vt);

    mstxSignalWait.cmpOp = CompareOp::LE;
    mstxSignalWait.cmpValue = 100;
    ASSERT_TRUE(database.Wait(mstxSignalWait, vtGet));
    ASSERT_EQ(vtGet, vt);
}
