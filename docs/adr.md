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
