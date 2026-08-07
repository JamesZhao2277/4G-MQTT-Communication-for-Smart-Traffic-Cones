# Before You Get Started

## English Version

This directory preserves the project background, early research, procurement records, and the original system diagram. It explains how the final technical direction was selected, but it is not the current deployment guide.

## System Context

The implemented command path is:

```text
PC controller -> MQTT Broker -> 4G DTU -> STM32 -> AGV chassis
```

The STM32 receives text commands forwarded by the DTU, validates them, and converts them into CRC16-protected AGV control frames.

## Contents

| Directory | Contents |
| --- | --- |
| `step1 background for the project/` | Project poster and the undergraduate research project list |
| `step2 research/` | 4G module research, procurement lists, architecture diagram, and the early Air780E proposal |

These files retain the decision history. The current hardware baseline is M100M + STM32F103C8T6 + UNNC AGV, so early Air780 and H7 materials must not be treated as final deployment instructions.

## Reading Order

1. Read the [root README](../README.md) for the current end-to-end setup.
2. Use the background files to understand the project goal.
3. Use the research files to understand hardware-selection decisions.
4. Follow the current [hardware guide](../hardware/README.md), [protocol](../docs/protocol.md), and [test guide](../docs/testing.md) for implementation.

## Maintenance Rules

- Keep historical files unchanged unless correcting an objective error.
- Record source, version, and acquisition date for third-party material.
- Check redistribution permission before publishing posters, spreadsheets, manuals, or software.
- Remove personal information, credentials, tokens, device identities, and private server details.
- When a research conclusion changes the current design, update the root README and the affected module documentation.

Return to the [project home](../README.md).

---

# 中文版

本目录用于保存项目交接、系统设计、通信协议、部署和测试文档。

## 系统概览

当前通信链路为：

```text
PC 控制程序 -> MQTT Broker -> 4G DTU -> STM32 -> AGV 底盘
```

STM32 负责接收 DTU 下发的文本指令，检查指令内容，并转换为带 CRC16 校验的 AGV 控制帧。

## 建议文档

后续整理资料时，建议逐步补充以下文件：

| 文件 | 内容 |
| --- | --- |
| `architecture.md` | 系统组成、数据流和模块职责 |
| `mqtt.md` | Broker、Topic、QoS 和客户端配置 |
| `protocol.md` | PC 指令格式及 STM32/AGV 二进制帧格式 |
| `deployment.md` | Broker、DTU、固件和 PC 端部署步骤 |
| `testing.md` | 单模块测试、联调步骤和验收结果 |
| `troubleshooting.md` | 常见故障、日志位置和排查方法 |

原始 PDF、数据手册或外部厂商文档应注明来源、版本和获取链接，并在上传前确认允许重新分发。

## 文档维护

- 文档中的命令、引脚和参数应与当前代码版本一致。
- 不要记录真实密码、Token、私钥或个人账号。
- 截图需隐藏公网地址、账号和设备唯一标识等敏感信息。
- 重大设计变更应同时更新根目录 `README.md` 和对应专题文档。

返回[项目主页](../README.md)。
