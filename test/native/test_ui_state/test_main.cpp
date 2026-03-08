#include <unity.h>
#include <cstdint>

// ─── AppScreen enum (mirrors UIManager.h) ─────────────────────────────────────

enum class AppScreen {
    DASHBOARD   = 0,
    TRACKING    = 1,
    STATS       = 2,
    TRACKS      = 3,
    WIFI_EXPORT = 4
};

// ─── Navigation logic (mirrors UIManager _onNext / _onPrev) ───────────────────

static const AppScreen NAV_ORDER[] = {
    AppScreen::DASHBOARD,
    AppScreen::TRACKING,
    AppScreen::STATS,
    AppScreen::TRACKS
};
static const uint8_t NAV_COUNT = 4;

static AppScreen navNext(AppScreen current) {
    if (current == AppScreen::WIFI_EXPORT) return current;
    for (uint8_t i = 0; i < NAV_COUNT; i++) {
        if (NAV_ORDER[i] == current) {
            return NAV_ORDER[(i + 1) % NAV_COUNT];
        }
    }
    return current;
}

static AppScreen navPrev(AppScreen current) {
    if (current == AppScreen::WIFI_EXPORT) return current;
    for (uint8_t i = 0; i < NAV_COUNT; i++) {
        if (NAV_ORDER[i] == current) {
            return NAV_ORDER[(i + NAV_COUNT - 1) % NAV_COUNT];
        }
    }
    return current;
}

// ─── Track list selection logic ───────────────────────────────────────────────

// Mirrors the TRACKS case in _onSelect: wrap-around increment
static uint8_t trackSelectNext(uint8_t selected, uint8_t count) {
    if (count == 0) return 0;
    return (selected + 1) % count;
}

// Mirrors PREV direction for tracks (used when entering from NEXT/PREV nav)
// In the real UI, track list navigation is only via SELECT (wrapping).
// Here we also test a clamped decrement helper (conceptual, for coverage).
static uint8_t trackSelectPrev(uint8_t selected, uint8_t count) {
    if (count == 0) return 0;
    if (selected == 0) return 0;   // clamped, not wrapped
    return selected - 1;
}

// ─── Tracking toggle logic (mirrors _onSelect for TRACKING screen) ────────────

struct TrackingState {
    bool isTracking = false;
};

static AppScreen selectOnTracking(TrackingState& state) {
    // Toggling tracking does not change the screen — stays on TRACKING
    state.isTracking = !state.isTracking;
    return AppScreen::TRACKING;
}

// ─── Long SELECT logic (mirrors _onLongSelect) ────────────────────────────────

static AppScreen longSelect(AppScreen current) {
    if (current == AppScreen::WIFI_EXPORT) {
        return AppScreen::DASHBOARD;
    }
    return AppScreen::WIFI_EXPORT;
}

// ─── Tests ────────────────────────────────────────────────────────────────────

void setUp(void) {}
void tearDown(void) {}

// NEXT navigation

void test_next_from_dashboard_goes_to_tracking() {
    TEST_ASSERT_TRUE(navNext(AppScreen::DASHBOARD) == AppScreen::TRACKING);
}

void test_next_from_tracking_goes_to_stats() {
    TEST_ASSERT_TRUE(navNext(AppScreen::TRACKING) == AppScreen::STATS);
}

void test_next_from_stats_goes_to_tracks() {
    TEST_ASSERT_TRUE(navNext(AppScreen::STATS) == AppScreen::TRACKS);
}

void test_next_from_tracks_wraps_to_dashboard() {
    TEST_ASSERT_TRUE(navNext(AppScreen::TRACKS) == AppScreen::DASHBOARD);
}

void test_next_from_wifi_export_is_noop() {
    TEST_ASSERT_TRUE(navNext(AppScreen::WIFI_EXPORT) == AppScreen::WIFI_EXPORT);
}

// PREV navigation

void test_prev_from_dashboard_wraps_to_tracks() {
    TEST_ASSERT_TRUE(navPrev(AppScreen::DASHBOARD) == AppScreen::TRACKS);
}

void test_prev_from_tracking_goes_to_dashboard() {
    TEST_ASSERT_TRUE(navPrev(AppScreen::TRACKING) == AppScreen::DASHBOARD);
}

void test_prev_from_stats_goes_to_tracking() {
    TEST_ASSERT_TRUE(navPrev(AppScreen::STATS) == AppScreen::TRACKING);
}

void test_prev_from_tracks_goes_to_stats() {
    TEST_ASSERT_TRUE(navPrev(AppScreen::TRACKS) == AppScreen::STATS);
}

