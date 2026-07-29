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


#include "plugin/record_calc_instructions.h"

using namespace Sanitizer;
#if !(defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1 && defined(__DAV_C220_CUBE__))
// #2970: dav-m200|dav-m200-vec|dav-c220-vec|dav-c220
SANITIZER_REPORT(vgather, __ubuf__ uint16_t *dst, __ubuf__ uint32_t *src, uint64_t config)
{
    RecordVgatherOpConfFunc(EXTRA_PARAMS, dst, src, config);
}

// #2972: dav-c220-vec|dav-c220
SANITIZER_REPORT(vgather, __ubuf__ uint16_t *dst, __ubuf__ uint32_t *src, uint32_t offsetAddr,
    uint16_t dstRepeatStride, uint8_t repeat)
{
    RecordVgatherOpFunc<InstrName::VGATHER>(EXTRA_PARAMS, dst, src, offsetAddr, dstRepeatStride, 1U, repeat, 128U, true);
}

// #2973: dav-m200|dav-m200-vec|dav-c220-vec|dav-c220
SANITIZER_REPORT(vgather, __ubuf__ uint32_t *dst, __ubuf__ uint32_t *src, uint64_t config)
{
    RecordVgatherOpConfFunc(EXTRA_PARAMS, dst, src, config);
}

// #2975: dav-c220-vec|dav-c220
SANITIZER_REPORT(vgather, __ubuf__ uint32_t *dst, __ubuf__ uint32_t *src, uint32_t offsetAddr,
    uint16_t dstRepeatStride, uint8_t repeat)
{
    RecordVgatherOpFunc<InstrName::VGATHER>(EXTRA_PARAMS, dst, src, offsetAddr, dstRepeatStride, 1U, repeat, 64U, true);
}

// #2971: dav-m200|dav-m200-vec
SANITIZER_REPORT(vgather, __ubuf__ uint16_t *dst, __ubuf__ uint32_t *src, uint32_t offsetAddr, bool repeatStrideMode)
{
    RecordVgatherOpFunc<InstrName::VGATHER>(EXTRA_PARAMS, dst, src, offsetAddr, 1U, 1U, 1U, 128U, true);
}

// #2974: dav-m200|dav-m200-vec
SANITIZER_REPORT(vgather, __ubuf__ uint32_t *dst, __ubuf__ uint32_t *src, uint32_t offsetAddr, bool repeatStrideMode)
{
    RecordVgatherOpFunc<InstrName::VGATHER>(EXTRA_PARAMS, dst, src, offsetAddr, 1U, 1U, 1U, 64U, true);
}

#endif
