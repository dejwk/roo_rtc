load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "calendar",
    srcs = [
        "src/roo_rtc/clock.cpp",
        "src/roo_rtc/internal/calendar.cpp",
    ],
    hdrs = [
        "src/roo_rtc/clock.h",
        "src/roo_rtc/internal/calendar.h",
    ],
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = ["@roo_time"],
)

cc_library(
    name = "i2c",
    srcs = glob(
        ["src/roo_rtc/*.cpp"],
        exclude = [
            "src/roo_rtc/clock.cpp",
            "src/roo_rtc/ds3234.cpp",
        ],
    ) + ["src/roo_rtc/internal/i2c_clock.cpp"],
    hdrs = glob(
        ["src/roo_rtc/*.h"],
        exclude = [
            "src/roo_rtc/clock.h",
            "src/roo_rtc/ds3234.h",
        ],
    ) + ["src/roo_rtc/internal/i2c_clock.h"],
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = [
        ":calendar",
        "@roo_io//:i2c",
    ],
)

cc_library(
    name = "spi",
    srcs = ["src/roo_rtc/ds3234.cpp"],
    hdrs = ["src/roo_rtc/ds3234.h"],
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = [
        ":calendar",
        "@roo_io//:spi",
    ],
)

cc_library(
    name = "roo_rtc",
    hdrs = ["src/roo_rtc.h"],
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = [
        ":i2c",
        ":spi",
    ],
)

# Compile production drivers/adapters against scripted transport boundaries.
# Native tests use actual IDF public headers; Arduino substitutes only Wire/SPI.
cc_test(
    name = "rtc_test",
    size = "small",
    srcs = glob([
        "src/**/*.cpp",
        "src/**/*.h",
    ]) + [
        "test/registers.h",
        "test/rtc_test.cpp",
    ] + select({
        "@roo_testing//roo_testing/platforms:is_arduino": glob(["test/stubs/*.h"]) + [
            "@roo_io//:arduino_i2c_sources",
            "@roo_io//:arduino_spi_sources",
        ],
        "//conditions:default": ["test/native_boundary.cpp"],
    }),
    includes = [
        "src",
        "test",
    ] + select({
        "@roo_testing//roo_testing/platforms:is_arduino": ["test/stubs"],
        "//conditions:default": [],
    }),
    linkstatic = True,
    deps = [
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
    name = "ds3231_integration_test",
    size = "small",
    srcs = ["test/ds3231_integration_test.cpp"],
    linkstatic = True,
    deps = [
        ":i2c",
        "@roo_testing//roo_testing/devices/clock/ds3231",
        "@roo_testing//roo_testing/microcontrollers/esp32:core",
    ] + select({
        "@roo_testing//roo_testing/platforms:is_idf": ["@roo_testing//:esp_idf_gtest_main"],
        "//conditions:default": ["@roo_testing//:arduino_gtest_main"],
    }),
)
