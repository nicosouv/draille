#include "DisplayManager.h"
#include <Arduino.h>
#include <cmath>
#include <ctime>

// ─── Color palette (Apple dark mode) ─────────────────────────────────────────
// RGB565 conversions: R5 = R/8, G6 = G/4, B5 = B/8

static constexpr uint16_t C_BG      = 0x0000;   // #000000  background
static constexpr uint16_t C_WHITE   = 0xFFFF;   // #FFFFFF  primary text
static constexpr uint16_t C_BLUE    = 0x043F;   // #0080FF  accent (Apple blue)
static constexpr uint16_t C_GREEN   = 0x368B;   // #30D158  Apple green
static constexpr uint16_t C_RED     = 0xFA07;   // #FF453A  Apple red / REC
static constexpr uint16_t C_AMBER   = 0xFCA0;   // #FF9500  Apple amber
static constexpr uint16_t C_AMBER_D = 0x7240;   // dim amber (comet mid-tail)
static constexpr uint16_t C_GREY_H  = 0x7BEF;   // #787878  secondary text
static constexpr uint16_t C_GREY_L  = 0x18E3;   // #1C1C1E  ring background

// Font file names (Inter, stored in SPIFFS as /Inter_X.vlw)
#define FONT_L_NAME "Inter_Bold_48"
#define FONT_M_NAME "Inter_Medium_24"
#define FONT_S_NAME "Inter_Light_18"

// Display center
static constexpr int CX = 120;
static constexpr int CY = 120;

// ─── Init ─────────────────────────────────────────────────────────────────────

void DisplayManager::begin() {
    _tft.init();
    _tft.setRotation(0);
    _tft.fillScreen(C_BG);

    _spr.setColorDepth(16);
    _spr.createSprite(240, 240);
    _spr.setTextWrap(false);

    _loadFonts();
    _lastActivityMs = millis();

#ifdef DEBUG
    Serial.printf("[DISPLAY] Init — smooth fonts: %s\n", _fontsLoaded ? "yes" : "no (fallback)");
#endif
}

void DisplayManager::update() {
    if (!_dimmed && millis() - _lastActivityMs > DISPLAY_DIM_TIMEOUT) {
        _dim();
    }
}

void DisplayManager::wake() {
    _dimmed = false;
    _lastActivityMs = millis();
}

bool DisplayManager::isDimmed() const { return _dimmed; }

void DisplayManager::setCurrentScreen(AppScreen s) {
    _currentScreen = s;
}

// ─── Screens ──────────────────────────────────────────────────────────────────

void DisplayManager::drawBoot(uint8_t step, uint8_t total, const char* label) {
    _clear();

    // Background ring
    _spr.drawSmoothArc(CX, CY, 116, 112, 0, 360, C_GREY_L, C_BG, true);

    // Progress arc fills clockwise from top
    if (step > 0) {
        float deg    = min((float)step / total, 1.0f) * 360.0f;
        float startA = 270.0f;
        float endA   = startA + deg;
        if (endA >= 360.0f) {
            _spr.drawSmoothArc(CX, CY, 116, 112, startA, 360.0f, C_BLUE, C_BG, false);
            _spr.drawSmoothArc(CX, CY, 116, 112, 0.0f, endA - 360.0f, C_BLUE, C_BG, false);
        } else {
            _spr.drawSmoothArc(CX, CY, 116, 112, startA, endA, C_BLUE, C_BG, false);
        }
    }

    // Step dots
    for (uint8_t i = 0; i < total; i++) {
        float a = (270.0f + (float)i / total * 360.0f) * DEG_TO_RAD;
        int x   = CX + (int)(114 * cosf(a));
        int y   = CY + (int)(114 * sinf(a));
        _spr.fillSmoothCircle(x, y, 3, i < step ? C_BLUE : C_GREY_L, C_BG);
    }

    // Name
    _setFontL();
    _text("DRAILLE", CX, 102, C_WHITE);

    // Current module label
    _setFontS();
    if (step == total) {
        _text("ready", CX, 148, C_GREEN);
    } else {
        char buf[32];
        snprintf(buf, sizeof(buf), "starting %s...", label);
        _text(buf, CX, 148, C_GREY_H);
    }

    _push();
}

