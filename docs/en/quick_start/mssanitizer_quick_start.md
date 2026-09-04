# msSanitizer Quick Start

<br>

## 1. Overview

msSanitizer is an exception check tool based on Ascend AI Processors. It provides memory check, race check, uninitialization check, and synchronization check in single-operator development scenarios. This document demonstrates the core functions of msSanitizer based on the simple addition operator developed in the introductory tutorial. It helps beginners intuitively experience the efficiency and convenience the tool brings to the operator development process.

This document assumes that you have completed all operations in <a href="https://gitcode.com/Ascend/msot/blob/26.1.0/docs/en/quick_start/op_tool_quick_start.md" target="_blank">Ascend Operator Development Toolchain Quick Start</a>. If you have not done so, complete that guide first for a better learning experience.

## 2. Procedure

### 2.1 [Environment] Preparing the Mandatory Environment (Prerequisite ⚠️)

🛑 **This section is a mandatory prerequisite! Skipping it will cause many failures in the subsequent operations.**  
This tutorial **supports only** standard CANN container environments. It is not compatible with bare-metal servers, virtual machines, or other non-standard container deployments.

#### 2.1.1 Installing the CANN Container Environment

✅ **Strictly follow the following guide to complete the environment installation:**  
👉 **<a href="https://gitcode.com/Ascend/msot/blob/26.1.0/docs/en/quick_start/installation_guide.md" target="_blank">Ascend AI Operator Development Toolchain Learning Environment Installation Guide</a>**

> ⏱️ **Estimated time in an environment with external network access: about 3 minutes**  
> After installation, you will get a standard container environment with all operator tools, sample code, and dependent libraries preinstalled.

#### 2.1.2 Running the Environment Self-Check Script (Must Pass!)

Before the hands-on experience, **copy the entire script**, paste it into the terminal, and run it. You can continue only when all output displays [PASS]:

```bash
# 1. Check the container environment
[ -f /.dockerenv ] && [ -n "$ASCEND_HOME_PATH" ] && [ -n "$ATB_HOME_PATH" ] && echo -e "\033[32m[PASS] CANN container environment OK \033[0m" || echo -e "\033[31m[FAIL] Non-standard container or not in the container\033[0m"
# 2. Check the sample code repository
[ -d ~/ot_demo/msot/example/quick_start ] && echo -e "\033[32m[PASS] Sample code repository OK\033[0m" || echo -e "\033[31m[FAIL] Code repository missing\033[0m"
```

### 2.2 [Prerequisite] Completing Operator Project Preparation

Follow Section 2.3 in <a href="https://gitcode.com/Ascend/msot/blob/26.1.0/docs/en/quick_start/op_tool_quick_start.md#23-developing-and-building-the-operator-project-msopgen" target="_blank">Ascend Operator Development Toolchain Quick Start</a> to complete the operator project preparation.

### 2.3 [Check] msSanitizer

The msSanitizer tool is used to detect serious runtime defects such as memory overwriting, race conditions, uninitialized variables, and synchronization exceptions, helping developers efficiently identify hidden runtime errors. You are advised to follow the operations first to experience the effect. You can read the principles later.

#### 2.3.1 Modifying Compilation Options

To enable the check capabilities, you need to insert the sanitizer compilation options to the first line of the `CMakeLists.txt` file on the kernel and inject the check stub code.

```bash
cd ~/ot_demo/workspace/src/AddCustom
\cp -f op_kernel/CMakeLists.txt op_kernel/CMakeLists.txt.bak
printf '%s\n' "if(COMMAND add_ops_compile_options)" "  add_ops_compile_options(ALL OPTIONS -sanitizer)" "elseif(COMMAND npu_op_kernel_options)" "  npu_op_kernel_options(ascendc_kernels ALL OPTIONS -sanitizer)" "endif()" | cat - op_kernel/CMakeLists.txt > tmp && mv -f tmp op_kernel/CMakeLists.txt;
```

#### 2.3.2 Constructing a Memory Overwriting Error

Modify the CopyOut function in `op_kernel/add_custom.cpp` as follows (double the length of the copied `DataCopy` memory to trigger an illegal read):

```diff
- AscendC::DataCopy(zGm[progress * this->tileLength], zLocal, this->tileLength);
+ AscendC::DataCopy(zGm[progress * this->tileLength], zLocal, 2 * this->tileLength);
```

#### 2.3.3 Rebuilding and Deploying

```bash
bash ./build.sh
MY_OP_PKG=$(find ./build_out -maxdepth 1 -name "custom_opp_*.run" | head -1) && bash $MY_OP_PKG
```

