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

/// 以 EXTRA_PARAMS 中的同名入参触发宏，验证静态插桩分支写入源码位置与 pc。
void StampLocation(LocationRecord &record, uint64_t fileNo, uint64_t lineNo, int64_t pc) {
    SET_RECORD_LOCATION(record);
}

} // namespace

TEST(RecordLocation, stamps_file_line_and_pc) {
    LocationRecord record{};
    StampLocation(record, 11, 22, 0x1234);

    EXPECT_EQ(record.location.fileNo, 11U);
    EXPECT_EQ(record.location.lineNo, 22U);
    EXPECT_EQ(record.location.pc, 0x1234U);
}

TEST(RecordLocation, keeps_block_id_untouched) {
    LocationRecord record{};
    record.location.blockId = 7;
    StampLocation(record, 1, 2, 3);

    EXPECT_EQ(record.location.blockId, 7U);
}

TEST(RecordLocation, overwrites_existing_location) {
    LocationRecord record{};
    record.location.fileNo = 100;
    record.location.lineNo = 200;
    record.location.pc = 300;
    StampLocation(record, 1, 2, 3);

    EXPECT_EQ(record.location.fileNo, 1U);
    EXPECT_EQ(record.location.lineNo, 2U);
    EXPECT_EQ(record.location.pc, 3U);
}