void DisplayManager::drawDashboard(const DashboardData& d) {
    _clear();
    wake();

    // ── Satellite ring / comet spinner ────────────────────────────────────────
    if (!d.gpsFixed) {
        // Background ring
        _spr.drawSmoothArc(CX, CY, 116, 112, 0, 360, C_GREY_L, C_BG, true);

        // Comet tail: 3 overlapping arcs from head backwards
        float head = fmod(millis() / 6.0f, 360.0f);

        auto drawWrappedArc = [&](float sa, float ea, uint16_t color) {
            sa = fmod(sa + 720.0f, 360.0f);
            ea = fmod(ea + 720.0f, 360.0f);
            if (ea <= sa) {
                _spr.drawSmoothArc(CX, CY, 116, 112, sa, 360.0f, color, C_BG, false);
                if (ea > 0.1f)
                    _spr.drawSmoothArc(CX, CY, 116, 112, 0.0f, ea, color, C_BG, false);
            } else {
                _spr.drawSmoothArc(CX, CY, 116, 112, sa, ea, color, C_BG, false);
            }
        };

        drawWrappedArc(head - 90.0f, head - 60.0f, C_GREY_L);  // dimmest tail
        drawWrappedArc(head - 60.0f, head - 30.0f, C_AMBER_D); // mid tail
        drawWrappedArc(head - 30.0f, head,         C_AMBER);   // bright head
    } else {
        _satRing(d.satellites);
    }

    // ── GPS status dot ────────────────────────────────────────────────────────
    uint16_t gpsColor = d.gpsFixed
        ? (d.satellites >= 6 ? C_GREEN : (d.satellites >= 4 ? C_AMBER : C_RED))
        : C_RED;
    _spr.fillSmoothCircle(CX + 52, 64, 5, gpsColor, C_BG);

    // ── Time ──────────────────────────────────────────────────────────────────
    _setFontL();
    _text(d.timeStr, CX, 94, C_WHITE);

    // ── Thin divider ──────────────────────────────────────────────────────────
    _divider(114);

    // ── Coordinates ───────────────────────────────────────────────────────────
    if (d.gpsFixed) {
        char latBuf[14], lonBuf[14];
        snprintf(latBuf, sizeof(latBuf), "%.5f  N", fabs(d.lat));
        snprintf(lonBuf, sizeof(lonBuf), "%.5f  E", fabs(d.lon));
        _setFontS();
        _text(latBuf, CX, 130, C_WHITE);
        _text(lonBuf, CX, 148, C_GREY_H);
    } else {
        _setFontS();
        _text("acquiring fix", CX, 139, C_AMBER);
    }

    // ── Steps ─────────────────────────────────────────────────────────────────
    _iconSteps(54, 177, 14, C_BLUE);

    char stepBuf[12];
    if (d.steps < 10000) {
        snprintf(stepBuf, sizeof(stepBuf), "%lu", (unsigned long)d.steps);
    } else {
        snprintf(stepBuf, sizeof(stepBuf), "%.1fk", d.steps / 1000.0f);
    }
    _setFontM();
    _text(stepBuf, CX + 10, 177, C_WHITE);

    _setFontS();
    _text("steps", CX + 10, 194, C_GREY_H);

    _drawNavDots();
    _push();
}

void DisplayManager::drawTracking(const TrackingData& d) {
    _clear();
    wake();

    // ── Outer ring: time progress (2h = full) ─────────────────────────────────
    float timeProgress = min(d.durationSec / 7200.0f, 1.0f);
    _progressArc(CX, CY, 116, 109, timeProgress, C_BLUE, C_GREY_L);

    // ── Inner ring: distance progress (10km = full) ───────────────────────────
    float distProgress = min(d.distanceKm / 10.0f, 1.0f);
    _progressArc(CX, CY, 104, 98, distProgress, C_GREEN, C_GREY_L);

    // ── REC indicator + status ────────────────────────────────────────────────
    if (d.isRecording) {
        bool pulse = (millis() / 800) % 2;
        _iconRec(40, 72, pulse);
        _setFontS();
        _text("REC", 52, 72, pulse ? C_RED : C_GREY_H, ML_DATUM);
    } else {
        _setFontS();
        _text("PAUSED", 40, 72, C_AMBER, ML_DATUM);
    }

    // ── Duration (right side of top bar) ──────────────────────────────────────
    char durBuf[9];
    _fmtDuration(d.durationSec, durBuf, sizeof(durBuf));
    _setFontS();
    _text(durBuf, 196, 72, C_GREY_H, MR_DATUM);

    // ── Distance (main metric) ────────────────────────────────────────────────
    char distBuf[8];
    snprintf(distBuf, sizeof(distBuf), "%.2f", d.distanceKm);
    _setFontL();
    _text(distBuf, CX, 112, C_WHITE);

    _setFontS();
    _text("km", CX, 136, C_GREY_H);

    // ── Speed (secondary row) ─────────────────────────────────────────────────
    char spdBuf[12];
    snprintf(spdBuf, sizeof(spdBuf), "%.1f km/h", d.speedKmh);
    _iconArrow(72, 158, true, C_GREY_H);
    _setFontM();
    _text(spdBuf, CX + 10, 158, C_WHITE);

    // ── Elevation ─────────────────────────────────────────────────────────────
    char elevBuf[10];
    snprintf(elevBuf, sizeof(elevBuf), "+%.0f m", d.elevationGain);
    _iconArrow(68, 180, true, C_GREEN);
    _setFontS();
    _text(elevBuf, CX + 10, 180, C_GREY_H);

    // ── Track point saved flash (green dot, top-right of rings) ───────────────
    if (d.pointJustSaved) {
        _spr.fillSmoothCircle(CX + 68, CY - 68, 5, C_GREEN, C_BG);
    }

    _drawNavDots();
    _push();
}

