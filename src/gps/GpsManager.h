#pragma once

#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include "../config.h"

struct TrackPoint {
    double   lat;
    double   lon;
    float    altitude;
    float    speed;       // km/h
    uint32_t timestamp;   // Unix epoch
    uint8_t  satellites;
    float    hdop;
};

class GpsManager {
public:
    void begin();
    void update();

    bool     isFixed()      const;
    double   getLat()       const;
    double   getLon()       const;
    float    getAltitude()  const;
    float    getSpeed()     const;
    uint8_t  getSatellites() const;
    float    getHdop()      const;
    uint32_t getTimestamp() const;

    // Returns true and fills point if a new point is ready to record
    bool     popTrackPoint(TrackPoint& point);

    // Call when tracking starts/stops to reset the recording timer
    void     startTracking();
    void     stopTracking();
    bool     isTracking()   const;

private:
    TinyGPSPlus    _gps;
    HardwareSerial _serial{1};

    bool     _tracking       = false;
    bool     _pointReady     = false;
    TrackPoint _pendingPoint = {};
    uint32_t _lastRecordMs   = 0;

    void _recordPoint();
};
