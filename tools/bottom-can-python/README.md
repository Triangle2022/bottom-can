# bottom-can Python Tools

Python CAN tester for the `bottom-can` firmware.

## What This Tool Does

This tool decodes the CAN frames transmitted by the STM32 `bottom-can` firmware:

```text
0x210 = roll, pitch, yaw, limit bits, sequence
0x211 = accel_x, accel_y, accel_z, limit bits, report_id
```

It can also send fake test frames with the same format.

## Requirements

- A real USB-CAN adapter connected to CANH/CANL.
- The Nucleo USB serial port is not a CAN adapter.
- For `--interface slcan`, the USB-CAN adapter must support SLCAN/LAWICEL serial protocol.
- CAN bus speed is `1 Mbps`.

## Install With Conda

```bash
cd /Users/choij/Documents/bottom-can/tools/bottom-can-python
conda create -n bottom-can python=3.11 -y
conda activate bottom-can
pip install -e .
```

Later:

```bash
cd /Users/choij/Documents/bottom-can/tools/bottom-can-python
conda activate bottom-can
```

## Install With venv

```bash
cd /Users/choij/Documents/bottom-can/tools/bottom-can-python
python3 -m venv .venv
source .venv/bin/activate
pip install -e .
```

## macOS Quick Start

1. Create and enter the virtual environment:

```bash
cd /Users/choij/Documents/bottom-can/tools/bottom-can-python
python3 -m venv .venv
source .venv/bin/activate
pip install -e .
```

2. Find the USB-CAN serial device:

```bash
ls /dev/tty.*
```

Common names look like:

```text
/dev/tty.usbserial-XXXX
/dev/tty.usbmodemXXXX
/dev/cu.usbmodemXXXX
/dev/tty.SLAB_USBtoUART
```

3. Send test frames at 1 Mbps:

```bash
bottom-can send --interface slcan --channel /dev/tty.usbserial-XXXX --bitrate 1000000
```

Example with a macOS `/dev/cu.*` device:

```bash
bottom-can send --interface slcan --channel /dev/cu.usbmodem2090319458421 --bitrate 1000000
```

4. Monitor and decode frames:

```bash
bottom-can monitor --interface slcan --channel /dev/cu.usbmodem2090319458421 --bitrate 1000000
```

5. Send one manual sample:

```bash
bottom-can send --interface slcan --channel /dev/tty.usbserial-XXXX --bitrate 1000000 --once --roll 1.23 --pitch -4.56 --yaw 90 --ax 0.1 --ay 0 --az 9.81 --limits 0x05
```

6. Send animated demo values:

```bash
bottom-can send --interface slcan --channel /dev/tty.usbserial-XXXX --bitrate 1000000 --demo
```

7. Print the transmitted payloads:

```bash
bottom-can send --interface slcan --channel /dev/cu.usbmodem11103 --bitrate 1000000 --print
```

Print every frame:

```bash
bottom-can send --interface slcan --channel /dev/cu.usbmodem11103 --bitrate 1000000 --print --print-every 1
```

If the adapter is not SLCAN-compatible, change `--interface` to the backend used by the adapter. Examples include `pcan`, `kvaser`, `canalystii`, or another `python-can` backend.

If no new `/dev/cu.*` device appears after plugging in the USB-CAN adapter, it is probably not an SLCAN serial adapter. In that case, check the adapter vendor driver or SDK.

## Send Test Frames

SocketCAN example:

```bash
bottom-can send --interface socketcan --channel can0
```

SLCAN/USB serial CAN adapter example:

```bash
bottom-can send --interface slcan --channel /dev/tty.usbserial-XXXX --bitrate 1000000
```

Manual values:

```bash
bottom-can send --roll 1.23 --pitch -4.56 --yaw 90 --ax 0.1 --ay 0 --az 9.81 --limits 0x05
```

## Monitor And Decode

```bash
bottom-can monitor --interface socketcan --channel can0
```

## Frame Format

ID `0x210`, 8 bytes:

| Byte | Field |
| --- | --- |
| 0..1 | roll `int16`, deg * 100, big-endian |
| 2..3 | pitch `int16`, deg * 100, big-endian |
| 4..5 | yaw `int16`, deg * 100, big-endian |
| 6 | limit bitfield |
| 7 | sequence |

ID `0x211`, 8 bytes:

| Byte | Field |
| --- | --- |
| 0..1 | accel_x `int16`, m/s^2 * 1000, big-endian |
| 2..3 | accel_y `int16`, m/s^2 * 1000, big-endian |
| 4..5 | accel_z `int16`, m/s^2 * 1000, big-endian |
| 6 | limit bitfield |
| 7 | report_id |

Limit bits:

```text
bit0 = LIMIT_SW1_1
bit1 = LIMIT_SW1_2
bit2 = LIMIT_SW2_1
bit3 = LIMIT_SW2_2
```
