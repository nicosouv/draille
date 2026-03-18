#include "ImuManager.h"
#include <Arduino.h>
#include <math.h>

void ImuManager::begin() {
    if (!_imu.begin(IMU_CS_PIN, IMU_MOSI, IMU_MISO, IMU_SCLK)) {
#ifdef DEBUG
        Serial.println("[IMU] QMI8658 not found — check wiring");
#endif
        return;
    }

    _imu.configAccelerometer(
        SensorQMI8658::ACC_RANGE_4G,
        SensorQMI8658::ACC_ODR_125Hz,
        SensorQMI8658::LPF_MODE_2
    );
    _imu.enableAccelerometer();

#ifdef DEBUG
    Serial.println("[IMU] QMI8658 initialized");
#endif
}

void ImuManager::update() {
    uint32_t now = millis();
    uint32_t interval = 1000 / IMU_SAMPLE_RATE;

    if (now - _lastSampleMs < interval) return;
    _lastSampleMs = now;

    if (!_imu.getDataReady()) return;

    float x, y, z;
    if (!_imu.getAccelerometer(x, y, z)) return;

    float mag = _computeMagnitude(x, y, z);

    // Low-pass filter
    _filteredMag = STEP_LPF_ALPHA * mag + (1.0f - STEP_LPF_ALPHA) * _filteredMag;

    // Peak detection
    if (_filteredMag > STEP_THRESHOLD && (now - _lastStepMs) > STEP_MIN_INTERVAL) {
        _stepCount++;
        _lastStepMs = now;

#ifdef DEBUG
        Serial.printf("[IMU] Step #%u (mag=%.3f)\n", _stepCount, _filteredMag);
#endif
    }
}

uint32_t ImuManager::getStepCount() const {
    return _stepCount;
}

void ImuManager::resetStepCount() {
    _stepCount   = 0;
    _lastStepMs  = 0;
    _filteredMag = 1.0f;
}

float ImuManager::_computeMagnitude(float x, float y, float z) const {
    return sqrtf(x * x + y * y + z * z);
}