void DisplayManager::drawStats(const StatsData& d) {
    _clear();
    wake();

    _spr.drawSmoothArc(CX, CY, 116, 114, 0, 360, C_GREY_L, C_BG, true);

    _setFontS();
    _text("SESSION", CX, 58, C_GREY_H);
    _divider(72, 40);

    // Horizontal center divider
    _divider(128, 70);

    // ── Top-left: Steps ───────────────────────────────────────────────────────
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%lu", (unsigned long)d.steps);
        _iconSteps(CX - 45, 78, 10, C_BLUE);
        _setFontM();
        _text(buf, CX - 45, 93, C_WHITE);
        _setFontS();
        _text("steps", CX - 45, 112, C_GREY_H);
    }

    // ── Top-right: Distance ───────────────────────────────────────────────────
    {
        char buf[12];
        snprintf(buf, sizeof(buf), "%.2f", d.distanceKm);
        _iconArrow(CX + 45, 78, true, C_GREEN);
        _setFontM();
        _text(buf, CX + 45, 93, C_WHITE);
        _setFontS();
        _text("km", CX + 45, 112, C_GREY_H);
    }

    // ── Bottom-left: Duration ─────────────────────────────────────────────────
    {
        char durBuf[9];
        _fmtDuration(d.durationSec, durBuf, sizeof(durBuf));
        _spr.fillSmoothCircle(CX - 45, 143, 4, C_AMBER, C_BG);
        _setFontM();
        _text(durBuf, CX - 45, 158, C_WHITE);
        _setFontS();
        _text("duration", CX - 45, 177, C_GREY_H);
    }

    // ── Bottom-right: Avg speed ────────────────────────────────────────────────
    {
        char buf[10];
        snprintf(buf, sizeof(buf), "%.1f", d.avgSpeedKmh);
        _iconArrow(CX + 45, 143, true, C_GREY_H);
        _setFontM();
        _text(buf, CX + 45, 158, C_GREY_H);
        _setFontS();
        _text("avg km/h", CX + 45, 177, C_GREY_H);
    }

    _drawNavDots();
    _push();
}

