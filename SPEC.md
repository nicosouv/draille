# Draille — Project Specifications

## Overview

**Draille** is an open-source GPS step-tracking firmware for the LILYGO T-Beam Supreme (ESP32-S3).  
The goal is a fully sovereign, privacy-first activity tracker: no cloud, no app, no account — raw data owned by the user.

Named after the ancient transhumance trails of the Tricastin region (Bollène, Lapalud, Saint-Paul-Trois-Châteaux), Provence, France.

---

## Hardware

| Component | Model | Interface |
|---|---|---|
| MCU | LILYGO T-Beam Supreme (ESP32-S3FN8) | — |
| GPS | Ublox NEO-M10S (on-board) | UART (Serial1) |
| IMU | QMI8658 6-axis (on-board) | I2C |
| LoRa | SX1262 868MHz (on-board) | SPI |
| Display | Waveshare GC9A01 1.28" round TFT 240x240 | SPI |
| Battery | 18650 LiPo (slot on-board) | — |
| Buttons | 3x external tactile momentary buttons | GPIO (INPUT_PULLUP) |

### GPIO Pin Mapping

> ⚠️ Verify against official LILYGO T-Beam Supreme schematic before wiring:
> https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series

```cpp
// GPS (UART)
#define GPS_RX_PIN   34
#define GPS_TX_PIN   12

// Display GC9A01 (SPI)
#define TFT_MOSI     11
#define TFT_SCLK     12
#define TFT_CS       10
#define TFT_DC        9
#define TFT_RST       8

// IMU QMI8658 (I2C)
#define IMU_SDA      21
#define IMU_SCL      22

// Buttons (external, active LOW with INPUT_PULLUP)
#define BTN_PREV      4   // Navigate previous / back
#define BTN_NEXT     38   // Navigate next / forward
#define BTN_SELECT   39   // Confirm / start / stop
```

---

## Development Environment

- **IDE**: CLion with PlatformIO plugin
- **Framework**: Arduino (via PlatformIO)
- **Platform**: espressif32
- **Board**: esp32-s3-devkitc-1
- **Language**: C++17

### `platformio.ini`

```ini
[env:draille]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
upload_speed = 921600

build_flags =
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue

lib_deps =
    mikalhart/TinyGPSPlus @ ^1.0.3
    bodmer/TFT_eSPI @ ^2.5.43
    bblanchon/ArduinoJson @ ^7.0.0
    lewisxhe/SensorLib @ ^0.1.4
```

### TFT_eSPI `User_Setup.h`

```cpp
#define GC9A01_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 240
#define TFT_MOSI   11
#define TFT_SCLK   12
#define TFT_CS     10
#define TFT_DC      9
#define TFT_RST     8
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define SPI_FREQUENCY 27000000
```

---

## Project Structure

```
draille/
├── platformio.ini
├── SPECS.md
├── README.md
├── src/
│   ├── main.cpp              # Entry point: setup() + loop()
│   ├── config.h              # All constants, GPIO, tuning params
│   ├── gps/
│   │   ├── GpsManager.h
│   │   └── GpsManager.cpp    # GPS read, NMEA parsing, track recording
│   ├── imu/
│   │   ├── ImuManager.h
│   │   └── ImuManager.cpp    # Step counting via QMI8658 accelerometer
│   ├── display/
│   │   ├── DisplayManager.h
│   │   └── DisplayManager.cpp # All screen rendering logic
│   ├── storage/
│   │   ├── TrackStorage.h
│   │   └── TrackStorage.cpp  # GPX file writing to SPIFFS/SD
│   ├── ui/
│   │   ├── UIManager.h
│   │   └── UIManager.cpp     # Screen state machine, button handling
│   └── export/
│       ├── WifiExport.h
│       └── WifiExport.cpp    # WiFi AP + HTTP server for GPX export
└── data/                     # SPIFFS static files if needed
```

---

## Features

### Core Features (MVP)

1. **GPS Tracking**
    - Read NMEA sentences from Ublox NEO-M10S via UART
    - Parse with TinyGPS++: lat, lon, altitude, speed, satellites, hdop
    - Record track points every 5 seconds when tracking is active
    - Store track as GPX format in SPIFFS

2. **Step Counting**
    - Read QMI8658 accelerometer via I2C
    - Detect steps using peak detection on acceleration magnitude
    - Display cumulative step count for current session
    - Reset on new session start

3. **Display**
    - Round GC9A01 240x240 TFT, optimized for circular layout
    - Multiple screens (see UI section below)
    - Auto-dim after 30 seconds of inactivity
    - Wake on any button press

4. **Button Navigation**
    - BTN_PREV: previous screen
    - BTN_NEXT: next screen
    - BTN_SELECT: context action (start/stop tracking, confirm)
    - Long press BTN_SELECT (2s): power screen / WiFi export mode
    - Debounce: 50ms software debounce on all buttons

5. **GPX Export via WiFi**
    - On demand: long press BTN_SELECT → WiFi mode
    - ESP32 creates a WiFi Access Point: SSID `Draille`, no password
    - User connects with phone/laptop
    - HTTP server at `192.168.4.1` serves:
        - `GET /` → list of recorded tracks (JSON)
        - `GET /track/{id}.gpx` → download GPX file
        - `DELETE /track/{id}` → delete track
    - WiFi mode auto-exits after 5 minutes of inactivity

