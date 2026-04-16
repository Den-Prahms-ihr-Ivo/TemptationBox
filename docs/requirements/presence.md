# Presence Detection (RFID + Weight),

## REQ-PRES-001 : Remote present only when both sensors agree

The presence module fuses RFID and weight sensor into a single authoritative PresenceState. Both sensors must report true for the remote to be considered present. Either sensor alone is insufficient.

### Acceptance criteria:

- RFID=true, weight=true → PRESENT
- RFID=true, weight=false → ABSENT
- RFID=false, weight=true → ABSENT
- RFID=false, weight=false → ABSENT

### Tests: test_presence.

c::test*presence_requires_both_sensors*\*()

---

## REQ-PRES-002 : Debounce transient sensor dropouts

A single ABSENT sample does not immediately transition state from PRESENT to ABSENT. N consecutive ABSENT samples are required before the state changes. A single PRESENT sample resets the counter and immediately restores PRESENT state.
The debounce threshold N is a named constant.

### Acceptance criteria:

- N-1 consecutive ABSENT samples → still PRESENT
- N consecutive ABSENT samples → transitions to ABSENT
- ABSENT streak interrupted by PRESENT before reaching N → stays PRESENT, counter resets
- ABSENT → PRESENT transition is immediate (no debounce in that direction)

### Tests:

test*presence.c::test_presence_debounce*\*()

---

## REQ-PRES-003 : iPad slot detection

A separate presence channel reports whether the
iPad is docked in the slot via the IR light-break sensor. Follows identical debounce rules as remote presence. Reported independently from remote presence: the two channels do not affect each other.

### Acceptance criteria:

- Light break detected for N samples → IPAD_DOCKED
- No light break → IPAD_ABSENT
- Transient flutter under N samples → no state change
- iPad channel state does not affect remote presence state

### Tests:

test*presence.c::test_ipad_slot*\*()
