#include "unity.h"
#include "hal_mock.h"
#include "../lock/lock.h"
#include "../schedule/schedule.h"
#include "../audio/audio.h"

// ── Fixtures ──────────────────────────────────────────────────────────────────

static HAL hal;

#define NOW        1700000000UL
#define AFTER_BOOT (NOW + LOCK_BOOT_GRACE_S + 1)



// ── Helpers ───────────────────────────────────────────────────────────────────

static ScheduleWindow free_window(void) {
    return (ScheduleWindow){ .mode = MODE_FREE, .valid = false, .until_unix = 0 };
}

static ScheduleWindow restricted_window(void) {
    return (ScheduleWindow){ .mode = MODE_RESTRICTED, .valid = true, .until_unix = NOW + 3600 };
}

static ScheduleWindow sleep_window(void) {
    return (ScheduleWindow){ .mode = MODE_SLEEP, .valid = true, .until_unix = NOW + 3600 };
}

static void drive_to_idle(void) {
    hal_mock_set_time(AFTER_BOOT);
    lock_tick(free_window(), false, &hal);
    TEST_ASSERT_EQUAL(LOCK_STATE_IDLE, lock_get_state());
}

static void drive_to_armed(void) {
    drive_to_idle();
    lock_tick(restricted_window(), true, &hal);
    TEST_ASSERT_EQUAL(LOCK_STATE_ARMED, lock_get_state());
}

static void drive_to_violation(void) {
    drive_to_armed();
    hal_mock_set_lid_closed(true);          // explicit precondition
    hal_mock_set_lock_engaged(true);        // commitment confirmed
    lock_tick(restricted_window(), true, &hal);  // settle armed state
    lock_tick(restricted_window(), false, &hal); // remote removed → violation
    TEST_ASSERT_EQUAL(LOCK_STATE_VIOLATION, lock_get_state());
}

// ── Initial State ─────────────────────────────────────────────────────────────

void test_lock_initial_state_after_init(void) {
    TEST_ASSERT_EQUAL(LOCK_STATE_BOOT, lock_get_state());
}

// ── REQ-LOCK-001 ─────────────────────────────────────────────────────────────

void test_lock_boot_holds_motor_open(void) {
    lock_tick(free_window(), false, &hal);
    TEST_ASSERT_TRUE(hal_mock_lock_hold_open_last());
}

void test_lock_boot_ignores_restricted_window(void) {
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(LOCK_STATE_BOOT, lock_get_state());
}

void test_lock_boot_does_not_transition_before_grace_period(void) {
    hal_mock_set_time(NOW + LOCK_BOOT_GRACE_S - 1);
    lock_tick(free_window(), false, &hal);
    TEST_ASSERT_EQUAL(LOCK_STATE_BOOT, lock_get_state());
}

void test_lock_boot_transitions_to_idle_after_grace_period(void) {
    hal_mock_set_time(AFTER_BOOT);
    lock_tick(free_window(), false, &hal);
    TEST_ASSERT_EQUAL(LOCK_STATE_IDLE, lock_get_state());
}

// ── REQ-LOCK-002 ─────────────────────────────────────────────────────────────

void test_lock_armed_lid_closed_remote_present_releases_motor(void) {
    drive_to_armed();
    hal_mock_set_lid_closed(true);
    lock_tick(restricted_window(), true, &hal);
    TEST_ASSERT_FALSE(hal_mock_lock_hold_open_last());
}

void test_lock_armed_lid_open_holds_motor_open(void) {
    drive_to_armed();
    hal_mock_set_lid_closed(false);
    lock_tick(restricted_window(), true, &hal);
    TEST_ASSERT_TRUE(hal_mock_lock_hold_open_last());
}

void test_lock_armed_remote_absent_lid_open_holds_motor_open(void) {
    drive_to_armed();
    hal_mock_set_lid_closed(false);  // lid open — prevents violation trigger
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_TRUE(hal_mock_lock_hold_open_last());
}

void test_lock_armed_lid_closed_but_not_engaged_holds_open(void) {
    drive_to_armed();
    hal_mock_set_lid_closed(true);
    hal_mock_set_lock_engaged(false);   // lid resting, latch not caught
    lock_tick(restricted_window(), true, &hal);
    TEST_ASSERT_TRUE(hal_mock_lock_hold_open_last());
}

void test_lock_armed_fully_engaged_with_remote_releases_motor(void) {
    drive_to_armed();
    hal_mock_set_lid_closed(true);
    hal_mock_set_lock_engaged(true);
    lock_tick(restricted_window(), true, &hal);
    TEST_ASSERT_FALSE(hal_mock_lock_hold_open_last());
}

