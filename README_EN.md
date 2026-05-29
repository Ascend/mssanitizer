<h1 align="center">MindStudio Sanitizer</h1>

<div align="center">
<h2>Ascend AI Operator Exception Check Tool</h2>
  
 [![Ascend](https://img.shields.io/badge/Community-MindStudio-blue.svg)](https://www.hiascend.com/developer/software/mindstudio) 
 [![License](https://badgen.net/badge/License/MulanPSL-2.0/blue)](./LICENSE)

</div>

## ✨ Latest Updates

<span style="font-size:14px;">

🔹 **[2025.12.31]**: MindStudio Sanitizer is fully open-sourced.

</span>

## ️ ℹ️ Overview

MindStudio Sanitizer (msSanitizer) is a single-operator exception check tool designed for Ascend AI Processors and can check the following issues: memory overwriting, data race, uninitialized access, and synchronization exceptions.

<div align="center">
  <h4>▶️ Quick demo</h4>
  <img src="./docs/en/figures/demo-sanitizer.gif" alt="Quick demo " width="600">
  <p><sup>Figure: Operator memory, uninitialized access, and race check process demonstration</sup></p>
</div>

## ⚙️ Functions

msSanitizer provides different exception check capabilities through multiple sub-modules. The following functions are supported:

| Function| Description |
|---------|--------|
| **Memory check**| Checks memory exceptions such as out-of-bounds access and unaligned access in the global memory and local memory.|
| **Race check**| Checks data race issues caused by concurrent memory access in a parallel computing environment.|
| **Uninitialization check** | Checks memory read exceptions caused by the use of uninitialized variables.|
| **Synchronization check**  |Checks for unpaired `SetFlag`/`WaitFlag` instructions in Ascend C operators.|

## 🚀 Quick Start

For details about how to quickly experience core functions through a simple addition operator example, see [msSanitizer Quick Start](./docs/en/quick_start/mssanitizer_quick_start.md).

## 📦 Installation Guide

For details about the environment dependencies and installation methods of the tool, see [msSanitizer Installation Guide](docs/en/install_guide/mssanitizer_install_guide.md).

## 📘 User Guide

For details about how to use the tool, see [msSanitizer User Guide](docs/en/user_guide/mssanitizer_user_guide.md).

## 💡 Typical Cases

For details about how to understand and use the tool through typical cases, see [msSanitizer Typical Cases](docs/en/best_practices/basic_cases.md).

## 📚 API Reference

For details about the APIs, including sanitizer APIs and MSTX APIs, see [msSanitizer API Reference](docs/en/api_reference/mssanitizer_api_reference.md).

## 💬 FAQs

For details about common issues and solutions, see [msSanitizer FAQs](docs/en/support/faq.md).

## 🛠️ Contribution Guide

You are welcome to contribute to the project. For details, see [Contribution Guide](./docs/en/contributing/contributing_guide.md). 

## ⚖️ Related Information

🔹 [Release Notes](./docs/en/release_notes/release_notes.md) 
🔹 [License Notice](./docs/en/legal/license_notice.md) 
🔹 [Security Statement](./docs/en/legal/security_statement.md) 
🔹 [Disclaimer](./docs/en/legal/disclaimer.md) 

## 🤝 Suggestions and Communication

You are welcome to contribute to the community. If you have any questions or suggestions, please submit an [issue](https://gitcode.com/Ascend/mssanitizer/issues). We will reply as soon as possible. Thank you for your support.

|                                      📱 Follow the MindStudio WeChat Account                                      | 💬 More Communication and Support                                                                                                                                                                                                                                                                                                                                                                                                                    |
|:-----------------------------------------------------------------------------------------------:|:-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| <img src="https://gitcode.com/Ascend/msot/blob/master/docs/zh/figures/readme/officialAccount.png" width="120"><br><sub>*Scan the QR code to follow us and get the latest updates.*</sub>| 💡 **Join the WeChat group**:<br>Follow the WeChat account and reply "communication group" to obtain the QR code for joining the group.<br><br>🛠️ ️**Other channels**:<br><br>|

## 🙏 Acknowledgements

This tool is jointly developed by the following Huawei departments:   
🔹 Ascend Computing MindStudio Development Department 
🔹 Ascend Computing Ecosystem Enablement Department 
🔹 Huawei Cloud AI Compute Service 
🔹 Compiler Technologies Lab, 2012 Labs 
🔹 Markov Lab, 2012 Labs 
Thank you to everyone in the community for your PRs. We warmly welcome your contributions.
