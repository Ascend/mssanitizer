# MindStudio Sanitizer常见问题

<br>

## 1. msSanitizer工具异常报告中未打印正确的文件名和行号

**问题现象**

文件名和行号显示为"<unknown\>:0"，或文件名显示正确，但行号显示为"0"。

**解决方案**

- 文件名和行号显示为"<unknown\>:0"。

    说明msSanitizer工具没有解析到正确的文件名和行号，根据用户的检测场景有以下两种解决方法：

    - 如果启用了"--check-cann-heap=yes"选项，对CANN软件栈内存进行检测，则可以通过引入Sanitizer API头文件并重新编译用户程序使检测工具获取到正确的文件名和行号，具体可参考《[example](../best_practices/basic_cases.md)》中“检测CANN软件栈的内存>内存泄漏检测使用原理>步骤4”。
    - 如果正在对算子进行异常检测，那么可能是在算子编译阶段未启用"-g"编译选项，启用"-g"编译选项后才能生成正确的文件名和行号，具体可参考《[MindStudio Sanitizer工具用户指南](../user_guide/mssanitizer_user_guide.md)》中的“使用前准备>内核调用符场景准备”。

- 文件名显示正确，但行号显示为"0"。

    这种情况一般是因为使用了"-O2"或"-O3"编译选项进行算子代码编译，编译器对算子代码进行优化时导致代码行变化，可通过在算子编译阶段使用"-O0"禁用编译器优化来解决这个问题。

## 2. msSanitizer工具使用"--cce-enable-sanitizer -g"编译算子时出现"InputSection too large"错误

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

## 3. msSanitizer工具提示--cache-size异常

**问题现象**

使用msSanitizer工具进行异常检测时，提示"113023 records undetected, please use --cache-size=_xx_  to run the operator again" 。

**原因分析**

算子执行信息的大小超过工具默认分配GM内存的大小，导致部分信息丢失。

**解决方案**

根据提示修改--cache-size值，并重新启动算子，进行异常检测。
