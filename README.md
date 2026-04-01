<h1 align="center">MindStudio Sanitizer</h1>

<div align="center">
<h2>昇腾 AI 算子异常检测工具</h2>
  
 [![Ascend](https://img.shields.io/badge/Community-MindStudio-blue.svg)](https://www.hiascend.com/developer/software/mindstudio) 
 [![License](https://badgen.net/badge/License/MulanPSL-2.0/blue)](./LICENSE)

</div>

## ✨ 最新消息

<span style="font-size:14px;">

🔹 **[2025.12.31]**：MindStudio Sanitizer 项目全面开源

</span>

## ️ ℹ️ 简介

MindStudio Sanitizer（算子异常检测，msSanitizer）是专为昇腾 AI 处理器打造的单算子异常检测工具，提供内存越界、数据竞争、未初始化访问及同步异常四大检测能力。

## ⚙️ 功能介绍

msSanitizer 通过多个子功能模块提供不同类型的异常检测能力，当前已支持的功能如下：

| 功能名称 | 功能描述  |
|---------|--------|
| **内存检测** | 检测 Global Memory 与 Local Memory 中的越界访问、未对齐访问等内存异常。 |
| **竞争检测** | 检测并行计算环境中因并发内存访问引发的数据竞争问题。 |
| **未初始化检测**  | 检测因使用未初始化变量导致的内存读取异常。 |
| **同步检测**   |检测 Ascend C 算子中未配对的 SetFlag/WaitFlag 指令。 |

## 🚀 快速入门

以简易加法算子为例，快速体验核心功能，请参见 [《msSanitizer 快速入门》](./docs/zh/quick_start/mssanitizer_quick_start.md)。

## 📦 安装指南

介绍工具的环境依赖与安装方法，请参见 [《msSanitizer 安装指南》](docs/zh/install_guide/mssanitizer_install_guide.md)。

## 📘 使用指南

工具的详细使用方法，请参见 [《msSanitizer 使用指南》](docs/zh/user_guide/mssanitizer_user_guide.md)

## 💡 典型案例

通过典型问题场景帮助用户理解并掌握工具使用，请参见 [《msSanitizer 典型案例》](docs/zh/best_practices/basic_cases.md)。

## 📚 API参考

包含sanitizer接口和mstx扩展接口两种类型，请参见 [《msSanitizer 对外接口使用说明》](docs/zh/api_reference/mssanitizer_api_reference.md)。

## ❓ FAQ

常见问题及解决方案，请参见 [《msSanitizer FAQ》](docs/zh/support/faq.md)。

## 🛠️ 贡献指南

欢迎参与项目贡献，请参见 [《贡献指南》](./docs/zh/contributing/contributing_guide.md)。  

## ⚖️ 相关说明

🔹 [《版本说明》](./docs/zh/release_notes/release_notes.md)  
🔹 [《许可证声明》](./docs/zh/legal/license_notice.md)  
🔹 [《安全声明》](./docs/zh/legal/security_statement.md)  
🔹 [《免责声明》](./docs/zh/legal/disclaimer.md)  

## 🤝 建议与交流

欢迎大家为社区做贡献。如果有任何疑问或建议，请提交[Issues](https://gitcode.com/Ascend/mssanitizer/issues)，我们会尽快回复。感谢您的支持。

|                                      📱 关注 MindStudio 公众号                                       | 💬 更多交流与支持                                                                                                                                                                                                                                                                                                                                                                                                                     |
|:-----------------------------------------------------------------------------------------------:|:-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| <img src="https://gitcode.com/Ascend/msot/blob/master/docs/zh/figures/readme/officialAccount.png" width="120"><br><sub>*扫码关注获取最新动态*</sub> | 💡 **加入微信交流群**：<br>关注公众号，回复“交流群”即可获取入群二维码。<br><br>🛠️ **其他渠道**：<br>👉 昇腾助手：[![WeChat](https://img.shields.io/badge/WeChat-07C160?style=flat-square&logo=wechat&logoColor=white)](https://gitcode.com/Ascend/msot/blob/master/docs/zh/figures/readme/xiaozhushou.png)<br>👉 昇腾论坛：[![Website](https://img.shields.io/badge/Website-%231e37ff?style=flat-square&logo=RSS&logoColor=white)](https://www.hiascend.com/forum/) |

## 🙏 致谢

本工具由华为公司的下列部门联合贡献：    
🔹 昇腾计算MindStudio开发部  
🔹 昇腾计算生态使能部  
🔹 华为云昇腾云服务  
🔹 2012编译器实验室  
🔹 2012马尔科夫实验室  
感谢来自社区的每一个 PR，欢迎贡献！
