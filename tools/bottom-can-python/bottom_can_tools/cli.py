from __future__ import annotations

import argparse
import itertools
import math
import time
from typing import Optional

import can

from .frames import (
    AccelFrame,
    ID_ACCEL,
    ID_RPY,
    RpyFrame,
    format_limits,
    pack_accel,
    pack_rpy,
    unpack_accel,
    unpack_rpy,
)


def _open_bus(args: argparse.Namespace) -> can.BusABC:
    kwargs = {
        "interface": args.interface,
        "channel": args.channel,
    }
    if args.bitrate:
        kwargs["bitrate"] = args.bitrate
    return can.Bus(**kwargs)


def _send(args: argparse.Namespace) -> None:
    period_s = 1.0 / args.rate_hz
    bus = _open_bus(args)

    print(
        f"Sending 0x{ID_RPY:03X}/0x{ID_ACCEL:03X} on "
        f"{args.interface}:{args.channel} at {args.rate_hz:g} Hz"
    )

    try:
        for sequence in itertools.count():
            t = time.monotonic()
            roll = args.roll
            pitch = args.pitch
            yaw = args.yaw

            if args.demo:
                roll = 15.0 * math.sin(t)
                pitch = 10.0 * math.sin(t * 0.7)
                yaw = (t * 30.0) % 360.0

            rpy = RpyFrame(
                roll_deg=roll,
                pitch_deg=pitch,
                yaw_deg=yaw,
                limits=args.limits,
                sequence=sequence,
            )
            accel = AccelFrame(
                accel_x=args.ax,
                accel_y=args.ay,
                accel_z=args.az,
                limits=args.limits,
                report_id=args.report_id,
            )

            rpy_payload = pack_rpy(rpy)
            accel_payload = pack_accel(accel)

            bus.send(can.Message(arbitration_id=ID_RPY, data=rpy_payload, is_extended_id=False))
            bus.send(can.Message(arbitration_id=ID_ACCEL, data=accel_payload, is_extended_id=False))

            if args.print_frames and ((sequence % args.print_every) == 0):
                rpy_hex = " ".join(f"{byte:02X}" for byte in rpy_payload)
                accel_hex = " ".join(f"{byte:02X}" for byte in accel_payload)
                print(
                    f"TX 0x{ID_RPY:03X} [{len(rpy_payload)}] {rpy_hex} "
                    f"roll={rpy.roll_deg:.2f} pitch={rpy.pitch_deg:.2f} "
                    f"yaw={rpy.yaw_deg:.2f} limits=0x{rpy.limits:02X} seq={rpy.sequence & 0xFF}"
                )
                print(
                    f"TX 0x{ID_ACCEL:03X} [{len(accel_payload)}] {accel_hex} "
                    f"ax={accel.accel_x:.3f} ay={accel.accel_y:.3f} "
                    f"az={accel.accel_z:.3f} limits=0x{accel.limits:02X} "
                    f"report_id=0x{accel.report_id & 0xFF:02X}"
                )

            if args.once:
                break

            elapsed = time.monotonic() - t
            time.sleep(max(0.0, period_s - elapsed))
    finally:
        bus.shutdown()


def _monitor(args: argparse.Namespace) -> None:
    bus = _open_bus(args)
    print(f"Monitoring {args.interface}:{args.channel}")

    try:
        while True:
            msg: Optional[can.Message] = bus.recv(timeout=1.0)
            if msg is None:
                continue

            if msg.arbitration_id == ID_RPY and len(msg.data) >= 8:
                frame = unpack_rpy(bytes(msg.data))
                print(
                    f"0x{ID_RPY:03X} roll={frame.roll_deg:8.2f} "
                    f"pitch={frame.pitch_deg:8.2f} yaw={frame.yaw_deg:8.2f} "
                    f"limits=0x{frame.limits:02X}({format_limits(frame.limits)}) "
                    f"seq={frame.sequence}"
                )
            elif msg.arbitration_id == ID_ACCEL and len(msg.data) >= 8:
                frame = unpack_accel(bytes(msg.data))
                print(
                    f"0x{ID_ACCEL:03X} ax={frame.accel_x:8.3f} "
                    f"ay={frame.accel_y:8.3f} az={frame.accel_z:8.3f} "
                    f"limits=0x{frame.limits:02X}({format_limits(frame.limits)}) "
                    f"report_id={frame.report_id}"
                )
            elif args.all:
                data = " ".join(f"{byte:02X}" for byte in msg.data)
                print(f"0x{msg.arbitration_id:X} [{msg.dlc}] {data}")
    finally:
        bus.shutdown()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="bottom-can")

    def add_bus_args(target: argparse.ArgumentParser) -> None:
        target.add_argument("--interface", default="socketcan", help="python-can interface")
        target.add_argument("--channel", default="can0", help="CAN channel/device")
        target.add_argument("--bitrate", type=int, default=1_000_000, help="CAN bitrate")

    add_bus_args(parser)

    subparsers = parser.add_subparsers(dest="command", required=True)

    send = subparsers.add_parser("send", help="send bottom-can test frames")
    add_bus_args(send)
    send.add_argument("--rate-hz", type=float, default=100.0)
    send.add_argument("--once", action="store_true")
    send.add_argument("--demo", action="store_true", help="animate roll/pitch/yaw")
    send.add_argument("--roll", type=float, default=0.0)
    send.add_argument("--pitch", type=float, default=0.0)
    send.add_argument("--yaw", type=float, default=0.0)
    send.add_argument("--ax", type=float, default=0.0)
    send.add_argument("--ay", type=float, default=0.0)
    send.add_argument("--az", type=float, default=9.81)
    send.add_argument("--limits", type=lambda value: int(value, 0), default=0)
    send.add_argument("--report-id", type=lambda value: int(value, 0), default=0x28)
    send.add_argument("--print", dest="print_frames", action="store_true", help="print transmitted payloads")
    send.add_argument("--print-every", type=int, default=10, help="print every Nth transmitted sample")
    send.set_defaults(func=_send)

    monitor = subparsers.add_parser("monitor", help="decode bottom-can frames")
    add_bus_args(monitor)
    monitor.add_argument("--all", action="store_true", help="print non-bottom-can frames too")
    monitor.set_defaults(func=_monitor)

    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
