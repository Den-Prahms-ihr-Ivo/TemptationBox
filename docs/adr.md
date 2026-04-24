# Architecture Decision Records

# Temptation Box — ESP32-S3

---

## ADR-001: ESP32-S3 as the primary microcontroller

**Decision:** Use the ESP32-S3 over alternatives like Raspberry Pi
or other ESP32 variants.

**Reason:**

- Fast startup, no OS overhead
- Dual-core — audio isolated on Core 1, logic on Core 0
- More RMT channels than ESP32 original — cleaner WS2812B driving
- Strong ESP-ADF support for proper audio pipeline
- More impressive portfolio piece than a Raspberry Pi

**Rejected alternative:** Raspberry Pi. Rejected because full Linux
OS overhead, slow boot, and unnecessary power for this use case.
Mit Kanonen auf Spatzen schießen.

---

## ADR-002: ESP-IDF + ESP-ADF over Arduino framework

**Decision:** Use ESP-IDF as the firmware framework with ESP-ADF
for audio, managed via PlatformIO.

**Reason:**

- Arduino is a compatibility layer on top of ESP-IDF that trades
  control for convenience
- Audio and WS2812B both have tight real-time timing requirements
  that compete badly in Arduino's single loop()
- ESP-ADF provides a proper I2S audio pipeline with buffering
- ESP-IDF exposes the RMT peripheral directly for WS2812B —
  Arduino abstracts this poorly
- Stronger portfolio story

**Rejected alternative:** Arduino framework. Rejected because audio
pipeline limitations and timing conflicts with LED control.

---

## ADR-003: Single WS2812B strip for ring and 7-segment display

**Decision:** One continuous WS2812B strip cut and routed through
both the mode indicator ring and the custom 7-segment display.

**Reason:**

- Single data pin, single power rail
- One atomic render call updates both displays simultaneously
- No timing conflicts between ring and display updates
- Segment mapping is a simple index lookup table in firmware
- Color feedback on both displays for free — no extra hardware

**Rejected alternative:** Separate strips or separate driver chips
for ring and display. Rejected because unnecessary complexity and
additional GPIO usage.

---

## ADR-004: Mechanical latch, spring-open, motor-release

**Decision:** The lock mechanism uses a mechanical latch held shut
by the latch itself. A motor fires briefly to release it. A spring
pushes the lid open once released.

**Reason:**

- Lock requires no power to stay closed — motor only fires on
  release, not to hold
- Fail-safe behavior on power loss: motor releases, spring opens,
  remote is accessible
- Low power draw during locked state
- The physical click of arming is a satisfying ritual moment

**Rejected alternative:** Solenoid or magnetic lock requiring
continuous power to stay locked. Rejected because power draw and
wrong failure mode — power loss should open the box, not trap
the remote.

---

## ADR-005: Python backend for Google Calendar sync

**Decision:** A personal Python server handles all Google Calendar
communication and serves a clean JSON schedule to the ESP32.

**Reason:**

- OAuth token management is painful on bare metal
- Python has mature Google API libraries
- Full control, no quota surprises, proper logging
- Easy to extend with violation logging, analytics later
- More maintainable than Google Apps Script which is prone to
  silent deprecation

**Rejected alternative:** Google Apps Script as a webhook endpoint.
Rejected due to quota limits, painful debugging, and maintenance
risk from Google deprecations.

---

## ADR-006: 2-day rule applied in backend only

**Decision:** The 48-hour modification filter runs exclusively in
the Python backend before serving the schedule JSON. The firmware
trusts the data it receives and applies no additional filtering.

**Consequence:** If the backend is unreachable, the firmware falls
back to its NVS-cached schedule. Events that were filtered at fetch
time but would pass the 2-day rule today remain absent from the
cache until the next successful sync.

**Why this is acceptable:**

- Extended backend downtime is not expected
- Schedule changes are infrequent by design
- The LED ring signals stale cache to the user via is_stale flag
- Running the filter on firmware duplicates logic across two systems

**Rejected alternative:** Running the 2-day filter on the ESP32 as
well. Rejected because the edge case requires simultaneously: a
prolonged outage AND a schedule change AND that change crossing the
48h threshold during the outage. Complexity cost exceeds the risk.

---

## ADR-007: SNTP for device time sync, not backend timestamp

**Decision:** The ESP32 uses SNTP (pool.ntp.org) for time
synchronisation. The backend's generated_at field is used only
for cache staleness detection, not as a time source.

**Reason:**

