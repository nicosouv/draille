// TFT_eSPI configuration for Draille / LILYGO T-Beam Supreme (ESP32-S3)
// This file overrides the library default User_Setup.h

#define USER_SETUP_LOADED

// ── Driver ───────────────────────────────────────────────────────────────────
// Use GC9A01_DRIVER for real hardware; ILI9341_DRIVER for Wokwi simulation.
// Comment/uncomment as needed:
//#define GC9A01_DRIVER
#define ILI9341_DRIVER

// ── Resolution ───────────────────────────────────────────────────────────────
#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// ── Pins (ESP32-S3 GPIO) ──────────────────────────────────────────────────────
#define TFT_MOSI  11
#define TFT_SCLK  13   // NOTE: was 12 in config.h — 12 conflicts with GPS_TX_PIN
#define TFT_CS    10
#define TFT_DC     9
#define TFT_RST    8

// ── SPI speed ────────────────────────────────────────────────────────────────
#define SPI_FREQUENCY       27000000
#define SPI_READ_FREQUENCY   8000000

// ── Fonts ────────────────────────────────────────────────────────────────────
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT
