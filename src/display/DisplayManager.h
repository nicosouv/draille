#pragma once

#include <TFT_eSPI.h>
#include "../config.h"
#include "../types.h"

// ─── Data structs passed by UIManager ────────────────────────────────────────

struct DashboardData {
    char     timeStr[6];    // "HH:MM"
    double   lat;
    double   lon;
    uint8_t  satellites;    // 0-12
    uint32_t steps;
    bool     gpsFixed;
};

struct TrackingData {
    uint32_t durationSec;
    float    distanceKm;
    float    speedKmh;
    float    elevationGain;
    bool     isRecording;
    bool     pointJustSaved;  // true for 500ms after each track point is written
};

struct StatsData {
    uint32_t steps;
    float    distanceKm;
    uint32_t durationSec;
    float    avgSpeedKmh;
};

struct WifiExportData {
    uint32_t elapsedMs;   // ms since WiFi export started
};

// ─── DisplayManager ───────────────────────────────────────────────────────────

class DisplayManager {
public:
    void begin();
    void update();
    void wake();

    void setCurrentScreen(AppScreen s);

    void drawBoot(uint8_t step, uint8_t total, const char* label);
    void setDebugData(bool pmuOk, uint32_t gpsBytes, uint8_t sats, bool gpsValid,
                      uint16_t nmeaOk, uint16_t nmeaFail);
    void drawDebugScreen();
    void drawDashboard(const DashboardData& d);
    void drawTracking(const TrackingData& d);
    void drawStats(const StatsData& d);
    void drawTracks(const Session* sessions, uint8_t count, uint8_t selected);
    void drawWifiExport(const WifiExportData& d);

    bool isDimmed() const;

private:
    TFT_eSPI    _tft;
    TFT_eSprite _spr  = TFT_eSprite(&_tft);

    bool      _dimmed         = false;
    bool      _fontsLoaded    = false;
    bool      _dbgPmuOk       = false;
    uint32_t  _dbgGpsBytes    = 0;
    uint8_t   _dbgSats        = 0;
    bool      _dbgValid       = false;
    uint16_t  _dbgNmeaOk      = 0;
    uint16_t  _dbgNmeaFail    = 0;
    uint32_t  _lastActivityMs = 0;
    AppScreen _currentScreen  = AppScreen::DASHBOARD;

    // ── Rendering core ───────────────────────────────────────────────────────
    void _push();
    void _clear();
    void _dim();

    // ── Font helpers ─────────────────────────────────────────────────────────
    void _loadFonts();
    void _setFontL();   // large  ~48px  — main metric
    void _setFontM();   // medium ~26px  — secondary value
    void _setFontS();   // small  ~16px  — labels / captions

    void _text(const char* t, int x, int y, uint16_t color, uint8_t datum = MC_DATUM);

    // ── Primitive helpers ─────────────────────────────────────────────────────
    void _progressArc(int x, int y, int rOut, int rIn,
                      float progress, uint16_t color, uint16_t bgColor);
    void _divider(int y, int halfW = 50);
    void _drawNavDots();

    // ── Icons (programmatic) ──────────────────────────────────────────────────
    void _iconGps   (int cx, int cy, int r, uint16_t color);
    void _iconSteps (int cx, int cy, int size, uint16_t color);
    void _iconWifi  (int cx, int cy, int size, uint16_t color);
    void _iconRec   (int cx, int cy, bool active);
    void _iconArrow (int cx, int cy, bool up, uint16_t color);

    // ── Satellite arc ring ────────────────────────────────────────────────────
    void _satRing(uint8_t satellites);

    // ── Formatting ────────────────────────────────────────────────────────────
    void _fmtDuration(uint32_t sec, char* buf, size_t len);
};
