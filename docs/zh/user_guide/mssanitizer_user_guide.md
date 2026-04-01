# MindStudio Sanitizer 使用指南

<br>

## 1. 功能概述

MindStudio Sanitizer（异常检测工具，msSanitizer）是一种基于AI处理器的工具，包含了单算子开发场景下的内存检测、竞争检测、未初始化检测和同步检测四个子功能：

| 功能类型     | 描述                                                                                               |
| ---------- |--------------------------------------------------------------------------------------------------|
| 内存检测   | 工具可以在用户开发算子的过程中，协助定位非法读写、多核踩踏、非对齐访问、内存泄漏以及非法释放等内存问题。<br>同时工具也支持对CANN软件栈的内存检测，帮助用户定界软件栈内存异常发生的模块。 |
| 竞争检测   | 工具可以协助用户定位由于竞争风险可能导致的数据竞争问题，包含核内竞争和核间竞争问题。其中，核内竞争包含流水间竞争和流水内竞争。                                  |
| 未初始化检测 | 工具可以协助用户定位由于内存未初始化可能导致的脏数据读取问题。                                                                  |
| 同步检测   | 工具可以协助用户定位由于前序算子中的未配对同步指令导致的后续算子同步失败的问题。                                                         |

---

## 2. 一分钟速查

### 四种检测工具速览

| 检测工具 | 解决的问题 | 快速启用命令 |
| --- | --- | --- |
| `memcheck`（默认） | 内存越界、多核踩踏、非对齐访问、内存泄漏、非法释放 | `mssanitizer --tool=memcheck ./application` |
| `racecheck` | 数据竞争（WAW / WAR / RAW） | `mssanitizer --tool=racecheck ./application` |
| `initcheck` | 读取未初始化内存导致的脏数据 | `mssanitizer --tool=initcheck ./application` |
| `synccheck` | SetFlag / WaitFlag 未配对导致的同步失败 | `mssanitizer --tool=synccheck ./application` |

### 使用场景选择指引

