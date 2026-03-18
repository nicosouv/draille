#include "UIManager.h"

// Screens in circular navigation order (excluding WIFI_EXPORT, accessed via long press)
static const AppScreen NAV_ORDER[] = {
    AppScreen::DASHBOARD,
    AppScreen::TRACKING,
    AppScreen::STATS,
    AppScreen::TRACKS
};
static const uint8_t NAV_COUNT = 4;

// ─── Init ─────────────────────────────────────────────────────────────────────

UIManager::UIManager(GpsManager& gps, ImuManager& imu, DisplayManager& display,
                     TrackStorage& storage, WifiExport& wifi)
    : _gps(gps), _imu(imu), _display(display), _storage(storage), _wifi(wifi) {}

void UIManager::begin() {
    pinMode(BTN_PREV,   INPUT_PULLUP);
    pinMode(BTN_NEXT,   INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);

    _screen = AppScreen::DASHBOARD;
    _render();

#ifdef DEBUG
    Serial.println("[UI] UIManager started");
#endif
}

// ─── Main loop ────────────────────────────────────────────────────────────────

void UIManager::update() {
    _readButton(_btnPrev,   BTN_PREV);
    _readButton(_btnNext,   BTN_NEXT);
    _readButton(_btnSelect, BTN_SELECT);

    _handleInput();
    _wireTracking();
    _periodicRender();
}

void UIManager::_periodicRender() {
    // Auto-return to DASHBOARD after stop tracking
    if (_autoReturnMs > 0 && millis() >= _autoReturnMs) {
        _autoReturnMs = 0;
        _screen = AppScreen::DASHBOARD;
        _render();
        return;
    }

    // Fast animation (100ms): comet spinner while acquiring, tracking pulse, WiFi
    bool needsFast = (_screen == AppScreen::WIFI_EXPORT)
                  || (_screen == AppScreen::DASHBOARD && !_gps.isFixed())
                  || (_screen == AppScreen::TRACKING  &&  _gps.isTracking());

    // Slow refresh (1s): dashboard clock update when GPS is fixed
    bool needsSlow = (_screen == AppScreen::DASHBOARD && _gps.isFixed());

    uint32_t now = millis();
    uint32_t interval = needsFast ? 100 : (needsSlow ? 1000 : 0);

    if (interval > 0 && now - _lastRenderMs >= interval) {
        _lastRenderMs = now;
        _render();
    }
}

// ─── Button reading ───────────────────────────────────────────────────────────

void UIManager::_readButton(BtnState& btn, int pin) {
    bool raw = digitalRead(pin);
    uint32_t now = millis();

    if (raw == btn.lastRaw) return;

    if (now - btn.changedAt < BTN_DEBOUNCE_MS) return;

    btn.lastRaw   = raw;
    btn.changedAt = now;

    if (raw == LOW) {
        btn.isDown     = true;
        btn.pressedAt  = now;
        btn.firedShort = false;
        btn.firedLong  = false;
    } else {
        btn.isDown = false;
    }
}

bool UIManager::_justPressed(const BtnState& btn) const {
    return !btn.isDown && !btn.firedShort && !btn.firedLong
           && btn.changedAt > 0
           && (millis() - btn.pressedAt) < BTN_LONG_PRESS_MS;
}

bool UIManager::_justLongPressed(BtnState& btn) {
    if (!btn.isDown || btn.firedLong) return false;
    if (millis() - btn.pressedAt >= BTN_LONG_PRESS_MS) {
        btn.firedLong = true;
        return true;
    }
    return false;
}

// ─── Input handling ───────────────────────────────────────────────────────────

void UIManager::_handleInput() {
    if (_display.isDimmed()) {
        if (_btnPrev.isDown || _btnNext.isDown || _btnSelect.isDown) {
            _display.wake();
            _render();
        }
        return;
    }

    if (_justLongPressed(_btnSelect)) {
        _onLongSelect();
        return;
    }

    if (_justPressed(_btnPrev)) {
        _btnPrev.firedShort = true;
        _onPrev();
    }

    if (_justPressed(_btnNext)) {
        _btnNext.firedShort = true;
        _onNext();
    }

    if (_justPressed(_btnSelect)) {
        _btnSelect.firedShort = true;
        _onSelect();
    }
}

void UIManager::_onPrev() {
    if (_screen == AppScreen::WIFI_EXPORT) return;

    // On TRACKS screen: scroll the list backwards
    if (_screen == AppScreen::TRACKS) {
        uint8_t count = _storage.getSessionCount();
        if (count > 0) {
            _trackSelected = (_trackSelected + count - 1) % count;
            _render();
        }
        return;
    }

    for (uint8_t i = 0; i < NAV_COUNT; i++) {
        if (NAV_ORDER[i] == _screen) {
            _screen = NAV_ORDER[(i + NAV_COUNT - 1) % NAV_COUNT];
            break;
        }
    }

    if (_screen == AppScreen::TRACKS) _trackSelected = 0;

    _render();

#ifdef DEBUG
    Serial.println("[UI] PREV");
#endif
}

void UIManager::_onNext() {
    if (_screen == AppScreen::WIFI_EXPORT) return;

    // On TRACKS screen: scroll the list forwards
    if (_screen == AppScreen::TRACKS) {
        uint8_t count = _storage.getSessionCount();
        if (count > 0) {
            _trackSelected = (_trackSelected + 1) % count;
            _render();
        }
        return;
    }

    for (uint8_t i = 0; i < NAV_COUNT; i++) {
        if (NAV_ORDER[i] == _screen) {
            _screen = NAV_ORDER[(i + 1) % NAV_COUNT];
            break;
        }
    }

    if (_screen == AppScreen::TRACKS) _trackSelected = 0;

    _render();

#ifdef DEBUG
    Serial.println("[UI] NEXT");
#endif
}

