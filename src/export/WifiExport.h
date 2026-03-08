#pragma once

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include "../config.h"
#include "../storage/TrackStorage.h"

class WifiExport {
public:
    explicit WifiExport(TrackStorage& storage);

    void begin()  {}
    void update();

    void startExport();
    void stopExport();
    bool isActive() const;

private:
    TrackStorage& _storage;
    WebServer     _server{80};
    bool          _active       = false;
    uint32_t      _lastRequestMs = 0;

    void _setupRoutes();
    void _handleRoot();
    void _handleGetTrack();
    void _handleDeleteTrack();
    void _handleNotFound();

    void _refreshTimeout();
};
