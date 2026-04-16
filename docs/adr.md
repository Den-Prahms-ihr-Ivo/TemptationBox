# Architecture Decision Document

- MikroController: ESP32-S3 -> better EMR channels, dual-corem strong ESP-ADF support
- ESP-IDF + ESP-ADF over Arduino — proper audio pipeline, RMT peripheral access
- WS2812B single strip for ring + 7-seg — one data pin, one render call
- Mechanical latch, spring-open, motor-release — low power, fail-safe open
- RFID + weight sensor for presence — belt-and-suspenders detection
- Own Python server for calendar sync — zero quota risk, full control
- 2-day rule lives in the backend, not on the ESP32
- Honor-system Pomodoro — 25 min timer on device, no desk enforcement
- iPad slot as deliberate mode-switch, not a second lock target

## ADR-001: 2-day rule is applied in the backend only

**Decision:** The 2-day filter runs exclusively in the Python backend before serving the schedule JSON. The firmware trusts the data it receives and applies no additional filtering.

**Consequence:** If the backend is unreachable, the firmware falls back to its NVS-cached schedule. This cache may contain events that were filtered at fetch time but would pass the 2-day rule today. Those events will remain absent from the cache until the next successful sync.

**Why this is acceptable:**

- Extended backend downtime is not expected
- Schedule changes are infrequent by design
- The LED ring signals stale cache to the user (is_stale flag), prompting manual intervention if needed
- The alternative: running the filter on the firmware. This duplicates logic across two systems and requires the ESP32 to distrust its own backend

**Rejected alternative:** Running the 2-day filter on the ESP32 as well.
Rejected because it solves an edge case that requires both a prolonged outage AND a schedule change within that window AND that change crossing the 48h threshold during the outage. The complexity cost exceeds the risk.
