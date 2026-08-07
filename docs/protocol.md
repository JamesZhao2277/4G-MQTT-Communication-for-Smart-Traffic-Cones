# Communication Protocol

## English Version

This document describes the two protocol layers implemented by the main F1 firmware:

1. Newline-delimited text commands from PC/EMQX/M100M to STM32.
2. CRC16-protected binary frames from STM32 to the AGV.

The code is the final source of truth. Any protocol change must update the PC program, F1 firmware, and this document together.

## MQTT Command

### Topic

The PC publish topic must exactly match the M100M subscribe topic.

- Historical script default: `cone1d`.
- Recommended multi-device format: `cone/<device-id>/command`.

When changing the topic, update the PC, DTU, and server ACL together. Client IDs must be unique.

### Publish Settings

| Parameter | Current Setting |
| --- | --- |
| QoS | 1 |
| Retain | false |
| Keep Alive | The PC currently uses 60 seconds |
| Encoding | ASCII/UTF-8-compatible numbers and punctuation |
| Message terminator | Newline `\n` |

Never retain a motion command. Otherwise, a reconnecting device may execute an old command.

### Payload

```text
left_pwm,right_pwm,pulses,angle_left,angle_right\n
```

| Field | Type | Firmware Range | Current Meaning |
| --- | --- | ---: | --- |
| `left_pwm` | Integer | -100 to 100 | Signed left-motor control value; 0 means stop |
| `right_pwm` | Integer | -100 to 100 | Signed right-motor control value; 0 means stop |
| `pulses` | Integer | 0 to 100000 | Pulse count sent to the AGV |
| `angle_left` | Float | -90 to 90 | Left steering angle in degrees |
| `angle_right` | Float | -90 to 90 | Right steering angle in degrees |

The current F1 comments define 0 degrees as straight ahead and clockwise as positive. Verify the physical direction with a small-angle controlled test before formal operation.

Safe zero-value example:

```text
0,0,0,0.0,0.0\n
```

### Parsing Rules

- Every command must contain exactly five fields.
- Commas separate fields.
- `\r` is ignored; `\n` triggers line parsing.
- Whitespace around numbers is accepted, but senders should not rely on it.
- The line buffer is 96 bytes; an overlong line is discarded.
- Invalid syntax or range prints `[PC CMD invalid]` on USART1 and does not produce an AGV frame.
- Buffer problems print `[PC CMD overflow]` or `[USART3 RX overflow]`.

## STM32 to AGV

### UART

| Parameter | Value |
| --- | --- |
| STM32 peripheral | USART2 |
| Pins | PA2 TX / PA3 RX |
| Baud rate | 460800 |
| Data format | 8N1 |
| Flow control | None |

### Frame Layout

| Order | Length | Field | Value or Meaning |
| ---: | ---: | --- | --- |
| 1 | 2 bytes | Magic | `AA 55` |
| 2 | 2 bytes | Payload length | Little-endian |
| 3 | 1 byte | Type ID | `11` for control command |
| 4 | 1 byte | Version | `01` |
| 5 | N bytes | Payload | `ctrl_frame_t` |
| 6 | 2 bytes | CRC16 | Little-endian |
| 7 | 2 bytes | Tail | `0D 0A` |

The CRC covers every byte from `AA 55` through the end of the payload. It excludes the CRC field and tail.

| CRC Parameter | Value |
| --- | --- |
| Algorithm | CRC16-CCITT |
| Polynomial | `0x1021` |
| Initial value | `0xFFFF` |

### Populated Control Fields

The STM32 maps the five text fields into `ctrl_frame_t`:

- `angle_left` and `angle_right` are copied to the steering-angle fields.
- `angle_camera` is fixed at 0.
- Chassis lock, both motor brake bits, and differential lock are fixed at 0.
- `left_pwm` and `right_pwm` are copied to the left/right motor-control fields.
- `pulses` is copied to the pulse field.
- LED-control fields remain 0.

The structure contains 32-bit floating-point values and C bit-fields. It works with the current ARM Cortex-M/Keil toolchain, but structure and bit-field layout may change with the compiler, options, or MCU. Capture and validate the binary frame after any such change.

## Not Implemented

- AGV status-feedback parsing and MQTT uplink.
- Application-level ACK, sequence numbers, and duplicate detection.
- Heartbeat, command expiry, and automatic stop on link loss.
- An independent remote emergency-stop safety channel.
- Message signing, TLS, and production-grade device identity management.

Until these controls are implemented, operate the system only in a controlled research environment.

## Code References

- PC sender: [`pc_auto_sender.py`](../pc%20control/Python_Control/pc_send/pc_auto_sender.py)
- F1 entry point: [`main.c`](../firmware/STM32%20F1%20Version/f1version%20program/F1/Core/Src/main.c)
- Frame configuration: [`uart_frame_config.h`](../firmware/STM32%20F1%20Version/f1version%20program/F1/Core/Inc/uart_frame_config.h)
- CRC configuration: [`uart_frame_crc.h`](../firmware/STM32%20F1%20Version/f1version%20program/F1/Core/Inc/uart_frame_crc.h)

Return to the [documentation index](README.md).

---

# 中文版

本文档描述当前 F1 主要固件实际实现的两层协议：

