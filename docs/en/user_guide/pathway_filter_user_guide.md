# Pathway Event Filtering User Guide

<br>

Before anomaly detection, this feature filters out pathway events that are known not to be supported by the hardware. This reduces false alarms caused by such events. Filtering applies only to events that match the rules and does not disable anomaly detection for the entire operator.

## Contents

- [1. Scope](#1-scope)
- [2. Prerequisites](#2-prerequisites)
- [3. Running Detection](#3-running-detection)
- [4. Viewing Results](#4-viewing-results)
- [5. Revalidation Method](#5-revalidation-method)
- [6. FAQ](#6-faq)
- [7. Limitations and Rollback](#7-limitations-and-rollback)

## 1. Scope

Pathway event filtering is enabled together with the MindStudio Sanitizer that includes this feature. **There is no separate CLI switch.** The current default rules require the following fields to match at the same time:

| Field | Default Match Value |
| --- | --- |
| Compute core | `AICUBE` |
| Event | `MEM_EVENT` |
| Pipeline | `PIPE_S_CAL` or `PIPE_S` |
| Memory | `UB` |
| Access mode | `WRITE` |
| Record type | `STORE`, `ST`, `ST_IO`, `STP`, `STI`, `STI_IO`, `ST_ATOMIC`, or `STI_ATOMIC` |

The preceding rules cover combinations of two pipeline representations and eight scalar Store record types. Only events that meet all conditions at the same time are removed. Filtering occurs before Kernel records enter the `memcheck`, `initcheck`, or `racecheck` detector. Host memory records and other detectors do not use this rule.

> [!NOTE]
>
> `initcheck` uses the consumption path of memory detection. Therefore, the consumer name in the filtering log is `AddressSanitizer`. The consumer name of `racecheck` is `RaceSanitizer`.

## 2. Prerequisites

1. Install the complete tool that includes this feature as described in the [Installation Guide](../install_guide/mssanitizer_install_guide.md). Ensure that CANN, the compiler, and the tool versions are compatible. Do not replace only a single dynamic library.
2. Compile the operator and prepare the input data according to the documentation of the operator itself, then run the operator directly to confirm that the output is as expected.
3. Run `mssanitizer --version` and `npu-smi info` to confirm that the current terminal invokes the expected tool and that the device status is normal.
4. Run detection in a writable directory. When you enable verbose debug logs, the event logs of a large operator may occupy a large amount of drive space.

Pathway event filtering requires no additional configuration. The operator compilation options and the tool support scope are still governed by the [MindStudio Sanitizer User Guide](./mssanitizer_user_guide.md).

## 3. Running Detection

The following example shows the command format. Replace `./application` with the operator program that you have already built, and append the parameters that the program requires. The program name in the example is not an executable file provided by the repository.

```bash
mssanitizer --tool=memcheck -- ./application
mssanitizer --tool=initcheck -- ./application
mssanitizer --tool=racecheck -- ./application
```

When you need to save the tool output and the detection report separately, run the following commands in a working directory that meets the data path requirements of the operator:

```bash
mkdir -p ./sanitizer-result
MSSANITIZER_ENABLE_DEBUG_LOG=1 INJ_LOG_LEVEL=1 \
  mssanitizer --tool=racecheck --log-level=info \
  --log-file=./sanitizer-result/report.log -- ./application \
  > ./sanitizer-result/tool.log 2>&1
echo "TOOL_EXIT=$?"
```

`--log-file` specifies the detection report for you to read. The tool startup information indicates the internal log location. Enabling verbose debug logs lets you view filtering statistics and does not change the default filtering rules. If the program depends on input data specified by relative paths, run it from the working directory that the program requires.

## 4. Viewing Results

When a Kernel ends, the internal log of the tool outputs the **number of events** actually removed for each consumer and Kernel, for example:

```text
Pathway filter consumer:RaceSanitizer, kernel:kernel, deviceId:0, deviceType:36, removed events:1
```

| Output | Meaning |
| --- | --- |
| `removed events:1` | This consumer removed 1 event that matches the rules in the current Kernel. The value is not a cumulative total for the entire process. |
| `removed events:0` | No event was removed in the current Kernel. A value of 0 is normal for an ordinary operator or a genuine error test case. |
| `====== ERROR:` | An exception reported by the detector. Filtering statistics and the number of errors in the report are two metrics. You cannot add them together or use one in place of the other. |
| `TOOL_EXIT=0` | The detection process exits normally. An operator that intentionally constructs an error may still produce the expected `ERROR` at the same time. |

When you run the same operator with `memcheck`, `initcheck`, and `racecheck` separately, each run is an independent detection. You cannot add up the `removed events:1` values from multiple runs to conclude that "multiple events were removed at once". A summary script may also display the same internal log repeatedly. When you check the count, view the original log of the current run.

To determine whether the feature takes effect, you must confirm all of the following at the same time: the detector has actually completed instrumentation and event collection, the change in the target alarms before and after filtering, and whether genuine exceptions that do not match the rules are still reported. Looking only at "no alarm" or "exit code 0" is not enough.

## 5. Revalidation Method

1. Use the same operator, the same input, and the same detection mode to run a control tool that does not include this feature and a tool that includes this feature separately. Each tool set should use its own complete installation directory with matching components.
2. For the target pathway event, compare the two `report.log` files, then check `removed events` in the internal log of the candidate tool. The expectation is that the target alarms disappear and the count is greater than 0.
3. Use genuine out-of-bounds, uninitialized read, and data race test cases to confirm that these errors, which do not match the rules, are still detected. Then use valid access test cases to confirm that the feature introduces no new false positives. The filtering count for such test cases is usually 0.
4. When you use a large operator to evaluate the overhead, fix the input, toolchain, and device, and perform multiple rounds of warm-up and paired measurement separately. A single on-site timing run cannot replace a formal performance conclusion.

For general operations related to operator compilation, detection modes, and anomaly reporting, see [Basic Cases](../best_practices/mssanitizer_basic_cases.md) and the [MindStudio Sanitizer User Guide](./mssanitizer_user_guide.md).

## 6. FAQ

| Symptom | Check Method |
| --- | --- |
| No filtering statistics | Confirm that you use the complete tool that includes this feature, that verbose logging is enabled, and that the Kernel ends normally, then check the internal log directory indicated by the tool. Do not treat "no statistics found" as zero removed events. |
| Target alarms still appear | Check the device, the CANN and tool versions, the original operator and input, and the detection mode. Confirm whether all six fields of the actual event match the rules. |
| Genuine errors still appear | This behavior is expected. This feature removes only target pathway events and does not mask other errors. |
| Detection reports no alarms but instrumentation fails | Check whether the tool output contains `E013`, a rollback to the original Kernel, empty collection, or an incomplete report. Do not record this case as successful filtering. |
| Large operator detection slows down or drive usage grows | Distinguish the elapsed time of the bare operator, the control tool with detection enabled, and the candidate tool. Check the size of the verbose event log and the drive writes. |

## 7. Limitations and Rollback

- The default rules cover only the combinations listed in Section 1 and do not guarantee that the feature identifies other pathways that the hardware does not support. Tool maintainers extend the rules by modifying the rule table and adding tests. The tool does not provide rule configuration on the user side.
- This feature has no independent off switch. To compare against the old behavior, rerun in an isolated environment with a **complete old version** that does not include this feature and its matching dependencies. Do not mix new and old components.
- The filter reuses the event copies that the detection consumer already has and removes matching events in place from them. This does not mean that the tool as a whole does not allocate or copy event memory.
