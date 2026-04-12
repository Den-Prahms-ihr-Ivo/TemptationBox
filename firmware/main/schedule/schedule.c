#include "schedule.h"

bool schedule_accept_event(const ScheduleEvent *ev, uint32_t now) {
    return true;   // stub — deliberately wrong, tests should fail
}

ScheduleWindow schedule_resolve(const ScheduleEvent *events,
                                uint8_t              count,
                                uint32_t             now) {
    return (ScheduleWindow){ .mode = MODE_FREE, .valid = false };   // stub
}