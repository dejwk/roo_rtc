#pragma once
#include "roo_time.h"

namespace roo_rtc {

/// Outcome of a fresh hardware read; validity is distinct from bus health.
enum class ReadStatus { kOk, kIoError, kTimeLost, kInvalidData, kUnstable };

/// Fresh reading and its validity. Failed readings always contain unset time.
struct ReadResult {
  /// Constructs a result, defaulting to an unset communication failure.
  ReadResult(roo_time::WallTime time = roo_time::WallTime::Unset(),
             ReadStatus status = ReadStatus::kIoError)
      : time(time), status(status) {}

  roo_time::WallTime time;
  ReadStatus status;
};

/// RTC-backed wall clock with optional uptime extrapolation between reads.
/// Instances are not thread-safe. The application serializes all access.
class Clock : public roo_time::WallTimeClock {
 public:
  /// Registers the device on an application-initialized bus, without changing
  /// clock or peripheral settings. Idempotent; failures permit retry.
  virtual bool init() = 0;

  /// Returns whole-second wall time, extrapolating a successful read for the
  /// configured cache interval. Cached calls do not probe hardware health or
  /// validity flags. A negative/zero interval disables caching. On a failed
  /// fresh read returns unset and retries on the next call.
  roo_time::WallTime now() const override;

  /// Reads hardware immediately, bypassing and replacing the cache. Reports
  /// communication errors, time-loss indicators, invalid calendar data, or a
  /// repeatedly unstable snapshot. Success does not establish synchronization
  /// to an external time reference. DS1307 cannot detect historical time loss.
  ReadResult read() const;

  /// Stores whole-second time and starts timekeeping, acknowledging time-loss
  /// flags only after the calendar write succeeds. Returns false for unset or
  /// unsupported times, or any failed transaction. Failure invalidates the
  /// cache; hardware may be partially changed, stopped, or contain a partial
  /// date. Retry the complete set after resolving the failure.
  bool set(roo_time::WallTime time);

  /// Forces the next now() call to read hardware.
  void invalidateCache() const;

 protected:
  /// Configures the maximum uptime interval trusted between hardware reads.
  explicit Clock(roo_time::Duration max_uptime_trusted);

  /// Obtains a validated hardware reading without touching the cache.
  virtual ReadResult readHardware() const = 0;

  /// Writes hardware; validates representability before changing registers.
  virtual bool writeHardware(roo_time::WallTime time) = 0;

 private:
  roo_time::Duration max_uptime_trusted_;
  mutable roo_time::WallTime cached_ = roo_time::WallTime::Unset();
  mutable roo_time::Uptime sampled_at_;
};

}  // namespace roo_rtc
