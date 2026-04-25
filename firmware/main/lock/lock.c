#include "lock.h"
#include "../audio/audio.h"
#include "../schedule/schedule.h"

// ── Internal state ────────────────────────────────────────────────────────────

static LockState      s_state;
static uint32_t       s_boot_time;

static uint32_t       s_violation_time;     // unix timestamp when violation began
static uint32_t       s_window_until;       // cached window end for stage computation
static ViolationStage s_violation_stage;    // monotonically non-decreasing within violation

static bool           s_button_was_pressed; // for hold-duration tracking
static uint32_t       s_button_press_time;

static bool           s_remote_was_present; // edge detection for IR-on-return

// ── Helpers ───────────────────────────────────────────────────────────────────

static bool mode_is_restricted(ScheduleMode mode) {
    return mode != MODE_FREE && mode != MODE_PERMITTED;
}

static ViolationStage compute_stage(uint32_t now) {
    if (now >= s_window_until)
        return VIOLATION_STAGE_LOUD;
    if ((s_window_until - now) <= LOCK_VIOLATION_WARN_S)
        return VIOLATION_STAGE_WARN;
    if ((now - s_violation_time) >= LOCK_VIOLATION_SOFT_S)
        return VIOLATION_STAGE_SOFT;
    return VIOLATION_STAGE_NONE;
}

static void enter_violation(uint32_t now, uint32_t window_until, const HAL *hal) {
    s_violation_time  = now;
    s_window_until    = window_until;
    s_violation_stage = VIOLATION_STAGE_NONE;
    s_state           = LOCK_STATE_VIOLATION;
    hal->ir_send_tv_off();
    hal->play_audio(CLIP_NAG_SOFT);
    hal->lock_hold_open(true);
}

// ── Public API ────────────────────────────────────────────────────────────────

void lock_init(const HAL *hal) {
    s_state              = LOCK_STATE_BOOT;
    s_boot_time          = hal->time_now_unix();
    s_violation_time     = 0;
    s_window_until       = 0;
    s_violation_stage    = VIOLATION_STAGE_NONE;
    s_button_was_pressed = false;
    s_button_press_time  = 0;
    s_remote_was_present = false;
}

void lock_tick(ScheduleWindow window, bool remote_present, const HAL *hal) {
    uint32_t now = hal->time_now_unix();

    // ── IR fires on every remote return, regardless of state (REQ-LOCK-007) ──
    if (remote_present && !s_remote_was_present) {
        hal->ir_send_tv_off();
    }

    switch (s_state) {

    // ── BOOT ─────────────────────────────────────────────────────────────────
    // Motor held open. Schedule and presence ignored. (REQ-LOCK-001)
    case LOCK_STATE_BOOT:
        hal->lock_hold_open(true);
        if ((now - s_boot_time) >= LOCK_BOOT_GRACE_S) {
            s_state = LOCK_STATE_IDLE;
        }
        break;

    // ── IDLE ─────────────────────────────────────────────────────────────────
    // Free window. Motor held open. Transitions to ARMED or VIOLATION on
    // restriction start. (REQ-LOCK-003, REQ-LOCK-009)
    case LOCK_STATE_IDLE:
        hal->lock_hold_open(true);
        if (window.valid && mode_is_restricted(window.mode)) {
            if (hal->lid_is_closed() && !remote_present) {
                enter_violation(now, window.until_unix, hal);
            } else {
                s_state = LOCK_STATE_ARMED;
            }
        }
        break;

    // ── ARMED ────────────────────────────────────────────────────────────────
    // Restriction active. Motor consent-based. (REQ-LOCK-002, REQ-LOCK-003,
    // REQ-LOCK-005, REQ-LOCK-006)
    case LOCK_STATE_ARMED: {
        bool lid     = hal->lid_is_closed();
        bool engaged = hal->lock_is_engaged();
        bool btn     = hal->release_button_pressed();
        ReleaseBehaviour behaviour = schedule_release_behaviour(window.mode);

        // Restriction ended → IDLE
        if (!window.valid || !mode_is_restricted(window.mode)) {
            s_state = LOCK_STATE_IDLE;
            hal->lock_hold_open(true);
            break;
        }

        // Violation: lid closed + remote absent (REQ-LOCK-006)
        if (lid && !remote_present) {
            enter_violation(now, window.until_unix, hal);
            break;
        }

        // Release button (REQ-LOCK-005)
        if (btn && behaviour != RELEASE_DENIED) {
            if (behaviour == RELEASE_INSTANT) {
                s_button_was_pressed = false;
                hal->lock_hold_open(true);
                break;
            }
            // RELEASE_HOLD: track how long the button has been held
            if (!s_button_was_pressed) {
                s_button_was_pressed = true;
                s_button_press_time  = now;
            }
            uint32_t hold_s = schedule_ipad_hold_ms(window.mode) / 1000;
            if ((now - s_button_press_time) >= hold_s) {
                s_button_was_pressed = false;
                hal->lock_hold_open(true);
                break;
            }
        } else {
            s_button_was_pressed = false;
        }

        // Consent: release motor only when lid is closed, latch is caught,
        // and remote is inside. (REQ-LOCK-002)
        if (lid && engaged && remote_present) {
            hal->lock_hold_open(false);
        } else {
            hal->lock_hold_open(true);
        }
        break;
    }

    // ── VIOLATION ────────────────────────────────────────────────────────────
    // Remote removed while locked. Escalating audio. (REQ-LOCK-004,
    // REQ-LOCK-007, REQ-LOCK-008)
    case LOCK_STATE_VIOLATION: {
        // Remote returned → COOLDOWN
        if (remote_present) {
            s_state = LOCK_STATE_COOLDOWN;
            hal->play_audio(CLIP_RETURN);
            hal->lock_hold_open(true);
            break;
        }

        hal->lock_hold_open(true);

        // Nag audio stops when the lid is open
        if (!hal->lid_is_closed()) break;

        // Advance stage monotonically and play audio on each escalation
        ViolationStage new_stage = compute_stage(now);
        if (new_stage > s_violation_stage) {
            s_violation_stage = new_stage;
            hal->play_audio(CLIP_NAG_LOUD);
        }
        break;
    }

    // ── COOLDOWN ─────────────────────────────────────────────────────────────
    // Remote returned. One-tick ceremony state, then back to ARMED or IDLE.
    // (REQ-LOCK-007)
    case LOCK_STATE_COOLDOWN:
        s_violation_stage = VIOLATION_STAGE_NONE;
        hal->lock_hold_open(true);
        if (window.valid && mode_is_restricted(window.mode)) {
            s_state = LOCK_STATE_ARMED;
        } else {
            s_state = LOCK_STATE_IDLE;
        }
        break;
    }

    s_remote_was_present = remote_present;
}

LockState lock_get_state(void) {
    return s_state;
}

ViolationStage lock_get_violation_stage(void) {
    if (s_state != LOCK_STATE_VIOLATION) return VIOLATION_STAGE_NONE;
    return s_violation_stage;
}
