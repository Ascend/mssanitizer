# MindStudio Sanitizer常见问题

<br>

## msSanitizer工具异常报告中未打印正确的文件名和行号

**问题现象**

文件名和行号显示为"<unknown\>:0"，或文件名显示正确，但行号显示为"0"。

**解决方案**

- 文件名和行号显示为"<unknown\>:0"。

    说明msSanitizer工具没有解析到正确的文件名和行号，根据用户的检测场景，有以下两种解决方法：

    - 如果启用了"--check-cann-heap=yes"选项，对CANN软件栈内存进行检测，则可以通过引入Sanitizer API头文件并重新编译用户程序使检测工具获取到正确的文件名和行号，具体可参考《[基础案例](../best_practices/mssanitizer_basic_cases.md)》中“检测CANN软件栈的内存>内存泄漏检测使用原理>步骤4”。
    - 如果正在对算子进行异常检测，那么可能是在算子编译阶段未启用"-g"编译选项，启用"-g"编译选项后才能生成正确的文件名和行号，具体可参考《[MindStudio Sanitizer工具用户指南](../user_guide/mssanitizer_user_guide.md)》中的“使用前准备>内核调用符场景准备”。

- 文件名显示正确，但行号显示为"0"。

    这种情况一般是因为使用了"-O2"或"-O3"编译选项进行算子代码编译，编译器对算子代码进行优化时导致代码行变化，可通过在算子编译阶段使用"-O0"禁用编译器优化来解决这个问题。

## msSanitizer工具使用"--cce-enable-sanitizer -g"编译算子时出现"InputSection too large"错误

**问题现象**

报错ld.lld: error: InputSection too large for range extension thunk。

**原因分析**

算子链接时输入代码段过大，超过编译器支持的指令跳转范围。

**解决方案**

通过增加编译选项，启用编译器扩大跳转范围的特性来解决。在《[MindStudio Sanitizer工具用户指南](../user_guide/mssanitizer_user_guide.md)》中的“使用前准备>配置编译选项>在编译选项中增加-g”选项"--cce-enable-sanitizer -g"后增加"-Xaicore-start -mcmodel=large -mllvm -cce-aicore-relax -Xaicore-end"。

```shell
target_compile_options(${smoke_testcase}_npu PRIVATE
                     -O2
                     -std=c++17
                     --cce-enable-sanitizer
                     -g
                     -Xaicore-start -mcmodel=large -mllvm -cce-aicore-relax -Xaicore-end
)
```

## msSanitizer工具提示--cache-size异常

**问题现象**

使用msSanitizer工具进行异常检测时，提示"113023 records undetected, please use --cache-size=_xx_  to run the operator again" 。

**原因分析**

算子执行信息的大小超过工具默认分配GM（Global Memory，全局内存）的大小，导致部分信息丢失。

**解决方案**

根据提示修改--cache-size值，并重新启动算子，进行异常检测。

## PyTorch场景下内存检测结果不准确或出现多核踩踏误报

**问题现象**

在`<<<>>>`自定义算子接入torch场景下，使用msSanitizer进行内存检测时，越界检测结果不准确或出现多核踩踏误报。

**原因分析**

PyTorch框架内默认使用内存池的方式管理GM（Global Memory）内存，内存池通常会一次性分配大量GM内存并在运行过程中复用，这会干扰工具的检测逻辑，导致检测信息不准确。

**解决方案**

在检测前设置以下环境变量关闭PyTorch的NPU内存池：

```shell
export PYTORCH_NO_NPU_MEMORY_CACHING=1
```

> [!NOTE]
> Triton算子调用场景同样会使用PyTorch创建Tensor，也需设置此环境变量和`TRITON_ALWAYS_COMPILE=1`以保证检测有效性。

## 不添加编译选项与添加编译选项的检测范围有何区别

**问题现象**

用户不确定是否需要在编译阶段添加`--cce-enable-sanitizer`或`--sanitizer`编译选项。

**解决方案**

两种方式的检测能力对比如下：

| 对比项 | 不添加编译选项（快速定界） | 添加编译选项（全量检测） |
|--------|--------------------------|------------------------|
| 指令检测范围 | 仅与GM相关的搬运指令 | 全量指令 |
| 异常检测范围 | 仅非法读写和非对齐访问 | 支持全量检测 |
| 调用栈信息 | 不显示 | 添加`-g`选项后显示 |
| 适用场景 | 快速定界异常算子 | 全量深度检测 |

建议先用不添加编译选项的方式快速定位异常算子，再添加编译选项对异常算子进行全量检测。

> [!NOTE]
> 不添加编译选项时算子的优化等级需为O2，并保证算子链接阶段增加`-q`选项保留符号重定位信息，否则检测功能可能失效。该方式不适用于Atlas推理系列产品，且仅适用于算子内核调用场景。

## 四种检测工具的使用顺序建议

**问题现象**

用户不确定应该先使用哪种检测工具（memcheck / racecheck / initcheck / synccheck）。

**解决方案**

建议按以下顺序使用：

1. **先运行 memcheck（内存检测）**，确认算子程序无内存异常（非法读写、多核踩踏、非对齐访问、内存泄漏、非法释放等）
2. 确认无内存异常后，再按需运行：
   - **racecheck（竞争检测）**：检查数据竞争问题
   - **initcheck（未初始化检测）**：检查脏数据读取问题
   - **synccheck（同步检测）**：检查同步指令配对问题

原因是内存异常可能导致程序运行状态异常，影响其他检测工具的准确性。

## --cce-enable-sanitizer与--sanitizer编译选项是否等价

**问题现象**

文档中同时出现了`--cce-enable-sanitizer`和`--sanitizer`两种编译选项，用户不确定应该使用哪一个。

**解决方案**

两个选项**完全等价**，可随意使用其一，均代表使能异常检测。示例：

```cmake
# 以下两种写法效果相同
target_compile_options(${smoke_testcase}_npu PRIVATE -O2 --cce-enable-sanitizer -g)
target_compile_options(${smoke_testcase}_npu PRIVATE -O2 --sanitizer -g)
```

## 调用栈信息偶尔获取失败或显示不全

**问题现象**

添加了`-g`编译选项后，异常报告中仍然偶尔看不到调用栈信息。

**原因分析**

因llvm-symbolizer开源软件的限制，调用栈的异常信息可能会获取失败。

**解决方案**

再次执行相同的检测命令，通常即可获取到调用栈的异常信息。

## cmake ../cmake 和 cmake .. 有什么区别

**问题现象**

在源码编译安装时，用户不确定应该使用`cmake ../cmake`还是`cmake ..`。

**解决方案**

请务必使用`cmake ../cmake`而非`cmake ..`，否则不会生成`.run`安装包。正确步骤：

```shell
mkdir build
cd build
cmake ../cmake
make -j$(nproc)
```

## 如何卸载msSanitizer

**问题现象**

用户需要卸载已安装的msSanitizer工具。

**解决方案**

1. 下载卸载脚本（需要联网环境）：

   ```bash
   curl -O https://inst.obs.cn-north-4.myhuaweicloud.com/26.0.0/ms_install.py
   ```

   > [!NOTE]
   > 若环境不允许联网，请先在可联网的环境下载该脚本后拷贝到目标设备。若执行命令无响应或出现连接失败、SSL证书错误等问题，请参见[FAQ](https://www.hiascend.com/developer/blog/details/02176213671719317003)。

2. 执行卸载：

   ```bash
   python ms_install.py uninstall {tools_name}
   ```
