#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Publish the original STM32 command to any enabled subset of five cars."""

from __future__ import annotations

import os
import sys
import threading
import time
from dataclasses import dataclass

try:
    from paho.mqtt import client as mqtt
except ImportError:
    mqtt = None


BROKER_HOST = "114.55.175.88"
BROKER_PORT = 1883
USERNAME = "pc"
PASSWORD = os.getenv("MQTT_PASSWORD")
PC_CLIENT_ID = "pc_multi_car_001"

# Test switch: set only the cars participating in this test to True.
# For example, to test cars 1 and 3: [True, False, True, False, False].
ENABLED = [True, True, False, False, False]
CAR_TOPICS = [f"cone{i}d" for i in range(1, 6)]

# Original control logic and five-field STM32 protocol remain unchanged.
LEFT_PWM = 25
RIGHT_PWM = 25
PULSES = 300
ANGLE_LEFT = 0.0
ANGLE_RIGHT = 0.0

PERIODIC_FORWARD_BACKWARD = True
MOVE_SECONDS = 3.0
STOP_SECONDS = 1.0
REPEAT_COUNT = 0       # 0 means repeat until Ctrl+C.
INTERVAL_SECONDS = 5.0 # Used only when periodic motion is disabled.
QOS = 1
RETAIN = False
APPEND_NEWLINE = True
CONNECT_TIMEOUT_SECONDS = 10.0
PUBLISH_TIMEOUT_SECONDS = 5.0

# 防止误运行脚本后立即驱动车辆。确认测试区和物理急停均已就绪后再改为 True。
ALLOW_MOTION = False

connected_event = threading.Event()


@dataclass(frozen=True)
class Car:
    number: int
    topic: str
    enabled: bool


def configured_cars() -> list[Car]:
    if len(ENABLED) != 5:
        raise ValueError("ENABLED must contain exactly five switches")
    if len(CAR_TOPICS) != 5:
        raise ValueError("CAR_TOPICS must contain exactly five topics")
    if any(not isinstance(enabled, bool) for enabled in ENABLED):
        raise TypeError("Every ENABLED item must be True or False")
    return [Car(i + 1, CAR_TOPICS[i], bool(ENABLED[i])) for i in range(5)]


def make_payload(left_pwm=LEFT_PWM, right_pwm=RIGHT_PWM, pulses=PULSES) -> str:
    payload = (
        f"{int(left_pwm)},{int(right_pwm)},{int(pulses)},"
        f"{float(ANGLE_LEFT):.2f},{float(ANGLE_RIGHT):.2f}"
    )
    return payload + ("\n" if APPEND_NEWLINE else "")


def validate_parameters() -> None:
    if not -100 <= LEFT_PWM <= 100:
        raise ValueError("LEFT_PWM must be between -100 and 100")
    if not -100 <= RIGHT_PWM <= 100:
        raise ValueError("RIGHT_PWM must be between -100 and 100")
    if not 0 <= PULSES <= 100000:
        raise ValueError("PULSES must be between 0 and 100000")
    if not -90.0 <= ANGLE_LEFT <= 90.0:
        raise ValueError("ANGLE_LEFT must be between -90 and 90")
    if not -90.0 <= ANGLE_RIGHT <= 90.0:
        raise ValueError("ANGLE_RIGHT must be between -90 and 90")
    if REPEAT_COUNT < 0:
        raise ValueError("REPEAT_COUNT cannot be negative")
    if INTERVAL_SECONDS <= 0:
        raise ValueError("INTERVAL_SECONDS must be positive")
    if MOVE_SECONDS <= 0 or STOP_SECONDS < 0:
        raise ValueError("MOVE_SECONDS must be positive and STOP_SECONDS cannot be negative")
    if not 0 <= QOS <= 2:
        raise ValueError("QOS must be 0, 1, or 2")

    if len(CAR_TOPICS) != 5 or len(set(CAR_TOPICS)) != 5:
        raise ValueError("CAR_TOPICS must contain five unique topics")
    cars = configured_cars()
    if not any(car.enabled for car in cars):
        raise ValueError("At least one car must be enabled")
    if not PASSWORD:
        raise ValueError("Set the MQTT_PASSWORD environment variable before running")
    if not ALLOW_MOTION and any((LEFT_PWM, RIGHT_PWM, PULSES)):
        raise ValueError(
            "This command moves vehicles; verify the test area and emergency stop, "
            "then set ALLOW_MOTION to True"
        )


def on_connect(client, userdata, flags, reason_code, properties=None) -> None:
    if reason_code == 0:
        connected_event.set()
        print(f"[MQTT] connected to {BROKER_HOST}:{BROKER_PORT}")
    else:
        print(f"[ERR] MQTT connection failed: {reason_code}")


