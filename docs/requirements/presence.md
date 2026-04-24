# Presence Detection (RFID + Weight),

## Initial state

- ipad absent
- remote absent

## REQ-PRES-001

**Status:** GREEN

**Title:** Remote present only when both sensors agree

**Description:**

The presence module fuses RFID and weight sensor into a single authoritative PresenceState. Both sensors must report true for the remote to be considered present. Either sensor alone is insufficient.

**Acceptance criteria:**

- RFID=true, weight=true → PRESENT
- RFID=true, weight=false → ABSENT
- RFID=false, weight=true → ABSENT
- RFID=false, weight=false → ABSENT

**Tests:**

- test_presence.c::test_presence_both_sensors_true_is_present()
- test_presence.c::test_presence_rfid_only_is_absent()
- test_presence.c::test_presence_weight_only_is_absent()
- test_presence.c::test_presence_neither_sensor_is_absent()

---

## REQ-PRES-002

**Status:** GREEN

**Title:** Debounce applied in both directions with separate thresholds

**Description:**

State transitions in both directions require N
consecutive samples before the state changes. ABSENT→PRESENT and
PRESENT→ABSENT use separate named threshold constants. A single
sample in the opposite direction resets the counter for that
direction immediately.

**Constants:**

- PRESENCE_DEBOUNCE_PRESENT_SAMPLES 2 (ticks to confirm insertion)
- PRESENCE_DEBOUNCE_ABSENT_SAMPLES 3 (ticks to confirm removal)

**Acceptance criteria:**

- fewer than PRESENT_SAMPLES consecutive present reads → stays absent
- exactly PRESENT_SAMPLES consecutive present reads → transitions to present
- fewer than ABSENT_SAMPLES consecutive absent reads → stays present
- exactly ABSENT_SAMPLES consecutive absent reads → transitions to absent
- absent streak interrupted by a present read before reaching
  ABSENT_SAMPLES → stays present, absent counter resets
- present streak interrupted by an absent read before reaching
  PRESENT_SAMPLES → stays absent, present counter resets

**Tests:**

- test_presence.c::test_presence_remote_present_debounce_insufficient_samples()
- test_presence.c::test_presence_remote_present_debounce_sufficient_samples()
- test_presence.c::test_presence_remote_absent_debounce_insufficient_samples()
- test_presence.c::test_presence_remote_absent_debounce_sufficient_samples()
- test_presence.c::test_presence_remote_absent_counter_resets_on_present_sample_negative()
- test_presence.
- c::test_presence_remote_absent_counter_resets_on_present_sample_positive()
- test_presence.c::test_presence_remote_present_counter_resets_on_absent_sample_negative()
- c::test_presence_remote_present_counter_resets_on_absent_sample_positive()

---

## REQ-PRES-003

**Status:** GREEN

**Title:** iPad slot detection with independent debounce

**Description:**

A separate boolean channel reports whether the
iPad is docked in the slot via the IR light-break sensor on the
HAL. Follows identical debounce rules as remote presence using
the same threshold constants. The two channels are fully
independent — state of one does not affect the other.

**Acceptance criteria:**

- light break detected for PRESENT_SAMPLES ticks → ipad present
- no light break for ABSENT_SAMPLES ticks → ipad absent
- transient flutter under threshold → no state change
- iPad channel state does not affect remote presence state
- remote channel state does not affect iPad channel state

**Tests:**

- test_presence.c::test_ipad_present_debounce_sufficient_samples()
- test_presence.c::test_ipad_absent_debounce_insufficient_samples()
- test_presence.c::test_ipad_absent_debounce_sufficient_samples()
- test_presence.c::test_ipad_channel_independent_from_remote_remote_absent_ipad_present()
- test_presence.c::test_ipad_channel_independent_from_remote_remote_present_ipad_absent
