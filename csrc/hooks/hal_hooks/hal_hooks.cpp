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


#include <string>
#include <iostream>
#include <dlfcn.h>
#include <cstdint>

#include "hook_report.h"
#include "hook_logger.h"
#include "hooks_verify.h"
#include "vallina_symbol.h"
#include "hal_hooks.h"

using namespace Sanitizer;

namespace {

struct HalLibLoader {
    static void *Load(void) {
        // libascend_hal.so 由 driver 提供，不在 $ASCEND_HOME_PATH/lib64 下，
        // 直接按 soname 交由动态链接器（LD_LIBRARY_PATH）解析。
        return dlopen("libascend_hal.so", RTLD_NOW | RTLD_GLOBAL);
    }
};

using HalSymbol = VallinaSymbol<HalLibLoader>;

/// 从真实 libascend_hal.so 中解析同名公有函数，穿透到原始实现
template <typename Func> Func RealHal(char const *symbol) {
    auto fn = HalSymbol::Instance().Get<Func>(symbol);
    if (fn == nullptr) {
        HOOK_LOG("vallina func %s get FAILED", symbol);
    }
    return fn;
}

} // namespace

inline int32_t GetMallocModuleId(unsigned long long flag)
{
    constexpr uint64_t moduleIdMask = (1U << 9) - 1;
    return flag & moduleIdMask;
}

drvError_t halMemAlloc(void **pp, unsigned long long size, unsigned long long flag)
{
    auto real = RealHal<HalMemAlloc>(__func__);
    if (real == nullptr) {
        return DRV_ERROR_INNER_ERR;
    }
    drvError_t ret = real(pp, size, flag);
    if (ret != DRV_ERROR_NONE) {
        return ret;
    }

    /// report to sanitizer here
    if (!pp) {
        HOOK_LOG("Sanitizer fail to report memory allocation. Argument pp should not be a null pointer.");
        return ret;
    }
    uint64_t addr = reinterpret_cast<uint64_t>(*pp);
    int32_t moduleId = GetMallocModuleId(flag);
    ReportAddrInfo addrInfo{addr, size, MemInfoSrc::HAL};
    if (!HookReport::Instance().ReportMalloc(addrInfo, moduleId)) {
        HOOK_LOG("report to saniziter FAILED");
    }

    return ret;
}

drvError_t halMemFree(void *pp)
{
    /// report to sanitizer here
    uint64_t addr = reinterpret_cast<uint64_t>(pp);
    ReportAddrInfo addrInfo{addr, MemInfoSrc::HAL};
    if (!HookReport::Instance().ReportFree(addrInfo)) {
        HOOK_LOG("report to saniziter FAILED");
    }

    auto real = RealHal<HalMemFree>(__func__);
    if (real == nullptr) {
        return DRV_ERROR_INNER_ERR;
    }
    drvError_t ret = real(pp);
    if (ret != DRV_ERROR_NONE) {
        return ret;
    }

    return ret;
}

drvError_t drvMemsetD8(DVdeviceptr dst, size_t destMax, uint8_t value, size_t num)
{
    auto real = RealHal<DrvMemsetD8>(__func__);
    if (real == nullptr) {
        return DRV_ERROR_INNER_ERR;
    }
    drvError_t ret = real(dst, destMax, value, num);
    ReportAddrInfo addrInfo{dst, num, MemInfoSrc::HAL};
    /// report to sanitizer here
    if (halIsDevicePtr(dst)) {
        if (!HookReport::Instance().ReportMemset(addrInfo)) {
            HOOK_LOG("report to saniziter FAILED");
        }
    }

    return ret;
}

drvError_t drvMemcpy(DVdeviceptr dst, size_t destMax, DVdeviceptr src, size_t byteCount)
{
    auto real = RealHal<DrvMemcpy>(__func__);
    if (real == nullptr) {
        return DRV_ERROR_INNER_ERR;
    }
    drvError_t ret = real(dst, destMax, src, byteCount);
    ReportAddrInfo addrInfo{dst, byteCount, MemInfoSrc::HAL};
    /// report to sanitizer here
    HookReport& report = HookReport::Instance();
    bool result = true;
    if (halIsDevicePtr(dst)) {
        result &= report.ReportStore(addrInfo);
    }
    if (halIsDevicePtr(src)) {
        addrInfo.addr = src;
        result &= report.ReportLoad(addrInfo);
    }
    if (!result) {
        HOOK_LOG("report to saniziter FAILED");
    }

    return ret;
}

drvError_t halMemCpyAsync(DVdeviceptr dst, size_t destMax, DVdeviceptr src,
                          size_t byteCount, uint64_t *copyFd)
{
    auto real = RealHal<HalMemCpyAsync>(__func__);
    if (real == nullptr) {
        return DRV_ERROR_INNER_ERR;
    }
    drvError_t ret = real(dst, destMax, src, byteCount, copyFd);
    ReportAddrInfo addrInfo{dst, byteCount, MemInfoSrc::HAL};
    /// report to sanitizer here
    HookReport& report = HookReport::Instance();
    bool result = true;
    if (halIsDevicePtr(dst)) {
        result &= report.ReportStore(addrInfo);
    }
    if (halIsDevicePtr(src)) {
        addrInfo.addr = src;
        result &= report.ReportLoad(addrInfo);
    }
    if (!result) {
        HOOK_LOG("report to saniziter FAILED");
    }

    return ret;
}

drvError_t halMemcpy2D(MEMCPY2D *pCopy)
{
    auto real = RealHal<HalMemcpy2D>(__func__);
    if (real == nullptr) {
        return DRV_ERROR_INNER_ERR;
    }
    drvError_t ret = real(pCopy);

    if (!IsValidHalMemcpy2DArg(pCopy)) {
        return ret;
    }

    /// report to sanitizer here
    /// 同步异步两种情况均使用此接口进行上报
    if (pCopy->type == DEVMM_MEMCPY2D_SYNC || pCopy->type == DEVMM_MEMCPY2D_ASYNC_CONVERT) {
        uint64_t dst = reinterpret_cast<uint64_t>(pCopy->copy2d.dst);
        uint64_t src = reinterpret_cast<uint64_t>(pCopy->copy2d.src);
        bool isDstDevicePtr = halIsDevicePtr(dst);
        bool isSrcDevicePtr = halIsDevicePtr(src);
        uint64_t dpitch = pCopy->copy2d.dpitch;
        uint64_t spitch = pCopy->copy2d.spitch;
        HookReport& report = HookReport::Instance();
        bool result = true;
        for (uint64_t r = 0; r < pCopy->copy2d.height; ++r) {
            if (r >= MAX_MEMORY_RECORD_HEIGHT) {  // height max num MAX_MEMORY_RECORD_HEIGHT Prevent infinite loops
                HOOK_LOG("halMemcpy2D pCopy copy2d height exceed 60*1024^3, skip others memcpy2d check");
                return ret;
            }
            ReportAddrInfo addrInfo{dst + r * dpitch, pCopy->copy2d.width, MemInfoSrc::HAL};
            if (isDstDevicePtr) {
                result &= report.ReportStore(addrInfo);
            }
            if (isSrcDevicePtr) {
                addrInfo.addr = src + r * spitch;
                result &= report.ReportLoad(addrInfo);
            }
        }
        if (!result) {
            HOOK_LOG("report to saniziter FAILED");
        }
    }

    return ret;
}
