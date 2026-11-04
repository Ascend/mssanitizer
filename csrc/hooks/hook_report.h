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


#ifndef __CORE_HAL_HOOK_REPORT_H__
#define __CORE_HAL_HOOK_REPORT_H__

#include <string>
#include <memory>
#include <mutex>
#include <vector>

#include "communication.h"
#include "platform_config.h"
#include "protocol.h"
#include "record_defs.h"
#include "arch_def.h"

namespace Sanitizer {

struct ReportAddrInfo {
    uint64_t addr{};
    uint64_t size{};
    uint64_t lineNo{};
    char const *fileName{};
    MemInfoSrc infoSrc{};

    /// 赋值 addr/infoSrc
    ReportAddrInfo(uint64_t addr, MemInfoSrc infoSrc) : addr(addr), infoSrc(infoSrc) {}

    /// 赋值 addr/size/infoSrc
    ReportAddrInfo(uint64_t addr, uint64_t size, MemInfoSrc infoSrc) : addr(addr), size(size), infoSrc(infoSrc) {}

    /// 赋值 all
    ReportAddrInfo(uint64_t addr, uint64_t size, uint64_t lineNo, char const *fileName, MemInfoSrc infoSrc)
        : addr(addr), size(size), lineNo(lineNo), fileName(fileName), infoSrc(infoSrc) {}
};

class HookReport {
public:
    uint64_t pcStartAddr;

public:
    static HookReport& Instance(void);

    HookReport(HookReport const&) = delete;
    HookReport& operator=(HookReport const&) = delete;

    bool InitClient(void);
    bool ReportDeviceType(DeviceType deviceType);
    bool ReportKernelInfo();
    bool ReportMemOp(MemOpRecord const &record);
    bool Report(SanitizerRecord &sanitizerRecord);
    bool ReportMalloc(const ReportAddrInfo &addrInfo, int32_t moduleId = -1);
    bool ReportFree(const ReportAddrInfo &addrInfo);
    bool ReportMemset(const ReportAddrInfo &addrInfo);
    bool ReportMemcpy(const ReportAddrInfo &addrInfo, uint64_t srcAddr);
    bool ReportLoad(const ReportAddrInfo &addrInfo);
    bool ReportStore(const ReportAddrInfo &addrInfo);
    bool ReportKernelBinary(std::vector<char> const &binary);
    bool ReportLogString(std::string const &logString);

    bool ReportMalloc(uint64_t addr, uint64_t size, MemInfoSrc memInfoSrc);
    bool ReportFree(uint64_t addr, MemInfoSrc memInfoSrc);
    bool ReportLoad(uint64_t addr, uint64_t size, MemInfoSrc memInfoSrc);
    bool ReportStore(uint64_t addr, uint64_t size, MemInfoSrc memInfoSrc);
    void DetermineMemInfoSrc(void);

    void SetBlockDim(uint32_t blockDim);
    DeviceType GetDeviceType(void);
    void SetKernelType(KernelType kernelType);
    KernelInfo GetKernelInfo(void);
    CheckParmsInfo GetCheckParms(void);

private:
    HookReport(void);
    // 本进程的 HookReport 连接按发送顺序给每个完整业务包编号。
    bool WriteOrdered(std::string message);

private:
    std::unique_ptr<CommunicationClient> client_;
    std::mutex sendMutex_;
    uint64_t nextOrderId_{1};
    bool determined_;
    MemInfoSrc memInfoSrc_;
    DeviceType deviceType_ = DeviceType::INVALID;
    uint32_t blockDim_ = 0U;
    std::vector<std::pair<MemInfoSrc, MemOpRecord>> recordBuffer_;
    KernelInfo kernelInfo_{};
    CheckParmsInfo checkParms_{};
    uint64_t serialNo_ = 0;
};

}  // namespace Sanitizer

#endif  // __CORE_HAL_HOOK_REPORT_H__