void DisplayManager::drawTracks(const Session* sessions, uint8_t count, uint8_t selected) {
    _clear();
    wake();

    _spr.drawSmoothArc(CX, CY, 116, 114, 0, 360, C_GREY_L, C_BG, true);

    _setFontS();
    _text("TRACKS", CX, 58, C_GREY_H);
    _divider(72, 40);

    if (count == 0) {
        _setFontS();
        _text("no saved tracks", CX, 120, C_GREY_H);
        _drawNavDots();
        _push();
        return;
    }

    // Show selected + 1 above + 2 below (4 items total)
    int itemH  = 22;
    int startY = 78;

    for (int offset = -1; offset <= 2; offset++) {
        int idx = (int)selected + offset;
        if (idx < 0 || idx >= (int)count) continue;

        int y      = startY + (offset + 1) * itemH;
        bool isSel = (offset == 0);

        char dateBuf[12];
        time_t ts = (time_t)sessions[idx].startTime;
        struct tm* t = gmtime(&ts);
        strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", t);

        if (isSel) {
            _spr.fillRoundRect(30, y - 10, 180, 20, 5, 0x0821);
            _spr.drawSmoothRoundRect(30, y - 10, 5, 4, 180, 20, C_BLUE, C_BG);
            _spr.setTextDatum(ML_DATUM);
            _spr.setTextColor(C_WHITE, 0x0821);
            _spr.drawString(dateBuf, 44, y, 2);
            char distBuf[10];
            snprintf(distBuf, sizeof(distBuf), "%.1fkm", sessions[idx].totalDistance);
            _spr.setTextDatum(MR_DATUM);
            _spr.drawString(distBuf, 196, y, 2);
        } else {
            _spr.setTextDatum(ML_DATUM);
            _spr.setTextColor(C_GREY_H, C_BG);
            _spr.drawString(dateBuf, 44, y, 2);
            char distBuf[10];
            snprintf(distBuf, sizeof(distBuf), "%.1fkm", sessions[idx].totalDistance);
            _spr.setTextDatum(MR_DATUM);
            _spr.drawString(distBuf, 196, y, 2);
        }
    }

    // ── Detail strip for selected session ─────────────────────────────────────
    {
        uint32_t dur = (sessions[selected].endTime > sessions[selected].startTime)
                       ? (sessions[selected].endTime - sessions[selected].startTime)
                       : 0;
        char durBuf[9];
        _fmtDuration(dur, durBuf, sizeof(durBuf));

        char detailBuf[40];
        snprintf(detailBuf, sizeof(detailBuf), "%lu steps  %.1fkm  %s",
                 (unsigned long)sessions[selected].stepCount,
                 sessions[selected].totalDistance,
                 durBuf);
        _setFontS();
        _text(detailBuf, CX, 188, C_GREY_H);
    }

    // ── X/Y counter ───────────────────────────────────────────────────────────
    {
        char cntBuf[8];
        snprintf(cntBuf, sizeof(cntBuf), "%u / %u", (unsigned)(selected + 1), (unsigned)count);
        _setFontS();
        _text(cntBuf, CX, 210, C_GREY_H);
    }

    _drawNavDots();
    _push();
}

void DisplayManager::drawWifiExport(const WifiExportData& d) {
    _clear();
    wake();

    _spr.drawSmoothArc(CX, CY, 116, 114, 0, 360, C_GREY_L, C_BG, true);

    // ── WiFi icon (arcs light up one by one) ──────────────────────────────────
    {
        uint8_t phase = (millis() / 400) % 5;
        int thick = 3;
        int base  = 90 + 40 / 2;

        uint16_t c0 = (phase >= 1) ? C_BLUE : C_GREY_L;
        uint16_t c1 = (phase >= 2) ? C_BLUE : C_GREY_L;
        uint16_t c2 = (phase >= 3) ? C_BLUE : C_GREY_L;

        int s = 40;
        _spr.drawSmoothArc(CX, base, s / 3,       s / 3 - thick, 225, 315, c0, C_BG, true);
        _spr.drawSmoothArc(CX, base, s * 2 / 3,   s * 2/3-thick, 225, 315, c1, C_BG, true);
        _spr.drawSmoothArc(CX, base, s,            s - thick,     225, 315, c2, C_BG, true);
        _spr.fillSmoothCircle(CX, base, thick, phase >= 1 ? C_BLUE : C_GREY_L, C_BG);
    }

    // ── SSID ──────────────────────────────────────────────────────────────────
    _setFontM();
    _text(WIFI_SSID, CX, 138, C_WHITE);

    // ── IP ────────────────────────────────────────────────────────────────────
    _setFontS();
    _text(WIFI_SERVER_IP, CX, 158, C_BLUE);

    // ── Status ────────────────────────────────────────────────────────────────
    bool pulse = (millis() / 1000) % 2;
    _spr.fillSmoothCircle(CX - 38, 178, 3, pulse ? C_GREEN : C_GREY_L, C_BG);
    _setFontS();
    _text("waiting", CX + 4, 178, C_GREY_H);

    // ── Countdown ─────────────────────────────────────────────────────────────
    uint32_t remaining = (d.elapsedMs < 300000) ? (300000 - d.elapsedMs) : 0;
    uint32_t m = remaining / 60000;
    uint32_t s = (remaining % 60000) / 1000;
    char countBuf[18];
    snprintf(countBuf, sizeof(countBuf), "auto-off %02lu:%02lu", (unsigned long)m, (unsigned long)s);
    _setFontS();
    _text(countBuf, CX, 196, C_GREY_H);

    _push();
}

