#pragma once
#include "roo_io/spi/spi.h"
#include "roo_rtc/internal/calendar.h"

namespace roo_rtc {

/// DS3234 SPI wall clock, supporting years 2000-2099, at 1 MHz in SPI mode 1.
class Ds3234Clock : public internal::CalendarClock {
 public:
  /// Uses the default SPI bus and active-low @p chip_select. The caller must
  /// initialize the bus. @p offset interprets stored time (UTC recommended);
  /// @p max_uptime_trusted bounds cached now() extrapolation.
  explicit Ds3234Clock(
      int chip_select, roo_time::UtcOffset offset = roo_time::timezone::UTC,
      roo_time::Duration max_uptime_trusted = roo_time::Seconds(10));

  /// Borrows an application-initialized @p bus and active-low @p chip_select.
  /// Keep the bus alive through destruction. @p offset interprets stored time;
  /// @p max_uptime_trusted bounds caching. Call init() before use.
  Ds3234Clock(roo_io::SpiMasterBusHandle bus, int chip_select,
              roo_time::UtcOffset offset = roo_time::timezone::UTC,
              roo_time::Duration max_uptime_trusted = roo_time::Seconds(10));

  /// Configures/registers CS without changing RTC registers; retryable.
  bool init() override;

 protected:
  bool readRegisters(uint8_t reg, uint8_t* data, size_t size) const override;
  bool writeRegisters(uint8_t reg, const uint8_t* data, size_t size) override;

 private:
  roo_io::SpiSlaveDevice device_;
};

}  // namespace roo_rtc
