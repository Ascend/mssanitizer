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
#define BUILD_DYNAMIC_PROBE
#define __NPU_ARCH__ 3101
#include "plugin/ccec/dbi/probes/cube_instructions.cpp"

TEST(DbiCubeInstructions, mmad_expect_get_correct_records)
{
    MmadA5Record record{};
    record.dst = 0;
    record.dstBlockNum = 16;
    record.src0 = 0;
    record.src0BlockNum = 32;
    record.src0AlignSize = 512;
    record.src1 = 0;
    record.src1BlockNum = 32;
    record.src1AlignSize = 512;
    record.m = 64;
    record.k = 128;
    record.n = 64;
    record.cmatrixInitVal = 1;
    record.src0Dtype = DetailedDataType::F16;
    record.src1Dtype = DetailedDataType::F16;
    std::vector<uint8_t> memInfo = CreateMemInfo();

    uint64_t config{};
    SetConfByUint<11, 0>(config, record.m);
    SetConfByUint<23, 12>(config, record.k);
    SetConfByUint<35, 24>(config, record.n);
    SetConfByUint<63, 63>(config, 1);

    __sanitizer_report_mad_f16_f32(memInfo.data(), record.location.pc, 0,
        reinterpret_cast<void*>(record.dst),
        reinterpret_cast<void*>(record.src0),
        reinterpret_cast<void*>(record.src1),
        config
    );

    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    ASSERT_TRUE(*reinterpret_cast<RecordType*>(ptr) == RecordType::MMAD_A5);
    auto x = reinterpret_cast<Sanitizer::MmadA5Record*>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(*x == record);
}

TEST(DbiCubeInstructions, all_mad_variants_expect_record) {
    auto record = CreateRandomBinaryOpRecord();
    uint64_t config{};
    SetConfByUint<11, 0>(config, 64);   // m
    SetConfByUint<23, 12>(config, 128); // k
    SetConfByUint<35, 24>(config, 64);  // n
    SetConfByUint<63, 63>(config, 1);

    auto run = [&](auto &&func) {
        std::vector<uint8_t> memInfo = CreateMemInfo();
        func(memInfo.data());
        uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
        EXPECT_EQ(*reinterpret_cast<RecordType *>(ptr), RecordType::MMAD_A5);
    };

    run([&](uint8_t *m) { __sanitizer_report_mad_s8(m, record.location.pc, 0, nullptr, nullptr, nullptr, config); });
    run([&](uint8_t *m) { __sanitizer_report_mad_bf16_f32(m, record.location.pc, 0, nullptr, nullptr, nullptr, config); });
    run([&](uint8_t *m) { __sanitizer_report_mad_f32_f32(m, record.location.pc, 0, nullptr, nullptr, nullptr, config); });
    run([&](uint8_t *m) { __sanitizer_report_mad_e4m3_e4m3(m, record.location.pc, 0, nullptr, nullptr, nullptr, config); });
    run([&](uint8_t *m) { __sanitizer_report_mad_e4m3_e5m2(m, record.location.pc, 0, nullptr, nullptr, nullptr, config); });
    run([&](uint8_t *m) { __sanitizer_report_mad_e5m2_e4m3(m, record.location.pc, 0, nullptr, nullptr, nullptr, config); });
    run([&](uint8_t *m) { __sanitizer_report_mad_e5m2_e5m2(m, record.location.pc, 0, nullptr, nullptr, nullptr, config); });
    run([&](uint8_t *m) { __sanitizer_report_mad_mx_e1m2_e1m2(m, record.location.pc, 0, nullptr, nullptr, nullptr, config); });
    run([&](uint8_t *m) { __sanitizer_report_mad_mx_e1m2_e2m1(m, record.location.pc, 0, nullptr, nullptr, nullptr, config); });
    run([&](uint8_t *m) { __sanitizer_report_mad_mx_e2m1_e1m2(m, record.location.pc, 0, nullptr, nullptr, nullptr, config); });
    run([&](uint8_t *m) { __sanitizer_report_mad_mx_e2m1_e2m1(m, record.location.pc, 0, nullptr, nullptr, nullptr, config); });
    run([&](uint8_t *m) { __sanitizer_report_mad_mx_e4m3_e4m3(m, record.location.pc, 0, nullptr, nullptr, nullptr, config); });
    run([&](uint8_t *m) { __sanitizer_report_mad_mx_e4m3_e5m2(m, record.location.pc, 0, nullptr, nullptr, nullptr, config); });
    run([&](uint8_t *m) { __sanitizer_report_mad_mx_e5m2_e4m3(m, record.location.pc, 0, nullptr, nullptr, nullptr, config); });
    run([&](uint8_t *m) { __sanitizer_report_mad_mx_e5m2_e5m2(m, record.location.pc, 0, nullptr, nullptr, nullptr, config); });
}
