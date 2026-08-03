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


#include <cstdlib>
#include <string>
#include <iostream>
#include <dlfcn.h>

#include "hook_logger.h"
#include "hook_report.h"
#include "ascend_hal.h"
#include "vallina_symbol.h"
#include "hooks_verify.h"
#include "acl_hooks.h"

using namespace Sanitizer;

namespace {

struct AclLibLoader {
    static void *Load(void)
    {
        return RuntimeLibLoad("libascendcl.so");
    }
};

using AclSymbol = VallinaSymbol<AclLibLoader>;

// aclrtSetDevice 成功后上报设备类型（DEVICE_SUMMARY）。
// 新版 CANN 的 aclrtSetDevice 内部不再经过公开 rtSetDevice，需要在 ACL 层补报。
void ReportDeviceTypeAfterSetDevice() {
    // 直接用本文件已加载的 AclSymbol(libascendcl.so) 拿 soc 名，无需额外 dlopen libruntime.so
    using AclGetSocNameFn = const char *(*)();
    auto getSocName = AclSymbol::Instance().Get<AclGetSocNameFn>("aclrtGetSocName");
    if (getSocName == nullptr) {
        HOOK_LOG("aclrtGetSocName get FAILED");
        return;
    }
    char const *socName = getSocName();
    if (socName == nullptr || socName[0] == '\0') {
        HOOK_LOG("get soc name FAILED");
        return;
    }
    auto it = SOC_VERSION_MAP.find(socName);
    DeviceType deviceType = it == SOC_VERSION_MAP.cend() ? DeviceType::INVALID : it->second;
    if (it == SOC_VERSION_MAP.cend()) {
        std::cout << "[mssanitizer] unsupported soc version " << socName << std::endl;
    }
    HookReport::Instance().ReportDeviceType(deviceType);
}

}  // namespace Dummy

aclError aclrtMalloc(void **devPtr, size_t size, aclrtMemMallocPolicy policy)
{
    return sanitizerRtMalloc(devPtr, size, policy, nullptr, 0);
}

aclError aclrtMallocCached(void **devPtr, size_t size, aclrtMemMallocPolicy policy)
{
    return sanitizerRtMallocCached(devPtr, size, policy, nullptr, 0);
}

aclError aclrtFree(void *devPtr)
{
    return sanitizerRtFree(devPtr, nullptr, 0);
}

aclError aclrtFreeWithDevSync(void *devPtr) { return sanitizerRtFreeWithDevSync(devPtr, nullptr, 0); }

aclError aclrtSetDevice(int32_t deviceId) { return sanitizerRtSetDevice(deviceId, nullptr, 0); }

aclError sanitizerRtSetDevice(int32_t deviceId, char const *filename, int lineno) {
    (void)filename;
    (void)lineno;
    auto vallina = AclSymbol::Instance().Get<AclrtSetDevice>("aclrtSetDevice");
    if (vallina == nullptr) {
        HOOK_LOG("vallina func get FAILED");
        return -1;
    }

    aclError ret = vallina(deviceId);
    if (ret != 0) {
        return ret;
    }

    /// 设备设置成功后上报设备类型，服务端据此初始化 shadow memory 并应用 leak-check 配置
    ReportDeviceTypeAfterSetDevice();

    return ret;
}

aclError aclrtMemset(void *devPtr, size_t maxCount, int32_t value, size_t count)
{
    return sanitizerRtMemset(devPtr, maxCount, value, count, nullptr, 0);
}

aclError aclrtMemsetAsync(void *devPtr, size_t maxCount, int32_t value, size_t count, aclrtStream stream)
{
    return sanitizerRtMemsetAsync(devPtr, maxCount, value, count, stream, nullptr, 0);
}

aclError aclrtMemcpy(void *dst, size_t destMax, const void *src, size_t count, aclrtMemcpyKind kind)
{
    return sanitizerRtMemcpy(dst, destMax, src, count, kind, nullptr, 0);
}

aclError aclrtMemcpyAsync(void *dst, size_t destMax, const void *src, size_t count,
                          aclrtMemcpyKind kind, aclrtStream stream)
{
    return sanitizerRtMemcpyAsync(dst, destMax, src, count, kind, stream, nullptr, 0);
}

aclError aclrtMemcpy2d(void *dst, size_t dpitch, const void *src, size_t spitch,
                       size_t width, size_t height, aclrtMemcpyKind kind)
{
    return sanitizerRtMemcpy2d(dst, dpitch, src, spitch, width, height, kind, nullptr, 0);
}

