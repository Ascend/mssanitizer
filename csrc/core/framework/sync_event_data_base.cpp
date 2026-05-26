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


#include "sync_event_data_base.h"

namespace Sanitizer {
void SyncEventDataBase::Set(const SyncEvent& event, const VectorTime &vt)
{
    syncDb_[event].push(vt);
}

bool SyncEventDataBase::Get(const SyncEvent& event, VectorTime &vt)
{
    auto it = syncDb_.find(event);
    if (it == syncDb_.end()) {
        return false;
    }

    if (it->second.empty()) {
        return false;
    }
    
    vt = it->second.front();
    it->second.pop();
    
    return true;
}
}