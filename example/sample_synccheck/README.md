# msSanitizer 算子同步检测样例介绍

## 概述

本样例演示如何使用 **mssanitizer** 工具对算子进行同步异常检测和定位。

## 适用场景

- 算子执行时出现精度问题，疑似同步指令异常。

## 支持的产品范围

- Ascend 950PR/Ascend 950DT
- Atlas A3 训练系列产品/Atlas A3 推理系列产品
- Atlas A2 训练系列产品/Atlas A2 推理系列产品

## 目录结构

```text
├── sample_synccheck/
│   ├── CMakeLists.txt          // 编译工程文件
│   ├── synccheck.asc           // Ascend C 样例实现（含同步异常注入）
│   └── README.md               // 本说明文件
```

## 样例描述

### 核心逻辑

本样例在 `synccheck.asc` 中实现了一个基础的搬入-搬出（DataCopy）算子，其正常执行流程为：

1. 将 Global Memory 中的输入数据搬入 Local Memory（`AscendC::DataCopy(localBuf, srcGlobal, bufSize)`）。
2. 将 Local Memory 中的结果数据搬回 Global Memory（`AscendC::DataCopy(dstGlobal, localBuf, bufSize)`）。

### 注入异常

在上述两步搬运操作之间，刻意插入了一条**孤立且未配对**的 `SetFlag` 指令：

```cpp
AscendC::SetFlag<AscendC::HardEvent::V_MTE2>(EVENT_ID0);
```

由于整个算子中不存在与之对应的 `WaitFlag` 调用，该 `SetFlag` 将导致同步指令不配对，引发同步异常。

## 编译运行

### 环境准备

请参照官方文档完成开发环境配置：[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

### 编译算子

在 `sample_synccheck/` 目录下执行：

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

    使用 mssanitizer 工具，指定 synccheck 模式拉起算子：

    ```bash
    mssanitizer -t synccheck ./demo
    ```

    > **说明**：mssanitizer命令参数含义请参考：[mssanitizer 用户指南](https://www.hiascend.com/document/detail/zh/canncommercial/900/devaids/optool/docs/zh/user_guide/mssanitizer_user_guide.md)。

    工具成功拉起算子后，将在终端输出如下日志：

    ```text
    [mssanitizer] logging to file: ./mindstudio_sanitizer_log/mssanitizer_XXX.log
    [mssanitizer] Start synccheck sanitizer on kernel "copy_demo(unsigned char*, unsigned char*, unsigned char*)"
    ```

2. 检测报告分析

   工具扫描完算子后，将输出以下异常日志：

    ```text
    ====== WARNING: Unpaired set_flag instructions detected
    ======    from PIPE_V to PIPE_MTE2 in "copy_demo(unsigned char*, unsigned char*, unsigned char*)"
    ======    in block aiv(0) on device 0
    ======    code in pc current 0x6e8 (serialNo:12)
    ======    #0 /path/to/sample_synccheck/synccheck.asc:36:9
    ======    #1 /path/to/sample_synccheck/synccheck.asc:53:8
    ```

    根据诊断报告，可以确认 `synccheck.asc` 第 36 行（即 `SetFlag`）存在同步异常。修复方式为删除该孤立指令，或为其添加对应的 `WaitFlag` 调用。

## 检测流程总结

1. 编译生成 demo 二进制文件
2. 拉起算子 `mssanitizer -t synccheck ./demo`
3. 分析诊断报告，定位源码异常行并修复

## 注意事项

- 检测自定义算子前，请确认编译选项中包含 `-g --cce-enable-sanitizer`，否则无法使用工具检测能力。
