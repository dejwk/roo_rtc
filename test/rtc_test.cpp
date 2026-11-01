#include <memory>
#include <vector>

#include "gtest/gtest.h"
#include "registers.h"
#include "roo_rtc.h"
#if defined(ARDUINO)
TwoWire Wire;
SPIClass SPI;
#endif

namespace rtc_test {
Registers regs;
int64_t uptime_us = 0;
}  // namespace rtc_test
namespace roo_time {
const Uptime Uptime::Now() {
  return Uptime::Start() + Micros(rtc_test::uptime_us);
}
}  // namespace roo_time
namespace {
using namespace roo_rtc;
using namespace roo_time;
using rtc_test::regs;

enum Chip { kDs3231, kDs3232, kDs3234, kDs1307, kPcf8523, kRv3028, kRv8803 };

class RtcTest : public testing::TestWithParam<Chip> {
 protected:
  void SetUp() override {
    regs = {};
    rtc_test::uptime_us = 0;
    if (GetParam() <= kDs3234) {
      regs.status_reg = 0x0f;
      regs.w0c_mask = 0x83;
    }
    if (GetParam() == kRv3028) {
      regs.status_reg = 0x0e;
      regs.w0c_mask = 0x7f;
      regs.read_only_mask = 0x80;
    }
    if (GetParam() == kRv8803) {
      regs.status_reg = 0x0e;
      regs.w0c_mask = 0x3f;
    }
    seed();
  }
  int base() const { return GetParam() == kPcf8523 ? 3 : 0; }
  int dayIndex() const { return GetParam() == kPcf8523 ? 3 : 4; }
  void seed() {
    const std::vector<uint8_t> data =
        GetParam() == kPcf8523
            ? std::vector<uint8_t>{0x56, 0x34, 0x12, 0x15, 6, 0x06, 0x24}
            : std::vector<uint8_t>{0x56, 0x34, 0x12, 7, 0x15, 0x06, 0x24};
    std::copy(data.begin(), data.end(), regs.data.begin() + base());
  }
  std::unique_ptr<Clock> make(Duration interval = Seconds(10),
                              UtcOffset offset = timezone::UTC) {
    switch (GetParam()) {
      case kDs3231:
        return std::unique_ptr<Clock>(new Ds3231Clock(offset, interval));
      case kDs3232:
        return std::unique_ptr<Clock>(new Ds3232Clock(offset, interval));
      case kDs3234:
        return std::unique_ptr<Clock>(new Ds3234Clock(5, offset, interval));
      case kDs1307:
        return std::unique_ptr<Clock>(new Ds1307Clock(offset, interval));
      case kPcf8523:
        return std::unique_ptr<Clock>(new Pcf8523Clock(offset, interval));
      case kRv3028:
        return std::unique_ptr<Clock>(new Rv3028Clock(offset, interval));
      case kRv8803:
        return std::unique_ptr<Clock>(new Rv8803Clock(offset, interval));
    }
    return nullptr;
  }
  WallTime expected(UtcOffset offset = timezone::UTC) {
    return DateTime(2024, 6, 15, 12, 34, 56, 0, offset).wallTime();
  }
  void loseTime() {
    switch (GetParam()) {
      case kDs3231:
      case kDs3232:
      case kDs3234:
        regs.data[0x0f] |= 0x80;
        break;
      case kDs1307:
        regs.data[0] |= 0x80;
        break;
      case kPcf8523:
        regs.data[3] |= 0x80;
        break;
      case kRv3028:
        regs.data[0x0e] |= 1;
        break;
      case kRv8803:
        regs.data[0x0e] |= 3;
        break;
    }
  }
};

// Verifies fresh calendar reads, explicit chip addresses, and read-only init.
TEST_P(RtcTest, ReadsAndInitializesWithoutChangingRegisters) {
  const std::array<uint8_t, 256> before = regs.data;
  std::unique_ptr<Clock> clock = make();
  ASSERT_TRUE(clock->init());
  ASSERT_TRUE(clock->init());
  EXPECT_EQ(before, regs.data);
  EXPECT_EQ(0, regs.writes);
  const ReadResult result = clock->read();
  EXPECT_EQ(ReadStatus::kOk, result.status);
  EXPECT_EQ(expected(), result.time);
  if (GetParam() != kDs3234) {
    EXPECT_EQ(GetParam() == kRv3028   ? 0x52
              : GetParam() == kRv8803 ? 0x32
                                      : 0x68,
              regs.address);
  }
}

// Verifies interpolation, cache expiry, forced fresh reads, and invalidation.
TEST_P(RtcTest, CacheAndFreshRead) {
  std::unique_ptr<Clock> clock = make();
  ASSERT_TRUE(clock->init());
  ASSERT_EQ(expected(), clock->now());
  const int reads = regs.reads;
  rtc_test::uptime_us = 3500000;
  EXPECT_EQ(expected() + Seconds(3), clock->now());
  EXPECT_EQ(reads, regs.reads);
  regs.data[base()] = 0x57;
  EXPECT_EQ(expected() + Seconds(1), clock->read().time);
  regs.data[base()] = 0x58;
  clock->invalidateCache();
  EXPECT_EQ(expected() + Seconds(2), clock->now());
  regs.data[base()] = 0x59;
  rtc_test::uptime_us += 10000000;
  EXPECT_EQ(expected() + Seconds(3), clock->now());
}

// Verifies lost/stopped clocks return unset and failed reads are never cached.
TEST_P(RtcTest, TimeLossAndRecovery) {
  std::unique_ptr<Clock> clock = make();
  ASSERT_TRUE(clock->init());
  ASSERT_EQ(expected(), clock->now());
  loseTime();
  EXPECT_EQ(ReadStatus::kTimeLost, clock->read().status);
  EXPECT_FALSE(clock->now().isSet());
  ASSERT_TRUE(clock->set(expected()));
  EXPECT_EQ(expected(), clock->read().time);
}

// Verifies independent datasheet write bytes, weekday conventions, and
// precision.
TEST_P(RtcTest, WritesWholeSecondsAndPreservesOtherRegisters) {
  std::unique_ptr<Clock> clock = make();
  ASSERT_TRUE(clock->init());
  regs.data[0x20] = 0xa5;
  ASSERT_TRUE(clock->set(expected() + Millis(123)));
  std::vector<uint8_t> want = {0x56, 0x34, 0x12, 7, 0x15, 0x06, 0x24};
  if (GetParam() == kPcf8523) want = {0x56, 0x34, 0x12, 0x15, 6, 0x06, 0x24};
  if (GetParam() == kRv3028) want[3] = 6;
  if (GetParam() == kRv8803) want[3] = 0x40;
  EXPECT_EQ(want, std::vector<uint8_t>(regs.data.begin() + base(),
                                       regs.data.begin() + base() + 7));
  EXPECT_EQ(0xa5, regs.data[0x20]);
  EXPECT_EQ(expected(), clock->now());
}

// Verifies unsupported years and unset inputs fail before changing hardware.
TEST_P(RtcTest, RejectsUnrepresentableWrites) {
  std::unique_ptr<Clock> clock = make();
  ASSERT_TRUE(clock->init());
  for (int year : {1999, 2100, 2199}) {
    EXPECT_FALSE(
        clock->set(DateTime(year, 1, 1, 0, 0, 0, 0, timezone::UTC).wallTime()));
  }
  EXPECT_FALSE(clock->set(WallTime::Unset()));
  EXPECT_EQ(0, regs.writes);
}

// Verifies BCD digits, field bounds, Gregorian leap days, and century policy.
TEST_P(RtcTest, RejectsInvalidCalendars) {
  std::unique_ptr<Clock> clock = make(Micros(0));
  ASSERT_TRUE(clock->init());
  const std::vector<std::pair<int, uint8_t>> corrupt = {
      {0, 0x6a}, {1, 0x60}, {2, 0x24}, {dayIndex(), 0},
      {5, 0},    {5, 0x13}, {6, 0xfa}};
  for (const std::pair<int, uint8_t>& entry : corrupt) {
    seed();
    regs.data[base() + entry.first] = entry.second;
    EXPECT_EQ(ReadStatus::kInvalidData, clock->read().status);
  }
  seed();
  regs.data[base() + 5] = 2;
  regs.data[base() + dayIndex()] = 0x29;
  EXPECT_EQ(ReadStatus::kOk, clock->read().status);
  regs.data[base() + 6] = 0x23;
  EXPECT_EQ(ReadStatus::kInvalidData, clock->read().status);
  seed();
  regs.data[base() + 5] = 0x86;
  EXPECT_EQ(ReadStatus::kInvalidData, clock->read().status);
}

// Verifies fixed offsets round-trip while the default interpretation is UTC.
TEST_P(RtcTest, FixedOffset) {
  const UtcOffset offset = UtcOffset(Hours(2));
  std::unique_ptr<Clock> clock = make(Micros(0), offset);
  ASSERT_TRUE(clock->init());
  EXPECT_EQ(expected(offset), clock->now());
  ASSERT_TRUE(clock->set(expected(offset)));
  EXPECT_EQ(0x12, regs.data[base() + 2]);
}

// Verifies actual 12-hour register encodings, including midnight and noon.
TEST_P(RtcTest, TwelveHourReadAndWrite) {
  if (GetParam() == kRv8803) GTEST_SKIP();
  std::unique_ptr<Clock> clock = make(Micros(0));
  ASSERT_TRUE(clock->init());
  const bool ds = GetParam() <= kDs1307;
  if (GetParam() == kPcf8523) regs.data[0] |= 8;
  if (GetParam() == kRv3028) regs.data[0x10] |= 2;
  for (int hour : {0, 12, 23}) {
    regs.data[base() + 2] =
        (ds ? 0x40 : 0) | (hour >= 12 ? 0x20 : 0) | (hour == 23 ? 0x11 : 0x12);
    EXPECT_EQ(DateTime(2024, 6, 15, hour, 34, 56, 0, timezone::UTC).wallTime(),
              clock->read().time);
    if (!ds) {
      ASSERT_TRUE(clock->set(
          DateTime(2024, 6, 15, hour, 34, 56, 0, timezone::UTC).wallTime()));
      EXPECT_EQ((hour >= 12 ? 0x20 : 0) | (hour == 23 ? 0x11 : 0x12),
                regs.data[base() + 2]);
    }
  }
}

// Verifies every transport failure in a complete read or set invalidates cache.
TEST_P(RtcTest, FailsEveryTransactionAndRecovers) {
#if defined(ARDUINO)
  // Arduino SPI API has no transport-error return value.
  if (GetParam() == kDs3234) GTEST_SKIP();
#endif
  std::unique_ptr<Clock> clock = make();
  ASSERT_TRUE(clock->init());
  regs.calls = 0;
  ASSERT_EQ(expected(), clock->read().time);
  const int read_calls = regs.calls;
  for (int failure = 1; failure <= read_calls; ++failure) {
    regs.calls = 0;
    regs.fail_at = failure;
    EXPECT_EQ(ReadStatus::kIoError, clock->read().status);
    regs.fail_at = -1;
    EXPECT_EQ(expected(), clock->now());
  }
  regs.calls = 0;
  ASSERT_TRUE(clock->set(expected()));
  const int write_calls = regs.calls;
  for (int failure = 1; failure <= write_calls; ++failure) {
    seed();
    regs.calls = 0;
    regs.fail_at = failure;
    EXPECT_FALSE(clock->set(expected()));
    regs.fail_at = -1;
    ASSERT_TRUE(clock->set(expected()));
    EXPECT_EQ(expected(), clock->now());
  }
}

// Verifies time loss arriving during the calendar read is detected afterward.
TEST_P(RtcTest, TimeLossDuringRead) {
  if (GetParam() == kDs1307 || GetParam() == kPcf8523) GTEST_SKIP();
  std::unique_ptr<Clock> clock = make();
  ASSERT_TRUE(clock->init());
  regs.before_read = [&](uint8_t reg, size_t size) {
    if (reg == base() && size == 7) loseTime();
  };
  EXPECT_EQ(ReadStatus::kTimeLost, clock->read().status);
}

// Verifies failed writes discard cached time even when transport remains down.
TEST_P(RtcTest, FailedSetInvalidatesCache) {
#if defined(ARDUINO)
  if (GetParam() == kDs3234) GTEST_SKIP();
#endif
  std::unique_ptr<Clock> clock = make();
  ASSERT_TRUE(clock->init());
  ASSERT_EQ(expected(), clock->now());
  regs.io_error = true;
  EXPECT_EQ(expected(), clock->now());
  EXPECT_FALSE(clock->set(expected()));
  EXPECT_FALSE(clock->now().isSet());
  regs.io_error = false;
  EXPECT_EQ(expected(), clock->now());
}

// Verifies setting trusted time preserves pending peripheral event flags.
TEST_P(RtcTest, SetPreservesEventFlagsAndControl) {
  std::unique_ptr<Clock> clock = make();
  ASSERT_TRUE(clock->init());
  if (GetParam() <= kDs3234) {
    regs.data[0x0e] = 0x9f;
    regs.data[0x0f] = 0x83;
  } else if (GetParam() == kRv3028) {
    regs.data[0x10] = 0xfc;
    regs.data[0x0e] = 0x3f;
  } else if (GetParam() == kRv8803) {
    regs.data[0x0f] = 0x3c;
    regs.data[0x0e] = 0x3f;
  } else if (GetParam() == kPcf8523) {
    regs.data[0] = 0xa7;
    regs.data[1] = 0x7f;
  }
  ASSERT_TRUE(clock->set(expected()));
  if (GetParam() <= kDs3234) {
    EXPECT_EQ(0x1f, regs.data[0x0e]);
    EXPECT_EQ(3, regs.data[0x0f]);
  } else if (GetParam() == kRv3028) {
    EXPECT_EQ(0xfc, regs.data[0x10]);
    EXPECT_EQ(0x3e, regs.data[0x0e]);
  } else if (GetParam() == kRv8803) {
    EXPECT_EQ(0x3c, regs.data[0x0f]);
    EXPECT_EQ(0x3c, regs.data[0x0e]);
  } else if (GetParam() == kPcf8523) {
    EXPECT_EQ(0x87, regs.data[0]);
    EXPECT_EQ(0x7f, regs.data[1]);
  }
}

// Verifies documented range endpoints and weekday-independent read decoding.
TEST_P(RtcTest, SupportedEndpointsAndIgnoredWeekday) {
  std::unique_ptr<Clock> clock = make(Micros(0));
  ASSERT_TRUE(clock->init());
  for (int year : {2000, 2099}) {
    const WallTime time =
        DateTime(year, 12, 31, 23, 59, 59, 0, timezone::UTC).wallTime();
    ASSERT_TRUE(clock->set(time));
    regs.data[base() + (dayIndex() == 3 ? 4 : 3)] = 0xff;
    EXPECT_EQ(time, clock->read().time);
  }
}

INSTANTIATE_TEST_SUITE_P(AllChips, RtcTest,
                         testing::Values(kDs3231, kDs3232, kDs3234, kDs1307,
                                         kPcf8523, kRv3028, kRv8803));

// Verifies coherent RV-8803 reads retry rollover and bound repeated
// instability.
TEST(Rv8803, RolloverRetry) {
  regs = {};
  rtc_test::uptime_us = 0;
  Rv8803Clock clock;
  ASSERT_TRUE(clock.init());
  const uint8_t before[] = {0x59, 0x59, 0x23, 0x10, 0x31, 0x12, 0x26};
  std::copy(before, before + 7, regs.data.begin());
  int seconds_reads = 0;
  regs.before_read = [&](uint8_t reg, size_t size) {
    if (reg == 0 && size == 1 && ++seconds_reads == 1) {
      const uint8_t after[] = {0, 0, 0, 0x20, 1, 1, 0x27};
      std::copy(after, after + 7, regs.data.begin());
    }
  };
  EXPECT_EQ(DateTime(2027, 1, 1, 0, 0, 0, 0, timezone::UTC).wallTime(),
            clock.read().time);
  regs.before_read = [](uint8_t reg, size_t size) {
    if (reg == 0 && size == 1) ++regs.data[0];
  };
  EXPECT_EQ(ReadStatus::kUnstable, clock.read().status);
}

// Verifies PCF8523 backup is opt-in, preserves interrupts, and retains BSF.
TEST(Pcf8523, BackupConfiguration) {
  regs = {};
  Pcf8523Clock clock;
  ASSERT_TRUE(clock.init());
  regs.data[2] = 0xe3;
  ASSERT_TRUE(clock.configureBackup(true));
  EXPECT_EQ(0x0b, regs.data[2]);
  ASSERT_TRUE(clock.configureBackup(false));
  EXPECT_EQ(0xeb, regs.data[2]);
}

// Verifies RV-3028 backup changes remain volatile and never enable charging.
TEST(Rv3028, BackupConfiguration) {
  regs = {};
  Rv3028Clock clock;
  ASSERT_TRUE(clock.init());
  regs.data[0x37] = 0xe3;
  ASSERT_TRUE(clock.configureBackup(Rv3028Clock::BackupMode::kDirect));
  EXPECT_EQ(0xd7, regs.data[0x37]);
  EXPECT_NE(0, regs.data[0x0f] & 8);
  ASSERT_TRUE(clock.configureBackup(Rv3028Clock::BackupMode::kLevel));
  EXPECT_EQ(0xdf, regs.data[0x37]);
  regs.data[0x0e] = 0x80;
  EXPECT_FALSE(clock.configureBackup(Rv3028Clock::BackupMode::kDisabled));
}

#if defined(ARDUINO)
// Verifies incomplete Wire reads and exhausted transmit buffers fail safely.
TEST(I2cBoundary, ShortTransfers) {
  regs = {};
  Ds3231Clock clock;
  ASSERT_TRUE(clock.init());
  for (size_t n = 0; n < 7; ++n) {
    regs.short_read = n;
    EXPECT_EQ(ReadStatus::kIoError, clock.read().status);
  }
  regs.short_read = 256;
  regs.write_limit = 0;
  EXPECT_EQ(ReadStatus::kIoError, clock.read().status);
  EXPECT_FALSE(
      clock.set(DateTime(2024, 1, 1, 0, 0, 0, 0, timezone::UTC).wallTime()));
}
#else
// Verifies native registration failure permits retry and balanced destruction.
TEST(NativeBoundary, InitializationAndLifetime) {
  regs = {};
  {
    Ds3231Clock clock;
    EXPECT_EQ(ReadStatus::kIoError, clock.read().status);
    regs.init_error = true;
    EXPECT_FALSE(clock.init());
    regs.init_error = false;
    EXPECT_TRUE(clock.init());
    EXPECT_TRUE(clock.init());
    EXPECT_EQ(1, regs.added);
  }
  EXPECT_EQ(1, regs.removed);
  {
    Ds3234Clock clock(5);
    EXPECT_EQ(ReadStatus::kIoError, clock.read().status);
    regs.init_error = true;
    EXPECT_FALSE(clock.init());
    regs.init_error = false;
    EXPECT_TRUE(clock.init());
    EXPECT_TRUE(clock.init());
    EXPECT_EQ(2, regs.added);
  }
  EXPECT_EQ(2, regs.removed);
}
#endif
}  // namespace
