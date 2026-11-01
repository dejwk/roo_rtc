#pragma once
#include "roo_rtc/internal/i2c_clock.h"

namespace roo_rtc {

/// PCF8523 wall-time clock; supports calendar years 2000-2099.
/// Uses roo_io on Arduino and native ESP-IDF. Store UTC unless compatibility
/// requires a fixed offset; apply timezone/DST conversion at presentation.
class Pcf8523Clock : public internal::I2cClock {
 public:
  /// Uses the default bus and @p offset for RTC calendar interpretation.
  /// @p max_uptime_trusted bounds caching; zero forces fresh reads.
  Pcf8523Clock(roo_time::UtcOffset offset = roo_time::timezone::UTC,
               roo_time::Duration max_uptime_trusted = roo_time::Seconds(10));

  /// Borrows @p bus (TwoWire& on Arduino). The application initializes it
  /// before use and keeps it alive. Call init() before accessing native IDF
  /// hardware. @p offset interprets the stored calendar, and
  /// @p max_uptime_trusted bounds cached now() extrapolation.
  Pcf8523Clock(roo_io::I2cMasterBusHandle bus,
               roo_time::UtcOffset offset = roo_time::timezone::UTC,
               roo_time::Duration max_uptime_trusted = roo_time::Seconds(10));

  /// Enables standard battery switchover and battery-low detection when true;
  /// disables both when false. Preserves interrupt enables and pending flags.
  /// init() leaves this setting untouched (factory default disables backup).
  bool configureBackup(bool enabled);
};

}  // namespace roo_rtc
