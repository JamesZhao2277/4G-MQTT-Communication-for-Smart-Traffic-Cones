# M100M 4G DTU Configuration

## English Version

This document records the wiring, MQTT setup, and verification procedure for the current M100M 4G DTU. Never store real passwords, IMEI values, SIM numbers, or public-server administration credentials in this README.

## Current Role

- Device: M100M 4G DTU.
- Variant: no GPS, DTU/transparent-transmission firmware.
- Purpose: subscribe to a command topic and output the MQTT payload unchanged to STM32 USART3.
- Implemented direction: PC -> Broker -> M100M -> STM32.

Add the exact device revision, DTU firmware version, and configuration-tool version to the handover record after checking the physical unit.

## Wiring

| M100M | STM32F103C8T6 | Purpose |
| --- | --- | --- |
| TXD | PB11 / USART3 RX | Commands from DTU to STM32 |
| RXD | PB10 / USART3 TX | Reserved for future uplink or debugging |
| GND | GND | Common ground required |

Confirm the power voltage and UART logic level from the current device label or official manual. Do not infer them from another DTU model.

## UART Settings

| Parameter | Value |
| --- | --- |
| Baud rate | 115200 |
| Data bits | 8 |
| Stop bits | 1 |
| Parity | None |
| Hardware flow control | None |
| Transfer mode | Raw UART transparent transmission |

The STM32 uses `\n` to terminate a command. The DTU must preserve the final newline and must not add a device ID, timestamp, or other prefix.

## MQTT Settings

Field names may vary by firmware, but configure at least:

| Field | Value |
| --- | --- |
| Mode | MQTT transparent transmission |
| Broker host | `<YOUR_BROKER_HOST>` |
| Broker port | `1883`, or the secure listener selected for TLS |
| Client ID | `cone-<DEVICE_ID>-dtu`, unique per device |
| Username | `<DTU_USERNAME>` |
| Password | `<DTU_PASSWORD>` |
| Subscribe topic | `cone/<DEVICE_ID>/command` |
| Publish topic | `cone/<DEVICE_ID>/status`, reserved for future uplink |
| QoS | 1 |
| Keep Alive | Follow the Broker policy and record the actual value |
| Retain | The command publisher must use false |

The historical PC script defaults to `cone1d`. To reproduce the current path unchanged, the M100M subscription must match it. For a maintained multi-device setup, migrate the PC, DTU, and ACL together to `cone/<DEVICE_ID>/command`.

## Configuration Procedure

1. Disconnect the M100M signal lines from the STM32 and AGV.
2. Insert the SIM and antenna, then power the DTU according to its hardware requirements.
3. Connect the vendor configuration tool to the correct serial port.
4. Read and back up the existing configuration. Do not publish that backup.
5. Select MQTT transparent-transmission mode.
6. Set UART to 115200, 8N1, and no flow control.
7. Enter the Broker, authentication, unique Client ID, and subscribe topic.
8. Save the settings and restart the DTU.
9. Confirm in the EMQX Dashboard that the DTU is online.
10. Connect the DTU UART to a USB-to-TTL adapter and publish a zero-value command.
11. After verification, power down and connect the STM32.

The original project used the Yinerda YEDTestTools utility. Its tutorial is recorded at:

```text
https://yinerda.yuque.com/yt1fh6/4gdtu/tl1vaqdylayghdhz
```

Do not commit the configuration-tool installer. Record the official source, version, and download date instead.

## Safe Zero-Value Test

Publish:

```text
0,0,0,0.0,0.0\n
```

The USB-to-TTL adapter should receive the exact bytes. Verify that:

- The final newline is present.
- No prefix or suffix other than an optional `\r` was added.
- The payload was not duplicated.
- Reconnecting the DTU does not replay an old retained command.

## Handover Record

The following may be recorded without including secrets:

| Item | Record |
| --- | --- |
| M100M hardware revision | To be completed |
| DTU firmware version | To be completed |
| Configuration-tool version | To be completed |
| SIM operator | Record the operator only, not the card number |
| UART | 115200 8N1 |
| MQTT transport | TCP / TLS |
| Topic convention | To be completed |
| Last verification date | YYYY-MM-DD |
| Related Git commit | Commit SHA |

## Troubleshooting

| Symptom | Check |
| --- | --- |
| Tool cannot find the DTU | USB driver, COM port, power, configuration mode, and port ownership |
| No mobile network | SIM state, antenna, signal, APN, and peak supply current |
| Network works but Broker is offline | Host, port, firewall, device time, credentials, and Client ID |
| Broker is online but no messages arrive | Topic spelling and case, slash placement, ACL, and QoS |
| UART has no output | Transparent mode, 115200 8N1, TX/RX, GND, and the final newline |
| STM32 reports invalid | Payload field count, ranges, encoding, and newline |
| Command repeats after reconnect | Clear the retained message and verify `RETAIN=False` on the PC |

Return to the [hardware guide](../../README.md).

---

# 中文版

