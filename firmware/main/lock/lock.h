#pragma once

typedef enum {
    LOCK_STATE_IDLE,
    LOCK_STATE_ARMED,
    LOCK_STATE_VIOLATION,
    LOCK_STATE_COOLDOWN,
} LockState;