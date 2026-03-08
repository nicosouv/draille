#pragma once
#include <stdint.h>

struct TrackPoint {
    double   lat;
    double   lon;
    float    altitude;
    float    speed;       // km/h
    uint32_t timestamp;   // Unix epoch
    uint8_t  satellites;
    float    hdop;
};

struct Session {
    uint32_t startTime;
    uint32_t endTime;
    uint32_t stepCount;
    float    totalDistance;  // km
    float    elevationGain;  // meters
    uint16_t pointCount;
    char     filename[32];
};
