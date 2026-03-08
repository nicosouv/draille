#pragma once

#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include "../config.h"
#include "../types.h"

class GpsManager {
public:
    void begin();
    void update();

    bool     isFixed();
    double   getLat();
    double   getLon();
    float    getAltitude();
    float    getSpeed();
    uint8_t  getSatellites();
    float    getHdop();
    uint32_t getTimestamp();

    bool     popTrackPoint(TrackPoint& point);

    void     startTracking();
    void     stopTracking();
    bool     isTracking() const;

private:
    TinyGPSPlus    _gps;
    HardwareSerial _serial{1};

    bool       _tracking     = false;
    bool       _pointReady   = false;
    TrackPoint _pendingPoint = {};
    uint32_t   _lastRecordMs = 0;

    void _recordPoint();
};
