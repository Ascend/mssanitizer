# msSanitizer 算子内存越界检测样例介绍

## 概述

本样例演示如何使用 **mssanitizer** 工具对算子进行 UB（Unified Buffer）缓冲区越界写入异常的检测和定位。

## 适用场景

- 算子运行时出现异常值、崩溃或结果不稳定，疑似存在越界内存访问。

## 支持的产品范围

- Ascend 950PR/Ascend 950DT
- Atlas A3 训练系列产品/Atlas A3 推理系列产品
- Atlas A2 训练系列产品/Atlas A2 推理系列产品

## 目录结构

```text
├── sample_memcheck/
│   ├── CMakeLists.txt          // 编译工程文件
│   ├── memcheck.asc            // Ascend C 样例实现（含越界写入注入）
│   └── README.md               // 本说明文件
```

## 样例描述

### 核心逻辑

本样例在 `memcheck.asc` 中实现了一个基础的搬入（DataCopy）算子。正常流程为：从 Global Memory 搬入数据到 Local Memory（`AscendC::DataCopy(localBuf, srcGlobal, elemCnt)`），搬运量应与 UB 缓冲区分配大小一致。

### 注入异常

在算子中分配了一个仅能容纳 `allocCnt`（64）个 float 元素的 UB 缓冲区，但 `DataCopy` 却尝试搬运 `elemCnt`（256）个元素：

```cpp
// 分配 64 个 float 的 UB 空间（不足）
AscendC::LocalTensor<float> localBuf = ubAlloc.Alloc<float, allocCnt>();

// 试图搬运 256 个 float 到仅 64 个 float 的 UB 缓冲区 → 越界写入！
AscendC::DataCopy(localBuf, srcGlobal, elemCnt);
```

由于 `elemCnt`（256）远大于 UB 实际分配量 `allocCnt`（64），超出部分将写入未分配/他人使用的 UB 空间，触发内存越界告警。

## 编译运行

### 环境准备

请参照官方文档完成开发环境配置：[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

### 编译算子

在 `sample_memcheck/` 目录下执行：

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
| `CMAKE_ASC_ARCHITECTURES` | `dav-2201`（默认）、`dav-3510` | NPU 架构：dav-2201 对应 Atlas A2/A3 系列产品，dav-3510 对应 Ascend 950PR/Ascend 950DT |

> **注意：** 切换编译模式前需清理 cmake 缓存，可在 build 目录下执行 `rm CMakeCache.txt` 后重新 cmake。

### 算子运行

1. 拉起算子

    使用 mssanitizer 工具，指定 memcheck 模式拉起算子（memcheck 为默认模式，可省略 `-t` 参数）：

    ```bash
    mssanitizer -t memcheck ./demo
    ```

    > **说明**：mssanitizer命令参数含义请参考：[mssanitizer 用户指南](https://www.hiascend.com/document/detail/zh/canncommercial/900/devaids/optool/docs/zh/user_guide/mssanitizer_user_guide.md)。

    工具成功拉起算子后，将在终端输出如下日志：

    ```text
    [mssanitizer] logging to file: ./mindstudio_sanitizer_log/mssanitizer_XXX.log
    [mssanitizer] Start memcheck sanitizer on kernel "bounds_write_demo(unsigned char*, unsigned char*)"
    ```

2. 检测报告分析

    工具扫描完算子后，将输出以下异常日志：

    ```text
    ====== ERROR: illegal write of size 768
    ======    at 0x0 on UB in "bounds_write_demo(unsigned char*, unsigned char*)"
    ======    in block aiv(0) on device 0
    ======    code in pc current 0x65c (serialNo:12)
    ======    #0 /path/to/sample_memcheck/memcheck.asc:33:9
    ======    #1 /path/to/sample_memcheck/memcheck.asc:47:8
    ```

    768 字节恰好等于 `(256 - 64) × sizeof(float)`，即超出 UB 分配范围的溢出量。根据诊断报告，可以确认 `memcheck.asc` 第 33 行（即 `DataCopy`）向 UB 缓冲区写入了超出其分配范围的数据。修复方式为将 UB 分配大小调整为与实际搬运量一致：

    ```cpp
    // 修复：分配足够大小的 UB 缓冲区
    AscendC::LocalTensor<float> localBuf = ubAlloc.Alloc<float, elemCnt>();
    AscendC::DataCopy(localBuf, srcGlobal, elemCnt);
    ```

## 检测流程总结

1. 编译生成 demo 二进制文件
2. 拉起算子 `mssanitizer -t memcheck ./demo`
3. 分析诊断报告，定位源码异常行并修复

## 注意事项

- 检测自定义算子前，请确认编译选项中包含 `-g --cce-enable-sanitizer`，否则无法使用工具检测能力。
- 内存检测为默认检测模式，运行 `mssanitizer ./demo` 等价于 `mssanitizer -t memcheck ./demo`。
- 可通过 `--leak-check=yes` 额外开启内存泄漏检测，`--check-unused-memory=yes` 开启分配未使用检测。
