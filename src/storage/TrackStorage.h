#pragma once

#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "../config.h"
#include "../gps/GpsManager.h"

struct Session {
    uint32_t startTime;
    uint32_t endTime;
    uint32_t stepCount;
    float    totalDistance;  // km
    float    elevationGain;  // meters
    uint16_t pointCount;
    char     filename[32];   // GPX filename on SPIFFS
};

class TrackStorage {
public:
    void begin();

    bool openSession(uint32_t startTime);
    bool appendPoint(const TrackPoint& point);
    bool closeSession(uint32_t endTime, uint32_t stepCount);

    bool    isSessionOpen() const;
    bool    getCurrentSession(Session& out) const;
    uint8_t getSessionCount() const;
    bool    getSession(uint8_t index, Session& out) const;
    bool    deleteSession(const char* filename);

private:
    File    _file;
    Session _current        = {};
    bool    _sessionOpen    = false;
    bool    _hasLastPoint   = false;
    TrackPoint _lastPoint   = {};

    Session  _sessions[STORAGE_MAX_SESSIONS];
    uint8_t  _sessionCount = 0;

    void  _loadIndex();
    void  _saveIndex();
    float _haversine(double lat1, double lon1, double lat2, double lon2) const;
    void  _timestampToISO8601(uint32_t ts, char* buf, size_t len) const;
};