aclError aclrtMemcpy2dAsync(void *dst, size_t dpitch, const void *src, size_t spitch,
                            size_t width, size_t height, aclrtMemcpyKind kind, aclrtStream stream)
{
    return sanitizerRtMemcpy2dAsync(dst, dpitch, src, spitch, width, height, kind, stream, nullptr, 0);
}

aclError sanitizerDvppMalloc(void **devPtr, size_t size, char const *filename, int lineno)
{
    auto vallina = AclSymbol::Instance().Get<AcldvppMalloc>("acldvppMalloc");
    if (vallina == nullptr) {
        HOOK_LOG("vallina func get FAILED");
        return -1;
    }

    aclError ret = vallina(devPtr, size);
    if (ret != 0) {
        return ret;
    }

    /// report to sanitizer here
    if (!devPtr) {
        HOOK_LOG("[ERROR] Argument devPtr should not be a null pointer.");
        return ret;
    }
    uint64_t addr = reinterpret_cast<uint64_t>(*devPtr);
    ReportAddrInfo addrInfo{addr, size, static_cast<uint64_t>(lineno), filename, MemInfoSrc::ACL};
    if (!HookReport::Instance().ReportMalloc(addrInfo, -1)) {
        HOOK_LOG("vallina func get FAILED");
    }

    return ret;
}

aclError sanitizerRtMalloc(void **devPtr, size_t size, aclrtMemMallocPolicy policy,
                           char const *filename, int lineno)
{
    auto vallina = AclSymbol::Instance().Get<AclMalloc>("aclrtMalloc");
    if (vallina == nullptr) {
        HOOK_LOG("vallina func get FAILED");
        return -1;
    }

    aclError ret = vallina(devPtr, size, policy);
    if (ret != 0) {
        return ret;
    }

    /// report to sanitizer here
    if (!devPtr) {
        HOOK_LOG("[ERROR] Argument devPtr should not be a null pointer.");
        return ret;
    }
    uint64_t addr = reinterpret_cast<uint64_t>(*devPtr);
    ReportAddrInfo addrInfo{addr, size, static_cast<uint64_t>(lineno), filename, MemInfoSrc::ACL};
    if (!HookReport::Instance().ReportMalloc(addrInfo, -1)) {
        HOOK_LOG("report to saniziter FAILED");
    }

    return ret;
}

aclError sanitizerRtMallocCached(void **devPtr, size_t size, aclrtMemMallocPolicy policy,
                                 char const *filename, int lineno)
{
    auto vallina = AclSymbol::Instance().Get<AclMallocCached>("aclrtMallocCached");
    if (vallina == nullptr) {
        HOOK_LOG("vallina func get FAILED");
        return -1;
    }

    aclError ret = vallina(devPtr, size, policy);
    if (ret != 0) {
        return ret;
    }

    /// report to sanitizer here
    if (!devPtr) {
        HOOK_LOG("[ERROR] Argument devPtr should not be a null pointer.");
        return ret;
    }
    uint64_t addr = reinterpret_cast<uint64_t>(*devPtr);
    ReportAddrInfo addrInfo{addr, size, static_cast<uint64_t>(lineno), filename, MemInfoSrc::ACL};
    if (!HookReport::Instance().ReportMalloc(addrInfo, -1)) {
        HOOK_LOG("report to saniziter FAILED");
    }

    return ret;
}

aclError sanitizerRtFree(void *devPtr, char const *filename, int lineno)
{
    auto vallina = AclSymbol::Instance().Get<AclrtFree>("aclrtFree");
    if (vallina == nullptr) {
        HOOK_LOG("vallina func get FAILED");
        return -1;
    }

    /// report to sanitizer here
    uint64_t addr = reinterpret_cast<uint64_t>(devPtr);
    ReportAddrInfo addrInfo{addr, 0, static_cast<uint64_t>(lineno), filename, MemInfoSrc::ACL};
    if (!HookReport::Instance().ReportFree(addrInfo)) {
        HOOK_LOG("report to saniziter FAILED");
    }

    aclError ret = vallina(devPtr);
    if (ret != 0) {
        return ret;
    }

    return ret;
}

aclError sanitizerRtFreeWithDevSync(void *devPtr, char const *filename, int lineno) {
    auto vallina = AclSymbol::Instance().Get<AclrtFreeWithDevSync>("aclrtFreeWithDevSync");
    if (vallina == nullptr) {
        HOOK_LOG("vallina func get FAILED");
        return -1;
    }

    /// report to sanitizer here
    uint64_t addr = reinterpret_cast<uint64_t>(devPtr);
    ReportAddrInfo addrInfo{addr, 0, static_cast<uint64_t>(lineno), filename, MemInfoSrc::ACL};
    if (!HookReport::Instance().ReportFree(addrInfo)) {
        HOOK_LOG("report to saniziter FAILED");
    }

    aclError ret = vallina(devPtr);
    if (ret != 0) {
        return ret;
    }

    return ret;
}

