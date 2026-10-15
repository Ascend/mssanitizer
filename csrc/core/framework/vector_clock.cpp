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


#include "vector_clock.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>

#if defined(__aarch64__) || defined(__ARM_NEON)
#include <arm_neon.h>
#elif defined(__x86_64__) || defined(_M_X64) || defined(__SSE2__)
#include <immintrin.h> // x86：SSE2 / SSE4.1 / AVX2 全部 intrinsic 声明
#endif

namespace Sanitizer {

void VectorClock::UpdateLogicTime(VectorTime &t, uint32_t pipe)
{
    t[pipe]++;
}

/// 更新完向量时间之后，对应pipe的逻辑时间需要 +1
/// 逐元素无符号 max，各架构用 SIMD 加速，语义与标量完全等价（整数 max 精确）。
///
/// 原本（标量）功能伪代码：
///   n = min(in.size(), out.size())
///   for i in [0, n):
///       out[i] = max(in[i], out[i])   // 逐元素无符号最大值
///   // 结果：out = elementwise_max(out, in)
///
/// 即把 in 的每个分量按最大值合并进 out，用于将另一份向量时间传播/并入当前向量时间。
void VectorClock::UpdateVectorTime(const VectorTime &in, VectorTime &out)
{
    const std::size_t count = std::min(in.size(), out.size());
    if (count == 0U) {
        return;
    }
    const uint32_t *a = in.data();
    uint32_t *b = out.data();
    std::size_t i = 0U;

#if defined(__aarch64__) || defined(__ARM_NEON)
    // aarch64: NEON 4 x uint32 无符号 max（vmaxq_u32 支持非对齐 vld1q/vst1q）
    for (; i + 4U <= count; i += 4U) {
        vst1q_u32(b + i, vmaxq_u32(vld1q_u32(a + i), vld1q_u32(b + i)));
    }
#elif defined(__x86_64__) || defined(_M_X64) || defined(__SSE2__)
#if defined(__AVX2__)
    // x86_64 + AVX2: 8 x uint32 无符号 max
    for (; i + 8U <= count; i += 8U) {
        __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(a + i));
        __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(b + i));
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(b + i), _mm256_max_epu32(va, vb));
    }
#endif
#if defined(__SSE4_1__)
    // x86_64 + SSE4.1: 4 x uint32 无符号 max（PMAXUD）
    for (; i + 4U <= count; i += 4U) {
        __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i *>(a + i));
        __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i *>(b + i));
        _mm_storeu_si128(reinterpret_cast<__m128i *>(b + i), _mm_max_epu32(va, vb));
    }
#else
    // 仅 SSE2（无 32 位 max 指令）：异或 0x80000000 转有符号序，_mm_cmpgt_epi32 比较后按掩码选择
    const __m128i signMask = _mm_set1_epi32(static_cast<int>(0x80000000u));
    for (; i + 4U <= count; i += 4U) {
        __m128i va = _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(a + i)), signMask);
        __m128i vb = _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i *>(b + i)), signMask);
        __m128i gt = _mm_cmpgt_epi32(va, vb); // 掩码：a > b
        __m128i m = _mm_or_si128(_mm_and_si128(gt, va), _mm_andnot_si128(gt, vb));
        _mm_storeu_si128(reinterpret_cast<__m128i *>(b + i), _mm_xor_si128(m, signMask));
    }
#endif
#endif
    for (; i < count; ++i) {
        b[i] = a[i] > b[i] ? a[i] : b[i];
    }
}

bool VectorClock::IsNotHappensBefore(const VectorTime &t1, const VectorTime &t2, uint32_t t1Pipe, uint32_t t2Pipe)
{
    /// 两个事件在两者各自的pipe上，其中一个pipe上的向量时间大于另外一个事件、另外一个pipe上的向量时间小于另外一个事件，
    /// 说明两个事件存在并发关系
    return (t1[t1Pipe] < t2[t1Pipe] && t1[t2Pipe] > t2[t2Pipe]) || (t1[t2Pipe] < t2[t2Pipe] && t1[t1Pipe] > t2[t1Pipe]);
}

bool VectorClock::IsHappensBefore(const VectorTime &t1, const VectorTime &t2, uint32_t t1Pipe, uint32_t t2Pipe) {
    return t1[t1Pipe] <= t2[t1Pipe] && t1[t2Pipe] <= t2[t2Pipe];
}

uint32_t VectorClock::GetLogicTime(const VectorTime &t, uint32_t pipe)
{
    return t[pipe];
}

bool VectorClock::IsEqual(const VectorTime &t1, const VectorTime &t2)
{
    return (t1 == t2);
}
}