- SNTP is more accurate than an HTTP round trip
- Works even if the backend is unreachable
- Purpose-built protocol for time synchronisation
- generated_at serves a different purpose — data freshness —
  not device clock accuracy

**Rejected alternative:** A dedicated backend time endpoint.
Rejected because SNTP solves the problem better and a second
endpoint would couple a hardware concern to the schedule service.

---

## ADR-008: HAL as a struct of function pointers

**Decision:** Hardware access is abstracted behind a HAL struct
containing function pointers. Two implementations exist:
hal_esp32.c for target builds, hal_mock.c for host tests.

**Reason:**

- Entire business logic layer is testable on host without hardware
- Feedback loop under one second for logic tests
- Clean boundary — no ESP-IDF headers appear above the HAL
- hal_mock.c provides controllable state via setters and
  observable behaviour via getters

**Rejected alternative:** Direct hardware calls in business logic,
or a separate mock framework. Rejected because it would make
host-side TDD impossible.

---

## ADR-009: One CMake executable per test module

**Decision:** Each firmware module has its own test executable
in the host test runner. test_schedule, test_cache, test_presence
etc. are separate binaries.

**Reason:**

- Each test file defines its own main(), setUp(), tearDown()
- Separate binaries avoid duplicate symbol errors
- A failure in one module does not prevent others from running
- Modules can be tested in isolation during development

**Rejected alternative:** Single test_runner binary containing all
tests. Rejected due to duplicate main() symbol conflicts.

---

## ADR-010: MODE_PERMITTED as a distinct schedule mode

**Decision:** Scheduled TV windows are expressed as MODE_PERMITTED
in the ScheduleMode enum, sitting at priority 1 between MODE_FREE
and MODE_RESTRICTED.

**Reason:**

- Behaviourally identical to FREE — no enforcement, box unlocked
- Distinguished only by LED ring colour to signal intentional
  scheduled window vs unstructured free time
- Backend owns the semantic mapping from calendar event type
  to integer mode value
- Lock state machine unchanged — no new logic required

**Rejected alternative:** Reusing MODE_FREE for permitted windows.
Rejected because the visual distinction between "free time" and
"you planned this TV window" is behaviorally meaningful to the user.

---

## ADR-011: 2-day rule moved entirely to backend

**Decision:** REQ-SCHED-001 as originally written for the firmware
— schedule_accept_event() — was deleted. The requirement and its
tests moved to the Python backend as REQ-BE-001.

**Reason:**

- Backend has full Google Calendar metadata including modified_at
- Python environment makes date arithmetic and testing trivial
- Firmware receives a pre-filtered trusted event list
- Maintaining the filter in two places is the same bug waiting
  to happen twice

---

## ADR-012: NVS access via raw byte HAL functions

**Decision:** The HAL exposes nvs_write(uint8_t*, size_t) and
nvs_read(uint8_t*, size_t) rather than cache-aware functions.
The cache module owns serialisation of ScheduleCache to bytes.

**Reason:**

- Avoids circular import between hal.h and cache.h
- HAL remains unaware of business logic types
- Cache module is fully testable via mock byte buffer
- Clean dependency direction: cache.h imports hal.h,
  hal.h imports nothing above it

**Rejected alternative:** HAL functions typed to ScheduleCache.
Rejected because it created a circular dependency between hal.h
and cache.h.

---

## ADR-013: IR off signal fires on every remote return

**Decision:** hal->ir_send_tv_off() is called unconditionally
whenever the remote transitions from ABSENT to PRESENT — in any
mode, including FREE and PERMITTED.

**Reason:**

- Closes the loophole of turning the TV on and returning the
  remote to avoid detection
- Reinforces the implicit contract: remote in box = TV off
- No IR receiver or TV state tracking required
- Minor friction in FREE windows is an acceptable tradeoff for
  a simpler and more honest system

**Rejected alternative:** Firing IR off only during restricted
windows. Rejected because it leaves the loophole open and
requires mode-aware logic in the return handler.

---

## ADR-014: Presence debounce applied in both directions with separate thresholds

**Decision:** Debounce is applied to both ABSENT→PRESENT and
PRESENT→ABSENT transitions. Each direction has its own named
threshold constant:

    PRESENCE_DEBOUNCE_PRESENT_SAMPLES  2   // ticks to confirm insertion
    PRESENCE_DEBOUNCE_ABSENT_SAMPLES   3   // ticks to confirm removal

**Reason:**
Sensors can produce noisy readings in both directions:

- On removal: vibration or brief contact can cause spurious
  dropouts during normal use — debouncing prevents false violations
