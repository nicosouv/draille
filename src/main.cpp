#include <Arduino.h>
#include "config.h"
#include "gps/GpsManager.h"
#include "imu/ImuManager.h"
#include "display/DisplayManager.h"
#include "ui/UIManager.h"
#include "storage/TrackStorage.h"
#include "export/WifiExport.h"

GpsManager     gps;
ImuManager     imu;
DisplayManager display;
TrackStorage   storage;
WifiExport     wifiExport(storage);
UIManager      ui(gps, imu, display, storage, wifiExport);

void setup() {
#ifdef DEBUG
    Serial.begin(115200);
    Serial.println("[BOOT] Draille starting...");
#endif

    gps.begin();
    imu.begin();
    storage.begin();
    display.begin();
    ui.begin();

#ifdef DEBUG
    Serial.println("[BOOT] All modules initialized.");
#endif
}

void loop() {
    gps.update();
    imu.update();
    ui.update();
    display.update();
    wifiExport.update();
}