void UIManager::_onSelect() {
    switch (_screen) {
        case AppScreen::TRACKING:
            if (_gps.isTracking()) {
                _stopTracking();
            } else {
                _startTracking();
            }
            _render();
            break;

        case AppScreen::TRACKS: {
            // SELECT on TRACKS is now a no-op (PREV/NEXT scroll)
            break;
        }

        case AppScreen::WIFI_EXPORT:
            _wifi.stopExport();
            _screen = AppScreen::DASHBOARD;
            _render();
            break;

        default:
            break;
    }

#ifdef DEBUG
    Serial.println("[UI] SELECT");
#endif
}

void UIManager::_onLongSelect() {
    if (_screen == AppScreen::WIFI_EXPORT) {
        _wifi.stopExport();
        _screen = AppScreen::DASHBOARD;
    } else {
        _screen = AppScreen::WIFI_EXPORT;
        _wifiStartMs = millis();
        _wifi.startExport();
    }
    _render();

#ifdef DEBUG
    Serial.println("[UI] LONG SELECT");
#endif
}

// ─── Tracking control ─────────────────────────────────────────────────────────

void UIManager::_startTracking() {
    _imu.resetStepCount();
    _gps.startTracking();
    _storage.openSession(_gps.getTimestamp());
    _sessionStartMs = millis();

#ifdef DEBUG
    Serial.println("[UI] Tracking started");
#endif
}

void UIManager::_stopTracking() {
    _gps.stopTracking();
    _storage.closeSession(_gps.getTimestamp(), _imu.getStepCount());

    // Auto-return to DASHBOARD after 4 seconds on STATS
    _screen       = AppScreen::STATS;
    _autoReturnMs = millis() + 4000;

#ifdef DEBUG
    Serial.println("[UI] Tracking stopped");
#endif
}

void UIManager::_wireTracking() {
    if (!_gps.isTracking()) return;

    TrackPoint point;
    if (_gps.popTrackPoint(point)) {
        _storage.appendPoint(point);
        _lastPointMs = millis();

        if (_screen == AppScreen::TRACKING) {
            _render();
        }
    }
}

// ─── Rendering ────────────────────────────────────────────────────────────────

void UIManager::_render() {
    _display.setCurrentScreen(_screen);

    switch (_screen) {
        case AppScreen::DASHBOARD: {
            DashboardData d;
            _buildDashboardData(d);
            _display.drawDashboard(d);
            break;
        }
        case AppScreen::TRACKING: {
            TrackingData d;
            _buildTrackingData(d);
            _display.drawTracking(d);
            break;
        }
        case AppScreen::STATS: {
            StatsData d;
            _buildStatsData(d);
            _display.drawStats(d);
            break;
        }
        case AppScreen::TRACKS: {
            uint8_t count = _storage.getSessionCount();
            Session sessions[STORAGE_MAX_SESSIONS];
            for (uint8_t i = 0; i < count; i++) _storage.getSession(i, sessions[i]);
            _display.drawTracks(sessions, count, _trackSelected);
            break;
        }
        case AppScreen::WIFI_EXPORT: {
            WifiExportData d;
            d.elapsedMs = _wifiStartMs > 0 ? (millis() - _wifiStartMs) : 0;
            _display.drawWifiExport(d);
            break;
        }
    }
}

// ─── Data builders ────────────────────────────────────────────────────────────

void UIManager::_buildDashboardData(DashboardData& d) const {
    uint32_t ts = _gps.getTimestamp();
    if (ts > 0) {
        time_t t = (time_t)(ts + UTC_OFFSET_SEC);
        struct tm* tm_info = gmtime(&t);
        strftime(d.timeStr, sizeof(d.timeStr), "%H:%M", tm_info);
    } else {
        strncpy(d.timeStr, "--:--", sizeof(d.timeStr));
    }

    d.lat        = _gps.getLat();
    d.lon        = _gps.getLon();
    d.satellites = _gps.getSatellites();
    d.steps      = _imu.getStepCount();
    d.gpsFixed   = _gps.isFixed();
}

void UIManager::_buildTrackingData(TrackingData& d) const {
    d.isRecording    = _gps.isTracking();
    d.durationSec    = _sessionStartMs > 0 ? (millis() - _sessionStartMs) / 1000 : 0;
    d.speedKmh       = _gps.getSpeed();
    d.distanceKm     = 0.0f;
    d.elevationGain  = 0.0f;
    d.pointJustSaved = (millis() - _lastPointMs) < 500;

    Session current;
    if (_storage.getCurrentSession(current)) {
        d.distanceKm    = current.totalDistance;
        d.elevationGain = current.elevationGain;
    }
}

void UIManager::_buildStatsData(StatsData& d) const {
    d.steps       = _imu.getStepCount();
    d.durationSec = _sessionStartMs > 0 ? (millis() - _sessionStartMs) / 1000 : 0;
    d.distanceKm  = 0.0f;

    Session current;
    if (_storage.getCurrentSession(current)) {
        d.distanceKm = current.totalDistance;
    }

    d.avgSpeedKmh = d.durationSec > 0
        ? (d.distanceKm / ((float)d.durationSec / 3600.0f))
        : 0.0f;
}
