# bottom-can C++ SocketCAN Tool

C++ monitor/sender for the `bottom-can` firmware using Linux SocketCAN.

This tool reads and writes CAN frames on an interface such as `can0`. It does not use SLCAN serial commands and does not use `python-can`.

## Requirements

- Linux with SocketCAN support.
- A USB-CAN adapter exposed as `can0`, `can1`, etc.
- CAN bus speed: `1 Mbps`.

## Bring Up CAN Interface

Check interfaces:

```bash
ip link
```

Set `can0` to 1 Mbps:

```bash
sudo ip link set can0 down
sudo ip link set can0 type can bitrate 1000000
sudo ip link set can0 up
```

Optional quick check with `can-utils`:

```bash
candump can0
```

## Build On Linux

```bash
cd /Users/choij/Documents/bottom-can/tools/bottom-can-cpp
cmake -S . -B build
cmake --build build
```

On a Linux machine the path may be different, for example:

```bash
cd ~/bottom-can/tools/bottom-can-cpp
cmake -S . -B build
cmake --build build
```

## Monitor STM32 Frames

```bash
./build/bottom-can-cpp monitor --interface can0
```

Expected decoded frames:

```text
0x210 roll=1.23 pitch=-4.56 yaw=90 limits=0x5 seq=12
0x211 ax=0.1 ay=0 az=9.81 limits=0x5 report_id=0x28
```

## Send Test Frames

```bash
./build/bottom-can-cpp send --interface can0 --print
```

One sample:

```bash
./build/bottom-can-cpp send --interface can0 --once --roll 1.23 --pitch -4.56 --yaw 90 --ax 0.1 --ay 0 --az 9.81 --limits 0x05 --print
```

Animated demo:

```bash
./build/bottom-can-cpp send --interface can0 --demo --print
```

## Frame IDs

- `0x210`: roll, pitch, yaw, limit bits, sequence
- `0x211`: accel_x, accel_y, accel_z, limit bits, report_id

