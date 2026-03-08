#include <unity.h>
#include <cstdio>
#include <cstdint>
#include <cstring>

// ─── Duration formatting (mirrors DisplayManager::_fmtDuration) ───────────────

static void fmtDuration(uint32_t sec, char* buf, size_t len) {
    uint32_t h = sec / 3600;
    uint32_t m = (sec % 3600) / 60;
    uint32_t s = sec % 60;
    if (h > 0) {
        snprintf(buf, len, "%02lu:%02lu:%02lu",
                 (unsigned long)h, (unsigned long)m, (unsigned long)s);
    } else {
        snprintf(buf, len, "%02lu:%02lu", (unsigned long)m, (unsigned long)s);
    }
}

// ─── Steps formatting (mirrors drawDashboard inline logic) ────────────────────

static void fmtSteps(uint32_t steps, char* buf, size_t len) {
    if (steps < 10000) {
        snprintf(buf, len, "%lu", (unsigned long)steps);
    } else {
        snprintf(buf, len, "%.1fk", steps / 1000.0f);
    }
}

// ─── Tests ────────────────────────────────────────────────────────────────────

void setUp(void) {}
void tearDown(void) {}

// Duration tests

void test_duration_zero_seconds() {
    char buf[9];
    fmtDuration(0, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("00:00", buf);
}

void test_duration_59_seconds() {
    char buf[9];
    fmtDuration(59, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("00:59", buf);
}

void test_duration_60_seconds_rolls_to_minute() {
    char buf[9];
    fmtDuration(60, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("01:00", buf);
}

void test_duration_3599_seconds_last_minute() {
    char buf[9];
    fmtDuration(3599, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("59:59", buf);
}

void test_duration_3600_seconds_shows_hours() {
    char buf[9];
    fmtDuration(3600, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("01:00:00", buf);
}

void test_duration_3661_seconds_one_hour_one_min_one_sec() {
    char buf[9];
    fmtDuration(3661, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("01:01:01", buf);
}

void test_duration_7322_seconds_two_hours() {
    char buf[9];
    fmtDuration(7322, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("02:02:02", buf);
}

// Steps tests

void test_steps_zero() {
    char buf[12];
    fmtSteps(0, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("0", buf);
}

void test_steps_below_threshold() {
    char buf[12];
    fmtSteps(9999, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("9999", buf);
}

void test_steps_at_threshold_uses_k_format() {
    char buf[12];
    fmtSteps(10000, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("10.0k", buf);
}

void test_steps_mid_range_k_format() {
    char buf[12];
    fmtSteps(12500, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("12.5k", buf);
}

void test_steps_large_k_format() {
    char buf[12];
    fmtSteps(100000, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("100.0k", buf);
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_duration_zero_seconds);
    RUN_TEST(test_duration_59_seconds);
    RUN_TEST(test_duration_60_seconds_rolls_to_minute);
    RUN_TEST(test_duration_3599_seconds_last_minute);
    RUN_TEST(test_duration_3600_seconds_shows_hours);
    RUN_TEST(test_duration_3661_seconds_one_hour_one_min_one_sec);
    RUN_TEST(test_duration_7322_seconds_two_hours);

    RUN_TEST(test_steps_zero);
    RUN_TEST(test_steps_below_threshold);
    RUN_TEST(test_steps_at_threshold_uses_k_format);
    RUN_TEST(test_steps_mid_range_k_format);
    RUN_TEST(test_steps_large_k_format);

    return UNITY_END();
}
