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

#ifndef __HOOKS_ACL_HOOKS_H__
#define __HOOKS_ACL_HOOKS_H__

typedef enum aclrtMemMallocPolicy {
    ACL_MEM_MALLOC_HUGE_FIRST,
    ACL_MEM_MALLOC_HUGE_ONLY,
    ACL_MEM_MALLOC_NORMAL_ONLY,
    ACL_MEM_MALLOC_HUGE_FIRST_P2P,
    ACL_MEM_MALLOC_HUGE_ONLY_P2P,
    ACL_MEM_MALLOC_NORMAL_ONLY_P2P,
    ACL_MEM_TYPE_LOW_BAND_WIDTH   = 0x0100,
    ACL_MEM_TYPE_HIGH_BAND_WIDTH  = 0x1000,
} aclrtMemMallocPolicy;

typedef enum aclrtMemcpyKind {
    ACL_MEMCPY_HOST_TO_HOST,
    ACL_MEMCPY_HOST_TO_DEVICE,
    ACL_MEMCPY_DEVICE_TO_HOST,
    ACL_MEMCPY_DEVICE_TO_DEVICE,
} aclrtMemcpyKind;

typedef int aclError;
typedef void *aclrtStream;

/// Interfaces to be hooked use C ABI
extern "C" {
typedef aclError (*AcldvppMalloc)(void **devPtr, size_t size);
typedef aclError (*AclMalloc)(void **devPtr, size_t size, aclrtMemMallocPolicy policy);
typedef aclError (*AclMallocCached)(void **devPtr, size_t size, aclrtMemMallocPolicy policy);
typedef aclError (*AclrtFree)(void *devPtr);
typedef aclError (*AclrtFreeWithDevSync)(void *devPtr);
typedef aclError (*AclrtSetDevice)(int32_t deviceId);
typedef aclError (*AclrtMemset)(void *devPtr, size_t maxCount, int32_t value, size_t count);
typedef aclError (*AclrtMemsetAsync)(void *devPtr, size_t maxCount, int32_t value, size_t count, aclrtStream stream);
typedef aclError (*AclrtMemcpy)(void *dst, size_t destMax, const void *src, size_t count, aclrtMemcpyKind kind);
typedef aclError (*AclrtMemcpyAsync)(void *dst, size_t destMax, const void *src, size_t count,
                                     aclrtMemcpyKind kind, aclrtStream stream);
typedef aclError (*AclrtMemcpy2d)(void *dst, size_t dpitch, const void *src, size_t spitch,
                                  size_t width, size_t height, aclrtMemcpyKind kind);
typedef aclError (*AclrtMemcpy2dAsync)(void *dst, size_t dpitch, const void *src, size_t spitch,
                                       size_t width, size_t height, aclrtMemcpyKind kind, aclrtStream stream);

aclError acldvppMalloc(void **devPtr, size_t size);
aclError aclrtMalloc(void **devPtr, size_t size, aclrtMemMallocPolicy policy);
aclError aclrtMallocCached(void **devPtr, size_t size, aclrtMemMallocPolicy policy);
aclError aclrtFree(void *devPtr);
aclError aclrtFreeWithDevSync(void *devPtr);
aclError aclrtSetDevice(int32_t deviceId);
aclError aclrtMemset(void *devPtr, size_t maxCount, int32_t value, size_t count);
aclError aclrtMemsetAsync(void *devPtr, size_t maxCount, int32_t value, size_t count, aclrtStream stream);
aclError aclrtMemcpy(void *dst, size_t destMax, const void *src, size_t count, aclrtMemcpyKind kind);
aclError aclrtMemcpyAsync(void *dst, size_t destMax, const void *src, size_t count,
                          aclrtMemcpyKind kind, aclrtStream stream);
aclError aclrtMemcpy2d(void *dst, size_t dpitch, const void *src, size_t spitch,
                       size_t width, size_t height, aclrtMemcpyKind kind);
aclError aclrtMemcpy2dAsync(void *dst, size_t dpitch, const void *src, size_t spitch,
                            size_t width, size_t height, aclrtMemcpyKind kind, aclrtStream stream);

aclError sanitizerDvppMalloc(void **devPtr, size_t size, char const *filename, int lineno);
aclError sanitizerRtMalloc(void **devPtr, size_t size, aclrtMemMallocPolicy policy,
                           char const *filename, int lineno);
aclError sanitizerRtMallocCached(void **devPtr, size_t size, aclrtMemMallocPolicy policy,
                                 char const *filename, int lineno);
aclError sanitizerRtFree(void *devPtr, char const *filename, int lineno);
aclError sanitizerRtFreeWithDevSync(void *devPtr, char const *filename, int lineno);
aclError sanitizerRtSetDevice(int32_t deviceId, char const *filename, int lineno);
aclError sanitizerRtMemset(void *devPtr, size_t maxCount, int32_t value, size_t count,
                           char const *filename, int lineno);
aclError sanitizerRtMemsetAsync(void *devPtr, size_t maxCount, int32_t value, size_t count, aclrtStream stream,
                                char const *filename, int lineno);
aclError sanitizerRtMemcpy(void *dst, size_t destMax, const void *src, size_t count, aclrtMemcpyKind kind,
                           char const *filename, int lineno);
aclError sanitizerRtMemcpyAsync(void *dst, size_t destMax, const void *src, size_t count,
                                aclrtMemcpyKind kind, aclrtStream stream, char const *filename, int lineno);
aclError sanitizerRtMemcpy2d(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width, size_t height,
                             aclrtMemcpyKind kind, char const *filename, int lineno);
aclError sanitizerRtMemcpy2dAsync(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width, size_t height,
                                  aclrtMemcpyKind kind, aclrtStream stream, char const *filename, int lineno);
const char *aclrtGetSocNameImpl();
aclError aclrtSetDeviceImpl(int32_t devId);
}

#endif  // __HOOKS_ACL_HOOKS_H__
