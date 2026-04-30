# 开启全量检测

<br>

如需启用全量检测，须在算子代码的编译阶段添加相应编译选项并重新编译算子。
由于不同算子工程中编译选项的配置位置存在差异，将以[模板库场景](#1-模板库场景)、[内核调用符场景](#2-内核调用符场景)、[msOpGen算子工程编译场景](#3-msopgen算子工程编译场景)和[Triton算子调用场景](#4-triton算子调用场景)为例分别说明。

## 1. 模板库场景

修改模板库中的**examples/CMakeLists.txt**文件，新增`-g --cce-enable-sanitizer`编译选项。

```text
set(BISHENG_COMPILER_OPTIONS -g --cce-enable-sanitizer)
```

## 2. 内核调用符场景

1. 样例工程代码请参考[LAddKernelInvocation代码示例](https://gitee.com/ascend/samples/tree/8.0.RC2/operator/AddCustomSample/KernelLaunch/AddKernelInvocation)，执行以下命令，下载分支版本的样例代码。

    ```sh
    git clone https://gitee.com/ascend/samples.git -b 8.0.RC2
    ```

    > [!NOTE]   
    > 此样例工程不支持Atlas A3 训练系列产品/Atlas A3 推理系列产品和Atlas 350 加速卡。

2. 进行算子代码编译，需添加以下编译选项：

    - -g  
    - --cce-enable-sanitizer或--sanitizer

    编辑样例工程目录下的“cmake/npu/CMakeLists.txt”文件，参考核函数开发和运行验证的完整样例。

    ```text
    target_compile_options(${smoke_testcase}_npu PRIVATE
                        -O2
                        -std=c++17
                        --cce-enable-sanitizer
                        -g
    )
    ```

    增加--cce-enable-sanitizer或--sanitizer选项代表使能异常检测。

    增加-g选项使编译器生成定位信息，将会在异常报告输出时打印异常发生的具体位置（文件名、行号以及调用栈等信息）。

    > [!NOTE]   
    > - --cce-enable-sanitizer和-O0同时开启的情况下，需要增加编译选项 --cce-ignore-always-inline=false。  
    > - 添加-g编译选项会在生成的二进制文件中附带调试信息，建议限制带有调试信息的用户程序的访问权限，确保只有授权人员可以访问该二进制文件。  
    > - 增加--cce-enable-sanitizer编译选项生成的算子二进制，需与msSanitizer工具配套使用。不建议单独使用该二进制，单独使用可能会导致不可预见的问题。  
    > - 因**llvm-symbolizer**开源软件限制，调用栈的异常信息可能会获取失败。此时，用户可再次执行检测命令，就可以获取调用栈的异常信息。  
    > - 目前Atlas 350 加速卡不支持--cce-enable-sanitizer或--sanitizer以及-O0编译选项，用于添加-g kernel侧编译选项后，如算子中有异常信息，则工具会展示异常信息对应的调用栈信息。

3. 链接阶段需增加target_link_options选项。

    - 编辑样例工程目录下的“cmake/npu/CMakeLists.txt”文件。

        ```text
        target_link_options(${smoke_testcase}_npu PRIVATE
            --cce-fatobj-link
            --cce-enable-sanitizer
        )
        ```

    - 编辑样例工程目录下的“cmake/Modules/CMakeCCEInformation.cmake”文件。

        ```text
        if(NOT CMAKE_CCE_LINK_EXECUTABLE)
        set(CMAKE_CCE_LINK_EXECUTABLE
            "<CMAKE_CCE_COMPILER> ${CMAKE_LIBRARY_CREATE_CCE_FLAGS} ${_CMAKE_COMPILE_AS_CCE_FLAG} <FLAGS> <CMAKE_CCE_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>${__IMPLICIT_LINKS}")
        endif()
        ```

4. 启用msSanitizer检测工具时，需要加载NPU侧可执行文件_<kernel\_name>__npu，该文件的获取可参考《Ascend C算子开发指南》中的“Kernel直调算子开发 >[Kernel直调](https://www.hiascend.com/document/detail/zh/canncommercial/83RC1/opdevg/Ascendcopdevg/atlas_ascendc_10_0056.html)”章节。

## 3. msOpGen算子工程编译场景

1. 单击[AddCustom代码示例](https://gitee.com/ascend/samples/tree/master/operator/ascendc/0_introduction/1_add_frameworklaunch/AddCustom)，在${git_clone_path}/samples/operator/ascendc/0_introduction/1_add_frameworklaunch目录下运行install.sh脚本，生成自定义算子工程。

    > [!NOTE]   
    > 下载代码样例时，需执行以下命令指定分支版本。
    > 
    > ```sh 
    > git clone https://gitee.com/ascend/samples.git -b master  
    > ```

    ```bash
    bash install.sh -v Ascendxxxyy    # xxxyy为用户实际使用的具体芯片类型
    ```

2. 切换至自定义算子工程目录。

    ```sh
    cd CustomOp
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
