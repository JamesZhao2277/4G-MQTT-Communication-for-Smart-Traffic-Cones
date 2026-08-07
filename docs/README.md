# Documentation

## English Version

This directory contains the maintained protocol, integration, acceptance, and troubleshooting documentation for the current implementation.

## Recommended Reading Order

1. [Project home](../README.md): system overview and end-to-end setup.
2. [Communication protocol](protocol.md): MQTT text commands and AGV binary frames.
3. [Integration and acceptance](testing.md): staged verification and test records.
4. [M100M configuration](../hardware/4G%20module/M100M/README.md): 4G DTU setup.
5. [EMQX deployment](../server/README.md): Broker and access control.
6. [F1 firmware](../firmware/STM32%20F1%20Version/README.md): build, flash, and UART verification.
7. [PC controller](../pc%20control/README.md): dependency setup and command publication.

## Available Material

| Path | Contents |
| --- | --- |
| [`protocol.md`](protocol.md) | Protocol implemented by the current code |
| [`testing.md`](testing.md) | Staged testing, acceptance, and troubleshooting |
| [`../before get started/step1 background for the project/`](../before%20get%20started/step1%20background%20for%20the%20project/) | Project background, poster, and project list |
| [`../before get started/step2 research/`](../before%20get%20started/step2%20research/) | 4G research, BOMs, and architecture diagram |

The material under `before get started/` records design history and does not define the final implementation. The current baseline is M100M + STM32F103C8T6 + UNNC AGV.

## Maintenance Requirements

- Keep protocol documentation synchronized with the main F1 firmware and PC controller.
- Every test record should include date, commit SHA, hardware version, a sanitized configuration summary, and the conclusion.
- Check redistribution permission and privacy before publishing third-party PDFs, posters, or spreadsheets.
- Never include real passwords, tokens, private keys, IMEI values, or SIM information.
- Explain the reason for a design change instead of silently replacing the old conclusion.

Return to the [project home](../README.md).

---

# 中文版

本目录集中保存项目设计、通信协议、部署、测试和研究背景资料。

## 接手顺序

首次接手建议按以下顺序阅读：

1. [项目主页](../README.md)：理解系统链路和从零运行流程。
2. [通信协议](protocol.md)：确认 MQTT 文本命令和 AGV 二进制帧。
3. [联调与验收](testing.md)：按模块排查并记录测试结果。
4. [M100M 配置](../hardware/4G%20module/M100M/README.md)：配置 4G DTU。
5. [EMQX 部署](../server/README.md)：部署 Broker 和配置权限。
6. [F1 固件](../firmware/STM32%20F1%20Version/README.md)：编译、烧录和串口验证。
7. [PC 控制程序](../pc%20control/README.md)：安装依赖并发送命令。

## 现有资料

| 路径 | 内容 |
| --- | --- |
| [`protocol.md`](protocol.md) | 当前代码对应的通信协议 |
| [`testing.md`](testing.md) | 分段测试、验收和故障排查 |
| [`../before get started/step1 background for the project/`](../before%20get%20started/step1%20background%20for%20the%20project/) | 项目背景、海报和项目清单 |
| [`../before get started/step2 research/`](../before%20get%20started/step2%20research/) | 4G 方案调研、物料表和系统结构图 |

`before get started/` 中的原始调研资料用于保留决策背景，不代表当前最终方案。当前硬件基线以 M100M + STM32F103C8T6 + UNNC AGV 为准。

## 维护要求

- 协议文档必须与 F1 主要固件和 PC 程序保持一致。
- 测试记录应包含日期、提交号、硬件版本、配置摘要和结论。
- 第三方 PDF、海报和表格上传前需确认再分发许可及隐私信息。
- 不要在截图和文档中记录真实密码、Token、私钥、IMEI 或 SIM 卡信息。
- 设计变更应说明原因，不要只覆盖旧结论。

返回[项目主页](../README.md)。
