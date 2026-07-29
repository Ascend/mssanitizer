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
#include <cstring>
#include <string>

#include "hook_report.h"
#include "record_defs.h"

namespace Sanitizer {
MemOpRecord CreateRecord(MemOpType type, const ReportAddrInfo &addrInfo);
}

using namespace Sanitizer;

TEST(HookReport, CreateRecord_valid_filename_expect_copied) {
    ReportAddrInfo addrInfo(0x1000, 0x100, 10, "test_file.cpp", MemInfoSrc::RT);
    MemOpRecord record = CreateRecord(MemOpType::LOAD, addrInfo);
    std::string fileName(record.fileName, sizeof(record.fileName));
    ASSERT_NE(fileName.find("test_file.cpp"), std::string::npos);
}

TEST(HookReport, CreateRecord_long_filename_expect_truncated) {
    std::string longName(1024, 'a');
    ReportAddrInfo addrInfo(0x1000, 0x100, 5, longName.c_str(), MemInfoSrc::BYPASS);
    MemOpRecord record = CreateRecord(MemOpType::STORE, addrInfo);
    std::string fileName(record.fileName, sizeof(record.fileName));
    ASSERT_EQ(fileName.find_first_of('\0'), sizeof(record.fileName) - 1);
}

TEST(HookReport, CreateRecord_with_invalid_chars_filename_expect_replaced) {
    ReportAddrInfo addrInfo(0x1000, 0x100, 20, "test\nfile\rname", MemInfoSrc::RT);
    MemOpRecord record = CreateRecord(MemOpType::MALLOC, addrInfo);
    std::string fileName(record.fileName, sizeof(record.fileName));
    ASSERT_EQ(fileName.find('\n'), std::string::npos);
    ASSERT_EQ(fileName.find('\r'), std::string::npos);
}

TEST(HookReport, CreateRecord_type_and_addr_expect_correct) {
    ReportAddrInfo addrInfo(0x2000, 0x200, 30, "demo.cpp", MemInfoSrc::BYPASS);
    MemOpRecord record = CreateRecord(MemOpType::FREE, addrInfo);
    ASSERT_EQ(record.type, MemOpType::FREE);
    ASSERT_EQ(record.dstAddr, 0x2000);
    ASSERT_EQ(record.memSize, 0x200);
    ASSERT_EQ(record.infoSrc, MemInfoSrc::BYPASS);
    ASSERT_EQ(record.srcAddr, 0x00);
    ASSERT_EQ(record.srcSpace, AddressSpace::GM);
    ASSERT_EQ(record.dstSpace, AddressSpace::GM);
}
