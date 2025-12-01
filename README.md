# ESP32-3248S035 Wi-Fi Scanner

Real-time 2.4 GHz Wi-Fi Scanner with TFT Display (ST7796)
Author: Saul Bejarano

---

## Overview

This project transforms the **Freenove ESP32-3248S035C** development board into a real-time Wi-Fi scanning tool with:

* Live scanning of nearby 2.4 GHz Wi-Fi networks
* Smooth, flicker-free on-screen updates
* Color-coded RSSI bar graphs
* Sorting by signal strength (best → worst)
* True 1-second scanning intervals using low-level ESP32 Wi-Fi APIs
* Serial output for logging or war-driving applications
* LED scan indicator (active-low blink)

This scanner is ideal for:

* Wi-Fi troubleshooting
* War-driving
* RF spectrum analysis
* Site surveys

The scanner does **not** connect to any network. It only listens to the Wi-Fi environment.

---

## Hardware Required

### ✔ ESP32-3248S035C Development Board

Includes:

* ESP32 dual-core processor
* 3.5" 480x320 ST7796 TFT display
* GT911 capacitive touch controller (unused in this version)
* Onboard red LED (active-low)
* TFT backlight controlled via GPIO 27

Reference project:
[https://github.com/ardnew/ESP32-3248S035](https://github.com/ardnew/ESP32-3248S035)

---

## 🖥 TFT Configuration (TFT_eSPI)

Configured for the ST7796 display using either `platformio.ini` or `User_Setup.h`.

Essential definitions:

```
ST7796_DRIVER=1
TFT_CS=15
TFT_DC=2
TFT_RST=-1
TFT_WIDTH=320
TFT_HEIGHT=480
TFT_BCKL=27
SPI_FREQUENCY=65000000
```

Touch settings can be disabled if not used.

---

## Features

### ✔ 1-Second Fast Wi-Fi Scan

Uses low-level ESP-IDF Wi-Fi APIs instead of the slow Arduino `scanNetworks()`, reducing blocking scan time from ~6 seconds to ~300 ms.

### ✔ Smooth Display Updates

Only the AP entries that change are redrawn — no full-screen flicker.

### ✔ Professional Color-Coded RSSI Bars

| RSSI Range | Category       | Color    |
| ---------- | -------------- | -------- |
| ≥ -67 dBm  | Excellent/Good | Green    |
| -68..-70   | Fair           | Yellow   |
| -71..-80   | Poor           | Red      |
| < -80      | Very Poor      | Burgundy |

### ✔ LED Scan Indicator

The onboard LED is **active-low**:

* OFF normally
* Brief ON pulse when a scan completes

### ✔ Serial Output

Useful for logging, mapping, or debugging.
Example output:

```
AA:BB:CC:DD:EE:FF  -62dBm  CH6  2437MHz  MyWiFi
```

### ✔ Multi-Core RTOS Architecture

* **Core 0** handles Wi-Fi scanning
* **Core 1** handles display rendering

Ensures ultra-smooth UI with uninterrupted scanning.

---

## Installation

### Arduino IDE

1. Install **ESP32 board support** (v3.x.x recommended)
2. Install **TFT_eSPI**
3. Configure `User_Setup.h` or `platformio.ini`
4. Load the provided `.ino` file
5. Select board: **ESP32 Dev Module**
6. Upload

---

## Code Structure

```
PROMISCUOUS.ino
├── wifiScanTask()       // Fast Wi-Fi scan on Core 0
├── displayTask()        // Screen update task on Core 1
├── drawCard()           // Draw individual AP blocks
├── macToString()        // MAC formatter
└── setup()              // Initialize TFT + tasks
```

---

## 🧪 What It Detects

For all visible (non-hidden) 2.4 GHz networks:

* SSID
* BSSID (MAC address)
* RSSI (signal level)
* Channel
* Frequency (derived)
* Color-coded signal bar

---

## ⚠ Limitations

* ESP32 supports **2.4 GHz only** (no 5 GHz scanning)
* Promiscuous packet capture not included in this version
* Touchscreen unused

---

## Future Enhancements

Potential add-ons:

* Touch-based UI for sorting and filtering
* Spectrum waterfall view
* Channel utilization map
* Promiscuous mode packet counters
* GPS integration for war-driving
* Auto-save JSON/CSV logs
* SD card logging support

---

## License

MIT License — free to use and modify.

---

## Contributions

Pull requests are welcome.
If you extend the tool or build something cool with it, feel free
