# MindStudio Sanitizer FAQs

<br>

## 1. msSanitizer Exception Report Does Not Output the Correct File Name and Line Number

**Symptom**

The file name and line number are displayed as `<unknown\>:0`, or the file name is correct but the line number is displayed as `0`.

**Solution**

- The file name and line number are displayed as `<unknown\>:0`.

    The msSanitizer tool fails to parse the correct file name and line number. You can use either of the following methods to solve the problem:

    - If the `--check-cann-heap=yes` option is enabled to check the memory of the CANN software stack, you can import the sanitizer API header file and recompile the user program so that the detection tool can obtain the correct file name and line number. For details, see "Checking the Memory of the CANN Software Stack" > "Memory Leak Detection Principles" > "Step 4" in [Example](../best_practices/mssanitizer_basic_cases.md).
    - If the operator is being checked for exceptions, the `-g` compilation option may not be enabled in the operator compilation phase. The correct file name and line number can be generated only after the `-g` compilation option is enabled. For details, see "Preparations" > "Preparations for Kernel Launch Symbol Scenario" in the [MindStudio Sanitizer User Guide](../user_guide/mssanitizer_user_guide.md).

- The file name is correct, but the line number is `0`.

    This problem generally occurs because the `-O2` or `-O3` compilation option is used to compile the operator code. When the compiler optimizes the operator code, the code line changes. You can use `-O0` to disable compiler optimization during operator compilation to solve this problem.

## 2. Error Message `InputSection Too Large` Is Displayed When msSanitizer Uses `--cce-enable-sanitizer -g` to Compile Operators

**Symptom**

The error message `ld.lld: error: InputSection too large for range extension thunk` is displayed.

**Possible Causes**

The input code segment for operator linking is too large and exceeds the instruction jump range supported by the compiler.

**Solution**

Add compilation options to increase the jump range of the compiler. According to "Preparations" > "Configuring Compilation Options" > "Adding -g to Compilation Options" in the [MindStudio Sanitizer User Guide](../user_guide/mssanitizer_user_guide.md), add `-Xaicore-start -mcmodel=large -mllvm -cce-aicore-relax -Xaicore-end` after `--cce-enable-sanitizer -g`.

```shell
target_compile_options(${smoke_testcase}_npu PRIVATE
                     -O2
                     -std=c++17
                     --cce-enable-sanitizer
                     -g
                     -Xaicore-start -mcmodel=large -mllvm -cce-aicore-relax -Xaicore-end
)
```

## 3. msSanitizer Error: `--cache-size` Exception

**Symptom**

When the msSanitizer tool is used to detect exceptions, the message `113023 records undetected, please use --cache-size=_xx_ to run the operator again` is displayed.

**Possible Causes**

The size of the operator execution information exceeds the default size of the Global Memory (GM) allocated by the tool, causing some information loss.

**Solution**

Change the value of `--cache-size` as prompted and restart the operator for exception detection.

## 4. Inaccurate Memory Check Results or Multi-Core Corruption False Alarms in PyTorch Scenarios

**Symptom**

In the scenario where custom operators launched through `<<<>>>` are integrated into torch, when msSanitizer is used for memory check, the out-of-bounds detection results may be inaccurate, or false alarms about multi-core corruption may occur.

**Possible Causes**

By default, the PyTorch framework manages the Global Memory (GM) in memory pool mode. A memory pool usually allocates a large amount of GM at one time and reuses it at runtime, which interferes with the detection logic of the tool and results in inaccurate detection information.

**Solution**

Before detection, set the following environment variable to disable the NPU memory pool of PyTorch:

```shell
export PYTORCH_NO_NPU_MEMORY_CACHING=1
```

> [!NOTE]
> In a Triton operator invocation scenario, PyTorch is also used to create tensors. You also need to set this environment variable and `TRITON_ALWAYS_COMPILE=1` to ensure the validity of the check.

## 5. Difference Between Detection Ranges With and Without Compilation Options

