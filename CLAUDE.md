# Draille — Project Context for Claude

## What this project is

ESP32-S3 (LILYGO T-Beam Supreme) GPS step-tracking firmware. PlatformIO + Arduino framework.

## Architecture

```
src/
  main.cpp          — setup/loop, module wiring
  config.h          — all pin constants, thresholds, paths
  types.h           — shared structs (TrackPoint, Session) — avoids circular includes
  gps/              — TinyGPS++ over UART, manual UTC epoch, Haversine accumulation
  imu/              — SensorLib QMI8658, LPF step detection (α=0.1, threshold 1.2g)
  display/          — TFT_eSPI sprite-based rendering, Inter smooth fonts from SPIFFS
  ui/               — state machine (DASHBOARD→TRACKING→STATS→TRACKS), button FSM
  storage/          — SPIFFS GPX streaming, session index JSON, auto-purge at 20 sessions
  export/           — WiFi soft AP, HTTP GET/DELETE routes for GPX files
test/
  native/           — desktop Unity tests (timestamp, haversine, step algo, gpx format)
  embedded/         — hardware integration tests (spiffs, imu)
docs/               — all .md docs except README.md
tools/              — gen_fonts.py (Inter TTF → .vlw)
```

## Key conventions

- All .md docs go in `docs/` (exception: README.md at root, CLAUDE.md at root)
- Commits: English, one-liner, no emoji, no "by Claude"
- No `const` on GpsManager getters — TinyGPS++ methods aren't const
- ISO8601 timestamp buffers must be `char[21]` (20 chars + null)
- `<FS.h>` must be included before TFT_eSPI in every file that uses both

## Display

- 240×240 round GC9A01, full sprite (TFT_eSprite) pushed atomically
- Dark mode palette: BG=0x0000, WHITE=0xFFFF, BLUE=0x043F, GREEN=0x368B, RED=0xFA07, AMBER=0xFCA0
- Fonts: Inter_Bold_48, Inter_Medium_24, Inter_Light_18 (loaded from SPIFFS)
- Periodic render at 100ms for animated screens (WiFi, GPS spinner, REC pulse)

## Build & test

```bash
make build        # compile
make test         # native tests (no device needed)
make flash        # build + upload
make monitor      # serial monitor
make fs-flash     # upload SPIFFS (fonts)
```

## Agents

Specialized sub-agents in `.claude/agents/`. Invoke them by mentioning the task or via the Agent tool.

| Agent | When to use |
|---|---|
| `firmware-review` | After modifying any `src/` file — catches ESP32-specific pitfalls (memory, timing, SPIFFS, TFT include order) |
| `ui-designer` | When designing a new screen — produces data struct + `drawXxx()` skeleton consistent with the Apple dark palette |
| `test-writer` | When adding a pure algorithm — scaffolds a `test/native/` Unity suite and verifies it passes |

## Slash commands

| Command | What it does |
|---|---|
| `/commit` | Generate a one-liner commit message from staged diff |
| `/build`  | Compile and show only errors/warnings |
| `/test`   | Run native tests and summarize results |
| `/flash`  | Build + flash to connected T-Beam Supreme |
| `/review` | Firmware quality review of current diff |
| `/screen` | Scaffold a new display screen end-to-end |
