# MindStudio Sanitizer User Guide

<br>

## 1. Overview

MindStudio Sanitizer (msSanitizer) is a tool based on AI Processors. It provides memory check, race check, uninitialization check, and synchronization check in single-operator development scenarios.

| Function    | Description                                                                                              |
| ---------- |--------------------------------------------------------------------------------------------------|
| Memory check  | During operator development, the tool can locate memory problems such as illegal read/write, multi-core corruption, non-aligned access, memory leak, and illegal release.<br>In addition, the tool can detect the memory of the CANN software stack, helping users locate the module with memory exception in the software stack.|
| Contention check  | The tool helps users locate data contention problems that may be caused by contention risks, including intra-core contention and inter-core contention. Intra-core contention includes inter-pipeline contention and intra-pipeline contention.                                 |
| Uninitialization check| The tool helps users locate dirty data read problems that may be caused by uninitialized memory.                                                                 |
| Synchronization check  | The tool helps users locate synchronization failures in subsequent operators due to unpaired synchronization instructions in the preceding operators.                                                        |

---

## 2. Quick Reference

### Overview of Four Check Tools

| Check Tool| Target Issues| Quick Start Command|
| --- | --- | --- |
| `memcheck` (default)| Memory overwriting, multi-core corruption, unaligned access, memory leak, and illegal release| `mssanitizer --tool=memcheck ./application` |
| `racecheck` | Data races (WAW/WAR/RAW)| `mssanitizer --tool=racecheck ./application` |
| `initcheck` | Dirty data caused by reading uninitialized memory| `mssanitizer --tool=initcheck ./application` |
| `synccheck` | Synchronization failure caused by unmatched SetFlag/WaitFlag| `mssanitizer --tool=synccheck ./application` |

### Application Scenario

