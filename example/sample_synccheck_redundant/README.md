# msSanitizer 算子同步冗余检测样例介绍

## 概述

本样例演示如何使用 **mssanitizer** 工具对算子进行同步冗余异常检测和定位。当算子在同一条流水线上连续发出两条参数完全相同的同步指令（SetFlag 或 WaitFlag），且中间无任何其他操作时，第二条同步指令是冗余的。

## 适用场景

- 算子代码中出现连续重复的 SetFlag/WaitFlag 调用，可能引发后续算子同步指令全量不匹配。

## 支持的产品范围

- Ascend 950PR/Ascend 950DT
- Atlas A3 训练系列产品/Atlas A3 推理系列产品
- Atlas A2 训练系列产品/Atlas A2 推理系列产品

## 目录结构

```text
├── sample_synccheck_redundant/
│   ├── CMakeLists.txt                // 编译工程文件
│   ├── synccheck_redundant.asc       // Ascend C 样例实现（含冗余同步指令注入）
│   └── README.md                     // 本说明文件
```

## 样例描述

### 核心逻辑

本样例在 `synccheck_redundant.asc` 中基于 Add 算子实现，其正常执行流程为：

1. 将输入 x、y 从 Global Memory 搬入 UB（`AscendC::DataCopy`）。
2. 执行向量加法 `AscendC::Add(zLocal, xLocal, yLocal, blockLength)`。
3. 将结果 z 从 UB 搬回 Global Memory。

### 注入异常

在两次 DataCopy 搬运之后、PipeBarrier 之前，连续插入了两对**参数完全相同**的 `SetFlag` + `WaitFlag`：

```cpp
// 第一对：SetFlag + WaitFlag（PIPE_V -> PIPE_MTE2）
AscendC::SetFlag<AscendC::HardEvent::V_MTE2>(EVENT_ID0);
AscendC::WaitFlag<AscendC::HardEvent::V_MTE2>(EVENT_ID0);

// 第二对：参数与第一对完全相同，构成冗余
AscendC::SetFlag<AscendC::HardEvent::V_MTE2>(EVENT_ID0);
AscendC::WaitFlag<AscendC::HardEvent::V_MTE2>(EVENT_ID0);
```

第二对 SetFlag/WaitFlag 的参数（源流水 PIPE_V、目标流水 PIPE_MTE2、事件 ID EVENT_ID0）与第一对完全一致。mssanitizer 将报告 **2 条冗余 SetFlag** 和 **2 条冗余 WaitFlag**，共 4 条冗余告警。

> **说明**：冗余检测的原理是，工具在遍历同一条流水线上的同步事件时，若发现当前 SetFlag/WaitFlag 与上一条保存的 SetFlag/WaitFlag 参数完全一致，则判定为冗余。非同步事件会清除已保存的上一条指令记录。

## 编译运行

### 环境准备

请参照官方文档完成开发环境配置：[算子工具开发环境安装指导](https://gitcode.com/Ascend/msot/blob/master/docs/zh/common/dev_env_setup.md)。

### 编译算子

在 `sample_synccheck_redundant/` 目录下执行：

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
    [mssanitizer] Start synccheck sanitizer on kernel "add_redundant_demo(float*, float*, float*)"
    ```

2. 检测报告分析

    工具扫描完算子后，将输出 4 条冗余告警：

    ```text
    ====== WARNING: Redundant set_flag instructions detected
    ======    from PIPE_V to PIPE_MTE2 in "void add_redundant_demo<256u>(float*, float*, float*, unsigned char*)"
    ======    in block aiv(0) on device 0
    ======    code in pc current 0xXXX (serialNo:X)
    ======    #0 ...synccheck_redundant.asc:40:5

    ====== WARNING: Redundant set_flag instructions detected
    ======    from PIPE_V to PIPE_MTE2 in "void add_redundant_demo<256u>(float*, float*, float*, unsigned char*)"
    ======    in block aiv(0) on device 0
    ======    code in pc current 0xXXX (serialNo:X)
    ======    #0 ...synccheck_redundant.asc:45:5

    ====== WARNING: Redundant wait_flag instructions detected
    ======    from PIPE_V to PIPE_MTE2 in "void add_redundant_demo<256u>(float*, float*, float*, unsigned char*)"
    ======    in block aiv(0) on device 0
    ======    code in pc current 0xXXX (serialNo:X)
    ======    #0 ...synccheck_redundant.asc:41:5

    ====== WARNING: Redundant wait_flag instructions detected
    ======    from PIPE_V to PIPE_MTE2 in "void add_redundant_demo<256u>(float*, float*, float*, unsigned char*)"
    ======    in block aiv(0) on device 0
    ======    code in pc current 0xXXX (serialNo:X)
    ======    #0 ...synccheck_redundant.asc:46:5
    ```

    报告中：
    - 第 1 条（Redundant set_flag）：指向 line 40 的第一条 SetFlag
    - 第 2 条（Redundant set_flag）：指向 line 45 的第二条 SetFlag
    - 第 3 条（Redundant wait_flag）：指向 line 41 的第一条 WaitFlag
    - 第 4 条（Redundant wait_flag）：指向 line 46 的第二条 WaitFlag

## 异常原理

冗余同步指令的危害在于：多余的 SetFlag 会额外递增硬件事件计数器。当后续算子中的 WaitFlag 等待该事件时，由于计数器值已被前序算子的冗余 SetFlag 提前改变，可能导致 WaitFlag 匹配到错误的事件，进而引发后续算子的数据竞争问题。
