#pragma once
#include <stdint.h>

#include <vector>

#include "registers.h"
#define MSBFIRST 1
#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3
struct SPISettings {
  SPISettings(uint32_t hz, uint8_t order, uint8_t mode)
      : hz(hz), order(order), mode(mode) {}
  uint32_t hz;
  uint8_t order;
  uint8_t mode;
};
class SPIClass {
 public:
  void beginTransaction(SPISettings settings) {
    last_mode = settings.mode;
    last_frequency = settings.hz;
    bytes.clear();
  }
  uint8_t transfer(uint8_t value) {
    if (bytes.empty()) {
      bytes.push_back(value);
      return 0;
    }
    const uint8_t reg = (bytes[0] & 0x7f) + bytes.size() - 1;
    bytes.push_back(value);
    return rtc_test::regs.data[reg];
  }
  void endTransaction() {
    if ((bytes[0] & 0x80) != 0) {
      bytes[0] &= 0x7f;
      rtc_test::regs.write(bytes.data(), bytes.size());
    } else {
      ++rtc_test::regs.reads;
      if (rtc_test::regs.before_read) {
        rtc_test::regs.before_read(bytes[0], bytes.size() - 1);
      }
    }
  }
  uint8_t last_mode = 0;
  uint32_t last_frequency = 0;

 private:
  std::vector<uint8_t> bytes;
};
extern SPIClass SPI;
