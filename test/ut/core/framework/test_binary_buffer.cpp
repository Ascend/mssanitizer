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

#include <memory>
#include <string>
#include <vector>

#include "binary_buffer.h"
#include "fd_test_util.h"
#include "protocol.h"

using namespace Sanitizer;

namespace {

const std::string KERNEL_PAYLOAD = "zero-copy-kernel-binary";

TEST(BinaryBuffer, from_null_vector_expect_null) { ASSERT_EQ(BinaryBuffer::FromVector(nullptr), nullptr); }

TEST(BinaryBuffer, from_vector_expect_view_of_data) {
    auto data = std::make_shared<const std::vector<char>>(std::vector<char>{'a', 'b', 'c'});
    auto buffer = BinaryBuffer::FromVector(data);
    ASSERT_NE(buffer, nullptr);
    ASSERT_EQ(buffer->size(), 3U);
    ASSERT_FALSE(buffer->empty());
    ASSERT_EQ(std::string(buffer->data(), buffer->size()), "abc");
}

TEST(BinaryBuffer, from_empty_vector_expect_empty_view) {
    auto buffer = BinaryBuffer::FromVector(std::make_shared<const std::vector<char>>());
    ASSERT_NE(buffer, nullptr);
    ASSERT_EQ(buffer->size(), 0U);
    ASSERT_TRUE(buffer->empty());
}

TEST(BinaryBuffer, from_mapped_fd_with_invalid_args_expect_null_and_close) {
    ASSERT_EQ(BinaryBuffer::FromMappedFd(-1, 8U), nullptr);
    int fd = TestUtility::CreatePayloadFd(KERNEL_PAYLOAD);
    ASSERT_GE(fd, 0);
    // len 为 0 时构建失败，并接管关闭 fd
    ASSERT_EQ(BinaryBuffer::FromMappedFd(fd, 0U), nullptr);
}

TEST(BinaryBuffer, from_mapped_fd_expect_zero_copy_view) {
    int fd = TestUtility::CreatePayloadFd(KERNEL_PAYLOAD);
    ASSERT_GE(fd, 0);
    auto buffer = BinaryBuffer::FromMappedFd(fd, KERNEL_PAYLOAD.size());
    ASSERT_NE(buffer, nullptr);
    ASSERT_EQ(buffer->size(), KERNEL_PAYLOAD.size());
    ASSERT_EQ(std::string(buffer->data(), buffer->size()), KERNEL_PAYLOAD);
}

TEST(Packet, from_binary_buffer_expect_expose_view_and_rebind) {
    int fd = TestUtility::CreatePayloadFd(KERNEL_PAYLOAD);
    ASSERT_GE(fd, 0);
    auto buffer = BinaryBuffer::FromMappedFd(fd, KERNEL_PAYLOAD.size());
    ASSERT_NE(buffer, nullptr);

    Packet packet(PacketType::KERNEL_BINARY, buffer);
    ASSERT_EQ(packet.GetType(), PacketType::KERNEL_BINARY);
    ASSERT_EQ(packet.GetBinaryBuffer(), buffer);
    ASSERT_EQ(packet.GetPayload().binary.len, KERNEL_PAYLOAD.size());
    ASSERT_EQ(std::string(packet.GetPayload().binary.buf, packet.GetPayload().binary.len), KERNEL_PAYLOAD);

    Packet moved(std::move(packet));
    ASSERT_EQ(moved.GetBinaryBuffer(), buffer);
    ASSERT_EQ(moved.GetPayload().binary.len, KERNEL_PAYLOAD.size());
    ASSERT_EQ(std::string(moved.GetPayload().binary.buf, moved.GetPayload().binary.len), KERNEL_PAYLOAD);
    // 移出后原对象不应再持有悬垂指针
    ASSERT_EQ(packet.GetPayload().binary.buf, nullptr);
    ASSERT_EQ(packet.GetPayload().binary.len, 0U);
}

} // namespace