aclError sanitizerRtMemset(void *devPtr, size_t maxCount, int32_t value, size_t count,
                           char const *filename, int lineno)
{
    auto vallina = AclSymbol::Instance().Get<AclrtMemset>("aclrtMemset");
    if (vallina == nullptr) {
        HOOK_LOG("vallina func get FAILED");
        return -1;
    }

    aclError ret = vallina(devPtr, maxCount, value, count);

    /// report to sanitizer here
    uint64_t addr = reinterpret_cast<uint64_t>(devPtr);
    if (halIsDevicePtr(addr)) {
        ReportAddrInfo addrInfo{addr, count, static_cast<uint64_t>(lineno), filename, MemInfoSrc::ACL};
        if (!HookReport::Instance().ReportMemset(addrInfo)) {
            HOOK_LOG("report to saniziter FAILED");
        }
    }

    return ret;
}

aclError sanitizerRtMemsetAsync(void *devPtr, size_t maxCount, int32_t value, size_t count, aclrtStream stream,
                                char const *filename, int lineno)
{
    auto vallina = AclSymbol::Instance().Get<AclrtMemsetAsync>("aclrtMemsetAsync");
    if (vallina == nullptr) {
        HOOK_LOG("vallina func get FAILED");
        return -1;
    }

    aclError ret = vallina(devPtr, maxCount, value, count, stream);

    /// report to sanitizer here
    uint64_t addr = reinterpret_cast<uint64_t>(devPtr);
    if (halIsDevicePtr(addr)) {
        ReportAddrInfo addrInfo{addr, count, static_cast<uint64_t>(lineno), filename, MemInfoSrc::ACL};
        if (!HookReport::Instance().ReportMemset(addrInfo)) {
            HOOK_LOG("report to saniziter FAILED");
        }
    }

    return ret;
}

aclError sanitizerRtMemcpy(void *dst, size_t destMax, const void *src, size_t count, aclrtMemcpyKind kind,
                           char const *filename, int lineno)
{
    auto vallina = AclSymbol::Instance().Get<AclrtMemcpy>("aclrtMemcpy");
    if (vallina == nullptr) {
        HOOK_LOG("vallina func get FAILED");
        return -1;
    }

    aclError ret = vallina(dst, destMax, src, count, kind);

    /// report to sanitizer here
    uint64_t dstAddr = reinterpret_cast<uint64_t>(dst);
    uint64_t srcAddr = reinterpret_cast<uint64_t>(src);
    HookReport& report = HookReport::Instance();
    bool result = true;
    if (kind == ACL_MEMCPY_HOST_TO_DEVICE || kind == ACL_MEMCPY_DEVICE_TO_DEVICE) {
        ReportAddrInfo addrInfo{dstAddr, count, static_cast<uint64_t>(lineno), filename, MemInfoSrc::ACL};
        result &= report.ReportStore(addrInfo);
    }
    if (kind == ACL_MEMCPY_DEVICE_TO_HOST || kind == ACL_MEMCPY_DEVICE_TO_DEVICE) {
        ReportAddrInfo addrInfo{srcAddr, count, static_cast<uint64_t>(lineno), filename, MemInfoSrc::ACL};
        result &= report.ReportLoad(addrInfo);
    }
    if (!result) {
        HOOK_LOG("report to saniziter FAILED");
    }

    return ret;
}

aclError sanitizerRtMemcpyAsync(void *dst, size_t destMax, const void *src, size_t count,
                                aclrtMemcpyKind kind, aclrtStream stream, char const *filename, int lineno)
{
    auto vallina = AclSymbol::Instance().Get<AclrtMemcpyAsync>("aclrtMemcpyAsync");
    if (vallina == nullptr) {
        HOOK_LOG("vallina func get FAILED");
        return -1;
    }

    aclError ret = vallina(dst, destMax, src, count, kind, stream);

    /// report to sanitizer here
    uint64_t dstAddr = reinterpret_cast<uint64_t>(dst);
    uint64_t srcAddr = reinterpret_cast<uint64_t>(src);
    HookReport& report = HookReport::Instance();
    bool result = true;
    if (kind == ACL_MEMCPY_HOST_TO_DEVICE || kind == ACL_MEMCPY_DEVICE_TO_DEVICE) {
        ReportAddrInfo addrInfo{dstAddr, count, static_cast<uint64_t>(lineno), filename, MemInfoSrc::ACL};
        result &= report.ReportStore(addrInfo);
    }
    if (kind == ACL_MEMCPY_DEVICE_TO_HOST || kind == ACL_MEMCPY_DEVICE_TO_DEVICE) {
        ReportAddrInfo addrInfo{srcAddr, count, static_cast<uint64_t>(lineno), filename, MemInfoSrc::ACL};
        result &= report.ReportLoad(addrInfo);
    }
    if (!result) {
        HOOK_LOG("report to saniziter FAILED");
    }

    return ret;
}

