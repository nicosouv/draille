#include "GpsManager.h"
#include <Arduino.h>

void GpsManager::begin() {
    _serial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

#ifdef DEBUG
    Serial.println("[GPS] UART initialized");
#endif
}

void GpsManager::update() {
    while (_serial.available()) {
        _gps.encode(_serial.read());
    }

    if (_tracking && isFixed()) {
        uint32_t now = millis();
        if (now - _lastRecordMs >= GPS_RECORD_INTERVAL) {
            _lastRecordMs = now;
            _recordPoint();
        }
    }
}

void GpsManager::startTracking() {
    _tracking     = true;
    _lastRecordMs = millis();
    _pointReady   = false;

#ifdef DEBUG
    Serial.println("[GPS] Tracking started");
#endif
}

void GpsManager::stopTracking() {
    _tracking = false;

#ifdef DEBUG
    Serial.println("[GPS] Tracking stopped");
#endif
}

bool GpsManager::isTracking() const {
    return _tracking;
}

bool GpsManager::isFixed() {
    return _gps.location.isValid() && _gps.location.isUpdated();
}

double GpsManager::getLat() {
    return _gps.location.lat();
}

double GpsManager::getLon() {
    return _gps.location.lng();
}

float GpsManager::getAltitude() {
    return _gps.altitude.isValid() ? (float)_gps.altitude.meters() : 0.0f;
}

float GpsManager::getSpeed() {
    return _gps.speed.isValid() ? (float)_gps.speed.kmph() : 0.0f;
}

uint8_t GpsManager::getSatellites() {
    return _gps.satellites.isValid() ? (uint8_t)_gps.satellites.value() : 0;
}

float GpsManager::getHdop() {
    return _gps.hdop.isValid() ? (float)_gps.hdop.hdop() : 99.9f;
}

uint32_t GpsManager::getTimestamp() {
    if (!_gps.date.isValid() || !_gps.time.isValid()) return 0;

    // Manual UTC epoch — avoids mktime() local-time dependency
    int y = _gps.date.year();
    int m = _gps.date.month();
    int d = _gps.date.day();

    // Days since Unix epoch using proleptic Gregorian calendar
    if (m < 3) { y--; m += 12; }
    uint32_t days = 365u * y + y / 4 - y / 100 + y / 400
                  + (153 * m - 457) / 5 + d - 719469;

    return days * 86400u
         + (uint32_t)_gps.time.hour()   * 3600u
         + (uint32_t)_gps.time.minute() * 60u
         + (uint32_t)_gps.time.second();
}

bool GpsManager::popTrackPoint(TrackPoint& point) {
    if (!_pointReady) return false;
    point       = _pendingPoint;
    _pointReady = false;
    return true;
}

void GpsManager::_recordPoint() {
    _pendingPoint = {
        .lat        = getLat(),
        .lon        = getLon(),
        .altitude   = getAltitude(),
        .speed      = getSpeed(),
        .timestamp  = getTimestamp(),
        .satellites = getSatellites(),
        .hdop       = getHdop()
    };
    _pointReady = true;

#ifdef DEBUG
    Serial.printf("[GPS] Point recorded: %.6f, %.6f alt=%.1fm\n",
        _pendingPoint.lat, _pendingPoint.lon, _pendingPoint.altitude);
#endif
}
