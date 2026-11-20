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

#ifndef __CORE_CONFIG_MANAGER_H__
#define __CORE_CONFIG_MANAGER_H__

#include "config.h"
#include "utility/singleton.h"

namespace Sanitizer {

class ConfigManager : public Singleton<ConfigManager> {
public:
    friend class Singleton<ConfigManager>;

    void Set(Config const &config) { config_ = config; }
    Config &Get() { return config_; }

    // 仅供主机检测使用，在启动检测线程前设置，不改变通信配置 Config。
    void SetInitcheckAddressSpaceMask(uint32_t mask) { initcheckAddressSpaceMask_ = mask; }
    uint32_t GetInitcheckAddressSpaceMask() const { return initcheckAddressSpaceMask_; }

private:
    Config config_{};
    uint32_t initcheckAddressSpaceMask_ = InitcheckAddressSpaceBit(AddressSpace::PRIVATE);
};

} // namespace Sanitizer

#endif // __CORE_CONFIG_MANAGER_H__
