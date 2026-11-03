#define RTC_STRING_IMPL(value) #value
#define RTC_STRING(value) RTC_STRING_IMPL(value)
#include RTC_STRING(RTC_HEADER)
#include RTC_STRING(FAKE_HEADER)

#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#if defined(ROO_TESTING_SOC_RP2040)
#include "roo_testing/microcontrollers/rp2040/fake_pico.h"
#if !defined(ARDUINO)
#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/spi.h>
#endif
#else
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#endif

#if RTC_DEVICE == 2 && !defined(ARDUINO) && !defined(ROO_TESTING_SOC_RP2040)
#include "driver/spi_master.h"
#endif

namespace {
using namespace roo_time;
#if defined(ROO_TESTING_SOC_RP2040)
constexpr int kMiso = 16;
constexpr int kMosi = 19;
constexpr int kCs = 17;
#else
constexpr int kMiso = 19;
constexpr int kMosi = 21;
constexpr int kCs = 5;
#endif

// The device oscillator is independent of application uptime/cache. A stepped
// source can exercise RV-8803's live reads while deterministic delays test
// carry.
class DeviceTime : public roo_testing_transducers::Clock {
 public:
  int64_t getTimeMicros() const override {
    time_ += step_;
    return time_;
  }

  void delayMicros(uint64_t delay) override { time_ += delay; }

  const std::string& name() const override {
    static const std::string name = "RTC oscillator";
    return name;
  }

  void reset() {
    time_ = 0;
    step_ = 0;
  }

  void stepEachRead(int64_t step) { step_ = step; }

 private:
  mutable int64_t time_ = 0;
  int64_t step_ = 0;
};

// Attached objects live for the process: the board retains borrowed wiring.
struct Hardware {
  DeviceTime time;
#if RTC_DEVICE == 0
  FAKE_CLASS device{time};
#else
  FAKE_CLASS device{&time};
#endif
  Hardware() {
#if defined(ROO_TESTING_SOC_RP2040)
#if RTC_DEVICE == 2
    FakePico().attachSpiDevice(device, 18, kMiso, kMosi);
    FakePico().attachOutput(kCs, device.cs());
#else
    FakePico().attachI2cDevice(device, 4, 5);
#endif
#else
#if RTC_DEVICE == 2
    FakeEsp32().attachSpiDevice(device, 18, kMiso, kMosi);
    FakeEsp32().gpio.attachOutput(kCs, device.cs());
#else
    FakeEsp32().attachI2cDevice(device, 18, 19);
#endif
#endif
  }
} hardware;

#if RTC_DEVICE == 2
class RawSpi : public FakeSpiInterface {
 public:
  RawSpi() : FakeSpiInterface("inspection") {}
  uint32_t clkHz() const override { return 1000000; }
  SpiDataMode dataMode() const override { return kSpiMode1; }
  SpiBitOrder bitOrder() const override { return kSpiMsbFirst; }
};
#endif

class DeviceTest : public testing::Test {
 protected:
  void SetUp() override {
    hardware.time.reset();
    hardware.device.reset();
#if RTC_DEVICE == 2
#if defined(ARDUINO)
#if defined(ROO_TESTING_SOC_RP2040)
    SPI.begin();
#else
    SPI.begin(18, kMiso, kMosi);
#endif
    driver_.reset(new roo_rtc::RTC_CLASS(roo_io::SpiMasterBusHandle(SPI), kCs));
#elif defined(ROO_TESTING_SOC_RP2040)
    spi_init(spi0, 1000000);
    for (int pin : {18, kMiso, kMosi}) gpio_set_function(pin, GPIO_FUNC_SPI);
    driver_.reset(
        new roo_rtc::RTC_CLASS(roo_io::SpiMasterBusHandle(spi0), kCs));
#else
    spi_bus_config_t config = {};
    config.sclk_io_num = 18;
    config.miso_io_num = kMiso;
    config.mosi_io_num = kMosi;
    config.quadwp_io_num = -1;
    config.quadhd_io_num = -1;
    ASSERT_EQ(ESP_OK, spi_bus_initialize(SPI2_HOST, &config, SPI_DMA_DISABLED));
    driver_.reset(
        new roo_rtc::RTC_CLASS(roo_io::SpiMasterBusHandle(SPI2_HOST), kCs));
#endif
#else
#if defined(ROO_TESTING_SOC_RP2040)
#if defined(ARDUINO)
    Wire.setSDA(4);
    Wire.setSCL(5);
    bus_.init();
#else
    i2c_init(i2c0, 100000);
    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);
#endif
#else
    ASSERT_TRUE(bus_.init(18, 19, 100000));
#endif
    driver_.reset(new roo_rtc::RTC_CLASS(bus_));
#endif
    ASSERT_TRUE(driver_->init());
  }

