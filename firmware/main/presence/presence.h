#pragma once
/**
 * The HAL calls you'll need — hal->rfid_present(), hal->weight_present(), and a new one you'll need to add to hal.h and hal_mock.c:
 */

#include <stdint.h>

#define PRESENCE_DEBOUNCE_SAMPLES 3

typedef enum {
    PRESENCE_ABSENT,
    PRESENCE_PRESENT,
} PresenceState;

typedef enum {
    IPAD_ABSENT,
    IPAD_DOCKED,
} IpadState;

typedef struct {
    uint8_t      absent_count;
    PresenceState state;
    uint8_t      ipad_absent_count;
    IpadState    ipad_state;
} PresenceContext;