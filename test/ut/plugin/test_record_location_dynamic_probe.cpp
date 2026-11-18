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

#include <cstdint>
#include <gtest/gtest.h>

#include "plugin/utils.h"
#include "core/framework/record_defs.h"

namespace {

struct LocationRecord {
    Sanitizer::Location location;
};

/// 动态插桩没有源码行列号入参，宏只应写入 pc。
void StampLocation(LocationRecord &record, int64_t pc) { SET_RECORD_LOCATION(record); }

} // namespace

TEST(RecordLocationDynamicProbe, only_stamps_pc) {
    LocationRecord record{};
    record.location.fileNo = 11;
    record.location.lineNo = 22;
    record.location.blockId = 7;
    StampLocation(record, 0x5678);

    EXPECT_EQ(record.location.pc, 0x5678U);
    EXPECT_EQ(record.location.fileNo, 11U);
    EXPECT_EQ(record.location.lineNo, 22U);
    EXPECT_EQ(record.location.blockId, 7U);
}

TEST(RecordLocationDynamicProbe, overwrites_previous_pc) {
    LocationRecord record{};
    record.location.pc = 1;
    StampLocation(record, 2);

    EXPECT_EQ(record.location.pc, 2U);
}
