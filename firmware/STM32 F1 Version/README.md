# STM32 F1 Firmware

## English Version

This directory contains the main delivery firmware for the **STM32F103C8T6**. The H7 project is an early prototype; handover and deployment should use this F1 version.

## Function

```text
M100M --USART3--> STM32F103 --USART2--> AGV
                         |
                       USART1
                       debug
```

The firmware:

1. Receives newline-delimited five-field text through USART3.
2. Validates syntax and ranges.
3. Fills `ctrl_frame_t`.
4. Packs an AGV frame using CRC16-CCITT.
5. Sends the frame through USART2 and prints diagnostics through USART1.

## Project Entry Points

| File | Purpose |
| --- | --- |
| [`F1.uvprojx`](./f1version%20program/F1/MDK-ARM/F1.uvprojx) | Keil project |
| [`F1.ioc`](./f1version%20program/F1/F1.ioc) | STM32CubeMX configuration |
| [`main.c`](./f1version%20program/F1/Core/Src/main.c) | Receive, parse, and forward logic |
| [`uart_frame_config.h`](./f1version%20program/F1/Core/Inc/uart_frame_config.h) | AGV types and payload structures |
| [`uart_frame.c`](./f1version%20program/F1/Core/Src/uart_frame.c) | Frame packing and parsing |
| [`uart_frame_crc.c`](./f1version%20program/F1/Core/Src/uart_frame_crc.c) | CRC16 implementation |
| [`uart_f103_test.py`](./f1version%20program/F1/MDK-ARM/uart_f103_test.py) | Direct PC-to-UART test utility |

## UARTs

| Interface | Pins | Settings | Purpose |
| --- | --- | --- | --- |
| USART1 | PA9 TX / PA10 RX | 115200, 8N1 | Debug output |
| USART2 | PA2 TX / PA3 RX | 460800, 8N1 | AGV binary frames |
| USART3 | PB10 TX / PB11 RX | 115200, 8N1 | M100M text commands |

Cross TX/RX and connect a common ground. Confirm interface voltage levels before power-up.

## Toolchain

| Item | Current Project |
| --- | --- |
| Target | `F1` |
| Device | `STM32F103C8` |
| Device Pack | `Keil.STM32F1xx_DFP.2.2.0` |
| Compiler mode | ARM Compiler 5 |

Historical `build.log`:

```text
"F1\F1.axf" - 0 Error(s), 0 Warning(s).
Program Size: Code=7742 RO-data=850 RW-data=36 ZI-data=3012
```

This proves only that the original environment built successfully. Every handover must rebuild in its own environment.

## Build and Flash

1. Install Keil MDK-ARM.
2. Install STM32F1 Device Family Pack 2.2.0, or record and validate the replacement version.
3. Open `f1version program/F1/MDK-ARM/F1.uvprojx`.
4. Select the `F1` target.
5. Build and confirm zero errors.
6. Connect ST-Link and confirm the target is STM32F103C8T6.
7. Flash and reset.
8. Open USART1 at 115200 baud and check diagnostics.

Do not install Keil or Device Packs from unverified binaries copied into the repository. Published documentation should point to a trusted source.

## Direct Test Without MQTT

Connect USB-to-TTL adapters to USART3 and, optionally, USART1:

```powershell
cd "firmware\STM32 F1 Version\f1version program\F1\MDK-ARM"
py -m pip install pyserial
py uart_f103_test.py --list
py uart_f103_test.py --cmd-port COM6 --debug-port COM7 --left 0 --right 0 --pulses 0 --angle-left 0 --angle-right 0
```

Replace COM6 and COM7 with the actual ports. The debug output should include `[PC CMD -> USART2]`.

## Input and Output

Input:

```text
left_pwm,right_pwm,pulses,angle_left,angle_right\n
```

Output frame:

- Header `AA 55`.
- Control type `11`.
- Protocol version `01`.
- CRC16-CCITT with polynomial `0x1021` and initial value `0xFFFF`.
- Tail `0D 0A`.

See the [communication protocol](../../docs/protocol.md).

## Diagnostic Messages

| Message | Meaning |
| --- | --- |
| `[PC CMD -> USART2]` | Command is valid and was packed and sent |
| `[PC CMD invalid]` | Field count, syntax, or range is invalid |
| `[PC CMD overflow]` | One command exceeded the line buffer |
| `[USART3 RX overflow]` | USART3 receive queue overflow |
| `[PC CMD pack/send failed]` | Frame packing or USART2 transmission failed |

## Current Limitations

- Only the command downlink is implemented.
- There is no ACK, sequence number, expiry time, or link-loss stop.
- Feedback types exist, but the main loop does not publish AGV feedback.
- The control payload contains floating-point values and C bit-fields; revalidate binary layout after a toolchain change.

Return to the [firmware index](../README.md).

---

# 中文版

