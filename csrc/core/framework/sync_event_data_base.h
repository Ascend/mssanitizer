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


#ifndef CORE_FRAMEWORK_SYNC_EVENT_DATA_BASE_H
#define CORE_FRAMEWORK_SYNC_EVENT_DATA_BASE_H

#include <queue>
#include <unordered_map>
#include <cstddef>
#include "core/framework/vector_clock.h"

namespace Sanitizer {

// 输入数据结构
union SyncEvent {
    struct {
        uint8_t srcPipe;
        uint8_t dstPipe;
        uint8_t eventId;
        uint8_t memType;
        bool isRetrogress;
    } info;
    
    size_t operator()(const SyncEvent &k) const
    {
        return std::hash<uint8_t>()(k.info.srcPipe) ^ std::hash<uint8_t>()(k.info.dstPipe) ^
            std::hash<uint8_t>()(k.info.eventId) ^ std::hash<uint8_t>()(k.info.memType) ^
            std::hash<uint8_t>()(k.info.isRetrogress);
    }
    
    bool operator()(const SyncEvent &k1, const SyncEvent &k2) const
    {
        return k1.info.srcPipe == k2.info.srcPipe && k1.info.dstPipe == k2.info.dstPipe &&
            k1.info.eventId == k2.info.eventId && k1.info.memType == k2.info.memType &&
            k1.info.isRetrogress == k2.info.isRetrogress;
    }
};

// 该类用于缓存算法运行过程中的同步事件，两个PIPE依此进行set/wait协同
// 注意:同一个set事件可能重复执行，且每个同步事件需要携带向量时间，所以需要使用queue来记录，先入先出
// 设置同步事件(携带向量时间)的能力
// 获取同步事件的能力
class SyncEventDataBase {
public:
    // set同步事件，并输入对应的向量时间
    void Set(const SyncEvent& event, const VectorTime &vt);
    // wait同步事件，并获取对应的向量时间
    bool Get(const SyncEvent& event, VectorTime &vt);

private:
    std::unordered_map<SyncEvent, std::queue<VectorTime>, SyncEvent, SyncEvent> syncDb_;
};
}
#endif
