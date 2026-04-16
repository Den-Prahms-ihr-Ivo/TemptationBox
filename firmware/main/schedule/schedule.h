#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdint.h>
#include <stdbool.h>

// ── Schedule mode ─────────────────────────────────────────────────────────────

typedef enum {
    MODE_FREE       = 0,  // remote may be removed, no enforcement
    MODE_RESTRICTED = 1,  // remote must remain in box
    MODE_DEEP_FOCUS = 2,  // restricted + longer iPad hold duration
    MODE_SLEEP      = 3,  // strict restriction, no overrides
} ScheduleMode;

// ── Schedule event ────────────────────────────────────────────────────────────

typedef struct {
    uint32_t     start_unix;      // unix timestamp: when restriction begins
    uint32_t     end_unix;        // unix timestamp: when restriction ends
    ScheduleMode mode;
} ScheduleEvent;

// ── Active window ─────────────────────────────────────────────────────────────
// Resolved view of what is happening right now.
// The schedule engine produces this from the event list.

typedef struct {
    ScheduleMode mode;
    uint32_t     until_unix;      // when the current mode ends
    bool         valid;           // false if no event covers the current time
} ScheduleWindow;

// ── API ───────────────────────────────────────────────────────────────────────

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