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


#ifndef ALIGN_CHECKER_H
#define ALIGN_CHECKER_H

#include <mutex>

#include "address_sanitizer.h"
#include "error_buffer.h"
#include "core/framework/utility/singleton.h"

namespace Sanitizer {

// 如果注册了通知函数，则CheckAlign会根据入参检查对齐，不满足则通过通知函数输出错误信息
class AlignChecker : public ThreadSingleton<AlignChecker> {
public:
    // 该函数根据入参alignSize，检查event的addr是否对齐,同时将alignSize赋值给event
    void CheckAlign(SanEvent &event, uint16_t alignSize);

    // 该函数根据入参event、recordType获取alignSize，检查event的addr是否对齐
    void CheckAlign(SanEvent &event, RecordType recordType);

    void RegisterNotifyFunc(const SanitizerBase::MSG_FUNC &func)
    {
        msgFunc_ = func;
    }
    void UnregisterNotifyFunc()
    {
        msgFunc_ = nullptr;
    }

    void SetDeviceType(DeviceType deviceType)
    {
        deviceType_ = deviceType;
    }

    void Notify();

    friend class std::pair<const std::thread::id, AlignChecker>;

private:
    AlignChecker() : msgFunc_(nullptr), deviceType_(DeviceType::INVALID) {};
    ~AlignChecker();

    SanitizerBase::MSG_FUNC msgFunc_;
    // 解析 worker 会并发调用 CheckAlign，错误低频，用一把锁保护 errorBuffer_ 即可
    std::mutex errMutex_;
    ErrorBuffer errorBuffer_;
    DeviceType deviceType_;
};

}  // namespace Sanitizer

#endif
