#include <stddef.h>  
#include "schedule.h"


ScheduleWindow schedule_resolve(const ScheduleEvent *events,
                                uint8_t              count,
                                uint32_t             now) {

    const ScheduleEvent *ev = events;
    ScheduleMode current_best = MODE_FREE;
    bool validity = false;
    uint32_t current_until = 0;

    if (events == NULL || count == 0) {
        return (ScheduleWindow){ .mode = MODE_FREE, .valid = false };
    }

    for (int i = 0; i < count; i++, ev++) {
        if (now >= ev->start_unix && now < ev->end_unix && ev->mode > current_best) {
            current_best = ev->mode; 
            validity = true;
            current_until = ev->end_unix;
        }
    }

    return (ScheduleWindow){ 
        .mode = current_best,
        .until_unix = current_until,
        .valid = validity 
    };
}

uint32_t schedule_ipad_hold_ms(ScheduleMode mode) {
    switch (mode) {
        case MODE_FREE:        return 0;
        case MODE_PERMITTED:   return 0;
        case MODE_RESTRICTED:  return 20000;
        case MODE_DEEP_FOCUS:  return 60000;
        case MODE_SLEEP:       return UINT32_MAX;
        default:               return UINT32_MAX;  // fail safe — unknown mode = no access
    }
}

ReleaseBehaviour schedule_release_behaviour(ScheduleMode mode) {
    return RELEASE_DENIED;
}

