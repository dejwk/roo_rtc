#include "roo_rtc/ds3232.h"

namespace roo_rtc {
Ds3232Clock::Ds3232Clock(roo_time::UtcOffset offset,
                         roo_time::Duration interval)
    : Ds3232Clock(roo_io::I2cMasterBusHandle(), offset, interval) {}

Ds3232Clock::Ds3232Clock(roo_io::I2cMasterBusHandle bus,
                         roo_time::UtcOffset offset,
                         roo_time::Duration interval)
    : I2cClock(internal::Model::kDs3232, 0x68, bus, offset, interval) {}
}  // namespace roo_rtc
