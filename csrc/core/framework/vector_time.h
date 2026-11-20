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


#ifndef CORE_FRAMEWORK_VECTOR_TIME_H
#define CORE_FRAMEWORK_VECTOR_TIME_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <initializer_list>
#include <utility>
#include <vector>

namespace Sanitizer {

using DenseVectorTime = std::vector<uint32_t>;

// FastTrack 式稀疏向量时钟：向量时钟各分量初值/缺省值均为 1，多数分量恒为 1，
// 因此这里只存 != 1 的分量（按 idx 升序），缺省分量取 1。
// 用于 MemEvent 携带的时间戳，避免逐事件存整条 N 宽向量（及其堆分配）。
struct VectorTime {
    static constexpr uint16_t INLINE_CAP = 48;
    uint16_t n = 0;
    bool heap = false;
    uint16_t idx[INLINE_CAP];
    uint32_t val[INLINE_CAP];
    std::vector<std::pair<uint32_t, uint32_t>> heapEntries;  // 仅 heap==true 时使用，按 idx 升序

    VectorTime() = default;

    bool empty() const { return n == 0U && heapEntries.empty(); }
    void clear();

    // 便于测试/构造：按位置初始化（分量值 <=1 即缺省，不存储）
    VectorTime(std::initializer_list<uint32_t> l);

    bool EqualTo(const VectorTime &o) const;
    bool operator==(const VectorTime &o) const { return EqualTo(o) && o.EqualTo(*this); }

    static VectorTime FromFull(const DenseVectorTime &full);
    VectorTime &operator=(const DenseVectorTime &full);

    uint32_t Get(uint32_t i) const
    {
        if (!heap) {
            // 内联数组只存 uint16_t 索引；超出范围的分量必然为缺省值 1
            if (i > UINT16_MAX) { return 1U; }
            // idx 恒为升序：二分查找
            int lo = 0;
            int hi = static_cast<int>(n) - 1;
            while (lo <= hi) {
                const int m = (lo + hi) >> 1;
                const uint16_t im = idx[m];
                if (im == i) { return val[m]; }
                if (im < i) { lo = m + 1; } else { hi = m - 1; }
            }
            return 1U;
        }
        auto it = std::lower_bound(heapEntries.begin(), heapEntries.end(), i,
            [](const std::pair<uint32_t, uint32_t> &a, uint32_t b) { return a.first < b; });
        if (it != heapEntries.end() && it->first == i) { return it->second; }
        return 1U;
    }

    // 置分量 i 为 max(现有, v)（v<=1 即缺省值，不存储）；用于逻辑时间自增与向量时间合并
    void Set(uint32_t i, uint32_t v)
    {
        if (v <= 1U) { return; }
        if (!heap && i > UINT16_MAX) {
            // 内联索引无法表示大索引，直接迁移到 heap 表示
            heap = true;
            heapEntries.reserve(static_cast<std::size_t>(n) + 1U);
            for (uint16_t t = 0; t < n; ++t) { heapEntries.emplace_back(idx[t], val[t]); }
            n = 0;
        }
        if (!heap) {
            // idx 升序：二分定位（lo 为插入点）
            int lo = 0;
            int hi = static_cast<int>(n) - 1;
            while (lo <= hi) {
                const int m = (lo + hi) >> 1;
                const uint16_t im = idx[m];
                if (im == i) {
                    if (v > val[m]) { val[m] = v; }
                    return;
                }
                if (im < i) { lo = m + 1; } else { hi = m - 1; }
            }
            const uint16_t pos = static_cast<uint16_t>(lo);
            if (n < INLINE_CAP) {
                for (uint16_t k = n; k > pos; --k) {
                    idx[k] = idx[k - 1U];
                    val[k] = val[k - 1U];
                }
                idx[pos] = static_cast<uint16_t>(i);
                val[pos] = v;
                ++n;
                return;
            }
            // 内联已满：迁移到堆，再走下面堆插入
            heap = true;
            heapEntries.reserve(static_cast<std::size_t>(n) + 1U);
            for (uint16_t t = 0; t < n; ++t) { heapEntries.emplace_back(idx[t], val[t]); }
            n = 0;
        }
        auto it = std::lower_bound(heapEntries.begin(), heapEntries.end(), i,
            [](const std::pair<uint32_t, uint32_t> &a, uint32_t b) { return a.first < b; });
        if (it != heapEntries.end() && it->first == i) {
            if (v > it->second) { it->second = v; }
        } else {
            heapEntries.insert(it, std::make_pair(i, v));
        }
    }

    // 有序稀疏列表线性归并取 max（原地更新 *this），避免逐分量 Set 的二分/搬移开销
    void MergeMax(const VectorTime &in);
};

// TIME_EVENT 的源 pipe 时钟快照池：SanEvent 只存指针（见 SanEvent::timeInfo），
// 避免把 320B 的 VectorTime 内联进每个 SanEvent。std::deque 保证元素地址稳定；
// freelist 复用已消费的槽位，使池大小受限于各 pipe 的在飞事件数。
struct TimeInfoPool {
    std::deque<VectorTime> pool;
    std::vector<VectorTime *> freeList;

    VectorTime *Alloc(const VectorTime &v);
    void Free(VectorTime *p);
};

}

#endif
