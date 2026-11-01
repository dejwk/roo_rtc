#include <Arduino.h>
#include <SPI.h>

#include "roo_rtc/ds3234.h"

roo_rtc::Ds3234Clock rtc(5);

void setup() {
  Serial.begin(115200);
  SPI.begin();
  if (!rtc.init()) Serial.println("RTC transport setup failed");
}

void loop() {
  const roo_rtc::ReadResult result = rtc.read();
  if (result.status != roo_rtc::ReadStatus::kOk) {
    Serial.printf("RTC read status: %d\n", static_cast<int>(result.status));
  } else {
    const roo_time::DateTime time(result.time, roo_time::timezone::UTC);
    Serial.printf("%04d-%02d-%02d %02d:%02d:%02d UTC\n", time.year(),
                  time.month(), time.day(), time.hour(), time.minute(),
                  time.second());
  }
  delay(1000);
}
