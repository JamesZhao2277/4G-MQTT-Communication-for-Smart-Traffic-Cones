"""Offline tests for the PC-side MQTT command and safety logic."""

from __future__ import annotations

import importlib.util
import math
import sys
import unittest
from pathlib import Path


PC_CONTROL_DIR = Path(__file__).resolve().parents[1]


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


controller_module = load_module(
    "cone_mqtt_controller_for_tests",
    PC_CONTROL_DIR / "control_communication_interface" / "cone_mqtt_controller.py",
)
one_cone = load_module(
    "pc_auto_sender_one_cone_for_tests",
    PC_CONTROL_DIR / "test_programme" / "pc_auto_sender_one_cone.py",
)
multiple_cones = load_module(
    "pc_auto_sender_multiple_cones_for_tests",
    PC_CONTROL_DIR / "test_programme" / "pc_auto_sender_multiple_cones.py",
)


class FakePublishInfo:
    def __init__(self, *, published: bool = True, rc: int = 0, mid: int = 7):
        self.rc = rc
        self.mid = mid
        self._published = published
        self.timeout = None

    def wait_for_publish(self, timeout=None) -> None:
        self.timeout = timeout

    def is_published(self) -> bool:
        return self._published


class FakeClient:
    def __init__(self, info: FakePublishInfo):
        self.info = info
        self.calls = []

    def publish(self, *args, **kwargs):
        self.calls.append((args, kwargs))
        return self.info


class CarCommandTests(unittest.TestCase):
    def test_payload_matches_firmware_format(self) -> None:
        command = controller_module.CarCommand(25, -30, 120, 1.5, -2)
        self.assertEqual(command.to_payload(), "25,-30,120,1.50,-2.00\n")

    def test_invalid_values_are_rejected(self) -> None:
        with self.assertRaises(TypeError):
            controller_module.CarCommand(True, 0, 0).to_payload()
        with self.assertRaises(ValueError):
            controller_module.CarCommand(101, 0, 0).to_payload()
        with self.assertRaises(ValueError):
            controller_module.CarCommand(0, 0, 0, math.nan, 0).to_payload()

    def test_topics_are_normalized_before_duplicate_check(self) -> None:
        with self.assertRaises(ValueError):
            controller_module.ConeMqttController(
                "localhost",
                topics={1: "cone1d", 2: " cone1d "},
            )


class ControllerPublishTests(unittest.TestCase):
    def make_controller(self, info: FakePublishInfo):
        controller = controller_module.ConeMqttController(
            "localhost",
            topics={1: "cone1d"},
        )
        controller._client = FakeClient(info)
        controller._connected.set()
        return controller

    def test_send_returns_message_id(self) -> None:
        info = FakePublishInfo(mid=42)
        controller = self.make_controller(info)
        self.assertEqual(controller.send(1, controller_module.CarCommand(0, 0, 0)), 42)
        self.assertEqual(info.timeout, controller.publish_timeout_seconds)

    def test_uncertain_publish_is_still_marked_for_safe_stop(self) -> None:
        controller = self.make_controller(FakePublishInfo(published=False))
        with self.assertRaises(TimeoutError):
            controller.send(1, controller_module.CarCommand(25, 25, 100))
        self.assertIn(1, controller._touched_cones)


class ManualSenderTests(unittest.TestCase):
    def test_short_topic_list_has_clear_validation_error(self) -> None:
        original_topics = multiple_cones.CAR_TOPICS
        try:
            multiple_cones.CAR_TOPICS = ["cone1d"]
            with self.assertRaisesRegex(ValueError, "exactly five topics"):
                multiple_cones.configured_cars()
        finally:
            multiple_cones.CAR_TOPICS = original_topics

    def test_publish_timeout_is_reported(self) -> None:
        info = FakePublishInfo(published=False)
        client = FakeClient(info)
        cars = [multiple_cones.Car(1, "cone1d", True)]
        self.assertFalse(multiple_cones.publish_round(client, cars, "0,0,0,0,0\n", 1))
        self.assertEqual(info.timeout, multiple_cones.PUBLISH_TIMEOUT_SECONDS)

    def test_motion_requires_explicit_safety_enable(self) -> None:
        original_password = multiple_cones.PASSWORD
        original_allow_motion = multiple_cones.ALLOW_MOTION
        try:
            multiple_cones.PASSWORD = "test-only"
            multiple_cones.ALLOW_MOTION = False
            with self.assertRaisesRegex(ValueError, "ALLOW_MOTION"):
                multiple_cones.validate_parameters()
        finally:
            multiple_cones.PASSWORD = original_password
            multiple_cones.ALLOW_MOTION = original_allow_motion

        original_password = one_cone.PASSWORD
        original_allow_motion = one_cone.ALLOW_MOTION
        try:
            one_cone.PASSWORD = "test-only"
            one_cone.ALLOW_MOTION = False
            with self.assertRaisesRegex(ValueError, "ALLOW_MOTION"):
                one_cone.validate_parameters()
        finally:
            one_cone.PASSWORD = original_password
            one_cone.ALLOW_MOTION = original_allow_motion


if __name__ == "__main__":
    unittest.main()
