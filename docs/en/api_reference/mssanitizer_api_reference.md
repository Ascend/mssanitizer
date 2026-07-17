# MindStudio Sanitizer API Reference

<br>

## APIs

**API Overview**

The msSanitizer tool contains two types of APIs: sanitizer APIs and mstx APIs. The sanitizer APIs are used to check the CANN software stack and correspond to the ACL APIs. These APIs additionally report the code file and line number of the API called to the tool. The header file of the sanitizer APIs needs to be imported and the dynamic library needs to be linked. For details, see "Checking the Memory of the CANN Software Stack" > "Importing the API Header File and Linking the Dynamic Library" in [MindStudio Sanitizer Typical Cases](../best_practices/basic_cases.md). The mstx APIs are used to report custom memory pool information for more accurate check. For details, see [MSTX Functions](#mstx-functions).

**Table 1** msSanitizer APIs

|API|Description|
|--|--|
|[sanitizer APIs](#sanitizer-apis)|Functions as the matching ACL APIs and reports the code file and line number of the sanitizer API calls to the msSanitizer tool.|
|[sanitizerRtMalloc](#sanitizerrtmalloc)|Calls `aclrtMalloc` to allocate `size` bytes linear memory on the device, returns the pointer to the allocated memory by using `*devPtr`, and reports the memory allocation information to the check tool. The actual memory allocation behavior and parameter meanings are the same as those of `aclrtMalloc`.|
|[sanitizerRtMallocCached](#sanitizerrtmalloccached)|Calls `aclrtMallocCached` to allocate `size` bytes linear memory on the device, returns the pointer to the allocated memory by using `*devPtr`, and reports the memory allocation information to the check tool. In any scenario, the allocated memory supports cache. The actual memory allocation behavior and parameter meanings are the same as those of `aclrtMallocCached`.|
|[sanitizerRtFree](#sanitizerrtfree)|Calls `aclrtFree` to release the memory on the device and reports the memory release information to the check tool. The actual memory release behavior and parameter meanings are the same as those of `aclrtFree`.|
|[sanitizerRtMemset](#sanitizerrtmemset)|Calls `aclrtMemset` to initialize the memory, sets the content in the memory to a specified value, and reports the memory initialization information to the check tool. The actual memory initialization behavior and parameter meanings are the same as those of `aclrtMemset`.|
|[sanitizerRtMemsetAsync](#sanitizerrtmemsetasync)|Calls `aclrtMemsetAsync` to initialize the memory, sets the content in the memory to a specified value, and reports the memory initialization information to the check tool. This API is asynchronous. The actual memory initialization behavior and parameter meanings are the same as those of `aclrtMemsetAsync`.|
|[sanitizerRtMemcpy](#sanitizerrtmemcpy)|Calls `aclrtMemcpy` to copy the memory and reports the memory copy information to the check tool. The actual memory copy behavior and parameter meanings are the same as those of `aclrtMemcpy`.|
|[sanitizerRtMemcpyAsync](#sanitizerrtmemcpyasync)|Calls `aclrtMemcpyAsync` to copy the memory and reports the memory copy information to the check tool. This API is asynchronous. The actual memory copy behavior and parameter meanings are the same as those of `aclrtMemcpyAsync`.|
|[sanitizerRtMemcpy2d](#sanitizerrtmemcpy2d)|Calls `aclrtMemcpy2d` to copy the matrix data memory and reports the memory copy information to the check tool. The actual memory copy behavior and parameter meanings are the same as those of `aclrtMemcpy2d`.|
|[sanitizerRtMemcpy2dAsync](#sanitizerrtmemcpy2dasync)|Calls `aclrtMemcpy2dAsync` to copy the matrix data memory and reports the memory copy information to the check tool. This API is asynchronous. The actual memory copy behavior and parameter meanings are the same as those of `aclrtMemcpy2dAsync`.|
|[sanitizerReportMalloc](#sanitizerreportmalloc)|Manually reports the GM allocation information.|
|[sanitizerReportFree](#sanitizerreportfree)|Manually reports the GM release information.|
|[MSTX Functions](#mstx-functions)|The mstx APIs are a set of extension APIs provided by MindStudio to allow you to insert specific tags in your application so that the memory issues of operators can be more accurately identified.|
|mstxDomainCreateA|Creates a domain.|
|mstxMemHeapRegister|Registers a memory pool.|
|mstxMemHeapUnregister|Deregisters a memory pool.|
|mstxMemRegionsRegister|Registers the secondary memory pool allocation.|
|mstxMemRegionsUnregister|Deregisters the secondary memory pool allocation.|

## Sanitizer APIs

<br>

### sanitizerRtMalloc

---

**Function**

Calls `aclrtMalloc` to allocate `size` bytes linear memory on the device, returns the pointer to the allocated memory by using `*devPtr`, and reports the memory allocation information to the check tool. The actual memory allocation behavior and parameter meanings are the same as those of `aclrtMalloc`.

>[!NOTE]NOTE
>For details about `aclrtMalloc`, see "ACL API Reference (C)"> "Runtime Management" > "Memory Management" in the [Application Development APIs](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html).

**Prototype**

```c
aclError sanitizerRtMalloc(void **devPtr, size_t size, aclrtMemMallocPolicy policy, char const *filename, int lineno);
```

**Parameters**

**Table 1** Parameter description

|Parameter|Input/Output|Description|
|--|--|--|
|devPtr|Output|Pointer to the "pointer to the allocated device memory".|
|size|Input|Allocated memory size, in bytes. Must not be `0`.|
|policy|Input|Memory allocation policy.|
|filename|Input|Name of the file where memory allocation is called.|
|lineno|Input|Number of the line where memory allocation is called.|

**Returns**

`0` on success; else, failure.

**Example**

For details, see step 4 in "Checking the Memory of the CANN Software Stack" > "Troubleshooting Procedure" in [MindStudio Sanitizer Best Practices](../best_practices/basic_cases.md).

<br>
<br>

### sanitizerRtMallocCached

---

**Function**

Calls `aclrtMallocCached` to allocate `size` bytes linear memory on the device, returns the pointer to the allocated memory by using `*devPtr`, and reports the memory allocation information to the check tool. In any scenario, the allocated memory supports cache. The actual memory allocation behavior and parameter meanings are the same as those of `aclrtMallocCached`.

> [!NOTE]NOTE       
> For details about `aclrtMallocCached`, see "ACL API Reference (C)" > "Runtime Management" > "Memory Management" in the [Application Development APIs](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html).

**Prototype**

```c
aclError sanitizerRtMallocCached(void **devPtr, size_t size, aclrtMemMallocPolicy policy, char const *filename, int lineno);
```

**Parameters**

**Table 1** Parameter description

|Parameter|Input/Output|Description|
|--|--|--|
|devPtr|Output|Pointer to the "pointer to the allocated device memory".|
|size|Input|Allocated memory size, in bytes. Must not be `0`.|
|policy|Input|Memory allocation policy.|
|filename|Input|Name of the file where memory allocation is called.|
|lineno|Input|Number of the line where memory allocation is called.|

**Returns**

`0` on success; else, failure.

**Example**

For details, see step 4 in "Checking the Memory of the CANN Software Stack" > "Troubleshooting Procedure" in [MindStudio Sanitizer Best Practices](../best_practices/basic_cases.md).

<br>
<br>

### sanitizerRtFree

---

**Function**

Calls `aclrtFree` to release the memory on the device and reports the memory release information to the check tool. The actual memory release behavior and parameter meanings are the same as those of `aclrtFree`.

>[!NOTE]NOTE
>For details about `aclrtFree`, see "ACL API Reference (C)" > "Runtime Management" > "Memory Management" in the [Application Development APIs](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html).

**Prototype**

```c
aclError sanitizerRtFree(void *devPtr, char const *filename, int lineno);
```

**Parameters**

**Table 1** Parameter description

|Parameter|Input/Output|Description|
|--|--|--|
|devPtr|Input|Pointer to memory to be released.|
|filename|Input|Name of the file where memory release is called.|
|lineno|Input|Number of the line where memory release is called.|

**Returns**

`0` on success; else, failure.

**Example**

For details, see step 4 in "Checking the Memory of the CANN Software Stack" > "Troubleshooting Procedure" in [MindStudio Sanitizer Best Practices](../best_practices/basic_cases.md).

<br>
<br>

### sanitizerRtMemset

---

**Function**

Calls `aclrtMemset` to initialize the memory, sets the content in the memory to a specified value, and reports the memory initialization information to the check tool. The actual memory initialization behavior and parameter meanings are the same as those of `aclrtMemset`.

>[!NOTE]NOTE
>For details about `aclrtMemset`, see "ACL API Reference (C)" > "Runtime Management" > "Memory Management" in the [Application Development APIs](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html).

**Prototype**

```c
aclError sanitizerRtMemset(void *devPtr, size_t maxCount, int32_t value, size_t count, char const *filename, int lineno);
```

**Parameters**

**Table 1** Parameter description

|Parameter|Input/Output|Description|
|--|--|--|
|devPtr|Input|Pointer to the start address of the memory.|
|maxCount|Input|Maximum memory size, in bytes.|
|value|Input|Specified value of the initialized memory.|
|count|Input|Memory size to set, in bytes.|
|filename|Input|Name of the file where memory initialization is called.|
|lineno|Input|Number of the line where memory initialization is called.|

**Returns**

`0` on success; else, failure.

**Example**

For details, see step 4 in "Checking the Memory of the CANN Software Stack" > "Troubleshooting Procedure" in [MindStudio Sanitizer Best Practices](../best_practices/basic_cases.md).

<br>
<br>

### sanitizerRtMemsetAsync

---

**Function**

Calls `aclrtMemsetAsync` to initialize the memory, sets the content in the memory to a specified value, and reports the memory initialization information to the check tool. This API is asynchronous. The actual memory initialization behavior and parameter meanings are the same as those of `aclrtMemsetAsync`.

>[!NOTE]NOTE
>For details about `aclrtMemsetAsync`, see "ACL API Reference (C)" > "Runtime Management" > "Memory Management" in the [Application Development APIs](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html).

**Prototype**

```c
aclError sanitizerRtMemsetAsync(void *devPtr, size_t maxCount, int32_t value, size_t count, aclrtStream stream, char const *filename, int lineno);
```

**Parameters**

**Table 1** Parameter description

|Parameter|Input/Output|Description|
|--|--|--|
|devPtr|Input|Pointer to the start address of the memory.|
|maxCount|Input|Maximum memory size, in bytes.|
|value|Input|Specified value of the initialized memory.|
|count|Input|Size of the initialized memory, in bytes.|
|stream|Input|Specified stream.|
|filename|Input|Name of the file where memory initialization is called.|
|lineno|Input|Number of the line where memory initialization is called.|

**Returns**

`0` on success; else, failure.

**Example**

For details, see step 4 in "Checking the Memory of the CANN Software Stack" > "Troubleshooting Procedure" in [MindStudio Sanitizer Best Practices](../best_practices/basic_cases.md).

<br>
<br>

### sanitizerRtMemcpy

---

**Function**

Calls `aclrtMemcpy` to copy the memory and reports the memory copy information to the check tool. The actual memory copy behavior and parameter meanings are the same as those of `aclrtMemcpy`.

>[!NOTE]NOTE
>For details about `aclrtMemcpy`, see "ACL API Reference (C)"> "Runtime Management" > "Memory Management" in the [Application Development APIs](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html).

**Prototype**

```c
aclError sanitizerRtMemcpy(void *dst, size_t destMax, const void *src, size_t count, aclrtMemcpyKind kind, char const *filename, int lineno);
```

**Parameters**

**Table 1** Parameter description

|Parameter|Input/Output|Description|
|--|--|--|
|dst|Input|Pointer to the destination memory address.|
|destMax|Input|Maximum memory size in bytes in the destination address.|
|src|Input|Pointer to the source memory address.|
|count|Input|Size in bytes to copy.|
|kind|Input|(Reserved) The system determines whether to copy data from the source address to the destination address based on the pointers to the source and destination memory addresses. If not, an error is reported.|
|filename|Input|Name of the file where memory copy is called.|
|lineno|Input|Number of the line where memory copy is called.|

**Returns**

`0` on success; else, failure.

**Example**

For details, see step 4 in "Checking the Memory of the CANN Software Stack" > "Troubleshooting Procedure" in [MindStudio Sanitizer Best Practices](../best_practices/basic_cases.md).

<br>
<br>

### sanitizerRtMemcpyAsync

---

**Function**

Calls `aclrtMemcpyAsync` to copy the memory and reports the memory copy information to the check tool. This API is asynchronous. The actual memory copy behavior and parameter meanings are the same as those of `aclrtMemcpyAsync`.

>[!NOTE]NOTE
>For details about `aclrtMemcpyAsync`, see "ACL API Reference (C)" > "Runtime Management" > "Memory Management" in the [Application Development APIs](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html).

**Prototype**

```c
aclError sanitizerRtMemcpyAsync(void *dst, size_t destMax, const void *src, size_t count, aclrtMemcpyKind kind, aclrtStream stream, char const *filename, int lineno);
```

**Parameters**

**Table 1** Parameter description

|Parameter|Input/Output|Description|
|--|--|--|
|dst|Input|Pointer to the destination memory address.|
|destMax|Input|Maximum memory size in bytes in the destination address.|
|src|Input|Pointer to the source memory address.|
|count|Input|Size in bytes to copy.|
|kind|Input|(Reserved) The system determines whether to copy data from the source address to the destination address based on the pointers to the source and destination memory addresses. If not, an error is reported.|
|stream|Input|Stream specified by the current memory copy behavior.|
|filename|Input|Name of the file where memory copy is called.|
|lineno|Input|Number of the line where memory copy is called.|

**Returns**

`0` on success; else, failure.

**Example**

For details, see step 4 in "Checking the Memory of the CANN Software Stack" > "Troubleshooting Procedure" in [MindStudio Sanitizer Best Practices](../best_practices/basic_cases.md).

<br>
<br>

### sanitizerRtMemcpy2d

---

**Function**

Calls `aclrtMemcpy2d` to copy the matrix data memory and reports the memory copy information to the check tool. The actual memory copy behavior and parameter meanings are the same as those of `aclrtMemcpy2d`.

>[!NOTE]NOTE
>For details about `aclrtMemcpy2d`, see "ACL API Reference (C)" > "Runtime Management" > "Memory Management" in the [Application Development APIs](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html).

**Prototype**

```c
aclError sanitizerRtMemcpy2d(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width, size_t height, aclrtMemcpyKind kind, char const *filename, int lineno);
```

**Parameters**

**Table 1** Parameter description

|Parameter|Input/Output|Description|
|--|--|--|
|dst|Input|Pointer to the destination memory address.|
|dpitch|Input|Address distance between two adjacent columns of vectors in the destination memory.|
|src|Input|Pointer to the source memory address.|
|spitch|Input|Address distance between two adjacent columns of vectors in the source memory.|
|width|Input|Matrix width to be copied.|
|height|Input|Matrix height to be copied. The maximum height can be set to `5242880` (5 × 1024 × 1024). Otherwise, a failure is returned.|
|kind|Input|Memory copy kind.|
|filename|Input|Name of the file where matrix data memory copy is called.|
|lineno|Input|Number of the line where matrix data memory copy is called.|

**Returns**

`0` on success; else, failure.

**Example**

For details, see step 4 in "Checking the Memory of the CANN Software Stack" > "Troubleshooting Procedure" in [MindStudio Sanitizer Best Practices](../best_practices/basic_cases.md).

<br>
<br>

### sanitizerRtMemcpy2dAsync

---

**Function**

Calls `aclrtMemcpy2dAsync` to copy the matrix data memory and reports the memory copy information to the check tool. This API is asynchronous. The actual memory copy behavior and parameter meanings are the same as those of `aclrtMemcpy2dAsync`.

>[!NOTE]NOTE
>For details about `aclrtMemcpy2dAsync`, see "ACL API Reference (C)" > "Runtime Management" > "Memory Management" in the [Application Development APIs](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/appdevgapi/aclcppdevg_03_0094.html).

**Prototype**

```c
aclError sanitizerRtMemcpy2dAsync(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width, size_t height, aclrtMemcpyKind kind, aclrtStream stream, char const *filename, int lineno);
```

**Parameters**

**Table 1** Parameter description

|Parameter|Input/Output|Description|
|--|--|--|
|dst|Input|Pointer to the destination memory address.|
|dpitch|Input|Address distance between two adjacent columns of vectors in the destination memory.|
|src|Input|Pointer to the source memory address.|
|spitch|Input|Address distance between two adjacent columns of vectors in the source memory.|
|width|Input|Matrix width to be copied.|
|height|Input|Matrix height to be copied. The maximum height can be set to `5242880` (5 × 1024 × 1024). Otherwise, a failure is returned.|
|kind|Input|Memory copy kind.|
|stream|Input|Stream specified by the current matrix data memory copy behavior.|
|filename|Input|Name of the file where matrix data memory copy is called.|
|lineno|Input|Number of the line where matrix data memory copy is called.|

**Returns**

`0` on success; else, failure.

**Example**

For details, see step 4 in "Checking the Memory of the CANN Software Stack" > "Troubleshooting Procedure" in [MindStudio Sanitizer Best Practices](../best_practices/basic_cases.md).

<br>
<br>

### sanitizerReportMalloc

---

**Function**

Manually reports the GM allocation information.

**Prototype**

```c
void sanitizerReportMalloc(void *ptr, uint64_t size);
```

> [!NOTE]NOTE     
> This API is the encapsulated `__sanitizer_report_malloc`. `__sanitizer_report_malloc` is a weak function and takes effect only when the user program is started by the check tool.

**Parameters**

**Table 1** Parameter description

|Parameter|Input/Output|Description|
|--|--|--|
|ptr|Input|Allocated memory address.|
|size|Input|Allocated memory size.|

**Returns**

None

**Example**

None

<br>
<br>

### sanitizerReportFree

---

**Function**

Manually reports the GM release information.

**Prototype**

```c
void sanitizerReportFree(void *ptr);
```

>[!NOTE]NOTE  
>This API is the encapsulation of `__sanitizer_report_free`. `__sanitizer_report_free` is a weak function and takes effect only when the user program is started by the check tool.

**Parameters**

**Table 1** Parameter description

|Parameter|Input/Output|Description|
|--|--|--|
|ptr|Input|Address of released memory.|

**Returns**

None

**Example**

None

## MSTX Functions

**MSTX API Overview**

The mstx APIs are a set of extension APIs provided by MindStudio to allow you to insert specific tags in your application so that the memory issues of operators can be more accurately identified. For example, for level-2 pointer operators, the address space obtained without the MSTX API call may be inaccurate. The accurate address space can be transferred to the exception check tool through the `mstxMemRegionsRegister` and `mstxMemRegionsUnregister` APIs in [MindStudio Tools Extension Library APIs](https://gitcode.com/Ascend/mstx/blob/master/docs/en/api_reference/README.md) to implement more accurate memory check.

> [!NOTE]NOTE
> 
> The MSTX APIs are not supported in the kernel launch symbol scenario described in "Exception Check Function Introduction" > "Function Description" > "Application Scenarios" > "Kernel Launch Operator Development" in [MindStudio Sanitizer User Guide](../user_guide/mssanitizer_user_guide.md).

**MSTX APIs**

[Table 1](#table111) describes the MSTX APIs called by the msSanitizer. For details, see *MSTX APIs*.

**Table 1** MSTX APIs called by the msSanitizer<a name="table111"></a>

|API|Description|
|--|--|
|mstxDomainCreateA|Creates an MSTX domain.|
|mstxMemHeapRegister|Registers a memory pool. Before calling this API to register a memory pool, ensure that the memory has been allocated in advance.|
|mstxMemRegionsRegister|Registers secondary memory pool allocation. Ensure that the memory of RegionsRegister is within the range registered by `mstxMemHeapRegister`. Otherwise, the tool displays a message indicating that out-of-bounds read/write occurs.|
|mstxMemRegionsUnregister|Deregisters secondary memory pool allocation.|
|mstxMemHeapUnregister|Deregisters the regions associated with a memory pool when the memory pool is deregistered.|

**MSTX API Usage**

- By default, the msSanitizer tool enables the MSTX APIs, allowing you to customize the memory space address and size for operators to identify operator memory issues quickly.
- Currently, MSTX APIs can be used in two ways: library files and header files. The following uses the code in [AclNNInvocation](https://gitee.com/ascend/samples/tree/master/operator/ascendc/0_introduction/1_add_frameworklaunch/AclNNInvocation) as an example:

    >[!NOTE]NOTE 
    > This sample project does not support Atlas A3 training products and Atlas A3 inference products.

- Add the library file **libms_tools_ext.so** to the `${git_clone_path}/samples/operator/ascendc/0_introduction/1_add_frameworklaunch/AclNNInvocation/src/CMakeLists.txt` directory. The address is `${INSTALL_DIR}/lib64/libms_tools_ext.so`.
    
    ```c  
        # Header path
        include_directories(
             ...
            ${CUST_PKG_PATH}/include
        )
        ...
        target_link_libraries( 
            ...
            dl
        )

    ```

- In the `${git_clone_path}/samples/operator/ascendc/0_introduction/1_add_frameworklaunch/AclNNInvocation/src/main.cpp` directory, build and link the user program to the DL library. The address of the corresponding header file **ms_tools_ext.h** is `${INSTALL_DIR}/include/mstx`.
    
    ```c
    ...
    #include "mstx/ms_tools_ext.h"
    ...
    ```

    > [!NOTE]NOTE       
    > Replace `${INSTALL_DIR}` with the file storage path after the CANN software is installed. For example, if the installation is performed as the root user, the default file storage path after the installation is `/usr/local/Ascend/cann`.

**Example**

```c
mstxMemVirtualRangeDesc_t rangeDesc = {};
    rangeDesc.deviceId = deviceId;       // Device ID
    rangeDesc.ptr = gm;                  // Start address of the registered memory pool CM
    rangeDesc.size = 1024;               // Memory pool size
    heapDesc.typeSpecificDesc = &rangeDesc;
    mstxMemHeapDesc_t heapDesc{};
    mstxMemHeapHandle_t memPool = mstxMemHeapRegister(globalDomain, &heapDesc); // Memory pool registration
    mstxMemVirtualRangeDesc_t rangesDesc[1] = {};                // Number of regions contained in the secondary allocation
    mstxMemRegionHandle_t regionHandles[1] = {};
    rangesDesc[0].deviceId = deviceId;                           // Device ID
    rangesDesc[0].ptr = gm;                                      // Address of the secondary GM allocation
    rangesDesc[0].size = 256;                                    // Size of the secondary allocation
    mstxMemRegionsRegisterBatch_t regionsDesc{};
    regionsDesc.heap = memPool;
    regionsDesc.regionType = MSTX_MEM_TYPE_VIRTUAL_ADDRESS;
    regionsDesc.regionCount = 1;
    regionsDesc.regionDescArray = rangesDesc;
    regionsDesc.regionHandleArrayOut = regionHandles;
    mstxMemRegionsRegister(globalDomain, regionsDesc);              // Secondary allocation registration
    Do(blockDim, nullptr, stream, gm);                            // Operator kernel function
    mstxMemRegionRef_t regionRef[1] = {};
    regionRef[0].refType = MSTX_MEM_REGION_REF_TYPE_HANDLE;
    regionRef[0].handle = regionHandles[0];
    mstxMemRegionsUnregisterBatch_t refsDesc = {};
    refsDesc.refCount = 1;
    refsDesc.refArray = regionRef;
    mstxMemRegionsUnregister(globalDomain, &refsDesc);                   // Secondary allocation deregistration
    mstxMemHeapUnregister(globalDomain, memPool);                        // Memory pool deregistration
```
