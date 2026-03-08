#pragma once

#include <TFT_eSPI.h>
#include "../config.h"
#include "../types.h"

// ─── Data structs passed by UIManager ────────────────────────────────────────

struct DashboardData {
    char     timeStr[9];    // "HH:MM:SS"
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

// ─── DisplayManager ───────────────────────────────────────────────────────────

class DisplayManager {
public:
    void begin();
    void update();
    void wake();

    void drawBoot(uint8_t step, uint8_t total, const char* label);
    void drawDashboard(const DashboardData& d);
    void drawTracking(const TrackingData& d);
    void drawStats(const StatsData& d);
    void drawTracks(const Session* sessions, uint8_t count, uint8_t selected);
    void drawWifiExport();

    bool isDimmed() const;

private:
    TFT_eSPI    _tft;
    TFT_eSprite _spr  = TFT_eSprite(&_tft);

    bool     _dimmed         = false;
    bool     _fontsLoaded    = false;
    uint32_t _lastActivityMs = 0;

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
