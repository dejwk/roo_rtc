# roo_rtc 0.1.0

- Verify all seven RTC drivers against device fakes on SDK-only Pico and both Arduino-Pico runtimes, and retain scripted short-read cleanup coverage. Align I/O/time/testing dependencies.

- Expose one public Bazel target per RTC device and restrict shared source
  targets to library/test packages. Update examples to select their driver.
- Exercise every driver through real Arduino/ESP-IDF adapters against its
  independent roo_testing fake, while retaining scripted transport faults.


Initial calendar drivers for DS3231, DS3232, DS3234, DS1307, PCF8523, RV-3028-C7,
and RV-8803-C7 on Arduino and native ESP-IDF. Adds explicit read validity,
cache control, bounded RV-8803 snapshot retries, and opt-in backup configuration.

---
