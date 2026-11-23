# roo_rtc 0.1.0

- Added calendar clock drivers for DS3231, DS3232, DS3234, DS1307, PCF8523, RV-3028-C7, and RV-8803-C7 on Arduino, native ESP-IDF, and the Pico SDK. The drivers report read validity, provide cache controls, and support opt-in backup configuration where available. RV-8803 snapshot reads use bounded retries.
- Reject `set()` timestamps outside 2000–2099 in the clock’s configured UTC offset before calendar conversion or register access, preventing extreme timestamps from wrapping into valid years.
- Exposed one public Bazel target per device and restricted shared implementation targets to library and test packages. Examples select their driver directly, and Arduino sketches explicitly include the matching `roo_io` I²C or SPI header for dependency discovery.
- Added integration tests for all seven drivers using independent `roo_testing` device fakes through the actual transport adapters on Arduino and native ESP-IDF, SDK-only Pico, and Arduino-Pico bare-metal and FreeRTOS. Retained scripted transport-failure coverage, including short reads.
- Set Bazel dependencies to `roo_io` 2.5.0, `roo_time` 2.2.1, and `roo_testing` 2.7.0, with CI also using `roo_testing` 2.7.0. PlatformIO declares minimum versions of `roo_io` 2.5.0 and `roo_time` 2.2.1. Arduino metadata declares both dependencies without version constraints.

---
