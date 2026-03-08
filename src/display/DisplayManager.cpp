#include "DisplayManager.h"
#include <Arduino.h>

// RGB565 color palette
#define COLOR_BG        TFT_BLACK
#define COLOR_PRIMARY   TFT_WHITE
#define COLOR_SECONDARY 0x7BEF   // mid-grey
#define COLOR_ACCENT    TFT_CYAN
#define COLOR_REC       TFT_RED
#define COLOR_OK        TFT_GREEN
#define COLOR_DIM       0x2104   // very dark grey

#define CX              120      // display center X
#define CY              120      // display center Y

// ─── Init ─────────────────────────────────────────────────────────────────────

void DisplayManager::begin() {
    _tft.init();
    _tft.setRotation(0);
    _tft.fillScreen(COLOR_BG);
    _lastActivityMs = millis();
    drawBoot();

#ifdef DEBUG
    Serial.println("[DISPLAY] GC9A01 initialized");
#endif
}

void DisplayManager::update() {
    if (!_dimmed && millis() - _lastActivityMs > DISPLAY_DIM_TIMEOUT) {
        _dim();
    }
}

void DisplayManager::wake() {
    _dimmed         = false;
    _lastActivityMs = millis();
}

bool DisplayManager::isDimmed() const {
    return _dimmed;
}

// ─── Screens ──────────────────────────────────────────────────────────────────

void DisplayManager::drawBoot() {
    _clear();
    _drawCentered("DRAILLE", 95, 4, COLOR_ACCENT);
    _drawCentered("booting...", 135, 2, COLOR_SECONDARY);
}

void DisplayManager::drawDashboard(const DashboardData& d) {
    _clear();
    wake();

    // Time
    _drawCentered(d.timeStr, 52, 4, COLOR_PRIMARY);

    // GPS status dot
    uint16_t dotColor = d.gpsFixed ? COLOR_OK : COLOR_REC;
    _tft.fillCircle(CX, 88, 4, dotColor);

    // Coordinates
    if (d.gpsFixed) {
        char latStr[16], lonStr[16];
        snprintf(latStr, sizeof(latStr), "%.5f", d.lat);
        snprintf(lonStr, sizeof(lonStr), "%.5f", d.lon);
        _drawCentered(latStr, 100, 2, COLOR_PRIMARY);
        _drawCentered(lonStr, 120, 2, COLOR_SECONDARY);
    } else {
        _drawCentered("no fix", 110, 2, COLOR_SECONDARY);
    }

    // Satellites
    char satStr[16];
    snprintf(satStr, sizeof(satStr), "%u sats", d.satellites);
    _drawCentered(satStr, 148, 2, COLOR_SECONDARY);

    // Steps
    char stepStr[16];
    snprintf(stepStr, sizeof(stepStr), "%lu steps", (unsigned long)d.steps);
    _drawCentered(stepStr, 168, 2, COLOR_ACCENT);
}

void DisplayManager::drawTracking(const TrackingData& d) {
    _clear();
    wake();

    // REC indicator
    if (d.isRecording) {
        _tft.fillCircle(55, 55, 6, COLOR_REC);
        _drawCentered("REC", 48, 2, COLOR_REC);
    }

    // Duration
    char dur[9];
    _formatDuration(d.durationSec, dur);
    _drawCentered(dur, 48, 4, COLOR_PRIMARY);

    // Distance
    char dist[16];
    snprintf(dist, sizeof(dist), "%.2f km", d.distanceKm);
    _drawCentered(dist, 95, 4, COLOR_PRIMARY);

    // Speed
    char spd[16];
    snprintf(spd, sizeof(spd), "%.1f km/h", d.speedKmh);
    _drawCentered(spd, 135, 2, COLOR_SECONDARY);

    // Elevation gain
    char elev[16];
    snprintf(elev, sizeof(elev), "+%.0f m", d.elevationGain);
    _drawCentered(elev, 158, 2, COLOR_ACCENT);

    _drawFooter(d.isRecording ? "STOP" : "START");
}

