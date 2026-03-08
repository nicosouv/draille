#include "WifiExport.h"
#include <ArduinoJson.h>
#include <Arduino.h>

WifiExport::WifiExport(TrackStorage& storage) : _storage(storage) {}

// ─── Lifecycle ────────────────────────────────────────────────────────────────

void WifiExport::startExport() {
    if (_active) return;

    WiFi.softAP(WIFI_SSID);

#ifdef DEBUG
    Serial.printf("[WIFI] AP started — SSID: %s  IP: %s\n",
        WIFI_SSID, WiFi.softAPIP().toString().c_str());
#endif

    _setupRoutes();
    _server.begin();

    _active        = true;
    _lastRequestMs = millis();
}

void WifiExport::stopExport() {
    if (!_active) return;

    _server.stop();
    WiFi.softAPdisconnect(true);

    _active = false;

#ifdef DEBUG
    Serial.println("[WIFI] AP stopped");
#endif
}

bool WifiExport::isActive() const {
    return _active;
}

void WifiExport::update() {
    if (!_active) return;

    _server.handleClient();

    if (millis() - _lastRequestMs > WIFI_TIMEOUT_MS) {
        stopExport();

#ifdef DEBUG
        Serial.println("[WIFI] Timeout — AP stopped");
#endif
    }
}

// ─── Routes ───────────────────────────────────────────────────────────────────

void WifiExport::_setupRoutes() {
    _server.on("/", HTTP_GET, [this]() { _handleRoot(); });

    // All /track/{filename} requests are caught here
    _server.onNotFound([this]() {
        String uri = _server.uri();
        if (uri.startsWith("/track/")) {
            if (_server.method() == HTTP_GET)    { _handleGetTrack();    return; }
            if (_server.method() == HTTP_DELETE) { _handleDeleteTrack(); return; }
        }
        _handleNotFound();
    });
}

void WifiExport::_handleRoot() {
    _refreshTimeout();

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    uint8_t count = _storage.getSessionCount();
    for (uint8_t i = 0; i < count; i++) {
        Session s;
        if (!_storage.getSession(i, s)) continue;

        JsonObject obj       = arr.add<JsonObject>();
        obj["id"]            = s.filename;
        obj["startTime"]     = s.startTime;
        obj["endTime"]       = s.endTime;
        obj["stepCount"]     = s.stepCount;
        obj["totalDistance"] = s.totalDistance;
        obj["elevationGain"] = s.elevationGain;
        obj["pointCount"]    = s.pointCount;
    }

    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);

#ifdef DEBUG
    Serial.printf("[WIFI] GET / — %u sessions\n", count);
#endif
}

void WifiExport::_handleGetTrack() {
    _refreshTimeout();

    // Extract filename from URI: /track/FILENAME.gpx
    String uri      = _server.uri();
    String filename = uri.substring(7);  // strip "/track/"

    if (filename.isEmpty()) {
        _server.send(400, "text/plain", "missing filename");
        return;
    }

    // Ensure path starts with /tracks/
    String path = filename.startsWith("/") ? filename : "/" + filename;
    if (!path.startsWith("/tracks/")) {
        path = "/tracks/" + filename;
    }

    if (!SPIFFS.exists(path)) {
        _server.send(404, "text/plain", "not found");
        return;
    }

    File f = SPIFFS.open(path, FILE_READ);
    if (!f) {
        _server.send(500, "text/plain", "cannot open file");
        return;
    }

    _server.streamFile(f, "application/gpx+xml");
    f.close();

#ifdef DEBUG
    Serial.printf("[WIFI] GET %s\n", path.c_str());
#endif
}

void WifiExport::_handleDeleteTrack() {
    _refreshTimeout();

    String uri      = _server.uri();
    String filename = uri.substring(7);

    if (filename.isEmpty()) {
        _server.send(400, "text/plain", "missing filename");
        return;
    }

    String path = "/tracks/" + filename;

    if (_storage.deleteSession(path.c_str())) {
        _server.send(200, "text/plain", "deleted");

#ifdef DEBUG
        Serial.printf("[WIFI] DELETE %s — ok\n", path.c_str());
#endif
    } else {
        _server.send(404, "text/plain", "not found");
    }
}

void WifiExport::_handleNotFound() {
    _server.send(404, "text/plain", "not found");
}

void WifiExport::_refreshTimeout() {
    _lastRequestMs = millis();
}
