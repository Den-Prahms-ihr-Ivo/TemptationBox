#include "unity.h"
#include "schedule.h"

// ── Fixtures ──────────────────────────────────────────────────────────────────

#define NOW 1700000000UL   // arbitrary fixed unix timestamp for testing

// ── Helpers ───────────────────────────────────────────────────────────────────

static ScheduleEvent make_event(uint32_t start_unix, uint32_t modified_unix) {
    return (ScheduleEvent) {
        .start_unix    = start_unix,
        .modified_unix = modified_unix,
        .mode          = MODE_RESTRICTED,
    };
}

// ── REQ-SCHED-001 ─────────────────────────────────────────────────────────────

void test_2day_rule_excludes_event_modified_47h_before_start(void) {
    ScheduleEvent ev = make_event(
        NOW + (48 * 3600),   // starts 48h from now
        NOW + (1  * 3600)    // modified only 47h before start
    );
    TEST_ASSERT_FALSE(schedule_accept_event(&ev, NOW));
}

void test_2day_rule_accepts_event_modified_49h_before_start(void) {
    ScheduleEvent ev = make_event(
        NOW + (48 * 3600),   // starts 48h from now
        NOW - (1  * 3600)    // modified 49h before start
    );
    TEST_ASSERT_TRUE(schedule_accept_event(&ev, NOW));
}

void test_2day_rule_rejects_event_modified_exactly_48h_before_start(void) {
    ScheduleEvent ev = make_event(
        NOW + (48 * 3600),
        NOW                  // modified exactly 48h before — boundary, should exclude
    );
    TEST_ASSERT_FALSE(schedule_accept_event(&ev, NOW));
}

// ── Runner ────────────────────────────────────────────────────────────────────

void setUp(void)    {}   // required by Unity, runs before each test
void tearDown(void) {}   // required by Unity, runs after each test

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_2day_rule_excludes_event_modified_47h_before_start);
    RUN_TEST(test_2day_rule_accepts_event_modified_49h_before_start);
    RUN_TEST(test_2day_rule_rejects_event_modified_exactly_48h_before_start);

    return UNITY_END();
}