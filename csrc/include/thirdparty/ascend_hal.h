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


#ifndef __HOOK_ASCEND_HAL_H__
#define __HOOK_ASCEND_HAL_H__

#include <cstddef>
#include <cstdint>

#define DV_MEM_RESV 8

typedef enum tagDrvError {
    DRV_ERROR_NONE = 0,                /**< success */
    DRV_ERROR_NO_DEVICE = 1,           /**< no valid device */
    DRV_ERROR_INVALID_DEVICE = 2,      /**< invalid device */
    DRV_ERROR_INVALID_VALUE = 3,       /**< invalid value */
    DRV_ERROR_INVALID_HANDLE = 4,      /**< invalid handle */
    DRV_ERROR_INVALID_MALLOC_TYPE = 5, /**< invalid malloc type */
    DRV_ERROR_OUT_OF_MEMORY = 6,       /**< out of memory */
    DRV_ERROR_INNER_ERR = 7,           /**< driver inside error */
    DRV_ERROR_PARA_ERROR = 8,          /**< driver wrong parameter */
    DRV_ERROR_UNINIT = 9,              /**< driver uninit */
    DRV_ERROR_REPEATED_INIT = 10,          /**< driver repeated init */
    DRV_ERROR_NOT_EXIST = 11,        /**< there is resource*/
    DRV_ERROR_REPEATED_USERD = 12,
    DRV_ERROR_BUSY = 13,                /**< task already running */
    DRV_ERROR_NO_RESOURCES = 14,        /**< driver short of resouces */
    DRV_ERROR_OUT_OF_CMD_SLOT = 15,
    DRV_ERROR_WAIT_TIMEOUT = 16,       /**< driver wait timeout*/
    DRV_ERROR_IOCRL_FAIL = 17,         /**< driver ioctl fail*/

    DRV_ERROR_SOCKET_CREATE = 18,      /**< driver create socket error*/
    DRV_ERROR_SOCKET_CONNECT = 19,     /**< driver connect socket error*/
    DRV_ERROR_SOCKET_BIND = 20,        /**< driver bind socket error*/
    DRV_ERROR_SOCKET_LISTEN = 21,      /**< driver listen socket error*/
    DRV_ERROR_SOCKET_ACCEPT = 22,      /**< driver accept socket error*/
    DRV_ERROR_CLIENT_BUSY = 23,        /**< driver client busy error*/
    DRV_ERROR_SOCKET_SET = 24,         /**< driver socket set error*/
    DRV_ERROR_SOCKET_CLOSE = 25,       /**< driver socket close error*/
    DRV_ERROR_RECV_MESG = 26,          /**< driver recv message error*/
    DRV_ERROR_SEND_MESG = 27,          /**< driver send message error*/
    DRV_ERROR_SERVER_BUSY = 28,
    DRV_ERROR_CONFIG_READ_FAIL = 29,
    DRV_ERROR_STATUS_FAIL = 30,
    DRV_ERROR_SERVER_CREATE_FAIL = 31,
    DRV_ERROR_WAIT_INTERRUPT = 32,
    DRV_ERROR_BUS_DOWN = 33,
    DRV_ERROR_DEVICE_NOT_READY = 34,
    DRV_ERROR_REMOTE_NOT_LISTEN = 35,
    DRV_ERROR_NON_BLOCK = 36,

    DRV_ERROR_OVER_LIMIT = 37,
    DRV_ERROR_FILE_OPS = 38,
    DRV_ERROR_MBIND_FAIL = 39,
    DRV_ERROR_MALLOC_FAIL = 40,

    DRV_ERROR_REPEATED_SUBSCRIBED = 41,
    DRV_ERROR_PROCESS_EXIT = 42,
    DRV_ERROR_DEV_PROCESS_HANG = 43,

    DRV_ERROR_REMOTE_NO_SESSION = 44,

    DRV_ERROR_HOT_RESET_IN_PROGRESS = 45,

    DRV_ERROR_OPER_NOT_PERMITTED = 46,

    DRV_ERROR_NO_EVENT_RESOURCES = 47,
    DRV_ERROR_NO_STREAM_RESOURCES = 48,
    DRV_ERROR_NO_NOTIFY_RESOURCES = 49,
    DRV_ERROR_NO_MODEL_RESOURCES = 50,
    DRV_ERROR_TRY_AGAIN = 51,

    DRV_ERROR_DST_PATH_ILLEGAL = 52,                    /**< send file dst path illegal*/
    DRV_ERROR_OPEN_FAILED = 53,                         /**< send file open failed */
    DRV_ERROR_NO_FREE_SPACE = 54,                       /**< send file no free space */
    DRV_ERROR_LOCAL_ABNORMAL_FILE = 55,                 /**< send file local file abnormal*/
    DRV_ERROR_DST_PERMISSION_DENIED = 56,               /**< send file dst Permission denied*/
    DRV_ERROR_DST_NO_SUCH_FILE = 57,                    /**< pull file no such file or directory*/

    DRV_ERROR_MEMORY_OPT_FAIL = 58,
    DRV_ERROR_RUNTIME_ON_OTHER_PLAT = 59,
    DRV_ERROR_SQID_FULL = 60,                           /**< driver SQ   is full */

    DRV_ERROR_SERVER_HAS_BEEN_CREATED = 61,
    DRV_ERROR_NO_PROCESS = 62,
    DRV_ERROR_NO_SUBSCRIBE_THREAD = 63,
    DRV_ERROR_NON_SCHED_GRP_MUL_THREAD = 64,
    DRV_ERROR_NO_GROUP = 65,
    DRV_ERROR_GROUP_EXIST = 66,
    DRV_ERROR_THREAD_EXCEEDS_SPEC = 67,
    DRV_ERROR_THREAD_NOT_RUNNIG = 68,
    DRV_ERROR_PROCESS_NOT_MATCH = 69,
    DRV_ERROR_EVENT_NOT_MATCH = 70,
    DRV_ERROR_PROCESS_REPEAT_ADD = 71,
    DRV_ERROR_GROUP_NON_SCHED = 72,
    DRV_ERROR_NO_EVENT = 73,
    DRV_ERROR_COPY_USER_FAIL = 74,
    DRV_ERROR_QUEUE_EMPTY = 75,
    DRV_ERROR_QUEUE_FULL = 76,
    DRV_ERROR_RUN_IN_ILLEGAL_CPU = 77,
    DRV_ERROR_SUBSCRIBE_THREAD_TIMEOUT = 78,
    DRV_ERROR_BAD_ADDRESS = 79,
    DRV_ERROR_DST_FILE_IS_BEING_WRITTEN = 80,           /**< send file The dts file is being written */
    DRV_ERROR_EPOLL_CLOSE = 81,                         /**< epoll close */
    DRV_ERROR_CDQ_ABNORMAL = 82,
    DRV_ERROR_CDQ_NOT_EXIST = 83,
    DRV_ERROR_NO_CDQ_RESOURCES = 84,
    DRV_ERROR_CDQ_QUIT = 85,
    DRV_ERROR_PARTITION_NOT_RIGHT = 86,
    DRV_ERROR_RESOURCE_OCCUPIED = 87,
    DRV_ERROR_PERMISSION = 88,
    DRV_ERROR_RESUME = 89,
    DEV_ERROR_UNAUTHORIZED_ACCESS_DEVICE = 90,
    DEV_ERROR_BIST_HW_ERR = 91,
    DEV_ERROR_BIST_SW_ERR = 92,

    DRV_ERROR_NOT_SUPPORT = 0xfffe,
    DRV_ERROR_RESERVED,
} drvError_t;

