# Integration Testing and Acceptance

## English Version

This guide verifies the communication path one module at a time. Complete each stage before continuing so network, UART, protocol, and mechanical faults remain separable.

## Safety Preconditions

- Lift the AGV drive wheels or place the vehicle in a cleared and isolated test area.
- Verify that the physical emergency stop is reachable and functional.
- Fix the first command to `0,0,0,0.0,0.0\n`.
- Set PC `REPEAT_COUNT` to 1 and `RETAIN` to false.
- Do not use historical weak credentials; use least-privilege test accounts.
- Check supply voltage, UART voltage levels, TX/RX direction, and common ground before power-up.

## Test Equipment

- Windows PC.
- EMQX Broker.
- M100M 4G DTU with an active SIM.
- STM32F103C8T6 flashed with the F1 firmware.
- UNNC AGV.
- ST-Link.
- At least one USB-to-TTL adapter; three are recommended for simultaneous DTU, debug, and AGV-frame observation.
- Serial terminal or logic analyzer.

## 1. Standalone Firmware Test

Bypass MQTT and the DTU. Send a command directly to STM32 USART3 through USB-to-TTL.

| USB-to-TTL | STM32 |
| --- | --- |
| TX | PB11 / USART3 RX |
| RX | PB10 / USART3 TX |
| GND | GND |

USART3 uses 115200, 8N1. USART1 debug uses the same settings.

Use the included test utility:

```powershell
cd "firmware\STM32 F1 Version\f1version program\F1\MDK-ARM"
py -m pip install pyserial
py uart_f103_test.py --list
py uart_f103_test.py --cmd-port COM6 --debug-port COM7 --left 0 --right 0 --pulses 0 --angle-left 0 --angle-right 0
```

Replace the COM ports. USART1 should echo the input and print:

```text
[PC CMD -> USART2]
```

Invalid input should print `[PC CMD invalid]` and must not produce a control frame on USART2.

## 2. AGV Frame Test

Do not connect the AGV RX yet. Observe STM32 PA2 with an RX-only USB-to-TTL connection or logic analyzer.

- Baud rate: 460800.
- First bytes: `AA 55`.
- Type: `11`.
- Version: `01`.
- Last bytes: `0D 0A`.
- CRC16 must match the [protocol](protocol.md).

Do not connect the USB-to-TTL TX to STM32 PA2. Two output drivers must never be tied together.

## 3. Broker Test

1. Start EMQX.
2. Open the Dashboard and confirm that the MQTT listener is running.
3. Use two test clients to emulate PC and DTU.
4. Subscribe the DTU-side client to the command topic.
5. Publish the zero-value message from the PC-side client.
6. Confirm that the subscriber receives exactly the same payload with Retain set to false.

Pass conditions:

- Both Client IDs are unique.
- Authentication succeeds.
- The QoS 1 message arrives.
- ACL rules prevent access to another device's topics.

## 4. M100M Transparent-Transmission Test

Connect the M100M UART to USB-to-TTL, not to the STM32.

1. Configure MQTT and UART.
2. Power the DTU and wait for both mobile-network and Broker connectivity.
3. Publish the zero-value message from the PC.
4. Confirm that UART outputs:

```text
0,0,0,0.0,0.0\n
```

There must be no device ID, timestamp, extra prefix, or encoding conversion. The STM32 will not parse a message without a newline.

## 5. PC-to-STM32 End-to-End Test

Connect M100M to STM32 USART3, but keep the AGV off the ground.

1. Open the STM32 USART1 debug terminal.
2. Send one zero-value command from the PC.
3. Confirm the publish in EMQX.
4. Confirm that USART1 shows the original text.
5. Confirm `[PC CMD -> USART2]`.
6. Capture USART2 and validate header, tail, and CRC.

If `[PC CMD invalid]` appears, check field count, ranges, and the final newline first.

## 6. Controlled AGV Test

Connect the AGV only after all previous stages pass:

