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
#ifndef ADDRESS_SANITIZER_ASAN_ACTION_H
#define ADDRESS_SANITIZER_ASAN_ACTION_H

#include "bounds_check.h"
#include "core/framework/config.h"
#include "core/framework/record_defs.h"
#include "mem_error_def.h"
#include "shadow_memory.h"

namespace Sanitizer {
// AsanAction 用于记录AddressSanitizer的操作内容
// 该类主要作用是，将对各类数据的处理，加工成为"标准"操作
// 并从AddressSanitizer中剥离，减少对主流程的干扰。
// 部分内存行为会同时加入到rtHeapBlock和dfxHeapBlock，会导致shadowMemory和HeapBlockManager运行两次出现问题
// 这种情况下ignoreShadowMemory字段为true，直接跳过shadowMemory的相关动作
class AsanAction {
public:
    explicit AsanAction(const MemOpRecord &record) : record_(record) {}
    virtual ~AsanAction() = default;
    virtual ErrorMsgList doAction(
        ShadowMemory &shadowMemory, BoundsCheck &boundsCheck, const Config &config, bool ignoreShadowMemory) = 0;

    MemOpRecord record_;
};

// 框架侧通过调用该函数，根据msg构造具体的AsanAction
class AsanActionFactory {
public:
    static std::shared_ptr<AsanAction> CreateAsanAction(const MemOpRecord &record);
};

class AsanMalloc : public AsanAction {
public:
    explicit AsanMalloc(const MemOpRecord &record) : AsanAction(record) {}
    ErrorMsgList doAction(
        ShadowMemory &shadowMemory, BoundsCheck &boundsCheck, const Config &config, bool ignoreShadowMemory) override;
};

class AsanFree : public AsanAction {
public:
    explicit AsanFree(const MemOpRecord &record) : AsanAction(record) {}
    ErrorMsgList doAction(
        ShadowMemory &shadowMemory, BoundsCheck &boundsCheck, const Config &config, bool ignoreShadowMemory) override;
};

class AsanMemcpyBlocks : public AsanAction {
public:
    explicit AsanMemcpyBlocks(const MemOpRecord &record) : AsanAction(record) {}
    ErrorMsgList doAction(
        ShadowMemory &shadowMemory, BoundsCheck &boundsCheck, const Config &config, bool ignoreShadowMemory) override;
};

// 该类对shadow memory指定内存进行load操作
class AsanLoad : public AsanAction {
public:
    explicit AsanLoad(const MemOpRecord &record) : AsanAction(record) {}
    ErrorMsgList doAction(
        ShadowMemory &shadowMemory, BoundsCheck &boundsCheck, const Config &config, bool ignoreShadowMemory) override;
};

class AsanStore : public AsanAction {
public:
    explicit AsanStore(const MemOpRecord &record) : AsanAction(record) {}
    ErrorMsgList doAction(
        ShadowMemory &shadowMemory, BoundsCheck &boundsCheck, const Config &config, bool ignoreShadowMemory) override;
};
}

#endif
