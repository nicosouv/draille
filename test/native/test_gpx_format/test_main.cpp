#include <unity.h>
#include <cstdio>
#include <cstring>
#include <ctime>

// ─── Helpers under test (mirrors TrackStorage GPX rendering) ─────────────────

static void timestampToISO8601(uint32_t ts, char* buf, size_t len) {
    time_t t = (time_t)ts;
    struct tm* tm_info = gmtime(&t);
    strftime(buf, len, "%Y-%m-%dT%H:%M:%SZ", tm_info);
}

static int buildTrkpt(char* out, size_t outLen,
                      double lat, double lon, float alt,
                      uint32_t ts, float speed) {
    char iso[21];  // "YYYY-MM-DDTHH:MM:SSZ" = 20 chars + \0
    timestampToISO8601(ts, iso, sizeof(iso));
    return snprintf(out, outLen,
        "      <trkpt lat=\"%.7f\" lon=\"%.7f\">\n"
        "        <ele>%.1f</ele>\n"
        "        <time>%s</time>\n"
        "        <extensions><speed>%.2f</speed></extensions>\n"
        "      </trkpt>\n",
        lat, lon, alt, iso, speed);
}

// ─── Tests ────────────────────────────────────────────────────────────────────

void setUp(void) {}
void tearDown(void) {}

void test_iso8601_epoch_zero() {
    char buf[21];
    timestampToISO8601(0, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("1970-01-01T00:00:00Z", buf);
}

void test_iso8601_known_date() {
    char buf[21];
    timestampToISO8601(1709886605u, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("2024-03-08T08:30:05Z", buf);
}

void test_trkpt_contains_lat_lon() {
    char buf[512];
    buildTrkpt(buf, sizeof(buf), 44.2800123, 4.7200456, 85.0f, 1709886605u, 3.2f);
    TEST_ASSERT_NOT_NULL(strstr(buf, "lat=\"44.2800123\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "lon=\"4.7200456\""));
}

void test_trkpt_contains_elevation() {
    char buf[512];
    buildTrkpt(buf, sizeof(buf), 44.28, 4.72, 85.5f, 1709886605u, 3.2f);
    TEST_ASSERT_NOT_NULL(strstr(buf, "<ele>85.5</ele>"));
}

void test_trkpt_contains_speed_extension() {
    char buf[512];
    buildTrkpt(buf, sizeof(buf), 44.28, 4.72, 85.0f, 1709886605u, 3.20f);
    TEST_ASSERT_NOT_NULL(strstr(buf, "<speed>3.20</speed>"));
}

void test_trkpt_contains_iso_time() {
    char buf[512];
    buildTrkpt(buf, sizeof(buf), 44.28, 4.72, 85.0f, 1709886605u, 0.0f);
    TEST_ASSERT_NOT_NULL(strstr(buf, "<time>2024-03-08T08:30:05Z</time>"));
}

void test_trkpt_is_valid_xml_fragment() {
    char buf[512];
    buildTrkpt(buf, sizeof(buf), 44.28, 4.72, 85.0f, 1709886605u, 3.2f);
    TEST_ASSERT_NOT_NULL(strstr(buf, "<trkpt "));
    TEST_ASSERT_NOT_NULL(strstr(buf, "</trkpt>"));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_iso8601_epoch_zero);
    RUN_TEST(test_iso8601_known_date);
    RUN_TEST(test_trkpt_contains_lat_lon);
    RUN_TEST(test_trkpt_contains_elevation);
    RUN_TEST(test_trkpt_contains_speed_extension);
    RUN_TEST(test_trkpt_contains_iso_time);
    RUN_TEST(test_trkpt_is_valid_xml_fragment);
    return UNITY_END();
}
