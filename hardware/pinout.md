> Avoid GPIO 45, 46 (strapping pins). Avoid GPIO 19, 20 (USB). Check the S3 datasheet for input-only pins before finalising

- GPIO 4 — WS2812B data (RMT channel 0)
- GPIO 5 — RFID SPI CLK (MFRC522)
- GPIO 6 — RFID SPI MISO
- GPIO 7 — RFID SPI MOSI
- GPIO 8 — RFID SPI CS
- GPIO 9 — Latch motor control (PWM)
- GPIO 10 — IR transmitter
- GPIO 11 — Weight sensor (load cell via HX711)
- GPIO 0 — Pomodoro / hold button (boot-safe)
- GPIO 1 — iPad slot IR / light break sensor
