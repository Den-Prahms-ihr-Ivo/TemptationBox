#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdint.h>
#include <stdbool.h>

// ── Schedule mode ─────────────────────────────────────────────────────────────
// Shared type — referenced by REQ-SCHED-002, REQ-SCHED-004, REQ-SCHED-005

typedef enum {
    MODE_FREE       = 0,  // remote may be removed, no enforcement
    MODE_PERMITTED  = 1,   // scheduled TV window — active but signalled differently
    MODE_RESTRICTED = 2,  // remote must remain in box
    MODE_DEEP_FOCUS = 3,  // restricted + longer iPad hold duration
    MODE_SLEEP      = 4,  // strict restriction, no overrides
} ScheduleMode;

/**
 * REQ-SCHED-005
 * Describes how the release button should behave for a given
 * ScheduleMode. The schedule module owns this policy so that all
 * mode-based behavioural decisions live in one place.
 *
 * RELEASE_INSTANT  — a single press releases the lock
 * RELEASE_HOLD     — the button must be held for N seconds, where
 *                    N is obtained from schedule_ipad_hold_ms()
 *                    applied to the release button — the same
 *                    friction model as the iPad slot
 * RELEASE_DENIED   — the button has no effect, used for MODE_SLEEP
 */
typedef enum {
    RELEASE_INSTANT,    // press once
    RELEASE_HOLD,       // hold for N seconds
    RELEASE_DENIED,     // button has no effect
} ReleaseBehaviour;

// ── Schedule event ────────────────────────────────────────────────────────────
// REQ-SCHED-002

typedef struct {
    uint32_t     start_unix;      // unix timestamp: when restriction begins
    uint32_t     end_unix;        // unix timestamp: when restriction ends
    ScheduleMode mode;
} ScheduleEvent;

// ── Active window ─────────────────────────────────────────────────────────────
// REQ-SCHED-002
// Resolved view of what is happening right now.
// The schedule engine produces this from the event list.

typedef struct {
    ScheduleMode mode;
    uint32_t     until_unix;      // when the current mode ends
    bool         valid;           // false if no event covers the current time
} ScheduleWindow;


// ── API ───────────────────────────────────────────────────────────────────────

/**
 * REQ-SCHED-005
 * Returns the release button behaviour for the given mode.
 *
 *   MODE_FREE       → RELEASE_INSTANT
 *   MODE_PERMITTED  → RELEASE_INSTANT
 *   MODE_RESTRICTED → RELEASE_HOLD
 *   MODE_DEEP_FOCUS → RELEASE_HOLD
 *   MODE_SLEEP      → RELEASE_DENIED
 *
 * @param mode  The current active ScheduleMode.
 * @return      The required button interaction for release.
 *
 * @note Pure function — no side effects.
 */
ReleaseBehaviour schedule_release_behaviour(ScheduleMode mode);


/**
 * REQ-SCHED-002
 * Resolves the active schedule window for the current moment.
 *
 * Iterates all events and finds those where now falls within
 * [start_unix, end_unix). If multiple events overlap, the one
 * with the highest ScheduleMode value wins. If no event covers
 * now, returns a window with mode=MODE_FREE and valid=false.
 *
 * @param events  Pointer to array of ScheduleEvent. May be NULL.
 * @param count   Number of events in the array.
 * @param now     Current unix timestamp (from hal->time_now_unix()).
 * @return        ScheduleWindow describing the active window.
 *
 * @note Pure function — no side effects, no hardware access.
 *       Safe to call on every logic tick.
 */
ScheduleWindow schedule_resolve(const ScheduleEvent *events,
                                uint8_t              count,
                                uint32_t             now);

/**
 * REQ-SCHED-004
 * Returns the required iPad slot hold duration in milliseconds
 * for the given mode. Returns UINT32_MAX for MODE_SLEEP —
 * callers must treat this as "no access permitted".
 *
 * @param mode  The current active ScheduleMode.
 * @return      Hold duration in milliseconds.
 *
 * @note No side effects.
 */
uint32_t schedule_ipad_hold_ms(ScheduleMode mode);

#endif // SCHEDULE_H