1. Keep the drive wheels lifted.
2. Send the zero-value command and verify no movement.
3. Use the smallest parameters approved by the on-site safety owner for a brief test.
4. Verify left/right direction, steering-angle sign, and pulse behavior separately.
5. Send a stop command and remove drive power after testing.

This guide intentionally provides no default motion values because wheel direction and mechanical installation may differ.

## Acceptance Record

Add one row for every formal handover test:

| Date | Git Commit | Hardware Revision | Broker/DTU Summary | F1 Build | Zero-Value Path | Controlled Motion | Tester | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| YYYY-MM-DD | Commit SHA | To be completed | No passwords | Pass/Fail | Pass/Fail | Pass/Fail | Name | Issue ID |

## Troubleshooting

| Symptom | First Checks |
| --- | --- |
| PC cannot connect to Broker | Network, port, firewall, credentials, system time, and Broker logs |
| PC publishes but DTU receives nothing | Exact topic match, DTU online state, ACL, and Client ID conflicts |
| DTU is online but UART is silent | Mode, subscription, 115200 8N1, TX/RX, and common ground |
| STM32 echoes but reports invalid | Five-field format, ranges, commas, and newline |
| STM32 reports RX overflow | DTU bursts, message length, and publish rate |
| STM32 reports pack/send failed | Frame configuration, buffers, USART2 state, and toolchain layout |
| USART2 has data but AGV does not move | 460800 setting, pin direction, CRC, frame type, AGV power, and emergency-stop state |
| Direction or angle is reversed | Mechanical installation, left/right mapping, and angle sign; use a low-risk single-axis test |
| Reconnect executes an old command | Confirm Retain=false and remove any retained Broker message |

## Artifacts After Testing

- Keep only sanitized serial logs and frame-capture summaries.
- Record firmware, Broker, and DTU firmware versions.
- Convert failures into Issues instead of leaving them only in chat history.
- Update implementation status and known limitations in the root README.

Return to the [documentation index](README.md).

---

# 中文版

本说明用于从单模块到整机逐步验证通信链路。每一步通过后再进入下一步，避免把网络、串口、协议和机械问题混在一起。

## 安全前置条件

- AGV 驱动轮抬离地面，或位于清空并隔离的测试区域。
- 物理急停可触达且已验证。
- 首条命令固定为 `0,0,0,0.0,0.0\n`。
- PC 程序 `REPEAT_COUNT` 设为 1，`RETAIN` 设为 false。
- 不使用历史弱密码，测试账号采用最小权限。
- 上电前检查供电、电平、TX/RX 方向和公共地。

## 测试设备

- Windows PC。
- EMQX Broker。
- M100M 4G DTU 和可用 SIM。
- STM32F103C8T6 与已烧录 F1 固件。
- UNNC AGV。
- ST-Link。
- 至少一个 USB-TTL；推荐准备三个用于同时观察 DTU、调试口和 AGV 帧。
- 串口终端或逻辑分析仪。

## 1. 固件独立测试

先绕过 MQTT 和 DTU，使用 USB-TTL 直接向 STM32 USART3 发送命令。

| USB-TTL | STM32 |
| --- | --- |
| TX | PB11 / USART3 RX |
| RX | PB10 / USART3 TX |
| GND | GND |

USART3 参数为 115200、8N1。USART1 调试口同样是 115200、8N1。

工程附带测试脚本：

```powershell
cd "firmware\STM32 F1 Version\f1version program\F1\MDK-ARM"
py -m pip install pyserial
py uart_f103_test.py --list
py uart_f103_test.py --cmd-port COM6 --debug-port COM7 --left 0 --right 0 --pulses 0 --angle-left 0 --angle-right 0
```

替换实际 COM 端口。预期 USART1 回显输入，并显示：

```text
[PC CMD -> USART2]
```

错误输入应显示 `[PC CMD invalid]`，且 USART2 不产生控制帧。

## 2. AGV 帧测试

暂不连接 AGV RX，使用只接收的 USB-TTL 或逻辑分析仪观察 STM32 PA2。

