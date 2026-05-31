# Vending Machine Controller Firmware v2.2.0

## Overview

This firmware runs on an ESP32-based vending machine controller and provides:

* BLE communication with a mobile application
* RFID card reading through UART
* Motor control through PCF8574A I/O expanders
* Door lock control for maintenance access
* Motor rotation validation using a sensor input
* Automatic motor timeout protection
* Conveyor/special relay control
* Mirrored output support on an additional PCF8574A device

---

## Hardware Used

### Controller

* ESP32

### I/O Expanders

| Device      | I2C Address | Purpose                     |
| ----------- | ----------- | --------------------------- |
| PCF8574A #1 | 0x3A        | Motor Selection Port B      |
| PCF8574A #2 | 0x3C        | Motor Selection Port A      |
| PCF8574A #3 | 0x3E        | Conveyor / Auxiliary Relays |
| PCF8574A #4 | 0x3F        | Mirrored Output Port        |

### Inputs

| Signal          | GPIO   |
| --------------- | ------ |
| Rotation Sensor | GPIO18 |
| Sensor 2        | GPIO19 |
| Sensor 3        | GPIO25 |
| Sensor 4        | GPIO26 |
| Sensor 5        | GPIO27 |

### Outputs

| Signal      | GPIO   |
| ----------- | ------ |
| Door Lock 1 | GPIO32 |
| Door Lock 2 | GPIO33 |

### Communication

* BLE
* UART RFID Reader (Serial2)
* I2C

---

## Firmware Features

### BLE Communication

BLE device name:

```text
Wending_Machine_V_051024
```

Firmware version:

```text
2.2.0
```

Functions:

* Receive motor dispense commands
* Receive maintenance commands
* Send RFID card data
* Send dispense completion notifications

---

## RFID Card Processing

RFID cards are read through Serial2.

Format:

```text
R:XXXXXXXXXXXX
```

When a card is scanned:

1. Card ID is read.
2. Data is sent to BLE client.
3. Maintenance cards are checked.

---

## Maintenance Cards

The following RFID cards are authorized for maintenance access:

```text
Maint_Card1
Maint_Card2
Maint_Card3
Maint_Card4
Maint_Card5
```

When a valid maintenance card is scanned:

1. Door1 unlocks.
2. Door2 unlocks.
3. Access remains open for 5 seconds.
4. Doors automatically lock again.

---

## Motor Control

Motor commands are received over BLE.

The firmware decodes:

```text
Motor Number
Rotation Count
```

Example:

```text
Motor 12
Rotate 1 Time
```

The firmware calculates:

```text
PortA_value
PortB_value
```

and drives the required relay combination through PCF8574A devices.

---

## Rotation Verification

Motor movement is verified using:

```text
GPIO18
```

Expected sensor sequence:

```text
LOW → HIGH → LOW
```

Each completed cycle counts as one motor rotation.

---

## Motor Timeout Protection

A hardware timer is used to prevent motors from running indefinitely.

Timeout:

```text
15 Seconds
```

If the expected sensor transitions are not detected before timeout:

1. Motor operation is aborted.
2. All relay outputs are disabled.
3. Controller returns to idle state.

---

## Conveyor / Auxiliary Relay Control

Motor numbers:

```text
91 - 98
```

are treated as special outputs.

These outputs are controlled through:

```text
PCF8574A @ 0x3E
```

Each output is activated briefly and then automatically released.

---

## Mirrored Output Feature

### PCF8574A @ 0x3F

A fourth PCF8574A device has been added.

Purpose:

* Mirror all output values written to PCF8574A @ 0x3A.
* Operate independently from the original device.

Operation:

```text
0x3A receives value
        ↓
0x3F receives same value
        ↓
Remains active for 5 seconds
        ↓
Automatically returns to 0xFF
```

The original PCF8574A @ 0x3A is not affected.

Example:

```text
0x3A = 0xDF
0x3F = 0xDF

After 5 seconds

0x3A = 0xDF
0x3F = 0xFF
```

---

## Configuration

### I2C Addresses

```cpp
#define PortExp_1 0x3A
#define PortExp_2 0x3C
#define PortExp_3 0x3E
#define PortExp_4 0x3F
```

### Port 4 Auto-Off Time

```cpp
#define PORT4_ON_TIME 5000UL
```

Values are specified in milliseconds.

Examples:

```cpp
#define PORT4_ON_TIME 5000UL   // 5 seconds
#define PORT4_ON_TIME 8000UL   // 8 seconds
#define PORT4_ON_TIME 10000UL  // 10 seconds
```

---

## Safety Features

* Motor timeout protection
* Automatic relay shutdown
* BLE auto-reconnect advertising
* Sensor-based dispense verification
* Automatic PortExp_4 shutdown
* Door auto-lock after maintenance access

---

## Revision History

### v2.2.0

* Added PCF8574A @ 0x3F support
* Added mirrored output functionality
* Added automatic 5-second shutdown for PortExp_4
* Optimized timer handling by creating timer once during setup
* Fixed BLE feedback formatting for motor numbers

### v2.1.0

* BLE motor control
* RFID integration
* Maintenance card support
* Conveyor relay control
* Rotation counting and timeout protection