// ── REQ-LOCK-003 ─────────────────────────────────────────────────────────────

void test_lock_idle_to_armed_on_restriction_start(void) {
    drive_to_idle();
    lock_tick(restricted_window(), true, &hal);
    TEST_ASSERT_EQUAL(LOCK_STATE_ARMED, lock_get_state());
}

void test_lock_armed_to_idle_when_restriction_ends(void) {
    drive_to_armed();
    lock_tick(free_window(), true, &hal);
    TEST_ASSERT_EQUAL(LOCK_STATE_IDLE, lock_get_state());
}

void test_lock_armed_stays_armed_during_restriction(void) {
    drive_to_armed();
    hal_mock_set_lid_closed(true);
    lock_tick(restricted_window(), true, &hal);
    TEST_ASSERT_EQUAL(LOCK_STATE_ARMED, lock_get_state());
}

// ── REQ-LOCK-004 ─────────────────────────────────────────────────────────────

void test_lock_armed_lid_closed_remote_absent_plays_nag_soft(void) {
    drive_to_armed();
    hal_mock_set_lid_closed(true);
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(1, hal_mock_audio_call_count());
    TEST_ASSERT_EQUAL(CLIP_NAG_SOFT, hal_mock_last_audio_clip());
}

void test_lock_nag_escalates_to_loud_after_30s(void) {
    drive_to_violation();               // violation entered at AFTER_BOOT
    hal_mock_set_time(AFTER_BOOT + 31); // 31s into violation
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(CLIP_NAG_LOUD, hal_mock_last_audio_clip());
}

void test_lock_nag_stops_when_lid_opens(void) {
    drive_to_violation();
    int count = hal_mock_audio_call_count();
    hal_mock_set_lid_closed(false);
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(count, hal_mock_audio_call_count());
}

void test_lock_no_nag_in_free_mode(void) {
    drive_to_idle();
    int before = hal_mock_audio_call_count();
    hal_mock_set_lid_closed(true);
    lock_tick(free_window(), false, &hal);
    TEST_ASSERT_EQUAL(before, hal_mock_audio_call_count());
}

// ── REQ-LOCK-005 ─────────────────────────────────────────────────────────────

void test_lock_release_button_instant_in_boot(void) {
    hal_mock_set_release_button(true);
    lock_tick(free_window(), false, &hal);
    TEST_ASSERT_TRUE(hal_mock_lock_hold_open_last());
}

void test_lock_release_button_instant_in_free_mode(void) {
    drive_to_idle();
    hal_mock_set_release_button(true);
    lock_tick(free_window(), false, &hal);
    TEST_ASSERT_TRUE(hal_mock_lock_hold_open_last());
}

void test_lock_release_button_restricted_hold_insufficient(void) {
    drive_to_armed();
    hal_mock_set_release_button(true);
    hal_mock_set_time(AFTER_BOOT + 1);   // press recorded at AFTER_BOOT+1
    lock_tick(restricted_window(), true, &hal);
    hal_mock_set_time(AFTER_BOOT + 10);  // 9s elapsed — below 20s threshold
    lock_tick(restricted_window(), true, &hal);
    TEST_ASSERT_FALSE(hal_mock_lock_hold_open_last());
}

void test_lock_release_button_restricted_hold_sufficient(void) {
    drive_to_armed();
    hal_mock_set_release_button(true);
    hal_mock_set_time(AFTER_BOOT + 1);   // press recorded at AFTER_BOOT+1
    lock_tick(restricted_window(), true, &hal);
    hal_mock_set_time(AFTER_BOOT + 22);  // 21s elapsed — above 20s threshold
    lock_tick(restricted_window(), true, &hal);
    TEST_ASSERT_TRUE(hal_mock_lock_hold_open_last());
}

void test_lock_release_button_no_effect_in_sleep(void) {
    drive_to_armed();
    hal_mock_set_lid_closed(true);
    hal_mock_set_release_button(true);
    lock_tick(sleep_window(), true, &hal);
    // MODE_SLEEP → RELEASE_DENIED: button must not override consent logic
    TEST_ASSERT_FALSE(hal_mock_lock_hold_open_last());
}

