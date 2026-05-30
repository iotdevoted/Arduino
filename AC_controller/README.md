# Update: ESP8266 Version

## Supported Hardware

This firmware is designed for ESP8266 only.

Supported boards:

* NodeMCU ESP8266
* Wemos D1 Mini
* ESP8266-based custom hardware

ESP32 support has been removed to simplify maintenance and reduce firmware complexity.

---

# Pin Configuration

| Function        | Pin |
| --------------- | --- |
| IR Receiver     | D5  |
| IR Sender       | D1  |
| Learning Button | D7  |
| Status LED      | D2  |
| PWM Input       | D6  |
| UART RX         | D3  |
| UART TX         | D4  |

---

# External UART Interface

The controller provides an external UART interface for communication with another microcontroller, Raspberry Pi, Linux system, PLC, HMI, or external controller.

## UART Configuration

| Parameter | Value  |
| --------- | ------ |
| Baud Rate | 115200 |
| Data Bits | 8      |
| Stop Bits | 1      |
| Parity    | None   |

Communication uses ASCII text commands terminated with newline ('\n').

---

# UART Command Protocol

The controller supports external control through UART commands.

Commands are case-insensitive.

Examples:

ON
on
On

All are treated as the same command.

---

## AC ON Command

Transmit:

ON

Controller Action:

* Sends learned AC ON IR command
* Updates internal AC state

Response:

ON_ACK

---

## AC OFF Command

Transmit:

OFF

Controller Action:

* Sends learned AC OFF IR command
* Updates internal AC state

Response:

OFF_ACK

---

## Temperature Command

Transmit:

TEMP:16
TEMP:20
TEMP:24
TEMP:30

Valid Range:

16°C to 30°C

Controller Action:

* Sends corresponding learned temperature command
* Updates internal temperature state

Responses:

TEMP_16_ACK
TEMP_20_ACK
TEMP_24_ACK
TEMP_30_ACK

Invalid Temperature Example:

TEMP:35

Response:

TEMP_INVALID

---

## Status Command

Transmit:

STATUS

Response Example:

STATUS,AC=ON,TEMP=24

or

STATUS,AC=OFF,TEMP=24

This allows external controllers to determine current operating state.

---

## Unknown Commands

If an unsupported command is received:

Response:

CMD_UNKNOWN

Example:

HELLO

Response:

CMD_UNKNOWN

---

# Example UART Session

External Controller:

ON

ESP8266:

ON_ACK

---

External Controller:

TEMP:26

ESP8266:

TEMP_26_ACK

---

External Controller:

STATUS

ESP8266:

STATUS,AC=ON,TEMP=26

---

External Controller:

OFF

ESP8266:

OFF_ACK

---

# Learning Mode Timeout Enhancement

Future firmware versions will include a learning session timeout.

Current Behavior:

* 20-second timeout per command

Planned Behavior:

* 2-minute overall learning session timeout

If no valid IR command is learned within 2 minutes:

* Exit learning mode
* Restore normal operation
* Resume PWM-based AC control

This prevents the controller from remaining in learning mode indefinitely.

---

# External Controller Integration

The UART protocol is suitable for integration with:

* Raspberry Pi
* Linux systems
* Home Assistant
* Node-RED
* STM32
* AVR
* PIC
* ESP32
* Industrial PLCs
* Touchscreen HMIs

The protocol is intentionally simple and human-readable to ease debugging and integration.
