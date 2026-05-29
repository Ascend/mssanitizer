# MindStudio Sanitizer Release Notes

## Version Mapping

### Product Version

| Product Name| Version  | Version Type|
|------|-------|------|
| msSanitizer | 26.0.0 | Internal test version|
| msSanitizer | 8.3.0 | Official version|

### Related Product Versions

| msSanitizer Version| CANN Version| Python version| JSON Version|SecureC Version| Makeself Version| llvm-project Version
|----------|-----------------|----------|----------|----------|----------|----------|
| 26.0.0 | 9.0.0 or later is recommended.| Python 3.11 or later is recommended.| v3.12.0 or later| v1.1.16 or later| release-2.5.0 or later| 19.1.7 |
| 8.3.0 | 8.2.RC1 or later| Python 3.11 or later is recommended.| v3.12.0 or later| v1.1.16 or later| release-2.5.0 or later| 19.1.7 |

## Version Compatibility

### 26.0.0

1. Adapted to the new `--cce-use-legacy-mixkernel-mangling` compilation option of the BiSheng Compiler.
2. Added compatibility with multiple new chip specifications and model variants and updated to accommodate changes in chip identification within CANN packages.

## Feature Updates

### 26.0.0

#### 1. New Features

Function:

- Check function:

1. Added out-of-bounds check support for `LocalTensor` across AscendC monocular and binocular computation APIs as well as data movement APIs.
2. Added support for memory corruption check between SIMT and Main-Scalar pipelines.
3. Added support for inter-thread race check within a SIMTR VF.
4. Added support for redundancy check of the `SET_FLAG` instruction.
5. Added instrumentation and processing for a large number of intra-core and inter-core synchronization instructions to enhance race check capabilities. Added commands `SET_FLAG`/`WAIT_FLAG`/`SET_FLAGI`/`WAIT_FLAGI`/`HSET`/`HWAIT`/`GET_BUF`/`RLS_BUF`.

- UI:

1. Added support for displaying kernel information during check. Upon completion of check, the user will be prompted to indicate whether any exceptions were detected.
2. Added the `--demangle` command line option to control the display format of function names in the user interface.
3. Added support for obtaining the real-time register status at the start and checking the default register values at the end of the program.

- Scalability:

1. Added the MSTX interface on the kernel for reporting inter-core barrier and `set_flag`/`wait_flag` semantics.
2. Exposed kernel-side MSTX interface via `sanitizer_report.h` header, enabling user-defined integration.
3. Removed the restriction on binding regions and heaps in the memory pool information reporting interface of MSTX, allowing direct region registration.

Build and release:

1. Added the debug compilation function to support VS Code breakpoint debugging.
2. Changed the minimum permission required by the root user on folders during installation to `700`.
3. Resolved the issue of UT compilation failures in the later GCC 11.x version.
4. Adapted to the GCC 12.x changes in the CANN image.
5. Optimized the download of UT dependencies, increasing the speed by 10 times and completely resolving the issue of occasional failures.
6. Enabled the debug compilation mode by default for UT compilation.
7. Standardized the installation package names.
8. Added the **sanitizer_report.h** header file to the package.

Document description:

1. Comprehensively optimized and reconstructed the documents to improve usability.

#### 2. Deleted Features

None

#### 3. Bug Fixes

1. Resolved the issue where SIMT LDK/STK failed to persist to drives through instructions.
2. Resolved the issue where file paths were not standardized before use and the sequence of including header files was incorrect.
3. Resolved the issue with SIMT UB range modeling, which may cause the tool to miss reporting UB out-of-bounds errors.
4. Changed the name of the command line option for call stack backtracking to comply with industry conventions.
5. Resolved the issue where the address space of the `load store` instruction was invalid.
6. Resolved the issue with ND NZ API preprocessing, which may cause false positives of memory out-of-bounds errors.
7. Resolved the issue where the internal implementation of AscendC APIs was not masked in the abnormal call stack for race and synchronization checks.
8. Resolved an issue where synchronization instructions were incorrectly used in the online check algorithm for inter-thread corruption.
9. Resolved the deadlock issue in the soft synchronization check algorithm, which may cause false negatives in race check.
10. Resolved the issue of false positives caused by the synchronization check algorithm not being reset in multi-operator scenarios.
11. Corrected the parsing of `LD`, `LD_IO`, `ST`, `ST_IO`, and `STI_IO` instructions to resolve the issue of missing unalignment check logic.
12. Resolved the issue of incorrect summary display after operator check is complete.
13. Resolved the issue of incorrect calculation of the blockIdx function during the linking of the Mix operator.
14. Resolved the issue of the "LOAD2D address overflow" error reported during operator runtime after dynamic instrumentation on specific chips.
15. Resolved the issue of repeated reporting of unalignment check for the DataCopy instruction.
16. Resolved the issue of false memory alarms caused by inconsistent GM addresses in the dual-page table range.
17. Resolved the issue of missing reporting of multi-thread corruption for the SIMT operator.
18. Resolved the modeling error of the `get_buf rls_buf` instruction.
19. Optimized the `create_folder` function to modify the owner group and permission of the folder only after the folder is created.
20. Resolved the issue that the tool exits in advance in some scenarios.
21. Resolved the issue of missing reporting of competition between pipe-s and other pipelines.

### 8.3.0

#### 1. New Features

This issue is the first official release. The following features are added:

1. Memory check: detects memory exceptions in global memory and local memory, such as out-of-bounds access and unaligned access.
2. Race check: checks data race issues caused by concurrent memory access in a parallel computing environment.
3. Uninitialization check: checks memory read exceptions caused by the use of uninitialized variables.
4. Synchronization check: checks for unpaired `SetFlag`/`WaitFlag` instructions in Ascend C operators.

#### 2. Deleted Features

None

#### 3. Bug Fixes

None
