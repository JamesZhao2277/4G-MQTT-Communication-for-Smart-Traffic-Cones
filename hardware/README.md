# Hardware

## English Version

This directory contains the current hardware baseline, wiring information, AGV interfaces, expansion-board files, and historical 4G research material.

## Current Baseline

| Module | Current Choice | Role |
| --- | --- | --- |
| 4G DTU | M100M without GPS, DTU/transparent firmware | Subscribe to MQTT commands and forward them over UART |
| Main controller | STM32F103C8T6 | Validate text commands and build AGV control frames |
| Mobile platform | UNNC AGV | Execute motor and steering commands |
| Debug equipment | ST-Link and USB-to-TTL adapters | Flashing and UART diagnosis |

The AIR780 MINI package is historical research material, not the final delivery hardware.

## Wiring

### M100M to STM32

| M100M | STM32F103C8T6 | Parameters |
| --- | --- | --- |
| TXD | PB11 / USART3 RX | 115200, 8N1 |
| RXD | PB10 / USART3 TX | 115200, 8N1 |
| GND | GND | Common ground required |

See the [M100M guide](./4G%20module/M100M/README.md).

### STM32 to AGV

| STM32F103C8T6 | AGV | Parameters |
| --- | --- | --- |
| PA2 / USART2 TX | AGV UART RX | 460800, 8N1 |
| PA3 / USART2 RX | AGV UART TX | 460800, 8N1 |
| GND | GND | Common ground required |

Use the [AGV hardware guide](./interface%20specification/AGV%20硬件说明.pdf) and [AGV secondary-development interface](./interface%20specification/AGV%20二次开发接口.pdf) as the authoritative references.

### Debug UART

| STM32F103C8T6 | USB-to-TTL | Parameters |
| --- | --- | --- |
| PA9 / USART1 TX | RX | 115200, 8N1 |
| PA10 / USART1 RX | TX, optional | 115200, 8N1 |
| GND | GND | Common ground required |

## Power and Safety

- Disconnect power before changing wiring; inspect for shorts and reversed polarity before first power-up.
- Never infer pin assignments from wire color.
- Do not connect different voltage-domain supply pins without checking the relevant hardware manuals.
- Avoid powering the same rail from both the target board and a USB-to-TTL adapter.
- Lift the AGV drive wheels or use an isolated test area, and keep the physical emergency stop available.

## Directory Guide

| Path | Contents |
| --- | --- |
| [`4G module/M100M/`](./4G%20module/M100M/README.md) | Current M100M setup and verification |
| `4G module/AIR780 MINI资料包 V1.2/` | Historical research package; must be screened before publication |
| `expansion board/` | Expansion-board design and Gerber files |
| `interface specification/` | AGV manuals, protocol PDFs, and UART SDK |
| [Second BOM](../before%20get%20started/step2%20research/第二次物料表.xlsx) | Current-stage procurement record |

## Pre-Publication Screening

- Remove token logs, accounts, IMEI values, SIM information, and personal paths.
- Check licenses before publishing third-party software, archives, or vendor documents.
- For downloadable tools, retain the version, checksum, and official source rather than the installer.
- Label wiring diagrams and photographs with their hardware revision and date.

Return to the [project home](../README.md).

---

# 中文版

本目录保存当前硬件基线、接线信息、AGV 接口、扩展板文件和历史 4G 方案资料。

## 当前硬件基线

| 模块 | 当前选择 | 作用 |
| --- | --- | --- |
| 4G DTU | M100M，无 GPS，DTU/透传固件 | 订阅 MQTT 命令并通过 UART 透传 |
| 主控制器 | STM32F103C8T6 | 校验文本命令并生成 AGV 控制帧 |
| 移动平台 | UNNC AGV | 执行电机与舵轮控制 |
| 调试设备 | ST-Link、USB-TTL | 烧录与串口诊断 |

AIR780 MINI 资料是早期调研和测试资料，不代表最终交付硬件。

## 接线

### M100M 到 STM32

| M100M | STM32F103C8T6 | 参数 |
| --- | --- | --- |
| TXD | PB11 / USART3 RX | 115200, 8N1 |
| RXD | PB10 / USART3 TX | 115200, 8N1 |
| GND | GND | 必须共地 |

详细配置参见 [M100M README](./4G%20module/M100M/README.md)。

### STM32 到 AGV

| STM32F103C8T6 | AGV | 参数 |
| --- | --- | --- |
| PA2 / USART2 TX | AGV UART RX | 460800, 8N1 |
| PA3 / USART2 RX | AGV UART TX | 460800, 8N1 |
| GND | GND | 必须共地 |

AGV 接口以 [AGV 硬件说明](./interface%20specification/AGV%20硬件说明.pdf) 和 [AGV 二次开发接口](./interface%20specification/AGV%20二次开发接口.pdf) 为准。

### 调试口

| STM32F103C8T6 | USB-TTL | 参数 |
| --- | --- | --- |
| PA9 / USART1 TX | RX | 115200, 8N1 |
| PA10 / USART1 RX | TX，可选 | 115200, 8N1 |
| GND | GND | 必须共地 |

## 供电与安全

- 修改接线前断电，首次上电前检查短路、反接和公共地。
- 不要根据线材颜色判断引脚。
- 不要把不同电压域的电源脚直接相连；以硬件手册和实测为准。
- 串口调试器和目标板同时供电时，避免重复向同一电源轨供电。
- AGV 测试时抬起驱动轮或使用隔离区域，并保证物理急停可用。

## 目录说明

| 路径 | 内容 |
| --- | --- |
| [`4G module/M100M/`](./4G%20module/M100M/README.md) | 当前 M100M 配置和验证步骤 |
| `4G module/AIR780 MINI资料包 V1.2/` | 早期调研资料，发布前需筛选 |
| `expansion board/` | 扩展板设计和 Gerber |
| `interface specification/` | AGV 硬件说明、协议 PDF 和 UART SDK |
| [第二次物料表](../before%20get%20started/step2%20research/第二次物料表.xlsx) | 当前阶段物料记录 |

## 发布前筛选

- 删除 Token 日志、账号、IMEI、SIM 信息和个人路径。
- 第三方软件、压缩包和厂商资料确认许可后再发布。
- 可重新下载的软件只保留版本、校验值和官方链接。
- 接线图和实物照片应标明硬件版本及拍摄日期。

返回[项目主页](../README.md)。
