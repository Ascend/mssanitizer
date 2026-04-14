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


#include "asan_action.h"
#include "core/framework/runtime_context.h"
#include "mem_error_def.h"

namespace {

using namespace Sanitizer;

inline void FillErrorLocInfo(MemOpRecord const &record, ErrorMsg &msg)
{
    msg.auxData.moduleId = record.moduleId;
    msg.auxData.fileName = record.fileName;
    msg.auxData.lineNo = record.lineNo;
    msg.auxData.coreId = record.coreId;
    msg.auxData.blockType = record.blockType;
    msg.auxData.pc = record.pc;
    msg.auxData.serialNo = record.serialNo;
    msg.auxData.side = record.side;
}

} // namespace [Dummy]

namespace Sanitizer {
std::shared_ptr<AsanAction> AsanActionFactory::CreateAsanAction(
    const MemOpRecord &record)
{
    using ActionCreateFunc = std::function<std::shared_ptr<AsanAction>(const MemOpRecord &)>;
    static const std::map<MemOpType, ActionCreateFunc> CREATE_FUNC_MAP = {
        {MemOpType::MALLOC,
            [](const MemOpRecord &record) { return std::make_shared<AsanMalloc>(record); }},
        {MemOpType::FREE,
            [](const MemOpRecord &record) { return std::make_shared<AsanFree>(record); }},
        {MemOpType::MEMCPY_BLOCKS,
            [](const MemOpRecord &record) { return std::make_shared<AsanMemcpyBlocks>(record); }},
        {MemOpType::LOAD,
            [](const MemOpRecord &record) { return std::make_shared<AsanLoad>(record); }},
        {MemOpType::STORE,
            [](const MemOpRecord &record) { return std::make_shared<AsanStore>(record); }},
    };

    auto it = CREATE_FUNC_MAP.find(record.type);
    if (it != CREATE_FUNC_MAP.end()) {
        return CREATE_FUNC_MAP.at(record.type)(record);
    }
    return nullptr;
}

ErrorMsgList AsanMalloc::doAction(ShadowMemory& shadowMemory, BoundsCheck &boundsCheck, const Config &config,
                                  bool ignoreShadowMemory)
{
    ErrorMsgList errorMsgs;
    if (record_.dstSpace != AddressSpace::GM) {
        return errorMsgs;
    }
    if (!ignoreShadowMemory && !shadowMemory.AddHeapBlock(record_)) {
        return errorMsgs;
    }
    // heap register之后的内存变为不可访问对于boundsCheck来说，是Remove的行为
    ErrorMsg errorMsg = record_.infoSrc == MemInfoSrc::MSTX_HEAP ?
        boundsCheck.Remove(record_.dstSpace, record_.dstAddr, record_.memSize) :
        boundsCheck.Add(record_.dstSpace, record_.dstAddr, record_.memSize);
    if (errorMsg.isError) {
        return {errorMsg};
    }
    return errorMsgs;
}

ErrorMsgList AsanFree::doAction(ShadowMemory& shadowMemory, BoundsCheck &boundsCheck, const Config &config,
                                bool ignoreShadowMemory)
{
    ErrorMsgList errorMsgs;
    if (record_.dstSpace != AddressSpace::GM) {
        return errorMsgs;
    }

    uint64_t size{};
    ErrorMsg msg{};
    if (!ignoreShadowMemory) {
        msg = shadowMemory.FreeHeapBlock(record_, size);
        if (msg.isError) {
            msg.auxData.badAddr.addr = record_.dstAddr;
            FillErrorLocInfo(record_, msg);
            errorMsgs.emplace_back(msg);
        }
    } else {
        size = record_.infoSrc == MemInfoSrc::BYPASS ?
            shadowMemory.GetHeapBlockBypassSize(record_) : shadowMemory.GetHeapBlockSize(record_);
    }
    // heap unregister之后的内存变为可访问，对于boundsCheck来说，是Add的行为
    msg = record_.infoSrc == MemInfoSrc::MSTX_HEAP ?
        boundsCheck.Add(record_.dstSpace, record_.dstAddr, size) :
        boundsCheck.Remove(record_.dstSpace, record_.dstAddr, size);
    if (msg.isError) {
        errorMsgs.emplace_back(msg);
    }
    if (config.checkUnusedMemory && !ignoreShadowMemory && record_.infoSrc != MemInfoSrc::BYPASS) {
        msg = shadowMemory.CheckUnusedMem(record_.dstAddr, size);
        if (msg.isError) {
            msg.auxData.side = record_.side;
            msg.auxData.serialNo = record_.serialNo;
            errorMsgs.emplace_back(msg);
        }
    }

    if (ignoreShadowMemory) { return errorMsgs; }
    if (record_.infoSrc == MemInfoSrc::EXTRA) {
        shadowMemory.ClearBlockId(record_.dstAddr, size);
    } else {
        shadowMemory.MakeMemUndefined(record_.dstAddr, size);
    }

    return errorMsgs;
}

ErrorMsgList AsanMemcpyBlocks::doAction(ShadowMemory& shadowMemory, BoundsCheck &boundsCheck, const Config &config,
                                        bool ignoreShadowMemory)
{
    std::swap(record_.srcAddr, record_.dstAddr);
    std::swap(record_.srcSpace, record_.dstSpace);
    AsanLoad asanLoad(record_);
    ErrorMsgList readErrorMsgs = asanLoad.doAction(shadowMemory, boundsCheck, config, ignoreShadowMemory);
    std::swap(record_.srcAddr, record_.dstAddr);
    std::swap(record_.srcSpace, record_.dstSpace);
    AsanStore asanStore(record_);
    ErrorMsgList writeErrorMsgs = asanStore.doAction(shadowMemory, boundsCheck, config, ignoreShadowMemory);
    readErrorMsgs.insert(readErrorMsgs.end(), writeErrorMsgs.cbegin(), writeErrorMsgs.cend());
    return readErrorMsgs;
}

ErrorMsgList AsanLoad::doAction(ShadowMemory& shadowMemory, BoundsCheck &boundsCheck, const Config &config,
                                bool ignoreShadowMemory)
{
    ErrorMsgList errorMsgs;
    if (config.memCheck && !record_.ignoreIllegalCheck) {
        ErrorMsg msg = boundsCheck.Check(record_.dstSpace, record_.dstAddr, record_.memSize);
        if (msg.isError) {
            msg.SetType(MemErrorType::ILLEGAL_ADDR_READ, record_.dstSpace, msg.auxData.badAddr.addr);
            FillErrorLocInfo(record_, msg);
            errorMsgs.emplace_back(msg);
        }
    }

    // 开启初始化检测后，不处理动态插桩算子的 load 事件
    bool isKernelWithDBI = RuntimeContext::Instance().kernelSummary_.isKernelWithDBI;
    if (config.initCheck && !ignoreShadowMemory && (record_.side == MemOpSide::HOST || !isKernelWithDBI)) {
        MemOpRecordForShadow memOpRevordForShadow(record_);
        ErrorMsgList loadErrors = shadowMemory.LoadNBytes(memOpRevordForShadow, config.initCheck);
        for (auto &msg : loadErrors) {
            FillErrorLocInfo(record_, msg);
        }
        errorMsgs.insert(errorMsgs.end(), loadErrors.cbegin(), loadErrors.cend());
    }
    return errorMsgs;
}

ErrorMsgList AsanStore::doAction(ShadowMemory& shadowMemory, BoundsCheck &boundsCheck, const Config &config,
                                 bool ignoreShadowMemory)
{
    ErrorMsgList errorMsgs;
    if (config.memCheck && !record_.ignoreIllegalCheck) {
        ErrorMsg msg = boundsCheck.Check(record_.dstSpace, record_.dstAddr, record_.memSize);
        if (msg.isError) {
            msg.SetType(MemErrorType::ILLEGAL_ADDR_WRITE, record_.dstSpace, msg.auxData.badAddr.addr);
            FillErrorLocInfo(record_, msg);
            errorMsgs.emplace_back(msg);
        }
    }

    // 写事件有两个条件需要进入 shadowmemory，一个是初始化检测，一个是多核踩踏
    if ((config.initCheck || (config.memCheck && record_.dstSpace == AddressSpace::GM)) && !ignoreShadowMemory) {
        MemOpRecordForShadow memOpRevordForShadow(record_);
        ErrorMsgList storeErrors = shadowMemory.StoreNBytes(memOpRevordForShadow, config.memCheck);
        for (auto &msg : storeErrors) {
            FillErrorLocInfo(record_, msg);
        }
        errorMsgs.insert(errorMsgs.end(), storeErrors.cbegin(), storeErrors.cend());
    }
    return errorMsgs;
}

}
