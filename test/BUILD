load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//:__pkg__"])

cc_test(
    name = "rtc_test",
    size = "small",
    srcs = [
        "registers.h",
        "rtc_test.cpp",
        "//src:driver_sources",
    ] + select({
        "@roo_testing//roo_testing/platforms:is_arduino": glob(["stubs/*.h"]) + [
            "@roo_io//:arduino_i2c_sources",
            "@roo_io//:arduino_spi_sources",
        ],
        "//conditions:default": ["native_boundary.cpp"],
    }),
    includes = ["."] + select({
        "@roo_testing//roo_testing/platforms:is_arduino": ["stubs"],
        "//conditions:default": [],
    }),
    linkstatic = True,
    deps = [
        "//src:driver_headers",
        "@googletest//:gtest_main",
        "@roo_io//:i2c_headers",
        "@roo_io//:spi_headers",
        "@roo_time",
    ] + select({
        "@roo_testing//roo_testing/platforms:is_idf": [
            "@roo_io//:i2c",
            "@roo_io//:spi",
            "@roo_testing//roo_testing/frameworks/esp-idf",
        ],
        "//conditions:default": [],
    }),
)

cc_test(
    name = "ds3231_test",
    size = "small",
    srcs = ["device_test.cpp"],
    defines = [
        "RTC_HEADER=roo_rtc/ds3231.h",
        "FAKE_HEADER=roo_testing/devices/clock/ds3231/ds3231.h",
        "RTC_CLASS=Ds3231Clock",
        "FAKE_CLASS=FakeDs3231",
        "RTC_DEVICE=0",
    ],
    linkstatic = True,
    deps = [
        "//:ds3231",
        "@roo_testing//roo_testing/devices/clock/ds3231",
        "@roo_testing//roo_testing/microcontrollers/esp32:core",
    ] + select({
        "@roo_testing//roo_testing/platforms:is_idf": ["@roo_testing//:esp_idf_gtest_main"],
        "//conditions:default": ["@roo_testing//:arduino_gtest_main"],
    }),
)

cc_test(
    name = "ds3232_test",
    size = "small",
    srcs = ["device_test.cpp"],
    defines = [
        "RTC_HEADER=roo_rtc/ds3232.h",
        "FAKE_HEADER=roo_testing/devices/clock/ds3232/ds3232.h",
        "RTC_CLASS=Ds3232Clock",
        "FAKE_CLASS=FakeDs3232",
        "RTC_DEVICE=1",
    ],
    linkstatic = True,
    deps = [
        "//:ds3232",
        "@roo_testing//roo_testing/devices/clock/ds3232",
        "@roo_testing//roo_testing/microcontrollers/esp32:core",
    ] + select({
        "@roo_testing//roo_testing/platforms:is_idf": ["@roo_testing//:esp_idf_gtest_main"],
        "//conditions:default": ["@roo_testing//:arduino_gtest_main"],
    }),
)

cc_test(
    name = "ds3234_test",
    size = "small",
    srcs = ["device_test.cpp"],
    defines = [
        "RTC_HEADER=roo_rtc/ds3234.h",
        "FAKE_HEADER=roo_testing/devices/clock/ds3234/ds3234.h",
        "RTC_CLASS=Ds3234Clock",
        "FAKE_CLASS=FakeDs3234",
        "RTC_DEVICE=2",
    ],
    linkstatic = True,
    deps = [
        "//:ds3234",
        "@roo_testing//roo_testing/devices/clock/ds3234",
        "@roo_testing//roo_testing/microcontrollers/esp32:core",
    ] + select({
        "@roo_testing//roo_testing/platforms:is_idf": ["@roo_testing//:esp_idf_gtest_main"],
        "//conditions:default": ["@roo_testing//:arduino_gtest_main"],
    }),
)

cc_test(
    name = "ds1307_test",
    size = "small",
    srcs = ["device_test.cpp"],
    defines = [
        "RTC_HEADER=roo_rtc/ds1307.h",
        "FAKE_HEADER=roo_testing/devices/clock/ds1307/ds1307.h",
        "RTC_CLASS=Ds1307Clock",
        "FAKE_CLASS=FakeDs1307",
        "RTC_DEVICE=3",
    ],
    linkstatic = True,
    deps = [
        "//:ds1307",
        "@roo_testing//roo_testing/devices/clock/ds1307",
        "@roo_testing//roo_testing/microcontrollers/esp32:core",
    ] + select({
        "@roo_testing//roo_testing/platforms:is_idf": ["@roo_testing//:esp_idf_gtest_main"],
        "//conditions:default": ["@roo_testing//:arduino_gtest_main"],
    }),
)

cc_test(
    name = "pcf8523_test",
    size = "small",
    srcs = ["device_test.cpp"],
    defines = [
        "RTC_HEADER=roo_rtc/pcf8523.h",
        "FAKE_HEADER=roo_testing/devices/clock/pcf8523/pcf8523.h",
        "RTC_CLASS=Pcf8523Clock",
        "FAKE_CLASS=FakePcf8523",
        "RTC_DEVICE=4",
    ],
    linkstatic = True,
    deps = [
        "//:pcf8523",
        "@roo_testing//roo_testing/devices/clock/pcf8523",
        "@roo_testing//roo_testing/microcontrollers/esp32:core",
    ] + select({
        "@roo_testing//roo_testing/platforms:is_idf": ["@roo_testing//:esp_idf_gtest_main"],
        "//conditions:default": ["@roo_testing//:arduino_gtest_main"],
    }),
)

cc_test(
    name = "rv3028_test",
    size = "small",
    srcs = ["device_test.cpp"],
    defines = [
        "RTC_HEADER=roo_rtc/rv3028.h",
        "FAKE_HEADER=roo_testing/devices/clock/rv3028/rv3028.h",
        "RTC_CLASS=Rv3028Clock",
        "FAKE_CLASS=FakeRv3028",
        "RTC_DEVICE=5",
    ],
    linkstatic = True,
    deps = [
        "//:rv3028",
        "@roo_testing//roo_testing/devices/clock/rv3028",
        "@roo_testing//roo_testing/microcontrollers/esp32:core",
    ] + select({
        "@roo_testing//roo_testing/platforms:is_idf": ["@roo_testing//:esp_idf_gtest_main"],
        "//conditions:default": ["@roo_testing//:arduino_gtest_main"],
    }),
)

cc_test(
    name = "rv8803_test",
    size = "small",
    srcs = ["device_test.cpp"],
    defines = [
        "RTC_HEADER=roo_rtc/rv8803.h",
        "FAKE_HEADER=roo_testing/devices/clock/rv8803/rv8803.h",
        "RTC_CLASS=Rv8803Clock",
        "FAKE_CLASS=FakeRv8803",
        "RTC_DEVICE=6",
    ],
    linkstatic = True,
    deps = [
        "//:rv8803",
        "@roo_testing//roo_testing/devices/clock/rv8803",
        "@roo_testing//roo_testing/microcontrollers/esp32:core",
    ] + select({
        "@roo_testing//roo_testing/platforms:is_idf": ["@roo_testing//:esp_idf_gtest_main"],
        "//conditions:default": ["@roo_testing//:arduino_gtest_main"],
    }),
)
