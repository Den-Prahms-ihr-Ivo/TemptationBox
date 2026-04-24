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

void test_resolve_handles_multiple_overlapping_events_picks_correct_one(void) {
    ScheduleEvent events[] = {
        make_event(NOW - 7200, NOW + 3600, MODE_FREE), // Ranking = 0 
        make_event(NOW - 1800, NOW + 1800, MODE_DEEP_FOCUS), // Ranking = 2 
        make_event(NOW - 3600, NOW + 7200, MODE_RESTRICTED), // Ranking = 1
        make_event(NOW + 3600, NOW + 7200, MODE_SLEEP), // highest ranking but in the future
    };
    ScheduleWindow w = schedule_resolve(events, 4, NOW);
    TEST_ASSERT_TRUE(w.valid);
    TEST_ASSERT_EQUAL(MODE_DEEP_FOCUS, w.mode);
    TEST_ASSERT_EQUAL(NOW + 1800, w.until_unix);
}

void test_resolve_overlap_returns_highest_mode(void) {
    ScheduleEvent events[] = {
        make_event(NOW - 3600, NOW + 3600, MODE_RESTRICTED),
        make_event(NOW - 1800, NOW + 1800, MODE_DEEP_FOCUS),
    };
    ScheduleWindow w = schedule_resolve(events, 2, NOW);
    TEST_ASSERT_TRUE(w.valid);
    TEST_ASSERT_EQUAL(MODE_DEEP_FOCUS, w.mode);
}

void test_resolve_overlap_highest_mode_wins_regardless_of_order(void) {
    ScheduleEvent events[] = {
        make_event(NOW - 1800, NOW + 1800, MODE_DEEP_FOCUS),
        make_event(NOW - 3600, NOW + 3600, MODE_RESTRICTED),
    };
    ScheduleWindow w = schedule_resolve(events, 2, NOW);
    TEST_ASSERT_TRUE(w.valid);
    TEST_ASSERT_EQUAL(MODE_DEEP_FOCUS, w.mode);
}

void test_resolve_overlap_until_unix_belongs_to_winning_event(void) {
    ScheduleEvent events[] = {
        make_event(NOW - 3600, NOW + 7200, MODE_RESTRICTED),
        make_event(NOW - 1800, NOW + 1800, MODE_DEEP_FOCUS),
    };
    ScheduleWindow w = schedule_resolve(events, 2, NOW);
    TEST_ASSERT_EQUAL(NOW + 1800, w.until_unix);
}

void test_resolve_overlap_sleep_beats_everything(void) {
    ScheduleEvent events[] = {
        make_event(NOW - 3600, NOW + 3600, MODE_RESTRICTED),
        make_event(NOW - 3600, NOW + 3600, MODE_DEEP_FOCUS),
        make_event(NOW - 3600, NOW + 3600, MODE_SLEEP),
    };
    ScheduleWindow w = schedule_resolve(events, 3, NOW);
    TEST_ASSERT_EQUAL(MODE_SLEEP, w.mode);
}

// ── REQ-SCHED-004 ─────────────────────────────────────────────────────────────

void test_ipad_hold_free_requires_no_hold(void) {
    TEST_ASSERT_EQUAL_UINT32(0, schedule_ipad_hold_ms(MODE_FREE));
}

void test_ipad_hold_permitted_requires_no_hold(void) {
    TEST_ASSERT_EQUAL_UINT32(0, schedule_ipad_hold_ms(MODE_PERMITTED));
}

void test_ipad_hold_restricted_requires_20s(void) {
    TEST_ASSERT_EQUAL_UINT32(20000, schedule_ipad_hold_ms(MODE_RESTRICTED));
}

void test_ipad_hold_deep_focus_requires_60s(void) {
    TEST_ASSERT_EQUAL_UINT32(60000, schedule_ipad_hold_ms(MODE_DEEP_FOCUS));
}

void test_ipad_hold_sleep_denies_access(void) {
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, schedule_ipad_hold_ms(MODE_SLEEP));
}


// ── REQ-SCHED-005 ─────────────────────────────────────────────────────────────

void test_release_behaviour_free_is_instant(void) {
    ReleaseBehaviour w = schedule_release_behaviour(MODE_FREE);
    TEST_ASSERT_EQUAL(RELEASE_INSTANT, w);
}

void test_release_behaviour_permitted_is_instant(void) {
    ReleaseBehaviour w = schedule_release_behaviour(MODE_PERMITTED);
    TEST_ASSERT_EQUAL(RELEASE_INSTANT, w);
}

void test_release_behaviour_restricted_is_hold(void) {
    ReleaseBehaviour w = schedule_release_behaviour(MODE_RESTRICTED);
    TEST_ASSERT_EQUAL(RELEASE_HOLD, w);
}

void test_release_behaviour_deep_focus_is_hold(void) {
    ReleaseBehaviour w = schedule_release_behaviour(MODE_DEEP_FOCUS);
    TEST_ASSERT_EQUAL(RELEASE_HOLD, w);
}

void test_release_behaviour_sleep_is_denied(void) {
    ReleaseBehaviour w = schedule_release_behaviour(MODE_SLEEP);
    TEST_ASSERT_EQUAL(RELEASE_DENIED, w);
}

void test_release_behaviour_unknown_defaults_to_denied(void) {
    ReleaseBehaviour w = schedule_release_behaviour(-1);
    TEST_ASSERT_EQUAL(RELEASE_DENIED, w);
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
    RUN_TEST(test_resolve_handles_multiple_overlapping_events_picks_correct_one);
    RUN_TEST(test_resolve_overlap_returns_highest_mode);
    RUN_TEST(test_resolve_overlap_highest_mode_wins_regardless_of_order);
    RUN_TEST(test_resolve_overlap_until_unix_belongs_to_winning_event);
    RUN_TEST(test_resolve_overlap_sleep_beats_everything);
    RUN_TEST(test_ipad_hold_free_requires_no_hold);
    RUN_TEST(test_ipad_hold_permitted_requires_no_hold);
    RUN_TEST(test_ipad_hold_restricted_requires_20s);
    RUN_TEST(test_ipad_hold_deep_focus_requires_60s);
    RUN_TEST(test_ipad_hold_sleep_denies_access);
    RUN_TEST(test_release_behaviour_free_is_instant);
    RUN_TEST(test_release_behaviour_permitted_is_instant);
    RUN_TEST(test_release_behaviour_restricted_is_hold);
    RUN_TEST(test_release_behaviour_deep_focus_is_hold);
    RUN_TEST(test_release_behaviour_sleep_is_denied);
    RUN_TEST(test_release_behaviour_unknown_defaults_to_denied);

    return UNITY_END();
}