void DisplayManager::drawStats(const StatsData& d) {
    _clear();
    wake();

    _drawTitle("Session Stats");

    char buf[24];

    snprintf(buf, sizeof(buf), "Steps : %lu", (unsigned long)d.steps);
    _drawCentered(buf, 90, 2, COLOR_PRIMARY);

    snprintf(buf, sizeof(buf), "Dist  : %.2f km", d.distanceKm);
    _drawCentered(buf, 112, 2, COLOR_PRIMARY);

    char dur[9];
    _formatDuration(d.durationSec, dur);
    snprintf(buf, sizeof(buf), "Time  : %s", dur);
    _drawCentered(buf, 134, 2, COLOR_PRIMARY);

    snprintf(buf, sizeof(buf), "Avg   : %.1f km/h", d.avgSpeedKmh);
    _drawCentered(buf, 156, 2, COLOR_SECONDARY);
}

void DisplayManager::drawTracks(const Session* sessions, uint8_t count, uint8_t selected) {
    _clear();
    wake();

    _drawTitle("Saved Tracks");

    if (count == 0) {
        _drawCentered("no tracks", 120, 2, COLOR_SECONDARY);
        return;
    }

    // Show up to 3 sessions centered around the selected one
    int startY = 88;
    int lineH  = 22;

    for (int i = -1; i <= 1; i++) {
        int idx = (int)selected + i;
        if (idx < 0 || idx >= (int)count) continue;

        char label[24];
        time_t ts = (time_t)sessions[idx].startTime;
        struct tm* t = gmtime(&ts);
        strftime(label, sizeof(label), "%Y-%m-%d", t);

        uint16_t color = (i == 0) ? COLOR_PRIMARY : COLOR_SECONDARY;
        int y = startY + (i + 1) * lineH;

        if (i == 0) {
            // Selected: draw arrow
            _tft.setTextDatum(MR_DATUM);
            _tft.setTextColor(COLOR_ACCENT, COLOR_BG);
            _tft.drawString(">", CX - 32, y, 2);
        }
        _drawCentered(label, y, 2, color);
    }

    _drawFooter("VIEW");
}

void DisplayManager::drawWifiExport() {
    _clear();
    wake();

    _drawTitle("WiFi Export");

    _drawCentered("SSID: Draille", 100, 2, COLOR_PRIMARY);
    _drawCentered(WIFI_SERVER_IP, 122, 2, COLOR_ACCENT);
    _drawCentered("Waiting...", 152, 2, COLOR_SECONDARY);

    _drawFooter("EXIT");
}

// ─── Helpers ──────────────────────────────────────────────────────────────────

void DisplayManager::_clear() {
    _tft.fillScreen(COLOR_BG);
}

void DisplayManager::_dim() {
    _dimmed = true;
    _tft.fillScreen(COLOR_DIM);

#ifdef DEBUG
    Serial.println("[DISPLAY] Dimmed");
#endif
}

void DisplayManager::_drawCentered(const char* text, int y, uint8_t font, uint16_t color) {
    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(color, COLOR_BG);
    _tft.drawString(text, CX, y, font);
}

void DisplayManager::_drawTitle(const char* title) {
    _drawCentered(title, 52, 2, COLOR_ACCENT);
    _tft.drawFastHLine(40, 68, 160, COLOR_SECONDARY);
}

void DisplayManager::_drawFooter(const char* label) {
    char buf[24];
    snprintf(buf, sizeof(buf), "[SEL: %s]", label);
    _drawCentered(buf, 194, 2, COLOR_SECONDARY);
}

void DisplayManager::_formatDuration(uint32_t sec, char* buf) {
    uint32_t h = sec / 3600;
    uint32_t m = (sec % 3600) / 60;
    uint32_t s = sec % 60;
    if (h > 0) {
        snprintf(buf, 9, "%02lu:%02lu:%02lu", (unsigned long)h, (unsigned long)m, (unsigned long)s);
    } else {
        snprintf(buf, 9, "%02lu:%02lu", (unsigned long)m, (unsigned long)s);
    }
}
