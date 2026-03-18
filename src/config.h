#pragma once

// ─── GPS (UART, onboard — internal PCB routing) ──────────────────────────────
#define GPS_RX_PIN          9   // GNSS TX → ESP32 RX
#define GPS_TX_PIN          8   // GNSS RX ← ESP32 TX
#define GPS_EN_PIN          7   // GNSS enable (active HIGH)
#define GPS_BAUD            9600
#define GPS_RECORD_INTERVAL 5000    // ms between track points

// ─── Display GC9A01 (SPI, expansion header GPIOs) ────────────────────────────
#define TFT_MOSI             3
#define TFT_SCLK             6
#define TFT_CS              45
#define TFT_DC              46
#define TFT_RST             48
#define DISPLAY_WIDTH       240
#define DISPLAY_HEIGHT      240
#define DISPLAY_DIM_TIMEOUT 30000   // ms before auto-dim

// ─── IMU QMI8658 (SPI, onboard — internal PCB routing) ───────────────────────
// Bus SPI interne : MOSI=35, MISO=37, SCLK=36 (not on expansion header)
#define IMU_CS_PIN          34
#define IMU_INT_PIN         33
#define IMU_MOSI            35
#define IMU_MISO            37
#define IMU_SCLK            36
#define IMU_SAMPLE_RATE     50      // Hz
#define STEP_THRESHOLD      1.2f    // g, peak detection threshold
#define STEP_MIN_INTERVAL   300     // ms between valid steps
#define STEP_LPF_ALPHA      0.1f    // low-pass filter coefficient

// ─── Buttons (active LOW, INPUT_PULLUP) ──────────────────────────────────────
#define BTN_PREV            21
#define BTN_NEXT            38
#define BTN_SELECT          39
#define BTN_DEBOUNCE_MS     50
#define BTN_LONG_PRESS_MS   2000

// ─── WiFi Export ─────────────────────────────────────────────────────────────
#define WIFI_SSID           "Draille"
#define WIFI_SERVER_IP      "192.168.4.1"
#define WIFI_TIMEOUT_MS     300000  // 5 minutes

// ─── Storage (SPIFFS) ────────────────────────────────────────────────────────
#define STORAGE_SESSIONS_INDEX  "/sessions.json"
#define STORAGE_TRACKS_DIR      "/tracks"
#define STORAGE_MAX_SESSIONS    20
