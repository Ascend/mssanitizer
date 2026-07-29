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


#include <gtest/gtest.h>

#include "hooks/hooks_verify.h"
#include "acl_hooks.h"

using namespace std;

TEST(TestHooksVerify, HalMemcpy2DArg_null_ptr_expect_false) { ASSERT_FALSE(Sanitizer::IsValidHalMemcpy2DArg(nullptr)); }

TEST(TestHooksVerify, RtMemcpy2dAsync_host_to_host_expect_true) {
    void *dst = reinterpret_cast<void *>(UINT64_MAX);
    void *src = reinterpret_cast<void *>(UINT64_MAX);
    size_t dpitch = UINT64_MAX;
    size_t spitch = UINT64_MAX;
    size_t height = 100;
    aclrtMemcpyKind kind = ACL_MEMCPY_HOST_TO_HOST;
    ASSERT_TRUE(Sanitizer::IsValidRtMemcpy2dAsyncArg(dst, dpitch, src, spitch, height, kind));
}

TEST(TestHooksVerify, HalMemcpy2DArg_non_sync_type_expect_true) {
    MEMCPY2D memCopy;
    auto &copy2D = memCopy.copy2d;
    copy2D.src = reinterpret_cast<unsigned long long int *>(0x1000ULL);
    copy2D.dst = reinterpret_cast<unsigned long long int *>(0x1000ULL);
    copy2D.dpitch = UINT64_MAX;
    copy2D.spitch = UINT64_MAX;
    copy2D.height = 100;
    memCopy.type = 999;
    ASSERT_TRUE(Sanitizer::IsValidHalMemcpy2DArg(&memCopy));
}

TEST(TestHooksVerify, RtMemcpy2dAsync_expect_good)
{
    void *dst = reinterpret_cast<void*>(0x1000);
    void *src = reinterpret_cast<void*>(0x1000);
    size_t dpitch = 100;
    size_t spitch = 100;
    size_t height = 10;
    aclrtMemcpyKind kind = ACL_MEMCPY_HOST_TO_DEVICE;
    ASSERT_TRUE(Sanitizer::IsValidRtMemcpy2dAsyncArg(dst, dpitch, src, spitch, height, kind));

    dst = reinterpret_cast<void*>(UINT64_MAX);
    height = 1;
    ASSERT_TRUE(Sanitizer::IsValidRtMemcpy2dAsyncArg(dst, dpitch, src, spitch, height, kind));
}

TEST(TestHooksVerify, RtMemcpy2dAsync_wraparound_expect_fail)
{
    void *dst = reinterpret_cast<void*>(0x1000);
    void *src = reinterpret_cast<void*>(0x1000);
    size_t dpitch = UINT64_MAX;
    size_t spitch = 100;
    size_t height = 10;
    aclrtMemcpyKind kind = ACL_MEMCPY_HOST_TO_DEVICE;
    ASSERT_FALSE(Sanitizer::IsValidRtMemcpy2dAsyncArg(dst, dpitch, src, spitch, height, kind));

    dpitch = 2;
    dst = reinterpret_cast<void*>(UINT64_MAX);
    ASSERT_FALSE(Sanitizer::IsValidRtMemcpy2dAsyncArg(dst, dpitch, src, spitch, height, kind));
}

TEST(TestHooksVerify, HalMemcpy2DArg_expect_good)
{
    MEMCPY2D memCopy;
    auto &copy2D = memCopy.copy2d;
    copy2D.src = reinterpret_cast<unsigned long long int*>(0x1000ULL);
    copy2D.dst = reinterpret_cast<unsigned long long int*>(0x1000ULL);
    copy2D.dpitch = 100;
    copy2D.spitch = 100;
    copy2D.height = 1;
    memCopy.type = DEVMM_MEMCPY2D_SYNC;
    ASSERT_TRUE(Sanitizer::IsValidHalMemcpy2DArg(&memCopy));

    copy2D.dpitch = UINT64_MAX;
    memCopy.type = DEVMM_MEMCPY2D_ASYNC_CONVERT;
    ASSERT_TRUE(Sanitizer::IsValidHalMemcpy2DArg(&memCopy));

    copy2D.src = reinterpret_cast<unsigned long long int*>(UINT64_MAX);
    ASSERT_TRUE(Sanitizer::IsValidHalMemcpy2DArg(&memCopy));
}

TEST(TestHooksVerify, HalMemcpy2DArg_wraparound_expect_fail)
{
    MEMCPY2D memCopy;
    auto &copy2D = memCopy.copy2d;
    copy2D.src = reinterpret_cast<unsigned long long int*>(0x1000ULL);
    copy2D.dst = reinterpret_cast<unsigned long long int*>(0x1000ULL);
    copy2D.dpitch = 100;
    copy2D.spitch = UINT64_MAX;
    copy2D.height = 5;
    memCopy.type = DEVMM_MEMCPY2D_SYNC;
    ASSERT_FALSE(Sanitizer::IsValidHalMemcpy2DArg(&memCopy));

    copy2D.dpitch = UINT64_MAX;
    copy2D.spitch = 100;
    memCopy.type = DEVMM_MEMCPY2D_ASYNC_CONVERT;
    ASSERT_FALSE(Sanitizer::IsValidHalMemcpy2DArg(&memCopy));

    copy2D.src = reinterpret_cast<unsigned long long int*>(UINT64_MAX);
    ASSERT_FALSE(Sanitizer::IsValidHalMemcpy2DArg(&memCopy));
}
