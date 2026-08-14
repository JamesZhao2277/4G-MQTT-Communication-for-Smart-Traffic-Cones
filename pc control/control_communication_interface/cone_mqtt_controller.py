#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""智能交通锥 MQTT 通信模块。

本文件只负责通信，不负责生成车辆控制策略。PPO、路径规划或人工控制程序
产生一条控制命令后，可以调用本模块将命令发送给指定的实体锥桶。

主要功能：

1. 检查 AGV 五字段命令是否合法；
2. 转换成 STM32 当前固件要求的、以换行符结尾的 CSV 文本；
3. 根据实体锥桶编号选择 ``cone1d``～``cone5d`` Topic；
4. 通过一个持续连接的 MQTT 客户端发布命令；
5. 提供单车停车、全部停车和关闭连接时的安全停车接口。

安装依赖：

    py -m pip install -U paho-mqtt

基本用法：

    controller = ConeMqttController(
        broker_host="当前使用的 Broker 地址",
        broker_port=1883,
        username="当前 MQTT 账号",
        password="当前 MQTT 密码",
    )
    controller.start()
    try:
        controller.send_command(
            cone_id=1,
            left_pwm=25,
            right_pwm=25,
            pulses=100,
            angle_left=0.0,
            angle_right=0.0,
        )
    finally:
        # 程序结束前，先停止本次控制过的车辆，再断开 MQTT。
        controller.close(stop_touched=True)

接收 PPO 环境 ``info`` 输出的用法：

    controller.send_command(
        cone_id=physical_cone_id,
        left_pwm=info["left_pwm"],
        right_pwm=info["right_pwm"],
        pulses=info["pulses"],
        angle_left=info["angle_left"],
        angle_right=info["angle_right"],
    )

重要说明：

