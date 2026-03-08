#include <Arduino.h>
#include <unity.h>
#include <Wire.h>
#include <SensorQMI8658.hpp>
#include "../../src/config.h"

// Integration tests — requires ESP32-S3 with QMI8658 wired on I2C

static SensorQMI8658 imu;
static bool imuReady = false;

void setUp(void) {}
void tearDown(void) {}

void test_imu_i2c_detected() {
    Wire.begin(IMU_SDA, IMU_SCL);
    imuReady = imu.begin(Wire, QMI8658_L_SLAVE_ADDRESS);
    TEST_ASSERT_TRUE_MESSAGE(imuReady, "QMI8658 not found — check I2C wiring (SDA=21, SCL=22)");
}

void test_imu_accelerometer_enable() {
    if (!imuReady) TEST_IGNORE_MESSAGE("IMU not detected, skipping");

    imu.configAccelerometer(
        SensorQMI8658::ACC_RANGE_4G,
        SensorQMI8658::ACC_ODR_125Hz,
        SensorQMI8658::LPF_MODE_2
    );
    imu.enableAccelerometer();
    TEST_PASS();  // no exception = success
}

void test_imu_data_readable() {
    if (!imuReady) TEST_IGNORE_MESSAGE("IMU not detected, skipping");

    // Poll for data up to 200ms
    uint32_t start = millis();
    bool got = false;
    while (millis() - start < 200) {
        if (imu.getDataReady()) { got = true; break; }
        delay(10);
    }
    TEST_ASSERT_TRUE_MESSAGE(got, "No data ready from QMI8658 within 200ms");
}

void test_imu_magnitude_reasonable() {
    if (!imuReady) TEST_IGNORE_MESSAGE("IMU not detected, skipping");

    float x, y, z;
    bool ok = imu.getAccelerometer(x, y, z);
    TEST_ASSERT_TRUE(ok);

    float mag = sqrtf(x * x + y * y + z * z);

    // At rest on a desk, magnitude should be ~1g (gravity)
    TEST_ASSERT_FLOAT_WITHIN(0.3f, 1.0f, mag);
}

void test_imu_no_nan_values() {
    if (!imuReady) TEST_IGNORE_MESSAGE("IMU not detected, skipping");

    float x, y, z;
    imu.getAccelerometer(x, y, z);

    TEST_ASSERT_FALSE(isnan(x));
    TEST_ASSERT_FALSE(isnan(y));
    TEST_ASSERT_FALSE(isnan(z));
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    UNITY_BEGIN();
    RUN_TEST(test_imu_i2c_detected);
    RUN_TEST(test_imu_accelerometer_enable);
    RUN_TEST(test_imu_data_readable);
    RUN_TEST(test_imu_magnitude_reasonable);
    RUN_TEST(test_imu_no_nan_values);
    UNITY_END();
}

void loop() {}
