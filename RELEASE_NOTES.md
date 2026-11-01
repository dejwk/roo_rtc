# Unreleased

- Expose one public Bazel target per RTC device and restrict shared source
  targets to library/test packages. Update examples to select their driver.
- Exercise every driver through real Arduino/ESP-IDF adapters against its
  independent roo_testing fake, while retaining scripted transport faults.

# 0.1.0

Initial calendar drivers for DS3231, DS3232, DS3234, DS1307, PCF8523, RV-3028-C7,
and RV-8803-C7 on Arduino and native ESP-IDF. Adds explicit read validity,
cache control, bounded RV-8803 snapshot retries, and opt-in backup configuration.