* ``cone_id`` 表示实体 MQTT Topic 编号，从 1 开始；
* PPO 仿真环境中的 ``cone_index`` 是仿真索引，不能直接作为 ``cone_id``；
* 本模块不会自动限制模型的推理频率，调用方必须控制发送时序；
* 当前系统没有完整的 AGV ACK 和状态回传，MQTT 发布成功不代表车辆已经执行；
* 凭据应由调用方传入，不要继续把真实账号和密码写死在本文件中。
"""

from __future__ import annotations

import math
import threading
import uuid
from dataclasses import dataclass
from numbers import Integral, Real
from typing import Mapping

try:
    from paho.mqtt import client as mqtt
except ImportError:  # 即使没有安装 paho-mqtt，也允许单独导入并测试命令格式。
    mqtt = None


DEFAULT_TOPICS = {cone_id: f"cone{cone_id}d" for cone_id in range(1, 6)}
DEFAULT_QOS = 1
DEFAULT_KEEPALIVE_SECONDS = 60
DEFAULT_CONNECT_TIMEOUT_SECONDS = 10.0
DEFAULT_PUBLISH_TIMEOUT_SECONDS = 5.0


def _require_integer(name: str, value: object) -> int:
    if isinstance(value, bool) or not isinstance(value, Integral):
        raise TypeError(f"{name} 必须是整数")
    return int(value)


def _require_finite_number(name: str, value: object) -> float:
    if isinstance(value, bool) or not isinstance(value, Real):
        raise TypeError(f"{name} 必须是实数")
    result = float(value)
    if not math.isfinite(result):
        raise ValueError(f"{name} 不能是 NaN 或无穷大")
    return result


@dataclass(frozen=True)
class CarCommand:
    """一条与当前 STM32 固件输入格式对应的五字段控制命令。"""

    left_pwm: int
    right_pwm: int
    pulses: int
    angle_left: float = 0.0
    angle_right: float = 0.0

    def normalized(self) -> "CarCommand":
        command = CarCommand(
            left_pwm=_require_integer("left_pwm", self.left_pwm),
            right_pwm=_require_integer("right_pwm", self.right_pwm),
            pulses=_require_integer("pulses", self.pulses),
            angle_left=_require_finite_number("angle_left", self.angle_left),
            angle_right=_require_finite_number("angle_right", self.angle_right),
        )
        command.validate()
        return command

    def validate(self) -> None:
        if not -100 <= self.left_pwm <= 100:
            raise ValueError("left_pwm 必须在 -100 到 100 之间")
        if not -100 <= self.right_pwm <= 100:
            raise ValueError("right_pwm 必须在 -100 到 100 之间")
        if not 0 <= self.pulses <= 100000:
            raise ValueError("pulses 必须在 0 到 100000 之间")
        if not -90.0 <= self.angle_left <= 90.0:
            raise ValueError("angle_left 必须在 -90 到 90 之间")
        if not -90.0 <= self.angle_right <= 90.0:
            raise ValueError("angle_right 必须在 -90 到 90 之间")

    def to_payload(self) -> str:
        command = self.normalized()
        return (
            f"{command.left_pwm},{command.right_pwm},{command.pulses},"
            f"{command.angle_left:.2f},{command.angle_right:.2f}\n"
        )


class ConeMqttController:
    """供 Python 控制程序调用的、保持长连接的 MQTT 控制器。"""

    def __init__(
        self,
        broker_host: str,
        *,
        broker_port: int = 1883,
        username: str | None = None,
        password: str | None = None,
        client_id: str | None = None,
        topics: Mapping[int, str] | None = None,
        qos: int = DEFAULT_QOS,
        keepalive_seconds: int = DEFAULT_KEEPALIVE_SECONDS,
        connect_timeout_seconds: float = DEFAULT_CONNECT_TIMEOUT_SECONDS,
        publish_timeout_seconds: float = DEFAULT_PUBLISH_TIMEOUT_SECONDS,
    ) -> None:
        if not broker_host or not broker_host.strip():
            raise ValueError("broker_host 不能为空")
        if not 1 <= broker_port <= 65535:
            raise ValueError("broker_port 必须在 1 到 65535 之间")
        if qos not in (0, 1, 2):
            raise ValueError("qos 必须是 0、1 或 2")
        if keepalive_seconds <= 0:
            raise ValueError("keepalive_seconds 必须大于 0")
        if connect_timeout_seconds <= 0 or publish_timeout_seconds <= 0:
            raise ValueError("MQTT 超时时间必须大于 0")
        if password is not None and username is None:
            raise ValueError("提供 password 时必须同时提供 username")

        selected_topics = dict(DEFAULT_TOPICS if topics is None else topics)
        if not selected_topics:
            raise ValueError("至少需要配置一个实体锥桶 Topic")
        normalized_topics: dict[int, str] = {}
        for cone_id, topic in selected_topics.items():
            if isinstance(cone_id, bool) or not isinstance(cone_id, Integral):
                raise TypeError("每个 cone_id 都必须是整数")
            if int(cone_id) < 1:
                raise ValueError("每个 cone_id 都必须从 1 开始且大于 0")
            if not isinstance(topic, str) or not topic.strip():
                raise ValueError(f"锥桶 {cone_id} 的 Topic 不能为空")
            normalized_topics[int(cone_id)] = topic.strip()
        if len(set(normalized_topics.values())) != len(normalized_topics):
            raise ValueError("每个实体锥桶必须使用不同的 MQTT Topic")

        self.broker_host = broker_host.strip()
        self.broker_port = int(broker_port)
        self.username = username
        self.password = password
        self.client_id = client_id or f"pc_cone_controller_{uuid.uuid4().hex[:8]}"
        self.topics = normalized_topics
        self.qos = qos
        self.keepalive_seconds = int(keepalive_seconds)
        self.connect_timeout_seconds = float(connect_timeout_seconds)
        self.publish_timeout_seconds = float(publish_timeout_seconds)

        self._client = None
        self._connected = threading.Event()
        self._publish_lock = threading.Lock()
        self._touched_cones: set[int] = set()

    @property
    def is_connected(self) -> bool:
        return self._connected.is_set()

    def _on_connect(self, client, userdata, flags, reason_code, properties=None) -> None:
        del client, userdata, flags, properties
        if reason_code == 0:
            self._connected.set()
            print(f"[MQTT] 已连接 {self.broker_host}:{self.broker_port}")
        else:
            self._connected.clear()
            print(f"[MQTT] 连接失败：{reason_code}")

    def _on_disconnect(self, client, userdata, *args) -> None:
        del client, userdata, args
        self._connected.clear()
        print("[MQTT] 已断开连接")

    def _create_client(self):
        if mqtt is None:
            raise RuntimeError(
                "尚未安装 paho-mqtt；请运行：py -m pip install -U paho-mqtt"
            )

        try:
            client = mqtt.Client(
                callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
                client_id=self.client_id,
                protocol=mqtt.MQTTv311,
            )
        except (AttributeError, TypeError):
            # 兼容仍然使用旧回调接口的 paho-mqtt 1.x。
            client = mqtt.Client(client_id=self.client_id, protocol=mqtt.MQTTv311)

        if self.username is not None:
            client.username_pw_set(self.username, self.password)
        client.on_connect = self._on_connect
        client.on_disconnect = self._on_disconnect
        client.reconnect_delay_set(min_delay=1, max_delay=10)
        return client

    def start(self) -> None:
        """建立一次 MQTT 连接，并在后台线程中运行网络循环。"""

        if self._client is not None:
            if self.is_connected:
                return
            raise RuntimeError("MQTT 客户端已经启动，但目前没有成功连接")

        client = self._create_client()
        self._client = client
        try:
            client.connect(
                self.broker_host,
                self.broker_port,
                keepalive=self.keepalive_seconds,
            )
            client.loop_start()
            if not self._connected.wait(self.connect_timeout_seconds):
                raise TimeoutError("等待 MQTT 连接超时")
        except Exception:
            client.loop_stop()
            try:
                client.disconnect()
            finally:
                self._client = None
                self._connected.clear()
            raise

    def send(self, cone_id: int, command: CarCommand) -> int:
        """检查并发布一条命令，成功后返回 MQTT 消息编号。"""

        physical_id = _require_integer("cone_id", cone_id)
        if physical_id not in self.topics:
            raise ValueError(
                f"尚未配置 cone_id={physical_id}；"
                f"当前可用编号：{sorted(self.topics)}"
            )
        if self._client is None or not self.is_connected:
            raise RuntimeError("MQTT 尚未连接，请先调用 start()")
        if not isinstance(command, CarCommand):
            raise TypeError("command 必须是 CarCommand 对象")

        payload = command.to_payload()
        topic = self.topics[physical_id]

        # 发布超时并不代表车辆一定没有收到命令。先记录该车辆，确保 close()
        # 在结果不确定时仍会尝试发送停车命令。
        self._touched_cones.add(physical_id)

        with self._publish_lock:
            info = self._client.publish(
                topic,
                payload,
                qos=self.qos,
                retain=False,
            )
            if info.rc != mqtt.MQTT_ERR_SUCCESS:
                raise RuntimeError(
                    f"锥桶 {physical_id} 的 MQTT 发布失败：rc={info.rc}"
                )
            info.wait_for_publish(timeout=self.publish_timeout_seconds)
            if not info.is_published():
                raise TimeoutError(
                    f"等待锥桶 {physical_id} 的 MQTT 发布完成时超时"
                )
        print(f"[发送] 锥桶={physical_id}，Topic={topic}，内容={payload!r}")
        return int(info.mid)

    def send_command(
        self,
        cone_id: int,
        left_pwm: int,
        right_pwm: int,
        pulses: int,
        angle_left: float = 0.0,
        angle_right: float = 0.0,
    ) -> int:
        """直接接收五个控制字段的便捷发送接口。"""

        return self.send(
            cone_id,
            CarCommand(
                left_pwm=left_pwm,
                right_pwm=right_pwm,
                pulses=pulses,
                angle_left=angle_left,
                angle_right=angle_right,
            ),
        )

    def stop(self, cone_id: int) -> int:
        """向指定实体锥桶发布项目约定的全零停车命令。"""

        return self.send(cone_id, CarCommand(0, 0, 0, 0.0, 0.0))

    def stop_all(self) -> None:
        """向所有已配置的实体锥桶发布停车命令。"""

        errors: list[Exception] = []
        for cone_id in sorted(self.topics):
            try:
                self.stop(cone_id)
            except Exception as exc:  # 先尝试停止所有车辆，再统一报告失败。
                errors.append(exc)
        if errors:
            raise RuntimeError(
                f"有 {len(errors)} 个锥桶停车失败："
                + "; ".join(str(error) for error in errors)
            )

    def close(self, *, stop_touched: bool = True) -> None:
        """根据参数先停止本次控制过的锥桶，然后关闭 MQTT 连接。"""

        client = self._client
        if client is None:
            return

        stop_error: Exception | None = None
        if stop_touched and self.is_connected:
            for cone_id in sorted(self._touched_cones):
                try:
                    self.stop(cone_id)
                except Exception as exc:
                    stop_error = exc
                    print(f"[警告] 关闭连接前停止锥桶 {cone_id} 失败：{exc}")

        client.loop_stop()
        try:
            client.disconnect()
        finally:
            self._client = None
            self._connected.clear()
            self._touched_cones.clear()

        if stop_error is not None:
            raise RuntimeError("关闭连接时有一个或多个停车命令发送失败") from stop_error

    def __enter__(self) -> "ConeMqttController":
        self.start()
        return self

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        del exc_type, exc_value, traceback
        self.close(stop_touched=True)
