#include "TrackStorage.h"
#include <Arduino.h>
#include <math.h>
#include <time.h>

// ─── Init ─────────────────────────────────────────────────────────────────────

void TrackStorage::begin() {
    if (!SPIFFS.begin(true)) {
#ifdef DEBUG
        Serial.println("[STORAGE] SPIFFS mount failed");
#endif
        return;
    }

    if (!SPIFFS.exists("/tracks")) {
        SPIFFS.mkdir("/tracks");
    }

    _loadIndex();

#ifdef DEBUG
    Serial.printf("[STORAGE] Ready — %u session(s) on disk\n", _sessionCount);
#endif
}

// ─── Session lifecycle ────────────────────────────────────────────────────────

bool TrackStorage::openSession(uint32_t startTime) {
    if (_sessionOpen) return false;

    char iso[20];
    _timestampToISO8601(startTime, iso, sizeof(iso));

    // Filename: /tracks/YYYYMMDD_HHMMSS.gpx
    time_t ts = (time_t)startTime;
    struct tm* t = gmtime(&ts);
    strftime(_current.filename, sizeof(_current.filename), "/tracks/%Y%m%d_%H%M%S.gpx", t);

    _file = SPIFFS.open(_current.filename, FILE_WRITE);
    if (!_file) {
#ifdef DEBUG
        Serial.printf("[STORAGE] Cannot open file: %s\n", _current.filename);
#endif
        return false;
    }

    // Write GPX header
    _file.printf(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<gpx version=\"1.1\" creator=\"Draille\"\n"
        "     xmlns=\"http://www.topografix.com/GPX/1/1\">\n"
        "  <metadata>\n"
        "    <time>%s</time>\n"
        "  </metadata>\n"
        "  <trk>\n"
        "    <trkseg>\n",
        iso
    );

    _current.startTime    = startTime;
    _current.endTime      = 0;
    _current.stepCount    = 0;
    _current.totalDistance = 0.0f;
    _current.elevationGain = 0.0f;
    _current.pointCount   = 0;
    _hasLastPoint         = false;
    _sessionOpen          = true;

#ifdef DEBUG
    Serial.printf("[STORAGE] Session opened: %s\n", _current.filename);
#endif
    return true;
}

bool TrackStorage::appendPoint(const TrackPoint& point) {
    if (!_sessionOpen || !_file) return false;

    char iso[20];
    _timestampToISO8601(point.timestamp, iso, sizeof(iso));

    _file.printf(
        "      <trkpt lat=\"%.7f\" lon=\"%.7f\">\n"
        "        <ele>%.1f</ele>\n"
        "        <time>%s</time>\n"
        "        <extensions><speed>%.2f</speed></extensions>\n"
        "      </trkpt>\n",
        point.lat, point.lon,
        point.altitude,
        iso,
        point.speed
    );

    // Accumulate stats
    if (_hasLastPoint) {
        _current.totalDistance += _haversine(
            _lastPoint.lat, _lastPoint.lon,
            point.lat, point.lon
        );

        float elevDiff = point.altitude - _lastPoint.altitude;
        if (elevDiff > 0.0f) {
            _current.elevationGain += elevDiff;
        }
    }

    _lastPoint    = point;
    _hasLastPoint = true;
    _current.pointCount++;

#ifdef DEBUG
    Serial.printf("[STORAGE] Point #%u appended\n", _current.pointCount);
#endif
    return true;
}

bool TrackStorage::closeSession(uint32_t endTime, uint32_t stepCount) {
    if (!_sessionOpen || !_file) return false;

    _file.print(
        "    </trkseg>\n"
        "  </trk>\n"
        "</gpx>\n"
    );
    _file.close();

    _current.endTime   = endTime;
    _current.stepCount = stepCount;
    _sessionOpen       = false;

    // Auto-purge oldest if at capacity
    if (_sessionCount >= STORAGE_MAX_SESSIONS) {
        deleteSession(_sessions[0].filename);
    }

    _sessions[_sessionCount++] = _current;
    _saveIndex();

#ifdef DEBUG
    Serial.printf("[STORAGE] Session closed — %.2fkm, %u steps, %u points\n",
        _current.totalDistance, _current.stepCount, _current.pointCount);
#endif
    return true;
}

