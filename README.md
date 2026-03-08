# Draille

Open-source GPS step-tracking firmware for the **LILYGO T-Beam Supreme (ESP32-S3)**.

Fully sovereign, privacy-first activity tracker — no cloud, no account, no app required. Raw data owned by the user, exported as standard GPX files over a local WiFi hotspot.

Named after the ancient transhumance trails of the Tricastin region, Provence, France.

---

## Hardware

| Component | Model | Interface |
|---|---|---|
| MCU | LILYGO T-Beam Supreme (ESP32-S3FN8) | — |
| GPS | Ublox NEO-M10S | UART |
| IMU | QMI8658 6-axis | I2C |
| LoRa | SX1262 868MHz | SPI *(unused in MVP)* |
| Display | Waveshare GC9A01 1.28" round TFT 240×240 | SPI |
| Battery | 18650 LiPo | — |
| Buttons | 3× external tactile momentary | GPIO INPUT_PULLUP |

---

## Quick Start

### Requirements

- [PlatformIO](https://platformio.org/) (CLI or CLion plugin)
- Python 3.8+ (for font generation)
- macOS / Linux

### Build & Flash

```bash
make build        # compile firmware
make flash        # compile + flash (auto-detects serial port)
make monitor      # open serial monitor at 115200 baud
make run          # build + flash + monitor in one step
```

Specify port manually if auto-detection fails:

```bash
make flash-port PORT=/dev/cu.usbserial-XXXX
```

### Flash the filesystem (SPIFFS)

The filesystem holds fonts and track data. Flash it once before first use:

```bash
make fs-flash
```

---

## User Guide

### Buttons

| Button | Short press | Long press (2s) |
|---|---|---|
| **PREV** (GPIO 4) | Previous screen | — |
| **NEXT** (GPIO 38) | Next screen | — |
| **SELECT** (GPIO 39) | Context action | Open WiFi Export |

Any button press wakes the display if dimmed.

### Screen Navigation

Screens form a circular loop navigated with PREV / NEXT:

```
DASHBOARD ↔ TRACKING ↔ STATS ↔ TRACKS
```

WiFi Export is accessed via **long press SELECT** from any screen.

---

### Screen: Dashboard

Default screen. Shows current GPS status, time, coordinates, and step count.

- **Satellite ring** — outer arc segments, one per satellite. Color indicates signal quality: green (8+), amber (4–7), red (<4)
- **GPS dot** — green = fix, amber = weak, red = no fix
- **Time** — UTC time from GPS
- **Coordinates** — lat/lon (hidden until GPS fix acquired)
- **Step count** — cumulative steps for the current session

### Screen: Tracking

Start and stop a GPS recording session.

- **Outer ring** — session time progress (full = 1 hour)
- **Inner ring** — distance progress (full = 5 km)
- **Center** — current speed in km/h
- **REC indicator** — pulsing red dot when recording is active
- **SELECT** — start or stop recording

When recording starts:
- IMU step counter resets to zero
- GPS track points are recorded every 5 seconds
- A GPX file is created on SPIFFS

### Screen: Stats

Session summary for the current or last session.

| Field | Description |
|---|---|
| Steps | Total step count |
| Distance | Total distance in km |
| Duration | Elapsed session time |
| Avg speed | Average speed in km/h |

### Screen: Tracks

List of saved sessions stored on the device.

- **PREV / NEXT** — scroll through the list
- **SELECT** — cycle through entries
- Up to **20 sessions** are stored; the oldest is automatically deleted when full

### Screen: WiFi Export

Activated by **long press SELECT** from any screen.

- The device creates a WiFi access point: **SSID `Draille`, no password**
- Connect with any phone or laptop
- Access `http://192.168.4.1` in a browser or HTTP client
- WiFi mode **auto-exits after 5 minutes** of inactivity

---

## WiFi Export API

| Method | Endpoint | Description |
|---|---|---|
| `GET` | `/` | List all saved sessions (JSON) |
| `GET` | `/track/{filename}.gpx` | Download a GPX file |
| `DELETE` | `/track/{filename}` | Delete a session |

### Example: list sessions

```bash
curl http://192.168.4.1/
```

```json
[
  {
    "id": "/tracks/20240308_083000.gpx",
    "startTime": 1709886600,
    "endTime": 1709890200,
    "stepCount": 4521,
    "totalDistance": 3.24,
    "elevationGain": 87.5,
    "pointCount": 720
  }
]
```

### Example: download a track

```bash
curl http://192.168.4.1/track/20240308_083000.gpx -o morning_walk.gpx
```

### Example: delete a track

```bash
curl -X DELETE http://192.168.4.1/track/20240308_083000.gpx
```

The downloaded `.gpx` file is standard GPX 1.1, compatible with:
Strava, Komoot, Garmin Connect, IGN Rando, Google Earth, QGIS, and any GPX-compatible app.

---

## Storage

| Path | Content |
|---|---|
| `/tracks/YYYYMMDD_HHMMSS.gpx` | One GPX file per session |
| `/sessions.json` | Session index (metadata) |

- Max **20 sessions** stored simultaneously
- Estimated **~50 KB per session** (1h walk, 1 point/5s)
- ~1.5 MB available in SPIFFS → ~30 hours of tracking total

---

## Display & Fonts

The display uses **Inter** (open-source, SIL license) rendered as anti-aliased smooth fonts via TFT_eSPI.

The firmware works out of the box with built-in fonts. To enable smooth Inter fonts:

**1. Download Inter**

[https://fonts.google.com/specimen/Inter](https://fonts.google.com/specimen/Inter)

Extract into `fonts/` at project root:

```
fonts/
├── Inter_28pt-Bold.ttf
├── Inter_24pt-Medium.ttf
└── Inter_18pt-Light.ttf
```

**2. Generate `.vlw` files**

```bash
pip install freetype-py
python3 tools/gen_fonts.py
```

**3. Flash to device**

```bash
make fs-flash
```

See `docs/fonts.md` for full details.

---

## Development

### Project Structure

```
draille/
├── platformio.ini          # build configuration
├── Makefile                # build / flash / test shortcuts
├── diagram.json            # Wokwi simulation circuit
├── wokwi.toml              # Wokwi configuration
├── src/
│   ├── main.cpp            # setup() + loop()
│   ├── config.h            # all GPIO, timings, constants
│   ├── types.h             # shared data structs (TrackPoint, Session)
│   ├── gps/                # GPS UART reading + NMEA parsing
│   ├── imu/                # QMI8658 step counting
│   ├── display/            # TFT_eSPI rendering, all screens
│   ├── ui/                 # screen state machine, button handling
│   ├── storage/            # GPX writing, SPIFFS session index
│   └── export/             # WiFi AP + HTTP server
├── test/
│   ├── native/             # unit tests — run on desktop, no hardware
│   │   ├── test_timestamp/ # GPS UTC epoch calculation
│   │   ├── test_haversine/ # distance calculation
│   │   ├── test_step_algo/ # step detection algorithm
│   │   └── test_gpx_format/# GPX XML output format
│   └── embedded/           # integration tests — run on device
│       ├── test_spiffs/    # SPIFFS read/write
│       └── test_imu/       # IMU I2C + data
├── tools/
│   └── gen_fonts.py        # Inter TTF → .vlw converter
├── docs/
│   ├── PLAN.md             # build plan + phase status
│   ├── PLAN_NAVIGATION.md  # upcoming navigation feature plan
│   └── fonts.md            # font setup guide
└── data/                   # SPIFFS files (fonts .vlw) — not committed
```

### Running Tests

```bash
make test            # native unit tests (no hardware required)
make test-embedded   # integration tests on device
make test-verbose    # native tests with assertion details
```

Native tests cover: GPS epoch calculation, Haversine distance, step detection algorithm (LPF + peak detection), GPX XML format.

### Wokwi Simulation

The project includes a Wokwi circuit (`diagram.json`) for partial simulation:

- Supported: ESP32-S3, 3 buttons, SPIFFS, serial output
- Not supported: GC9A01 display, QMI8658 IMU

Open in [wokwi.com](https://wokwi.com) or via the VS Code / CLion extension.

To simulate GPS input in Wokwi, paste NMEA sentences into the serial monitor (Serial1).

### Debug Output

Enable verbose serial logging via the `DEBUG` build flag (active by default in `platformio.ini`):

```
[BOOT] Draille starting...
[GPS] UART initialized
[IMU] QMI8658 initialized
[DISPLAY] Init — smooth fonts: yes
[BOOT] All modules initialized.
[GPS] Point recorded: 44.280012, 4.720045 alt=85.3m
[IMU] Step #142 (mag=1.247)
[STORAGE] Session closed — 3.24km, 4521 steps, 720 points
[WIFI] AP started — SSID: Draille  IP: 192.168.4.1
```

Remove `-DDEBUG` from `platformio.ini` build flags for production builds.

---

## Makefile Reference

```bash
make build           # compile firmware
make flash           # compile + flash (auto port)
make flash-port      # compile + flash (PORT=/dev/cu.xxx)
make monitor         # serial monitor 115200 baud
make run             # build + flash + monitor
make clean           # clean build artifacts
make fs-build        # build SPIFFS image
make fs-flash        # flash SPIFFS to device
make ports           # list available serial ports
make test            # native unit tests
make test-embedded   # integration tests on device
make test-verbose    # native tests verbose
```

---

## Roadmap

- [x] GPS tracking + GPX recording
- [x] Step counting (QMI8658 peak detection)
- [x] Round TFT display (GC9A01) with smooth fonts
- [x] Button navigation (3-button state machine)
- [x] WiFi GPX export (AP + HTTP server)
- [ ] GPX route import via WiFi
- [ ] Turn-by-turn navigation with cross-track guidance
- [ ] Battery percentage display
- [ ] Configurable track point interval
- [ ] Meshtastic integration via LoRa

---

## License

MIT — see LICENSE file.

Data format (GPX 1.1): [topografix.com/gpx](https://www.topografix.com/gpx.asp)
Font (Inter): SIL Open Font License — [rsms.me/inter](https://rsms.me/inter)
