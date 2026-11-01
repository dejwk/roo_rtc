# roo_rtc

RTC drivers implementing `roo_time::WallTimeClock` for Arduino and native
ESP-IDF. The library reads and sets calendar time, reports hardware validity,
and optionally extrapolates time using uptime between hardware reads.

| Header | Class in `roo_rtc` | Bus | Address | Calendar years |
| --- | --- | --- | --- | --- |
| `roo_rtc/ds3231.h` | `Ds3231Clock` | I²C | `0x68` | 2000–2099 |
| `roo_rtc/ds3232.h` | `Ds3232Clock` | I²C | `0x68` | 2000–2099 |
| `roo_rtc/ds3234.h` | `Ds3234Clock` | SPI, mode 1, 1 MHz | CS pin | 2000–2099 |
| `roo_rtc/ds1307.h` | `Ds1307Clock` | I²C | `0x68` | 2000–2099 |
| `roo_rtc/pcf8523.h` | `Pcf8523Clock` | I²C | `0x68` | 2000–2099 |
| `roo_rtc/rv3028.h` | `Rv3028Clock` | I²C | `0x52` | 2000–2099 |
| `roo_rtc/rv8803.h` | `Rv8803Clock` | I²C | `0x32` | 2000–2099 |

`roo_rtc.h` includes all drivers. Individual headers let I²C-only consumers avoid
SPI headers. These are explicit chip drivers: several share an address, so the
library does not probe addresses to guess chip identity. Board voltage, crystal,
battery wiring, and pull-ups must suit the chip and MCU. DS1307 needs a 5 V main
supply; a breakout board does not necessarily provide logic-level conversion.

## Usage

```cpp
#include <Wire.h>
#include "roo_rtc/ds3231.h"

roo_rtc::Ds3231Clock rtc(Wire);

void setup() {
  Wire.begin();
  if (!rtc.init()) return;
  // Set only from a trusted reference, not unconditionally at every boot:
  // rtc.set(roo_time::DateTime(2026, 11, 1, 12, 0, 0, 0,
  //                          roo_time::timezone::UTC).wallTime());
}

void loop() {
  const roo_time::WallTime now = rtc.now();
  if (!now.isSet()) return;
  const roo_time::DateTime utc(now, roo_time::timezone::UTC);
  // Use utc.year(), utc.month(), utc.day(), utc.hour(), etc.
}
```

Other I²C drivers use the same constructors. `init()` registers the transport
without resetting the RTC, starting a stopped clock, clearing validity flags,
changing hour mode, or enabling backup/charging. On Arduino, the application
calls `Wire.begin()`; `init()` is optional for I²C but recommended consistently.
On native IDF, initialize `roo_io::I2cMasterBusHandle` and call `rtc.init()`:

```cpp
roo_io::I2cMasterBusHandle bus;
if (!bus.init(18, 19, 100000)) return;
roo_rtc::Ds3231Clock rtc(bus);
if (!rtc.init()) return;
const roo_rtc::ReadResult result = rtc.read();
```

The underlying `roo_io` bus lifetime rules apply. Arduino references borrow Wire;
native I²C handles retain an owned bus or borrow an external bus. SPI always
borrows the bus, which must outlive all devices. Initialize it separately:

```cpp
// Arduino: SPI.begin(...);
// Native IDF: spi_bus_initialize(SPI2_HOST, &config, SPI_DMA_DISABLED);
roo_io::SpiMasterBusHandle bus;
roo_rtc::Ds3234Clock rtc(bus, 5);  // Active-low CS on GPIO 5.
if (!rtc.init()) return;
const roo_rtc::ReadResult result = rtc.read();
```

Native firmware components need `esp_driver_i2c` and, for DS3234,
`esp_driver_spi`, plus `roo_time` and the corresponding `roo_io` sources. Arduino
SPI success cannot detect absent hardware: its API has no transfer-error return.
IDF SPI reports driver errors but synchronous polling has no finite timeout.

## Time interpretation and validity

Store **UTC** in hardware and apply timezones/DST when displaying time. A fixed
`UtcOffset` constructor argument remains available for existing deployments;
it changes how the calendar is interpreted, not the hardware's timezone.

`read()` always accesses hardware and returns a `ReadResult`:

| Status | Meaning |
| --- | --- |
| `kOk` | Calendar is representable and hardware reports no known time loss |
| `kIoError` | Required transport operation failed or was incomplete |
| `kTimeLost` | Oscillator-stop, halted/frozen clock, power-on-reset, or voltage-loss indication |
| `kInvalidData` | Invalid BCD, impossible Gregorian date, unsupported century, or field bounds |
| `kUnstable` | Snapshot/control changed during the read or rollover retries were exhausted |

Failure always yields `WallTime::Unset()`. Success does not prove the clock was
synchronized. DS1307 has only a clock-halt bit, with no sticky oscillator-stop
history: a plausible running calendar cannot establish that backup survived.
RV-8803 conservatively rejects both V1F (temperature compensation interrupted)
and V2F (data lost). Clear these by setting a trusted time.