**Symptom**

You are not sure whether you need to add the `--cce-enable-sanitizer` or `--sanitizer` compilation option in the compilation phase.

**Solution**

The following table compares the detection capabilities of the two methods:

| Comparison Item | Without Compilation Options (Quick Locating) | With Compilation Options (Full Check) |
|------|------|------|
| Instruction detection range | Only GM-related move instructions | All instructions |
| Exception detection range | Only illegal read/write and unaligned access | Full check supported |
| Call stack information | Not displayed | Displayed after the `-g` option is added |
| Applicable scenario | Quickly locating abnormal operators | Full in-depth check |

You are advised to first quickly locate the abnormal operator without adding compilation options, and then add compilation options to perform full check on the abnormal operator.

> [!NOTE]
> When compilation options are not added, the optimization level of the operator must be `O2`, and the `-q` option must be added in the operator linking phase to retain the symbol relocation information. Otherwise, the check function may fail. This method does not apply to Atlas inference products and is applicable only to operator kernel invocation scenarios.

## 6. Recommended Usage Order of the Four Detection Tools

**Symptom**

You are not sure which detection tool (memcheck, racecheck, initcheck, or synccheck) to run first.

**Solution**

You are advised to use the tools in the following order:

1. **Run memcheck (memory check) first** to confirm that the operator program has no memory exceptions (illegal read/write, multi-core corruption, unaligned access, memory leak, illegal release, and so on).
2. After confirming that no memory exception exists, run the following tools as needed:
   - **racecheck (contention check)**: Checks data contention problems.
   - **initcheck (uninitialization check)**: Checks dirty data read problems.
   - **synccheck (synchronization check)**: Checks synchronization instruction pairing problems.

This is because memory exceptions may cause the program to run abnormally, affecting the accuracy of other detection tools.

## 7. Equivalence of the `--cce-enable-sanitizer` and `--sanitizer` Compilation Options

**Symptom**

Both the `--cce-enable-sanitizer` and `--sanitizer` compilation options appear in the documentation, and you are not sure which one to use.

**Solution**

The two options are **completely equivalent**. You can use either of them, as both enable exception detection. Example:

```cmake
# The following two methods have the same effect
target_compile_options(${smoke_testcase}_npu PRIVATE -O2 --cce-enable-sanitizer -g)
target_compile_options(${smoke_testcase}_npu PRIVATE -O2 --sanitizer -g)
```

## 8. Call Stack Information Occasionally Fails to Be Obtained or Is Incompletely Displayed

**Symptom**

After you add the `-g` compilation option, you may still occasionally fail to see call stack information in exception reports.

**Possible Causes**

Due to the limitations of llvm-symbolizer, an open-source software, the exception information in the call stack may fail to be obtained.

**Solution**

Run the same detection command again. You can usually obtain the call stack exception information in this way.

## 9. Difference Between `cmake ../cmake` and `cmake ..`

**Symptom**

During source code compilation and installation, you are not sure whether to use `cmake ../cmake` or `cmake ..`.

**Solution**

Be sure to use `cmake ../cmake` instead of `cmake ..`. Otherwise, the `.run` installation package will not be generated. The correct steps are as follows:

```shell
mkdir build
cd build
cmake ../cmake
make -j$(nproc)
```

## 10. How to Uninstall msSanitizer

**Symptom**

You need to uninstall the installed msSanitizer tool.

**Solution**

1. Download the uninstall script (a network connection is required):

   ```bash
   curl -O https://inst.obs.cn-north-4.myhuaweicloud.com/26.1.0/ms_install.py
   ```

   > [!NOTE]
   > If the environment does not allow network access, download the script in a networked environment first and copy it to the target device. If the command does not respond, or connection failures, SSL certificate errors, or other issues occur, see [FAQ](https://www.hiascend.com/developer/blog/details/02176213671719317003).

2. Run the uninstall command:

   ```bash
   python ms_install.py uninstall {tools_name}
   ```
