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
 * REQ-LOCK-001 through REQ-LOCK-008
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