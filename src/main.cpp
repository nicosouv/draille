#include <FS.h>
#include <SPIFFS.h>
#include <Arduino.h>
#include "config.h"
#include "pmu/PmuManager.h"
#include "gps/GpsManager.h"
#include "imu/ImuManager.h"
#include "display/DisplayManager.h"
#include "ui/UIManager.h"
#include "storage/TrackStorage.h"
#include "export/WifiExport.h"

PmuManager     pmu;
GpsManager     gps;
ImuManager     imu;
DisplayManager display;
TrackStorage   storage;
WifiExport     wifiExport(storage);
UIManager      ui(gps, imu, display, storage, wifiExport);

void setup() {
#ifdef DEBUG
    Serial.begin(115200);
    uint32_t _t = millis();
    while (!Serial && millis() - _t < 2000);
    Serial.println("[BOOT] Draille starting...");
#endif

    // PMU first — powers GPS and peripherals
    pmu.begin();
    delay(2000);  // GPS needs time to power up after PMU enables ALDO4

    bool spiffsOk = SPIFFS.begin(true);
#ifdef DEBUG
    Serial.printf("[BOOT] SPIFFS=%s\n", spiffsOk ? "ok" : "FAILED");
#endif

    // Display first — everything else shows boot progress
    display.begin();
    display.drawBoot(0, 4, "GPS");
#ifdef DEBUG
    Serial.println("[BOOT] display ok");
#endif

    gps.begin();
    display.drawBoot(1, 4, "IMU");

    imu.begin();
    display.drawBoot(2, 4, "Storage");
#ifdef DEBUG
    Serial.println("[BOOT] imu ok");
#endif

    storage.begin();
    display.drawBoot(3, 4, "UI");
#ifdef DEBUG
    Serial.println("[BOOT] storage ok");
#endif

    ui.begin();
    display.drawBoot(4, 4, "Ready");

    delay(500);

#ifdef DEBUG
    Serial.println("[BOOT] all modules ok");
#endif
}

void loop() {
    gps.update();
    imu.update();
    wifiExport.update();

#ifdef DEBUG
    static uint32_t _lastDbg = 0;
    if (millis() - _lastDbg > 500) {
        _lastDbg = millis();
        display.setDebugData(pmu.isOk(), gps.rawBytesReceived, gps.getSatellites(), gps.isFixed(),
                             gps.nmeaOk, gps.nmeaFail);
        display.drawDebugScreen();
    }
#else
    ui.update();
    display.update();
#endif
}
