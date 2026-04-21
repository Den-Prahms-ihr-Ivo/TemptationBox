#include "presence.h"
#include "hal.h"

// ── Internal state ────────────────────────────────────────────────────────────

static struct {
    // remote
    uint8_t present_count;
    uint8_t absent_count;
    bool    is_present;
    // ipad
    uint8_t ipad_present_count;
    uint8_t ipad_absent_count;
    bool    ipad_is_present;
} ctx;

// ── Internal helpers ──────────────────────────────────────────────────────────

static void update_channel(bool raw_present,
                            uint8_t *present_count,
                            uint8_t *absent_count,
                            bool    *state) {
    if (raw_present) {
        *absent_count = 0;
        if (*present_count < PRESENCE_DEBOUNCE_PRESENT_SAMPLES) {
            (*present_count)++;
        }
        if (*present_count >= PRESENCE_DEBOUNCE_PRESENT_SAMPLES) {
            *state = true;
        }
    } else {
        *present_count = 0;
        if (*absent_count < PRESENCE_DEBOUNCE_ABSENT_SAMPLES) {
            (*absent_count)++;
        }
        if (*absent_count >= PRESENCE_DEBOUNCE_ABSENT_SAMPLES) {
            *state = false;
        }
    }
}

// ── Public API ────────────────────────────────────────────────────────────────

void presence_init(void) {
    ctx.present_count      = 0;
    ctx.absent_count       = 0;
    ctx.is_present         = false;
    ctx.ipad_present_count = 0;
    ctx.ipad_absent_count  = 0;
    ctx.ipad_is_present    = false;
}

void presence_tick(const HAL *hal) {
    bool raw_remote = hal->rfid_present() && hal->weight_present();
    bool raw_ipad   = hal->ipad_slot_present();

    update_channel(raw_remote,
                   &ctx.present_count,
                   &ctx.absent_count,
                   &ctx.is_present);

    update_channel(raw_ipad,
                   &ctx.ipad_present_count,
                   &ctx.ipad_absent_count,
                   &ctx.ipad_is_present);
}

bool presence_is_remote_present(void) {
    return ctx.is_present;
}

bool presence_is_ipad_present(void) {
    return ctx.ipad_is_present;
}