- On insertion: a noisy RFID read during remote placement could
  prematurely report PRESENT and arm the lock before the remote
  is settled — debouncing prevents premature state changes

The thresholds differ deliberately. Removal requires more
confirmation samples because a false ABSENT during a restricted
window triggers a violation — the higher cost justifies the
higher threshold. Insertion requires fewer samples because the
remote settling into the box is a less ambiguous physical event
than a vibration dropout.

At a 500ms tick rate:

- PRESENT confirmed after 1 second (2 samples)
- ABSENT confirmed after 1.5 seconds (3 samples)

Both thresholds apply equally to the remote presence channel
and the iPad slot channel.

Debounce logic lives entirely in presence.c, not in the HAL.
The HAL reports raw sensor state per tick and has no memory.
This keeps the HAL dumb and the debounce logic host-testable.

**Rejected alternative:** Debouncing only the PRESENT→ABSENT
direction. Rejected because it left the insertion path vulnerable
to noisy reads that could prematurely arm the lock or trigger
the IR off signal on a spurious PRESENT transition.

## ADR-015: Five-minute boot grace period before enforcement begins

**Decision:** On power-on the system enters LOCK_STATE_BOOT for
LOCK_BOOT_GRACE_S (300 seconds). During this window the motor holds
the lock open, no restrictions are enforced, and the release button
behaves as unrestricted regardless of the active mode. After the
grace period expires the state machine transitions to IDLE and
normal schedule-driven logic resumes.

**Reason:**

- On boot the device has no guarantee that WiFi has connected,
  SNTP has synced, or the calendar cache is fresh
- Enforcing restrictions with stale or missing schedule data would
  either trap the user unnecessarily or deny access based on wrong
  state
- The grace period gives task_net_sync time to fetch the current
  schedule and task_logic time to resolve the real active window
  before committing to any enforcement action
- Five minutes is generous but the device boots rarely enough that
  the cost is negligible

**Rejected alternative:** Immediate enforcement on boot using
cached data. Rejected because the cache may be hours old and the
first tick after boot could produce a false violation or a
false-secure state.

**User visibility:** The LED ring should show a distinct colour
during BOOT (suggest cyan or white pulsing) so the user understands
the device is initialising rather than malfunctioning.

---

## ADR-016: Consent-based lock — no automatic closure

**Decision:** The lock never engages automatically on a state
transition. Transition to ARMED only signals that the system wants
to be closed — the actual engagement requires the user to
physically close the lid with the remote inside. The motor holds
the lock open whenever either the lid is open or the remote is
absent. The user physically commits to the restriction by closing
the box.

**Reason:**

- An automatic closing click during deep work creates a Pavlovian
  cue that interrupts the exact mental state the device is meant
  to protect
- Self-imposed restriction only works when the user consents to it
  at the moment of closure — forcing it removes that consent
- The physical act of closing the lid becomes a micro-ritual
  that reinforces intention rather than undermining it
- Eliminates the edge case of the box slamming shut with the
  remote on the couch, creating deadlock

**Consequence:** The user can technically avoid enforcement by
never closing the lid. This is acceptable because:

- Leaving the box open during restricted windows is itself a
  visible failure the user cannot ignore
- Nag audio (REQ-LOCK-004) triggers if the lid is closed without
  the remote, closing the "close an empty box" loophole
- The whole system relies on consent — removing consent entirely
  means the device is not being used, which is a different problem

**Rejected alternative:** Solenoid-driven automatic closure on
restriction start. Rejected because the click cue is a concrete
behavioural harm that undermines the device's core purpose.

---

## ADR-017: Release button with mode-dependent hold duration

**Decision:** A physical release button on the box triggers
lock_hold_open(true). The required press behaviour depends on
the active mode via schedule_release_behaviour():

    MODE_FREE       → RELEASE_INSTANT   (single press)
    MODE_PERMITTED  → RELEASE_INSTANT   (single press)
    MODE_RESTRICTED → RELEASE_HOLD_20S
    MODE_DEEP_FOCUS → RELEASE_HOLD_60S
    MODE_SLEEP      → RELEASE_DENIED    (button has no effect)
    LOCK_STATE_BOOT → RELEASE_INSTANT   (always works during grace)

**Reason:**

- Extends the same friction model used for iPad access to remote
  retrieval — deliberate effort, not willpower, filters impulse
- Keeps the consent-based design consistent: the user physically
  commits to accessing the remote through a visible action
- The LED ring indicates which behaviour is active so the user
  knows what to expect before pressing

**Why not reuse the existing hold button for iPad mode?**