  void TearDown() override {
    driver_.reset();
#if RTC_DEVICE == 2
    EXPECT_FALSE(hardware.device.protocolError());
#if defined(ARDUINO)
    SPI.end();
#elif defined(ROO_TESTING_SOC_RP2040)
    spi_deinit(spi0);
#else
    EXPECT_EQ(ESP_OK, spi_bus_free(SPI2_HOST));
#endif
#elif defined(ROO_TESTING_SOC_RP2040)
#if defined(ARDUINO)
    Wire.end();
#else
    i2c_deinit(i2c0);
#endif
#endif
  }

  WallTime expected() const {
    return DateTime(2024, 6, 15, 12, 34, 56, 0, timezone::UTC).wallTime();
  }

  // Inspect fake state through its bus protocol, independently of roo_rtc.
  uint8_t rawRead(uint8_t reg) {
#if RTC_DEVICE == 2
    RawSpi spi;
    uint8_t data[] = {reg, 0};
    hardware.device.cs().write(0.0f);
    hardware.device.transfer(spi, data, 16);
    hardware.device.cs().write(3.3f);
    return data[1];
#else
    uint8_t value = 0;
    EXPECT_EQ(FakeI2cDevice::I2C_ERROR_OK,
              hardware.device.write(&reg, 1, false, 100));
    EXPECT_EQ(FakeI2cDevice::I2C_ERROR_OK,
              hardware.device.read(&value, 1, true, 100));
    return value;
#endif
  }

  static constexpr int kTimeBase = RTC_DEVICE == 4 ? 3 : 0;
  static constexpr int kDayIndex = RTC_DEVICE == 4 ? 3 : 4;
#if RTC_DEVICE != 2
  roo_io::I2cMasterBusHandle bus_;
#endif
  std::unique_ptr<roo_rtc::RTC_CLASS> driver_;
};

// Verifies the selected Arduino/IDF/SDK adapter reaches the chip fake and
// observes power-on invalidity, and sets/reads a trusted date with correct raw
// bytes.
TEST_F(DeviceTest, PowerOnAndRoundTrip) {
  EXPECT_EQ(roo_rtc::ReadStatus::kTimeLost, driver_->read().status);
  ASSERT_TRUE(driver_->set(expected()));
  EXPECT_EQ(expected(), driver_->read().time);
  EXPECT_EQ(0x56, rawRead(kTimeBase));
  EXPECT_EQ(0x34, rawRead(kTimeBase + 1));
  EXPECT_EQ(0x12, rawRead(kTimeBase + 2));
  EXPECT_EQ(0x15, rawRead(kTimeBase + kDayIndex));
  const uint8_t weekday = RTC_DEVICE == 6                      ? 0x40
                          : RTC_DEVICE == 4 || RTC_DEVICE == 5 ? 6
                                                               : 7;
  EXPECT_EQ(weekday, rawRead(kTimeBase + (kDayIndex == 3 ? 4 : 3)));
}

// Verifies calendar progression against a separately modeled oscillator,
// including leap day, month carry, and year carry instead of just readback.
TEST_F(DeviceTest, LeapDayAndYearRollover) {
  const WallTime leap =
      DateTime(2024, 2, 28, 23, 59, 59, 0, timezone::UTC).wallTime();
  ASSERT_TRUE(driver_->set(leap));
  hardware.time.delayMicros(1000000);
  EXPECT_EQ(DateTime(2024, 2, 29, 0, 0, 0, 0, timezone::UTC).wallTime(),
            driver_->read().time);
  hardware.time.delayMicros(86400000000ULL);
  EXPECT_EQ(DateTime(2024, 3, 1, 0, 0, 0, 0, timezone::UTC).wallTime(),
            driver_->read().time);
  ASSERT_TRUE(driver_->set(
      DateTime(2024, 12, 31, 23, 59, 59, 0, timezone::UTC).wallTime()));
  hardware.time.delayMicros(2000000);
  EXPECT_EQ(DateTime(2025, 1, 1, 0, 0, 1, 0, timezone::UTC).wallTime(),
            driver_->read().time);
}

// Verifies a plausible but untrustworthy calendar is rejected after hardware
// flags time loss, and that a trusted write acknowledges the indication.
TEST_F(DeviceTest, LossAndRecovery) {
  ASSERT_TRUE(driver_->set(expected()));
  ASSERT_EQ(expected(), driver_->now());
  hardware.device.simulateTimeLoss();
  EXPECT_EQ(roo_rtc::ReadStatus::kTimeLost, driver_->read().status);
  EXPECT_FALSE(driver_->now().isSet());
  ASSERT_TRUE(driver_->set(expected() + Seconds(1)));
  EXPECT_EQ(expected() + Seconds(1), driver_->read().time);
}

// Verifies malformed hardware BCD stays malformed in the fake and cannot be
// accepted by the production decoder as a plausible date.
TEST_F(DeviceTest, InvalidHardwareCalendar) {
  ASSERT_TRUE(driver_->set(expected()));
  hardware.device.injectRegister(kTimeBase + 1, 0x6a);
  hardware.time.delayMicros(1000000);
  EXPECT_EQ(roo_rtc::ReadStatus::kInvalidData, driver_->read().status);
  ASSERT_TRUE(driver_->set(expected()));
  EXPECT_EQ(expected(), driver_->read().time);
}