### Future Features (post-MVP)

- Battery percentage display (via ADC read on battery pin)
- Configurable track point interval
- Meshtastic integration via LoRa for offline mesh sharing
- Heart rate (if external sensor added)

---

## UI / Screen States

The UI is a simple circular state machine navigated with PREV/NEXT buttons.

```
[BOOT] → [DASHBOARD] ↔ [TRACKING] ↔ [STATS] ↔ [TRACKS] ↔ [SETTINGS]
                                                              ↓
                                                         [WIFI EXPORT]
```

### Screen: DASHBOARD (default)

```
     ┌─────────────────┐
     │   12:34         │  ← time (from GPS)
     │                 │
     │  📍 48.8566     │  ← lat
     │     2.3522      │  ← lon
     │                 │
     │  🛰 8 sats      │  ← satellites
     │  👟 1,234 steps │  ← step count
     └─────────────────┘
```

### Screen: TRACKING

```
     ┌─────────────────┐
     │  ● REC  00:23   │  ← recording indicator + duration
     │                 │
     │  2.4 km         │  ← distance
     │  1.2 km/h       │  ← speed
     │  +45 m          │  ← elevation gain
     │                 │
     │  [SELECT: STOP] │
     └─────────────────┘
```

### Screen: STATS

```
     ┌─────────────────┐
     │  Session Stats  │
     │                 │
     │  Steps : 4,521  │
     │  Dist  : 3.2 km │
     │  Time  : 01:02  │
     │  Avg   : 3.1km/h│
     └─────────────────┘
```

### Screen: TRACKS LIST

```
     ┌─────────────────┐
     │  Saved Tracks   │
     │                 │
     │  > 2024-03-08   │  ← selectable list
     │    2024-03-07   │
     │    2024-03-05   │
     │                 │
     │  [SELECT: VIEW] │
     └─────────────────┘
```

### Screen: WIFI EXPORT

```
     ┌─────────────────┐
     │   WiFi Export   │
     │                 │
     │  SSID: Draille  │
     │  192.168.4.1    │
     │                 │
     │  Waiting...     │
     │  [SELECT: EXIT] │
     └─────────────────┘
```

---

## Data Model

### Track Point

```cpp
struct TrackPoint {
    double   lat;          // degrees
    double   lon;          // degrees
    float    altitude;     // meters
    float    speed;        // km/h
    uint32_t timestamp;    // Unix epoch (from GPS)
    uint8_t  satellites;
    float    hdop;
};
```

### Session

```cpp
struct Session {
    uint32_t   startTime;      // Unix epoch
    uint32_t   endTime;
    uint32_t   stepCount;
    float      totalDistance;  // km
    float      elevationGain;  // meters
    uint16_t   pointCount;
    char       filename[32];   // GPX filename on SPIFFS
};
```

### GPX Output Format

```xml
<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="Draille"
     xmlns="http://www.topografix.com/GPX/1/1">
  <metadata>
    <name>Draille Track 2024-03-08</name>
    <time>2024-03-08T08:30:00Z</time>
  </metadata>
  <trk>
    <name>Morning walk</name>
    <trkseg>
      <trkpt lat="44.2800" lon="4.7200">
        <ele>85.0</ele>
        <time>2024-03-08T08:30:05Z</time>
        <extensions>
          <speed>3.2</speed>
        </extensions>
      </trkpt>
      <!-- ... -->
    </trkseg>
  </trk>
</gpx>
```

---

## Storage

- **SPIFFS** (internal flash, ~1.5MB available)
- One GPX file per session: `/tracks/YYYYMMDD_HHMMSS.gpx`
- Session index: `/sessions.json` (array of Session metadata)
- Max stored sessions: 20 (oldest auto-deleted when full)
- Estimated size per session: ~50KB for 1h walk at 1 point/5s

---

## Step Detection Algorithm

Use QMI8658 accelerometer raw data (X, Y, Z axes) sampled at ~50Hz.

```
1. Compute magnitude: mag = sqrt(x² + y² + z²)
2. Apply low-pass filter (alpha=0.1) to remove noise
3. Detect peaks above threshold (1.2g) with min interval 300ms
4. Each peak = 1 step
5. Calibration: adjustable threshold via SETTINGS screen
```

---

## Code Style & Conventions

- Comments in **English**
- Class names: `PascalCase`
- Methods and variables: `camelCase`
- Constants and macros: `UPPER_SNAKE_CASE`
- Each module encapsulated in its own class with `.h` / `.cpp` pair
- No blocking `delay()` in main loop — use `millis()` based timers
- Serial debug output only in `DEBUG` build flag

```cpp
// Good
#ifdef DEBUG
  Serial.printf("[GPS] Lat: %.6f Lon: %.6f\n", lat, lon);
#endif

// Bad
delay(1000); // never in loop()
```

---

## Non-Goals

- No cloud sync, no account, no telemetry
- No Bluetooth (to preserve battery)
- No OTA firmware update (keep it simple)
- No touchscreen interaction (buttons only for robustness)
- LoRa not used in MVP (hardware present but unused)