typedef enum tagMemType {
    DV_MEM_SVM = 0x0001,
    DV_MEM_SVM_HOST = 0x0002,
    DV_MEM_SVM_DEVICE = 0x0004,
    DV_MEM_LOCK_HOST = 0x0008,
    DV_MEM_LOCK_DEV = 0x0010,
    DV_MEM_LOCK_DEV_DVPP = 0x0020,
} DV_MEM_TYPE;

typedef uint64_t DVdeviceptr;

struct DVattribute {
    /**< DV_MEM_SVM_DEVICE : svm memory & mapped device */
    /**< DV_MEM_SVM_HOST : svm memory & mapped host */
    /**< DV_MEM_SVM : svm memory & no mapped */
    /**< DV_MEM_LOCK_HOST :    host mapped memory & lock host */
    /**< DV_MEM_LOCK_DEV : dev mapped memory & lock dev */
    /**< DV_MEM_LOCK_DEV_DVPP : dev_dvpp mapped memory & lock dev */
    uint32_t memType;
    uint32_t resv1;
    uint32_t resv2;

    uint32_t devId;
    uint32_t pageSize;
    uint32_t resv[DV_MEM_RESV];
};

typedef enum tagMemcpy2dType {
    DEVMM_MEMCPY2D_SYNC = 0,
    DEVMM_MEMCPY2D_ASYNC_CONVERT,
    DEVMM_MEMCPY2D_ASYNC_DESTROY,
} DEVMM_MEMCPY2D_TYPE;

struct drvMem2D {
    unsigned long long *dst;        /**< destination memory address */
    unsigned long long dpitch;      /**< pitch of destination memory */
    unsigned long long *src;        /**< source memory address */
    unsigned long long spitch;      /**< pitch of source memory */
    unsigned long long width;       /**< width of matrix transfer */
    unsigned long long height;      /**< height of matrix transfer */
    unsigned long long fixed_size;  /**< Input: already converted size. if fixed_size < width*height,
                                         need to call halMemcpy2D multi times */
    unsigned int direction;         /**< copy direction */
    unsigned int resv1;
    unsigned long long resv2;
};

struct DMA_OFFSET_ADDR {
    unsigned long long offset;
    unsigned int devid;
};

struct DMA_PHY_ADDR {
    void *src;           /**< src addr(physical addr) */
    void *dst;           /**< dst addr(physical addr) */
    unsigned int len;    /**< length */
    unsigned char flag;  /**< Flag=0 Non-chain, SRC and DST are physical addresses,
                              can be directly DMA copy operations */
                         /**< Flag=1 chain, SRC is the address of the dma list and can be used for
                              direct dma copy operations */
    void *priv;
};

struct DMA_ADDR {
    union {
        struct DMA_PHY_ADDR phyAddr;
        struct DMA_OFFSET_ADDR offsetAddr;
    };
    unsigned int fixed_size; /**< Output: the actual conversion size */
    unsigned int virt_id;    /**< store logic id for destroy addr */
};

struct drvMem2DAsync {
    struct drvMem2D copy2dInfo;
    struct DMA_ADDR *dmaAddr;
};

struct MEMCPY2D {
    unsigned int type;  /**< DEVMM_MEMCPY2D_SYNC: memcpy2d sync */
                        /**< DEVMM_MEMCPY2D_ASYNC_CONVERT: memcpy2d async convert */
                        /**< DEVMM_MEMCPY2D_ASYNC_DESTROY: memcpy2d async destroy */
    unsigned int resv;
    union {
        struct drvMem2D copy2d;
        struct drvMem2DAsync copy2dAsync;
    };
};

/// HAL interfaces
extern "C" {
drvError_t drvMemGetAttribute(DVdeviceptr vptr, struct DVattribute *attr);
}  // extern "C"

#endif  // __HOOK_ASCEND_HAL_H__
