# msSanitizer 算子未初始化检测多场景样例介绍

## 概述

本样例演示如何使用 **mssanitizer** 工具对不同地址空间（GM / UB）上的未初始化内存读取进行检测和定位，帮助用户全面了解未初始化检测能力。

## 适用场景

- 算子运行时结果不符合预期或偶发异常值，疑似读取了未初始化的内存（脏数据）。
- 需要在不同地址空间（GM / UB）定位未初始化读取的来源。

## 支持的产品范围

- 昇腾950PR&950DT系列产品
- 昇腾A3系列产品
- 昇腾A2系列产品

## 目录结构

```text
├── sample_initcheck_multiscene/
│   ├── CMakeLists.txt              // 编译工程文件
│   ├── initcheck_multiscene.asc    // Ascend C 样例实现（含 GM/UB 两种未初始化场景注入）
│   └── README.md                   // 本说明文件
```

## 样例描述

本样例在 `initcheck_multiscene.asc` 中定义了两个独立的 kernel 函数，分别演示不同地址空间上的未初始化读取异常。

### 场景一：Global Memory（GM）未初始化读取

**核心逻辑**：正常的 Add 算子流程——将 x、y 从 GM 搬入 UB，执行加法，将结果 z 搬回 GM。

**注入异常**：在 `main()` 中通过 `aclrtMalloc` 分配 `devY` 后**不调用 `aclrtMemset`** 初始化。算子内核中 `DataCopy` 读取 `devY` 时，访问到未初始化的 GM 内存，触发告警。

```text
main() 中：
aclrtMalloc((void**)&devY, bufBytes, ...);
// devX 做了 aclrtMemset，但 devY 有意不做

kernel 中：
AscendC::DataCopy(yLocal, yGm, blockLength);  // 读取未初始化的 GM
```

### 场景二：Unified Buffer（UB）未初始化读取

**核心逻辑**：简化的 Add 算子，仅读入 x 到 UB，在 UB 上完成加法后输出。

**注入异常**：分配了 `yLocal` 的 UB 空间，但**未通过 `DataCopy` 写入任何数据**。在 `Add` 计算中直接读取未初始化的 `yLocal`，触发 UB 上的未初始化告警。

```text
kernel 中：
AscendC::DataCopy(xLocal, xGm, blockLength);   // xLocal 已初始化
// yLocal 已分配但未写入任何数据
AscendC::Add(zLocal, xLocal, yLocal, blockLength);  // 读取未初始化的 UB（yLocal）
```

> **说明**：L0{A,B,C} / 栈空间的未初始化检测原理同上。若需检测这些地址空间，可通过对应 AscendC API 分配 buffer 后不写入即读取，mssanitizer 将同样报告异常。

## 编译运行

### 环境准备

请参照官方文档完成开发环境配置：[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

### 编译算子

在 `sample_initcheck_multiscene/` 目录下执行：

```bash
mkdir -p build && cd build
cmake -DCMAKE_ASC_ARCHITECTURES=dav-2201 ..
make -j96
```

编译完成后，`build` 目录下将生成算子二进制文件 `demo`。

> **说明**：编译选项 `--npu-arch` 已通过 CMakeLists.txt 自动配置。若需检测自己的算子，请确保在编译参数中增加 `-g --cce-enable-sanitizer`，可在 CMakeLists.txt 中追加相应编译选项。

- 编译选项说明

| 选项 | 可选值 | 说明 |
|------|--------|------|
| `CMAKE_ASC_RUN_MODE` | `npu`（默认）、`cpu`、`sim` | 运行模式：NPU 运行、CPU调试、NPU仿真 |
| `CMAKE_ASC_ARCHITECTURES` | `dav-2201`（默认）、`dav-3510` | NPU 架构：dav-2201 对应昇腾A2系列产品/昇腾A3系列产品，dav-3510 对应昇腾950PR&950DT系列产品 |

> **注意：** 切换编译模式前需清理 cmake 缓存，可在 build 目录下执行 `rm CMakeCache.txt` 后重新 cmake。

### 算子运行

1. 拉起算子

    使用 mssanitizer 工具，指定 initcheck 模式拉起算子：

    ```bash
    mssanitizer -t initcheck ./demo
    ```

    > **说明**：mssanitizer命令参数含义请参考：[mssanitizer 用户指南](https://www.hiascend.com/document/detail/zh/canncommercial/900/devaids/optool/docs/zh/user_guide/mssanitizer_user_guide.md)。

    工具成功拉起算子后，将在终端输出如下日志：

    ```text
    [mssanitizer] logging to file: ./mindstudio_sanitizer_log/mssanitizer_XXX.log
    [mssanitizer] Start initcheck sanitizer on kernel "void gm_uninit_demo<256u>(float*, float*, float*, unsigned char*)"
    [mssanitizer] Start initcheck sanitizer on kernel "void ub_uninit_demo<256u>(float*, float*, unsigned char*)"
    ```

2. 检测报告分析

    工具扫描完算子后，将输出类似以下异常日志（以 GM 场景为例）：

    ```text
    ====== ERROR: uninitialized read of size 1024
    ======    at 0xXXXXXXXX on GM in "void gm_uninit_demo<256u>(float*, float*, float*, unsigned char*)"
    ======    in block aiv(0) on device 0
    ======    code in pc current 0xXXX (serialNo:X)
    ======    #0 ...initcheck_multiscene.asc:41:5
    ```

    UB 场景的报告格式相同，仅在地址空间和 kernel 名称上有所区别：
    - GM 场景：地址空间显示 `on GM`，kernel 名 `gm_uninit_demo`
    - UB 场景：地址空间显示 `on UB`，kernel 名 `ub_uninit_demo`

## 异常原理

mssanitizer 通过影子内存（Shadow Memory）追踪每个字节的初始化状态。当算子申请内存时，工具将对应影子内存标记为"未初始化"；当算子写入数据时，工具将对应影子内存标记为"已初始化"；当算子读取数据时，工具检查影子内存状态，若发现读取了未初始化字节，则报告异常。