- Separate physical buttons for separate actions prevent
  accidental triggering
- The iPad button is inside near the slot, the release button
  is on the exterior — they belong to different use contexts

**Failure mode:** If the release button itself fails, the user
is locked out with no path to the remote. Mitigation: the boot
grace period on power cycle always grants instant access, so
pulling the plug is always a safe recovery path.

---

## ADR-018: Lid state and lock engagement are separate HAL signals

**Decision:** The HAL exposes three distinct lock-related inputs
and one output:

    bool lid_is_closed(void);          // is the lid physically shut?
    bool lock_is_engaged(void);        // is the latch mechanically caught?
    bool release_button_pressed(void); // is the release button held?
    void lock_hold_open(bool hold);    // motor holds lock open when true

The lid sensor and the lock engagement sensor are separate
physical signals.

**Reason:**

- The user can close the lid without pushing hard enough to
  engage the latch — these are physically distinct events
- The lock module needs to distinguish them to provide accurate
  feedback: "close the lid harder" is different from "you forgot
  to close the lid at all"
- Detecting lock_is_engaged confirms the mechanism worked — if
  the motor releases and the lock is still engaged the system
  can alert the user to a hardware problem

**Implementation:** Two separate sensors behind the HAL —
likely a simple reed switch or mechanical microswitch for the
lid, and a limit switch on the latch mechanism itself for
engagement detection.

**Rejected alternative:** A single "closed and locked" signal.
Rejected because it merges two physical events and prevents the
system from giving precise feedback about partial closure.

---

## ADR-019: Release behaviour policy lives in the schedule module

**Decision:** The schedule module owns the mapping from
ScheduleMode to ReleaseBehaviour via schedule_release_behaviour().
The lock module queries this on every tick rather than
hardcoding mode-specific behaviour internally.

**Reason:**

- Keeps all mode-based policy in one place — schedule.c already
  owns schedule_ipad_hold_ms() for the same reason
- A new mode added to the enum requires updating exactly one
  file to define its behaviour
- The lock module becomes mechanism, not policy — it knows how
  to engage and release but not why

**Consequence:** The lock module depends on schedule.h. This is
a clean one-way dependency with no risk of circular import since
schedule.h does not reference lock types.

---

## ADR-020: Nag audio for closed lid without remote

**Decision:** If the lid is closed during a restricted mode
(not FREE or PERMITTED) while the remote is absent, the system
plays escalating nag audio until the lid is reopened. This is
distinct from the VIOLATION audio because the user has not yet
physically removed the remote — they are attempting to fake
compliance with an empty box.

**Reason:**

- Without this, the user could close the empty box to mislead
  the system into thinking enforcement is active while the
  remote stays on the couch
- The nag reinforces the physical contract: the box must
  contain the remote when closed during restriction

**Behavioural distinction:** Nag audio is "close this correctly,"
violation audio is "you broke the contract." The device should
treat these as different experiences with different sounds —
nag is corrective, violation is consequential.

---

## ADR-021: Lock state machine has intrinsic time awareness

**Decision:** Unlike schedule_resolve which is stateless and
pure, the lock state machine is explicitly stateful and
time-aware. It records timestamps for:

    - boot_time_unix         (for BOOT grace expiry)
    - violation_started_unix (for audio escalation)
    - cooldown_entered_unix  (for hand-off timing)

These are read from hal->time_now_unix() on the relevant
transition tick and stored in internal module state.

**Reason:**

- Several behaviours (grace period, audio escalation, cooldown
  timing) are fundamentally time-driven
- Passing these timestamps through function signatures on every
  tick would be noisy and error-prone
- The state machine's job is to track exactly this kind of state

**Testability:** hal_mock_set_time() allows tests to advance the
clock deterministically between ticks. Tests set a known
initial time, call lock_tick, advance the mock clock, call
lock_tick again, and assert the resulting state.

---

## ADR-022: BOOT state does not consult schedule or presence

**Decision:** During LOCK_STATE_BOOT, lock_tick() ignores the
passed ScheduleWindow and remote_present arguments entirely.
It only checks whether the grace period has elapsed.

**Reason:**

- During BOOT the schedule and presence data may be stale,
  missing, or not yet initialised from NVS
- Acting on that data could produce incorrect transitions
- Explicitly ignoring the inputs documents that BOOT is a pure
  time-based state, independent of the rest of the system

**Consequence:** A test can pass any arbitrary ScheduleWindow
and presence value during BOOT and assert the state remains
BOOT until the grace period expires. This makes BOOT tests
simpler, not more complex.
