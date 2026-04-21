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
