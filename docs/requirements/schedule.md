# Schedule + Calendar Logic, REQ-SCHED-XXX

---

## REQ-SCHED-001 (MOVED TO BACKEND)

### 2-day rule filters recent calendar changes

Events created or modified less than 48h before their start time shall be ignored by the schedule engine.

#### Acceptance criteria:

- Event modified 47h before start → excluded
- Event modified 49h before start → included

#### Tests:

---

## REQ-SCHED-002

### Resolve active window from event list

Given an array of events and the current unix time,
schedule_resolve() returns the ScheduleWindow whose start/end bracket now. If no event covers now, returns MODE_FREE with valid=false.

#### Acceptance criteria:

- now inside event → correct mode, correct until_unix, valid=true
- now before all events → MODE_FREE, valid=false
- now after all events → MODE_FREE, valid=false
- now exactly at start → included (boundary)
- now exactly at end → excluded (boundary)
- empty event list → MODE_FREE, valid=false

#### Tests:

- test_schedule.c::test_resolve\*\*()

## REQ-SCHED-003

### Cache returns safe data under all conditions

cache_get() always returns a usable ScheduleCache.
If no data has been stored, it returns an empty cache with is_stale=true.
If the stored data is older than 2 hours, is_stale is set to true.
If a fresh fetch has been stored, is_stale is false.

#### Acceptance criteria:

- No data stored → empty cache, is_stale=true
- Data stored, generated_at within 2h → is_stale=false
- Data stored, generated_at older than 2h → is_stale=true
- cache\*store() followed by cache_get() → returns same events

#### Tests:

- test_cache.c::test_cache\*\*()

## REQ-SCHED-004

### iPad hold duration scales with restriction mode

A pure helper function returns the required button hold duration in milliseconds for a given ScheduleMode. Higher restriction modes require longer holds. MODE_SLEEP grants no access.

### Acceptance criteria:

- MODE_FREE → 0ms
- MODE_RESTRICTED → 20000ms
- MODE_DEEP_FOCUS → 60000ms
- MODE_SLEEP → UINT32_MAX (no access sentinel)

#### Tests:

- test*schedule.c::test_ipad_hold_duration*\*()
