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


#include <utility>
#include <gtest/gtest.h>
#include "../data_process.h"
#include "../../ccec_defs.h"
#include "plugin/ccec/move_instructions/dc_preload.cpp"

using namespace Sanitizer;

DcPreloadRecord CreateDcPreloadRecord()
{
    DcPreloadRecord record{};
    record.addr = 0x12c0c0013000; // GM地址
    record.space = AddressSpace::GM;
    record.location.fileNo = RandInt(0, 0x100000);
    record.location.lineNo = RandInt(0, 0x1000);

    record.offset = 32U;
    return record;
}

TEST(DcPreloadInstructions, dc_preload)
{
    auto record = CreateDcPreloadRecord();

    std::vector<uint8_t> memInfo = CreateMemInfo();
    constexpr int16_t offset = 32U;

    __sanitizer_report_dc_preload(memInfo.data(), record.location.fileNo,
        record.location.lineNo, record.location.pc, reinterpret_cast<__gm__ uint64_t *>(record.addr), offset);

    __sanitizer_report_dc_preload(memInfo.data(), record.location.fileNo,
        record.location.lineNo, record.location.pc, reinterpret_cast<__gm__ uint64_t *>(record.addr), record.offset);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::DC_PRELOAD>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(DcPreloadRecord);
    ASSERT_TRUE(CheckRecordEqual<RecordType::DC_PRELOAD>(ptr, record));
}
