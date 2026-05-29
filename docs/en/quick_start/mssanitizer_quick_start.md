# msSanitizer Quick Start

<br>

## 1. Overview

msSanitizer is an exception check tool based on Ascend AI Processors. It provides memory check, race check, uninitialization check, and synchronization check in single-operator development scenarios.
This document demonstrates the core functions of msSanitizer based on the simple addition operator developed in the introductory tutorial. It helps beginners intuitively experience the efficiency and convenience the tool brings to the operator development process.
<br>

### 1.1 Recommendations

This document assumes that you have completed all operations in <a href="https://gitcode.com/Ascend/msot/blob/master/docs/en/quick_start/op_tool_quick_start.md" target="_blank">Ascend Operator Development Toolchain Quick Start</a>. If you have not done so, complete that guide first for a better learning experience.

### 1.2 Environment Setup

Strictly follow the <a href="https://gitcode.com/Ascend/msot/blob/master/docs/en/quick_start/installation_guide.md" target="_blank">Ascend AI Operator Development Toolchain Learning Environment Installation Guide</a> to complete the environment installation and workspace configuration.
Even if you have a similar environment, perform the steps in the guide again to ensure that all dependent components and environment variables are complete and consistent.

## 2. Procedure

### 2.1 [Environment] Pre-checking the Runtime Environment

#### 2.1.1 Verifying Installation of Python Dependencies

Run the following command. If `All is OK` is displayed, the required Python packages and their versions meet the specifications:

```shell
python3 -c "import numpy, sympy, scipy, attrs, psutil, decorator; from packaging import version; assert version.parse(numpy.__version__) <= version.parse('1.26.4'); print('All is OK')"
```

If an error occurs, refer to [Section 1.2](#12-environment-setup) for correct installation.

### 2.2 [Prerequisite] Completing Operator Project Preparation

Follow the instructions in <a href="https://gitcode.com/Ascend/msot/blob/master/docs/en/quick_start/op_tool_quick_start.md" target="_blank">Ascend Operator Development Toolchain Quick Start</a> to complete Sections 2.1 and 2.3.

### 2.3 [Check] msSanitizer

The msSanitizer tool is used to detect serious runtime defects such as memory overwriting, race conditions, uninitialized variables, and synchronization exceptions, helping developers efficiently identify hidden runtime errors. You are advised to follow the operations first to experience the effect. You can read the principles later.

#### 2.3.1 Modifying Compilation Options

To enable the check capabilities, you need to insert the sanitizer compilation options to the first line of the `CMakeLists.txt` file on the kernel and inject the check stub code.

```shell
cd ~/ot_demo/workspace/src/AddCustom
\cp -f op_kernel/CMakeLists.txt op_kernel/CMakeLists.txt.orig.bak
sed -i "1i\\add_ops_compile_options(ALL OPTIONS -sanitizer)" op_kernel/CMakeLists.txt
```

#### 2.3.2 Constructing a Memory Overwriting Error

Modify the CopyOut function in `op_kernel/add_custom.cpp` as follows (change the length of the copied `DataCopy` memory from `TILE_LENGTH` to 2 * `TILE_LENGTH`):

```diff
- AscendC::DataCopy(zGm[progress * this->tileLength], zLocal, this->tileLength);
+ AscendC::DataCopy(zGm[progress * this->tileLength], zLocal, 2 * this->tileLength);
```

#### 2.3.3 Rebuilding and Deploying

```shell
bash ./build.sh
MY_OP_PKG=$(find ./build_out -maxdepth 1 -name "custom_opp_*.run" | head -1) && bash $MY_OP_PKG
```

#### 2.3.4 Performing a Memory Check

```shell
cd ~/ot_demo/workspace/src/caller
mssanitizer --tool=memcheck bash run.sh
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

```shell
cd ~/ot_demo/workspace/src/caller
mssanitizer --tool=racecheck bash run.sh
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

```shell
cd ~/ot_demo/workspace/src/caller
mssanitizer --tool=initcheck bash run.sh
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

```shell
cd ~/ot_demo/workspace/src/AddCustom
\cp -f op_kernel/CMakeLists.txt.orig.bak op_kernel/CMakeLists.txt
```
