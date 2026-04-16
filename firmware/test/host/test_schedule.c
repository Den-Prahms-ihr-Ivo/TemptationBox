#include "unity.h"
#include "schedule.h"

// ── Fixtures ──────────────────────────────────────────────────────────────────

#define NOW 1700000000UL   // arbitrary fixed unix timestamp for testing

// ── Helpers ───────────────────────────────────────────────────────────────────

static ScheduleEvent make_event(uint32_t start, uint32_t end, ScheduleMode mode) {
    return (ScheduleEvent){
        .start_unix = start,
        .end_unix   = end,
        .mode       = mode,
    };
}

// ── REQ-SCHED-002 ─────────────────────────────────────────────────────────────

void test_resolve_returns_correct_mode_when_now_inside_event(void) {
    ScheduleEvent events[] = {
        make_event(NOW - 3600, NOW + 3600, MODE_RESTRICTED),
    };
    ScheduleWindow w = schedule_resolve(events, 1, NOW);
    TEST_ASSERT_TRUE(w.valid);
    TEST_ASSERT_EQUAL(MODE_RESTRICTED, w.mode);
    TEST_ASSERT_EQUAL(NOW + 3600, w.until_unix);
}

void test_resolve_returns_free_when_now_before_all_events(void) {
    ScheduleEvent events[] = {
        make_event(NOW + 3600, NOW + 7200, MODE_RESTRICTED),
    };
    ScheduleWindow w = schedule_resolve(events, 1, NOW);
    TEST_ASSERT_FALSE(w.valid);
    TEST_ASSERT_EQUAL(MODE_FREE, w.mode);
}

void test_resolve_returns_free_when_now_after_all_events(void) {
    ScheduleEvent events[] = {
        make_event(NOW - 7200, NOW - 3600, MODE_RESTRICTED),
    };
    ScheduleWindow w = schedule_resolve(events, 1, NOW);
    TEST_ASSERT_FALSE(w.valid);
    TEST_ASSERT_EQUAL(MODE_FREE, w.mode);
}

void test_resolve_includes_now_at_start_boundary(void) {
    ScheduleEvent events[] = {
        make_event(NOW, NOW + 3600, MODE_RESTRICTED),
    };
    ScheduleWindow w = schedule_resolve(events, 1, NOW);
    TEST_ASSERT_TRUE(w.valid);
    TEST_ASSERT_EQUAL(MODE_RESTRICTED, w.mode);
}

void test_resolve_excludes_now_at_end_boundary(void) {
    ScheduleEvent events[] = {
        make_event(NOW - 3600, NOW, MODE_RESTRICTED),
    };
    ScheduleWindow w = schedule_resolve(events, 1, NOW);
    TEST_ASSERT_FALSE(w.valid);
    TEST_ASSERT_EQUAL(MODE_FREE, w.mode);
}

void test_resolve_returns_free_for_empty_event_list(void) {
    ScheduleWindow w = schedule_resolve(NULL, 0, NOW);
    TEST_ASSERT_FALSE(w.valid);
    TEST_ASSERT_EQUAL(MODE_FREE, w.mode);
}

void test_resolve_handles_multiple_events_picks_correct_one(void) {
    ScheduleEvent events[] = {
        make_event(NOW - 7200, NOW - 3600, MODE_RESTRICTED),  // past
        make_event(NOW - 1800, NOW + 1800, MODE_DEEP_FOCUS),  // current
        make_event(NOW + 3600, NOW + 7200, MODE_SLEEP),       // future
    };
    ScheduleWindow w = schedule_resolve(events, 3, NOW);
    TEST_ASSERT_TRUE(w.valid);
    TEST_ASSERT_EQUAL(MODE_DEEP_FOCUS, w.mode);
    TEST_ASSERT_EQUAL(NOW + 1800, w.until_unix);
}

// ── Runner ────────────────────────────────────────────────────────────────────

void setUp(void)    {}   // required by Unity, runs before each test
void tearDown(void) {}   // required by Unity, runs after each test

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_resolve_returns_correct_mode_when_now_inside_event);
    RUN_TEST(test_resolve_returns_free_when_now_before_all_events);
    RUN_TEST(test_resolve_returns_free_when_now_after_all_events);
    RUN_TEST(test_resolve_includes_now_at_start_boundary);
    RUN_TEST(test_resolve_excludes_now_at_end_boundary);
    RUN_TEST(test_resolve_returns_free_for_empty_event_list);
    RUN_TEST(test_resolve_handles_multiple_events_picks_correct_one);

    return UNITY_END();
}