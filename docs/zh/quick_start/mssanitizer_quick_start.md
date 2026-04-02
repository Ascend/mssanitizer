# msSanitizer算子检测工具快速入门

<br>

## 1. 概述

msSanitizer工具是基于昇腾AI处理器的异常检测工具，包含单算子开发场景下的内存检测、竞争检测、未初始化检测和同步检测四个子功能。
本文档基于入门教程中开发的简易加法算子，演示 msSanitizer 工具的核心功能，帮助初学者直观感受其为算子开发过程带来的高效与便捷。
<br>

### 1.1 建议

本章节以您已完成<a href="https://gitcode.com/Ascend/msot/blob/master/docs/zh/quick_start/op_tool_quick_start.md" target="_blank">《昇腾算子开发工具链快速入门》</a>的全流程操作为前提；若尚未体验，建议先完成该指南以获得更佳的学习效果。

### 1.2 环境准备

请严格按照<a href="https://gitcode.com/Ascend/msot/blob/master/docs/zh/quick_start/installation_guide.md" target="_blank">《昇腾 AI 算子开发工具链学习环境安装指南》</a>完成环境安装与工作区配置。
即使您已具备类似环境，也需按该指南重新执行一遍，以确保所有依赖组件、环境变量等完整且一致。

## 2. 操作步骤

### 2.1 【环境】运行环境预检

#### 2.1.1 确认 Python 依赖包已安装

执行以下命令，若输出"All is OK"，则表明所需 Python 包及其版本均满足规范：

```shell
python3 -c "import numpy, sympy, scipy, attrs, psutil, decorator; from packaging import version; assert version.parse(numpy.__version__) <= version.parse('1.26.4'); print('All is OK')"
```

若报错，请参照[第 1.2 节](#12-环境准备)进行正确安装。

### 2.2 【前提】算子工程准备完成

按照<a href="https://gitcode.com/Ascend/msot/blob/master/docs/zh/quick_start/op_tool_quick_start.md" target="_blank">《昇腾算子开发工具链快速入门》</a>中的指导，完成 2.1 节和 2.3 节。

### 2.3 【检测】算子异常检测（msSanitizer）

msSanitizer 工具用于检测内存越界、竞争条件、未初始化变量及同步异常等严重运行时缺陷，帮助开发者高效定位隐蔽的运行时错误。建议先跟随操作体验效果，原理部分可稍后阅读。

#### 2.3.1 修改编译选项

为启用检测能力，需在 Kernel 侧的 CMakeLists.txt 首行插入 sanitizer 编译选项，注入检测桩代码：

```shell
cd ~/ot_demo/workspace/src/AddCustom
\cp -f op_kernel/CMakeLists.txt op_kernel/CMakeLists.txt.orig.bak
sed -i "1i\\add_ops_compile_options(ALL OPTIONS -sanitizer)" op_kernel/CMakeLists.txt
```

#### 2.3.2 构造内存越界错误

修改 op_kernel/add_custom.cpp 中的 CopyOut 函数，具体修改如下（将DataCopy内存拷贝长度从TILE_LENGTH改为2 * TILE_LENGTH）：

```diff
- AscendC::DataCopy(zGm[progress * this->tileLength], zLocal, this->tileLength);
+ AscendC::DataCopy(zGm[progress * this->tileLength], zLocal, 2 * this->tileLength);
```

#### 2.3.3 重新编译部署

```shell
bash ./build.sh
MY_OP_PKG=$(find ./build_out -maxdepth 1 -name "custom_opp_*.run" | head -1) && bash $MY_OP_PKG
```

#### 2.3.4 执行内存检测

```shell
cd ~/ot_demo/workspace/src/caller
mssanitizer --tool=memcheck bash run.sh
```

工具输出类似如下错误报告：

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

#### 2.3.5 执行竞争检测

```shell
cd ~/ot_demo/workspace/src/caller
mssanitizer --tool=racecheck bash run.sh
```

工具输出如下错误报告：

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

#### 2.3.6 未初始化检测

```shell
cd ~/ot_demo/workspace/src/caller
mssanitizer --tool=initcheck bash run.sh
```

工具输出如下错误报告：

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

#### 2.3.7 恢复被修改的文件

执行如下命令：

```shell
cd ~/ot_demo/workspace/src/AddCustom
\cp -f op_kernel/CMakeLists.txt.orig.bak op_kernel/CMakeLists.txt
```
