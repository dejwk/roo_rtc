#include "gtest/gtest.h"
#include "roo_rtc/ds3231.h"
#include "roo_testing/devices/clock/ds3231/ds3231.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"

namespace {
struct Emulator {
  FakeDs3231 rtc;
  Emulator() { FakeEsp32().attachI2cDevice(rtc, 18, 19); }
} emulator;

// Verifies the real adapter and pin-routed DS3231 emulator agree on read/write.
TEST(Ds3231Integration, RoundTripAndFreshRead) {
  using namespace roo_time;
  roo_io::I2cMasterBusHandle bus;
  ASSERT_TRUE(bus.init(18, 19));
  roo_rtc::Ds3231Clock clock(bus);
  ASSERT_TRUE(clock.init());
  const WallTime time =
      DateTime(2024, 6, 15, 12, 34, 56, 0, timezone::UTC).wallTime();
  ASSERT_TRUE(clock.set(time));
  EXPECT_EQ(time, clock.read().time);
  roo_rtc::Ds3231Clock other(bus, timezone::UTC, Micros(0));
  ASSERT_TRUE(other.init());
  EXPECT_EQ(time, other.now());
}
}  // namespace
