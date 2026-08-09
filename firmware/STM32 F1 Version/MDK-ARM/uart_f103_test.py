#!/usr/bin/env python3
"""
PC-side UART test tool for the STM32F103C8T6 bridge firmware.

Firmware ports:
  USART3 PB11/PB10 @ 115200: PC/DTU command input.
  USART1 PA10/PA9  @ 115200: debug echo output.
  USART2 PA3/PA2   @ 460800: packed AGV frame output.

Examples:
  py uart_f103_test.py --list
  py uart_f103_test.py --cmd-port COM6 --left 20 --right 20 --pulses 500 --angle-left 0 --angle-right 0
  py uart_f103_test.py --cmd-port COM6 --debug-port COM7 --sequence smoke
  py uart_f103_test.py --cmd-port COM6 --debug-port COM7 --agv-tap-port COM8 --sequence smoke
"""

from __future__ import annotations

import argparse
import sys
import threading
import time
from dataclasses import dataclass

serial = None
list_ports = None


def ensure_pyserial() -> None:
    global serial, list_ports

    if serial is not None and list_ports is not None:
        return

    try:
        import serial as serial_module
        from serial.tools import list_ports as list_ports_module
    except ImportError:
        print("Missing dependency: pyserial")
        print("Install it with: py -m pip install pyserial")
        sys.exit(1)

    serial = serial_module
    list_ports = list_ports_module


CMD_BAUD = 115200
DEBUG_BAUD = 115200
AGV_TAP_BAUD = 460800

PWM_MIN = -100
PWM_MAX = 100
PULSES_MIN = 0
PULSES_MAX = 100000
ANGLE_MIN = -90.0
ANGLE_MAX = 90.0


@dataclass(frozen=True)
class Command:
    left_pwm: int
    right_pwm: int
    pulses: int
    angle_left: float
    angle_right: float

    def validate(self) -> None:
        if not PWM_MIN <= self.left_pwm <= PWM_MAX:
            raise ValueError("left_pwm must be between -100 and 100")
        if not PWM_MIN <= self.right_pwm <= PWM_MAX:
            raise ValueError("right_pwm must be between -100 and 100")
        if not PULSES_MIN <= self.pulses <= PULSES_MAX:
            raise ValueError("pulses must be between 0 and 100000")
        if not ANGLE_MIN <= self.angle_left <= ANGLE_MAX:
            raise ValueError("angle_left must be between -90 and 90")
        if not ANGLE_MIN <= self.angle_right <= ANGLE_MAX:
            raise ValueError("angle_right must be between -90 and 90")

    def line(self) -> str:
        self.validate()
        return (
            f"{self.left_pwm},{self.right_pwm},{self.pulses},"
            f"{self.angle_left:.2f},{self.angle_right:.2f}\n"
        )


SMOKE_SEQUENCE = [
    Command(0, 0, 0, 0.0, 0.0),
    Command(15, 15, 300, 0.0, 0.0),
    Command(0, 0, 0, 0.0, 0.0),
    Command(-15, -15, 300, 0.0, 0.0),
    Command(0, 0, 0, 5.0, -5.0),
    Command(0, 0, 0, 0.0, 0.0),
]


def list_serial_ports() -> None:
    ensure_pyserial()
    ports = list(list_ports.comports())
    if not ports:
        print("No serial ports found.")
        return

    print("Available serial ports:")
    for port in ports:
        print(f"  {port.device:8s} {port.description}")


def debug_reader(port_name: str, baud: int, stop_event: threading.Event) -> None:
    ensure_pyserial()
    try:
        with serial.Serial(port_name, baudrate=baud, timeout=0.1) as ser:
            print(f"[DEBUG] listening on {port_name} @ {baud}")
            while not stop_event.is_set():
                data = ser.read(256)
                if data:
                    text = data.decode("utf-8", errors="replace")
                    print(text, end="", flush=True)
    except serial.SerialException as exc:
        print(f"\n[DEBUG] port error: {exc}")


