# Latch State Machine, REQ-LOCK-XXX

## REQ-LOCK-001

**Status:** TODO

**Title:** Boot grace period defers all enforcement

**Description:**

After power-on the system spends
LOCK_BOOT_GRACE_S in LOCK_STATE_BOOT during which the motor
holds the lock open and no restrictions are enforced. After
the grace period expires the state machine transitions to
IDLE and normal logic begins.

**Acceptance criteria:**

- t=0 → BOOT, lock_hold_open(true) called
- t < LOCK_BOOT_GRACE_S → stays BOOT regardless of inputs
- t >= LOCK_BOOT_GRACE_S → transitions to IDLE, lock_hold_open(false)
- Schedule and presence inputs are ignored during BOOT

---

## REQ-LOCK-002

**Status:** TODO

**Title:** ARMED is a consent-based state, not a physical lock

**Description:**

In ARMED state the motor releases only when
both the lid is closed AND the remote is present. If either is
false, the motor holds the lock open to prevent deadlock. The
lock mechanically engages when the user physically closes the lid.

**Acceptance criteria:**

- ARMED + lid closed + remote present → lock_hold_open(false)
- ARMED + lid open → lock_hold_open(true)
- ARMED + remote absent → lock_hold_open(true)
- Box opening is never automatic — the click incentive is
  eliminated by design

---

## REQ-LOCK-003

**Status:** TODO

**Title:** Transition to ARMED happens on restriction start

**Description:**

When schedule transitions from MODE_FREE to
a restricted mode, state moves from IDLE to ARMED. Audio and
LED hand-offs happen via display module (outside this scope).
No immediate enforcement — the user must physically close the
lid with the remote inside for the lock to engage.

**Acceptance criteria:**

- IDLE + restricted mode starts → ARMED
- ARMED + restriction ends → IDLE, lock_hold_open(true)
- ARMED + same restriction continues → stays ARMED

---

## REQ-LOCK-004

**Status:** TODO

**Title:** Closed lid without remote triggers escalating audio

**Description:**

If the user closes the lid while the remote
is not inside during a restricted mode, the system plays
escalating audio warnings until they reopen the box. Audio
does not play in MODE_FREE even with this condition.

**Acceptance criteria:**

- ARMED + lid closed + remote absent → play_audio(CLIP_NAG_SOFT)
- Condition persists >30s → play_audio(CLIP_NAG_LOUD)
- Lid reopens → nag audio stops
- MODE_FREE + lid closed + remote absent → no audio

---

## REQ-LOCK-005

**Status:** TODO

**Title:** Release button behaviour scales with mode

**Description:**

The release button triggers lock_hold_open
based on the current mode's ReleaseBehaviour. The hold duration
matches schedule_ipad_hold_ms() to keep the friction model
consistent between the remote and iPad decisions.

**Acceptance criteria:**

- MODE_FREE → instant press releases
- MODE_PERMITTED → instant press releases
- MODE_RESTRICTED → hold 20s releases
- MODE_DEEP_FOCUS → hold 60s releases
- MODE_SLEEP → button has no effect
- BOOT state → instant press releases

---

# REQ-LOCK-006

**Status:** TODO

**Title:** ARMED to VIOLATION when remote is removed

**Description:**

When the remote is removed during ARMED
while the lid is closed, the system transitions to VIOLATION.
ir_send_tv_off() fires once on the transition tick.

**Acceptance criteria:**

- ARMED + lid closed + remote removed → VIOLATION, IR fires once
- VIOLATION + remote still absent → stays VIOLATION, no repeat IR
- Lid open during ARMED is not a violation — it is a neutral state

---

# REQ-LOCK-007

**Status:** TODO

**Title:** VIOLATION to COOLDOWN on remote return

**Description:**

When the remote returns during VIOLATION,
the system transitions to COOLDOWN and plays the return
ceremony. IR off signal fires on every remote return in any mode.

---

# REQ-LOCK-008

**Status:** TODO

**Title:** Violation audio escalates by elapsed time

**Description:**

Previously REQ-LOCK-004, now with BOOT and
consent logic clarified. Escalation starts only after the user
has had reasonable time to comply.
