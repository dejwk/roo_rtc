#include "roo_rtc/internal/i2c_clock.h"

#include <string.h>

namespace roo_rtc {
namespace internal {
I2cClock::I2cClock(Model model, uint8_t address, roo_io::I2cMasterBusHandle bus,
                   roo_time::UtcOffset offset, roo_time::Duration interval)
    : CalendarClock(model, offset, interval), device_(bus, address) {}

bool I2cClock::init() { return device_.init(); }

bool I2cClock::readRegisters(uint8_t reg, uint8_t* data, size_t size) const {
  return device_.transmitReceive(reinterpret_cast<const roo::byte*>(&reg), 1,
                                 reinterpret_cast<roo::byte*>(data), size);
}

bool I2cClock::writeRegisters(uint8_t reg, const uint8_t* data, size_t size) {
  // Seven calendar bytes plus the address fit even small Wire buffers.
  uint8_t buffer[8];
  if (size == 0 || size > sizeof(buffer) - 1) return false;
  buffer[0] = reg;
  memcpy(buffer + 1, data, size);
  return device_.transmit(reinterpret_cast<const roo::byte*>(buffer), size + 1);
}
}  // namespace internal
}  // namespace roo_rtc
