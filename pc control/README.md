# PC MQTT Controller

## English Version

This directory contains the PC-side program that publishes smart traffic-cone control commands over MQTT.

## Entry Point

```text
pc control/Python_Control/pc_send/pc_auto_sender.py
```

Data path:

```text
pc_auto_sender.py -> EMQX -> M100M -> STM32F103 -> AGV
```

## Environment

- Windows 10 or 11.
- Python 3.10 or later.
- `paho-mqtt`.

Install:

```powershell
cd "pc control\Python_Control\pc_send"
py -m venv .venv
.\.venv\Scripts\Activate.ps1
py -m pip install paho-mqtt
```

A maintained `requirements.txt` should be added later to pin a verified dependency range.

## Configuration Before Running

The current script reads these values from its source configuration section:

```python
BROKER_HOST = "<YOUR_BROKER_HOST>"
BROKER_PORT = 1883
USERNAME = "<PC_USERNAME>"
PASSWORD = "<PC_PASSWORD>"
PC_CLIENT_ID = "pc-controller-01"
COMMAND_TOPIC = "cone/<DEVICE_ID>/command"
```

Requirements:

- Broker and credentials must match the [EMQX configuration](../server/README.md).
- `COMMAND_TOPIC` must exactly match the [M100M](../hardware/4G%20module/M100M/README.md) subscription.
- `PC_CLIENT_ID` must not duplicate a DTU or another PC.
- Never commit real values to Git.

The current working copy still contains a historical public address and weak test password. Replace them before running or publishing the code, and rotate the server-side password.

## Safe First-Test Values

The historical script currently contains non-zero motion values. Before the first run, change them to:

```python
LEFT_PWM = 0
RIGHT_PWM = 0
PULSES = 0
ANGLE_LEFT = 0.0
ANGLE_RIGHT = 0.0
REPEAT_COUNT = 1
INTERVAL_SECONDS = 5
QOS = 1
RETAIN = False
APPEND_NEWLINE = True
```

The first payload must be:

```text
0,0,0,0.0,0.0\n
```

## Run

```powershell
py pc_auto_sender.py
```

Expected output includes:

- A sanitized Broker and topic summary.
- Successful MQTT connection.
- The published payload.
- QoS 1 publish confirmation.

Do not expose credentials, full public-server configuration, or device identities in logs or screenshots.

## Parameter Ranges

| Parameter | Range |
| --- | ---: |
| `LEFT_PWM` | -100 to 100 |
| `RIGHT_PWM` | -100 to 100 |
| `PULSES` | 0 to 100000 |
| `ANGLE_LEFT` | -90 to 90 degrees |
| `ANGLE_RIGHT` | -90 to 90 degrees |

See the [communication protocol](../docs/protocol.md) for parsing details.

## Recommended Refactor

Move configuration to environment variables:

```dotenv
MQTT_HOST=broker.example.com
MQTT_PORT=1883
MQTT_USERNAME=replace_me
MQTT_PASSWORD=replace_me
MQTT_TOPIC=cone/device-id/command
MQTT_CLIENT_ID=pc-controller-01
```

Commit only `.env.example`. The real `.env` must be excluded by `.gitignore`.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| Missing `paho` | Run `py -m pip install paho-mqtt` in the active virtual environment |
| Connection timeout | Broker host, port, firewall, and network |
| Not authorized | Username, password, authentication chain, and ACL |
| Publish succeeds but DTU receives nothing | Topic, DTU online state, Client ID, and QoS |
| STM32 does not parse | Five-field format and `APPEND_NEWLINE=True` |
| Old command runs after reconnect | Verify `RETAIN=False` and clear retained messages |

Return to the [project home](../README.md).

---

# 中文版

本目录保存通过 MQTT 向智能交通锥发送控制命令的 PC 端程序。

## 程序入口

```text
pc control/Python_Control/pc_send/pc_auto_sender.py
```

链路：

```text
pc_auto_sender.py -> EMQX -> M100M -> STM32F103 -> AGV
```

## 环境

- Windows 10/11。
- Python 3.10 或更高版本。
- `paho-mqtt`。

安装：

```powershell
cd "pc control\Python_Control\pc_send"
py -m venv .venv
.\.venv\Scripts\Activate.ps1
py -m pip install paho-mqtt
```

后续应增加 `requirements.txt` 固定经过验证的依赖范围。

## 运行前配置

当前脚本仍在源码配置区读取以下变量：

```python
BROKER_HOST = "<YOUR_BROKER_HOST>"
BROKER_PORT = 1883
USERNAME = "<PC_USERNAME>"
PASSWORD = "<PC_PASSWORD>"
PC_CLIENT_ID = "pc-controller-01"
COMMAND_TOPIC = "cone/<DEVICE_ID>/command"
```

要求：

- Broker 和认证信息与 [EMQX 配置](../server/README.md)一致。
- `COMMAND_TOPIC` 与 [M100M](../hardware/4G%20module/M100M/README.md) 的订阅 Topic 完全一致。
- `PC_CLIENT_ID` 不得与 DTU 或其他 PC 重复。
- 不要把真实值提交到 Git。

当前工作副本仍含历史公网地址和弱密码。运行或发布前必须替换，并在服务器端轮换该密码。

## 首次安全参数

当前历史脚本中的运动参数不是零值。首次运行前必须改成：

```python
LEFT_PWM = 0
RIGHT_PWM = 0
PULSES = 0
ANGLE_LEFT = 0.0
ANGLE_RIGHT = 0.0
REPEAT_COUNT = 1
INTERVAL_SECONDS = 5
QOS = 1
RETAIN = False
APPEND_NEWLINE = True
```

首条消息应为：

```text
0,0,0,0.0,0.0\n
```

## 运行

```powershell
py pc_auto_sender.py
```

程序应输出：

- Broker 和 Topic 摘要。
- MQTT 连接成功。
- 发布的 Payload。
- QoS 1 发布确认。

不要在日志或截图中暴露密码、完整公网配置或设备身份信息。

## 参数范围

| 参数 | 范围 |
| --- | ---: |
| `LEFT_PWM` | -100 到 100 |
| `RIGHT_PWM` | -100 到 100 |
| `PULSES` | 0 到 100000 |
| `ANGLE_LEFT` | -90 到 90 度 |
| `ANGLE_RIGHT` | -90 到 90 度 |

完整规则参见 [通信协议](../docs/protocol.md)。

## 推荐的后续重构

为避免凭据再次进入源码，后续应让脚本从环境变量读取：

```dotenv
MQTT_HOST=broker.example.com
MQTT_PORT=1883
MQTT_USERNAME=replace_me
MQTT_PASSWORD=replace_me
MQTT_TOPIC=cone/device-id/command
MQTT_CLIENT_ID=pc-controller-01
```

仓库只提交 `.env.example`；真实 `.env` 必须被 `.gitignore` 排除。

## 常见问题

| 现象 | 检查项 |
| --- | --- |
| 缺少 `paho` | 在当前虚拟环境运行 `py -m pip install paho-mqtt` |
| 连接超时 | Broker 地址、端口、防火墙和网络 |
| Not authorized | 用户名、密码、认证链和 ACL |
| 发布成功但 DTU 无数据 | Topic、DTU 在线状态、Client ID 和 QoS |
| STM32 不解析 | 五字段格式和 `APPEND_NEWLINE=True` |
| 设备重连后执行旧命令 | 确认 `RETAIN=False` 并清除 retained message |

返回[项目主页](../README.md)。
