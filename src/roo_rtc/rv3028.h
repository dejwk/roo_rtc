#pragma once
#include "roo_rtc/internal/i2c_clock.h"

namespace roo_rtc {

/// RV-3028-C7 wall-time clock; supports calendar years 2000-2099.
/// Uses roo_io on Arduino and native ESP-IDF. Store UTC unless compatibility
/// requires a fixed offset; apply timezone/DST conversion at presentation.
class Rv3028Clock : public internal::I2cClock {
 public:
  /// Uses the default bus and @p offset for RTC calendar interpretation.
  /// @p max_uptime_trusted bounds caching; zero forces fresh reads.
  Rv3028Clock(roo_time::UtcOffset offset = roo_time::timezone::UTC,
              roo_time::Duration max_uptime_trusted = roo_time::Seconds(10));

  /// Borrows @p bus (TwoWire& on Arduino). The application initializes it
  /// before use and keeps it alive. Call init() before accessing native IDF
  /// hardware. @p offset interprets the stored calendar, and
  /// @p max_uptime_trusted bounds cached now() extrapolation.
  Rv3028Clock(roo_io::I2cMasterBusHandle bus,
              roo_time::UtcOffset offset = roo_time::timezone::UTC,
              roo_time::Duration max_uptime_trusted = roo_time::Seconds(10));

  /// Selects backup switching: disabled, direct voltage comparison, or fixed
  /// threshold. Charging is disabled. Changes volatile configuration only and
  /// disables automatic EEPROM refresh so the setting survives midnight.
  /// Call after each power-on; no EEPROM is written. On error refresh may
  /// remain disabled; retry this method after resolving the bus failure.
  enum class BackupMode { kDisabled, kDirect, kLevel };

  /// Applies @p mode without enabling charging or writing EEPROM.
  /// Returns false if configuration is busy or a transaction fails.
  bool configureBackup(BackupMode mode);
};

}  // namespace roo_rtc