本目录保存基于 **STM32F103C8T6** 的当前主要交付固件。H7 工程是早期原型，首次接手和部署应使用本版本。

## 功能

```text
M100M --USART3--> STM32F103 --USART2--> AGV
                         |
                       USART1
                       debug
```

固件完成：

1. 从 USART3 接收换行分隔的五字段文本。
2. 校验字段格式和范围。
3. 填充 `ctrl_frame_t`。
4. 使用 CRC16-CCITT 打包 AGV 控制帧。
5. 从 USART2 发给 AGV，并从 USART1 输出诊断信息。

## 工程入口

| 文件 | 用途 |
| --- | --- |
| [`F1.uvprojx`](./f1version%20program/F1/MDK-ARM/F1.uvprojx) | Keil 工程 |
| [`F1.ioc`](./f1version%20program/F1/F1.ioc) | STM32CubeMX 配置 |
| [`main.c`](./f1version%20program/F1/Core/Src/main.c) | 接收、解析和转发主逻辑 |
| [`uart_frame_config.h`](./f1version%20program/F1/Core/Inc/uart_frame_config.h) | AGV 类型和 Payload 结构 |
| [`uart_frame.c`](./f1version%20program/F1/Core/Src/uart_frame.c) | 帧打包与解析 |
| [`uart_frame_crc.c`](./f1version%20program/F1/Core/Src/uart_frame_crc.c) | CRC16 实现 |
| [`uart_f103_test.py`](./f1version%20program/F1/MDK-ARM/uart_f103_test.py) | PC 直连串口测试工具 |

## 串口

| 接口 | 引脚 | 参数 | 用途 |
| --- | --- | --- | --- |
| USART1 | PA9 TX / PA10 RX | 115200, 8N1 | 调试日志 |
| USART2 | PA2 TX / PA3 RX | 460800, 8N1 | AGV 二进制帧 |
| USART3 | PB10 TX / PB11 RX | 115200, 8N1 | M100M 文本命令 |

TX/RX 交叉连接并共地。上电前确认所有设备的接口电平。

## 工具链

| 项目 | 当前工程 |
| --- | --- |
| Target | `F1` |
| Device | `STM32F103C8` |
| Device Pack | `Keil.STM32F1xx_DFP.2.2.0` |
| 编译器模式 | ARM Compiler 5 |

历史 `build.log` 记录：

```text
"F1\F1.axf" - 0 Error(s), 0 Warning(s).
Program Size: Code=7742 RO-data=850 RW-data=36 ZI-data=3012
```

该记录只说明原环境曾成功编译。接手者仍需在自己的环境重新构建。

## 编译与烧录

1. 安装 Keil MDK-ARM。
2. 安装 STM32F1 Device Family Pack 2.2.0，或记录并验证替代版本。
3. 打开 `f1version program/F1/MDK-ARM/F1.uvprojx`。
4. 选择 `F1` Target。
5. 执行 Build，确认 0 errors。
6. 连接 ST-Link 并确认目标芯片为 STM32F103C8T6。
7. 烧录后复位。
8. 打开 USART1 115200 串口检查日志。

不要从仓库安装来源不明的 Keil 安装程序或 Device Pack；发布仓库应只提供官方下载说明。

## 不连接 MQTT 的直连测试

使用 USB-TTL 连接 USART3 和可选 USART1：

```powershell
cd "firmware\STM32 F1 Version\f1version program\F1\MDK-ARM"
py -m pip install pyserial
py uart_f103_test.py --list
py uart_f103_test.py --cmd-port COM6 --debug-port COM7 --left 0 --right 0 --pulses 0 --angle-left 0 --angle-right 0
```

将 COM6、COM7 替换为实际端口。预期调试口显示 `[PC CMD -> USART2]`。

## 输入和输出

输入：

```text
left_pwm,right_pwm,pulses,angle_left,angle_right\n
```

输出帧使用：

- 帧头 `AA 55`。
- 控制类型 `11`。
- 协议版本 `01`。
- CRC16-CCITT，多项式 `0x1021`，初始值 `0xFFFF`。
- 帧尾 `0D 0A`。

完整定义参见 [通信协议](../../docs/protocol.md)。

## 诊断信息

| 日志 | 含义 |
| --- | --- |
| `[PC CMD -> USART2]` | 命令合法且已打包发送 |
| `[PC CMD invalid]` | 字段数量、格式或范围错误 |
| `[PC CMD overflow]` | 单行命令超过缓冲区 |
| `[USART3 RX overflow]` | USART3 接收队列溢出 |
| `[PC CMD pack/send failed]` | 帧打包或 USART2 发送失败 |

## 当前限制

- 只实现控制下行。
- 没有命令 ACK、序号、过期时间和失联停车。
- AGV 反馈类型虽有定义，但主循环尚未接入反馈上行。
- 控制 Payload 包含浮点数和 C 位域，更换工具链后需要重新验证布局。

返回[固件目录](../README.md)。
