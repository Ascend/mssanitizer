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

#include <iostream>

#include "runtime.h"
#include "acl_hooks.h"
#include "vallina_symbol.h"
#include "hook_report.h"
#include "hook_logger.h"

using namespace Sanitizer;

namespace {

struct RuntimeLibLoader {
    static void *Load(void) { return RuntimeLibLoad("libruntime.so"); }
};

using RuntimeSymbol = VallinaSymbol<RuntimeLibLoader>;

struct AclRtImplLibLoader {
    static void *Load(void) { return RuntimeLibLoad("libacl_rt_impl.so"); }
};

using AclRtImplSymbol = VallinaSymbol<AclRtImplLibLoader>;

/// aclrtXXXImpl 符号优先从 libruntime.so 获取；旧版 libruntime.so 未包含时回退到 libacl_rt_impl.so
template <typename Func> Func GetAclRtImplSymbol(const char *symbol) {
    Func fn = RuntimeSymbol::Instance().Get<Func>(symbol);
    if (fn != nullptr) {
        return fn;
    }
    return AclRtImplSymbol::Instance().Get<Func>(symbol);
}

/// rtSetDevice / aclrtSetDeviceImpl 公共流程：日志、设置设备、按 soc 版本上报设备类型
template <typename Ret, typename SocGetter>
Ret SetDeviceAndReport(int32_t devId, const char *funcName, Ret (*vallina)(int32_t), SocGetter &&getSocVersion) {
    /// 根据该日志信息判断算子是否上板
    HOOK_LOG("%s enable success device %d", funcName, devId);

    if (vallina == nullptr) {
        std::cout << "[" << funcName << "] vallina func get FAILED" << std::endl;
        return static_cast<Ret>(RT_ERROR_RESERVED);
    }

    /// 必须在设置了当前的 deviceId 之后才能获取芯片型号
    std::string socVersion = getSocVersion();
    if (!socVersion.empty()) {
        HOOK_LOG("%s soc version %s", funcName, socVersion.c_str());
        typename decltype(SOC_VERSION_MAP)::const_iterator it = SOC_VERSION_MAP.find(socVersion);
        DeviceType deviceType = it == SOC_VERSION_MAP.cend() ? DeviceType::INVALID : it->second;
        HookReport::Instance().ReportDeviceType(deviceType);
        if (it == SOC_VERSION_MAP.cend()) {
            std::cout << "[mssanitizer] unsupported soc version " << socVersion << std::endl;
        }
    } else {
        std::cout << "[" << funcName << "] get soc version FAILED" << std::endl;
    }
    // check-device-heap场景, setDevice会调用大量的hal Malloc 和 Copy, 所以先上报socversion
    Ret ret = vallina(devId);
    return ret;
}

} // namepasce Dummy

RTS_API rtError_t rtSetDevice(int32_t devId) {
    using RtSetDevice = decltype(&rtSetDevice);
    using RtGetSocVersion = decltype(&rtGetSocVersion);
    const char *funcName = __func__;
    auto vallina = RuntimeSymbol::Instance().Get<RtSetDevice>(funcName);
    auto getSocVersion = [funcName]() -> std::string {
        auto vallinaRtGetSocVersion = RuntimeSymbol::Instance().Get<RtGetSocVersion>("rtGetSocVersion");
        if (vallinaRtGetSocVersion == nullptr) {
            std::cout << "[" << funcName << "] get vallina func rtGetSocVersion FAILED" << std::endl;
            return "";
        }
        constexpr uint64_t socVersionBufLen = 64UL;
        char socVersion[socVersionBufLen] = "";
        if (vallinaRtGetSocVersion(socVersion, sizeof(socVersion)) != RT_ERROR_NONE) {
            return "";
        }
        return std::string(socVersion);
    };
    return SetDeviceAndReport<rtError_t>(devId, funcName, vallina, std::move(getSocVersion));
}

aclError aclrtSetDeviceImpl(int32_t devId) {
    using AclrtSetDevice = decltype(&aclrtSetDeviceImpl);
    using AclrtGetSocName = decltype(&aclrtGetSocNameImpl);
    const char *funcName = __func__;
    auto vallina = GetAclRtImplSymbol<AclrtSetDevice>("aclrtSetDeviceImpl");
    auto getSocVersion = [funcName]() -> std::string {
        auto getSocName = GetAclRtImplSymbol<AclrtGetSocName>("aclrtGetSocNameImpl");
        if (getSocName == nullptr) {
            std::cout << "[" << funcName << "] get vallina func aclrtGetSocNameImpl FAILED" << std::endl;
            return "";
        }
        const char *socName = getSocName();
        return socName == nullptr ? "" : std::string(socName);
    };
    return SetDeviceAndReport<aclError>(devId, funcName, vallina, std::move(getSocVersion));
}
