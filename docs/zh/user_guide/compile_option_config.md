# 开启全量检测

<br>

如需启用全量检测，须在算子代码的编译阶段添加相应编译选项并重新编译算子。
由于不同算子工程中编译选项的配置位置存在差异，将以[模板库场景](#1-模板库场景)、[内核调用符场景](#2-内核调用符场景)、[msOpGen算子工程编译场景](#3-msopgen算子工程编译场景)和[Triton算子调用场景](#4-triton算子调用场景)为例分别说明。

## 1. 模板库场景

修改模板库中的**examples/CMakeLists.txt**文件，新增`-g --cce-enable-sanitizer`编译选项。

```cmake
set(BISHENG_COMPILER_OPTIONS -g --cce-enable-sanitizer)
```

## 2. 内核调用符场景

1. 样例工程代码请参考[basic_api_tque_add代码示例](https://gitcode.com/cann/asc-devkit/tree/9.0.0/examples/01_simd_cpp_api/00_introduction/01_add/basic_api_tque_add)，执行以下命令，下载分支版本的样例代码。

    ```sh
    git clone https://gitcode.com/cann/asc-devkit.git -b 9.0.0
    cd asc-devkit/examples/01_simd_cpp_api/00_introduction/01_add/basic_api_tque_add
    ```

2. 进行算子代码编译，需添加以下编译选项：

    - -g
    - --cce-enable-sanitizer或--sanitizer

    编辑样例工程目录下的“CMakeLists.txt”文件，参考核函数开发和运行验证的完整样例。根据实际部署的 NPU 硬件架构选择对应的 `npu-arch` 参数，例如Atlas 950PR&950DT系列产品需改为`dav-3510`。本样例参数值`dav-2201`适用于昇腾A2/A3系列产品。

    ```cmake
    target_compile_options(add PRIVATE
        $<$<COMPILE_LANGUAGE:ASC>:--npu-arch=dav-2201 --cce-enable-sanitizer -g>
        # $<$<COMPILE_LANGUAGE:ASC>:--npu-arch=dav-3510>
    )
    ```

    增加--cce-enable-sanitizer或--sanitizer选项代表使能异常检测，两个选项完全等价，可随意使用其一。

    增加-g选项使编译器生成定位信息，将会在异常报告输出时打印异常发生的具体位置（文件名、行号以及调用栈等信息）。

    > [!NOTE]
    >
    > - --cce-enable-sanitizer和-O0同时开启的情况下，需要增加编译选项 --cce-ignore-always-inline=false。
    > - 添加-g编译选项会在生成的二进制文件中附带调试信息，建议限制带有调试信息的用户程序的访问权限，确保只有授权人员可以访问该二进制文件。
    > - 增加--cce-enable-sanitizer编译选项生成的算子二进制，需与msSanitizer工具配套使用。不建议单独使用该二进制，单独使用可能会导致不可预见的问题。
    > - 因**llvm-symbolizer**开源软件限制，调用栈的异常信息可能会获取失败。此时，用户可再次执行检测命令，就可以获取调用栈的异常信息。

3. 启用msSanitizer检测工具时，需要加载NPU侧可执行文件_<kernel\_name>__npu，该文件的获取可参考《Ascend C算子开发指南》中的“Kernel直调算子开发 >[Kernel直调](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_0056.html)”章节。

## 3. msOpGen算子工程编译场景

1. 参考[链接](https://gitcode.com/cann/cann-learning-hub/tree/8.5.0/tutorials/ascendc_operator_development/03_intermediate_vector_operator_development/src/custom_op_04)，在`${git_clone_path}/cann-learning-hub/tutorials/ascendc_operator_development/03_intermediate_vector_operator_development/src/custom_op_04`路径下准备算子工程。

    > [!NOTE]
    >
    > 此样例工程默认按 Atlas A2 系列产品配置。若使用其他系列产品，需修改以下两处默认配置：
    >
    > 1. 根目录 `CMakeLists.txt` 中 `ASCEND_COMPUTE_UNIT` 默认为 `ascend910b`，请根据实际芯片型号（通过 `npu-smi info` 查询 Chip Name）修改。例如 Atlas 950PR&950DT 系列产品需改为 `ascend950`。
    > 2. `op_host` 目录下 `add_custom_template.cpp` 中 `.AddConfig("ascend910b")` 的值需要同样修改。
    >
    > 下载代码样例时，需执行以下命令指定分支版本。
    >
    > ```sh
    > git clone https://gitcode.com/cann/cann-learning-hub.git -b 8.5.0
    > ```

2. 切换至自定义算子工程目录。

    ```sh
    cd custom_op_04
    ```

3. 编辑样例工程目录下的“op_kernel/CMakeLists.txt”文件，在编译选项中添加-sanitizer选项，具体请参考《MindStudio Ops Generator工具用户指南》中的“算子编译部署>支持自定义编译选项”章节。

```cmake
add_ops_compile_options(ALL OPTIONS -sanitizer)
```

## 4. Triton算子调用场景

Triton算子采用Python语言进行开发，并且采用即时编译（JIT）方式来编译算子Kernel。在执行算子脚本前，需要配置以下环境变量支持全量检测。

```sh
export TRITON_ENABLE_SANITIZER=1
```

---

## 5. 验证与后续步骤

完成上述编译选项配置后：

1. 重新编译算子，确认编译过程无报错。
2. 使用 msSanitizer 工具拉起算子进行检测，具体用法请参见《[MindStudio Sanitizer 使用指南](./mssanitizer_user_guide.md)》。
3. 若检测报告中的文件名和行号显示异常，请参见《[MindStudio Sanitizer 常见问题](../support/mssanitizer_faq.md)》。
