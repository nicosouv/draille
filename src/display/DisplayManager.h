#pragma once

#include <TFT_eSPI.h>
#include "../config.h"
#include "../storage/TrackStorage.h"

struct DashboardData {
    char    timeStr[9];   // "HH:MM:SS"
    double  lat;
    double  lon;
    uint8_t satellites;
    uint32_t steps;
    bool    gpsFixed;
};

struct TrackingData {
    uint32_t durationSec;
    float    distanceKm;
    float    speedKmh;
    float    elevationGain;
    bool     isRecording;
};

struct StatsData {
    uint32_t steps;
    float    distanceKm;
    uint32_t durationSec;
    float    avgSpeedKmh;
};

class DisplayManager {
public:
    void begin();
    void update();
    void wake();

    void drawBoot();
    void drawDashboard(const DashboardData& d);
    void drawTracking(const TrackingData& d);
    void drawStats(const StatsData& d);
    void drawTracks(const Session* sessions, uint8_t count, uint8_t selected);
    void drawWifiExport();

    bool isDimmed() const;

private:
    TFT_eSPI _tft;
    bool     _dimmed         = false;
    uint32_t _lastActivityMs = 0;

    void _dim();
    void _clear();
    void _drawCentered(const char* text, int y, uint8_t font, uint16_t color);
    void _drawTitle(const char* title);
    void _drawFooter(const char* label);
    void _formatDuration(uint32_t sec, char* buf);
};
