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


#ifndef __HOOK_RUNETIME_H__
#define __HOOK_RUNETIME_H__

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RTS_API
#define RTS_API
#endif  // RTS_API

#ifndef char_t
typedef char char_t;
#endif

/**
  * @ingroup rt_kernel
  * @brief magic number of elf binary for aicore
  */
#define RT_DEV_BINARY_MAGIC_ELF 0x43554245

/**
 * @ingroup rt_kernel
 * @brief magic number of elf binary for aicpu
 */
#define RT_DEV_BINARY_MAGIC_ELF_AICPU 0x41415243

/**
 * @ingroup rt_kernel
 * @brief magic number of elf binary for aivector
 */
#define RT_DEV_BINARY_MAGIC_ELF_AIVEC 0x41415246

/**
 * @ingroup rt_kernel
 * @brief magic number of elf binary for aicube
 */
#define RT_DEV_BINARY_MAGIC_ELF_AICUBE 0x41494343U

/**
 * @ingroup dvrt_base
 * @brief runtime error numbers.
 */
typedef enum tagRtError {
    RT_ERROR_NONE = 0x0,                      // success
    RT_ERROR_INVALID_VALUE = 0x1,             // invalid value
    RT_ERROR_MEMORY_ALLOCATION = 0x2,         // memory allocation fail
    RT_ERROR_INVALID_RESOURCE_HANDLE = 0x3,   // invalid handle
    RT_ERROR_INVALID_DEVICE_POINTER = 0x4,    // invalid device point
    RT_ERROR_INVALID_MEMCPY_DIRECTION = 0x5, // invalid memory copy direction
    RT_ERROR_INVALID_DEVICE = 0x6,            // invalid device
    RT_ERROR_NO_DEVICE = 0x7,                 // no valid device
    RT_ERROR_CMD_OCCUPY_FAILURE = 0x8,        // command occpuy failure
    RT_ERROR_SET_SIGNAL_FAILURE = 0x9,        // set signal failure
    RT_ERROR_UNSET_SIGNAL_FAILURE = 0xA,      // unset signal failure
    RT_ERROR_OPEN_FILE_FAILURE = 0xB,         // unset signal failure
    RT_ERROR_WRITE_FILE_FAILURE = 0xC,
    RT_ERROR_MEMORY_ADDRESS_UNALIGNED = 0xD,
    RT_ERROR_DRV_ERR = 0xE,
    RT_ERROR_LOST_HEARTBEAT = 0xF,
    RT_ERROR_REPORT_TIMEOUT = 0x10,
    RT_ERROR_NOT_READY = 0x11,
    RT_ERROR_DATA_OPERATION_FAIL = 0x12,
    RT_ERROR_INVALID_L2_INSTR_SIZE = 0x13,
    RT_ERROR_DEVICE_PROC_HANG_OUT = 0x14,
    RT_ERROR_DEVICE_POWER_UP_FAIL = 0x15,
    RT_ERROR_DEVICE_POWER_DOWN_FAIL = 0x16,
    RT_ERROR_FEATURE_NOT_SUPPROT = 0x17,
    RT_ERROR_KERNEL_DUPLICATE = 0x18,         // register same kernel repeatly
    RT_ERROR_MODEL_STREAM_EXE_FAILED = 0x91,  // the model stream failed
    RT_ERROR_MODEL_LOAD_FAILED = 0x94,        // the model stream failed
    RT_ERROR_END_OF_SEQUENCE = 0x95,          // end of sequence
    RT_ERROR_NO_STREAM_CB_REG = 0x96,         // no callback register info for stream
    RT_ERROR_DATA_DUMP_LOAD_FAILED = 0x97,    // data dump load info fail
    RT_ERROR_CALLBACK_THREAD_UNSUBSTRIBE = 0x98,    // callback thread unsubstribe
    RT_ERROR_RESERVED
} rtError_t;

/**
 * @ingroup dvrt_mem
 * @brief memory copy type
 */
typedef enum tagRtMemcpyKind {
    RT_MEMCPY_HOST_TO_HOST = 0,  // host to host
    RT_MEMCPY_HOST_TO_DEVICE,    // host to device
    RT_MEMCPY_DEVICE_TO_HOST,    // device to host
    RT_MEMCPY_DEVICE_TO_DEVICE,  // device to device, 1P && P2P
    RT_MEMCPY_MANAGED,           // managed memory
    RT_MEMCPY_ADDR_DEVICE_TO_DEVICE,
    RT_MEMCPY_HOST_TO_DEVICE_EX, // host  to device ex (only used for 8 bytes)
    RT_MEMCPY_DEVICE_TO_HOST_EX, // device to host ex
    RT_MEMCPY_RESERVED,
} rtMemcpyKind_t;

/**
 * @ingroup dvrt_base
 * @brief stream handle.
 */
typedef void *rtStream_t;

/**
 * @ingroup dvrt_mem
 * @brief memory type | memory Policy
 */
typedef uint32_t rtMemType_t;

/**
 * @ingroup rt_kernel
 * @brief device binary type
 */
