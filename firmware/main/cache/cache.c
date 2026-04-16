
#include "cache.h"  

void cache_store(const HAL *hal, const ScheduleCache *cache) {
    //hal->nvs_write((const uint8_t *)cache, sizeof(ScheduleCache));
}

ScheduleCache cache_get(const HAL *hal) {
    ScheduleCache c = {0};
    bool found = hal->nvs_read((uint8_t *)&c, sizeof(ScheduleCache));
     if (!found) {
         c.is_stale = true;
         return c;
     }
    uint32_t age = hal->time_now_unix() - c.generated_at;
    c.is_stale = (age > CACHE_STALE_THRESHOLD_S);
    return c;
}