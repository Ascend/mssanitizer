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