aclError sanitizerRtMemcpy2d(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width, size_t height,
                             aclrtMemcpyKind kind, char const *filename, int lineno)
{
    auto vallina = AclSymbol::Instance().Get<AclrtMemcpy2d>("aclrtMemcpy2d");
    if (vallina == nullptr) {
        HOOK_LOG("vallina func get FAILED");
        return -1;
    }

    aclError ret = vallina(dst, dpitch, src, spitch, width, height, kind);

    /// report to sanitizer here
    uint64_t dstAddr = reinterpret_cast<uint64_t>(dst);
    uint64_t srcAddr = reinterpret_cast<uint64_t>(src);
    HookReport& report = HookReport::Instance();
    bool result = true;
    for (uint64_t r = 0; r < height; ++r) {
        if (r >= MAX_MEMORY_RECORD_HEIGHT) {  // height max num MAX_MEMORY_RECORD_HEIGHT Prevent infinite loops
            HOOK_LOG("halMemcpy2D pCopy copy2d height exceed 60*1024^3, skip others memcpy2d check");
            return ret;
        }
        if (kind == ACL_MEMCPY_HOST_TO_DEVICE || kind == ACL_MEMCPY_DEVICE_TO_DEVICE) {
            ReportAddrInfo addrInfo{dstAddr + r * dpitch, width, static_cast<uint64_t>(lineno),
                filename, MemInfoSrc::ACL};
            result &= report.ReportStore(addrInfo);
        }
        if (kind == ACL_MEMCPY_DEVICE_TO_HOST || kind == ACL_MEMCPY_DEVICE_TO_DEVICE) {
            ReportAddrInfo addrInfo{srcAddr + r * spitch, width, static_cast<uint64_t>(lineno),
                filename, MemInfoSrc::ACL};
            result &= report.ReportLoad(addrInfo);
        }
    }
    if (!result) {
        HOOK_LOG("report to saniziter FAILED");
    }

    return ret;
}

aclError sanitizerRtMemcpy2dAsync(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width, size_t height,
                                  aclrtMemcpyKind kind, aclrtStream stream, char const *filename, int lineno)
{
    auto vallina = AclSymbol::Instance().Get<AclrtMemcpy2dAsync>("aclrtMemcpy2dAsync");
    if (vallina == nullptr) {
        HOOK_LOG("vallina func get FAILED");
        return -1;
    }

    aclError ret = vallina(dst, dpitch, src, spitch, width, height, kind, stream);

    if (!IsValidRtMemcpy2dAsyncArg(dst, dpitch, src, spitch, height, kind)) {
    // 返回acl接口的错误码
        return ret;
    }

    /// report to sanitizer here
    uint64_t dstAddr = reinterpret_cast<uint64_t>(dst);
    uint64_t srcAddr = reinterpret_cast<uint64_t>(src);
    HookReport& report = HookReport::Instance();
    bool result = true;
    // 地址参数回绕防护
    for (uint64_t r = 0; r < height; ++r) {
        if (r >= MAX_MEMORY_RECORD_HEIGHT) {  // height max num MAX_MEMORY_RECORD_HEIGHT Prevent infinite loops
            HOOK_LOG("halMemcpy2D pCopy copy2d height exceed 60*1024^3, skip others memcpy2d check");
            return ret;
        }
        if (kind == ACL_MEMCPY_HOST_TO_DEVICE || kind == ACL_MEMCPY_DEVICE_TO_DEVICE) {
            ReportAddrInfo addrInfo{dstAddr + r * dpitch, width, static_cast<uint64_t>(lineno),
                filename, MemInfoSrc::ACL};
            result &= report.ReportStore(addrInfo);
        }
        if (kind == ACL_MEMCPY_DEVICE_TO_HOST || kind == ACL_MEMCPY_DEVICE_TO_DEVICE) {
            ReportAddrInfo addrInfo{srcAddr + r * spitch, width, static_cast<uint64_t>(lineno),
                filename, MemInfoSrc::ACL};
            result &= report.ReportLoad(addrInfo);
        }
    }
    if (!result) {
        HOOK_LOG("report to saniziter FAILED");
    }

    return ret;
}