typedef struct tagRtDevBinary {
    uint32_t magic;    // magic number
    uint32_t version;  // version of binary
    const void *data;  // binary data
    uint64_t length;   // binary length
} rtDevBinary_t;

/**
 * @ingroup rt_kernel
 * @brief shared memory data control
 */
typedef struct tagRtSmData {
    uint64_t L2_mirror_addr;          // preload or swap source addr
    uint32_t L2_data_section_size;    // every data size
    uint8_t L2_preload;               // 1 - preload from mirrorAddr, 0 - no preload
    uint8_t modified;                 // 1 - data will be modified by kernel, 0 - no modified
    uint8_t priority;                 // data priority
    int8_t prev_L2_page_offset_base;  // remap source section offset
    uint8_t L2_page_offset_base;      // remap destination section offset
    uint8_t L2_load_to_ddr;           // 1 - need load out, 0 - no need
    uint8_t reserved[2];              // reserved
} rtSmData_t;

/**
 * @ingroup rt_kernel
 * @brief shared memory description
 */
typedef struct tagRtSmCtrl {
    rtSmData_t data[8];  // data description
    uint64_t size;       // max page Num
    uint8_t remap[64];   /* just using for static remap mode, default:0xFF
                            array index: virtual l2 page id, array value: physic l2 page id */
    uint8_t l2_in_main;  // 0-DDR, 1-L2, default:0xFF
    uint8_t reserved[3];
} rtSmDesc_t;

typedef struct rtArgsSizeInfo {
    void *infoAddr; /* info : atomicIndex|input num input offset|size|size */
    uint32_t atomicIndex;
} rtArgsSizeInfo_t;

/**
 * @ingroup rt_kernel
 * @brief host memory input struct
 */
typedef struct rtHostInputInfo {
    uint32_t addrOffset;
    uint32_t dataOffset;
} rtHostInputInfo_t;

/**
 * @ingroup rt_kernel
 * @brief args struct
 */
typedef struct tagRtArgsEx {
    void *args;                     // args host mem addr
    rtHostInputInfo_t *hostInputInfoPtr;     // nullptr means no host mem input
    uint32_t argsSize;              // input + output + tiling addr size + tiling data size + host mem
    uint32_t tilingAddrOffset;      // tiling addr offset
    uint32_t tilingDataOffset;      // tiling data offset
    uint16_t hostInputInfoNum;      // hostInputInfo num
    uint8_t hasTiling;              // if has tiling: 0 means no tiling
    uint8_t isNoNeedH2DCopy;        // is no need host to device copy: 0 means need H2D copy,
                                    // others means doesn't need H2D copy.
    uint8_t reserved[4];
} rtArgsEx_t;

typedef struct tagRtTaskCfgInfo {
    uint8_t qos;
    uint8_t partId;
    uint8_t schemMode; // rtschemModeType_t 0:normal;1:batch;2:sync
    bool d2dCrossFlag;
    uint32_t blockDimOffset;
    uint8_t dumpflag;
    uint8_t neverTimeout;
    uint8_t res[2]; // res
    uint32_t localMemorySize;
} rtTaskCfgInfo_t;

typedef struct DrvMemHandle {
    int32_t id;
    uint32_t side;
    uint32_t devid;
    uint64_t pg_num;
} rtDrvMemHandle_t;

/**
 * @ingroup dvrt_mem
 * @brief alloc device memory
 * @param [in|out] devPtr   memory pointer
 * @param [in] size   memory size
 * @param [in] type   memory type
 * @param [in] moduleid alloc memory module id
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtMalloc(void **devPtr, uint64_t size, rtMemType_t type, const uint16_t moduleId);

/**
 * @ingroup dvrt_mem
 * @brief free device memory
 * @param [in|out] devPtr   memory pointer
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtFree(void *devPtr);

/**
 * @ingroup dvrt_mem
 * @brief set memory with uint32_t value
 * @param [in] devPtr
 * @param [in] Max length of destination address memory
 * @param [in] val
 * @param [in] cnt byte num
 * @return RT_ERROR_NONE for ok, errno for failed
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtMemset(void *devPtr, uint64_t destMax, uint32_t val, uint64_t cnt);

/**
 * @ingroup dvrt_mem
 * @brief synchronized memcpy
 * @param [in] dst   destination address pointer
 * @param [in] Max length of destination address memory
 * @param [in] src   source address pointer
 * @param [in] cnt   the number of byte to copy
 * @param [in] kind   memcpy type
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtMemcpy(void *dst, uint64_t destMax, const void *src, uint64_t cnt, rtMemcpyKind_t kind);

/**
 * @ingroup dvrt_mem
 * @brief This command is used to map an allocation handle to a reserved virtual address range.
 * @attention Only support ONLINE scene.
 * @param [in] devPtr Address where memory will be mapped.
 * @param [in] size Size of the memory mapping.
 * @param [in] offset Currently unused, must be zero.
 * @param [in] handle Value of handle which was returned previously by halMemCreate.
 * @param [in] flag Currently unused, must be zero.
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 * @return RT_ERROR_DRV_ERR for driver error
 */
