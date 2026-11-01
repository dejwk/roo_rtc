#include "roo_rtc/ds3234.h"

#include <string.h>

namespace roo_rtc {
Ds3234Clock::Ds3234Clock(int cs, roo_time::UtcOffset offset,
                         roo_time::Duration interval)
    : Ds3234Clock(roo_io::SpiMasterBusHandle(), cs, offset, interval) {}

Ds3234Clock::Ds3234Clock(roo_io::SpiMasterBusHandle bus, int cs,
                         roo_time::UtcOffset offset,
                         roo_time::Duration interval)
    : CalendarClock(internal::Model::kDs3234, offset, interval),
      device_(bus, cs, 1000000, 1) {}

bool Ds3234Clock::init() { return device_.init(); }

bool Ds3234Clock::readRegisters(uint8_t reg, uint8_t* data, size_t size) const {
  uint8_t tx[8] = {};
  uint8_t rx[8];
  if (size == 0 || size > 7) return false;
  tx[0] = reg & 0x7f;
  if (!device_.transfer(tx, rx, size + 1)) return false;
  memcpy(data, rx + 1, size);
  return true;
}

bool Ds3234Clock::writeRegisters(uint8_t reg, const uint8_t* data,
                                 size_t size) {
  uint8_t tx[8];
  if (size == 0 || size > 7) return false;
  tx[0] = reg | 0x80;
  memcpy(tx + 1, data, size);
  return device_.transfer(tx, nullptr, size + 1);
}
}  // namespace roo_rtc
