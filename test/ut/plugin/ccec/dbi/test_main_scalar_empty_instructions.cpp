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

#include <algorithm>
#include <gtest/gtest.h>

#include "ccec/data_process.h"
#define BUILD_DYNAMIC_PROBE
#define BISHENG_SUPPORT_SIMT_CALL_DBI
#include "plugin/ccec/dbi/probes/main_scalar_empty_instructions.cpp"

using namespace Sanitizer;

namespace SanitizerTest {

TEST(SimtCallInstructions, dump_simt_call_expect_get_success)
{
    std::vector<uint8_t> memInfo = CreateMemInfo();
    RecordGlobalHead head{};
    head.checkParms.defaultcheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());

    __sanitizer_report_simt_call(memInfo.data(), 0x100, 0);
    auto blockHead = reinterpret_cast<SimtRecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead->recordWriteCount, 1);
    ASSERT_EQ(blockHead->recordCount, 1);
}

}