// Verifies hour-mode decoding and preservation of mode-controlled alarms.
TEST_F(DeviceTest, TwelveHourMode) {
#if RTC_DEVICE == 6
  GTEST_SKIP() << "RV-8803 has a 24-hour calendar";
#else
  ASSERT_TRUE(driver_->set(expected()));
#if RTC_DEVICE == 4
  hardware.device.injectRegister(0, 8);
  hardware.device.injectRegister(5, 0x31);
#elif RTC_DEVICE == 5
  hardware.device.injectRegister(0x10, 2);
  hardware.device.injectRegister(2, 0x31);
#else
  hardware.device.injectRegister(2, 0x71);
#endif
  const WallTime late =
      DateTime(2024, 6, 15, 23, 34, 56, 0, timezone::UTC).wallTime();
  EXPECT_EQ(late, driver_->read().time);
  ASSERT_TRUE(driver_->set(late));
  EXPECT_EQ(late, driver_->read().time);
#if RTC_DEVICE == 4 || RTC_DEVICE == 5
  EXPECT_EQ(0x31, rawRead(kTimeBase + 2));
#endif
#endif
}

// Verifies STOP/CH/RESET controls hold the emulated oscillator, fresh reads
// reject frozen clocks, and restart does not accidentally include stopped time.
TEST_F(DeviceTest, StoppedClock) {
#if RTC_DEVICE == 3 || RTC_DEVICE == 4 || RTC_DEVICE == 6
  ASSERT_TRUE(driver_->set(expected()));
  const uint8_t reg = RTC_DEVICE == 6 ? 0x0f : 0;
  const uint8_t mask = RTC_DEVICE == 3 ? 0x80 : RTC_DEVICE == 4 ? 0x20 : 1;
  const uint8_t previous = rawRead(reg);
  hardware.device.injectRegister(reg, previous | mask);
  EXPECT_EQ(roo_rtc::ReadStatus::kTimeLost, driver_->read().status);
  hardware.time.delayMicros(5000000);
  hardware.device.injectRegister(reg, previous);
  EXPECT_EQ(expected(), driver_->read().time);
  hardware.time.delayMicros(1000000);
  EXPECT_EQ(expected() + Seconds(1), driver_->read().time);
#else
  GTEST_SKIP() << "No software halt in this calendar API";
#endif
}

// Verifies successful time-setting clears validity loss without losing
// pending peripheral events in the fake's write-zero-clear status register.
TEST_F(DeviceTest, PreservesPendingFlags) {
  ASSERT_TRUE(driver_->set(expected()));
#if RTC_DEVICE <= 2
  hardware.device.injectRegister(0x0f, 0x83);
  ASSERT_TRUE(driver_->set(expected()));
  EXPECT_EQ(3, rawRead(0x0f) & 0x83);
#elif RTC_DEVICE == 5
  hardware.device.injectRegister(0x0e, 0x3f);
  ASSERT_TRUE(driver_->set(expected()));
  EXPECT_EQ(0x3e, rawRead(0x0e));
#elif RTC_DEVICE == 6
  hardware.device.injectRegister(0x0e, 0x3f);
  ASSERT_TRUE(driver_->set(expected()));
  EXPECT_EQ(0x3c, rawRead(0x0e));
#else
  GTEST_SKIP() << "No separate validity/event register in this API";
#endif
}

// Verifies backup configuration against hardware masks, without enabling
// charging or making initialization alter power settings.
TEST_F(DeviceTest, BackupConfiguration) {
#if RTC_DEVICE == 4
  EXPECT_EQ(0xe0, rawRead(2) & 0xe0);
  ASSERT_TRUE(driver_->configureBackup(true));
  EXPECT_EQ(0, rawRead(2) & 0xe0);
#elif RTC_DEVICE == 5
  EXPECT_EQ(0, rawRead(0x37) & 0x0c);
  ASSERT_TRUE(
      driver_->configureBackup(roo_rtc::RTC_CLASS::BackupMode::kDirect));
  EXPECT_EQ(4, rawRead(0x37) & 0x0c);
  EXPECT_EQ(0, rawRead(0x37) & 0x20);
  EXPECT_EQ(8, rawRead(0x0f) & 8);
#else
  GTEST_SKIP() << "Backup configuration is fixed or external";
#endif
}

// Verifies a live RV-8803 that changes on every oscillator read exhausts
// bounded retries; a steady oscillator immediately permits recovery.
TEST_F(DeviceTest, LiveReadRetries) {
#if RTC_DEVICE == 6
  ASSERT_TRUE(driver_->set(expected()));
  hardware.time.stepEachRead(1000000);
  EXPECT_EQ(roo_rtc::ReadStatus::kUnstable, driver_->read().status);
  hardware.time.stepEachRead(0);
  ASSERT_TRUE(driver_->set(expected()));
  EXPECT_EQ(expected(), driver_->read().time);
#else
  GTEST_SKIP() << "This chip latches its calendar read";
#endif
}
}  // namespace
