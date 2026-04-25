#pragma once

#include "../hal/hal.h"
#include "../schedule/schedule.h"

/**
 * Shared type — REQ-LOCK-001 through REQ-LOCK-008
 * States of the lock state machine.
 *
 * LOCK_STATE_BOOT      — startup grace window; lock held open, all
 *                         restrictions deferred. Schedule and presence
 *                         inputs are ignored. Exits to IDLE after
 *                         LOCK_BOOT_GRACE_S seconds.
 * LOCK_STATE_IDLE      — free window or permitted window; no
 *                         enforcement, lock held open.
 * LOCK_STATE_ARMED     — restricted window active; lock engages only
 *                         when the user physically closes the lid
 *                         with the remote inside. Consent-based —
 *                         the lock is never forced shut automatically.
 * LOCK_STATE_VIOLATION — remote removed during ARMED while lid closed.
 *                         IR off signal fires, audio escalates over
 *                         time until the remote is returned.
 * LOCK_STATE_COOLDOWN  — remote returned during violation. Plays the
 *                         return ceremony, then transitions to ARMED
 *                         if still restricted or IDLE if not.
 */
typedef enum {
    LOCK_STATE_BOOT,    
    LOCK_STATE_IDLE,
    LOCK_STATE_ARMED,
    LOCK_STATE_VIOLATION,
    LOCK_STATE_COOLDOWN,
} LockState;

/**
 * Escalation stages during a VIOLATION state.
 *
 * The lock module owns this concept because it owns the timing —
 * other modules (audio, display, future logging) react to the
 * current stage rather than computing escalation themselves.
 *
 * Stages are monotonically non-decreasing during a single
 * violation. Returning to ARMED or IDLE resets the stage to NONE.
 *
 * REQ-LOCK-008
 */
typedef enum {
    VIOLATION_STAGE_NONE = 0,   // not in violation, or just started
    VIOLATION_STAGE_SOFT,       // 30+ minutes elapsed since violation began
    VIOLATION_STAGE_WARN,       // 5 minutes or fewer remain in the window
    VIOLATION_STAGE_LOUD,       // restriction window has ended
} ViolationStage;

/**
 * Time elapsed since violation started before the SOFT stage activates.
 * Tunable — changing this constant updates all dependent behaviour
 * across consumer modules without touching their code.
 *
 * REQ-LOCK-008
 */
#define LOCK_VIOLATION_SOFT_S  (30 * 60)

/**
 * Time remaining in the restriction window at which the WARN stage
 * activates. Once the window has fewer than this many seconds left,
 * the stage escalates from SOFT to WARN.
 *
 * REQ-LOCK-008
 */
#define LOCK_VIOLATION_WARN_S  (5 * 60)



/**
 * REQ-LOCK-001
 * Duration of the startup grace period in seconds.
 * During this window the motor holds the lock open and no
 * restrictions are enforced, giving task_net_sync time to fetch
 * the current schedule before committing to any action.
 */
#define LOCK_BOOT_GRACE_S  (5 * 60)

/**
 * REQ-LOCK-001
 * Resets the lock state machine. Records current time as the boot
 * timestamp and enters LOCK_STATE_BOOT. Must be called once on
 * startup before the first tick.
 *
 * @param hal  HAL pointer for reading the current time.
 */
void lock_init(const HAL *hal);

/**
 * REQ-LOCK-001 through REQ-LOCK-009
 * Advances the lock state machine by one tick. Called from
 * task_logic every 500ms. Reads required inputs directly through
 * the HAL (lid, engagement, release button, time) and drives all
 * outputs (motor, audio, IR, LEDs) via HAL calls.
 *
 * The state machine is the only module that commands hardware
 * actions in response to schedule and presence changes — other
 * modules only report state.
 *
 * @param window          Current resolved schedule window.
 * @param remote_present  Debounced remote presence state from
 *                         the presence module.
 * @param hal             HAL pointer for all I/O and time.
 *
 * @note During LOCK_STATE_BOOT, window and remote_present are
 *        intentionally ignored — see ADR-022.
 */
void lock_tick(ScheduleWindow window, bool remote_present, const HAL *hal);

/**
 * Shared — used by display module and tests to observe state.
 * Returns the current lock state.
 */
LockState lock_get_state(void);

/**
 * REQ-LOCK-008
 * Returns the current violation stage based on elapsed time since
 * the violation began and time remaining in the restriction window.
 *
 * Returns VIOLATION_STAGE_NONE when the system is not in
 * LOCK_STATE_VIOLATION. Used by audio, display, and any future
 * module that needs to react to escalation.
 *
 * @return The current violation stage. Safe to call in any state.
 *
 * @note Pure read of internal state — no side effects.
 *       Stage advancement happens during lock_tick().
 *
 */
ViolationStage lock_get_violation_stage(void);