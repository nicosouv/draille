#pragma once

#include <FS.h>
#include <SPIFFS.h>
#include <Arduino.h>
#include "../config.h"
#include "../gps/GpsManager.h"
#include "../imu/ImuManager.h"
#include "../display/DisplayManager.h"
#include "../storage/TrackStorage.h"
#include "../export/WifiExport.h"

enum class AppScreen {
    DASHBOARD,
    TRACKING,
    STATS,
    TRACKS,
    WIFI_EXPORT
};

class UIManager {
public:
    UIManager(GpsManager& gps, ImuManager& imu, DisplayManager& display,
              TrackStorage& storage, WifiExport& wifi);

    void begin();
    void update();

private:
    GpsManager&     _gps;
    ImuManager&     _imu;
    DisplayManager& _display;
    TrackStorage&   _storage;
    WifiExport&     _wifi;

    AppScreen _screen         = AppScreen::DASHBOARD;
    uint8_t   _trackSelected  = 0;
    uint32_t  _sessionStartMs = 0;
    uint32_t  _lastPointMs    = 0;   // timestamp of last saved track point
    uint32_t  _lastRenderMs   = 0;   // for periodic animated renders

    // Button state
    struct BtnState {
        bool     lastRaw    = HIGH;
        bool     isDown     = false;
        uint32_t pressedAt  = 0;
        uint32_t changedAt  = 0;
        bool     firedShort = false;
        bool     firedLong  = false;
    };

    BtnState _btnPrev;
    BtnState _btnNext;
    BtnState _btnSelect;

    void _readButton(BtnState& btn, int pin);
    bool _justPressed(const BtnState& btn)  const;
    bool _justLongPressed(BtnState& btn);

    void _handleInput();
    void _render();
    void _wireTracking();
    void _periodicRender();

    void _onPrev();
    void _onNext();
    void _onSelect();
    void _onLongSelect();

    void _startTracking();
    void _stopTracking();

    void _buildDashboardData(DashboardData& d) const;
    void _buildTrackingData(TrackingData& d)   const;
    void _buildStatsData(StatsData& d)         const;
};