void test_prev_from_wifi_export_is_noop() {
    TEST_ASSERT_TRUE(navPrev(AppScreen::WIFI_EXPORT) == AppScreen::WIFI_EXPORT);
}

// Round-trip: four NEXTs returns to origin

void test_four_nexts_returns_to_start() {
    AppScreen s = AppScreen::DASHBOARD;
    s = navNext(s);
    s = navNext(s);
    s = navNext(s);
    s = navNext(s);
    TEST_ASSERT_TRUE(s == AppScreen::DASHBOARD);
}

// Round-trip: four PREVs returns to origin

void test_four_prevs_returns_to_start() {
    AppScreen s = AppScreen::STATS;
    s = navPrev(s);
    s = navPrev(s);
    s = navPrev(s);
    s = navPrev(s);
    TEST_ASSERT_TRUE(s == AppScreen::STATS);
}

// Track list selection

void test_track_select_increments_index() {
    TEST_ASSERT_EQUAL_UINT8(1, trackSelectNext(0, 5));
}

void test_track_select_wraps_at_end() {
    TEST_ASSERT_EQUAL_UINT8(0, trackSelectNext(4, 5));
}

void test_track_select_noop_when_empty() {
    TEST_ASSERT_EQUAL_UINT8(0, trackSelectNext(0, 0));
}

void test_track_select_prev_decrements_index() {
    TEST_ASSERT_EQUAL_UINT8(2, trackSelectPrev(3, 5));
}

void test_track_select_prev_clamped_at_zero() {
    TEST_ASSERT_EQUAL_UINT8(0, trackSelectPrev(0, 5));
}

// Short SELECT on TRACKING: toggles isTracking, stays on TRACKING screen

void test_select_on_tracking_starts_tracking() {
    TrackingState state;
    state.isTracking = false;
    AppScreen next = selectOnTracking(state);
    TEST_ASSERT_TRUE(state.isTracking);
    TEST_ASSERT_TRUE(next == AppScreen::TRACKING);
}

void test_select_on_tracking_stops_tracking() {
    TrackingState state;
    state.isTracking = true;
    AppScreen next = selectOnTracking(state);
    TEST_ASSERT_FALSE(state.isTracking);
    TEST_ASSERT_TRUE(next == AppScreen::TRACKING);
}

// Long SELECT: goes to WIFI_EXPORT from any normal screen, back to DASHBOARD from WIFI_EXPORT

void test_long_select_from_dashboard_goes_to_wifi_export() {
    TEST_ASSERT_TRUE(longSelect(AppScreen::DASHBOARD) == AppScreen::WIFI_EXPORT);
}

void test_long_select_from_tracking_goes_to_wifi_export() {
    TEST_ASSERT_TRUE(longSelect(AppScreen::TRACKING) == AppScreen::WIFI_EXPORT);
}

void test_long_select_from_wifi_export_returns_to_dashboard() {
    TEST_ASSERT_TRUE(longSelect(AppScreen::WIFI_EXPORT) == AppScreen::DASHBOARD);
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_next_from_dashboard_goes_to_tracking);
    RUN_TEST(test_next_from_tracking_goes_to_stats);
    RUN_TEST(test_next_from_stats_goes_to_tracks);
    RUN_TEST(test_next_from_tracks_wraps_to_dashboard);
    RUN_TEST(test_next_from_wifi_export_is_noop);

    RUN_TEST(test_prev_from_dashboard_wraps_to_tracks);
    RUN_TEST(test_prev_from_tracking_goes_to_dashboard);
    RUN_TEST(test_prev_from_stats_goes_to_tracking);
    RUN_TEST(test_prev_from_tracks_goes_to_stats);
    RUN_TEST(test_prev_from_wifi_export_is_noop);

    RUN_TEST(test_four_nexts_returns_to_start);
    RUN_TEST(test_four_prevs_returns_to_start);

    RUN_TEST(test_track_select_increments_index);
    RUN_TEST(test_track_select_wraps_at_end);
    RUN_TEST(test_track_select_noop_when_empty);
    RUN_TEST(test_track_select_prev_decrements_index);
    RUN_TEST(test_track_select_prev_clamped_at_zero);

    RUN_TEST(test_select_on_tracking_starts_tracking);
    RUN_TEST(test_select_on_tracking_stops_tracking);

    RUN_TEST(test_long_select_from_dashboard_goes_to_wifi_export);
    RUN_TEST(test_long_select_from_tracking_goes_to_wifi_export);
    RUN_TEST(test_long_select_from_wifi_export_returns_to_dashboard);

    return UNITY_END();
}
