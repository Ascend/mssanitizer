# msSanitizer 典型内存异常检测样例

## 概述

本样例构造多核踩踏、L1 越界写、L1 越界读、L1 非对齐访问、UB 非对齐访问、非法释放、内存泄漏和分配未使用八个典型内存异常场景，并演示使用 `mssanitizer` 复现、检测和定位问题的方法。样例中的异常均为教学用途的故意注入，不能直接用于生产算子。

## 支持范围

- 多核踩踏、三类 L1 异常和 UB 非对齐访问需要 1 个 mssanitizer 支持的 NPU。
- 非法释放、内存泄漏和分配未使用场景需要 CANN heap 检测能力。
- `dav-2201` 适用于 Atlas A2/A3 系列产品，`dav-3510` 适用于 Ascend 950PR/Ascend 950DT。
- 三个 L1 kernel 使用 `__cube__`，保证 GM 与 L1 间的数据搬运在 AI Cube Core 上实际执行。

## 目录结构

```text
sample_memcheck_advanced/
├── CMakeLists.txt
├── README.md
├── memcheck_advanced.asc
└── test_sample.sh
```

## 场景设计

| 模式 | 故意注入的异常 | 预期报告 |
| --- | --- | --- |
| `multi-core` | 两个 block 同址写 GM | `WARNING: out of bounds` |
| `l1-oob` | L1 只分配 64 个 `float`，却从 GM 写入 256 个 | `ERROR: illegal write`、`on L1` |
| `l1-oob-read` | L1 只分配并初始化 64 个 `float`，却向 GM 搬出 256 个 | `ERROR: illegal read`、`on L1` |
| `l1-misaligned` | L1 首地址偏移 4 字节后执行普通 DMA | `ERROR: misaligned access`、`on L1` |
| `misaligned` | UB 首地址偏移 4 字节后执行普通 DMA | `ERROR: misaligned access` |
| `illegal-free` | 对同一 GM 地址连续调用两次 `aclrtFree` | `ERROR: illegal free()` |
| `leak` | 申请 GM 后不释放且不 reset Device | `ERROR: LeakCheck` |
| `unused` | 申请 GM 后不访问直接释放 | `WARNING: Unused memory` |

### 多核踩踏设计

`multi_core_stomp` 启动两个 block。两个 block 均调用 `SetValue` 写入
同一 GM 首地址，没有按 block 偏移输出地址。mssanitizer 会报告第二个
block 写入了不属于它的区间，并给出 block、Device 和源码调用栈。

### L1 异常设计

`l1_out_of_bounds` 使用 `LocalMemAllocator<Hardware::L1>` 在 L1 中只分配
64 个 `float`，随后通过 `DataCopy` 从 GM 搬入 256 个 `float`。超出已分配
L1 tensor 的写入应被 mssanitizer 报告为 L1 上的 `illegal write`，并给出
kernel、block、Device 和源码调用栈。

`l1_read_out_of_bounds` 先将 64 个 `float` 从 GM 搬入同等容量的 L1 tensor，
确保合法区间已经初始化；随后尝试从该 L1 tensor 向 GM 搬出 256 个 `float`。
超出已分配 L1 tensor 的读取应被报告为 L1 上的 `illegal read`，且不会与
未初始化读取混淆。

`l1_misaligned_access` 分配 `kElements + 1` 个 `float` 的 L1，使用
`storage[1]` 构造偏移 4 字节的子 tensor，再从 GM 执行 `DataCopy`。偏移后的
剩余容量恰好覆盖 256 个元素，因此该场景只触发 L1 非对齐访问，不依赖越界。

三个 L1 kernel 均限定为 cube kernel。自动复验不仅检查错误类型和内存空间，
还要求报告出现 `block aic(...)`，避免 L1 搬运未在正确核型执行时被误判为通过。

### 非对齐访问设计

`misaligned_access` 分配 `kElements + 1` 个 `float` 的 UB，使用
`storage[1]` 构造偏移 4 字节的子 tensor，再从 GM 执行 `DataCopy`。
分配容量覆盖全部搬运数据，因此该场景只触发非对齐访问，不依赖越界行为。

### Host 内存生命周期异常

后三个模式通过 mssanitizer 提供的 ACL 包装头文件记录申请和释放位置，并在执行时单独开启 `--check-cann-heap=yes`。非法释放、泄漏和未使用内存分别在独立进程中触发，避免一个故意注入的异常污染其他场景。

## 编译

进入本目录后执行：

```bash
source /usr/local/Ascend/ascend-toolkit/set_env.sh
cmake -S . -B build -DCMAKE_ASC_ARCHITECTURES=dav-2201
cmake --build build -j
```

CMake 会从 `${ASCEND_HOME_PATH}/tools/mssanitizer` 查找 ACL 包装头文件和 `libascend_acl_hook.so`。若工具安装在其他位置，可显式指定：

