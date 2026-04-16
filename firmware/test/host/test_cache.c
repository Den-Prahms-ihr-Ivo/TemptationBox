#include <string.h>
#include "unity.h"
#include "cache.h"
#include "hal_mock.h"

// ── Fixtures ──────────────────────────────────────────────────────────────────

#define NOW 1700000000UL
#define STALE_THRESHOLD_S (2 * 60 * 60)   // 2 hours, matches cache.h


static uint32_t mock_time;
static uint32_t mock_time_now(void) { return mock_time; }


// ── REQ-SCHED-003 ─────────────────────────────────────────────────────────────
void test_cache_returns_stale_when_nothing_stored(void) {
    HAL hal = hal_mock_create();
    hal_mock_set_time(NOW);

    ScheduleCache c = cache_get(&hal);

    TEST_ASSERT_TRUE(c.is_stale);
    TEST_ASSERT_EQUAL(0, c.count);
}

void test_cache_not_stale_when_generated_at_is_fresh(void) {
    HAL hal = hal_mock_create();
    hal_mock_set_time(NOW);

    ScheduleCache fresh = { .generated_at = NOW - 3600, .count = 0 };
    cache_store(&hal, &fresh);

    ScheduleCache c = cache_get(&hal);
    TEST_ASSERT_FALSE(c.is_stale);
}


// TODO: More Tests!


// ── Runner ────────────────────────────────────────────────────────────────────

void setUp(void)    { hal_mock_nvs_clear(); }
void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cache_returns_stale_when_nothing_stored);
    RUN_TEST(test_cache_not_stale_when_generated_at_is_fresh);
    //RUN_TEST(test_cache_stale_when_generated_at_exceeds_threshold);
    //RUN_TEST(test_cache_store_and_get_preserves_events);
    //RUN_TEST(test_cache_store_clears_stale_flag);
    UNITY_END();
    return 0;
}