| My Development Scenario| Reference Section|
| --- | --- |
| Directly calling kernel functions through `<<<>>>`| [4.1 Kernel Launch](#41-kernel-launch)|
| Calling `aclnn` single-operator APIs| [4.2 Single-Operator API Calling Scenario](#42-single-operator-api-calling-scenario)|
| Integrating operators in the PyTorch framework (TorchAir graph mode, etc.)| [4.3 PyTorch Framework Adaptation Scenario](#43-pytorch-framework-adaptation-scenario)|
| Triton-Ascend operators| [4.4 Triton Operator Calling Scenario](#44-triton-operator-calling-scenario)|

### Most Frequently Used Options

| Options| Function| Example|
| --- | --- | --- |
| `--tool` / `-t` | Specifies the check tool (memcheck by default).| `-t racecheck` |
| `--leak-check=yes` | Enables memory leak check.| `--leak-check=yes` |
| `--check-unused-memory=yes` | Enables the check for unused allocated memory.| `--check-unused-memory=yes` |
| `--log-file` | Exports the report to a file.| `--log-file=result.log` |
| `--kernel-name` | Checks only the operator with the specified name (fuzzy match supported).| `--kernel-name="add"` |
| `--block-id` | Checks only the specified block (single-block debugging mode).| `--block-id=0` |
| `--full-backtrace=yes` | Shows the complete call stack of the AscendC API.| `--full-backtrace=yes` |

> [!NOTE]NOTE  
> For details about other options, see [7.2 Options](#72-options).

---

## 3. Usage Process

The standard process of using msSanitizer to check operator exceptions is as follows:

1. **Select an application scenario**: Determine the running mode based on your development environment. For details, see [4. Application Scenarios](#4-application-scenarios).
2. **(Optional) Configure compilation options**: If full check and call stack information are required, recompile the operator. For details, see [5. (Optional) Configuring Operator Compilation Options](#5-optional-configuring-operator-compilation-options).
3. **Run the check**: Run the `mssanitizer` command to start the check. For details, see [6. Check Functions](#6-check-functions) and [7. Commands and Options](#7-commands-and-options).
4. **Interpret the report**: Refer to the exception report output in the console to identify and resolve the issues. For details, see "Interpreting the Exception Report" in each section.

> [!NOTE]NOTE  
> You are advised to run memcheck (memory check) first to ensure that the operator program has no memory exception, and then run racecheck, initcheck, or synccheck as required.

---

## 4. Application Scenarios

### 4.1 Kernel Launch

If you directly call the kernel function through `<<<>>>` for testing, run the check tool as follows:

```shell
mssanitizer --tool=memcheck ./add_npu
```

> [!NOTE]NOTE  
> When the custom operators are launched through `<<<>>>` and integrated by torch, the GM is managed in memory pool mode by default, which may cause inaccurate out-of-bounds check results. Therefore, you need to set the following environment variable to disable the memory pool before the check to obtain more accurate check results:
>
> ```shell
> export PYTORCH_NO_NPU_MEMORY_CACHING=1
> ```

### 4.2 Single-Operator API Calling Scenario

1. In the single-operator API calling (aclnn) scenario, use the check tool to start the operator API run script. For example:

    ```shell
    mssanitizer --tool=memcheck bash run.sh
    ```

2. When calling an API with the `aclnn` prefix, run the following command to pass the `acl.json` file through the `aclInit` API to ensure the accuracy of memory check:

    ```c
    auto ret = aclInit("./acl.json"); // The content of acl.json file is {"dump":{"dump_scene":"lite_exception"}}.
    ```

### 4.3 PyTorch Framework Adaptation Scenario

1. In PyTorch graph mode (TorchAir), the check can be performed only when no compilation option is added to msSanitizer. For details, see [5. (Optional) Configuring Operator Compilation Options](#5-optional-configuring-operator-compilation-options).
2. In PyTorch graph mode (TorchAir), the Ascend IR graph execution mode and aclgraph graph execution mode are supported. For details, see "PyTorch Graph Mode (TorchAir)" > "Reduce-Overhead Mode" > "Configuring the Reduce-Overhead Mode" in the [Ascend Extension for PyTorch](https://www.hiascend.com/document/detail/zh/Pytorch/720/modthirdparty/torchairuseguide/torchair_00015.html).
3. For details about the PyTorch framework calling scenario, see "PyTorch Framework Feature Guide" > "Custom Operator Adaptation Development" > "OpPlugin-based Operator Adaptation Development" in [Ascend Extension for PyTorch](https://www.hiascend.com/document/detail/zh/Pytorch/720/ptmoddevg/Frameworkfeatures/featuresguide_00021.html). For details, see "Checking Operators Called by PyTorch APIs" in [example](../best_practices/basic_cases.md).

### 4.4 Triton Operator Calling Scenario

#### Prerequisites

- The Triton and Triton-Ascend plugins have been installed and configured. For details, see [Triton-Ascend project repository](https://gitcode.com/Ascend/triton-ascend).
- **Atlas inference series products are not supported.**

#### Configuring Environment Variables

To ensure check accuracy and avoid cache interference, set the following environment variables before running the program:

| Environment Variable| Description|
| --- | --- |
| `TRITON_ALWAYS_COMPILE=1` | Forcibly recompiles the Triton operator to avoid using the cached old version.|
| `PYTORCH_NO_NPU_MEMORY_CACHING=1` | Disables the NPU memory pool mechanism of PyTorch to prevent it from interfering with the memory check result.|

> [!NOTE]NOTE
> In a Triton scenario, PyTorch is used to create tensors. In the PyTorch framework, the GM is managed in memory pool mode by default, which interferes with memory check. Therefore, the memory cache must be disabled to ensure the validity of the check.

For details, see section "Detecting Triton Operators" in [Basic Scenario Cases](../best_practices/basic_cases.md).

## 5. (Optional) Configuring Operator Compilation Options

You can determine whether to modify the compilation options and recompile the operator as required. The details of the two scenarios are as follows.

### 5.1 Modification Not Required (Quick Identification)

- **Instruction check scope:** GM-related transfer instructions.
- **Exception check scope:** Only invalid read/write and unaligned access in memory check are supported. The call stack information is not displayed in the exception report.
- **Application scenario**: Applicable only to quickly identify invalid read/write and unaligned access exceptions in operator memory.

> [!NOTE]NOTE
>
> - In this scenario, the optimization level of the operator must be `O2`, and the `-q` option must be added in the operator linking phase to retain the symbol relocation information. Otherwise, the check function will fail.
> - This scenario is not applicable to Atlas inference series products.
> - This scenario applies only to the operator kernel launch symbol scenario.

### 5.2 Modification Required (Full Check)

- **Instruction check scope:** all instructions.
- **Exception check scope:** Full check is supported. After the `-g` option is added to the compilation options, the call stack information is displayed in the exception report.
- **Application scenario:** Quickly locate the abnormal operator by not adding compilation options, and then add compilation options to perform full check on the abnormal operator.

To enable full check, modify the compilation options by referring to [Enabling Full Check](./compile_option_config.md) and recompile the operator.

---

## 6. Check Functions

> [!NOTE]NOTE
>
> Exception reports are classified into the following levels:
>
> - **WARNING**: This level indicates an unclear risk. Possible exceptions, such as multi-core corruption and unused memory allocation, depend on the real situation. The risk of multi-core corruption involves operations on the same memory block by multiple cores. Experienced users can avoid this risk by means of inter-core synchronization. But this exception for beginners is a high risk. Currently, the WARNING-level report of multi-core corruption can be generated only for inter-core synchronization of the atomic type.
> - **ERROR**: This is the highest severity level of exceptions, which are deterministic memory errors, such as invalid read/write, memory leak, unaligned access, memory not initialized, and contention exceptions. It is strongly recommended that you check for exceptions at this level.

### 6.1 Memory Check

Memory check is used to detect exceptions during program running. It can detect and report memory access exceptions such as out-of-bounds issues related to the external memory (global memory) and internal memory (local memory) and unaligned memory during operator running.

#### 6.1.1 Memory Exception Types

The following table lists memory exception types supported by memory check, including illegal memory read/write, multi-core corruption, unaligned access, memory leak, illegal release, and allocated memory unused.

**Table 1** Memory exception types

| Exception| Description| Location| Address Space|
| --- | --- | --- | --- |
| [Illegal Read/Write](#6131-illegal-readwrite)| This exception occurs when unallocated memory is accessed.| Kernel, Host| GM, UB, L0{A,B,C}, L1|
| [Multi-Core Corruption](#6132-multi-core-corruption)| This exception occurs when the AI Core accesses overlapped memory.| Kernel | GM |
| [Unaligned Access](#6133-unaligned-access)| This exception occurs when the address transferred by DMA (responsible for transferring data between the global memory and local memory) is not aligned with the minimum memory access granularity.| Kernel | GM, UB, L0{A,B,C}, L1|
| [Illegal Release](#6135-illegal-release)| This exception occurs when the IP address that is not allocated or has been released is released.| Host | GM |
| [Memory Leak](#6134-memory-leak)| This exception occurs when the memory usage keeps increasing during program running after the allocated memory is not released after being used.| Host | GM |
| [Allocated Memory Unused](#6136-allocated-memory-unused)| This exception occurs when memory is not used after being allocated.| Kernel, Host| GM |
| [Ascend 950 SIMT unit exception](#6137-ascend-950-simt-unit-exception)| Displays the thread where the exception occurs in the SIMT architecture.| Kernel | GM |
| [Inter-Thread Corruption](#6138-inter-thread-corruption)| This exception occurs when threads in the AI Core access overlapped memory.| Kernel | GM |
| [Register Alarm](#6139-register-alarm)| If the register value is not restored to the default value, an alarm is generated, indicating that there is residual register value.| Kernel | GM |

#### 6.1.2 Enabling Memory Check

When msSanitizer is running, memory check is enabled by default. In the command, `application` indicates the user program.

- You can run the following command to explicitly specify memory check types. By default, illegal read/write, multi-core corruption, unaligned access, and illegal release checks are enabled.

    ```shell
    mssanitizer --tool=memcheck application
    ```

- Run the following command to manually enable memory check and memory leak check.

    ```shell
    mssanitizer --tool=memcheck --leak-check=yes application
    ```

- Run the following command to manually enable memory check and check for unused allocated memory:

    ```shell
    mssanitizer --tool=memcheck --check-unused-memory=yes application
    ```

> [!NOTE]NOTE
>
> - After the user program is complete, an exception report is displayed on the GUI.
> - When you use a framework such as PyTorch to access an operator, the framework may use a memory pool to manage the GM. However, the memory pool usually allocates a large amount of the GM at a time and reuses the memory during running. In this case, if you check the operator and record all memory allocation and release information about the GM, the check may be inaccurate due to the memory management mode of the memory pool. Therefore, the check tool provides APIs for manually reporting the GM allocation information so that you can manually report the GM range used by an operator when the operator is called. For details about the APIs, see the `sanitizerReportMalloc` and `sanitizerReportFree` APIs in [MindStudio Sanitizer External API Reference](../api_reference/mssanitizer_api_reference.md).
> - msSanitizer also supports the invalid read and write check for the `AllReduce`, `AllGather`, `ReduceScatter`, and `AlltoAll` APIs of Atlas A2 training products/Atlas A2 inference products and the `AllGather`, `ReduceScatter`, and `AlltoAllV` APIs of Atlas A3 training products/Atlas A3 inference products. For details, see "Advanced APIs" > "HCCL" > "HCCL Kernel APIs" in [Ascend C Operator Development APIs](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/ascendcopapi/atlasascendc_api_07_0869.html).
> - msSanitizer can also check invalid read and write operations of MC2 operators.
> - Currently, Ascend 950 supports GM/UB/L1/L0A/L0B/L0C in memory check. Other types are not supported.

#### 6.1.3 Interpreting a Memory Exception Report

The memory check exception report contains multiple types of exception information. The following provides some examples about simple exception information to help you understand the information in the exception report.

##### 6.1.3.1 Illegal Read/Write

An illegal read/write exception occurs when an operator program accesses unallocated memory through a read or write operation. This error usually occurs in the GM or on-chip memory. The GM exception occurs because the allocated GM size is inconsistent with the actual access range by the operator program. The on-chip memory exception occurs because the access range of the operator program exceeds the upper limit of the hardware capacity.

    ====== ERROR: illegal read of size 224  // Basic error information, including invalid read/write type (invalid read or invalid write) and invalid access bytes.
    ======    at 0x12c0c0015000 on GM in add_custom_kernel  // Memory location where the exception occurs, including kernel function name, address space, and memory address. The memory address refers to the start address in a memory access.
    ======    in block aiv(0) on device 0  // Block index of the Vector core corresponding to the exception code.
    ======    code in pc current 0x77c (serialNo:10) // PC pointer where the exception occurs and sequence number of the API call behavior.
    ====== #0 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:58:9  // The following is the code call stack where the exception occurs, including the file name, line number, and column number.
    ======    #1 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:58:9
    ======    #2 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:443:5
    ======    #3 illegal_read_and_write/add_custom.cpp:18:5
    

In the preceding example, the 0x12c0c0015000 address on GM is illegally read, and the instruction that causes the exception corresponds to line 18 in the operator implementation file `add_custom.cpp`.

   > [!NOTE]NOTE  
    > If no compilation option is added, the following call stack information is not displayed in the exception report:

    ====== #0 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:58:9  // The following is the code call stack where the exception occurs, including the file name, line number, and column number.
    ======    #1 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:58:9
    ======    #2 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:443:5
    ======    #3 illegal_read_and_write/add_custom.cpp:18:5

##### 6.1.3.2 Multi-Core Corruption

 AI Core is the compute core within the AI processor. The AI processor contains multiple AI Cores, and operator execution takes place on these AI Cores. During computation, these AI Cores transfer data to and from GM. In the absence of explicit inter-core synchronization, a multi-core corruption may occur when the GM regions accessed by multiple cores overlap, and at least one core performs a write operation to the overlapped address. Here we use the concept of ownership to ensure that no corruption occurs between multiple cores. Once a memory region has been written to by a given core, that memory region becomes owned by that core. Any access to this memory region by another core will result in an out-of-bounds exception.

```text
    ====== WARNING: out of bounds of size 256  // Basic error information, including the number of corrupted bytes.
    ======    at 0x12c0c00150fc on GM when writing data in add_custom_kernel  // Memory location where the exception occurs, including kernel function name, address space, and memory address. The memory address refers to the start address in a memory access.
    ======    in block aiv(9) on device 0  // Block index of the Vector core corresponding to the exception code.
    ======    code in pc current 0x7b8 (serialNo:22)  // PC pointer where the exception occurs and sequence number of the API call behavior.
    ======    #0 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:103:9  // The following is the code call stack where the exception occurs, including the file name, line number, and column number.
    ======    #1 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:155:9
    ======    #2 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:461:5
    ======    #3 out_of_bound/add_custom.cpp:21:5
```

In the preceding example, a total of 256 bytes are corrupted, and multi-core corruption occurs when the 0x12c0c00150fc address in the GM is accessed. In addition, the instruction that causes the exception corresponds to line 21 in the operator implementation file **add_custom.cpp**.

##### 6.1.3.3 Unaligned Access

Ascend AI Processor integrates multiple types of memory. When accessed via DMA, different memory types may have varying minimum access granularities depending on the specific processor. If the memory address being accessed is not aligned with the corresponding minimum access granularity, issues such as data or AI Core exceptions may occur. Access alignment check can output alignment exception information when an alignment problem occurs.

 ```text
    ====== ERROR: misaligned access of size 13  // Basic error information, including the number of bytes pertaining to the alignment exception.
    ======    at 0x6 on UB in add_custom_kernel   // Memory location where the exception occurs, including kernel function name, address space, and memory address.
    ======    in block aiv(0) on device 0  // Block index of the Vector core corresponding to the exception code.
    ======    code in pc current 0x780 (serialNo:33)  // PC pointer where the exception occurs and sequence number of the API call behavior.
    ======    #0 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:103:9  // The following is the code call stack where the exception occurs, including the file name, line number, and column number.
    ======    #1 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:155:9
    ======    #2 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:461:5
    ======    #3 illegal_align/add_custom.cpp:18:5
```

In the preceding example, the alignment exception occurs on 13 bytes when the 0x6 address on the UB is accessed. The instruction that causes this exception corresponds to line 18 in the operator implementation file `add_custom.cpp`.

> [!NOTE]NOTE
> 
    > If no compilation option is added, the following call stack information is not displayed in the exception report:
    >
    > ```text
    > ======    #0 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:103:9  // The following is the code call stack where the exception occurs, including the file name, line number, and column number.
    > ======    #1 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:155:9
    > ======    #2 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:461:5
    > ======    #3 illegal_align/add_custom.cpp:18:5
    > ```

##### 6.1.3.4 Memory Leak

Memory check can check memory leak on the device. This problem is usually caused by developers' failure to correctly free the memory allocated by AscendCL APIs. Memory allocation is not performed in local memory. Therefore, memory leak occurs only in the GM. You can enable memory leak check by specifying the command line option `--leak-check=yes`.

```text
    ====== ERROR: LeakCheck: detected memory leaks     // Memory leak occurs.
    ======    Direct leak of 100 byte(s)      // Information about the leaked memory
    ======      at 0x124080013000 on GM allocated in add_custom.cpp:14 (serialNo:37)
    ======    Direct leak of 1000 byte(s)
    ======      at 0x124080014000 on GM allocated in add_custom.cpp:15 (serialNo:55)
    ====== SUMMARY: 1100 byte(s) leaked in 2 allocation(s)     // Summary of memory leaks, including the number of memory leaks and the number of leaked bytes.
```

In the preceding example, the first piece of memory leak information includes the address space, memory address, memory size, and code location. The code location points to the file name and line number of the call that allocates the memory.

##### 6.1.3.5 Illegal Release

Illegal release refers to the release of an unassigned or released address, which usually occurs on the GM.

```text
    ====== ERROR: illegal free()     // Basic error information, indicating that an illegal release exception occurs.
    ======    at 0x124080013000 on GM      // Memory location where the exception occurs, including the address space and memory address.
    ======    code in add_custom.cpp:84 (serialNo:63)    // Code location when the exception occurs, including the file name, line number, and sequence number of the API call behavior.
```

In the preceding example, the 0x124080013000 address in the GM is illegally released, and the instruction that causes the exception corresponds to line 84 in the operator implementation file **add_custom.cpp**.

##### 6.1.3.6 Allocated Memory Unused

This exception occurs when memory that is allocated during operator running has never been used until the operation running ends. This exception is generally caused by incorrect memory access by an operator or flawed operator logic, and it often occurs in the GM.

```text
    ====== WARNING: Unused memory of 1000 byte(s)     // Basic error information, indicating that the allocated memory is not used.
    ======    at 1240c0016000 on GM                    // Memory location where the exception occurs, including the address space and memory address.
    ======    code in add_custom.cpp:2 (serialNo:69)   // Code location when the exception occurs, including the file name, line number, and sequence number of the API call behavior.
    ====== SUMMARY: 1100byte(s) unused memory in 2 allocation(s) // Exception summary, including the number of used memory blocks and bytes.
 ```

##### 6.1.3.7 Ascend 950 SIMT Unit Exception

In the SIMT architecture, the location of the thread where the exception occurs is also provided. The thread ID starts from 0. For example, the following exception occurs at the thread whose ID X=1, ID Y=0, and ID Z=0. When the SIMT unit of the Ascend 950 product is abnormal, the error information is displayed as follows:

 ```text
    ====== ERROR: illegal read of size 4
    ======    at 0x300000018ffc on GM in vec_add
    ======    by thread (1,0,0) in block aiv(0-1) on device 0
    ======    code in pc current 0x178 (serialNo:16)
    ======    #0 ${ASCEND_HOME_PATH}/illegal_read_and_write_simt_gm_float/kernel.cpp:16:21
```

##### 6.1.3.8 Inter-Thread Corruption

In the SIMT architecture, when programming with multiple threads, improper handling of GM accesses may result in multiple threads writing data to the same memory address simultaneously, leading to a memory corruption issue between threads. The check mechanism operates similarly to multi-core corruption check. Once a given thread writes to a memory block for the first time, the memory block is considered exclusively owned by that thread. If any other thread attempts to write to that memory location, an out-of-bounds exception occurs.

```text
    ====== WARNING: out of bounds of size 4
    ======    at 0x300000056000 on GM when writing data in vec_add
    ======    by thread (1,0,0) in block aiv(0) on device 0
    ======    code in pc current 0xd8 (serialNo:16)
    ======    #0 vec_add_simt.cpp:20:12
 ```

##### 6.1.3.9 Register Alarm

When a register is found not to have its default value, an alarm is generated to notify the user. The alert displays the name of the non-reset register, the associated core number, the operator name, the expected default value, and the current actual value.

```text
    [mssanitizer]Warning:Register XXX was not reset to default in block aiv(XXX) on kernel XXX. Expected default value is (XXX), but current value is (XXX)
```

> [!NOTE]NOTE
    > Currently, only memory check supports register alarms.

### 6.2 Race Check

Race check is used to resolve memory access race in a parallel computing environment. In the Ascend AI Processor architecture, the external storage and internal storage are usually used as temporary buffers to store data being processed. The external storage or internal storage can be accessed by multiple pipelines at the same time, and the external storage can be accessed by multiple cores. If the operator program does not correctly process inter-core, inter-pipeline, or intra-pipeline synchronization, data race may occur.

#### 6.2.1 Race Exception Types

Memory race occurs when two memory events, at least one of which is a write event, attempt to access the same memory block, with results that do not conform to the expected order of execution. This exception causes a data race, which causes the program's execution or output to depend on the order in which memory events are actually executed. The race check function can identify the following typical memory races:

**Table 1** Memory race types

| Exception| Description| Location| Address Space|
| --- | --- | --- | --- |
| Write-After-Write(WAW) | This exception may occur when two memory events attempt to write to the same memory block. As a result, the memory result value depends on the actual access sequence of the two memory events.| Kernel | GM, UB, L0{A,B,C}, L1|
| Write-After-Read(WAR) | This exception may occur when two memory events (one memory read event and one memory write event) attempt to access the same memory block. The write operation event is actually executed before the read operation event, and the read memory value is not the expected start value.| Kernel | GM, UB, L0{A,B,C}, L1|
| Read-After-Write(RAW) | This exception may occur when two memory events (one memory read event and one memory write event) attempt to access the same memory block. The read operation event is actually executed before the write operation event, and the read memory value is not updated.| Kernel | GM, UB, L0{A,B,C}, L1|

When race check identifies an exception, you can modify the program to ensure that the exception does not exist. In the case of "write before read" or "read before write", the sequence is determined based on the value of `serialNo`. The task with a smaller `serialNo` is executed first on `PIPE_S`.

#### 6.2.2 Enabling Race Check

When running the msSanitizer tool, run the following command to enable race check:

```shell
mssanitizer --tool=racecheck application    // application indicates the user program.
```

> [!NOTE]NOTE
>
> - Race check does not check for memory errors. You are advised to perform memory check first to ensure that the operator program can be executed properly.
> - After the user program is complete, an exception report is displayed on the GUI.
> - After the tool is started, the tool run log file `mssanitizer_{TIMESTAMP}_{PID}.log` is automatically generated in the current directory.

#### 6.2.3 Interpreting a Race Exception Report

Race check outputs information to describe the memory race access risks between pipes of the operator.

- The following example indicates that there is a write-before-read race risk on the UB within the Vector core of AI Core 0. The PIPE_MTE2 pipeline writes to the 0x0 address, and this operation corresponds to line 17 in the operator implementation file `add_custom.cpp`. The PIPE_MTE3 pipeline reads the 0x0 address, and this operation corresponds to line 22 in the operator implementation file `add_custom.cpp`.

    ```text
    ====== ERROR: Potential RAW hazard detected at GM in kernel_float on device 0: // Race event type, abnormal memory block information, and name of the kernel function where race occurs.
    ======    PIPE_MTE2 Write at RAW()+0x0 in block 0 (aiv) on device 0 at pc current 0xa98 (serialNo:14)  // Detailed information about the race event, including the pipe where the event is located, operation type, memory access start address, core type, AI Core information, PC pointer executed by the code, and sequence number of the API call behavior.
    ====== #0 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:58:9  // The following is the code call stack where the exception occurs, including the file name, line number, and column number.
    ======    #1 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:58:9
    ======    #2 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:443:5
    ======    #3 Racecheck/add_custom.cpp:17:5
    ======    PIPE_MTE3 Read at RAW()+0x0 in block 0 (aiv) on device 0 at pc current 0xad4 (serialNo:17)
    ======    #0 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:103:9
    ======    #1 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:155:9
    ======    #2 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:461:5
    ======    #3 Racecheck/add_custom.cpp:22:5
    ```

- The following example shows that a write-write race occurs between Thread (0,0,0) and Thread (0,0,1) at line 88 in `kernel.cpp`, and both threads write data to the 0x0 UB address of core 0. This race check function supports only memory races in the GM or UB space.

    ```text
    ====== ERROR: Potential WAW hazard detected at UB in vec_add_5 on device 0:
    ======    Write Thread(0,0,0) at WAW()+0x0 in block 0(aiv) on device 0 at pc current 0xcc8
    ======    #0 racecheck/simt/inner_core/ub_error/kernel.cpp:88:13
    ======    Write Thread(0,0,1) at WAW()+0x0 in block 0(aiv) on device 0 at pc current 0xcc8
    ======    #0 racecheck/simt/inner_core/ub_error/kernel.cpp:88:13
    ```

### 6.3 Uninitialization Check

Uninitialization check is an important memory security protection mechanism designed to identify and prevent memory exceptions caused by uninitialized variables.

#### 6.3.1 Uninitialization Exception Types

**Table 1** Uninitialization exception types

| Exception| Description| Location| Address Space|
| --- | --- | --- | --- |
| Uninitialization| After memory allocation, the memory is in the uninitialized state. The memory is not written and the uninitialized value is directly read.| Kernel, Host| GM, UB, L1, L0{ABC}, stack space|

#### 6.3.2 Enabling Uninitialization Check

When running the msSanitizer tool, run the following command to enable the uninitialization check function:

```shell
mssanitizer --tool=initcheck application   // application indicates the user program.
```

> [!NOTE]NOTE
>
> - After the tool is started, the tool run log file `mssanitizer_{TIMESTAMP}_{PID}.log` is automatically generated in the current directory.
> - Due to hardware restrictions, some instructions support data transfer only in blocks. If the actual data volume involved in the computation is not an integer multiple of the block size, some invalid data (dirty data) may be inevitably imported. This may trigger tool initialization anomaly reports. You need to determine whether the dirty data affects the computation result.
> - Uninitialized memory check is not supported by Ascend 950 products.

#### 6.3.3 Interpreting an Uninitialized Memory Exception Report

The uninitialization exception report contains multiple types of exception information. The following provides some examples about simple exception information to help you understand the information in the exception report.

Abnormal scenarios where initialization is not performed are as follows: The operator reads the allocated but uninitialized memory in the GM, UB, L1, L0{ABC}, and stack space.

```text
====== ERROR: uninitialized read of size 224  // Basic error information, including the number of uninitialized bytes that are read.
======    at 0x12c0c0015000 on GM in add_custom_kernel  // Memory location where the exception occurs, including kernel function name, address space, and memory address. The memory address refers to the start address in a memory access.
======    in block aiv(0) on device 0  // Block index of the Vector core corresponding to the exception code.
======    code in pc current 0x77c (serialNo:10) // PC pointer where the exception occurs and sequence number of the API call behavior.
======    #0 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:58:9  // The following is the code call stack where the exception occurs, including the file name, line number, and column number.
======    #1 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:58:9
======    #2 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:443:5
======    #3 uninitialized_read/add_custom.cpp:18:5
```

### 6.4 Synchronization Check

During the development of an Ascend C operator, `SetFlag` and `WaitFlag` must be used in pairs. The synchronization check function is used to find the unpaired `SetFlag` instructions in the operator.

A redundant `SetFlag` instruction does not directly cause a race problem of the current operator, but changes the status of the hardware counter. This may subsequently cause a synchronization instruction pairing error of subsequent operators. If the subsequent operators do not race with each other, no error is reported during race check. However, the counter change of preceding operators may cause actual contention. The synchronization check function can effectively identify redundant `SetFlag` instructions in the preceding operators, preventing the subsequent operators from being affected.

If there are redundant `SetFlag` instructions, and two or more redundant `SetFlag` instructions exist, synchronization check will trigger both matching exceptions and redundancy exceptions.

> [!NOTE]NOTE
>
> - When synchronization check is enabled independently, memory check and race check are not performed. Therefore, you are advised to enable memory check and race check first. If no exception is reported during race check but operator race occurs, you can use synchronization detection to check the preceding operators.
> - A redundant `WaitFlag` instruction blocks the subsequent instructions of the current operator, causing the operator to stop running. In this case, developers can detect problems without tool prompts.
> - Synchronization check is not supported by Ascend 950 products.

#### 6.4.1 Synchronization Exception Types

**Table 1** Synchronization exception types

| Exception| Description| Location|
| --- | --- | --- |
| Synchronization check| Unpaired `SetFlag` synchronization instructions will cause counter status errors, even if they do not directly affect the functionality of the current operator. Synchronization instruction pairing of subsequent operators may be disturbed, furthermore, computational precision of the subsequent operators is affected.| Kernel |
| Redundancy check| If two `set_flag` instructions with identical parameters (`pipe` and `eventId`) are written and no operation is performed on the target pipe, all subsequent operator synchronization instructions will fail to pair, leading to exceptions.| Kernel |

#### 6.4.2 Enabling Synchronization Check

When running the msSanitizer tool, run the following command to enable synchronization check:

```shell
mssanitizer --tool=synccheck application // application indicates the user program.
```

> [!NOTE]NOTE
>
> - After the tool is started, the tool run log file `mssanitizer_{TIMESTAMP}_{PID}.log` is automatically generated in the current directory.
> - After the user program is complete, an exception report is displayed on the GUI.

#### 6.4.3 Interpreting a Synchronization Exception Report

The synchronization check exception report lists the information about the unpaired `SetFlag` instructions in each operator in sequence, including the source flow, target flow, and specific location.

```text
====== WARNING: Unpaired set_flag instructions detected  // Unpaired `set_flag` instruction detected.
======    from PIPE_S to PIPE_MTE3 in kernel  // Identifies the synchronization from PIPE_S to PIPE_MTE3. PIPE_MTE3 waits for PIPE_S.
======    in block aiv(0) on device 1  // The exception code corresponds to the block index and device number of the Vector core. In this example, the block index is 0 and the device number is 1.
======    code in pc current 0x2c94 (serialNo:31) // PC pointer of the current exception and sequence number of the API call behavior.
======    #0 /home/Ascend/compiler/tikcpp/tikcfw/impl/kernel_event.h:785:13  // The following is the call stack of the code where the exception occurs, including the file name, line number, and column number.
======    #1 /home/Ascend/compiler/tikcpp/tikcfw/interface/kernel_common.h:150:5
======    #2 /home/test/ascendc_test_syncall/kernel.cpp:26:9
```

---

## 7. Commands and Options

### 7.1 Command Syntax

You can run the following command to call msSanitizer:

```shell
mssanitizer [<options>] [--] <user_program> [<user_options>]
```

The configuration of each part is described as follows:

1. `options`: command line options of the tool. For details about the options and their default arguments, see [7.2 Options](#72-options).
2. `user_program`: user operator program.
3. `user_options`: command line options of the user program.
4. `--`: If the program to be loaded contains command line options, use `--` to separate the tool from the user program before the program, for example, `mssanitizer -- application parameter1 parameter2 ...`.

### 7.2 Options

**Table 1: Common options**

| Option| Description| Value| Mandatory (Yes or No)|
| --- | --- | --- | --- |
| -v, --version| Queries the msSanitizer version.| - | No|
| -t, --tool| Specifies the sub-tool for exception check.| `memcheck`: memory check (default)<br>`racecheck`: race check<br>`initcheck`: uninitialization check<br>`synccheck`: synchronization check| No|
| --log-file | Exports the check report to a file.| `{file_name}`, for example, `test_log`<br>Note:<br>Only digits, uppercase letters, lowercase letters, and the following special characters are supported -./_<br>To prevent log leakage, you are advised to restrict the file permission to ensure that only authorized personnel can access the file.<br>The tool exports the report to the `test_log` file in overwriting mode. If the `test_log` file contains content, the content will be cleared. You are advised to specify an empty file for exporting the report.| No|
| --log-level | Specifies the output level of the check report.| `info`: outputs running information at the info, warn, and error levels.<br>`warn`: outputs running information at the warn and error levels (default).<br>`error`: outputs running information at the error level.| No|
| --max-debuglog-size | Specifies the maximum size of a single debug log file output by the check tool.| The value is an integer ranging from 1 to 10240, in MB.<br>Defaults to `1024`.<br>Note:<br>`--max-debuglog-size=100` indicates that the maximum size of a debug log file is 100 MB.| No|
| --block-id | Enables the single-block check function or not.| The value is an integer ranging from 0 to 200.<br>Disabled<br>Memory check, uninitialized check, and synchronization check: all blocks are checked by default.<br>Race check: By default, inter-core check covers all blocks, while intra-core check covers race within and between the pipelines of block 0.<br>Enabled<br>Memory check, uninitialized check, and synchronization check: Specified blocks are checked.<br>Race check: Inter-core check is not performed. The race within and between the pipelines of a specified block is checked.| No|
| --cache-size | Specifies the GM size of a single block.| The value is an integer ranging from 1 to 8192, in MB.<br>The default value for a single block is 100 MB, indicating that a single block can be allocated with 100 MB of memory.<br>Note:<br>When single-block check is enabled, the maximum value of `--cache-size` is 8192 MB. If single-block check is disabled, the maximum value of `--cache-size` is (24 × 1024/Number of blocks).<br>If the value of `--cache-size` does not meet the requirements, the exception check tool will print a message prompting you to reset the value of `--cache-size`. For details, see the description of the msSanitizer tool prompting that `--cache-size` is abnormal in the MindStudio Sanitizer FAQs.| No|
| --kernel-name | Specifies the name of the operator to be checked.| Partial strings within operator names can be used for fuzzy matching. If this option is not specified, the system checks all operators scheduled during the execution of the program by default.<br>For example, to check operators named `abcd` and `bcd` at the same time, you can set `--kernel-name="bc"`. The system will automatically identify and check all operators containing the string "bc".| No|
| --full-backtrace | Displays the call stack backtrace within the AscendC API.| `yes`: Displays the complete call stack backtrace.<br>`no` (default): Does not display the call stack of the AscendC API.| No|
| --demangle | Sets the demangle mode for displaying function names in the output.| `full` (default): Displays the complete demangled function name.<br>`simple`: Displays only the function name, excluding the return value and parameter list.<br>`no`: Displays the function name that is not demangled.| No|
| -h, --help| Outputs help information.| - | No|

**Table 2: Memory check options**

| Option| Description| Value| Mandatory (Yes or No)|
| --- | --- | --- | --- |
| --check-unused-memory | Enables the check for unused allocated memory.| `yes`/`no` (default)| No|
| --leak-check | Enables memory leak check.| `yes`/`no` (default)| No|
| --check-device-heap | Enables device memory check.| `yes`/`no` (default)| No|
| --check-cann-heap | Enables memory check for the CANN software stack.| `yes`/`no` (default)| No|

> [!NOTE]NOTE
>
> - After `--check-device-heap` or `--check-cann-heap` is enabled, the kernel will not be checked.
> - The device memory check and CANN software stack memory check cannot be enabled at the same time. If they are enabled at the same time, the error message CANNOT enable both --check-cann-heap and --check-device-heap` is displayed.
> - Programs that are recompiled by using the API header file provided by msSanitizer can be used only for leak check on AscendCL-series APIs and cannot be used for leak check on Device APIs.

### 7.3 Rules for Combining Check Functions

The exception check tool provides memory check, race check, uninitialization check, and synchronization check. Multiple check functions can be combined and enabled based on the following principles:

- Multiple check functions can be enabled at the same time by specifying the `--tool` option for multiple times. For example, you can run the following command to enable memory check and race check at the same time:

    ```shell
    mssanitizer -t memcheck -t racecheck ./application
    ```

- If the sub-option corresponding to the check function is enabled, the corresponding check function is also enabled by default. For example, if the leak check sub-option corresponding to memory check is enabled, the memory check function is automatically enabled.

    ```shell
    mssanitizer -t racecheck --leak-check=yes ./application
    ```

    The preceding command is equivalent to:

    ```shell
    mssanitizer -t racecheck -t memcheck --leak-check=yes ./application
    ```

- If no check function is specified, memory check is enabled by default.

    ```shell
    mssanitizer ./application
    ```

    The preceding command is equivalent to:

    ```shell
    mssanitizer -t memcheck ./application
    ```

### 7.4 Output File Description

| Result File| Description|
| --- | --- |
| mssanitizer_{TIMESTAMP}_{PID}.log | msSanitizer logs generated in the `mindstudio_sanitizer_log` directory during the running of the tool. `TIMESTAMP` indicates the current timestamp, and `PID` indicates the PID of the current check tool.|
| kernel.{PID}.o | Operator cache files generated during the running of the msSanitizer tool. `PID` indicates the PID of the current check tool. The operator cache files are used to parse the abnormal call stack. In normal cases, the operator cache file is automatically cleared when msSanitizer exits. When msSanitizer exits abnormally (for example, terminated by pressing **CTRL+C**), the operator cache files are retained in the file system. The operator cache file contains the debug information of the operator. You are advised to restrict the access permission of other users to the file, and delete the file as soon as possible after the check tool is executed.|
| tmp_{PID}_{TIMESTAMP} | Temporary folder generated during the running of the msSanitizer tool. `PID` indicates the PID of the current check tool, and `TIMESTAMP` indicates the current timestamp. This folder is used to generate the operator kernel binary file. In normal cases, the folder is automatically cleared when the msSanitizer exits. When the environment variable `export INJ_LOG_LEVEL=0` is used to enable the debug log function or the tool exits abnormally (for example, the tool is stopped by pressing **Ctrl+C**), the folder is retained in the file system for debugging. This folder contains the operator debug information. You are advised to restrict the access permission of other users to the folder, and delete it as soon as possible after the debugging is complete.|

## 8. Restrictions and Precautions

1. msSanitizer does not support the check of multi-thread operators and vector instructions that use masks.
2. After `--check-device-heap` or `--check-cann-heap` is enabled, the kernel will not be checked.
3. The device memory check and CANN software stack memory check cannot be enabled at the same time. If they are enabled at the same time, the error message `CANNOT enable both --check-cann-heap and --check-device-heap` is displayed.
4. Programs that are recompiled by using the API header file provided by msSanitizer can be used only for memory leak check on AscendCL APIs and cannot be used on device APIs.

## 9. Security Guidelines

1. For security and least privilege purposes, you are advised to use a common user account instead of a high-permission user account (such as `root`) to run the tools in this code repository.
2. Before using the operator development tools, ensure that the execution user's `umask` is `0027` or more restrictive. Failure to do so may result in excessively permissive permissions on the directories and files where profile data is stored.
3. Before using the tools, ensure that the least privilege principle is used. For example, the `other` user is not allowed to write data, which is often implemented by disabling permissions `666` and `777`.
4. You are not advised to configure or run custom scripts in directories of the `other` user to avoid privilege escalation.
5. You need to ensure the security of executable files or user programs.
6. Avoid high-risk operations (such as deleting files, deleting directories, changing passwords, and running privilege escalation commands) to prevent security risks.
