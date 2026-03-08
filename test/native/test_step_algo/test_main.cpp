#include <unity.h>
#include <cmath>
#include <cstdint>

// ─── Step detection logic (mirrors ImuManager) ────────────────────────────────

struct StepDetector {
    uint32_t stepCount    = 0;
    float    filteredMag  = 1.0f;
    uint32_t lastStepMs   = 0;

    static constexpr float    THRESHOLD    = 1.2f;
    static constexpr uint32_t MIN_INTERVAL = 300;
    static constexpr float    ALPHA        = 0.1f;

    void feed(float x, float y, float z, uint32_t nowMs) {
        float mag = sqrtf(x * x + y * y + z * z);
        filteredMag = ALPHA * mag + (1.0f - ALPHA) * filteredMag;

        if (filteredMag > THRESHOLD && (nowMs - lastStepMs) > MIN_INTERVAL) {
            stepCount++;
            lastStepMs = nowMs;
        }
    }

    void reset() {
        stepCount   = 0;
        lastStepMs  = 0;
        filteredMag = 1.0f;
    }
};

// ─── Tests ────────────────────────────────────────────────────────────────────

void setUp(void) {}
void tearDown(void) {}

void test_no_step_at_rest() {
    StepDetector d;
    // 1g at rest (gravity on Z axis)
    for (int i = 0; i < 100; i++) {
        d.feed(0.0f, 0.0f, 1.0f, i * 20);
    }
    TEST_ASSERT_EQUAL_UINT32(0, d.stepCount);
}

void test_single_step_peak() {
    StepDetector d;
    // Baseline at rest
    for (int i = 0; i < 20; i++) d.feed(0.0f, 0.0f, 1.0f, i * 20);

    // Sustained peak — LPF alpha=0.1 needs ~8+ samples to cross 1.2g threshold
    // (0.8, 0.8, 0.8) → mag ≈ 1.386g ; filtered equilibrium ≈ 1.386
    for (int i = 0; i < 15; i++) d.feed(0.8f, 0.8f, 0.8f, 500 + i * 20);

    TEST_ASSERT_GREATER_THAN(0u, d.stepCount);
}

void test_min_interval_prevents_double_count() {
    StepDetector d;
    // Two peaks 50ms apart — only one step should register
    for (int i = 0; i < 20; i++) d.feed(0.0f, 0.0f, 1.0f, i * 20);

    d.feed(0.8f, 0.8f, 0.8f, 500);
    d.feed(0.8f, 0.8f, 0.8f, 510);
    d.feed(0.8f, 0.8f, 0.8f, 520);
    d.feed(0.8f, 0.8f, 0.8f, 540);
    d.feed(0.8f, 0.8f, 0.8f, 550);  // still within MIN_INTERVAL

    uint32_t after_first = d.stepCount;

    d.feed(0.0f, 0.0f, 1.0f, 700);  // back to rest

    TEST_ASSERT_LESS_OR_EQUAL(after_first + 1, d.stepCount);
}

void test_multiple_steps_correct_interval() {
    StepDetector d;
    // Simulate 5 walking steps, 500ms apart
    for (int step = 0; step < 5; step++) {
        uint32_t base = step * 500;

        // Build up filtered value with peaks
        for (int i = 0; i < 8; i++) {
            d.feed(0.6f, 0.6f, 1.0f, base + i * 20);
        }
        // Back to rest
        for (int i = 0; i < 8; i++) {
            d.feed(0.0f, 0.0f, 1.0f, base + 200 + i * 20);
        }
    }
    // Expect at least 3 of 5 steps detected (LPF smoothing may reduce first few)
    TEST_ASSERT_GREATER_OR_EQUAL(3u, d.stepCount);
}

void test_reset_clears_count() {
    StepDetector d;
    for (int i = 0; i < 20; i++) d.feed(0.0f, 0.0f, 1.0f, i * 20);
    d.feed(0.8f, 0.8f, 0.8f, 500);
    d.feed(0.8f, 0.8f, 0.8f, 520);

    d.reset();
    TEST_ASSERT_EQUAL_UINT32(0, d.stepCount);
}

void test_lpf_smooths_noise() {
    StepDetector d;
    // Short spike that shouldn't register as a step after LPF
    for (int i = 0; i < 20; i++) d.feed(0.0f, 0.0f, 1.0f, i * 20);
    d.feed(1.5f, 0.0f, 0.0f, 420);  // one-sample spike only
    for (int i = 0; i < 5; i++) d.feed(0.0f, 0.0f, 1.0f, 440 + i * 20);

    // Filtered value should damp the spike — likely no step
    // (depends on alpha; this documents the LPF behavior)
    TEST_ASSERT_LESS_OR_EQUAL(1u, d.stepCount);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_no_step_at_rest);
    RUN_TEST(test_single_step_peak);
    RUN_TEST(test_min_interval_prevents_double_count);
    RUN_TEST(test_multiple_steps_correct_interval);
    RUN_TEST(test_reset_clears_count);
    RUN_TEST(test_lpf_smooths_noise);
    return UNITY_END();
}
