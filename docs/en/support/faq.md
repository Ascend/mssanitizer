# MindStudio Sanitizer FAQs

<br>

## 1. msSanitizer Exception Report Does Not Output the Correct File Name and Line Number

**Symptom**

The file name and line number are displayed as `<unknown\>:0`, or the file name is correct but the line number is displayed as `0`.

**Solution**

- The file name and line number are displayed as `<unknown\>:0`.

    The msSanitizer tool fails to parse the correct file name and line number. You can use either of the following methods to solve the problem:

    - If the `--check-cann-heap=yes` option is enabled to check the memory of the CANN software stack, you can import the sanitizer API header file and recompile the user program so that the detection tool can obtain the correct file name and line number. For details, see "Checking the Memory of the CANN Software Stack" > "Memory Leak Detection Principles" > "Step 4" in [Example](../best_practices/basic_cases.md).
    - If the operator is being checked for exceptions, the `-g` compilation option may not be enabled in the operator compilation phase. The correct file name and line number can be generated only after the `-g` compilation option is enabled. For details, see "Preparations" > "Preparations for Kernel Launch Symbol Scenario" in [MindStudio Sanitizer User Guide](../user_guide/mssanitizer_user_guide.md).

- The file name is correct, but the line number is `0`.

    This problem generally occurs because the `-O2` or `-O3` compilation option is used to compile the operator code. When the compiler optimizes the operator code, the code line changes. You can use `-O0` to disable compiler optimization during operator compilation to solve this problem.

## 2. Error Message "InputSection Too Large" Is Displayed When msSanitizer Uses --cce-enable-sanitizer -g to Compile Operators

**Symptom**

The error message `ld.lld: error: InputSection too large for range extension thunk` is displayed.

**Possible Causes**

The input code segment for operator linking is too large and exceeds the instruction jump range supported by the compiler.

**Solution**

Add compilation options to increase the jump range of the compiler. According to "Preparations" > "Configuring Compilation Options" > "Adding -g to Compilation Options" in [MindStudio Sanitizer User Guide](../user_guide/mssanitizer_user_guide.md), add `-Xaicore-start -mcmodel=large -mllvm -cce-aicore-relax -Xaicore-end` after `--cce-enable-sanitizer -g`.

```shell
target_compile_options(${smoke_testcase}_npu PRIVATE
                     -O2
                     -std=c++17
                     --cce-enable-sanitizer
                     -g 
                     -Xaicore-start -mcmodel=large -mllvm -cce-aicore-relax -Xaicore-end
)
```

## 3. msSanitizer Error: --cache-size Exception

**Symptom**

When the msSanitizer tool is used to detect exceptions, the message `113023 records undetected, please use --cache-size=_xx_ to run the operator again` is displayed.

**Possible Causes**

The size of the operator execution information exceeds the default size of the GM allocated by the tool, causing some information loss.

**Solution**

Change the value of `--cache-size` as prompted and restart the operator for exception detection.
