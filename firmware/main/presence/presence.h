#pragma once
/**
 * The HAL calls you'll need — hal->rfid_present(), hal->weight_present(), and a new one you'll need to add to hal.h and hal_mock.c:
 */

#include <stdint.h>
#include <stdbool.h>

#include "../hal/hal.h"

#define PRESENCE_DEBOUNCE_ABSENT_SAMPLES  3   // ticks to confirm removal
#define PRESENCE_DEBOUNCE_PRESENT_SAMPLES 2   // ticks to confirm insertion


/** Resets all internal state. Call once on boot and in setUp(). */
void presence_init(void);

/** Call once per logic tick. Reads HAL, updates internal state. */
void presence_tick(const HAL *hal);

/** Returns current debounced remote presence state. 
 * It merges the RFID and weight sensor into a single 
 * presence state of the remote.
 */
bool presence_is_remote_present(void);

/** Returns current debounced iPad slot state. */
bool presence_is_ipad_present(void);
