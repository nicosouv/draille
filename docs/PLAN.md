# Draille — Build Plan

## Project State

- `platformio.ini`: present, missing `SensorLib` dependency
- `src/`: does not exist — project is empty
- No C++ source files yet

---

## Phase 1 — Foundation

**Goal: compilable project with empty loop**

| File | Content |
|---|---|
| `platformio.ini` | Add `SensorLib`, add `DEBUG` build flag |
| `src/config.h` | All GPIO constants, timings, SSID, thresholds |
| `src/main.cpp` | `setup()` + `loop()` skeletons, manager instantiation |

Dependencies: none. Starting point.

---

## Phase 2 — Hardware Drivers (independent)

**Goal: read GPS and IMU, output to Serial debug**

| Module | Files | Dependencies |
|---|---|---|
| GPS | `gps/GpsManager.h/.cpp` | TinyGPS++, `config.h` |
| IMU | `imu/ImuManager.h/.cpp` | SensorLib, `config.h` |

- **GPS**: UART Serial1, NMEA parsing, exposes lat/lon/alt/speed/sats/hdop/time
- **IMU**: I2C QMI8658, magnitude + low-pass filter + peak detection → step count

> GPS and IMU have no dependency on each other — build in parallel.

---

## Phase 3 — Storage

**Goal: write and read GPX files on SPIFFS**

| File | Content |
|---|---|
| `storage/TrackStorage.h/.cpp` | Init SPIFFS, open session, append TrackPoint, close → valid GPX |

- `TrackPoint` and `Session` structs as defined in SPEC
- Session index at `/sessions.json` (max 20, auto-purge oldest)
- GPX 1.1 format compliant with SPEC

Dependencies: `config.h` only. Testable in isolation with dummy points.

---

## Phase 4 — Display + UI

**Goal: navigable interface with 3 buttons**

| Module | Files | Content |
|---|---|---|
| Display | `display/DisplayManager.h/.cpp` | TFT_eSPI init, render all screens |
| UI | `ui/UIManager.h/.cpp` | State machine, button debounce, long press |

Build order:
1. `DisplayManager`: GC9A01 init, render 5 screens (DASHBOARD, TRACKING, STATS, TRACKS, WIFI)
2. `UIManager`: circular state machine, button polling with 50ms debounce, 2s long press on BTN_SELECT

> `DisplayManager` and `UIManager` are tightly coupled — build together.

---

## Phase 5 — WiFi Export

**Goal: WiFi AP + HTTP server to retrieve GPX files**

| File | Content |
|---|---|
| `export/WifiExport.h/.cpp` | WiFi AP `Draille`, WebServer, GET/DELETE routes, 5min timeout |

Routes:
- `GET /` → JSON list of sessions
- `GET /track/{id}.gpx` → GPX file download
- `DELETE /track/{id}` → delete track

Dependencies: `TrackStorage`. Independent from display/UI.

---

## Phase 6 — Integration

**Goal: everything wired in `main.cpp`, complete firmware**

1. `setup()`: init GPS, IMU, Display, Storage, UI
2. `loop()`: orchestrate each manager's `update()` — no `delay()`
3. Wire callbacks: tracking active → `GpsManager` → `TrackStorage.appendPoint()`
4. Wire UIManager → trigger WifiExport on long press
5. Display auto-dim after 30s inactivity

---

## Delivery Order

```
Phase 1 → Phase 2 (GPS ∥ IMU) → Phase 3 → Phase 4 → Phase 5 → Phase 6
```

---

## Status

| Phase | Status |
|---|---|
| 1 — Foundation | done |
| 2 — GPS | done |
| 2 — IMU | done |
| 3 — Storage | done |
| 4 — Display + UI | done |
| 5 — WiFi Export | done |
| 6 — Integration | done |