def on_disconnect(client, userdata, *args) -> None:
    connected_event.clear()
    print("[MQTT] disconnected")


def publish_round(client, cars: list[Car], payload: str, round_no: int) -> bool:
    """Send the same original command once to every enabled car."""
    all_ok = True
    for car in cars:
        if not car.enabled:
            continue
        info = client.publish(car.topic, payload, qos=QOS, retain=RETAIN)
        if info.rc != mqtt.MQTT_ERR_SUCCESS:
            all_ok = False
            print(f"[ERR] round {round_no}, car {car.number}: rc={info.rc}")
            continue
        info.wait_for_publish(timeout=PUBLISH_TIMEOUT_SECONDS)
        if not info.is_published():
            all_ok = False
            print(f"[ERR] round {round_no}, car {car.number}: publish timed out")
            continue
        print(f"[SEND] round {round_no}, car {car.number}, topic={car.topic}, payload={payload!r}")
    return all_ok


def run_periodic_motion(client, cars: list[Car]) -> None:
    """Move enabled cars forward and backward slowly until Ctrl+C."""
    cycle = 0
    while REPEAT_COUNT == 0 or cycle < REPEAT_COUNT:
        cycle += 1
        print(f"[CYCLE] {cycle}: forward")
        if not publish_round(
            client,
            cars,
            make_payload(abs(LEFT_PWM), abs(RIGHT_PWM), PULSES),
            cycle,
        ):
            raise RuntimeError(f"cycle {cycle} forward command failed")
        time.sleep(MOVE_SECONDS)
        if not publish_round(client, cars, make_payload(0, 0, 0), cycle):
            raise RuntimeError(f"cycle {cycle} forward stop command failed")
        time.sleep(STOP_SECONDS)

        print(f"[CYCLE] {cycle}: backward")
        if not publish_round(
            client,
            cars,
            make_payload(-abs(LEFT_PWM), -abs(RIGHT_PWM), PULSES),
            cycle,
        ):
            raise RuntimeError(f"cycle {cycle} backward command failed")
        time.sleep(MOVE_SECONDS)
        if not publish_round(client, cars, make_payload(0, 0, 0), cycle):
            raise RuntimeError(f"cycle {cycle} backward stop command failed")
        time.sleep(STOP_SECONDS)


def main() -> int:
    try:
        validate_parameters()
    except (TypeError, ValueError) as exc:
        print(f"[ERR] invalid configuration: {exc}")
        return 1
    cars = configured_cars()
    active = [car for car in cars if car.enabled]
    payload = make_payload()

    print("Enabled cars: " + ", ".join(f"{car.number}({car.topic})" for car in active))
    print(f"Payload: {payload!r}")

    if mqtt is None:
        print("[ERR] paho-mqtt is missing; install it with: py -m pip install -U paho-mqtt")
        return 1

    client = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        client_id=PC_CLIENT_ID,
        protocol=mqtt.MQTTv311,
    )
    client.username_pw_set(USERNAME, PASSWORD)
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.reconnect_delay_set(min_delay=1, max_delay=10)

    exit_code = 0
    loop_started = False
    connected_event.clear()
    try:
        client.connect(BROKER_HOST, BROKER_PORT, keepalive=60)
        client.loop_start()
        loop_started = True
        if not connected_event.wait(CONNECT_TIMEOUT_SECONDS):
            raise TimeoutError("MQTT connection timed out")

        if PERIODIC_FORWARD_BACKWARD:
            run_periodic_motion(client, cars)
        else:
            round_no = 0
            while REPEAT_COUNT == 0 or round_no < REPEAT_COUNT:
                round_no += 1
                if not publish_round(client, cars, payload, round_no):
                    raise RuntimeError(f"round {round_no} publish failed")
                if REPEAT_COUNT != 0 and round_no >= REPEAT_COUNT:
                    break
                time.sleep(INTERVAL_SECONDS)
    except KeyboardInterrupt:
        print("\n[INFO] stopped by user")
    except Exception as exc:
        print(f"[ERR] {exc}")
        exit_code = 1
    finally:
        if connected_event.is_set():
            print("[SAFE] sending stop command to enabled cars")
            if not publish_round(client, cars, make_payload(0, 0, 0), 0):
                exit_code = 1
        if loop_started:
            client.loop_stop()
        try:
            client.disconnect()
        except Exception as exc:
            print(f"[WARN] disconnect failed: {exc}")
            exit_code = 1

    return exit_code


if __name__ == "__main__":
    sys.exit(main())
