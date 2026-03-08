#include <unity.h>
#include <cmath>

// ─── Function under test (mirrors TrackStorage::_haversine) ──────────────────

static float haversine(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371.0;
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = sin(dLat / 2) * sin(dLat / 2)
             + cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0)
             * sin(dLon / 2) * sin(dLon / 2);
    return (float)(R * 2.0 * atan2(sqrt(a), sqrt(1.0 - a)));
}

// ─── Tests ────────────────────────────────────────────────────────────────────

void setUp(void) {}
void tearDown(void) {}

void test_same_point_is_zero() {
    float d = haversine(48.8566, 2.3522, 48.8566, 2.3522);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, d);
}

void test_paris_to_london() {
    // Paris (48.8566, 2.3522) → London (51.5074, -0.1278) ≈ 340 km
    float d = haversine(48.8566, 2.3522, 51.5074, -0.1278);
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 340.0f, d);
}

void test_short_distance() {
    // ~111 m for 0.001° latitude shift at equator
    float d = haversine(0.0, 0.0, 0.001, 0.0);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.111f, d);
}

void test_elevation_not_factored() {
    // Haversine is 2D — result is symmetric
    float d1 = haversine(44.28, 4.72, 44.29, 4.73);
    float d2 = haversine(44.29, 4.73, 44.28, 4.72);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, d1, d2);
}

void test_accumulation_is_additive() {
    // A→B + B→C should approximate A→C for short distances
    float ab = haversine(44.280, 4.720, 44.281, 4.721);
    float bc = haversine(44.281, 4.721, 44.282, 4.722);
    float ac = haversine(44.280, 4.720, 44.282, 4.722);
    TEST_ASSERT_FLOAT_WITHIN(0.002f, ac, ab + bc);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_same_point_is_zero);
    RUN_TEST(test_paris_to_london);
    RUN_TEST(test_short_distance);
    RUN_TEST(test_elevation_not_factored);
    RUN_TEST(test_accumulation_is_additive);
    return UNITY_END();
}
