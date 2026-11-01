#pragma once
#include <stddef.h>
#include <stdint.h>

#include "roo_rtc/clock.h"

namespace roo_rtc {
namespace internal {

// Register layouts, not inferred identities. Several chips share I2C address.
enum class Model {
  kDs3231,
  kDs3232,
  kDs3234,
  kDs1307,
  kPcf8523,
  kRv3028,
  kRv8803
};

// Calendar semantics shared independently of the I2C/SPI register transport.
class CalendarClock : public Clock {
 public:
  CalendarClock(Model model, roo_time::UtcOffset offset,
                roo_time::Duration max_uptime_trusted);

 protected:
  ReadResult readHardware() const override;
  bool writeHardware(roo_time::WallTime time) override;
  virtual bool readRegisters(uint8_t reg, uint8_t* data, size_t size) const = 0;
  virtual bool writeRegisters(uint8_t reg, const uint8_t* data,
                              size_t size) = 0;
  bool writeRegister(uint8_t reg, uint8_t value);

 private:
  // Reads validity/control without clearing interrupt flags as a side effect.
  bool readState(uint8_t& control, uint8_t& flags) const;
  bool timeLost(uint8_t control, uint8_t flags) const;
  bool isDs323x() const;
  uint8_t timeRegister() const;
  uint8_t controlRegister() const;
  ReadResult decode(const uint8_t* data, uint8_t control) const;
  void encode(const roo_time::DateTime& time, uint8_t control,
              uint8_t* data) const;
  Model model_;
  roo_time::UtcOffset offset_;
};

}  // namespace internal
}  // namespace roo_rtc
