# msSanitizer 典型数据竞争检测样例

## 概述

本样例构造 WAW（写后写）、WAR（写后读）和 RAW（读后写）三类数据竞争，并分别演示流水间、核间和卡间竞争的检测与定位方法。样例中的竞争均为教学用途的故意注入，不能直接用于生产算子。

## 支持范围

- 流水间与核间场景需要 1 个 mssanitizer 支持的 NPU。
- 卡间场景需要至少 2 个支持 P2P 访问的 NPU，并要求 CANN 提供 `aclrtMemP2PMap` 接口。
- `dav-2201` 适用于昇腾A2系列产品/昇腾A3系列产品，`dav-3510` 适用于昇腾950PR&950DT系列产品。

## 目录结构

```text
sample_racecheck_advanced/
├── CMakeLists.txt
├── README.md
├── racecheck_advanced.asc
└── test_sample.sh
```

## 场景设计

算子将 GM 划分为三个互不重叠的区域，每个区域分别承载一种竞争：

| 区域 | 访问 1 | 访问 2 | 预期类型 |
| --- | --- | --- | --- |
| `WAW` | 写 | 写 | WAW |
| `RAW` | 写 | 读 | RAW |
| `WAR` | 读 | 写 | WAR |

### 流水间竞争

`pipeline_hazards` 在同一 Vector 核内使用 PIPE_S、PIPE_MTE2 和
PIPE_MTE3 访问上述区域，并故意省略流水同步。`pipeline_raw_hazard`
额外提供稳定的 PIPE_S 写后 PIPE_MTE2 读场景。

### 核间竞争

`cross_core_hazards` 启动两个 block。block 0 和 block 1 对同一组 GM
区域执行互相冲突的读写操作，且没有核间同步或地址分片，因此可检出
WAW、WAR 和 RAW。

### 卡间竞争

主程序在 device 0 上申请 GM，通过 `aclrtMemP2PMap` 将同一地址映射到
device 1，并在两个 Device 上分别启动同名 `cross_npu_hazards` kernel。
两端使用 MSTX 将该区间标记为可读、可写和多设备共享，使 mssanitizer
能够对同一真实 P2P 内存的访问进行卡间比较。

卡间场景会先通过 `aclrtDeviceCanAccessPeer` 检查能力，再双向调用
`aclrtDeviceEnablePeerAccess`。任一能力检查、映射、MSTX 注入或 kernel
执行失败时，样例返回非零状态。

## 编译

进入本目录后执行：

```bash
source /usr/local/Ascend/ascend-toolkit/set_env.sh
cmake -S . -B build -DCMAKE_ASC_ARCHITECTURES=dav-2201
cmake --build build -j
```

若 MSTX 头文件不在 CANN 默认搜索路径，可显式指定其上级 include 目录：

```bash
cmake -S . -B build \
  -DCMAKE_ASC_ARCHITECTURES=dav-2201 \
  -DMSTX_INCLUDE_DIR=/path/to/mstx/include
```

编译参数已包含 `-g --cce-enable-sanitizer`，生成的可执行文件为 `build/demo`。

## 执行与结果分析

### 流水间场景

```bash
mssanitizer --tool=racecheck ./build/demo pipeline
```

结果中应同时出现：

```text
Potential WAW hazard detected
Potential WAR hazard detected
Potential RAW hazard detected
```

报告中的 PIPE 名称用于判断冲突访问来自哪条流水，调用栈指向 `PipelineHazards` 中缺少同步的读写语句。

### 核间场景

```bash
mssanitizer --tool=racecheck ./build/demo cross-core
```

结果应同时包含 WAW、WAR、RAW，并在冲突访问中出现 `block 0` 和 `block 1`。这说明两个核访问了重叠 GM，而不是单核内部的流水问题。

### 卡间场景

卡间检测默认关闭，必须显式开启：

```bash
mssanitizer --tool=racecheck --check-cross-npu-races=yes ./build/demo cross-npu
```

结果应同时包含 WAW、WAR、RAW；每条竞争的两个访问分别来自 `device 0` 和 `device 1`，末尾显示：

```text
Cross npu racecheck finished. See all detected errors above.
```

## 自动复验

脚本会重新编译样例、依次执行三个场景，并校验竞争类型、block、Device、跨卡汇总结论以及程序是否异常退出：

```bash
bash test_sample.sh
```

日志默认保存在 `test_output/`，也可传入其他目录：

```bash
bash test_sample.sh /tmp/racecheck_advanced_logs
```

全部检查通过时，脚本输出：

```text
[PASS] pipeline: WAW/WAR/RAW 检测结果符合预期
[PASS] cross-core: WAW/WAR/RAW 检测结果符合预期
[PASS] cross-npu: WAW/WAR/RAW 检测结果符合预期
[PASS] sample_racecheck_advanced 全部场景验证通过
```

## 定位与修复思路

- 流水间竞争：在存在依赖的流水间补充正确的 `SetFlag` / `WaitFlag`，或改写为具有明确依赖关系的 API 组合。
- 核间竞争：按 block 划分互不重叠的 GM 区间；确需共享时，应增加符合算法语义的核间同步。
- 卡间竞争：为共享数据建立明确的跨 Device 同步或所有权转移协议，确保冲突访问之间存在先后关系；不再共享时及时解除共享资源。

修复后使用相同命令复验，对应竞争报告应消失，同时还需验证算子结果正确。

## 注意事项

- 卡间比较只处理标记为 `MSTX_MEM_PERMISSIONS_REGION_FLAGS_SHARED` 的区域，漏标记会导致卡间竞争不参与检查。
- `--check-cross-npu-races=yes` 只影响卡间检查；流水间和核间检查仍由普通 racecheck 完成。
- 检测报告中的地址和 PC 会随运行环境变化，应以竞争类型、Device、block、PIPE 和源码调用栈为判断依据。