void test_lock_release_button_release_resets_timer(void) {
    drive_to_armed();
    hal_mock_set_release_button(true);
    hal_mock_set_time(AFTER_BOOT + 1);
    lock_tick(restricted_window(), true, &hal);

    // release the button at 10s
    hal_mock_set_release_button(false);
    hal_mock_set_time(AFTER_BOOT + 11);
    lock_tick(restricted_window(), true, &hal);

    // press again — timer should restart
    hal_mock_set_release_button(true);
    hal_mock_set_time(AFTER_BOOT + 22);   // 21s since first press, 0s since restart
    lock_tick(restricted_window(), true, &hal);
    TEST_ASSERT_FALSE(hal_mock_lock_hold_open_last());
}

// ── REQ-LOCK-006 ─────────────────────────────────────────────────────────────

void test_lock_armed_to_violation_on_remote_removed_lid_closed(void) {
    drive_to_armed();
    hal_mock_set_lid_closed(true);
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(LOCK_STATE_VIOLATION, lock_get_state());
    TEST_ASSERT_EQUAL(1, hal_mock_ir_call_count());
}

void test_lock_violation_no_repeat_ir(void) {
    drive_to_violation();
    int before = hal_mock_ir_call_count();

    // first tick: IDLE → VIOLATION, IR fires
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(before + 1, hal_mock_ir_call_count());

    // second tick: still in VIOLATION, IR must not fire again
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(before + 1, hal_mock_ir_call_count());
}

void test_lock_armed_lid_open_remote_absent_no_violation(void) {
    drive_to_armed();
    hal_mock_set_lid_closed(false);
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(LOCK_STATE_ARMED, lock_get_state());
}

// ── REQ-LOCK-007 ─────────────────────────────────────────────────────────────

void test_lock_violation_to_cooldown_on_remote_return(void) {
    drive_to_violation();
    lock_tick(restricted_window(), true, &hal);
    TEST_ASSERT_EQUAL(LOCK_STATE_COOLDOWN, lock_get_state());
}

void test_lock_cooldown_to_armed_if_restriction_active(void) {
    drive_to_violation();
    lock_tick(restricted_window(), true, &hal);   // COOLDOWN
    lock_tick(restricted_window(), true, &hal);   // next tick → ARMED
    TEST_ASSERT_EQUAL(LOCK_STATE_ARMED, lock_get_state());
}

void test_lock_cooldown_to_idle_if_restriction_ended(void) {
    drive_to_violation();
    lock_tick(restricted_window(), true, &hal);   // COOLDOWN
    lock_tick(free_window(), true, &hal);         // restriction over
    TEST_ASSERT_EQUAL(LOCK_STATE_IDLE, lock_get_state());
}

void test_lock_ir_fires_on_remote_return_in_free_mode(void) {
    drive_to_idle();
    hal_mock_set_lid_closed(false);
    lock_tick(free_window(), false, &hal);   // remote out, free window
    int before = hal_mock_ir_call_count();
    lock_tick(free_window(), true, &hal);    // remote returned
    TEST_ASSERT_EQUAL(before + 1, hal_mock_ir_call_count());
}

void test_lock_return_ceremony_plays_audio_once(void) {
    drive_to_violation();
    int before = hal_mock_audio_call_count();
    lock_tick(restricted_window(), true, &hal);
    TEST_ASSERT_EQUAL(before + 1, hal_mock_audio_call_count());
    TEST_ASSERT_EQUAL(CLIP_RETURN, hal_mock_last_audio_clip());
}

// ── REQ-LOCK-008 ─────────────────────────────────────────────────────────────


void test_lock_violation_stage_is_none_outside_violation(void) {
    drive_to_idle();
    TEST_ASSERT_EQUAL(VIOLATION_STAGE_NONE, lock_get_violation_stage());

    drive_to_armed();
    TEST_ASSERT_EQUAL(VIOLATION_STAGE_NONE, lock_get_violation_stage());
}

void test_lock_violation_stage_is_none_immediately_after_violation_starts(void) {
    drive_to_violation();   // violation entered at AFTER_BOOT
    TEST_ASSERT_EQUAL(VIOLATION_STAGE_NONE, lock_get_violation_stage());
}

void test_lock_violation_stage_is_none_before_soft_threshold(void) {
    drive_to_violation();
    hal_mock_set_time(AFTER_BOOT + LOCK_VIOLATION_SOFT_S - 1);  // 29:59 elapsed
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(VIOLATION_STAGE_NONE, lock_get_violation_stage());
}

void test_lock_violation_stage_advances_to_soft_at_threshold(void) {
    drive_to_violation();
    hal_mock_set_time(AFTER_BOOT + LOCK_VIOLATION_SOFT_S);  // exactly 30 min
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(VIOLATION_STAGE_SOFT, lock_get_violation_stage());
}

