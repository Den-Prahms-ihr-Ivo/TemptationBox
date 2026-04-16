#include <string.h>
#include "unity.h"
#include "cache.h"

// ── Fixtures ──────────────────────────────────────────────────────────────────

#define NOW 1700000000UL
#define STALE_THRESHOLD_S (2 * 60 * 60)   // 2 hours, matches cache.h

// mock NVS state
static uint8_t mock_nvs_buf[sizeof(ScheduleCache)];
static bool    mock_nvs_has_data = false;

static void mock_nvs_write(const uint8_t *data, size_t len) {
    memcpy(mock_nvs_buf, data, len);
    mock_nvs_has_data = true;
}

static bool mock_nvs_read(uint8_t *out, size_t len) {
    if (!mock_nvs_has_data) return false;
    memcpy(out, mock_nvs_buf, len);
    return true;
}

static uint32_t mock_time;
static uint32_t mock_time_now(void) { return mock_time; }

static HAL make_hal(void) {
    return (HAL){
        .nvs_write   = mock_nvs_write,
        .nvs_read    = mock_nvs_read,
        .time_now_unix = mock_time_now,
    };
}

// ── REQ-SCHED-003 ─────────────────────────────────────────────────────────────

void test_cache_returns_stale_when_nothing_stored(void) {
    mock_nvs_has_data = false;
    mock_time         = NOW;
    HAL hal           = make_hal();

    ScheduleCache c   = cache_get(&hal);

    TEST_ASSERT_TRUE(c.is_stale);
    TEST_ASSERT_EQUAL(0, c.count);
}

void test_cache_not_stale_when_generated_at_is_fresh(void) {
    mock_time      = NOW;
    HAL hal        = make_hal();

    ScheduleCache fresh = {
        .generated_at = NOW - 3600,   // 1 hour old — within threshold
        .count        = 0,
        .is_stale     = false,
    };
    cache_store(&hal, &fresh);

    ScheduleCache c = cache_get(&hal);
    TEST_ASSERT_FALSE(c.is_stale);
}

void test_cache_stale_when_generated_at_exceeds_threshold(void) {
    mock_time      = NOW;
    HAL hal        = make_hal();

    ScheduleCache old = {
        .generated_at = NOW - STALE_THRESHOLD_S - 1,   // just over 2 hours
        .count        = 0,
        .is_stale     = false,
    };
    cache_store(&hal, &old);

    ScheduleCache c = cache_get(&hal);
    TEST_ASSERT_TRUE(c.is_stale);
}

void test_cache_store_and_get_preserves_events(void) {
    mock_time      = NOW;
    HAL hal        = make_hal();

    ScheduleCache input = {
        .generated_at = NOW - 60,
        .count        = 2,
        .is_stale     = false,
        .events       = {
            { .start_unix = NOW + 3600, .end_unix = NOW + 7200, .mode = MODE_RESTRICTED },
            { .start_unix = NOW + 7200, .end_unix = NOW + 10800, .mode = MODE_DEEP_FOCUS },
        }
    };
    cache_store(&hal, &input);

    ScheduleCache c = cache_get(&hal);
    TEST_ASSERT_EQUAL(2, c.count);
    TEST_ASSERT_EQUAL(MODE_RESTRICTED, c.events[0].mode);
    TEST_ASSERT_EQUAL(MODE_DEEP_FOCUS, c.events[1].mode);
}

void test_cache_store_clears_stale_flag(void) {
    mock_time      = NOW;
    HAL hal        = make_hal();

    // first store something stale
    ScheduleCache old = { .generated_at = NOW - STALE_THRESHOLD_S - 1 };
    cache_store(&hal, &old);

    // now store something fresh
    ScheduleCache fresh = { .generated_at = NOW - 60, .count = 0 };
    cache_store(&hal, &fresh);

    ScheduleCache c = cache_get(&hal);
    TEST_ASSERT_FALSE(c.is_stale);
}

// ── Runner ────────────────────────────────────────────────────────────────────

void setUp(void)    { mock_nvs_has_data = false; }
void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cache_returns_stale_when_nothing_stored);
    RUN_TEST(test_cache_not_stale_when_generated_at_is_fresh);
    RUN_TEST(test_cache_stale_when_generated_at_exceeds_threshold);
    RUN_TEST(test_cache_store_and_get_preserves_events);
    RUN_TEST(test_cache_store_clears_stale_flag);
    UNITY_END();
    return 0;
}