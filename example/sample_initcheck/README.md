# msSanitizer 算子未初始化检测样例介绍

## 概述

本样例演示如何使用 **mssanitizer** 工具对算子进行未初始化内存读取异常的检测和定位。

## 适用场景

- 算子运行时结果不符合预期，或偶发异常值，疑似读取了未初始化的内存（脏数据）。

## 支持的产品范围

- Ascend 950PR/Ascend 950DT
- Atlas A3 训练系列产品/Atlas A3 推理系列产品
- Atlas A2 训练系列产品/Atlas A2 推理系列产品

## 目录结构

```text
├── sample_initcheck/
│   ├── CMakeLists.txt          // 编译工程文件
│   ├── initcheck.asc           // Ascend C 样例实现（含未初始化读取注入）
│   └── README.md               // 本说明文件
```

## 样例描述

### 核心逻辑

本样例在 `initcheck.asc` 中实现了一个基础的搬入（DataCopy）算子，其正常执行流程为：将 Global Memory 中的数据搬入 Local Memory（`AscendC::DataCopy(localBuf, srcGlobal, bufSize)`）。

正常情况下，Global Memory 中的数据应当在搬运前已完成初始化（如通过 `aclrtMemset` 赋值）。本样例有意跳过初始化步骤，构造脏数据读取场景。

### 注入异常

在 `main()` 函数中，通过 `aclrtMalloc` 分配了一段 Global Memory，但**未调用 `aclrtMemset` 进行初始化**。算子内 `DataCopy` 直接读取该段未初始化内存：

```cpp
// main() 中：
aclrtMalloc((void**)&devBuf, bufBytes, ACL_MEM_MALLOC_HUGE_FIRST);
// 注意：此处有意不对 devBuf 调用 aclrtMemset

// 算子内 Process()：
AscendC::DataCopy(localBuf, srcGlobal, bufSize);
```

由于 devBuf 所指向的内存从未被写入任何有效数据，`DataCopy` 读取时将触发 mssanitizer 的未初始化读取告警。

## 编译运行

### 环境准备

请参照官方文档完成开发环境配置：[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

### 编译算子

在 `sample_initcheck/` 目录下执行：

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

    使用 mssanitizer 工具，指定 initcheck 模式拉起算子：

    ```bash
    mssanitizer -t initcheck ./demo
    ```

    > **说明**：mssanitizer命令参数含义请参考：[mssanitizer 用户指南](https://www.hiascend.com/document/detail/zh/canncommercial/900/devaids/optool/docs/zh/user_guide/mssanitizer_user_guide.md)。

    工具成功拉起算子后，将在终端输出如下日志：

    ```text
    [mssanitizer] logging to file: ./mindstudio_sanitizer_log/mssanitizer_XXX.log
    [mssanitizer] Start initcheck sanitizer on kernel "uninit_read_demo(unsigned char*, unsigned char*)"
    ```

2. 检测报告分析

    工具扫描完算子后，将输出以下异常日志：

    ```text
    ====== ERROR: uninitialized read of size 256
    ======    at 0x12c0c0015000 on GM in "uninit_read_demo(unsigned char*, unsigned char*)"
    ======    in block aiv(0) on device 0
    ======    code in pc current 0x6a0 (serialNo:30)
    ======    #0 /path/to/sample_initcheck/initcheck.asc:32:9
    ======    #1 /path/to/sample_initcheck/initcheck.asc:45:8
    ```

    根据诊断报告，可以确认 `initcheck.asc` 第 32 行（即 `DataCopy`）读取了未初始化的 Global Memory。修复方式为在 `main()` 中 `aclrtMalloc` 之后调用 `aclrtMemset` 对 devBuf 进行初始化：

    ```cpp
    aclrtMalloc((void**)&devBuf, bufBytes, ACL_MEM_MALLOC_HUGE_FIRST);
    aclrtMemset(devBuf, bufBytes, 0, bufBytes);  // 初始化 GM 内存
    ```

## 检测流程总结

1. 编译生成 demo 二进制文件
2. 拉起算子 `mssanitizer -t initcheck ./demo`
3. 分析诊断报告，定位源码异常行并修复

## 注意事项

- 检测自定义算子前，请确认编译选项中包含 `-g --cce-enable-sanitizer`，否则无法使用工具检测能力。
- 未初始化检测仅能发现"读取了从未被写入的内存"，如果内存曾被写入（即使是脏数据），则不会被检出。
