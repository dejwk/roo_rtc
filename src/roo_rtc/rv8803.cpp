#include "roo_rtc/rv8803.h"

namespace roo_rtc {
Rv8803Clock::Rv8803Clock(roo_time::UtcOffset offset,
                         roo_time::Duration interval)
    : Rv8803Clock(roo_io::I2cMasterBusHandle(), offset, interval) {}

Rv8803Clock::Rv8803Clock(roo_io::I2cMasterBusHandle bus,
                         roo_time::UtcOffset offset,
                         roo_time::Duration interval)
    : I2cClock(internal::Model::kRv8803, 0x32, bus, offset, interval) {}
}  // namespace roo_rtc
