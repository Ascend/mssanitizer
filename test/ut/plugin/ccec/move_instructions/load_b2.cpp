
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
#include "plugin/ccec/move_instructions/load_b2.cpp"

template<MemType srcMemType, MemType dstMemType, AddrCalMode addrCalMode = AddrCalMode::INC>
LoadB2Record CreateLoadB2Record()
{
    LoadB2Record record{};
    record.dst = RandInt(0, 0x1000000);
    record.src = RandInt(0, 0x1000000);
    record.dataType = DataType::DATA_B8;
    record.repeat = RandInt(0, 0xFFF);
    record.location.fileNo = RandInt(0, 0x100000);
    record.location.lineNo = RandInt(0, 0x1000);

    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;

    return record;
}

uint64_t ExtractLoadB2Record(const LoadB2Record &record)
{
    constexpr uint64_t repeatMask = 0xFFUL;
    constexpr uint64_t fixDigit = 0x01UL;

    constexpr uint64_t repeatShift = 16;
    constexpr uint64_t fixDigitShift = 24;

    return ((record.repeat & repeatMask) << repeatShift) |
            ((1 & fixDigit) << fixDigitShift);
}

TEST(LoadB2Instructions, load_cbuf_to_cb_b8)
{
    auto record = CreateLoadB2Record<MemType::L1, MemType::L0B>();
    record.dataType = DataType::DATA_B8;

    std::vector<uint8_t> memInfo = CreateMemInfo();

    uint64_t config = ExtractLoadB2Record(record);

    __sanitizer_report_load_cbuf_to_cb_b2(memInfo.data(), record.location.fileNo, record.location.lineNo,
            record.location.pc,
            reinterpret_cast<__cbuf__ int8_t*>(record.dst), reinterpret_cast<__cb__ int8_t*>(record.src), config);
    __sanitizer_report_load_cbuf_to_cb_b2(memInfo.data(), record.location.fileNo, record.location.lineNo,
            record.location.pc,
            reinterpret_cast<__cbuf__ uint8_t*>(record.dst), reinterpret_cast<__cb__ uint8_t*>(record.src), config);

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::LOAD_B2>(ptr, record));
    ptr += sizeof(RecordType) + sizeof(LoadB2Record);
    ASSERT_TRUE(CheckRecordEqual<RecordType::LOAD_B2>(ptr, record));
}

TEST(LoadB2Instructions, load_cbuf_to_cb_f16)
{
    auto record = CreateLoadB2Record<MemType::L1, MemType::L0B>();
    record.dataType = DataType::DATA_B16;

    std::vector<uint8_t> memInfo = CreateMemInfo();


    uint64_t config = ExtractLoadB2Record(record);

    __sanitizer_report_load_cbuf_to_cb_b2(memInfo.data(), record.location.fileNo, record.location.lineNo,
            record.location.pc,
            reinterpret_cast<__cbuf__ half*>(record.dst), reinterpret_cast<__cb__ half*>(record.src), config);

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(CheckRecordEqual<RecordType::LOAD_B2>(ptr, record));
}

TEST(LoadB2Instructions, load_cbuf_to_cb_b2_repeat_overloads_expect_record) {
    auto record = CreateLoadB2Record<MemType::L1, MemType::L0B>();
    auto call = [&](auto &&func) {
        std::vector<uint8_t> memInfo = CreateMemInfo();
        func(memInfo.data());
        uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
        EXPECT_EQ(*reinterpret_cast<RecordType *>(ptr), RecordType::LOAD_B2);
    };

    uint8_t repeat = 4, sid = 0;
    call([&](uint8_t *m) { __sanitizer_report_load_cbuf_to_cb_b2(m, record.location.fileNo, record.location.lineNo, record.location.pc, reinterpret_cast<__cb__ half *>(record.dst), reinterpret_cast<__cbuf__ half *>(record.src), repeat, sid); });
    call([&](uint8_t *m) {
        __sanitizer_report_load_cbuf_to_cb_b2(m, record.location.fileNo, record.location.lineNo, record.location.pc,
            reinterpret_cast<__cb__ half *>(record.dst), reinterpret_cast<__cbuf__ half *>(record.src), repeat);
    });
    call([&](uint8_t *m) { __sanitizer_report_load_cbuf_to_cb_b2(m, record.location.fileNo, record.location.lineNo, record.location.pc, reinterpret_cast<__cb__ int8_t *>(record.dst), reinterpret_cast<__cbuf__ int8_t *>(record.src), repeat, sid); });
    call([&](uint8_t *m) {
        __sanitizer_report_load_cbuf_to_cb_b2(m, record.location.fileNo, record.location.lineNo, record.location.pc,
            reinterpret_cast<__cb__ int8_t *>(record.dst), reinterpret_cast<__cbuf__ int8_t *>(record.src), repeat);
    });
    call([&](uint8_t *m) { __sanitizer_report_load_cbuf_to_cb_b2(m, record.location.fileNo, record.location.lineNo, record.location.pc, reinterpret_cast<__cb__ uint8_t *>(record.dst), reinterpret_cast<__cbuf__ uint8_t *>(record.src), repeat, sid); });
    call([&](uint8_t *m) { __sanitizer_report_load_cbuf_to_cb_b2(m, record.location.fileNo, record.location.lineNo, record.location.pc, reinterpret_cast<__cb__ uint8_t *>(record.dst), reinterpret_cast<__cbuf__ uint8_t *>(record.src), repeat); });
}
