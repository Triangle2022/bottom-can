# bottom-can

STM32G431RB bottom-board firmware for reading a BNO085 IMU and publishing the result on Classic CAN.

The firmware:

- talks to the BNO085 over `SPI2`
- enables ARVR stabilized rotation vector and accelerometer reports
- converts quaternion to roll/pitch/yaw
- reads four limit switch inputs
- controls the PB1 brake/relay output from a CAN command frame
- sends Classic CAN telemetry/status frames at 1 Mbps

## Hardware

MCU/project:

```text
Board: NUCLEO-G431RB
MCU:   STM32G431RB
IDE:   STM32CubeIDE / CubeMX project
```

BNO085 SPI pins:

```text
PB13 = SPI2_SCK
PB14 = SPI2_MISO
PB15 = SPI2_MOSI
PB12 = SPI2_CS
PB10 = SPI2_RST
PB11 = SPI2_INT
```

FDCAN pins:

```text
PA11 = FDCAN1_RX
PA12 = FDCAN1_TX
```

Limit switch inputs:

```text
PC6 = LIMIT_SW1_1
PC7 = LIMIT_SW1_2
PC8 = LIMIT_SW2_1
PC9 = LIMIT_SW2_2
```

Brake/relay output:

```text
PB1 = BRAKE
```

Important: PA11/PA12 are CAN controller pins, not CANH/CANL. Use a CAN transceiver between the STM32 and the CAN bus.

## BNO085 Startup

The BNO085 startup intentionally skips `sh2_getProdIds()` because that command was unstable on this board.

Automatic sequence:

```text
1 reset
2 sh2_open
4 sh2_setSensorCallback
5 sh2_setSensorConfig reports
6 sh2_service loop
```

Manual debug command variable:

```text
bno085_cmd = 1  reset
bno085_cmd = 2  open
bno085_cmd = 3  product IDs, not used in auto flow
bno085_cmd = 4  callback
bno085_cmd = 5  enable reports
bno085_cmd = 6  stream on
bno085_cmd = 7  stream off
```

Useful debug fields:

```text
bno085_dbg.stage
bno085_dbg.cmd_status
bno085_dbg.open_status
bno085_dbg.enable_status
bno085_dbg.callback_count
bno085_dbg.rpy_count
bno085_dbg.accel_count
bno085_dbg.roll_deg
bno085_dbg.pitch_deg
bno085_dbg.yaw_deg
bno085_dbg.accel_x
bno085_dbg.accel_y
bno085_dbg.accel_z
```

If CAN frames are visible but values are zero, first check:

```text
bno085_dbg.rpy_count
bno085_dbg.accel_count
```

`0x210` is only sent after `rpy_count > 0`.  
`0x211` is only sent after `accel_count > 0`.

## CAN Settings

Classic CAN, 1 Mbps:

```text
FDCAN1 nominal bitrate: 1 Mbps
Frame format: Classic CAN
ID type: Standard ID
Auto retransmission: enabled
```

CAN bus requirements:

```text
CANH to CANH
CANL to CANL
GND common
120 ohm termination at bus ends
Same bitrate on every node
```

## CAN Frame Format

All numeric fields are big-endian signed `int16`.

ID `0x210`, 8 bytes:

```text
bytes 0..1: roll  = int16(deg * 100)
bytes 2..3: pitch = int16(deg * 100)
bytes 4..5: yaw   = int16(deg * 100)
byte  6:    limit bitfield
byte  7:    sequence
```

ID `0x211`, 8 bytes:

```text
bytes 0..1: accel_x = int16(m/s^2 * 1000)
bytes 2..3: accel_y = int16(m/s^2 * 1000)
bytes 4..5: accel_z = int16(m/s^2 * 1000)
byte  6:    limit bitfield
byte  7:    BNO085 report_id
```

ID `0x212`, 8 bytes, RX command:

```text
byte 0: command
byte 1: value
bytes 2..7: reserved
```

Supported commands:

```text
command 0x01, value 0x00: brake/relay OFF
command 0x01, value 0x01: brake/relay ON
```

Example command frames:

```text
0x212 [01 01 00 00 00 00 00 00]  PB1/BRAKE ON
0x212 [01 00 00 00 00 00 00 00]  PB1/BRAKE OFF
```

ID `0x213`, 8 bytes, TX status:

```text
byte  0: limit bitfield
byte  1: brake/relay state, 0 OFF / 1 ON
byte  2: BNO085 rx_ready
bytes 3..6: reserved
byte  7: sequence
```

