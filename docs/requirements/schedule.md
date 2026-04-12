# Schedule + Calendar Logic, REQ-SCHED-XXX

---

## REQ-SCHED-001

### 2-day rule filters recent calendar changes

Events created or modified less than 48h before their start time shall be ignored by the schedule engine.

#### Acceptance criteria:

- Event modified 47h before start → excluded
- Event modified 49h before start → included

#### Tests:

> TODO: Implement is tnur ein Platzhalter!!

- test_schedule.py::test_2day_rule_excludes_recent

---
