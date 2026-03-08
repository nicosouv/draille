#include <unity.h>
#include <cstdint>

// ─── Function under test (mirrors GpsManager::getTimestamp logic) ─────────────

static uint32_t gpsToEpoch(int year, int month, int day, int hour, int min, int sec) {
    int y = year;
    int m = month;
    int d = day;
    if (m < 3) { y--; m += 12; }
    uint32_t days = 365u * y + y / 4 - y / 100 + y / 400
                  + (153 * m - 457) / 5 + d - 719469;
    return days * 86400u + (uint32_t)hour * 3600u + (uint32_t)min * 60u + (uint32_t)sec;
}

// ─── Tests ────────────────────────────────────────────────────────────────────

void setUp(void) {}
void tearDown(void) {}

void test_unix_epoch_origin() {
    // 1970-01-01 00:00:00 UTC = 0
    TEST_ASSERT_EQUAL_UINT32(0, gpsToEpoch(1970, 1, 1, 0, 0, 0));
}

void test_known_timestamp() {
    // 2024-03-08 08:30:05 UTC = 1709886605
    TEST_ASSERT_EQUAL_UINT32(1709886605u, gpsToEpoch(2024, 3, 8, 8, 30, 5));
}

void test_leap_year_day() {
    // 2024-02-29 00:00:00 UTC = 1709164800 (2024 is a leap year)
    TEST_ASSERT_EQUAL_UINT32(1709164800u, gpsToEpoch(2024, 2, 29, 0, 0, 0));
}

void test_year_boundary() {
    // 2024-12-31 23:59:59 -> 2025-01-01 00:00:00 is +1
    uint32_t a = gpsToEpoch(2024, 12, 31, 23, 59, 59);
    uint32_t b = gpsToEpoch(2025,  1,  1,  0,  0,  0);
    TEST_ASSERT_EQUAL_UINT32(1u, b - a);
}

void test_time_components() {
    // Same day, 1h apart
    uint32_t a = gpsToEpoch(2024, 6, 1, 12,  0, 0);
    uint32_t b = gpsToEpoch(2024, 6, 1, 13,  0, 0);
    TEST_ASSERT_EQUAL_UINT32(3600u, b - a);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_unix_epoch_origin);
    RUN_TEST(test_known_timestamp);
    RUN_TEST(test_leap_year_day);
    RUN_TEST(test_year_boundary);
    RUN_TEST(test_time_components);
    return UNITY_END();
}
