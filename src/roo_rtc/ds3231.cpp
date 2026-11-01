#include "roo_rtc/ds3231.h"

namespace roo_rtc {
Ds3231Clock::Ds3231Clock(roo_time::UtcOffset offset,
                         roo_time::Duration interval)
    : Ds3231Clock(roo_io::I2cMasterBusHandle(), offset, interval) {}

Ds3231Clock::Ds3231Clock(roo_io::I2cMasterBusHandle bus,
                         roo_time::UtcOffset offset,
                         roo_time::Duration interval)
    : I2cClock(internal::Model::kDs3231, 0x68, bus, offset, interval) {}
}  // namespace roo_rtc
