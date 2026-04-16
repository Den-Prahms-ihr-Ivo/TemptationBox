#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>
#include <stdbool.h>

#include "../schedule/schedule.h"
#include "../hal/hal.h"

#define CACHE_MAX_EVENTS 32
#define CACHE_STALE_THRESHOLD_S (2 * 60 * 60)   // 2 hours
#define NET_SYNC_INTERVAL_MS  (15 * 60 * 1000)  // 15 minutes

typedef struct {
    uint32_t      generated_at;              // backend timestamp
    ScheduleEvent events[CACHE_MAX_EVENTS];  // the event list
    uint8_t       count;                     // how many are valid
    bool          is_stale;                  // set true if last fetch failed
} ScheduleCache;


/**
 * Stores a freshly fetched schedule. Clears is_stale flag.
 * Persists to NVS via HAL.
 */
void cache_store(const HAL *hal, const ScheduleCache *cache);

/**
 * Returns the current cached schedule.
 * If nothing is stored, returns an empty cache with is_stale=true.
 * If stored data is older than CACHE_STALE_THRESHOLD_S, sets is_stale=true.
 *
 * @param hal  HAL pointer for NVS access and current time.
 * @return     ScheduleCache — always valid to use, never crashes.
 */
ScheduleCache cache_get(const HAL *hal);

#endif // CACHE_H