#include "roo_rtc/rv3028.h"

namespace roo_rtc {
Rv3028Clock::Rv3028Clock(roo_time::UtcOffset offset,
                         roo_time::Duration interval)
    : Rv3028Clock(roo_io::I2cMasterBusHandle(), offset, interval) {}

Rv3028Clock::Rv3028Clock(roo_io::I2cMasterBusHandle bus,
                         roo_time::UtcOffset offset,
                         roo_time::Duration interval)
    : I2cClock(internal::Model::kRv3028, 0x52, bus, offset, interval) {}

bool Rv3028Clock::configureBackup(BackupMode mode) {
  uint8_t bits;
  switch (mode) {
    case BackupMode::kDisabled:
      bits = 0;
      break;
    case BackupMode::kDirect:
      bits = 4;
      break;
    case BackupMode::kLevel:
      bits = 12;
      break;
    default:
      return false;
  }
  uint8_t control;
  uint8_t status;
  if (!readRegisters(0x0f, &control, 1) || !writeRegister(0x0f, control | 8) ||
      !readRegisters(0x0e, &status, 1) || (status & 0x80) != 0) {
    return false;
  }
  uint8_t backup;
  if (!readRegisters(0x37, &backup, 1)) return false;
  // Preserve calibration/interrupt/resistance; disable charger, enable fast
  // edge detection as recommended, and apply the chosen BSM bits.
  return writeRegister(0x37, (backup & ~0x2c) | 0x10 | bits);
}
}  // namespace roo_rtc