RTS_API rtError_t rtMapMem(void* devPtr, size_t size, size_t offset, rtDrvMemHandle_t* handle, uint64_t flags);

/**
 * @ingroup dvrt_mem
 * @brief This command is used to unmap the backing memory of a given address range.
 * @attention Only support ONLINE scene.
 * @param [in] devPtr Starting address for the virtual address range to unmap.
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 * @return RT_ERROR_DRV_ERR for driver error
 */
RTS_API rtError_t rtUnmapMem(void* devPtr);

/**
 * @ingroup rt_kernel
 * @brief register device binary
 * @param [in] bin   device binary description
 * @param [out] hdl   device binary handle
 * @return RT_ERROR_NONE for ok
 * @note:if this interface is changed, pls notify the compiler changing at the same time.
 */
RTS_API rtError_t rtDevBinaryRegister(const rtDevBinary_t *bin, void **hdl);

/**
 * @ingroup rt_kernel
 * @brief register device binary with all kernel
 * @param [in] bin   device binary description
 * @param [out] hdl   device binary handle
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtRegisterAllKernel(const rtDevBinary_t *bin, void **hdl);

/**
 * @ingroup rt_kernel
 * @brief unregister device binary
 * @param [in] hdl   device binary handle
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtDevBinaryUnRegister(void *hdl);

/**
 * @ingroup rt_kernel
 * @brief register device function
 * @param [in] binHandle   device binary handle
 * @param [in] stubFunc   stub function
 * @param [in] stubName   stub function name
 * @param [in] kernelInfoExt   kernel Info extension. device function description or tiling key,
 *                             depending static shape or dynmaic shape.
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtFunctionRegister(void *binHandle, const void *stubFunc, const char_t *stubName,
                                     const void *kernelInfoExt, uint32_t funcMode);

/**
 * @ingroup rt_kernel
 * @brief launch kernel to device
 * @param [in] stubFunc   stub function
 * @param [in] blockDim   block dimentions
 * @param [in] args   argments address for kernel function
 * @param [in] argsSize   argements size
 * @param [in] smDesc   shared memory description
 * @param [in] stm   associated stream
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtKernelLaunch(const void *stubFunc, uint32_t blockDim, void *args,
                                 uint32_t argsSize, rtSmDesc_t *smDesc, rtStream_t stm);

/**
 * @ingroup rt_kernel
 * @brief launch kernel with handle to device
 * @param [in] hdl             program
 * @param [in] tilingKey       tilingKey
 * @param [in] blockDim        block dimentions
 * @param [in] argsInfo        argments address for kernel function
 * @param [in] smDesc          shared memory description
 * @param [in] stm             associated stream
 * @param [in] cfgInfo      task config
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtKernelLaunchWithHandleV2(void *hdl, const uint64_t tilingKey, uint32_t blockDim,
                                             rtArgsEx_t *argsInfo, rtSmDesc_t *smDesc, rtStream_t stm,
                                             const rtTaskCfgInfo_t *cfgInfo);

/**
 * @ingroup rtKernelLaunchWithFlag
 * @brief launch kernel to device
 * @param [in] stubFunc   stub function
 * @param [in] blockDim   block dimentions
 * @param [in] argsInfo   argments address for kernel function
 * @param [in] smDesc     shared memory description
 * @param [in] stm        associated stream
 * @param [in] flags      dump flag
 * @param [in] cfgInfo      task config info
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtKernelLaunchWithFlagV2(const void *stubFunc, uint32_t blockDim, rtArgsEx_t *argsInfo,
                                           rtSmDesc_t *smDesc, rtStream_t stm, uint32_t flags,
                                           const rtTaskCfgInfo_t *cfgInfo);

/**
* @ingroup rt_kernel
* @brief set input argments size for exception
* @param [in] sizeInfo argments size info
* @return RT_ERROR_NONE for ok
* @return RT_ERROR_INVALID_VALUE for error input
*/
RTS_API rtError_t rtSetExceptionExtInfo(const rtArgsSizeInfo_t * const sizeInfo);

/**
 * @ingroup dvrt_dev
 * @brief set target device for current thread
 * @param [int] devId   the device id
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtSetDevice(int32_t devId);

/**
 * @ingroup dvrt_dev
 * @brief get chipType
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtGetSocVersion(char_t *ver, const uint32_t maxLen);

/**
 * @ingroup dvrt_stream
 * @brief wait stream to be complete
 * @param [in] stm   stream to wait
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtStreamSynchronize(rtStream_t stm);

/**
 * @ingroup rt_kernel
 * @brief get kernel address and prefetchCnt
 * @param [in] hdl           program for dynamic shape
 * @param [in] tilingKey     tilingKey for dynamic shape
 * @param [in] stubFunc      stubFunc for static shape
 * @param [in] flag          flag for distinguishing between dynamic shape and static shape
 * @param [out] addr         address of kernel function
 * @param [out] prefetchCnt  prefetchCnt of kernel function
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtKernelGetAddrAndPrefCnt(void *hdl, const uint64_t tilingKey, const void * const stubFunc,
                                            const uint32_t flag, void **addr, uint32_t *prefetchCnt);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __HOOK_RUNETIME_H__
