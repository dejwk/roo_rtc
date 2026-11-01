#include "roo_rtc/pcf8523.h"

namespace roo_rtc {
Pcf8523Clock::Pcf8523Clock(roo_time::UtcOffset offset,
                           roo_time::Duration interval)
    : Pcf8523Clock(roo_io::I2cMasterBusHandle(), offset, interval) {}

Pcf8523Clock::Pcf8523Clock(roo_io::I2cMasterBusHandle bus,
                           roo_time::UtcOffset offset,
                           roo_time::Duration interval)
    : I2cClock(internal::Model::kPcf8523, 0x68, bus, offset, interval) {}

bool Pcf8523Clock::configureBackup(bool enabled) {
  uint8_t control;
  if (!readRegisters(2, &control, 1)) return false;
  // BSF is write-zero-clear; write one so a concurrently arriving event
  // survives.
  return writeRegister(2, (control & 0x1f) | 8 | (enabled ? 0 : 0xe0));
}
}  // namespace roo_rtc
