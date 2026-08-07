# Firmware

## English Version

This directory contains the STM32 firmware used by the smart traffic-cone communication subsystem.

## Versions

| Directory | Purpose | Status |
| --- | --- | --- |
| [`STM32 F1 Version`](./STM32%20F1%20Version/) | Final communication firmware for STM32F103C8T6 | Main version |
| [`STM32 H7 Version`](./STM32%20H7%20Version/) | Early high-performance prototype for STM32H750 | Archived prototype |

The H7 version is retained to explain the design history. Handover, deployment, and new testing should start from the F1 version.

## What Each Firmware Version Must Retain

- A directly openable Keil project and all required source code.
- The STM32CubeMX configuration file, when available.
- Exact MCU, development tool, compiler, and Device Pack versions.
- Build, flash, UART connection, and verification instructions.
- Protocol definitions synchronized with the PC and AGV sides.

## Do Not Commit

- `Objects`, `Listings`, compiler output, temporary debug files, or IDE user settings.
- Keil installers, Device Pack installers, or other redistributable large downloads.
- Nested `.git` directories left inside imported projects.
- Logs or configuration containing credentials, tokens, personal paths, or device identities.

## Release Check

Before releasing firmware, record the Git commit, build result, target hardware, wiring, and physical test result. Verified `.hex` or `.bin` artifacts may be attached to a GitHub Release; routine build output should not be committed to the source tree.

Return to the [project home](../README.md).

---

# 中文版

本目录保存交通锥控制器的 STM32 固件工程。

## 版本说明

| 目录 | 定位 | 状态 |
| --- | --- | --- |
| [`STM32 F1 Version`](./STM32%20F1%20Version/) | STM32F103C8T6 最终通信版本 | 当前主要版本 |
| [`STM32 H7 Version`](./STM32%20H7%20Version/) | STM32H750 早期高性能原型 | 原型归档 |

H7 版本用于保留早期实验和设计演进记录；当前交接与部署应优先参考 F1 主要版本。

## 代码整理原则

每个固件版本应保留：

- 可直接打开的 Keil 工程文件及必要源代码。
- STM32CubeMX 配置文件（如果存在）。
- 使用的芯片型号、开发工具和 Pack 版本。
- 编译、烧录、串口连接和验证步骤。
- 与 PC 端及 AGV 端一致的协议定义。

不要提交：

- `Objects`、`Listings`、临时调试文件和 IDE 用户配置。
- Keil 安装程序、器件包安装文件及其他可重新下载的大文件。
- 工程内部遗留的 `.git` 目录。
- 包含密码、Token、个人路径或设备凭据的日志和配置。

## 发布检查

上传新固件前，请记录对应 Git 提交、编译结果、目标硬件、接线方式和实机测试结果。正式版本建议在 GitHub Release 中附带经过验证的 `.hex` 或 `.bin` 文件，而不是把每次编译产物都提交到源码目录。

返回[项目主页](../README.md)。