本目录记录当前 M100M 4G DTU 的接线、MQTT 配置和验证步骤。不要在 README 中保存真实密码、IMEI、SIM 卡号或公网管理凭据。

## 当前定位

- 设备：M100M 4G DTU。
- 版本：无 GPS，DTU/透传固件。
- 用途：订阅控制 Topic，并把 MQTT Payload 原样输出到 STM32 USART3。
- 数据方向：当前主要实现 PC -> Broker -> M100M -> STM32。

设备标签、固件版本和配置工具版本应在现场交接记录中补充。

## 接线

| M100M | STM32F103C8T6 | 说明 |
| --- | --- | --- |
| TXD | PB11 / USART3 RX | DTU 向 STM32 下发命令 |
| RXD | PB10 / USART3 TX | 为后续上行或调试保留 |
| GND | GND | 必须共地 |

电源电压和串口电平必须根据当前设备标签或厂商手册确认，不要从其他型号 DTU 的资料推断。

## UART 配置

| 参数 | 值 |
| --- | --- |
| 波特率 | 115200 |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验位 | None |
| 硬件流控 | None |
| 传输方式 | 原始 UART 透传 |

STM32 使用换行符 `\n` 判断一条命令结束，因此 DTU 必须保留 Payload 末尾的换行符，不能添加设备编号、时间戳或其他前缀。

## MQTT 配置

配置工具中的字段名称可能随固件变化，但至少需要填写：

| 字段 | 应填写的内容 |
| --- | --- |
| Mode | MQTT transparent transmission |
| Broker host | `<YOUR_BROKER_HOST>` |
| Broker port | `1883`，或按 TLS 部署使用安全端口 |
| Client ID | `cone-<DEVICE_ID>-dtu`，每台设备唯一 |
| Username | `<DTU_USERNAME>` |
| Password | `<DTU_PASSWORD>` |
| Subscribe topic | `cone/<DEVICE_ID>/command` |
| Publish topic | `cone/<DEVICE_ID>/status`，当前主链路暂未使用 |
| QoS | 1 |
| Keep Alive | 按服务器策略配置，建议记录实际值 |
| Retain | 命令发布端必须为 false |

当前历史 PC 脚本默认 Topic 是 `cone1d`。若要原样复现现有链路，M100M 订阅值必须与它一致；整理仓库时建议把 PC、DTU 和 ACL 一起迁移到 `cone/<DEVICE_ID>/command`。

## 配置步骤

1. 断开 M100M 与 STM32、AGV 的信号连接。
2. 插入 SIM 卡和天线，按设备要求供电。
3. 使用厂商配置工具连接正确串口。
4. 读取并备份当前配置，备份文件不得包含在公共仓库中。
5. 选择 MQTT 透传模式。
6. 设置 UART 为 115200、8N1、无流控。
7. 填写 Broker、认证、唯一 Client ID 和订阅 Topic。
8. 保存配置并重启 DTU。
9. 在 EMQX Dashboard 确认 DTU 在线。
10. 将 DTU UART 接入 USB-TTL，发布零值命令并检查原始输出。
11. 验证通过后断电，再连接 STM32。

本地原始资料使用银尔达 YEDTestTools。工具使用教程记录在：

```text
https://yinerda.yuque.com/yt1fh6/4gdtu/tl1vaqdylayghdhz
```

配置工具安装包不建议提交到 Git 仓库，应记录官方来源、版本和下载日期。

## 安全零值测试

PC 发布：

```text
0,0,0,0.0,0.0\n
```

USB-TTL 应收到完全相同的字节。检查：

- 没有丢失末尾换行符。
- 没有额外 `\r` 以外的前缀或后缀。
- 没有重复发送。
- DTU 断线重连后不会自动下发旧的 retained command。

## 配置交接记录

以下内容可以填写，但不得包含密码或设备唯一隐私信息：

| 项目 | 记录 |
| --- | --- |
| M100M 硬件版本 | 待填写 |
| DTU 固件版本 | 待填写 |
| 配置工具版本 | 待填写 |
| SIM 运营商 | 待填写，不写卡号 |
| UART | 115200 8N1 |
| MQTT 传输 | TCP / TLS，待填写 |
| Topic 规则 | 待填写 |
| 最后验证日期 | YYYY-MM-DD |
| 对应 Git 提交 | commit SHA |

## 故障排查

| 现象 | 检查项 |
| --- | --- |
| 配置工具找不到设备 | USB 驱动、COM 端口、供电、配置模式和串口占用 |
| 无移动网络 | SIM 状态、天线、信号、APN 和供电峰值 |
| 能联网但 Broker 离线 | 域名/IP、端口、防火墙、系统时间、账号和 Client ID |
| Broker 在线但无订阅数据 | Topic 大小写、斜杠、ACL 和 QoS |
| UART 无输出 | 透传模式、115200 8N1、TX/RX、GND 和消息换行 |
| STM32 显示 invalid | Payload 字段数、范围、编码和换行符 |
| 重连后重复命令 | 清除 retained message，并确认 PC 使用 `RETAIN=False` |

返回[硬件说明](../../README.md)。
