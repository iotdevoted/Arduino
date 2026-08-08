# LM35 Trigger Feature

## Overview

The firmware supports an LM35-based trigger mechanism that can initiate AC operation independently of the PWM brightness control logic.

This feature is intended for external systems that require a one-time AC startup based on a hardware trigger signal.

---

## Hardware Connections

| Signal             | ESP8266 Pin |
| ------------------ | ----------- |
| LM35 Analog Output | A0          |
| LM35 Trigger Input | D0          |

### LM35 Sensor

The LM35 temperature sensor is connected to the ESP8266 analog input.

The firmware reads the room temperature only when the trigger input becomes active.

---

## Operating Principle

The LM35 feature is event-driven.

### Trigger LOW

When:

D0 = LOW

The firmware performs no LM35-related actions.

The normal PWM-based AC control remains active.

---

### Trigger HIGH

When:

D0 = HIGH

The firmware performs the following sequence once:

1. Read the LM35 temperature.
2. Print the measured temperature to the debug console.
3. If the AC is OFF:

   * Send the learned AC ON command.
4. Determine the target AC temperature:

   * Use the most recent UART temperature command (`TEMP:XX`).
   * If no valid UART temperature exists, use the default temperature (24°C).
5. Send the corresponding learned AC temperature command.
6. Update internal AC state information.
7. Return to normal operation.

---

## One-Shot Trigger Behavior

The LM35 trigger is processed only once per activation.

Example:

Trigger LOW
→ No action

Trigger HIGH
→ Execute LM35 sequence

Trigger remains HIGH
→ No additional commands sent

Trigger LOW
→ Trigger is re-armed

Trigger HIGH again
→ Execute LM35 sequence again

This prevents repeated transmission of AC ON and temperature commands.

---

## Interaction with PWM Control

The existing PWM brightness control logic remains unchanged.

The LM35 trigger does not replace PWM control.

After the trigger sequence completes:

* PWM monitoring continues normally.
* Brightness-to-temperature mapping continues normally.
* Existing AC automation remains unchanged.

---

## UART Temperature Integration

The LM35 trigger uses the latest temperature received through UART.

Example:

TEMP:26

Response:

TEMP_26_ACK

The next LM35 trigger event will:

* Turn the AC ON (if required).
* Set the AC temperature to 26°C.

If no UART temperature has been configured, the firmware uses:

24°C

as the default target temperature.

---

## Debug Output Example

LM35 Temp = 31.4 C

LM35 Trigger -> AC ON

LM35 Trigger -> Set Temp 26

This output is available on the primary serial debug interface.

---

## Notes

* LM35 temperature is currently used for monitoring and event logging.
* The measured room temperature does not currently affect AC setpoint calculations.
* Future firmware versions may implement dynamic temperature control based on LM35 readings.
* The trigger mechanism is edge-based and prevents repeated IR transmissions while the trigger input remains active.