| 我的开发场景 | 参考章节 |
| --- | --- |
| 通过 `<<<>>>` 直接调用 Kernel 函数 | [4.1 Kernel直调场景](#41-kernel直调场景) |
| 调用 `aclnn` 系列单算子 API | [4.2 单算子API调用场景](#42-单算子api调用场景) |
| PyTorch 框架接入算子（TorchAir 图模式等） | [4.3 PyTorch框架适配场景](#43-pytorch框架适配场景) |
| Triton-Ascend 算子 | [4.4 Triton 算子调用场景](#44-triton-算子调用场景) |

### 最常用参数速查

| 参数 | 作用 | 示例 |
| --- | --- | --- |
| `--tool` / `-t` | 指定检测子工具（默认 memcheck） | `-t racecheck` |
| `--leak-check=yes` | 开启内存泄漏检测 | `--leak-check=yes` |
| `--check-unused-memory=yes` | 开启分配内存未使用检测 | `--check-unused-memory=yes` |
| `--log-file` | 将报告输出到文件 | `--log-file=result.log` |
| `--kernel-name` | 只检测指定名称的算子（支持模糊匹配） | `--kernel-name="add"` |
| `--block-id` | 只检测指定 block（单 block 调试模式） | `--block-id=0` |
| `--full-backtrace=yes` | 显示 AscendC API 内的完整调用栈 | `--full-backtrace=yes` |

> [!NOTE] 说明
> 完整参数说明请参见 [7.2 参数列表](#72-参数列表)。

---

## 3. 使用流程

使用 msSanitizer 检测算子异常的标准流程如下：

1. **选择适用场景**：根据您的开发环境确定运行方式，参见 [4. 适用场景](#4-适用场景)。
2. **配置编译选项（可选）**：如需全量检测和调用栈信息，需重新编译算子，参见 [5. 算子编译选项配置（可选）](#5-算子编译选项配置可选)。
3. **运行检测**：使用 `mssanitizer` 命令启动检测，参见 [6. 检测功能详解](#6-检测功能详解) 及 [7. 命令与参数参考](#7-命令与参数参考)。
4. **解读报告**：根据控制台输出的异常报告定位并修复问题，参见各子章节中的"异常报告解读"。

> [!NOTE] 说明
> 建议先运行 **memcheck**（内存检测），确认算子程序无内存异常后，再按需运行 racecheck / initcheck / synccheck。

---

## 4. 适用场景

### 4.1 Kernel直调场景

如果您通过`<<<>>>`直接调用 Kernel 函数进行测试时，类似如下运行检测工具：

```shell
mssanitizer --tool=memcheck ./add_npu
```

> [!NOTE] 说明
> 在`<<<>>>`自定义算子接入torch场景时，默认使用内存池的方式管理GM内存，可能会导致越界检测结果不准确。因此，在检测前需要额外设置如下环境变量关闭内存池，从而获得更精确的检测结果。
>
> ```shell
> export PYTORCH_NO_NPU_MEMORY_CACHING=1
> ```

### 4.2 单算子API调用场景

1. 单算子API调用(aclnn)场景，使用检测工具拉起算子API运行脚本，例如：

    ```shell
    mssanitizer --tool=memcheck bash run.sh
    ```

2. 在调用含有aclnn前缀的API时，需执行以下命令，通过aclInit接口传入acl.json文件以保证内存检测的准确性。

    ```c
    auto ret = aclInit("./acl.json"); // acl.json文件内容为{"dump":{"dump_scene":"lite_exception"}}
    ```

### 4.3 PyTorch框架适配场景

1. PyTorch图模式（TorchAir）下，仅支持在msSanitizer工具不添加编译选项的情况下进行检测，具体请参见[5. 算子编译选项配置（可选）](#5-算子编译选项配置可选)。
2. PyTorch图模式（TorchAir）下，支持Ascend IR图执行模式和aclgraph图执行模式，具体请参见《[Ascend Extension for PyTorch](https://www.hiascend.com/document/detail/zh/Pytorch/720/modthirdparty/torchairuseguide/torchair_00015.html)》中"PyTorch图模式使用(TorchAir)>reduce-overhead模式功能>reduce-overhead模式配置"章节。
3. PyTorch框架调用场景，详细信息可参考《[Ascend Extension for PyTorch](https://www.hiascend.com/document/detail/zh/Pytorch/720/ptmoddevg/Frameworkfeatures/featuresguide_00021.html)》中"PyTorch框架特性指南>自定义算子适配开发>基于OpPlugin算子适配开发"章节。具体操作请参见《[example](../best_practices/basic_cases.md)》中的"检测PyTorch接口调用的算子"。

### 4.4 Triton 算子调用场景

#### 前置条件

- 已完成 Triton 及 Triton-Ascend 插件的安装与配置，具体操作请参见 [Triton-Ascend 项目仓库](https://gitcode.com/Ascend/triton-ascend)。
- **不支持 Atlas 推理系列产品**。

#### 环境变量配置

为确保检测准确性并避免缓存干扰，需在运行前设置以下环境变量：

| 环境变量 | 说明 |
| --- | --- |
| `TRITON_ALWAYS_COMPILE=1` | 强制重新编译 Triton 算子，避免使用缓存的旧版本 |
| `PYTORCH_NO_NPU_MEMORY_CACHING=1` | 禁用 PyTorch 的 NPU 内存池机制，防止其干扰内存检测结果 |

> [!NOTE] 说明
> Triton 场景会使用 PyTorch 创建 Tensor，PyTorch 框架内默认以内存池的方式管理 GM 内存，会对内存检测产生干扰，因此必须关闭内存缓存以保证检测的有效性。

详细使用示例可参考《[基础场景案例](../best_practices/basic_cases.md)》中的"检测 Triton 算子"章节。

## 5. 算子编译选项配置（可选）

用户可根据需求自行选择是否修改编译选项重新编译算子，两种场景的详细说明如下。

### 5.1 不修改编译选项（快速定界）

- **指令检测范围：** 与GM相关的搬运指令。
- **异常检测范围：** 仅支持内存检测中的非法读写和非对齐访问；异常报告中不显示调用栈信息。
- **适用场景：** 该场景支持的功能有限，仅适用于对算子内存异常中的非法读写和非对齐访问异常的快速定界。

> [!WARNING] 注意
>
> - 该场景算子的优化等级需为O2，并保证算子链接阶段增加-q选项，保留符号重定位信息，否则会导致检测功能失效。
> - 该场景不适用于Atlas 推理系列产品。
> - 该场景仅适用于算子内核调用符场景。

### 5.2 修改编译选项（全量检测）

- **指令检测范围：** 全量指令。
- **异常检测范围：** 支持全量检测；在编译选项中增加-g选项后，异常报告将会显示调用栈信息。
- **适用场景：** 通过不添加编译选项的功能快速定位异常算子后，再添加编译选项对异常算子进行全量检测。

如需开启全量检测，请参见《[开启全量检测](./compile_option_config.md)》修改编译选项后重新编译算子。

---

## 6. 检测功能详解

> [!NOTE] 异常报告级别说明
>
> 异常报告具有以下级别：
>
> - **WARNING**：此级别被定义为不确定性的风险，可能出现的异常现象由实际情况决定，如多核踩踏、内存分配未使用等。其中，多核踩踏风险涉及多个核对同一块内存的操作，高阶用户可以通过核间同步的手段来规避此风险，初级用户遇到此类异常，应该将其视为危险源。目前，多核踩踏的WARNING级别的报告仅能识别atomic类的核间同步信息。
> - **ERROR**：最高严重级别的异常，涉及针对内存操作的确定性错误，如非法读写、内存泄漏、非对齐访问、内存未初始化、竞争异常等。强烈建议用户检查此严重级别的异常。

### 6.1 内存检测

内存检测是针对用户程序运行时的一种异常检测，该工具可以检测并报告算子运行中对外部存储（Global Memory）和内部存储（Local Memory）的越界及未对齐等内存访问异常。

#### 6.1.1 内存异常类型

内存检测能够检测并报告诸如内存非法读写、多核踩踏、非对齐访问、内存泄漏、非法释放及分配内存未使用等异常操作，如下表所示。

**表 1**  内存异常类型

| 异常名 | 描述 | 位置 | 支持地址空间 |
| --- | --- | --- | --- |
| [非法读写](#6131-非法读写) | 由于访问了未分配的内存导致的异常。 | Kernel、Host | GM、UB、L0{A,B,C}、L1 |
| [多核踩踏](#6132-多核踩踏) | AI Core核心访问了重叠的内存导致的踩踏问题。 | Kernel | GM |
| [非对齐访问](#6133-非对齐访问) | DMA（负责在Global Memory和Local Memory之间搬运数据）搬运的地址与内存的最小访问粒度未对齐导致的异常。 | Kernel | GM、UB、L0{A,B,C}、L1 |
| [非法释放](#6135-非法释放) | 对未分配或已释放的地址进行释放导致的异常。 | Host | GM |
| [内存泄漏](#6134-内存泄漏) | 申请内存使用后未释放，导致程序在运行过程中内存占用持续增加的异常。 | Host | GM |
| [分配内存未使用](#6136-分配内存未使用) | 对内存分配后未使用导致的异常。 | Kernel、Host | GM |
| [昇腾950代际产品simt单元异常信息](#6137-昇腾950代际产品simt单元异常信息) | SIMT架构下展示异常信息发生的线程位置。 | Kernel | GM |
| [线程间踩踏](#6138-线程间踩踏) | AI Core核心内线程间访问了重叠的内存导致的踩踏问题。 | Kernel | GM |
| [寄存器告警](#6139-寄存器告警) | 寄存器未回归默认值时，告警提示用户有寄存器值残留。 | Kernel | GM |

#### 6.1.2 启用内存检测

运行msSanitizer工具时，默认启用内存检测功能（**memcheck**）。其中application为用户程序。

- 执行如下命令可显式指定内存检测，默认会开启非法读写、多核踩踏、非对齐访问和非法释放的检测功能：

    ```shell
    mssanitizer --tool=memcheck application
    ```

- 执行如下命令，可在memcheck检测功能项的基础上，手动启用内存泄漏的检测功能：

    ```shell
    mssanitizer --tool=memcheck --leak-check=yes application
    ```

- 执行如下命令，可在memcheck检测功能项的基础上，手动启用分配内存未使用的检测功能：

    ```shell
    mssanitizer --tool=memcheck --check-unused-memory=yes application
    ```

> [!NOTE] 说明
>
> - 当用户程序运行完成后，界面将会打印异常报告。
> - 当用户使用PyTorch等框架接入算子时，框架内部可能会通过内存池管理GM内存，而内存池通常会一次性分配大量GM内存，并在运行过程中复用。此时，若用户对算子进行检测并记录GM上所有内存分配和释放的信息，会因为内存池的内存管理方式导致检测信息不准确。因此检测工具提供了手动上报GM内存分配信息的接口，方便用户在算子调用时手动上报该算子应当使用的GM内存范围，详细接口介绍请参见《[MindStudio Sanitizer对外接口使用说明](../api_reference/mssanitizer_api_reference.md)》中的sanitizerReportMalloc和sanitizerReportFree接口。
> - msSanitizer工具也支持对Atlas A2 训练系列产品/Atlas A2 推理系列产品的AllReduce、AllGather、ReduceScatter、AlltoAll接口及Atlas A3 训练系列产品/Atlas A3 推理系列产品的AllGather、ReduceScatter、AlltoAllV接口进行非法读写的检测，具体介绍请参见《[Ascend C算子开发接口](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/API/ascendcopapi/atlasascendc_api_07_0869.html)》中的"高阶API > Hccl >  Hccl Kernel侧接口"章节。
> - msSanitizer工具也支持对通算融合类算子的非法读写检测。
> - 当前昇腾950代际产品支持内存检测中GM/UB/L1/L0A/L0B/L0C，其他暂不支持。

#### 6.1.3 内存异常报告解读

内存检测异常报告会输出多种不同类型的异常信息，以下将对一些简单的异常信息示例进行说明，帮助用户解读异常报告中的信息。

##### 6.1.3.1 非法读写

    非法读写异常信息的产生是由于算子程序中，通过读或写的方式访问了一块未分配的内存。此错误一般发生在GM或片上内存上，GM异常是由于GM分配的大小与实际算子程序中访问的范围不一致导致，而片上内存的异常是由于算子程序的访问范围超过硬件容量上限导致。

    ```text
    ====== ERROR: illegal read of size 224  // 异常的基本信息,包含非法读写的类型以及被非法访问的字节数,非法读写包括read(非法读取)和write(非法写入)
    ======    at 0x12c0c0015000 on GM in add_custom_kernel  // 异常发生的内存位置信息，包含发生的核函数名、地址空间与内存地址，此处的内存地址指一次内存访问中的首地址
    ======    in block aiv(0) on device 0  // 异常代码对应Vector核的block索引
    ======    code in pc current 0x77c (serialNo:10) // 当前异常发生的pc指针和调用api行为的序列号
    ======    #0 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:58:9  // 以下为异常发生代码的调用栈，包含文件名、行号和列号
    ======    #1 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:58:9
    ======    #2 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:443:5
    ======    #3 illegal_read_and_write/add_custom.cpp:18:5
    ```

    以上示例中，对GM上的"0x12c0c0015000"地址存在非法读取，且导致异常发生的指令对应于算子实现文件add_custom.cpp的第18行。

    > [!NOTE] 说明
    > 不添加编译选项的情况下，异常报告将不会出现以下调用栈信息：
    >
    > ```text
    > ======    #0 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:58:9  // 以下为异常发生代码的调用栈，包含文件名、行号和列号
    > ======    #1 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:58:9
    > ======    #2 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:443:5
    > ======    #3 illegal_read_and_write/add_custom.cpp:18:5
    > ```

##### 6.1.3.2 多核踩踏

    AI Core是AI处理器中的计算核心，AI处理器内部有多个AI Core，算子运行就在这些AI Core上。这些AI Core会在计算过程中从GM上搬入或搬出数据。当没有显式地进行核间同步时，如果各个核之间访问的GM内存存在重叠并且至少有一个核对重叠地址进行写入时，则会发生多核踩踏问题。这里我们通过所有者的概念来保证多核之间不会发生踩踏问题，当一块内存被某一个核写入后，这块内存就由该核所有。当其他核对这块内存进行访问时就会产生out of bounds异常。

    ```text
    ====== WARNING: out of bounds of size 256  // 异常的基本信息，包含发生踩踏的字节数
    ======    at 0x12c0c00150fc on GM when writing data in add_custom_kernel  // 异常发生的内存位置信息，包含发生的核函数名、地址空间与内存地址，此处的内存地址指一次内存访问中的首地址
    ======    in block aiv(9) on device 0  // 异常代码对应Vector核的block索引
    ======    code in pc current 0x7b8 (serialNo:22)  // 当前异常发生的pc指针和调用api行为的序列号
    ======    #0 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:103:9  // 以下为异常发生代码的调用栈，包含文件名、行号和列号
    ======    #1 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:155:9
    ======    #2 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:461:5
    ======    #3 out_of_bound/add_custom.cpp:21:5
    ```

    以上示例中，共有256个字节的访问发生踩踏，对GM上的"0x12c0c00150fc"地址进行访问时存在多核踩踏，且导致异常发生的指令对应于算子实现文件add_custom.cpp的第21行。

##### 6.1.3.3 非对齐访问

    昇腾处理器上包含多种类型的内存，当通过DMA进行访问时，不同类型的内存在不同处理器上有不同的最小访问粒度。当访问的内存地址与最小访问粒度不对齐时，会发生数据异常或AI Core异常等问题。访问对齐检测可以在对齐问题发生时输出对齐异常信息。

    ```text
    ====== ERROR: misaligned access of size 13  // 异常的基本信息，包含发生对齐异常操作的字节数
    ======    at 0x6 on UB in add_custom_kernel   // 异常发生的内存位置信息，包含发生的核函数名、地址空间与内存地址
    ======    in block aiv(0) on device 0  // 异常代码对应Vector核的block索引
    ======    code in pc current 0x780 (serialNo:33)  // 当前异常发生的pc指针和调用api行为的序列号
    ======    #0 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:103:9  // 以下为异常发生代码的调用栈，包含文件名、行号和列号
    ======    #1 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:155:9
    ======    #2 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:461:5
    ======    #3 illegal_align/add_custom.cpp:18:5
    ```

    以上示例中，共有针对13个字节的对齐异常访问，对UB上的"0x6"地址进行访问时存在对齐问题，且导致异常发生的指令对应于算子实现文件add_custom.cpp的第18行。

    > [!NOTE] 说明
    > 不添加编译选项的情况下，异常报告将不会出现以下调用栈信息：
    >
    > ```text
    > ======    #0 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:103:9  // 以下为异常发生代码的调用栈，包含文件名、行号和列号
    > ======    #1 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:155:9
    > ======    #2 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:461:5
    > ======    #3 illegal_align/add_custom.cpp:18:5
    > ```

##### 6.1.3.4 内存泄漏

    内存检测可以检测出Device侧的内存泄漏问题，这些问题通常是开发者没有正确释放使用AscendCL接口申请的内存导致的，由于内部存储（Local Memory）目前不存在内存分配的概念，因此内存泄漏只可能出现在GM上。通过指定命令行参数"--leak-check=yes"可以开启内存泄漏检测。

    ```text
    ====== ERROR: LeakCheck: detected memory leaks     // 检测到内存泄漏
    ======    Direct leak of 100 byte(s)      // 具体每次的内存泄漏信息
    ======      at 0x124080013000 on GM allocated in add_custom.cpp:14 (serialNo:37)
    ======    Direct leak of 1000 byte(s)
    ======      at 0x124080014000 on GM allocated in add_custom.cpp:15 (serialNo:55)
    ====== SUMMARY: 1100 byte(s) leaked in 2 allocation(s)     // 全部内存泄漏的总结，包括发生泄漏的次数以及总共泄漏了多少字节等信息
    ```

    以上示例中，第一个内存泄漏信息包含了地址空间、内存地址、内存长度以及代码定位信息，代码定位信息指向具体分配这块内存的调用所在的文件名和行号。

##### 6.1.3.5 非法释放

    非法释放是指对一个未分配的地址或者已释放的地址进行了释放操作，一般发生在GM上。

    ```text
    ====== ERROR: illegal free()     // 异常的基本信息，表明发生了非法释放异常
    ======    at 0x124080013000 on GM      // 异常发生的内存位置信息，包含发生的地址空间与内存地址
    ======    code in add_custom.cpp:84 (serialNo:63)    // 异常发生的代码定位信息,包含文件名、行号和调用api行为的序列号
    ```

    以上示例中，对GM上的"0x124080013000"地址进行了非法释放，且导致异常发生的指令对应于算子实现文件add_custom.cpp的第84行。

##### 6.1.3.6 分配内存未使用

    分配内存未使用是指算子运行时申请了内存，但直到算子运行完成，都没有使用该内存。该异常场景一般是算子使用了错误的内存或算子逻辑存在问题，一般发生在GM上。

    ```text
    ====== WARNING: Unused memory of 1000 byte(s)     //异常的基本信息，表明检测到内存分配未使用异常
    ======    at 1240c0016000 on GM                    // 异常发生的内存位置信息,包含发生的地址空间与内存地址
    ======    code in add_custom.cpp:2 (serialNo:69)   //异常发生的代码定位信息,包含文件名、行号和调用api行为的序列号
    ====== SUMMARY: 1100byte(s) unused memory in 2 allocation(s) // 内存分配未使用的总结信息，包括未使用内存块的个数及字节等信息
    ```

##### 6.1.3.7 昇腾950代际产品simt单元异常信息

    SIMT架构下异常信息的展示会额外提供异常信息发生的线程位置，线程id从0开始计数，例如下方异常信息发生在线程idX=1 idY=0，idZ=0处。昇腾950代际产品SIMT单元异常时，错误信息展示如下：

    ```text
    ====== ERROR: illegal read of size 4
    ======    at 0x300000018ffc on GM in vec_add
    ======    by thread (1,0,0) in block aiv(0-1) on device 0
    ======    code in pc current 0x178 (serialNo:16)
    ======    #0 ${ASCEND_HOME_PATH}/illegal_read_and_write_simt_gm_float/kernel.cpp:16:21
    ```

##### 6.1.3.8 线程间踩踏

    SIMT架构下多线程编程时，如果各线程对GM的访问未正确处理，可能导致多个线程同时向同一内存地址写入数据，从而引发线程间"内存踩踏"问题。线程间踩踏的检测机制与"多核踩踏"类似：假设某线程首次写入某块内存后，该内存即被视为由该线程独占。若其他线程尝试对该内存位置进行写操作，则会触发out of bounds异常。

    ```text
    ====== WARNING: out of bounds of size 4
    ======    at 0x300000056000 on GM when writing data in vec_add
    ======    by thread (1,0,0) in block aiv(0) on device 0
    ======    code in pc current 0xd8 (serialNo:16)
    ======    #0 vec_add_simt.cpp:20:12
    ```

##### 6.1.3.9 寄存器告警

    出现寄存器未复位默认值的情况时，通过告警提示用户。告警显示未重置寄存器的名称、所在核编号、算子名称、期望的默认值和当前实际值。

    ```text
    [mssanitizer]Warning:Register XXX was not reset to default in block aiv(XXX) on kernel XXX. Expected default value is (XXX), but current value is (XXX)
    ```

    > [!NOTE] 说明
    > 当前仅内存检测支持寄存器告警。

### 6.2 竞争检测

竞争检测用于解决在并行计算环境中内存访问竞争的问题。在昇腾处理器架构下，外部存储和内部存储通常被用作临时缓冲区保存正在处理的数据，外部存储或内部存储可以同时被多个流水访问，外部存储还可以被多个核访问，算子程序若没有正确处理核间、流水间或流水内的同步，就可能会导致数据竞争的问题。

#### 6.2.1 竞争异常类型

内存竞争是指两个内存事件（其中至少有一个为写事件）尝试访问同一块内存时，出现不符合基于预期执行顺序的结果。这种异常会导致数据竞争，从而使程序的运行或输出取决于内存事件的实际执行顺序。竞争检测功能可识别以下三种典型的内存竞争：

**表 1**  内存竞争类型

| 异常名 | 描述 | 位置 | 支持地址空间 |
| --- | --- | --- | --- |
| Write-After-Write(WAW) | 当两个内存事件尝试向同一块内存写入时，可能存在这种异常，导致内存结果值取决于两个内存事件的实际访问顺序。 | Kernel | GM、UB、L0{A,B,C}、L1 |
| Write-After-Read(WAR) | 当两个内存事件（一个事件执行读取操作，另一个事件执行写入操作）尝试访问同一块内存时，可能存在这种异常，即写操作事件实际在读操作事件之前执行完毕，并导致读取到的内存值并非预期起始值。 | Kernel | GM、UB、L0{A,B,C}、L1 |
| Read-After-Write(RAW) | 当两个内存事件（一个事件执行读取操作，另一个事件执行写入操作）尝试访问同一块内存时，可能存在这种异常，即读操作事件实际在写操作事件之前执行完毕，并导致读取到的内存值还未更新。 | Kernel | GM、UB、L0{A,B,C}、L1 |

当竞争检测识别出异常，用户就可以修改程序以确保该异常不再存在。在出现先写后读或先读后写的情况下，会根据serialNo大小顺序确定先后顺序，serialNo小的在PIPE_S上先执行。

#### 6.2.2 启用竞争检测

运行msSanitizer工具时，执行如下命令，启用竞争检测功能（racecheck）。

```shell
mssanitizer --tool=racecheck application    // application为用户程序
```

> [!NOTE] 说明
>
> - 竞争检测不会执行内存错误检查，建议用户先运行内存检测，确保算子程序能够正常执行，没有运行异常。
> - 当用户程序运行完成后，界面将会打印异常报告。
> - 启动工具后，将会在当前目录下自动生成工具运行日志文件mssanitizer__{TIMESTAMP}__{_PID_}.log。

#### 6.2.3 竞争异常报告解读

竞争检测会输出一系列信息，详细说明有关算子各PIPE之间存在的内存数据竞争访问风险。

- 以下示例中表示了AICore 0的Vector核内部中存在对UB的先写后读竞争风险，PIPE_MTE2流水中存在对"0x0"地址的写入操作事件，该操作对应算子实现文件add_custom.cpp中的第17行，PIPE_MTE3流水中存在对"0x0"地址的读取操作事件，该操作对应算子实现文件add_custom.cpp中的第22行。

    ```text
    ====== ERROR: Potential RAW hazard detected at GM in kernel_float on device 0:  // 竞争事件类型、异常内存块信息、竞争发生的核函数名
    ======    PIPE_MTE2 Write at RAW()+0x0 in block 0 (aiv) on device 0 at pc current 0xa98 (serialNo:14)  // 竞争事件的详细信息,包含该事件所在的PIPE、操作类型、内存访问起始地址、核类型、AICore信息以及代码执行的pc指针和调用api行为的序列号
    ======    #0 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:58:9  // 以下为异常发生代码的调用栈，包含文件名、行号和列号
    ======    #1 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:58:9
    ======    #2 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:443:5
    ======    #3 Racecheck/add_custom.cpp:17:5
    ======    PIPE_MTE3 Read at RAW()+0x0 in block 0 (aiv) on device 0 at pc current 0xad4 (serialNo:17)
    ======    #0 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:103:9
    ======    #1 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:155:9
    ======    #2 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:461:5
    ======    #3 Racecheck/add_custom.cpp:22:5
    ```

- 以下示例表示了Thread(0,0,0)和Thread(0,0,1)两个线程在kernel.cpp的第88行发生了写写竞争，均往0核的0x0 UB地址上写入数据。该竞争检测仅支持GM或者UB空间中的内存竞争。

    ```text
    ====== ERROR: Potential WAW hazard detected at UB in vec_add_5 on device 0:
    ======    Write Thread(0,0,0) at WAW()+0x0 in block 0(aiv) on device 0 at pc current 0xcc8
    ======    #0 racecheck/simt/inner_core/ub_error/kernel.cpp:88:13
    ======    Write Thread(0,0,1) at WAW()+0x0 in block 0(aiv) on device 0 at pc current 0xcc8
    ======    #0 racecheck/simt/inner_core/ub_error/kernel.cpp:88:13
    ```

### 6.3 未初始化检测

未初始化检测功能是一种重要的内存安全保护机制，旨在识别并防止由于使用未初始化的变量而导致的内存异常。

#### 6.3.1 未初始化异常类型

**表 1**  未初始化异常类型

| 异常名 | 描述 | 位置 | 支持地址空间 |
| --- | --- | --- | --- |
| 未初始化 | 内存申请后为未初始化状态，未对内存进行写入，直接读取未初始化的值导致的异常。 | Kernel、Host | GM、UB、L1、L0{ABC}、栈空间 |

#### 6.3.2 启用未初始化检测

运行msSanitizer工具时，执行如下命令，启用未初始化检测功能（**initcheck**）。

```shell
mssanitizer --tool=initcheck application   // application为用户程序
```

> [!NOTE] 说明
>
> - 启动工具后，将会在当前目录下自动生成工具运行日志文件mssanitizer__{TIMESTAMP}__{_PID_}.log。
> - 由于硬件限制，某些指令仅支持以Block形式进行数据搬运。当参与计算的实际数据量不是Block大小的整数倍时，可能会不可避免地带入部分无效数据（即"脏数据"），这可能导致工具报告初始化异常，用户需自行判断这些"脏数据"是否会影响计算结果。
> - 未初始化检测不支持昇腾950代际产品。

#### 6.3.3 未初始化异常报告解读

未初始化检测异常报告会输出多种不同类型的异常信息，以下将对一些简单的异常信息示例进行说明，帮助用户解读异常报告中的信息。

未初始化的异常场景一般是算子读取了已申请但未初始化的内存，发生在GM、UB、L1、L0{ABC}、栈空间上。

```text
====== ERROR: uninitialized read of size 224  // 异常的基本信息，包含读取的未初始化字节数
======    at 0x12c0c0015000 on GM in add_custom_kernel  // 异常发生的内存位置信息，包含发生的核函数名、地址空间与内存地址，此处的内存地址指一次内存访问中的首地址
======    in block aiv(0) on device 0  // 异常代码对应Vector核的block索引
======    code in pc current 0x77c (serialNo:10) // 当前异常发生的pc指针和调用api行为的序列号
======    #0 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/impl/dav_c220/kernel_operator_data_copy_impl.h:58:9  // 以下为异常发生代码的调用栈，包含文件名、行号和列号
======    #1 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:58:9
======    #2 ${ASCEND_HOME_PATH}/compiler/tikcpp/tikcfw/inner_interface/inner_kernel_operator_data_copy_intf.cppm:443:5
======    #3 uninitialized_read/add_custom.cpp:18:5
```

### 6.4 同步检测

在Ascend C算子开发过程中，必须成对使用SetFlag和WaitFlag，同步检测功能用于找出算子中未配对的SetFlag指令。

若存在多余SetFlag指令，不会直接导致当前算子的竞争问题，却会改变硬件计数器的状态，进而可能导致后续算子的同步指令配对错误。若这些后续算子本身不存在竞争，竞争检测也不会报错，但前序算子的计数器变化可能导致实际竞争情况的发生，通过同步检测功能，能够有效识别前序算子中的多余SetFlag指令问题，避免后续算子受影响。

如果存在多余的SetFlag指令，当有两个及两个以上多余的SetFlag指令存在时，同步检测将同时触发匹配异常和冗余异常。

> [!NOTE] 说明
>
> - 同步检测单独启用时不会执行内存检测和竞争检测，因此建议用户先使用内存检测和竞争检测，若竞争检测无异常报告，但算子存在竞争现象时，再考虑使用同步检测对前序算子进行检查。
> - 若存在多余WaitFlag指令，将会导致当前算子的后续指令被阻塞，从而出现算子运行停滞的现象。此时，开发者无需工具提示，便可自行发现问题。
> - 同步检测不支持昇腾950代际产品。

#### 6.4.1 同步异常类型

**表 1**  同步异常类型

| 异常名 | 描述 | 位置 |
| --- | --- | --- |
| 同步检测 | 算子中存在未配对的SetFlag同步指令时，虽然对当前算子的功能没有直接影响，却会引发计数器状态错误。可能会扰乱后续算子的同步指令配对，进而影响后续算子的计算精度。 | Kernel |
| 冗余检测 | 当用户编写了两个参数完全相同的set_flag（即pipe和eventId完全一致），且两者之间未对目标pipe执行任何操作时，将导致后续算子同步指令出现全量不配对，进而引发异常问题。 | Kernel |

#### 6.4.2 启用同步检测

运行msSanitizer工具时，执行如下命令，启用同步检测功能（synccheck）。

```shell
mssanitizer --tool=synccheck application   // application为用户程序
```

> [!NOTE] 说明
>
> - 启动工具后，将会在当前目录下自动生成工具运行日志文件mssanitizer__{TIMESTAMP}__{_PID_}.log。
> - 当用户程序运行完成后，界面将会打印异常报告。

#### 6.4.3 同步异常报告解读

同步检测异常报告会依次列出每个算子中未配对的SetFlag指令的相关信息，包括源流水和目标流水以及具体位置。

```text
====== WARNING: Unpaired set_flag instructions detected  // 提示检出未配对的set_flag指令
======    from PIPE_S to PIPE_MTE3 in kernel  // 标识从PIPE_S到PIPE_MTE3的同步，PIPE_MTE3等待PIPE_S
======    in block aiv(0) on device 1  // 异常代码对应Vector核的block索引和设备号，此处为0核1卡
======    code in pc current 0x2c94 (serialNo:31) // 当前异常发生的pc指针和调用api行为的序列号
======    #0 /home/Ascend/compiler/tikcpp/tikcfw/impl/kernel_event.h:785:13  // 以下为异常发生代码的调用栈，包含文件名、行号和列号
======    #1 /home/Ascend/compiler/tikcpp/tikcfw/interface/kernel_common.h:150:5
======    #2 /home/test/ascendc_test_syncall/kernel.cpp:26:9
```

---

## 7. 命令与参数参考

### 7.1 命令格式

可以通过运行以下命令来调用msSanitizer工具：

```shell
mssanitizer [<options>] [--] <user_program> [<user_options>]
```

各部分配置说明如下：

1. options：为检测工具的命令行选项，详细的参数选项及其默认值，请参考 [7.2 参数列表](#72-参数列表)。
2. user_program：为用户算子程序。
3. user_options：为用户程序的命令行选项。
4. --：如要加载的程序本身带有命令行参数时，在程序之前使用"--"分隔检测工具和用户程序，例如：`mssanitizer -- application parameter1 parameter2 ...`

### 7.2 参数列表

**表1：通用参数说明**

| 参数名称 | 参数描述 | 参数取值 | 必选 |
| --- | --- | --- | --- |
| -v，--version | 查询msSanitizer工具版本。 | - | 否 |
| -t，--tool | 指定异常检测的子工具。 | memcheck：内存检测（默认）<br>racecheck：竞争检测<br>initcheck：未初始化检测<br>synccheck：同步检测 | 否 |
| --log-file | 指定检测报告输出到文件。 | {file_name}，如配置为test_log。<br>说明：<br>仅支持数字、大小写字母和- . / _四种符号。<br>为避免日志泄漏风险，建议限制该文件权限，确保只有授权人员才能访问该文件。<br>工具会以覆盖的方式将报告输出到test_log文件。若test_log文件中已有内容，这些内容将会被清空。因此，建议指定一个空文件用于输出报告。 | 否 |
| --log-level | 指定检测报告输出等级。 | info：输出info/warn/error级别的运行信息。<br>warn：输出warn/error级别的运行信息（默认）。<br>error：输出error级别的运行信息。 | 否 |
| --max-debuglog-size | 指定检测工具调试输出日志中单个文件大小的上限。 | 可设定范围为1~10240之间的整数，单位为MB。<br>默认值为1024。<br>说明：<br>--max-debuglog-size=100就表示单个调试日志的大小上限为100MB。 | 否 |
| --block-id | 是否启用单block检测功能。 | 可设定范围为0~200之间的整数。<br>启用前<br>内存检测、未初始化检测和同步检测：默认检测所有block。<br>竞争检测：核间默认检测所有block，核内默认检测block 0的流水内及流水间的竞争。<br>启用后<br>内存检测、未初始化检测和同步检测：检测指定block。<br>竞争检测：核间不进行检测，检测指定block的流水内及流水间的竞争。 | 否 |
| --cache-size | 表示单block的GM内存大小。 | 单block可设定范围为1~8192之间的整数，单位为MB。<br>单block默认值为100MB，表示单block可申请100MB的内存大小。<br>说明：<br>启用单block检测时，--cache-size的最大值为8192MB。不启用单block检测时，--cache-size可设置的最大值为(24*1024 / block数量) 。<br>当--cache-size值不满足需求时，异常检测工具将会打印信息提示用户重新设置--cache-size值，具体请参见《MindStudio Sanitizier常见问题》中的msSanitizer工具提示--cache-size异常。 | 否 |
| --kernel-name | 指定要检测的算子名称。 | 支持使用算子名中的部分字符串来进行模糊匹配。如果不指定，则系统默认会对整个程序执行期间所调度的所有算子进行检测。<br>例如，需要同时检测名为"abcd"和"bcd"的算子时，可以通过配置--kernel-name="bc"来实现这一需求，系统会自动识别并检测所有包含"bc"字符串的算子。 | 否 |
| --full-backtrace | 显示 AscendC API 内的调用栈回溯。 | yes：显示完整的调用栈回溯。<br>no（默认）：不显示 AscendC API 内的调用栈。 | 否 |
| --demangle | 设置输出中函数名显示的 demangle 模式。 | full（默认）：显示完整的 demangle 后的函数名。<br>simple：仅显示函数名，不包含返回值、参数列表。<br>no：显示未 demangle 的函数名。 | 否 |
| -h，--help | 输出帮助信息。 | - | 否 |

**表2：内存检测参数说明**

| 参数名称 | 参数描述 | 参数取值 | 必选 |
| --- | --- | --- | --- |
| --check-unused-memory | 使能分配内存未使用检测。 | yes/no（默认） | 否 |
| --leak-check | 使能内存泄漏检测。 | yes/no（默认） | 否 |
| --check-device-heap | 使能Device侧内存检测。 | yes/no（默认） | 否 |
| --check-cann-heap | 使能CANN软件栈内存检测。 | yes/no（默认） | 否 |

> [!NOTE] 说明
>
> - --check-device-heap或--check-cann-heap使能后，将不会对Kernel内进行检测。
> - Device侧内存检测和CANN软件栈内存检测不能同时使能，若同时使能会提示"CANNOT enable both --check-cann-heap and --check-device-heap"。
> - 使用msSanitizer工具提供的API头文件重新编译的待检测程序只能用于Ascend CL系列接口的泄漏检测，无法用于Device接口的检测。

### 7.3 检测功能组合规则

异常检测工具提供内存检测（memcheck）、竞争检测（racecheck）、未初始化检测（initcheck）和同步检测（synccheck）四种检测功能，多种检测功能可以组合开启，组合启用检测功能需满足以下原则：

- 多个检测功能可通过多次指定--tool参数同时开启。如执行以下命令可同时开启内存检测和竞争检测：

    ```shell
    mssanitizer -t memcheck -t racecheck ./application
    ```

- 若开启了检测功能对应的子选项，则对应的检测功能也会被默认开启。如开启了内存检测对应的泄漏检测子选项，则内存检测功能会被自动开启：

    ```shell
    mssanitizer -t racecheck --leak-check=yes ./application
    ```

    以上命令等价于：

    ```shell
    mssanitizer -t racecheck -t memcheck --leak-check=yes ./application
    ```

- 若不指定任何检测功能，则默认启用内存检测：

    ```shell
    mssanitizer ./application
    ```

    以上命令等价于：

    ```shell
    mssanitizer -t memcheck ./application
    ```

### 7.4 输出文件说明

| 结果件名称 | 说明 |
| --- | --- |
| mssanitizer_{TIMESTAMP}_{PID}.log | msSanitizer工具运行过程中，在mindstudio_sanitizer_log目录下生成的工具日志，TIMESTAMP为当前时间戳，PID为当前检测工具的PID。 |
| kernel.{PID}.o | msSanitizer工具运行过程中，会在当前路径下生成的算子缓存文件。其中，PID为当前使用的检测工具的PID，该算子缓存文件用于解析异常调用栈。正常情况下，msSanitizer工具退出时会自动清理该算子缓存文件。当msSanitizer工具异常退出（如被用户"CTRL+C"中止）时，该算子缓存文件会保留在文件系统中。因为该算子缓存文件包含算子的调试信息，建议限制其他用户对此文件的访问权限，并在检测工具运行完成后及时删除。 |
| tmp_{PID}_{TIMESTAMP} | msSanitizer工具运行过程中，会在当前路径下生成的临时文件夹。其中，PID为当前使用的检测工具的PID，TIMESTAMP为当前时间戳，该文件夹用于生成算子Kernel二进制。正常情况下，msSanitizer工具退出时会自动清理该文件夹。当通过环境变量export INJ_LOG_LEVEL=0开启DEBUG等级日志，或工具异常退出（如被用户"CTRL+C"中止）时，该文件夹会保留在文件系统中，方便用户调测使用。因为该文件夹包含算子的调试信息，建议限制其他用户对此文件的访问权限，并在调测完成后及时删除。 |

## 8. 限制与注意事项

1. msSanitizer工具不支持对多线程算子及使用掩码的向量类计算指令的检测。
2. 启用 `--check-device-heap` 或 `--check-cann-heap` 后，将不再对 Kernel 内部进行检测。
3. Device 侧内存检测与 CANN 软件栈内存检测不可同时启用；若同时启用，将报错：“CANNOT enable both --check-cann-heap and --check-device-heap”。
4. 使用 msSanitizer 提供的 API 头文件重新编译的程序，仅适用于基于 Ascend CL 接口的内存泄漏检测，不支持 Device 接口检测。

## 9. 安全使用规范

1. 出于安全性及权限最小化角度考虑，本代码仓中的工具均不应使用root等高权限账户进行操作，建议使用普通用户权限安装执行。
2. 使用算子开发工具前，请确保执行用户的umask值大于等于0027，否则会造成获取的性能数据所在目录和文件权限过大。
3. 使用算子工具前，请保证使用最小权限原则（如：禁止other用户可写，禁止666或777）。
4. 不建议配置或运行其他用户目录下的自定义脚本，避免提权风险。
5. 用户需自行保证可执行文件或用户程序执行的安全性。
6. 不建议进行高危操作（删除文件、删除目录、修改密码及提权命令等），避免安全风险。