void DisplayManager::setDebugData(bool pmuOk, uint32_t gpsBytes, uint8_t sats, bool gpsValid,
                                   uint16_t nmeaOk, uint16_t nmeaFail) {
    _dbgPmuOk    = pmuOk;
    _dbgGpsBytes = gpsBytes;
    _dbgSats     = sats;
    _dbgValid    = gpsValid;
    _dbgNmeaOk   = nmeaOk;
    _dbgNmeaFail = nmeaFail;
}

// ─── Rendering core ───────────────────────────────────────────────────────────

void DisplayManager::_push() {
    _spr.pushSprite(0, 0);
}

void DisplayManager::drawDebugScreen() {
    _clear();
    _spr.unloadFont();
    _spr.setTextDatum(MC_DATUM);

    _spr.setTextColor(C_BLUE, C_BG);
    _spr.drawString("-- DEBUG --", CX, 40, 2);

    char buf[40];
    uint16_t y = 75;
    const uint16_t step = 22;

    _spr.setTextColor(_dbgPmuOk ? C_GREEN : C_RED, C_BG);
    snprintf(buf, sizeof(buf), "PMU: %s", _dbgPmuOk ? "OK" : "FAIL");
    _spr.drawString(buf, CX, y, 2); y += step;

    _spr.setTextColor(C_WHITE, C_BG);
    snprintf(buf, sizeof(buf), "bytes: %lu", (unsigned long)_dbgGpsBytes);
    _spr.drawString(buf, CX, y, 2); y += step;

    snprintf(buf, sizeof(buf), "nmea ok:%u fail:%u", _dbgNmeaOk, _dbgNmeaFail);
    _spr.drawString(buf, CX, y, 2); y += step;

    _spr.setTextColor(_dbgSats > 0 ? C_GREEN : C_AMBER, C_BG);
    snprintf(buf, sizeof(buf), "sats: %u", _dbgSats);
    _spr.drawString(buf, CX, y, 2); y += step;

    _spr.setTextColor(_dbgValid ? C_GREEN : C_RED, C_BG);
    snprintf(buf, sizeof(buf), "fix: %s", _dbgValid ? "YES" : "no");
    _spr.drawString(buf, CX, y, 2);

    _push();
}

void DisplayManager::_clear() {
    _spr.fillScreen(C_BG);
}

void DisplayManager::_dim() {
    _dimmed = true;
    _tft.fillScreen(0x0000);
#ifdef DEBUG
    Serial.println("[DISPLAY] Dimmed");
#endif
}

// ─── Font helpers ─────────────────────────────────────────────────────────────

void DisplayManager::_loadFonts() {
    // Smooth font loading disabled — using built-in fonts only for now
    _fontsLoaded = false;
}

void DisplayManager::_setFontL() {
    _spr.setTextFont(2);
    _spr.setTextSize(3);   // 16 * 3 = ~48px
}

void DisplayManager::_setFontM() {
    _spr.setTextFont(2);
    _spr.setTextSize(2);   // 16 * 2 = ~32px
}

void DisplayManager::_setFontS() {
    _spr.setTextFont(2);
    _spr.setTextSize(1);   // 16px
}

void DisplayManager::_text(const char* t, int x, int y, uint16_t color, uint8_t datum) {
    _spr.setTextDatum(datum);
    _spr.setTextColor(color, C_BG);
    _spr.drawString(t, x, y, 2);
}

// ─── Primitive helpers ────────────────────────────────────────────────────────

void DisplayManager::_progressArc(int x, int y, int rOut, int rIn,
                                   float progress, uint16_t color, uint16_t bgColor) {
    // Background ring
    _spr.drawSmoothArc(x, y, rOut, rIn, 0, 360, bgColor, C_BG, true);

    if (progress < 0.005f) return;

    float deg = min(progress, 1.0f) * 360.0f;
    float startA = 270.0f;
    float endA   = startA + deg;

    if (endA >= 360.0f) {
        _spr.drawSmoothArc(x, y, rOut, rIn, startA, 360.0f, color, C_BG, false);
        _spr.drawSmoothArc(x, y, rOut, rIn, 0.0f, endA - 360.0f, color, C_BG, false);
    } else {
        _spr.drawSmoothArc(x, y, rOut, rIn, startA, endA, color, C_BG, false);
    }
}

