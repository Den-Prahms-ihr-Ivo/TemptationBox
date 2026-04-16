#include "hal.h"
#include <string.h>

// ── NVS ───────────────────────────────────────────────────────────────────────
static uint8_t mock_nvs_buf[4096];
static bool    mock_nvs_has_data = false;

static void mock_nvs_write(const uint8_t *data, size_t len) {
    memcpy(mock_nvs_buf, data, len);
    mock_nvs_has_data = true;
}

static bool mock_nvs_read(uint8_t *out, size_t len) {
    if (!mock_nvs_has_data) return false;
    memcpy(out, mock_nvs_buf, len);
    return true;
}

// ── Time ──────────────────────────────────────────────────────────────────────
static uint32_t mock_time = 0;

static uint32_t mock_time_now_unix(void) {
    return mock_time;
}

// ── RFID ──────────────────────────────────────────────────────────────────────
static bool mock_rfid_state = false;

static bool mock_rfid_present(void) {
    return mock_rfid_state;
}

// ── Weight ────────────────────────────────────────────────────────────────────
static bool mock_weight_state = false;

static bool mock_weight_present(void) {
    return mock_weight_state;
}

// ── Lock ──────────────────────────────────────────────────────────────────────
static bool mock_lock_armed = false;

static void mock_lock_arm(void) {
    mock_lock_armed = true;
}

static void mock_lock_release(void) {
    mock_lock_armed = false;
}

// ── Audio ─────────────────────────────────────────────────────────────────────
static AudioClip mock_last_clip = 0;
static int       mock_audio_call_count = 0;

static void mock_play_audio(AudioClip clip) {
    mock_last_clip = clip;
    mock_audio_call_count++;
}

// ── IR ────────────────────────────────────────────────────────────────────────
static int mock_ir_call_count = 0;

static void mock_ir_send_tv_off(void) {
    mock_ir_call_count++;
}

// ── LED ───────────────────────────────────────────────────────────────────────
static void mock_led_set(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    (void)idx; (void)r; (void)g; (void)b;  // no-op for now
}

// ── Public: HAL factory ───────────────────────────────────────────────────────
HAL hal_mock_create(void) {
    return (HAL){
        .rfid_present    = mock_rfid_present,
        .weight_present  = mock_weight_present,
        .lock_arm        = mock_lock_arm,
        .lock_release    = mock_lock_release,
        .led_set         = mock_led_set,
        .time_now_unix   = mock_time_now_unix,
        .play_audio      = mock_play_audio,
        .ir_send_tv_off  = mock_ir_send_tv_off,
        .nvs_write       = mock_nvs_write,
        .nvs_read        = mock_nvs_read,
    };
}

// ── Public: setters for test control ─────────────────────────────────────────
void hal_mock_set_time(uint32_t t)        { mock_time         = t; }
void hal_mock_set_rfid(bool present)      { mock_rfid_state   = present; }
void hal_mock_set_weight(bool present)    { mock_weight_state = present; }
void hal_mock_nvs_clear(void)             { mock_nvs_has_data = false; }

// ── Public: getters for test assertions ───────────────────────────────────────
bool         hal_mock_lock_is_armed(void)      { return mock_lock_armed; }
AudioClip    hal_mock_last_audio_clip(void)    { return mock_last_clip; }
int          hal_mock_audio_call_count(void)   { return mock_audio_call_count; }
int          hal_mock_ir_call_count(void)      { return mock_ir_call_count; }