```bash
cmake -S . -B build \
  -DCMAKE_ASC_ARCHITECTURES=dav-2201 \
  -DMSSANITIZER_ACL_INCLUDE_DIR=/path/to/mssanitizer/include/acl \
  -DMSSANITIZER_ACL_HOOK_LIBRARY=/path/to/libascend_acl_hook.so
```

编译参数已包含 `-g --cce-enable-sanitizer`，生成的可执行文件为 `build/demo`。

## 执行与结果分析

### 多核踩踏

```bash
mssanitizer --tool=memcheck ./build/demo multi-core
```

结果应包含：

```text
WARNING: out of bounds of size 4
on GM when writing data in "multi_core_stomp..."
in block aiv(1) on device 0
```

修复时应按 block 划分互不重叠的 GM 区间，或只启动实际需要的 block 数量。

### L1 越界写

```bash
mssanitizer --tool=memcheck ./build/demo l1-oob
```

结果应包含：

```text
ERROR: illegal write of size ...
at ... on L1 in "l1_out_of_bounds..."
```

修复时应保证 L1 tensor 的分配容量覆盖实际搬运量，或将搬运量限制在已分配范围内。

### L1 越界读

```bash
mssanitizer --tool=memcheck ./build/demo l1-oob-read
```

结果应包含：

```text
ERROR: illegal read of size ...
at ... on L1 in "l1_read_out_of_bounds..."
```

修复时应让源 L1 tensor 的已分配容量覆盖实际读取量，并区分越界读取与未初始化读取。

### L1 非对齐访问

```bash
mssanitizer --tool=memcheck ./build/demo l1-misaligned
```

结果应包含：

```text
ERROR: misaligned access of size ...
at ... on L1 in "l1_misaligned_access..."
```

修复时应保证参与 DMA 的 L1 地址满足 32 字节对齐。

### 非对齐访问

```bash
mssanitizer --tool=memcheck ./build/demo misaligned
```

结果应包含：

```text
ERROR: misaligned access of size ...
on UB ... in "misaligned_access..."
```

修复时应让 UB 普通 DMA 地址满足 32 字节对齐，不能从 `float` 粒度的非对齐子 tensor 开始搬运。

### 非法释放

```bash
mssanitizer --tool=memcheck --check-cann-heap=yes \
  ./build/demo illegal-free
```

结果应包含 `ERROR: illegal free()` 和触发二次释放的代码位置。修复时应在成功释放后立即清空所有权，并保证清理路径只执行一次。

### 内存泄漏

```bash
mssanitizer --tool=memcheck --check-cann-heap=yes --leak-check=yes \
  ./build/demo leak
```

结果应包含：

```text
ERROR: LeakCheck: detected memory leaks
Direct leak of ... byte(s)
SUMMARY: ... byte(s) leaked in 1 allocation(s)
```

修复时应确保申请和释放在正常路径与错误路径上均成对出现。

### 分配未使用

```bash
mssanitizer --tool=memcheck --check-cann-heap=yes --check-unused-memory=yes \
  ./build/demo unused
```

结果应包含：

```text
WARNING: Unused memory of ... byte(s)
SUMMARY: ... byte(s) unused memory in 1 allocation(s)
```

修复时应删除无效申请，或补齐遗漏的数据初始化和实际读写逻辑。

## 自动复验

脚本会清理并重新编译样例，依次执行八个模式，并校验诊断类型、内存空间、源码位置、汇总数量以及程序是否异常退出：

```bash
bash test_sample.sh
```

日志默认保存在 `test_output/`，也可传入其他目录：

```bash
bash test_sample.sh /tmp/memcheck_advanced_logs
```

若 mssanitizer 的 ACL 包装文件不在默认安装目录，可通过环境变量传入：

```bash
MSSANITIZER_ACL_INCLUDE_DIR=/path/to/mssanitizer/include/acl \
MSSANITIZER_ACL_HOOK_LIBRARY=/path/to/libascend_acl_hook.so \
bash test_sample.sh
```

全部检查通过时，脚本输出：

```text
[PASS] multi-core: 检测结果符合预期
[PASS] l1-oob: 检测结果符合预期
[PASS] l1-oob-read: 检测结果符合预期
[PASS] l1-misaligned: 检测结果符合预期
[PASS] misaligned: 检测结果符合预期
[PASS] illegal-free: 检测结果符合预期
[PASS] leak: 检测结果符合预期
[PASS] unused: 检测结果符合预期
[PASS] sample_memcheck_advanced 全部场景验证通过
```

## 注意事项

- 五个 kernel 场景使用默认 memcheck；三个 Host 内存生命周期场景使用 `--check-cann-heap=yes`，自动复验脚本会按模式设置参数。
- 检测报告中的地址、PC、序列号和绝对源码路径会随运行环境变化，应以异常类型、内存空间、block、Device、有效源码位置和汇总数量为判断依据。
- `unused` 是主动检测项，必须显式传入 `--check-unused-memory=yes`；泄漏检测必须显式传入 `--leak-check=yes`。
