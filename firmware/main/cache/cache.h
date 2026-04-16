// cache.h
#include <stdint.h>
#include <stdbool.h>

#include "../schedule/schedule.h"

#define CACHE_MAX_EVENTS 32   // upper bound — tune to your NVS budget

typedef struct {
    uint32_t     generated_at;              // backend timestamp
    ScheduleEvent events[CACHE_MAX_EVENTS]; // the event list
    uint8_t      count;                     // how many are valid
    bool         is_stale;                  // set true if last fetch failed
} ScheduleCache;

// called by task_net_sync after a successful fetch
void cache_write(const ScheduleCache *cache);

// called by schedule_resolve — reads from NVS, returns current cache
ScheduleCache cache_read(void);