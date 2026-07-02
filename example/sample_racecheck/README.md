# msSanitizer 算子数据竞争检测样例介绍

## 概述

本样例演示如何使用 **mssanitizer** 工具对算子进行 RAW（Read-After-Write）数据竞争异常的检测和定位。

## 适用场景

- 多核/多 block 场景下算子运行结果不稳定、偶发错误，疑似存在数据竞争。

## 支持的产品范围

- Ascend 950PR/Ascend 950DT
- Atlas A3 训练系列产品/Atlas A3 推理系列产品
- Atlas A2 训练系列产品/Atlas A2 推理系列产品

## 目录结构

```text
├── sample_racecheck/
│   ├── CMakeLists.txt          // 编译工程文件
│   ├── racecheck.asc           // Ascend C 样例实现（含数据竞争注入）
│   └── README.md               // 本说明文件
```

## 样例描述

### 核心逻辑

本样例在 `racecheck.asc` 中启动 2 个 block。每个 block 执行以下流程：

1. V 流水（PIPE_V）向 Global Memory 写入一个标量值（`gmBuf.SetValue(blockIdx, 100.0f)`）。
2. MTE2 流水（PIPE_MTE2）从 Global Memory 读取整段数据到 Local Memory（`AscendC::DataCopy(localBuf, gmBuf, elemCnt)`）。

正常情况下，V 流水写入与 MTE2 流水读取之间应当通过 `SetFlag` / `WaitFlag` 进行同步。

### 注入异常

在 V 流水写入和 MTE2 流水读取之间**刻意省略了 SetFlag / WaitFlag 同步**：

```cpp
gmBuf.SetValue(blockIdx, (float)100.0f);        // V pipe: 写入 GM
// 缺少 SetFlag / WaitFlag 同步！
AscendC::DataCopy(localBuf, gmBuf, elemCnt);    // MTE2 pipe: 读取 GM
```

由于缺少同步，MTE2 流水可能在 V 流水写入完成之前就开始读取，导致读到旧值（RAW 竞争）。加上本样例使用了 2 个 block，block 0 的 V 写入和 block 1 的 MTE2 读取之间也构成了跨 block 的数据竞争。

## 编译运行

### 环境准备

请参照官方文档完成开发环境配置：[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

### 编译算子

在 `sample_racecheck/` 目录下执行：

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

    使用 mssanitizer 工具，指定 racecheck 模式拉起算子：

    ```bash
    mssanitizer -t racecheck ./demo
    ```

    > **说明**：mssanitizer命令参数含义请参考：[mssanitizer 用户指南](https://www.hiascend.com/document/detail/zh/canncommercial/900/devaids/optool/docs/zh/user_guide/mssanitizer_user_guide.md)。

    工具成功拉起算子后，将在终端输出如下日志：

    ```text
    [mssanitizer] logging to file: ./mindstudio_sanitizer_log/mssanitizer_XXX.log
    [mssanitizer] Start racecheck sanitizer on kernel "raw_hazard_demo(unsigned char*, unsigned char*)"
    ```

2. 检测报告分析

    工具扫描完算子后，将输出以下异常日志：

    ```text
    ====== ERROR: Potential RAW hazard detected at GM in "raw_hazard_demo(unsigned char*, unsigned char*)":
    ======    PIPE_S Write at RAW()+0x<addr> in block 0 (aiv) on device 0 at pc current 0x168 (serialNo:9)
    ======    #0 /path/to/sample_racecheck/racecheck.asc:34:15
    ======    #1 /path/to/sample_racecheck/racecheck.asc:48:8
    ======    PIPE_MTE2 Read at RAW()+0x<addr> in block 0 (aiv) on device 0 at pc current 0x700 (serialNo:11)
    ======    #0 /path/to/sample_racecheck/racecheck.asc:35:9
    ======    #1 /path/to/sample_racecheck/racecheck.asc:48:8

    ====== ERROR: Potential RAW hazard detected at GM in "raw_hazard_demo(unsigned char*, unsigned char*)":
    ======    PIPE_S Write at RAW()+0x<addr> in block 0 (aiv) on device 0 at pc current 0x168 (serialNo:9)
    ======    #0 /path/to/sample_racecheck/racecheck.asc:34:15
    ======    #1 /path/to/sample_racecheck/racecheck.asc:48:8
    ======    PIPE_MTE2 Read at RAW()+0x<addr> in block 1 (aiv) on device 0 at pc current 0x700 (serialNo:16)
    ======    #0 /path/to/sample_racecheck/racecheck.asc:35:9
    ======    #1 /path/to/sample_racecheck/racecheck.asc:48:8

    ====== ERROR: Potential WAR hazard detected at GM in "raw_hazard_demo(unsigned char*, unsigned char*)":
    ======    PIPE_MTE2 Read at WAR()+0x<addr> in block 0 (aiv) on device 0 at pc current 0x700 (serialNo:11)
    ======    #0 /path/to/sample_racecheck/racecheck.asc:35:9
    ======    #1 /path/to/sample_racecheck/racecheck.asc:48:8
    ======    PIPE_S Write at WAR()+0x<addr> in block 1 (aiv) on device 0 at pc current 0x168 (serialNo:14)
    ======    #0 /path/to/sample_racecheck/racecheck.asc:34:15
    ======    #1 /path/to/sample_racecheck/racecheck.asc:48:8
    ```

    根据诊断报告，可以确认：
    - **RAW 竞争**：`racecheck.asc` 第 34 行（PIPE_S 写入 `SetValue`）与第 35 行（PIPE_MTE2 读取 `DataCopy`）之间缺少同步，且跨 block（block 0 vs block 1）存在同样的 RAW 竞争。
    - **WAR 竞争**：block 0 的 PIPE_MTE2 读取（第 35 行）与 block 1 的 PIPE_S 写入（第 34 行）之间构成 WAR（Write-After-Read）竞争。

    修复方式为在 V 流水写入与 MTE2 流水读取之间插入同步指令：

    ```cpp
    gmBuf.SetValue(blockIdx, (float)100.0f);
    AscendC::SetFlag<AscendC::HardEvent::V_MTE2>(EVENT_ID0);   // 添加同步
    AscendC::WaitFlag<AscendC::HardEvent::V_MTE2>(EVENT_ID0);  // 添加同步
    AscendC::DataCopy(localBuf, gmBuf, elemCnt);
    ```

## 检测流程总结

1. 编译生成 demo 二进制文件
2. 拉起算子 `mssanitizer -t racecheck ./demo`
3. 分析诊断报告，定位源码异常行并修复

## 注意事项

- 检测自定义算子前，请确认编译选项中包含 `-g --cce-enable-sanitizer`，否则无法使用工具检测能力。
- 数据竞争检测依赖实际运行时的内存访问顺序，检测结果可能因运行时序不同而有所差异。建议多次运行以增强检测覆盖率。
- 开启跨卡竞争检测需额外指定 `--check-cross-npu-races=yes` 参数。