1. PC/EMQX/M100M 到 STM32 的换行分隔文本命令。
2. STM32 到 AGV 的带 CRC16 二进制帧。

代码是最终事实来源。修改协议时，应同步更新 PC 程序、F1 固件和本文档。

## MQTT 命令

### Topic

PC 发布 Topic 必须与 M100M 订阅 Topic 完全一致。

- 当前历史脚本默认值：`cone1d`。
- 推荐多设备格式：`cone/<device-id>/command`。

迁移 Topic 时必须同时修改 PC、DTU 和服务器 ACL。Client ID 不能重复。

### 发布参数

| 参数 | 当前设置 |
| --- | --- |
| QoS | 1 |
| Retain | false |
| Keep Alive | PC 当前使用 60 秒 |
| 编码 | ASCII/UTF-8 兼容的数字与标点 |
| 消息结束 | 换行符 `\n` |

禁止把运动命令设置为 Retained Message，否则设备重连后可能执行旧命令。

### Payload

```text
left_pwm,right_pwm,pulses,angle_left,angle_right\n
```

| 字段 | 类型 | 固件允许范围 | 当前含义 |
| --- | --- | ---: | --- |
| `left_pwm` | 整数 | -100 到 100 | 左电机有符号控制值，0 为停止 |
| `right_pwm` | 整数 | -100 到 100 | 右电机有符号控制值，0 为停止 |
| `pulses` | 整数 | 0 到 100000 | 发送给 AGV 的脉冲数量 |
| `angle_left` | 浮点数 | -90 到 90 | 左舵轮角度，单位为度 |
| `angle_right` | 浮点数 | -90 到 90 | 右舵轮角度，单位为度 |

当前 F1 注释约定 0 度为车头正前方、顺时针为正。正式实机运行前必须用小角度命令确认机械方向，不能只依赖符号约定。

安全零值示例：

```text
0,0,0,0.0,0.0\n
```

### 解析规则

- 每条命令必须恰好包含五个字段。
- 逗号用于分隔字段。
- `\r` 被忽略，`\n` 触发整行解析。
- 数字前后允许空格，但不建议依赖这一行为。
- 行缓冲区大小为 96 字节；过长输入会被丢弃。
- 格式错误或越界时，USART1 输出 `[PC CMD invalid]`，不会发送 AGV 控制帧。
- 缓冲区溢出时输出 `[PC CMD overflow]` 或 `[USART3 RX overflow]`。

## STM32 到 AGV

### 串口

| 参数 | 值 |
| --- | --- |
| STM32 外设 | USART2 |
| 引脚 | PA2 TX / PA3 RX |
| 波特率 | 460800 |
| 数据格式 | 8N1 |
| 流控 | 无 |

### 帧布局

| 顺序 | 长度 | 字段 | 值或说明 |
| ---: | ---: | --- | --- |
| 1 | 2 bytes | Magic | `AA 55` |
| 2 | 2 bytes | Payload length | 小端序 |
| 3 | 1 byte | Type ID | 控制命令为 `11` |
| 4 | 1 byte | Version | `01` |
| 5 | N bytes | Payload | `ctrl_frame_t` |
| 6 | 2 bytes | CRC16 | 小端序 |
| 7 | 2 bytes | Tail | `0D 0A` |

CRC 覆盖从 `AA 55` 开始到 Payload 末尾的所有字节，不包含 CRC 自身和帧尾。

| CRC 参数 | 值 |
| --- | --- |
| 算法 | CRC16-CCITT |
| 多项式 | `0x1021` |
| 初始值 | `0xFFFF` |

### 当前填充的控制字段

STM32 将五个文本字段映射到 `ctrl_frame_t`：

- `angle_left`、`angle_right` 直接写入舵轮角度。
- `angle_camera` 固定为 0。
- `chassis_lock`、两个电机刹车位和差速锁均固定为 0。
- `left_pwm`、`right_pwm` 写入左右电机控制字段。
- `pulses` 写入脉冲字段。
- LED 控制字段保持为 0。

该结构包含 32 位浮点数和 C 位域。它在当前 ARM Cortex-M/Keil 工具链下工作，但位域和结构布局可能受编译器影响；更换编译器、编译选项或 MCU 后必须重新抓取帧并验证字节布局。

## 当前未实现

- AGV 状态反馈解析和 MQTT 上行。
- 应用层 ACK、序号与重复命令检测。
- 心跳、命令有效期和失联自动停车。
- 远程急停的独立安全通道。
- 消息签名、TLS 和生产级设备身份管理。

在这些机制完成前，本系统只能在受控研究环境中运行。

## 代码位置

- PC 发送程序：[`pc_auto_sender.py`](../pc%20control/Python_Control/pc_send/pc_auto_sender.py)
- F1 入口：[`main.c`](../firmware/STM32%20F1%20Version/f1version%20program/F1/Core/Src/main.c)
- 帧配置：[`uart_frame_config.h`](../firmware/STM32%20F1%20Version/f1version%20program/F1/Core/Inc/uart_frame_config.h)
- CRC 配置：[`uart_frame_crc.h`](../firmware/STM32%20F1%20Version/f1version%20program/F1/Core/Inc/uart_frame_crc.h)

返回[文档索引](README.md)。
