#include "PmuManager.h"
#include <Arduino.h>
#include <Wire.h>

// PMU I2C bus (dedicated, separate from main I2C)
#define PMU_SDA 42
#define PMU_SCL 41

bool PmuManager::begin() {
    if (!_pmu.begin(Wire1, AXP2101_SLAVE_ADDRESS, PMU_SDA, PMU_SCL)) {
#ifdef DEBUG
        Serial.println("[PMU] AXP2101 not found");
#endif
        return false;
    }

    // Power on GPS module (U-blox MAX-M10S on T-Beam Supreme)
    _pmu.setALDO4Voltage(3300);
    _pmu.enableALDO4();

    // Power on SPI peripherals
    _pmu.setALDO3Voltage(3300);
    _pmu.enableALDO3();

    _ok = true;
#ifdef DEBUG
    Serial.println("[PMU] AXP2101 ok — GPS powered on");
#endif
    return true;
}
