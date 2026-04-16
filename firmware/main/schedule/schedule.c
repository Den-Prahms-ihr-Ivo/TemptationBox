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
        if (now >= ev->start_unix && now < ev->end_unix) {
            current_best = ev->mode > current_best ? ev->mode : current_best; 
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