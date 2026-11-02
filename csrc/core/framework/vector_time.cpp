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


#include "vector_time.h"

#include <algorithm>

namespace Sanitizer {

void VectorTime::clear() {
    n = 0U;
    heap = false;
    heapEntries.clear();
}

VectorTime::VectorTime(std::initializer_list<uint32_t> l) {
    uint32_t i = 0U;
    for (uint32_t v : l) { Set(i, v); ++i; }
}

bool VectorTime::EqualTo(const VectorTime &o) const {
    if (!heap) {
        for (uint16_t k = 0; k < n; ++k) {
            if (o.Get(idx[k]) != val[k]) { return false; }
        }
    } else {
        for (const auto &e : heapEntries) {
            if (o.Get(e.first) != e.second) { return false; }
        }
    }
    return true;
}

VectorTime VectorTime::FromFull(const DenseVectorTime &full) {
    VectorTime s;
    uint32_t cnt = 0;
    bool hasLargeIdx = false;
    for (uint32_t i = 0; i < full.size(); ++i) {
        if (full[i] != 1U) {
            ++cnt;
            hasLargeIdx = hasLargeIdx || (i > UINT16_MAX);
        }
    }
    if (cnt <= INLINE_CAP && !hasLargeIdx) {
        uint16_t k = 0;
        for (uint32_t i = 0; i < full.size(); ++i) {
            if (full[i] != 1U) {
                s.idx[k] = static_cast<uint16_t>(i);
                s.val[k] = full[i];
                ++k;
            }
        }
        s.n = k;
    } else {
        s.heap = true;
        s.heapEntries.reserve(cnt);
        for (uint32_t i = 0; i < full.size(); ++i) {
            if (full[i] != 1U) { s.heapEntries.emplace_back(i, full[i]); }
        }
    }
    return s;
}

VectorTime &VectorTime::operator=(const DenseVectorTime &full) {
    *this = FromFull(full);
    return *this;
}

namespace {

// 双方均为内联且结果仍可内联：栈上临时数组线性归并
bool MergeInline(VectorTime &out, const VectorTime &in, std::size_t inN)
{
    if (out.heap || in.heap ||
        static_cast<std::size_t>(out.n) + inN > VectorTime::INLINE_CAP) {
        return false;
    }

    uint16_t mergedIdx[VectorTime::INLINE_CAP * 2U];
    uint32_t mergedVal[VectorTime::INLINE_CAP * 2U];
    std::size_t i = 0U;
    std::size_t j = 0U;
    std::size_t k = 0U;

    while (i < out.n && j < inN) {
        if (out.idx[i] < in.idx[j]) {
            mergedIdx[k] = out.idx[i];
            mergedVal[k] = out.val[i];
            ++i;
        } else if (out.idx[i] > in.idx[j]) {
            mergedIdx[k] = in.idx[j];
            mergedVal[k] = in.val[j];
            ++j;
        } else {
            mergedIdx[k] = out.idx[i];
            mergedVal[k] = std::max(out.val[i], in.val[j]);
            ++i;
            ++j;
        }
        ++k;
    }
    while (i < out.n) {
        mergedIdx[k] = out.idx[i];
        mergedVal[k] = out.val[i];
        ++i;
        ++k;
    }
    while (j < inN) {
        mergedIdx[k] = in.idx[j];
        mergedVal[k] = in.val[j];
        ++j;
        ++k;
    }
    for (std::size_t t = 0U; t < k; ++t) {
        out.idx[t] = mergedIdx[t];
        out.val[t] = mergedVal[t];
    }
    out.n = static_cast<uint16_t>(k);
    return true;
}

// 把内联表示迁移到 heap，保持 idx 升序
void MigrateToHeap(VectorTime &out)
{
    if (out.heap) {
        return;
    }
    out.heapEntries.reserve(out.n);
    for (uint16_t t = 0U; t < out.n; ++t) {
        out.heapEntries.emplace_back(out.idx[t], out.val[t]);
    }
    out.n = 0U;
    out.heap = true;
}

// 两个有序稀疏列表在 heap 上线性归并取 max
void MergeHeap(VectorTime &out, const VectorTime &in, std::size_t inN)
{
    const std::size_t outN = out.heapEntries.size();

    auto inIdx = [&in](std::size_t pos) {
        return in.heap ? in.heapEntries[pos].first
                       : static_cast<uint32_t>(in.idx[pos]);
    };
    auto inVal = [&in](std::size_t pos) {
        return in.heap ? in.heapEntries[pos].second : in.val[pos];
    };

    // 先统计去重后的结果长度，避免容量累积到 outN + inN
    std::size_t resultCount = 0U;
    {
        std::size_t i = 0U;
        std::size_t j = 0U;
        while (i < outN && j < inN) {
            const uint32_t outIndex = out.heapEntries[i].first;
            const uint32_t inIndex = inIdx(j);
            if (outIndex <= inIndex) { ++i; }
            if (inIndex <= outIndex) { ++j; }
            ++resultCount;
        }
        resultCount += (outN - i) + (inN - j);
    }

    if (out.heapEntries.capacity() < resultCount) {
        out.heapEntries.reserve(std::max(resultCount, out.heapEntries.capacity() * 2U));
    }
    out.heapEntries.resize(resultCount);

    // 从后往前就地归并
    std::size_t i = outN;
    std::size_t j = inN;
    std::size_t k = resultCount;
    while (i > 0U && j > 0U) {
        const uint32_t outIndex = out.heapEntries[i - 1U].first;
        const uint32_t inIndex = inIdx(j - 1U);
        --k;
        if (outIndex > inIndex) {
            out.heapEntries[k] = out.heapEntries[--i];
        } else if (outIndex < inIndex) {
            out.heapEntries[k] = std::make_pair(inIndex, inVal(j - 1U));
            --j;
        } else {
            out.heapEntries[k] = std::make_pair(
                outIndex, std::max(out.heapEntries[i - 1U].second, inVal(j - 1U)));
            --i;
            --j;
        }
    }
    while (i > 0U) {
        out.heapEntries[--k] = out.heapEntries[--i];
    }
    while (j > 0U) {
        out.heapEntries[--k] = std::make_pair(inIdx(j - 1U), inVal(j - 1U));
        --j;
    }
}

} // namespace

void VectorTime::MergeMax(const VectorTime &in)
{
    if (this == &in || in.empty()) {
        return;
    }
    const std::size_t inN = in.heap ? in.heapEntries.size() : in.n;
    if (MergeInline(*this, in, inN)) {
        return;
    }
    MigrateToHeap(*this);
    MergeHeap(*this, in, inN);
}

VectorTime *TimeInfoPool::Alloc(const VectorTime &v) {
    if (!freeList.empty()) {
        VectorTime *p = freeList.back();
        freeList.pop_back();
        *p = v;
        return p;
    }
    pool.emplace_back(v);
    return &pool.back();
}

void TimeInfoPool::Free(VectorTime *p) {
    if (p != nullptr) {
        freeList.push_back(p);
    }
}

}
