# STM32 H7 Prototype Firmware

## English Version

This directory archives an early high-performance prototype based on the **STM32H750 (H7 family)**. It is not the final delivery firmware.

## Purpose

The H7 project records early hardware selection, communication experiments, and software evolution. A maintainer should confirm the exact target through:

- The Keil Target Device.
- The MCU selected in the STM32CubeMX `.ioc` file.
- Startup files, linker settings, and CMSIS Device directories.
- The actual chip marking on the board.

## Archive Requirements

- Explain what the prototype tested and why it was not selected as the final version.
- Record whether it builds, its toolchain version, and known issues.
- Do not mix F1 production build output into this directory.
- Do not commit Keil installers, Device Packs, caches, or nested `.git` directories.
- If it does not build, preserve the relevant error summary and do not claim that it is verified.

Return to the [firmware index](../README.md).

---

# 中文版

本目录用于保存基于 **STM32H750（H7 系列）** 的早期高性能原型代码，目前不应视为最终交付版本。

## 定位

该版本用于记录前期硬件选型、通信实验和软件设计演进。接手人员应通过以下位置再次确认具体器件与工程配置：

- Keil 工程的 Target Device。
- STM32CubeMX `.ioc` 文件中的 MCU 型号。
- 启动文件、链接脚本和 CMSIS Device 目录。
- 实际开发板芯片丝印。

## 归档要求

- 说明该版本的实验目的，以及未继续采用的原因。
- 标明可编译状态、工具链版本和已知问题。
- 不要混入 F1 最终版本的构建产物。
- 不要提交 Keil 安装程序、Device Pack、缓存或嵌套 `.git` 目录。
- 如无法编译，应保留原始错误信息并在此说明，不要声称已经验证。

返回[固件目录](../README.md)。
