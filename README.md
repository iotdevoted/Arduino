# ESP8266 / ESP32 Smart AC Controller (IR + PWM Based)

## 📌 Overview

This project is a **universal Smart AC Controller** built for both:

* ESP8266 (NodeMCU)
* ESP32

It learns IR commands from your AC remote and automatically controls temperature based on a PWM input signal.

---

## ⚙️ Features

* 📡 IR Learning (ON / OFF / Temp+ / Temp-)
* 💾 EEPROM storage (persistent after reboot)
* 🌡️ Automatic temperature control via PWM input
* 🔘 Long-press button to enter learning mode
* ⚡ Interrupt-based PWM measurement
* 🔁 Periodic AC adjustment (every 2 seconds)
* 🔄 Same code works on ESP8266 & ESP32

---

## 🧰 Hardware Requirements

* ESP8266 (NodeMCU) OR ESP32
* IR Receiver Module (e.g. TSOP1838)
* IR LED (with transistor recommended)
* Push Button
* Status LED
* PWM signal source (dimmer / controller)

---

## 🔌 Pin Configuration

### ESP8266 (NodeMCU)

| Function    | GPIO | NodeMCU Pin |
| ----------- | ---- | ----------- |
| IR Receiver | 14   | D5          |
| IR Sender   | 5    | D1          |
| Button      | 13   | D7          |
| Status LED  | 4    | D2          |
| PWM Input   | 12   | D6          |

---

### ESP32

| Function    | GPIO |
| ----------- | ---- |
| IR Receiver | 14   |
| IR Sender   | 5    |
| Button      | 13   |
| Status LED  | 2    |
| PWM Input   | 12   |

---

## 📦 Required Library

You must install the following Arduino library:

* **IRremoteESP8266**

### Install via Arduino IDE

1. Open Arduino IDE
2. Go to:

   ```
   Sketch → Include Library → Manage Libraries
   ```
3. Search:

   ```
   IRremoteESP8266
   ```
4. Install latest version

---

### Manual Install (Alternative)

* Download ZIP from:
  https://github.com/crankyoldgit/IRremoteESP8266
* Add via:

  ```
  Sketch → Include Library → Add .ZIP Library
  ```

---

## 🚀 How It Works

### 1. Learning Mode

* Press and hold button for **5 seconds**
* System captures 4 IR commands:

  1. AC ON
  2. AC OFF
  3. Temperature Increase
  4. Temperature Decrease
* Commands are stored in EEPROM

---

### 2. Normal Operation

#### PWM Input → Brightness

* Duty cycle measured using interrupt
* Converted to brightness (0–100%)

#### Brightness → Temperature Mapping

| Brightness | Temperature |
| ---------- | ----------- |
| 15–20      | 16°C        |
| 25         | 17°C        |
| ...        | ...         |
| 99         | 30°C        |

---

### 3. AC Control Logic

* **Brightness = 0**
  → AC OFF

* **Brightness > 0**
  → AC ON (default 24°C)

* Temperature adjusted step-by-step:

  * Sends Temp+ or Temp- IR commands
  * Maintains internal temperature state

---

## 🧠 Internal Flow

```text
PWM Signal
   ↓
Duty Cycle Measurement
   ↓
Brightness (0–100)
   ↓
Mapped Temperature (16–30°C)
   ↓
IR Command Transmission
   ↓
AC Controlled
```

---

## 💾 EEPROM Storage Layout

| Function | Address |
| -------- | ------- |
| ON       | 0       |
| OFF      | 800     |
| TEMP+    | 1200    |
| TEMP-    | 1600    |

Each stores:

* Length (2 bytes)
* Raw IR timing data

---

## ⚠️ Important Notes

* Uses **RAW IR signals** → works with most AC remotes
* Temperature control is **step-by-step**, not direct
* Blocking delays are used (intentional for reliability)
* PWM signal is treated as **inverted**

---

## 🛠️ Troubleshooting

### ❌ Error: `IRremoteESP8266.h: No such file or directory`

👉 Solution:

* Install **IRremoteESP8266** library from Library Manager
* Restart Arduino IDE

---

### ❌ IR Not Working

* Check IR LED polarity
* Use transistor for IR LED
* Ensure correct pin connections

---

### ❌ PWM Not Detected

* Ensure signal is 3.3V compatible
* Check interrupt pin connection

---

## 📂 Project Structure

```
/src
  └── AC_controller.ino

README.md
```

---

## 👨‍💻 Author

Developed for automation and embedded control applications using ESP8266 / ESP32.

---

## 📜 License

This project is open-source and free to use.
