#include "roo_rtc/internal/calendar.h"

namespace roo_rtc {
namespace internal {
namespace {
// Checks both BCD digits before checking the field's inclusive bounds.
bool DecodeBcd(uint8_t raw, uint8_t minimum, uint8_t maximum, uint8_t& value) {
  if ((raw & 15) > 9 || (raw >> 4) > 9) return false;
  value = (raw >> 4) * 10 + (raw & 15);
  return value >= minimum && value <= maximum;
}

uint8_t EncodeBcd(uint8_t value) { return (value / 10) * 16 + value % 10; }

// Rejects impossible Gregorian dates; month/day bounds have already passed.
bool ValidDate(int year, int month, int day) {
  static const uint8_t days[] = {31, 28, 31, 30, 31, 30,
                                 31, 31, 30, 31, 30, 31};
  const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
  return day <= days[month - 1] + (month == 2 && leap);
}
}  // namespace
using namespace roo_time;

CalendarClock::CalendarClock(Model model, UtcOffset offset, Duration interval)
    : Clock(interval), model_(model), offset_(offset) {}

bool CalendarClock::isDs323x() const {
  return model_ == Model::kDs3231 || model_ == Model::kDs3232 ||
         model_ == Model::kDs3234;
}

uint8_t CalendarClock::timeRegister() const {
  return model_ == Model::kPcf8523 ? 3 : 0;
}

uint8_t CalendarClock::controlRegister() const {
  if (isDs323x()) return 0x0e;
  if (model_ == Model::kPcf8523) return 0;
  return model_ == Model::kRv3028 ? 0x10 : 0x0f;
}

bool CalendarClock::readState(uint8_t& control, uint8_t& flags) const {
  control = 0;
  flags = 0;
  if (model_ == Model::kDs1307) return true;
  if (!readRegisters(controlRegister(), &control, 1)) return false;
  if (isDs323x()) return readRegisters(0x0f, &flags, 1);
  if (model_ == Model::kRv3028 || model_ == Model::kRv8803) {
    return readRegisters(0x0e, &flags, 1);
  }
  return true;
}

bool CalendarClock::timeLost(uint8_t control, uint8_t flags) const {
  if (isDs323x()) return (flags & 0x80) != 0;
  if (model_ == Model::kPcf8523) return (control & 0x20) != 0;
  if (model_ == Model::kRv3028) return (flags & 1) != 0;
  if (model_ == Model::kRv8803) return (control & 1) != 0 || (flags & 3) != 0;
  return false;
}

ReadResult CalendarClock::readHardware() const {
  uint8_t control;
  uint8_t flags;
  if (!readState(control, flags)) return {};
  if (timeLost(control, flags)) {
    return {WallTime::Unset(), ReadStatus::kTimeLost};
  }
  uint8_t data[7];
  const Uptime started = Uptime::Now();
  // RV-8803 does not latch its calendar. Confirm seconds and retry a changed
  // snapshot. Bound both attempts and elapsed time so a full minute cannot
  // alias.
  for (int attempt = 0; attempt < 3; ++attempt) {
    if (!readRegisters(timeRegister(), data, sizeof(data))) return {};
    if (model_ != Model::kRv8803) break;
    uint8_t second;
    if (!readRegisters(0, &second, 1)) return {};
    if (Uptime::Now() - started >= Seconds(1)) {
      return {WallTime::Unset(), ReadStatus::kUnstable};
    }
    if (second == data[0]) break;
    if (attempt == 2) return {WallTime::Unset(), ReadStatus::kUnstable};
  }
  // Check time-loss flags again in case supply failed during calendar access.
  uint8_t after_control;
  if (!readState(after_control, flags)) return {};
  if (timeLost(after_control, flags)) {
    return {WallTime::Unset(), ReadStatus::kTimeLost};
  }
  if (after_control != control) {
    return {WallTime::Unset(), ReadStatus::kUnstable};
  }
  if ((model_ == Model::kDs1307 || model_ == Model::kPcf8523) &&
      (data[0] & 0x80) != 0) {
    return {WallTime::Unset(), ReadStatus::kTimeLost};
  }
  return decode(data, control);
}

ReadResult CalendarClock::decode(const uint8_t* data, uint8_t control) const {
  const ReadResult invalid = {WallTime::Unset(), ReadStatus::kInvalidData};
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t day;
  uint8_t month;
  uint8_t year;
  const int day_index = model_ == Model::kPcf8523 ? 3 : 4;
  const uint8_t month_mask = isDs323x() ? 0x1f : 0xff;
  if (!DecodeBcd(data[0], 0, 59, second) ||
      !DecodeBcd(data[1], 0, 59, minute) ||
      !DecodeBcd(data[day_index], 1, 31, day) ||
      !DecodeBcd(data[5] & month_mask, 1, 12, month) ||
      !DecodeBcd(data[6], 0, 99, year)) {
    return invalid;
  }
  bool twelve = false;
  uint8_t raw_hour = data[2];
  if (isDs323x() || model_ == Model::kDs1307) {
    if ((raw_hour & 0x80) != 0) return invalid;
    twelve = (raw_hour & 0x40) != 0;
    raw_hour &= 0x3f;
  } else if (model_ == Model::kPcf8523) {
    twelve = (control & 8) != 0;
  } else if (model_ == Model::kRv3028) {
    twelve = (control & 2) != 0;
  }
  if (twelve) {
    if ((raw_hour & 0xc0) != 0 || !DecodeBcd(raw_hour & 0x1f, 1, 12, hour)) {
      return invalid;
    }
    hour = hour % 12 + ((raw_hour & 0x20) != 0 ? 12 : 0);
  } else if (!DecodeBcd(raw_hour, 0, 23, hour)) {
    return invalid;
  }
  // Limit the DS323x calendar to its guaranteed leap-year compensation range.
  // The hardware's century bit does not fix its 2100 leap-year behavior.
  if (isDs323x() && (data[5] & 0xe0) != 0) return invalid;
  if (!ValidDate(2000 + year, month, day)) return invalid;
  return {DateTime(2000 + year, month, day, hour, minute, second, 0, offset_)
              .wallTime(),
          ReadStatus::kOk};
}

void CalendarClock::encode(const DateTime& time, uint8_t control,
                           uint8_t* data) const {
  data[0] = EncodeBcd(time.second());
  data[1] = EncodeBcd(time.minute());
  data[2] = EncodeBcd(time.hour());
  // Preserve configured 12-hour mode, including alarm interpretation.
  const bool twelve = model_ == Model::kPcf8523
                          ? (control & 8) != 0
                          : model_ == Model::kRv3028 && (control & 2) != 0;
  if (twelve) {
    data[2] = EncodeBcd(time.hour() % 12 == 0 ? 12 : time.hour() % 12) |
              (time.hour() >= 12 ? 0x20 : 0);
  }
  const uint8_t weekday = static_cast<uint8_t>(time.dayOfWeek());
  const int day_index = model_ == Model::kPcf8523 ? 3 : 4;
  data[day_index] = EncodeBcd(time.day());
  uint8_t encoded_weekday = weekday + 1;
  if (model_ == Model::kRv8803) {
    encoded_weekday = 1 << weekday;
  } else if (model_ == Model::kPcf8523 || model_ == Model::kRv3028) {
    encoded_weekday = weekday;
  }
  data[day_index == 3 ? 4 : 3] = encoded_weekday;
  data[5] = EncodeBcd(time.month());
  data[6] = EncodeBcd(time.year() - 2000);
}

bool CalendarClock::writeRegister(uint8_t reg, uint8_t value) {
  return writeRegisters(reg, &value, 1);
}

bool CalendarClock::writeHardware(WallTime time) {
  const DateTime dt(time, offset_);
  if (dt.year() < 2000 || dt.year() > 2099) return false;
  uint8_t control;
  uint8_t flags;
  if (!readState(control, flags)) return false;
  uint8_t data[7];
  encode(dt, control, data);
  const bool stop = model_ == Model::kPcf8523 || model_ == Model::kRv8803;
  const uint8_t stop_mask = model_ == Model::kPcf8523 ? 0x20 : 1;
  if (stop && !writeRegister(controlRegister(), control | stop_mask)) {
    return false;
  }
  if (!writeRegisters(timeRegister(), data, sizeof(data))) return false;
  if (stop && !writeRegister(controlRegister(), control & ~stop_mask)) {
    return false;
  }
  if (isDs323x()) {
    // Explicit set() also enables the oscillator on battery power.
    if (!writeRegister(0x0e, control & ~0x80)) return false;
    if (!readRegisters(0x0f, &flags, 1)) return false;
    // Write-one-preserve alarm flags, including events arriving after the read.
    return writeRegister(0x0f, (flags & ~0x80) | 3);
  }
  if (model_ == Model::kRv3028) return writeRegister(0x0e, 0xfe);
  if (model_ == Model::kRv8803) return writeRegister(0x0e, 0x3c);
  return true;
}
}  // namespace internal
}  // namespace roo_rtc
