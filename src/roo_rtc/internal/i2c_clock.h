#pragma once
#include "roo_io/i2c/i2c.h"
#include "roo_rtc/internal/calendar.h"

namespace roo_rtc {
namespace internal {

// I2C register transport shared by the calendar drivers.
class I2cClock : public CalendarClock {
 public:
  I2cClock(Model model, uint8_t address, roo_io::I2cMasterBusHandle bus,
           roo_time::UtcOffset offset, roo_time::Duration interval);
  bool init() override;

 protected:
  bool readRegisters(uint8_t reg, uint8_t* data, size_t size) const override;
  bool writeRegisters(uint8_t reg, const uint8_t* data, size_t size) override;

 private:
  roo_io::I2cSlaveDevice device_;
};

}  // namespace internal
}  // namespace roo_rtc
