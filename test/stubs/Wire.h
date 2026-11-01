#pragma once
#include <algorithm>

#include "registers.h"

class TwoWire {
 public:
  bool begin(int = -1, int = -1, uint32_t = 0) { return true; }
  void beginTransmission(uint8_t address) {
    rtc_test::regs.address = address;
    sent.clear();
  }
  size_t write(const uint8_t* bytes, size_t len) {
    const size_t accepted = std::min(len, rtc_test::regs.write_limit);
    sent.insert(sent.end(), bytes, bytes + accepted);
    return accepted;
  }
  uint8_t endTransmission(bool = true) {
    if (sent.empty()) return 4;
    reg = sent[0];
    if (sent.size() == 1) return rtc_test::regs.fail() ? 4 : 0;
    return rtc_test::regs.write(sent.data(), sent.size()) ? 0 : 4;
  }
  size_t requestFrom(uint8_t, uint8_t len) {
    received.resize(len);
    index = 0;
    if (!rtc_test::regs.read(reg, received.data(), len)) return 0;
    received.resize(std::min<size_t>(len, rtc_test::regs.short_read));
    return received.size();
  }
  int read() { return index < received.size() ? received[index++] : -1; }

 private:
  uint8_t reg = 0;
  size_t index = 0;
  std::vector<uint8_t> sent;
  std::vector<uint8_t> received;
};
extern TwoWire Wire;
