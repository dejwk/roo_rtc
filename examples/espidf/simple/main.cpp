#include <cstdio>

#include "roo_rtc/ds3231.h"

#if defined(ARDUINO)
#error This example uses native ESP-IDF only.
#endif
#if defined(ROO_TESTING)
#include "roo_testing/devices/clock/ds3231/ds3231.h"
#include "roo_testing/microcontrollers/esp32/fake_esp32.h"
#endif
extern "C" void app_main() {
#if defined(ROO_TESTING)
  static FakeDs3231 rtc;
  FakeEsp32().attachI2cDevice(rtc, 18, 19);
#endif
  // Choose pins appropriate to your board. Keep bus and clock alive together.
  roo_io::I2cMasterBusHandle bus;
  if (!bus.init(18, 19)) {
    std::printf("Cannot initialize I2C bus\n");
    return;
  }
  roo_rtc::Ds3231Clock clock(bus);
  if (!clock.init()) {
    std::printf("Cannot register RTC device\n");
    return;
  }
  std::printf(clock.now().isSet() ? "RTC read succeeded\n"
                                  : "Cannot read RTC\n");
}