The weekday field is ignored when reading; the calendar determines weekday.
Writes use each chip's numbering, including RV-8803's one-hot weekday.
Both 12-hour and 24-hour formats are read where supported. PCF8523 and RV-3028
writes preserve their configured hour mode so existing alarms keep their meaning;
DS-family calendar writes select 24-hour mode. No alarm registers are changed.

All drivers support years 2000–2099. DS323x century values outside this range
are rejected rather than promising calendar behavior beyond the documented
leap-year compensation range. RTC values have whole-second precision;
subseconds in `set()` are truncated. RV-3028's independent Unix counter is not
used or synchronized by this calendar API.

## Caching and writes

`now()` uses a successful read for up to `max_uptime_trusted` (default ten
seconds), advancing by whole elapsed uptime seconds. Cached calls do not check
bus health or validity flags. Pass `roo_time::Micros(0)` to disable caching,
call `read()` to bypass it, or call `invalidateCache()` to force the next
`now()` to read hardware. Failed reads are never cached. Serialize all calls;
clock instances are not thread-safe.

`set()` returns false for unset/unsupported times and on any failed transaction.
Success confirms the required transport operations, not a readback comparison.
RV-3028 must be unlocked; password-protected writes can be acknowledged by the
chip while being ignored. The driver does not unlock devices or manage passwords.
Setting time starts timekeeping and acknowledges time-loss flags only after
writing the calendar. It invalidates the cache even on failure. The next read confirms
hardware; the supplied timestamp is not inserted into the cache. Writes are
not atomic across transactions. Failure can leave a partial calendar or a frozen
clock (PCF8523/RV-8803); fix the bus and retry the complete `set()`. Pending alarm
and event flags are preserved when time-loss flags are acknowledged.

RV-8803 has no latched calendar snapshot. Reads confirm the seconds register
and retry up to three times, within one second, before returning `kUnstable`.
Other calendars are transferred in one burst using their hardware snapshot or
register-blocking behavior. Hardware connections should provide normal bus
speeds; multi-second transfers cannot produce a reliable RTC snapshot.

## Battery backup

Backup settings are explicit, because factory defaults and board batteries vary:

```cpp
// PCF8523: standard switching with battery-low detection.
if (!pcf.configureBackup(true)) return;

// RV-3028: choose switching mode appropriate to the circuit. No EEPROM writes.
if (!rv.configureBackup(roo_rtc::Rv3028Clock::BackupMode::kDirect)) return;
```

PCF8523 factory defaults disable switching. RV-3028 also defaults to switching
disabled. Its `configureBackup()` changes the volatile RAM configuration,
disables charging, enables fast-edge detection, and disables automatic EEPROM
refresh so the configuration survives midnight. Repeat after power-on reset.
No EEPROM is modified; an error can leave refresh disabled, so retry after
repairing the bus. `kLevel` selects the fixed-threshold mode; `kDisabled` turns
switching off. RV-8803 has no separate automatic battery-switch circuit; the
board must supply continuous power or external backup switching.

Alarms, calibration, temperature, SRAM, event timestamps, and EEPROM programming
are outside this initial calendar-driver API.

## Build and tests

Bazel targets `:i2c` and `:spi` let applications select transports; `:roo_rtc`
aggregates both. `:rtc_test` compiles the production code against scripted
register boundaries. It checks independent datasheet bytes, calendar bounds,
12-hour modes, cache behavior, time loss, every required transaction failure,
short Arduino reads/writes, initialization retry, and RV-8803 rollover.
`:ds3231_integration_test` uses roo_testing's pin-routed device emulator through
the real Arduino/IDF adapters. Tests establish software behavior, not electrical
behavior on physical hardware.

Until dependencies are published, use local overrides from this repository:

```sh
bazel test //... \
  --override_module=roo_io=../roo_io \
  --override_module=roo_time=../roo_time \
  --override_module=roo_testing=../roo_testing
bazel test --config=roo_testing_idf_esp32 //... \
  --override_module=roo_io=../roo_io \
  --override_module=roo_time=../roo_time \
  --override_module=roo_testing=../roo_testing
```

Profiles are vendored from roo_testing. Honor the machine's global Bazel cache
and resource limits; keep output/cache/sandbox directories on persistent disk.

## Migration from roo_time_ds3231

`roo_time_ds3231` forwards its existing include and `roo_time::Ds3231Clock` name
to this library. New code can use `roo_rtc/ds3231.h` and
`roo_rtc::Ds3231Clock` directly. Constructors, `init()`, `now()`, and `set()`
remain source-compatible. Reads now check oscillator-stop flags, years outside
2000–2099 are rejected, and successful writes are confirmed on the next read
instead of directly seeding the cache. This is source, not binary, compatibility.

## Register references

- [DS3231 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf)
- [DS3232 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3232.pdf)
- [DS3234 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3234.pdf)
- [DS1307 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS1307.pdf)
- [PCF8523 datasheet](https://www.nxp.com/docs/en/data-sheet/PCF8523.pdf)
- [RV-3028 application manual](https://www.microcrystal.com/fileadmin/Media/Products/RTC/App.Manual/RV-3028-C7_App-Manual.pdf)
- [RV-8803 application manual](https://www.microcrystal.com/fileadmin/Media/Products/RTC/App.Manual/RV-8803-C7_App-Manual.pdf)
