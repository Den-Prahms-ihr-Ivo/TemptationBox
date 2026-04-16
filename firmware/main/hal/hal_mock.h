// hal_mock.h
#ifndef HAL_MOCK_H
#define HAL_MOCK_H

#include "hal.h"

// factory
HAL  hal_mock_create(void);

// setters — control mock state from tests
void hal_mock_set_time(uint32_t t);
void hal_mock_set_rfid(bool present);
void hal_mock_set_weight(bool present);
void hal_mock_nvs_clear(void);

// getters — assert on what the mock observed
bool      hal_mock_lock_is_armed(void);
AudioClip hal_mock_last_audio_clip(void);
int       hal_mock_audio_call_count(void);
int       hal_mock_ir_call_count(void);

#endif