void test_lock_violation_stage_advances_to_warn_when_window_nearly_over(void) {
    drive_to_violation();
    // restricted_window ends at NOW + 3600 — set time so 5 min remain
    uint32_t five_min_before_end = (NOW + 3600) - LOCK_VIOLATION_WARN_S;
    hal_mock_set_time(five_min_before_end);
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(VIOLATION_STAGE_WARN, lock_get_violation_stage());
}

void test_lock_violation_stage_advances_to_loud_when_window_ends(void) {
    drive_to_violation();
    hal_mock_set_time(NOW + 3600);   // exactly at window end
    // window has ended — schedule_resolve would normally return MODE_FREE,
    // but we keep restricted_window in the test to isolate stage logic
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(VIOLATION_STAGE_LOUD, lock_get_violation_stage());
}

void test_lock_violation_stage_is_monotonic_during_single_violation(void) {
    drive_to_violation();

    // SOFT
    hal_mock_set_time(AFTER_BOOT + LOCK_VIOLATION_SOFT_S);
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(VIOLATION_STAGE_SOFT, lock_get_violation_stage());

    // WARN
    hal_mock_set_time((NOW + 3600) - LOCK_VIOLATION_WARN_S);
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(VIOLATION_STAGE_WARN, lock_get_violation_stage());

    // LOUD
    hal_mock_set_time(NOW + 3600);
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(VIOLATION_STAGE_LOUD, lock_get_violation_stage());
}

void test_lock_violation_stage_resets_on_recovery_to_armed(void) {
    drive_to_violation();
    hal_mock_set_time(AFTER_BOOT + LOCK_VIOLATION_SOFT_S);
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(VIOLATION_STAGE_SOFT, lock_get_violation_stage());

    // remote returned → COOLDOWN → ARMED
    lock_tick(restricted_window(), true, &hal);
    lock_tick(restricted_window(), true, &hal);

    TEST_ASSERT_EQUAL(LOCK_STATE_ARMED, lock_get_state());
    TEST_ASSERT_EQUAL(VIOLATION_STAGE_NONE, lock_get_violation_stage());
}

void test_lock_violation_stage_resets_on_recovery_to_idle(void) {
    drive_to_violation();
    hal_mock_set_time(AFTER_BOOT + LOCK_VIOLATION_SOFT_S);
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(VIOLATION_STAGE_SOFT, lock_get_violation_stage());

    // remote returned during free window → COOLDOWN → IDLE
    lock_tick(restricted_window(), true, &hal);   // COOLDOWN
    lock_tick(free_window(), true, &hal);         // restriction over → IDLE

    TEST_ASSERT_EQUAL(LOCK_STATE_IDLE, lock_get_state());
    TEST_ASSERT_EQUAL(VIOLATION_STAGE_NONE, lock_get_violation_stage());
}

void test_lock_violation_stage_warn_takes_precedence_over_soft(void) {
    // edge case: violation started <30 min ago but window ends in <5 min
    // window-remaining check should win over elapsed-time check
    drive_to_violation();
    uint32_t five_min_before_end = (NOW + 3600) - LOCK_VIOLATION_WARN_S;
    hal_mock_set_time(five_min_before_end);
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(VIOLATION_STAGE_WARN, lock_get_violation_stage());
}

// ── REQ-LOCK-009 ─────────────────────────────────────────────────────────────

void test_lock_idle_to_violation_when_restriction_starts_with_remote_absent(void) {
    drive_to_idle();
    hal_mock_set_lid_closed(true);
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(LOCK_STATE_VIOLATION, lock_get_state());
}

void test_lock_idle_to_armed_when_restriction_starts_remote_absent_lid_open(void) {
    drive_to_idle();
    hal_mock_set_lid_closed(false);
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(LOCK_STATE_ARMED, lock_get_state());
}

void test_lock_idle_to_violation_fires_ir_once(void) {
    drive_to_idle();
    hal_mock_set_lid_closed(true);
    int before = hal_mock_ir_call_count();
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(before + 1, hal_mock_ir_call_count());
}

void test_lock_boot_does_not_violate_with_restriction_and_remote_absent(void) {
    // setUp() leaves us in BOOT with mock_time = NOW
    hal_mock_set_lid_closed(true);
    lock_tick(restricted_window(), false, &hal);
    TEST_ASSERT_EQUAL(LOCK_STATE_BOOT, lock_get_state());
    TEST_ASSERT_EQUAL(0, hal_mock_ir_call_count());
}

