from __future__ import annotations

from dataclasses import dataclass
import struct

ID_RPY = 0x210
ID_ACCEL = 0x211


@dataclass(frozen=True)
class RpyFrame:
    roll_deg: float
    pitch_deg: float
    yaw_deg: float
    limits: int = 0
    sequence: int = 0


@dataclass(frozen=True)
class AccelFrame:
    accel_x: float
    accel_y: float
    accel_z: float
    limits: int = 0
    report_id: int = 0


def _clamp_i16(value: int) -> int:
    return max(-32768, min(32767, value))


def _scale_i16(value: float, scale: float) -> int:
    return _clamp_i16(round(value * scale))


def pack_rpy(frame: RpyFrame) -> bytes:
    return struct.pack(
        ">hhhBB",
        _scale_i16(frame.roll_deg, 100.0),
        _scale_i16(frame.pitch_deg, 100.0),
        _scale_i16(frame.yaw_deg, 100.0),
        frame.limits & 0x0F,
        frame.sequence & 0xFF,
    )


def pack_accel(frame: AccelFrame) -> bytes:
    return struct.pack(
        ">hhhBB",
        _scale_i16(frame.accel_x, 1000.0),
        _scale_i16(frame.accel_y, 1000.0),
        _scale_i16(frame.accel_z, 1000.0),
        frame.limits & 0x0F,
        frame.report_id & 0xFF,
    )


def unpack_rpy(data: bytes) -> RpyFrame:
    roll, pitch, yaw, limits, sequence = struct.unpack(">hhhBB", data[:8])
    return RpyFrame(
        roll_deg=roll / 100.0,
        pitch_deg=pitch / 100.0,
        yaw_deg=yaw / 100.0,
        limits=limits,
        sequence=sequence,
    )


def unpack_accel(data: bytes) -> AccelFrame:
    ax, ay, az, limits, report_id = struct.unpack(">hhhBB", data[:8])
    return AccelFrame(
        accel_x=ax / 1000.0,
        accel_y=ay / 1000.0,
        accel_z=az / 1000.0,
        limits=limits,
        report_id=report_id,
    )


def format_limits(limits: int) -> str:
    names = ["SW1_1", "SW1_2", "SW2_1", "SW2_2"]
    active = [name for index, name in enumerate(names) if limits & (1 << index)]
    return ",".join(active) if active else "-"