// ─── Accessors ────────────────────────────────────────────────────────────────

bool TrackStorage::isSessionOpen() const {
    return _sessionOpen;
}

bool TrackStorage::getCurrentSession(Session& out) const {
    if (!_sessionOpen) return false;
    out = _current;
    return true;
}

uint8_t TrackStorage::getSessionCount() const {
    return _sessionCount;
}

bool TrackStorage::getSession(uint8_t index, Session& out) const {
    if (index >= _sessionCount) return false;
    out = _sessions[index];
    return true;
}

bool TrackStorage::deleteSession(const char* filename) {
    if (!SPIFFS.remove(filename)) {
#ifdef DEBUG
        Serial.printf("[STORAGE] Delete failed: %s\n", filename);
#endif
        return false;
    }

    // Shift sessions array
    for (uint8_t i = 0; i < _sessionCount; i++) {
        if (strcmp(_sessions[i].filename, filename) == 0) {
            for (uint8_t j = i; j < _sessionCount - 1; j++) {
                _sessions[j] = _sessions[j + 1];
            }
            _sessionCount--;
            break;
        }
    }

    _saveIndex();

#ifdef DEBUG
    Serial.printf("[STORAGE] Deleted: %s\n", filename);
#endif
    return true;
}

// ─── Index ────────────────────────────────────────────────────────────────────

void TrackStorage::_loadIndex() {
    _sessionCount = 0;

    File f = SPIFFS.open(STORAGE_SESSIONS_INDEX, FILE_READ);
    if (!f) return;

    JsonDocument doc;
    if (deserializeJson(doc, f) != DeserializationError::Ok) {
        f.close();
        return;
    }
    f.close();

    JsonArray arr = doc.as<JsonArray>();
    for (JsonObject obj : arr) {
        if (_sessionCount >= STORAGE_MAX_SESSIONS) break;
        Session& s = _sessions[_sessionCount++];
        s.startTime     = obj["startTime"]     | 0u;
        s.endTime       = obj["endTime"]       | 0u;
        s.stepCount     = obj["stepCount"]     | 0u;
        s.totalDistance = obj["totalDistance"] | 0.0f;
        s.elevationGain = obj["elevationGain"] | 0.0f;
        s.pointCount    = obj["pointCount"]    | 0u;
        strncpy(s.filename, obj["filename"] | "", sizeof(s.filename));
    }
}

void TrackStorage::_saveIndex() {
    File f = SPIFFS.open(STORAGE_SESSIONS_INDEX, FILE_WRITE);
    if (!f) return;

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (uint8_t i = 0; i < _sessionCount; i++) {
        JsonObject obj       = arr.add<JsonObject>();
        obj["startTime"]     = _sessions[i].startTime;
        obj["endTime"]       = _sessions[i].endTime;
        obj["stepCount"]     = _sessions[i].stepCount;
        obj["totalDistance"] = _sessions[i].totalDistance;
        obj["elevationGain"] = _sessions[i].elevationGain;
        obj["pointCount"]    = _sessions[i].pointCount;
        obj["filename"]      = _sessions[i].filename;
    }

    serializeJson(doc, f);
    f.close();
}

// ─── Helpers ──────────────────────────────────────────────────────────────────

float TrackStorage::_haversine(double lat1, double lon1, double lat2, double lon2) const {
    const double R = 6371.0;
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = sin(dLat / 2) * sin(dLat / 2)
             + cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0)
             * sin(dLon / 2) * sin(dLon / 2);
    return (float)(R * 2.0 * atan2(sqrt(a), sqrt(1.0 - a)));
}

void TrackStorage::_timestampToISO8601(uint32_t ts, char* buf, size_t len) const {
    time_t t = (time_t)ts;
    struct tm* tm_info = gmtime(&t);
    strftime(buf, len, "%Y-%m-%dT%H:%M:%SZ", tm_info);
}
