#pragma once
#include <stddef.h>
#include <stdint.h>

#include <array>
#include <functional>
#include <vector>

namespace rtc_test {
// Scripted register boundary. Expectations use explicit datasheet bytes rather
// than driver codecs; this is fault injection, not electrical chip emulation.
struct Registers {
  std::array<uint8_t, 256> data = {};
  int calls = 0;
  int fail_at = -1;
  int reads = 0;
  int writes = 0;
  bool io_error = false;
  bool init_error = false;
  size_t short_read = 256;
  size_t write_limit = 256;
  uint8_t address = 0;
  uint8_t status_reg = 0xff;
  uint8_t w0c_mask = 0;
  uint8_t read_only_mask = 0;
  int added = 0;
  int removed = 0;
  std::vector<std::vector<uint8_t>> transactions;
  std::function<void(uint8_t, size_t)> before_read;
  bool fail() { return io_error || ++calls == fail_at; }
  bool read(uint8_t reg, uint8_t* out, size_t len) {
    ++reads;
    if (fail()) return false;
    if (before_read) before_read(reg, len);
    for (size_t i = 0; i < len; ++i)
      out[i] = data[static_cast<uint8_t>(reg + i)];
    return true;
  }
  bool write(const uint8_t* bytes, size_t len) {
    ++writes;
    transactions.emplace_back(bytes, bytes + len);
    if (fail()) return false;
    for (size_t i = 1; i < len; ++i) {
      uint8_t& dest = data[static_cast<uint8_t>(bytes[0] + i - 1)];
      if (bytes[0] + i - 1 == status_reg) {
        const uint8_t writable =
            (bytes[i] & ~w0c_mask) | (dest & bytes[i] & w0c_mask);
        dest = (dest & read_only_mask) | (writable & ~read_only_mask);
      } else {
        dest = bytes[i];
      }
    }
    return true;
  }
};
extern Registers regs;
extern int64_t uptime_us;
}  // namespace rtc_test