// ── Runner ────────────────────────────────────────────────────────────────────

void setUp(void) {
    hal = hal_mock_create();
    hal_mock_reset();
    hal_mock_set_time(NOW);
    lock_init(&hal);
}

void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();

    // ── Initial state ────────────────
    RUN_TEST(test_lock_initial_state_after_init);

    // ── REQ-LOCK-001 ────────────────
    RUN_TEST(test_lock_boot_holds_motor_open);
    RUN_TEST(test_lock_boot_ignores_restricted_window);
    RUN_TEST(test_lock_boot_does_not_transition_before_grace_period);
    RUN_TEST(test_lock_boot_transitions_to_idle_after_grace_period);

    // ── REQ-LOCK-002 ────────────────
    RUN_TEST(test_lock_armed_lid_closed_remote_present_releases_motor);
    RUN_TEST(test_lock_armed_lid_open_holds_motor_open);
    RUN_TEST(test_lock_armed_remote_absent_lid_open_holds_motor_open);
    RUN_TEST(test_lock_armed_lid_closed_but_not_engaged_holds_open);
    RUN_TEST(test_lock_armed_fully_engaged_with_remote_releases_motor);

    // ── REQ-LOCK-003 ────────────────
    RUN_TEST(test_lock_idle_to_armed_on_restriction_start);
    RUN_TEST(test_lock_armed_to_idle_when_restriction_ends);
    RUN_TEST(test_lock_armed_stays_armed_during_restriction);

    // ── REQ-LOCK-004 ────────────────
    RUN_TEST(test_lock_armed_lid_closed_remote_absent_plays_nag_soft);
    RUN_TEST(test_lock_nag_escalates_to_loud_after_30s);
    RUN_TEST(test_lock_nag_stops_when_lid_opens);
    RUN_TEST(test_lock_no_nag_in_free_mode);

    // ── REQ-LOCK-005 ────────────────
    RUN_TEST(test_lock_release_button_instant_in_boot);
    RUN_TEST(test_lock_release_button_instant_in_free_mode);
    RUN_TEST(test_lock_release_button_restricted_hold_insufficient);
    RUN_TEST(test_lock_release_button_restricted_hold_sufficient);
    RUN_TEST(test_lock_release_button_no_effect_in_sleep);
    RUN_TEST(test_lock_release_button_release_resets_timer);

    // ── REQ-LOCK-006 ────────────────
    RUN_TEST(test_lock_armed_to_violation_on_remote_removed_lid_closed);
    RUN_TEST(test_lock_violation_no_repeat_ir);
    RUN_TEST(test_lock_armed_lid_open_remote_absent_no_violation);

    // ── REQ-LOCK-007 ────────────────
    RUN_TEST(test_lock_violation_to_cooldown_on_remote_return);
    RUN_TEST(test_lock_cooldown_to_armed_if_restriction_active);
    RUN_TEST(test_lock_cooldown_to_idle_if_restriction_ended);
    RUN_TEST(test_lock_ir_fires_on_remote_return_in_free_mode);
    RUN_TEST(test_lock_return_ceremony_plays_audio_once);

    // ── REQ-LOCK-008 ────────────────

    RUN_TEST(test_lock_violation_stage_is_none_outside_violation);
    RUN_TEST(test_lock_violation_stage_is_none_immediately_after_violation_starts);
    RUN_TEST(test_lock_violation_stage_is_none_before_soft_threshold);
    RUN_TEST(test_lock_violation_stage_advances_to_soft_at_threshold);
    RUN_TEST(test_lock_violation_stage_advances_to_warn_when_window_nearly_over);
    RUN_TEST(test_lock_violation_stage_advances_to_loud_when_window_ends);
    RUN_TEST(test_lock_violation_stage_is_monotonic_during_single_violation);
    RUN_TEST(test_lock_violation_stage_resets_on_recovery_to_armed);
    RUN_TEST(test_lock_violation_stage_resets_on_recovery_to_idle);
    RUN_TEST(test_lock_violation_stage_warn_takes_precedence_over_soft);

    // ── REQ-LOCK-009 ────────────────
    RUN_TEST(test_lock_idle_to_violation_when_restriction_starts_with_remote_absent);
    RUN_TEST(test_lock_idle_to_armed_when_restriction_starts_remote_absent_lid_open);
    RUN_TEST(test_lock_idle_to_violation_fires_ir_once);
    RUN_TEST(test_lock_boot_does_not_violate_with_restriction_and_remote_absent);

    UNITY_END();
    return 0;
}