void DisplayManager::_divider(int y, int halfW) {
    _spr.drawFastHLine(CX - halfW, y, halfW * 2, C_GREY_L);
}

void DisplayManager::_drawNavDots() {
    static const AppScreen order[4] = {
        AppScreen::DASHBOARD, AppScreen::TRACKING,
        AppScreen::STATS,     AppScreen::TRACKS
    };
    int xs[4] = { CX - 15, CX - 5, CX + 5, CX + 15 };
    for (int i = 0; i < 4; i++) {
        uint16_t c = (order[i] == _currentScreen) ? C_WHITE : C_GREY_L;
        _spr.fillSmoothCircle(xs[i], 215, 3, c, C_BG);
    }
}

// ─── Satellite ring ───────────────────────────────────────────────────────────

void DisplayManager::_satRing(uint8_t satellites) {
    _spr.drawSmoothArc(CX, CY, 116, 112, 0, 360, C_GREY_L, C_BG, true);

    uint8_t n = min((uint8_t)12, satellites);
    if (n == 0) return;

    float segW   = 25.0f;
    float step   = 30.0f;
    float origin = 270.0f;

    for (int i = 0; i < n; i++) {
        float sa = fmod(origin + i * step, 360.0f);
        float ea = fmod(sa + segW, 360.0f);

        uint16_t c = (satellites >= 6) ? C_GREEN
                   : (satellites >= 4) ? C_AMBER
                   :                     C_RED;

        if (ea > sa) {
            _spr.drawSmoothArc(CX, CY, 116, 112, sa, ea, c, C_BG, false);
        } else {
            _spr.drawSmoothArc(CX, CY, 116, 112, sa, 360.0f, c, C_BG, false);
            _spr.drawSmoothArc(CX, CY, 116, 112, 0.0f, ea,   c, C_BG, false);
        }
    }
}

// ─── Icons ────────────────────────────────────────────────────────────────────

void DisplayManager::_iconGps(int cx, int cy, int r, uint16_t color) {
    _spr.fillSmoothCircle(cx, cy - r / 3, r / 2, color, C_BG);
    for (int dy = 0; dy <= r; dy++) {
        float t = (float)dy / r;
        int hw  = (int)((r / 2) * (1.0f - t));
        if (hw > 0) _spr.drawFastHLine(cx - hw, cy - r / 3 + r / 2 + dy, hw * 2, color);
    }
    _spr.fillSmoothCircle(cx, cy - r / 3, r / 4, C_BG, color);
}

void DisplayManager::_iconSteps(int cx, int cy, int size, uint16_t color) {
    int s = size / 3;
    _spr.fillSmoothCircle(cx - s,     cy - s / 2, s,     color, C_BG);
    _spr.fillSmoothCircle(cx + s + 1, cy + s / 2, s - 1, color, C_BG);
}

void DisplayManager::_iconWifi(int cx, int cy, int size, uint16_t color) {
    int thick = 3;
    for (int i = 0; i < 3; i++) {
        int r = size - i * (size / 3);
        if (r < 4) break;
        _spr.drawSmoothArc(cx, cy + size / 2, r, r - thick, 225, 315, color, C_BG, true);
    }
    _spr.fillSmoothCircle(cx, cy + size / 2, thick, color, C_BG);
}

void DisplayManager::_iconRec(int cx, int cy, bool active) {
    uint16_t c = active ? C_RED : C_GREY_H;
    _spr.fillSmoothCircle(cx, cy, 5, c, C_BG);
}

void DisplayManager::_iconArrow(int cx, int cy, bool up, uint16_t color) {
    int dir = up ? -1 : 1;
    _spr.drawFastVLine(cx, cy - 5 * dir, 10, color);
    for (int i = 0; i < 4; i++) {
        _spr.drawFastHLine(cx - i, cy - (5 - i) * dir, 2 * i + 1, color);
    }
}

// ─── Formatting ───────────────────────────────────────────────────────────────

void DisplayManager::_fmtDuration(uint32_t sec, char* buf, size_t len) {
    uint32_t h = sec / 3600;
    uint32_t m = (sec % 3600) / 60;
    uint32_t s = sec % 60;
    if (h > 0) {
        snprintf(buf, len, "%02lu:%02lu:%02lu", (unsigned long)h, (unsigned long)m, (unsigned long)s);
    } else {
        snprintf(buf, len, "%02lu:%02lu", (unsigned long)m, (unsigned long)s);
    }
}