def agv_tap_reader(port_name: str, baud: int, stop_event: threading.Event) -> None:
    ensure_pyserial()
    try:
        with serial.Serial(port_name, baudrate=baud, timeout=0.1) as ser:
            print(f"[AGV TAP] listening on {port_name} @ {baud}")
            while not stop_event.is_set():
                data = ser.read(256)
                if data:
                    hex_text = " ".join(f"{byte:02X}" for byte in data)
                    print(f"\n[AGV TAP] {hex_text}")
    except serial.SerialException as exc:
        print(f"\n[AGV TAP] port error: {exc}")


def send_command(ser: serial.Serial, command: Command) -> None:
    line = command.line()
    ser.write(line.encode("ascii"))
    ser.flush()
    print(f"[SEND] {line!r}", flush=True)


def build_commands(args: argparse.Namespace) -> list[Command]:
    if args.sequence == "smoke":
        return SMOKE_SEQUENCE

    return [
        Command(
            left_pwm=args.left,
            right_pwm=args.right,
            pulses=args.pulses,
            angle_left=args.angle_left,
            angle_right=args.angle_right,
        )
    ]


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Send test commands to STM32F103 USART3 and optionally monitor debug output."
    )
    parser.add_argument("--list", action="store_true", help="List available serial ports and exit.")
    parser.add_argument("--cmd-port", help="USB-TTL port connected to F103 USART3.")
    parser.add_argument("--cmd-baud", type=int, default=CMD_BAUD)
    parser.add_argument("--debug-port", help="Optional USB-TTL port connected to F103 USART1.")
    parser.add_argument("--debug-baud", type=int, default=DEBUG_BAUD)
    parser.add_argument("--agv-tap-port", help="Optional USB-TTL RX connected to F103 USART2 TX.")
    parser.add_argument("--agv-tap-baud", type=int, default=AGV_TAP_BAUD)
    parser.add_argument("--left", type=int, default=0, help="Left PWM, -100..100.")
    parser.add_argument("--right", type=int, default=0, help="Right PWM, -100..100.")
    parser.add_argument("--pulses", type=int, default=0, help="Pulse count, 0..100000.")
    parser.add_argument("--angle-left", type=float, default=0.0)
    parser.add_argument("--angle-right", type=float, default=0.0)
    parser.add_argument("--repeat", type=int, default=1, help="Repeat count. Use 0 for infinite.")
    parser.add_argument("--interval", type=float, default=1.0, help="Seconds between sends.")
    parser.add_argument("--sequence", choices=["single", "smoke"], default="single")
    parser.add_argument("--dry-run", action="store_true", help="Print commands without opening serial ports.")
    args = parser.parse_args()

    if args.list:
        list_serial_ports()
        return 0

    commands = build_commands(args)
    for command in commands:
        command.validate()

    if args.dry_run:
        for command in commands:
            print(command.line(), end="")
        return 0

    if not args.cmd_port:
        parser.error("--cmd-port is required unless --list or --dry-run is used")

    ensure_pyserial()

    stop_event = threading.Event()
    threads: list[threading.Thread] = []

    if args.debug_port:
        thread = threading.Thread(
            target=debug_reader,
            args=(args.debug_port, args.debug_baud, stop_event),
            daemon=True,
        )
        thread.start()
        threads.append(thread)

    if args.agv_tap_port:
        thread = threading.Thread(
            target=agv_tap_reader,
            args=(args.agv_tap_port, args.agv_tap_baud, stop_event),
            daemon=True,
        )
        thread.start()
        threads.append(thread)

    try:
        with serial.Serial(args.cmd_port, baudrate=args.cmd_baud, timeout=0.2) as ser:
            print(f"[CMD] opened {args.cmd_port} @ {args.cmd_baud}")
            send_count = 0

            while args.repeat == 0 or send_count < args.repeat:
                for command in commands:
                    if args.repeat != 0 and send_count >= args.repeat:
                        break
                    send_command(ser, command)
                    send_count += 1
                    time.sleep(args.interval)

    except KeyboardInterrupt:
        print("\n[INFO] stopped by user")
    except (ValueError, serial.SerialException) as exc:
        print(f"[ERR] {exc}")
        return 1
    finally:
        stop_event.set()
        for thread in threads:
            thread.join(timeout=0.5)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
