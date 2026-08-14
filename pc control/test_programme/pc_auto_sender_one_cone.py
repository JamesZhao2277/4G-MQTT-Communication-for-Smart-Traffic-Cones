#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PC 自动发送 MQTT 控制指令
链路：
    本电脑上的 Python 脚本 -> EMQX -> DTU/4G -> UART -> STM32

使用：
1. 先修改“配置区”中的 Broker、Topic 和要发送的控制参数。
2. 通过环境变量 ``MQTT_PASSWORD`` 提供密码，不要把密码写入代码。
3. 在 Windows 终端中执行：
       py -m pip install -U paho-mqtt
       $env:MQTT_PASSWORD = "当前 MQTT 密码"
       py pc_auto_sender_one_cone.py
"""

import os
import sys
import threading
import time

try:
    from paho.mqtt import client as mqtt
except ImportError:
    mqtt = None


# ============================================================
# 配置区
# ============================================================

# 你的 EMQX 服务器公网 IP 和 MQTT 端口
BROKER_HOST = "114.55.175.88"
BROKER_PORT = 1883

# EMQX 中创建的 MQTT 账号
USERNAME = os.getenv("MQTT_USERNAME", "pc")
PASSWORD = os.getenv("MQTT_PASSWORD")

# 这是“PC 控制端”的客户端 ID，不能与 DTU 的 Client ID 重复
PC_CLIENT_ID = "pc_001"

# 必须与 DTU 页面里设置的“订阅主题”完全一致
COMMAND_TOPIC = "cone1d"

# -------- 要发送的 STM32 控制参数 --------
# 协议：left_pwm,right_pwm,pulses,angle_left,angle_right\n

LEFT_PWM = -40
RIGHT_PWM = -40
PULSES = 500
ANGLE_LEFT = 0.0
ANGLE_RIGHT = 0.0

# -------- 自动发送策略 --------
# 1 = 只发送一次（推荐先测试）
# 大于 1 = 固定次数重复发送
# 0 = 无限循环发送，按 Ctrl+C 结束
REPEAT_COUNT = 1

# 多次发送时的时间间隔（秒）
INTERVAL_SECONDS = 5

# 控制消息使用 QoS 1；不保留旧命令，避免设备重连后重复执行
QOS = 1
RETAIN = False

# 原始 STM32 协议说明中要求每条指令以换行符结尾
APPEND_NEWLINE = True
PUBLISH_TIMEOUT_SECONDS = 5.0

# 防止误运行脚本后立即驱动车辆。确认测试区和物理急停均已就绪后再改为 True。
ALLOW_MOTION = False


# ============================================================
# MQTT 连接逻辑
# ============================================================

# 用线程事件记录 MQTT 是否真正连接成功。
# paho-mqtt 的网络循环在后台线程中运行，连接结果会通过 on_connect 回调返回。
connected_event = threading.Event()


def on_connect(client, userdata, flags, reason_code, properties):
    """MQTT 连接完成后的回调函数。"""
    if reason_code == 0:
        print(f"[MQTT] 已连接到 {BROKER_HOST}:{BROKER_PORT}")
        print(f"[MQTT] PC Client ID: {PC_CLIENT_ID}")
        # 通知主线程：连接已经建立，可以开始发布控制指令。
        connected_event.set()
    else:
        print(f"[ERR] MQTT 连接失败，原因码：{reason_code}")


def on_disconnect(client, userdata, disconnect_flags, reason_code, properties):
    """MQTT 断开连接后的回调函数。"""
    # 清除连接标志，避免主流程误以为当前仍然在线。
    connected_event.clear()
    print(f"[MQTT] 已断开连接，原因码：{reason_code}")


def make_payload(
    left_pwm=LEFT_PWM,
    right_pwm=RIGHT_PWM,
    pulses=PULSES,
    angle_left=ANGLE_LEFT,
    angle_right=ANGLE_RIGHT,
):
    """按 STM32 原有五参数协议生成控制指令。"""
    # STM32 端按逗号分隔字段解析，所以这里严格保持字段顺序：
    # 左轮 PWM、右轮 PWM、脉冲数、左舵机角度、右舵机角度。
    payload = (
        f"{int(left_pwm)},{int(right_pwm)},{int(pulses)},"
        f"{float(angle_left):.2f},{float(angle_right):.2f}"
    )

    # 如果下位机用换行符判断一条命令结束，就需要在末尾补上 "\n"。
    if APPEND_NEWLINE:
        payload += "\n"
    return payload


def validate_parameters():
    """在真正发送前检查配置值，提前拦截明显不安全或不合法的参数。"""
    # PWM 限制在 -100 到 100，避免误填过大的占空比。
    if not (-100 <= LEFT_PWM <= 100):
        raise ValueError("LEFT_PWM 必须在 -100 到 100 之间")
    if not (-100 <= RIGHT_PWM <= 100):
        raise ValueError("RIGHT_PWM 必须在 -100 到 100 之间")

    # 脉冲数用于控制运动距离或转向量，负数没有实际意义。
    if not (0 <= PULSES <= 100000):
        raise ValueError("PULSES 必须在 0 到 100000 之间")

    # 舵机角度按当前协议限制在 -90 到 90 度。
    if not (-90.0 <= ANGLE_LEFT <= 90.0):
        raise ValueError("ANGLE_LEFT 必须在 -90 到 90 之间")
    if not (-90.0 <= ANGLE_RIGHT <= 90.0):
        raise ValueError("ANGLE_RIGHT 必须在 -90 到 90 之间")

    # REPEAT_COUNT 为 0 表示无限循环，正数表示固定发送次数。
    if REPEAT_COUNT < 0:
        raise ValueError("REPEAT_COUNT 不能小于 0")
    if INTERVAL_SECONDS <= 0:
        raise ValueError("INTERVAL_SECONDS 必须大于 0")
    if QOS not in (0, 1, 2):
        raise ValueError("QOS 必须是 0、1 或 2")
    if not COMMAND_TOPIC.strip():
        raise ValueError("COMMAND_TOPIC 不能为空")
    if not PASSWORD:
        raise ValueError("请先通过 MQTT_PASSWORD 环境变量提供 MQTT 密码")
    if not ALLOW_MOTION and any((LEFT_PWM, RIGHT_PWM, PULSES)):
        raise ValueError(
            "当前命令会驱动车辆；确认测试区和物理急停后，将 ALLOW_MOTION 改为 True"
        )


def publish_payload(client, payload: str):
    """发布一条消息，并在限定时间内确认 Broker 已接收。"""

    info = client.publish(
        topic=COMMAND_TOPIC,
        payload=payload,
        qos=QOS,
        retain=RETAIN,
    )
    if info.rc != mqtt.MQTT_ERR_SUCCESS:
        raise RuntimeError(f"MQTT 发布失败，错误码：{info.rc}")
    info.wait_for_publish(timeout=PUBLISH_TIMEOUT_SECONDS)
    if not info.is_published():
        raise TimeoutError("等待 MQTT 发布完成时超时")
    return info


def main() -> int:
    # 先校验参数、生成最终要发给 STM32 的字符串。
    try:
        validate_parameters()
    except (TypeError, ValueError) as exc:
        print(f"[ERR] 配置无效：{exc}")
        return 1
    payload = make_payload()

    # 打印本次发送任务摘要，便于运行前确认 Topic 和 Payload。
    print("=" * 60)
    print("PC 自动 MQTT 发送脚本")
    print("=" * 60)
    print(f"Broker : {BROKER_HOST}:{BROKER_PORT}")
    print(f"Topic  : {COMMAND_TOPIC}")
    print(f"Payload: {payload!r}")
    print(f"QoS    : {QOS}")
    if REPEAT_COUNT == 0:
        print(f"模式   : 无限循环，每 {INTERVAL_SECONDS} 秒发送一次")
    else:
        print(f"模式   : 共发送 {REPEAT_COUNT} 次，间隔 {INTERVAL_SECONDS} 秒")
    print("=" * 60)

    if mqtt is None:
        print("[ERR] 尚未安装 paho-mqtt；请运行：py -m pip install -U paho-mqtt")
        return 1

    # 创建 MQTT 客户端。这里使用 MQTT v3.1.1，兼容大多数 EMQX/DTU 配置。
    client = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        client_id=PC_CLIENT_ID,
        protocol=mqtt.MQTTv311,
    )

    # 设置账号密码和连接状态回调。
    client.username_pw_set(USERNAME, PASSWORD)
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect

    # 网络抖动或服务器临时断开时，paho 会按这个范围自动尝试重连。
    client.reconnect_delay_set(min_delay=1, max_delay=10)

    interrupted_or_failed = False
    loop_started = False
    try:
        # connect() 只发起连接；loop_start() 启动后台网络循环后，回调才会被触发。
        client.connect(BROKER_HOST, BROKER_PORT, keepalive=60)
        client.loop_start()
        loop_started = True

        # 等待 on_connect 设置 connected_event，避免还没连上就发布消息。
        if not connected_event.wait(timeout=10):
            raise TimeoutError(
                "10 秒内没有连上 EMQX。请检查公网 IP、1883 端口、"
                "账号密码、安全组和 EMQX 服务状态。"
            )

        sent_count = 0

        # REPEAT_COUNT 为 0 时无限循环；否则发送到指定次数后退出。
        while REPEAT_COUNT == 0 or sent_count < REPEAT_COUNT:
            publish_payload(client, payload)
            sent_count += 1
            print(f"[SEND] 第 {sent_count} 次已发送：{payload!r}")

            # 固定次数发送完成后直接退出循环，不再额外 sleep。
            if REPEAT_COUNT != 0 and sent_count >= REPEAT_COUNT:
                break

            time.sleep(INTERVAL_SECONDS)

    except KeyboardInterrupt:
        interrupted_or_failed = True
        print("\n[INFO] 用户按下 Ctrl+C，已停止自动发送。")
    except Exception as exc:
        interrupted_or_failed = True
        print(f"[ERR] {exc}")
        return 1
    finally:
        # 发布结果不确定或用户中断时，尽量先发全零停车命令。
        if interrupted_or_failed and connected_event.is_set():
            try:
                publish_payload(client, make_payload(0, 0, 0, 0.0, 0.0))
                print("[SAFE] 已发送停车命令")
            except Exception as exc:
                print(f"[WARN] 停车命令发送失败：{exc}")

        try:
            if loop_started:
                client.loop_stop()
            client.disconnect()
        except Exception:
            pass

    print("[INFO] 脚本执行结束。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
