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


#ifndef __HOOKS_HAL_HOOKS_H__
#define __HOOKS_HAL_HOOKS_H__

#include "ascend_hal.h"

/// Interfaces to be hooked use C ABI
extern "C" {
typedef drvError_t (*HalMemAlloc)(void **pp, unsigned long long size, unsigned long long flag);
typedef drvError_t (*HalMemFree)(void *pp);
typedef drvError_t (*DrvMemsetD8)(DVdeviceptr dst, size_t destMax, uint8_t value, size_t num);
typedef drvError_t (*DrvMemcpy)(DVdeviceptr dst, size_t destMax, DVdeviceptr src, size_t byteCount);
typedef drvError_t (*HalMemCpyAsync)(
    DVdeviceptr dst, size_t destMax, DVdeviceptr src, size_t byteCount, uint64_t *copyFd);
typedef drvError_t (*HalMemcpy2D)(MEMCPY2D *pCopy);

drvError_t halMemAlloc(void **pp, unsigned long long size, unsigned long long flag);
drvError_t halMemFree(void *pp);
drvError_t drvMemsetD8(DVdeviceptr dst, size_t destMax, uint8_t value, size_t num);
drvError_t drvMemcpy(DVdeviceptr dst, size_t destMax, DVdeviceptr src, size_t byteCount);
drvError_t halMemCpyAsync(DVdeviceptr dst, size_t destMax, DVdeviceptr src,
                          size_t byteCount, uint64_t *copyFd);
drvError_t halMemcpy2D(MEMCPY2D *pCopy);
}  // extern "C"

#endif  // __HOOKS_HAL_HOOKS_H__
