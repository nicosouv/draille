#pragma once

// ─── GPS (UART) ───────────────────────────────────────────────────────────────
#define GPS_RX_PIN          34
#define GPS_TX_PIN          12
#define GPS_BAUD            9600
#define GPS_RECORD_INTERVAL 5000    // ms between track points

// ─── Display GC9A01 (SPI) ────────────────────────────────────────────────────
#define TFT_MOSI            11
#define TFT_SCLK            12
#define TFT_CS              10
#define TFT_DC               9
#define TFT_RST              8
#define DISPLAY_WIDTH       240
#define DISPLAY_HEIGHT      240
#define DISPLAY_DIM_TIMEOUT 30000   // ms before auto-dim

// ─── IMU QMI8658 (I2C) ───────────────────────────────────────────────────────
#define IMU_SDA             21
#define IMU_SCL             22
#define IMU_SAMPLE_RATE     50      // Hz
#define STEP_THRESHOLD      1.2f    // g, peak detection threshold
#define STEP_MIN_INTERVAL   300     // ms between valid steps
#define STEP_LPF_ALPHA      0.1f    // low-pass filter coefficient

// ─── Buttons (active LOW, INPUT_PULLUP) ──────────────────────────────────────
#define BTN_PREV            4
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
