#include "roo_rtc/ds1307.h"

namespace roo_rtc {
Ds1307Clock::Ds1307Clock(roo_time::UtcOffset offset,
                         roo_time::Duration interval)
    : Ds1307Clock(roo_io::I2cMasterBusHandle(), offset, interval) {}

Ds1307Clock::Ds1307Clock(roo_io::I2cMasterBusHandle bus,
                         roo_time::UtcOffset offset,
                         roo_time::Duration interval)
    : I2cClock(internal::Model::kDs1307, 0x68, bus, offset, interval) {}
}  // namespace roo_rtc
