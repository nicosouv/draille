#include <FS.h>
#include <SPIFFS.h>
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

    // Display first — everything else shows boot progress
    display.begin();
    display.drawBoot(0, 4, "GPS");

    gps.begin();
    display.drawBoot(1, 4, "IMU");

    imu.begin();
    display.drawBoot(2, 4, "Storage");

    storage.begin();
    display.drawBoot(3, 4, "UI");

    ui.begin();
    display.drawBoot(4, 4, "Ready");

    delay(500); // brief pause on "Ready" before switching to dashboard

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