Limit bits:

```text
bit0 = LIMIT_SW1_1
bit1 = LIMIT_SW1_2
bit2 = LIMIT_SW2_1
bit3 = LIMIT_SW2_2
```

The limit inputs use pull-ups, so the firmware reports a limit bit as `1` when the corresponding GPIO reads `GPIO_PIN_RESET`.

## Live Expressions Debug

Useful STM32CubeIDE Live Expressions:

```text
bottom_can_relay_state
bottom_can_tx_seq
bno085_dbg.rx_ready
bno085_dbg.rpy_count
bno085_dbg.accel_count
```

`bottom_can_relay_state` is a state mirror only. Changing it directly in Live Expressions changes the variable value, but it does not call `HAL_GPIO_WritePin()`, so PB1 may not change.

To test the actual PB1 relay output, send the `0x212` CAN command frame instead:

```text
0x212 [01 01 00 00 00 00 00 00]  relay ON
0x212 [01 00 00 00 00 00 00 00]  relay OFF
```

Expected Live Expressions result:

```text
relay ON command  -> bottom_can_relay_state = 1
relay OFF command -> bottom_can_relay_state = 0
```

The relay output is active-high by default:

```text
bottom_can_relay_state = 1 -> PB1 HIGH
bottom_can_relay_state = 0 -> PB1 LOW
```

If the relay module is active-low, swap `BOTTOM_RELAY_ON_LEVEL` and `BOTTOM_RELAY_OFF_LEVEL` in `Core/Src/main.c`.

## Python CAN Tool

Location:

```text
tools/bottom-can-python
```

Use this for SLCAN-style serial USB-CAN adapters.

Install:

```bash
cd /Users/choij/Documents/bottom-can/tools/bottom-can-python
conda create -n bottom-can python=3.11 -y
conda activate bottom-can
pip install -e .
```

Monitor STM32 frames:

```bash
bottom-can monitor --interface slcan --channel /dev/cu.usbserial-XXXX --bitrate 1000000
```

Send fake test frames:

```bash
bottom-can send --interface slcan --channel /dev/cu.usbserial-XXXX --bitrate 1000000 --print
```

The Nucleo USB serial port is not a CAN adapter. A real USB-CAN adapter is required.

## C++ SocketCAN Tool

Location:

```text
tools/bottom-can-cpp
```

Use this on Linux when the USB-CAN adapter appears as a SocketCAN interface such as `can0`.

Build:

```bash
cd ~/bottom-can/tools/bottom-can-cpp
cmake -S . -B build
cmake --build build
```

Bring up CAN:

```bash
sudo ip link set can0 down
sudo ip link set can0 type can bitrate 1000000
sudo ip link set can0 up
```

Monitor:

```bash
./build/bottom-can-cpp monitor --interface can0
```

## ROS Noetic Package

The ROS package is external to this firmware repo:

```text
/Users/choij/Documents/bottom_can_noetic
```

It subscribes to SocketCAN frames and publishes:

```text
/bottom_can/imu         bottom_can_noetic/BottomCanImu
/bottom_can/limits      bottom_can_noetic/LimitState
/bottom_can/raw_frame   bottom_can_noetic/BottomCanFrame
/bottom_can/sensor_imu  sensor_msgs/Imu
```

Build in a catkin workspace:

```bash
mkdir -p ~/catkin_ws/src
cp -r /Users/choij/Documents/bottom_can_noetic ~/catkin_ws/src/
cd ~/catkin_ws
catkin_make
source devel/setup.bash
```

Run:

```bash
sudo ip link set can0 down
sudo ip link set can0 type can bitrate 1000000
sudo ip link set can0 up
roslaunch bottom_can_noetic bottom_can.launch
```

## Quick Debug Checklist

If no CAN frames appear:

```text
1. Is FDCAN1 started?
2. Is there a CAN transceiver between PA11/PA12 and CANH/CANL?
3. Is CAN bitrate 1 Mbps on both sides?
4. Are CANH/CANL/GND/termination correct?
5. Does another node ACK the frames?
```

If CAN frames appear but data is zero:

```text
1. Check bno085_dbg.rpy_count and bno085_dbg.accel_count.
2. Check bno085_dbg.callback_count.
3. Check bno085_dbg.enable_status == 0.
4. Check bno085_dbg.roll_deg / accel_x directly in debugger.
```

If ROS topics exist but values are zero:

```bash
rostopic echo /bottom_can/raw_frame
```

If raw frame data is zero, ROS is decoding correctly and the STM32 payload is zero.