- 波特率：460800。
- 首字节：`AA 55`。
- 类型：`11`。
- 版本：`01`。
- 末字节：`0D 0A`。
- CRC16 应与 [协议文档](protocol.md) 一致。

不要把 USB-TTL TX 与 STM32 PA2 同时连接，避免两个输出端冲突。

## 3. Broker 测试

1. 启动 EMQX。
2. 登录 Dashboard，确认 MQTT Listener 正常。
3. 使用两个测试客户端模拟 PC 和 DTU。
4. 让 DTU 模拟端订阅命令 Topic。
5. 让 PC 模拟端发布零值消息。
6. 确认订阅端收到完全相同的消息且 Retain 为 false。

通过条件：

- 两个 Client ID 唯一。
- 认证成功。
- QoS 1 消息可达。
- ACL 不允许客户端访问其他设备 Topic。

## 4. M100M 透传测试

先把 M100M UART 接到 USB-TTL，不连接 STM32。

1. 配置 MQTT 和 UART 参数。
2. 上电并等待移动网络及 Broker 连接成功。
3. PC 发布零值消息。
4. 检查 UART 是否收到：

```text
0,0,0,0.0,0.0\n
```

确认没有设备标识、时间戳、额外前缀或字符编码转换。若缺少换行，STM32 不会开始解析。

## 5. PC 到 STM32 端到端测试

连接 M100M 与 STM32 USART3，暂不让 AGV 落地运行。

1. 打开 STM32 USART1 调试终端。
2. 从 PC 发送一次零值命令。
3. 在 EMQX 确认发布成功。
4. 在 STM32 调试口确认收到原始文本。
5. 确认出现 `[PC CMD -> USART2]`。
6. 捕获 USART2 并校验帧头、帧尾和 CRC。

若出现 `[PC CMD invalid]`，优先检查字段数量、范围和换行符。

## 6. AGV 受控测试

只有前五步全部通过后才能连接 AGV：

1. 保持驱动轮离地。
2. 先发送零值命令并观察无运动。
3. 根据现场安全负责人批准的最小参数进行短时测试。
4. 单独验证左右方向、角度正负方向和脉冲行为。
5. 测试后立即发送停止命令并切断驱动电源。

本文档不提供默认运动值，因为不同机械安装和轮向可能相反。

## 验收记录

每次正式交付测试填写一行：

| 日期 | Git 提交 | 硬件版本 | Broker/DTU 摘要 | F1 编译 | 零值链路 | 受控运动 | 测试人 | 备注 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| YYYY-MM-DD | commit SHA | 待填写 | 不含密码 | 通过/失败 | 通过/失败 | 通过/失败 | 姓名 | 问题编号 |

## 故障排查

| 现象 | 优先检查 |
| --- | --- |
| PC 无法连接 Broker | 网络、端口、防火墙、账号、系统时间和 Broker 日志 |
| PC 发布成功但 DTU 无数据 | Topic 是否完全一致、DTU 是否在线、ACL 和 Client ID 冲突 |
| DTU 在线但 UART 无输出 | 工作模式、订阅 Topic、115200 8N1、TX/RX 和共地 |
| STM32 回显但显示 invalid | 五字段格式、范围、逗号和换行符 |
| STM32 显示 RX overflow | DTU 是否突发发送、消息是否过长、发布频率是否过高 |
| STM32 显示 pack/send failed | 帧配置、缓冲区、USART2 状态和工具链布局 |
| USART2 有数据但 AGV 不动作 | 460800、接口方向、CRC、帧类型、AGV 供电和急停状态 |
| 方向或角度相反 | 机械安装、左右电机定义和角度符号，先做低风险单轴验证 |
| 设备重连后意外执行 | 确认 Retain=false，并检查 Broker 中是否存在旧 retained message |

## 测试完成后的资料

- 保存不含敏感信息的串口日志和抓帧摘要。
- 记录使用的固件、Broker 和 DTU 固件版本。
- 将失败项转换成 Issue，不要只留在聊天记录中。
- 更新根 README 的实现状态和已知限制。

返回[文档索引](README.md)。
