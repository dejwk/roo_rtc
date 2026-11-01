#include "roo_rtc/clock.h"

namespace roo_rtc {
using namespace roo_time;

Clock::Clock(Duration max_uptime_trusted)
    : max_uptime_trusted_(max_uptime_trusted) {}

WallTime Clock::now() const {
  const Duration age = Uptime::Now() - sampled_at_;
  if (cached_.isSet() && age >= Micros(0) && age < max_uptime_trusted_) {
    return cached_ + Seconds(age.inSeconds());
  }
  return read().time;
}

ReadResult Clock::read() const {
  invalidateCache();
  // Timestamp before bus access, consistently with the old DS3231 driver.
  const Uptime sampled_at = Uptime::Now();
  ReadResult result = readHardware();
  if (result.status == ReadStatus::kOk && result.time.isSet()) {
    cached_ = result.time;
    sampled_at_ = sampled_at;
  } else {
    result.time = WallTime::Unset();
    if (result.status == ReadStatus::kOk) {
      result.status = ReadStatus::kInvalidData;
    }
  }
  return result;
}

bool Clock::set(WallTime time) {
  invalidateCache();
  // Do not cache the supplied value: hardware truncates fractional seconds,
  // and a fresh read confirms that the calendar and validity flags are usable.
  return time.isSet() && writeHardware(time);
}

void Clock::invalidateCache() const { cached_ = WallTime::Unset(); }
}  // namespace roo_rtc
