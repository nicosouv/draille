#pragma once

#include <Wire.h>
#include <SensorQMI8658.hpp>
#include "../config.h"

class ImuManager {
public:
    void begin();
    void update();

    uint32_t getStepCount() const;
    void     resetStepCount();

private:
    SensorQMI8658 _imu;

    uint32_t _stepCount    = 0;
    float    _filteredMag  = 1.0f;
    uint32_t _lastStepMs   = 0;
    uint32_t _lastSampleMs = 0;

    float _computeMagnitude(float x, float y, float z) const;
};
