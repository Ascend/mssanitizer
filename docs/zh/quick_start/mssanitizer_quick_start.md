# msSanitizer 算子检测工具快速入门

<br>

## 1. 概述

msSanitizer 工具是基于昇腾 AI 处理器的异常检测工具，包含单算子开发场景下的内存检测、竞争检测、未初始化检测和同步检测四个子功能。本文档基于入门教程中开发的简易加法算子，演示 msSanitizer 工具的核心功能，帮助初学者直观感受其为算子开发过程带来的高效与便捷。

本章节以您已完成<a href="https://gitcode.com/Ascend/msot/blob/master/docs/zh/quick_start/op_tool_quick_start.md" target="_blank">《算子开发工具链快速入门》</a>的全流程操作为前提；若尚未体验，建议先完成该指南以获得更佳的学习效果。

## 2. 操作步骤

### 2.1【环境】必备环境准备（强制前置 ⚠️）

🛑 **本节为强制前置步骤！跳过将导致后续操作大量出现失败。**  
本教程**仅支持**标准化 CANN 容器环境，不兼容裸机、虚拟机或其他非标准容器部署。

#### 2.1.1 安装 CANN 容器环境

✅ **请严格按以下指南完成环境安装：**  
👉 **<a href="https://gitcode.com/Ascend/msot/blob/master/docs/zh/quick_start/installation_guide.md" target="_blank">《昇腾 AI 算子开发工具链学习环境安装指南》</a>**

> ⏱️ **外网可达环境下预计耗时：约 3 分钟**  
> 安装完成后，您将获得一个预装所有算子工具、示例代码和依赖库的标准化容器环境。

#### 2.1.2 执行环境自检脚本（必须通过！）

在如下正式体验前，请**全文复制下方整段脚本**，粘贴到终端执行，只有输出全部显示为 [PASS] 才能继续：

```bash
# 1. 容器环境检查
[ -f /.dockerenv ] && [ -n "$ASCEND_HOME_PATH" ] && [ -n "$ATB_HOME_PATH" ] && echo -e "\033[32m[PASS] CANN 容器环境 OK \033[0m" || echo -e "\033[31m[FAIL] 非标容器或未进入容器！\033[0m"
# 2. 示例代码仓检查
[ -d ~/ot_demo/msot/example/quick_start ] && echo -e "\033[32m[PASS] 示例代码仓 OK\033[0m" || echo -e "\033[31m[FAIL] 代码仓缺失\033[0m"
```

### 2.2【前提】算子工程准备完成

按照<a href="https://gitcode.com/Ascend/msot/blob/master/docs/zh/quick_start/op_tool_quick_start.md#23开发构建算子工程msopgen" target="_blank">《算子开发工具链快速入门》</a>中 2.3 节操作，完成算子工程准备。

### 2.3【检测】算子异常检测（msSanitizer）

msSanitizer 工具用于检测内存越界、竞争条件、未初始化变量及同步异常等严重运行时缺陷，帮助开发者高效定位隐蔽的运行时错误。建议先跟随操作体验效果，原理部分可稍后阅读。

#### 2.3.1 修改编译选项

为启用检测能力，需在 Kernel 侧的 CMakeLists.txt 首行插入 sanitizer 编译选项，注入检测桩代码：

```bash
cd ~/ot_demo/workspace/src/AddCustom
\cp -f op_kernel/CMakeLists.txt op_kernel/CMakeLists.txt.bak
printf '%s\n' "if(COMMAND add_ops_compile_options)" "  add_ops_compile_options(ALL OPTIONS -sanitizer)" "elseif(COMMAND npu_op_kernel_options)" "  npu_op_kernel_options(ascendc_kernels ALL OPTIONS -sanitizer)" "endif()" | cat - op_kernel/CMakeLists.txt > tmp && mv -f tmp op_kernel/CMakeLists.txt;
```

#### 2.3.2 构造内存越界错误

修改 op_kernel/add_custom.cpp 中的 CopyOut 函数，具体修改如下（将 DataCopy 内存拷贝长度增加一倍，触发“非法读取”）：

```diff
- AscendC::DataCopy(zGm[progress * this->tileLength], zLocal, this->tileLength);
+ AscendC::DataCopy(zGm[progress * this->tileLength], zLocal, 2 * this->tileLength);
```

#### 2.3.3 重新编译部署

```bash
bash ./build.sh
MY_OP_PKG=$(find ./build_out -maxdepth 1 -name "custom_opp_*.run" | head -1) && bash $MY_OP_PKG
```

> [!NOTE]说明
> 如果重新部署时出现如下告警，则说明环境变量 ASCEND_CUSTOM_OPP_PATH 值不正确或包含多个以冒号间隔的路径：
>
> ```text
> [ERROR] environment variable ASCEND_CUSTOM_OPP_PATH=/home/gitcode/samples/operator/ascendc/0_introduction/12_matmulleakyrelu_frameworklaunch/CustomOp/build_out/test/vendors/customize: is set and has multiple path in it (colon inside), which will cause the custom op installed incorrectly. Please use the --install-path option to specify an installation path instead.
> ```
>
> 此时需要手动删除此环境变量再重新部署：
>
> ```bash
> unset ASCEND_CUSTOM_OPP_PATH
> ```

#### 2.3.4 执行内存检测

```bash
cd ~/ot_demo/workspace/src/caller
mssanitizer --tool=memcheck -- bash run.sh
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

```bash
cd ~/ot_demo/workspace/src/caller
mssanitizer --tool=racecheck -- bash run.sh
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

```bash
cd ~/ot_demo/workspace/src/caller
mssanitizer --tool=initcheck -- bash run.sh
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

```bash
cd ~/ot_demo/workspace/src/AddCustom
\cp -f ~/ot_demo/msot/example/quick_start/msopgen/code/op_kernel/add_custom.cpp ~/ot_demo/workspace/src/AddCustom/op_kernel/
\cp -f op_kernel/CMakeLists.txt.bak op_kernel/CMakeLists.txt
```