> [!NOTE]
> 
> If the following warning appears during redeployment, the environment variable `ASCEND_CUSTOM_OPP_PATH` is set to an incorrect value or contains multiple colon-separated paths:
>
> ```text
> [ERROR] environment variable ASCEND_CUSTOM_OPP_PATH=/home/gitcode/samples/operator/ascendc/0_introduction/12_matmulleakyrelu_frameworklaunch/CustomOp/build_out/test/vendors/customize: is set and has multiple path in it (colon inside), which will cause the custom op installed incorrectly. Please use the --install-path option to specify an installation path instead.
> ```
>
> In this case, delete the environment variable manually and then redeploy:
>
> ```bash
> unset ASCEND_CUSTOM_OPP_PATH
> ```

#### 2.3.4 Performing a Memory Check

```bash
cd ~/ot_demo/workspace/src/caller
mssanitizer --tool=memcheck -- bash run.sh
```

The tool outputs an error report similar to the following:

```text
====== WARNING: out of bounds of size 256
======    at 0x12c0c0026000 on GM when writing data in AddCustom_ab1b6750d7f510985325b603cb06dc8b_0
======    in block aiv(1) on device 0
======    code in pc current 0x1e28 (serialNo:87)
======    #0 /usr/local/Ascend/ascend-toolkit/8.3.RC1/aarch64-linux/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:124:9
======    #1 /usr/local/Ascend/ascend-toolkit/8.3.RC1/aarch64-linux/tikcpp/tikcfw/impl/kernel_operator_data_copy_intf_impl.h:204:9
======    #2 /usr/local/Ascend/ascend-toolkit/8.3.RC1/aarch64-linux/tikcpp/tikcfw/impl/kernel_operator_data_copy_intf_impl.h:573:5
======    #3 /root/ot_demo/workspace/src/AddCustom/op_kernel/add_custom.cpp:128:10
======    #4 /root/ot_demo/workspace/src/AddCustom/op_kernel/add_custom.cpp:63:14
======    #5 /root/ot_demo/workspace/src/AddCustom/op_kernel/add_custom.cpp:169:9
```

#### 2.3.5 Performing a Race Check

```bash
cd ~/ot_demo/workspace/src/caller
mssanitizer --tool=racecheck -- bash run.sh
```

The tool outputs the following error report:

```text
====== ERROR: Potential WAR hazard detected at UB in AddCustom_ab1b6750d7f510985325b603cb06dc8b_0 on device 0:
======    PIPE_MTE3 Read at WAR()+0x400 in block 0 (aiv) on device 0 at pc current 0x1e28 (serialNo:31)
======    #0 /usr/local/Ascend/ascend-toolkit/8.3.RC1/aarch64-linux/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:124:9
======    #1 /usr/local/Ascend/ascend-toolkit/8.3.RC1/aarch64-linux/tikcpp/tikcfw/impl/kernel_operator_data_copy_intf_impl.h:204:9
======    #2 /usr/local/Ascend/ascend-toolkit/8.3.RC1/aarch64-linux/tikcpp/tikcfw/impl/kernel_operator_data_copy_intf_impl.h:573:5
======    #3 /root/ot_demo/workspace/src/AddCustom/op_kernel/add_custom.cpp:128:10
======    #4 /root/ot_demo/workspace/src/AddCustom/op_kernel/add_custom.cpp:63:14
======    #5 /root/ot_demo/workspace/src/AddCustom/op_kernel/add_custom.cpp:169:9
```

#### 2.3.6 Performing an Uninitialization Check

```bash
cd ~/ot_demo/workspace/src/caller
mssanitizer --tool=initcheck -- bash run.sh
```

The tool outputs the following error report:

```text
====== ERROR: uninitialized read of size 256
======    at 0x400 on UB in AddCustom_ab1b6750d7f510985325b603cb06dc8b_0
======    in block aiv(0-7) on device 3
======    code in pc current 0x1e34 (serialNo:241)
======    #0 /usr/local/Ascend/cann-8.5.0/aarch64-linux/asc/impl/basic_api/dav_c220/kernel_operator_data_copy_impl.h:124:9
======    #1 /usr/local/Ascend/cann-8.5.0/aarch64-linux/asc/impl/basic_api/kernel_operator_data_copy_intf_impl.h:265:9
======    #2 /usr/local/Ascend/cann-8.5.0/aarch64-linux/asc/impl/basic_api/kernel_operator_data_copy_intf_impl.h:736:5
======    #3 /root/ot_demo/workspace/src/AddCustom/op_kernel/add_custom.cpp:126:9
======    #4 /root/ot_demo/workspace/src/AddCustom/op_kernel/add_custom.cpp:63:13
======    #5 /root/ot_demo/workspace/src/AddCustom/op_kernel/add_custom.cpp:167:8
```

#### 2.3.7 Restoring Modified Files

Run the following commands:

```bash
cd ~/ot_demo/workspace/src/AddCustom
\cp -f ~/ot_demo/msot/example/quick_start/msopgen/code/op_kernel/add_custom.cpp ~/ot_demo/workspace/src/AddCustom/op_kernel/
\cp -f op_kernel/CMakeLists.txt.bak op_kernel/CMakeLists.txt
```
