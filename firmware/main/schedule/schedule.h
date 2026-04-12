#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdint.h>
#include <stdbool.h>

// ── Schedule mode ─────────────────────────────────────────────────────────────

typedef enum {
    MODE_FREE,          // remote may be removed, no enforcement
    MODE_RESTRICTED,    // remote must remain in box
    MODE_DEEP_FOCUS,    // restricted + longer iPad hold duration
    MODE_SLEEP,         // strict restriction, no overrides
} ScheduleMode;

// ── Schedule event ────────────────────────────────────────────────────────────

typedef struct {
    uint32_t     start_unix;      // unix timestamp: when restriction begins
    uint32_t     end_unix;        // unix timestamp: when restriction ends
    uint32_t     modified_unix;   // unix timestamp: when event was last modified
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
 * REQ-SCHED-001
 * Returns true if the event passes the 2-day rule and may be accepted
 * into the local schedule cache. Returns false if the event was created
 * or modified less than 48 hours before its start time.
 */
bool schedule_accept_event(const ScheduleEvent *ev, uint32_t now);

/**
 * REQ-SCHED-002
 * Given an array of accepted events and the current unix time, returns
 * the active ScheduleWindow. If no event covers now, returns a window
 * with mode MODE_FREE and valid = false.
 */
ScheduleWindow schedule_resolve(const ScheduleEvent *events,
                                uint8_t              count,
                                uint32_t             now);

#endif // SCHEDULE_H