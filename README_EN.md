<h1 align="center">MindStudio Sanitizer</h1>

<div align="center">
<p><b><span style="font-size:24px;">Ascend AI Operator Exception Check Tool</span></b></p>

 [![Quick Start](https://badgen.net/badge/Quick%20Start/QuickStart/blue)](./docs/en/quick_start/mssanitizer_quick_start.md)
 [![Exact Search](https://badgen.net/badge/Exact%20Search/ReadTheDocs/blue)](https://mindstudio-operator-tools-docs.readthedocs.io/zh-cn/latest/)
 [![AI Q&A (DeepWiki)](https://badgen.net/badge/AI%20Q%26A/DeepWiki/blue)](https://deepwiki.com/mindstudio-docs/26.2.0)
 [![AI Q&A (ZRead)](https://badgen.net/badge/AI%20Q%26A/ZRead/blue)](https://zread.ai/mindstudio-docs/26.2.0)
 [![Ascend Community](https://badgen.net/badge/Ascend%20Community/Community/blue)](https://www.hiascend.com/eng/developer/software/mindstudio)
 [![Report Issues](https://badgen.net/badge/Report%20Issues/Issues/blue)](https://atomgit.com/Ascend/mssanitizer/issues)

</div>

English | [简体中文](./README.md)

## ✨ Latest Updates

<span style="font-size:14px;">

🔹 **[Dec 31, 2025]**: MindStudio Sanitizer is fully open-sourced.

</span>

## ️ ℹ️ Overview

MindStudio Sanitizer (msSanitizer) is a single-operator exception check tool designed for Ascend AI Processors and can check the following issues: memory overwriting, data race, uninitialized access, and synchronization exceptions.

<div align="center">
  <h4>▶️ Quick Demo</h4>
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

The tool supports online installation, offline installation, and source code installation. For details, see the [msSanitizer Installation Guide](docs/en/install_guide/mssanitizer_install_guide.md).

## 📘 User Guide

For details about how to use the tool, see the [msSanitizer User Guide](docs/en/user_guide/mssanitizer_user_guide.md).

## 💡 Typical Cases

For details about how to understand and use the tool through typical cases, see [msSanitizer Typical Cases](docs/en/best_practices/mssanitizer_basic_cases.md).

## 📚 API Reference

For details about the APIs, including sanitizer APIs and msTX APIs, see [msSanitizer API Reference](docs/en/api_reference/mssanitizer_api_reference.md).

## 💬 FAQs

For details about common issues and solutions, see [msSanitizer FAQs](docs/en/support/mssanitizer_faq.md).

## 🌌 Smart Search

To improve documentation search efficiency, we provide multiple efficient search methods:

- **[Precise Search (ReadTheDocs)](https://mindstudio-operator-tools-docs.readthedocs.io/zh-cn/latest/)**: Performs millisecond-level structured search across all documents to precisely locate underlying configuration and API details.
- **[AI Q&A (DeepWiki)](https://deepwiki.com/mindstudio-docs/26.2.0)**: A context-based AI development assistant that answers natural language questions within seconds.

## 🛠️ Contribution Guide

You are welcome to contribute to the project. For details, see the [Contribution Guide](./docs/en/contributing/contributing_guide.md).

## ⚖️ Related Information

🔹 [Release Notes](https://atomgit.com/Ascend/mssanitizer/releases)  
🔹 [License Notice](./docs/en/legal/license_notice.md)  
🔹 [Security Statement](./docs/en/legal/security_statement.md)  
🔹 [Disclaimer](./docs/en/legal/disclaimer.md)

## 🤝 Suggestions and Communication

You are welcome to contribute to the community. If you have any questions or suggestions, please submit an [issue](https://atomgit.com/Ascend/mssanitizer/issues). We will reply as soon as possible. Thank you for your support.

| Instant Interaction (WeChat Group) | Official Updates (WeChat Account) | In-depth Support (Assistant/Forum) |
|:--:|:--:|:--:|
| <img src="https://raw.atomgit.com/Ascend/docs/files/master/common/Writing_Template/figures/qr_code_wechat_work.png" width="120"><br><sub>*Scan to join the technical discussion group*</sub> | <img src="https://raw.atomgit.com/Ascend/docs/files/master/common/Writing_Template/figures/qr_code_wechat_official_account.png" width="120"><br><sub>*Scan to follow the official WeChat account*</sub> | Scan to join the group and follow the account to access the fastest communication platform for MindStudio users and developers: <br> **Ask questions:** Discuss technical issues with community peers in real time<br>**Stay informed:** Get version release and feature update notifications as soon as they are released<br> **Share experience:** Exchange best practices and hands-on insights with developers  <br> <br> **More support channels**: 👉 Ascend Assistant: [![WeChat](https://img.shields.io/badge/WeChat-07C160?style=flat-square&logo=wechat&logoColor=white)](https://atomgit.com/Ascend/msit/blob/master/docs/zh/figures/readme/xiaozhushou.png) 👉 Ascend Forum: [![Website](https://img.shields.io/badge/Website-%231e37ff?style=flat-square&logo=RSS&logoColor=white)](https://www.hiascend.com/forum/) |

## 🙏 Acknowledgements

This tool is jointly contributed by Huawei Technologies Co., Ltd. and developers.

Thanks for every PR from the community, and contributions are welcome.
