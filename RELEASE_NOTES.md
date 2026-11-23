# roo_rtc 1.0.0

- Added calendar drivers for DS3231, DS3232, DS3234, DS1307, PCF8523, RV-3028-C7, and RV-8803-C7 on Arduino, native ESP-IDF, and the Pico SDK. Drivers report read validity, offer cache controls, and support opt-in battery backup configuration where available. RV-8803 snapshot reads use bounded retries.
- Fixed `set()` to reject timestamps outside 2000–2099 in the clock’s configured UTC offset before calendar conversion or register access.
- Replaced the Bazel `:i2c`, `:spi`, and `:calendar` targets with public per-device targets and restricted access to shared implementation targets. Examples select their driver directly, and Arduino sketches explicitly include the matching `roo_io` transport header for dependency discovery.
- Added integration tests for all seven drivers using independent `roo_testing` device fakes through the real transport adapters on Arduino and native ESP-IDF, SDK-only Pico, and Arduino-Pico bare-metal and FreeRTOS. Retained scripted transport-failure tests, including short reads.
- Set Bazel dependencies to `roo_io` 2.5.0, `roo_time` 2.2.1, and `roo_testing` 2.7.0. CI also uses `roo_testing` 2.7.0. PlatformIO requires at least `roo_io` 2.5.0 and `roo_time` 2.2.1, while Arduino declares both without version constraints.

---
