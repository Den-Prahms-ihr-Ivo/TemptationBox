# Latch State Machine, REQ-LOCK-XXX

## REQ-LOCK-001 : IDLE transitions to ARMED when restriction starts and remote is present

When schedule_resolve returns a restricted mode
and the remote is present, the lock transitions from IDLE to ARMED.
lock_arm() is called exactly once on the transition tick.
If the remote is absent when restriction starts, the system
transitions directly to VIOLATION instead.

### Acceptance criteria:

- IDLE + restriction starts + remote present → ARMED, lock_arm() called once
- IDLE + restriction starts + remote absent → VIOLATION, lock_arm() not called
- IDLE + MODE_FREE → stays IDLE, lock_arm() not called
- ARMED + same restriction continues → stays ARMED, lock_arm() not called again

### Tests:

- test_lock.c::test_lock_idle_to_armed_remote_present()
- test_lock.c::test_lock_idle_to_violation_when_remote_absent()
- test_lock.c::test_lock_stays_idle_during_free_window()
- test_lock.c::test_lock_stays_armed_on_continued_restriction()

---

## REQ-LOCK-002 : ARMED transitions to VIOLATION when remote is removed

When the remote is removed during ARMED state,
the system transitions to VIOLATION. ir_send_tv_off() is called
exactly once on the transition tick. Subsequent ticks in VIOLATION
with remote still absent do not repeat the IR signal.

### Acceptance criteria:

- ARMED + remote removed → VIOLATION, ir_send_tv_off() called once
- VIOLATION + remote still absent → stays VIOLATION, no repeated IR call
- ARMED + remote still present → stays ARMED
- ARMED + restriction ends → transitions to IDLE, lock_release() called

### Tests:

- test_lock.c::test_lock_armed_to_violation_on_remote_removal()
- test_lock.c::test_lock_violation_no_repeated_ir_signal()
- test_lock.c::test_lock_stays_armed_while_remote_present()
- test_lock.c::test_lock_armed_to_idle_when_restriction_ends()

---

## REQ-LOCK-003 : VIOLATION transitions to COOLDOWN when remote is returned

When the remote is returned during VIOLATION,
the system transitions to COOLDOWN. play_audio(CLIP_RETURN) and
led green flash are triggered exactly once on the transition tick.
From COOLDOWN the system transitions to ARMED if still in a
restricted window, or IDLE if the window has ended.
ir_send_tv_off() is called on every remote return regardless of mode.

### Acceptance criteria:

- VIOLATION + remote returned → COOLDOWN, play_audio(CLIP_RETURN) called once
- COOLDOWN + restriction active → ARMED, lock_arm() called
- COOLDOWN + no restriction → IDLE, lock_release() called
- ir_send_tv_off() called on remote return in any mode

### Tests:

- test_lock.c::test_lock_violation_to_cooldown_on_remote_return()
- test_lock.c::test_lock_cooldown_to_armed_if_restricted()
- test_lock.c::test_lock_cooldown_to_idle_if_free()
- test_lock.c::test_lock_ir_fires_on_every_remote_return()

---

## REQ-LOCK-004 : Violation audio escalates based on elapsed time

During VIOLATION, audio feedback escalates in
stages based on time elapsed since violation started. Stage
timings are named constants, not magic numbers. Each stage
triggers exactly once — audio does not repeat on every tick.

### Constants:

- LOCK_VIOLATION_SOFT_AUDIO_S (30 \* 60) 30 minutes
- LOCK_VIOLATION_WARN_AUDIO_S (restriction end - 5 \* 60) 5 min before end
- LOCK_VIOLATION_LOUD_AUDIO_S (restriction end) at restriction end

### Acceptance criteria:

- violation elapsed < 30 min → no audio
- violation elapsed >= 30 min → play_audio(CLIP_VIOLATION_SOFT) once
- 5 min before restriction end → play_audio(CLIP_VIOLATION_WARN) once
- restriction end reached → play_audio(CLIP_VIOLATION_LOUD) repeats each tick
- each stage triggers exactly once except LOUD which repeats

### Tests:

- test_lock.c::test_lock_violation_no_audio_before_threshold()
- test_lock.c::test_lock_violation_soft_audio_at_30min()
- test_lock.c::test_lock_violation_warn_audio_at_5min_before_end()
- test_lock.c::test_lock_violation_loud_audio_at_restriction_end()
- test_lock.c::test_lock_violation_soft_audio_triggers_only_once()
