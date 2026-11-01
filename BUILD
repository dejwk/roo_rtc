load("@rules_cc//cc:cc_library.bzl", "cc_library")

# Device drivers are the public build surface. Shared implementation lives in
# //src with visibility restricted to this package and the test package.

cc_library(
    name = "ds3231",
    srcs = ["//src:roo_rtc/ds3231.cpp"],
    hdrs = ["//src:roo_rtc/ds3231.h"],
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = ["//src:i2c_clock"],
)

cc_library(
    name = "ds3232",
    srcs = ["//src:roo_rtc/ds3232.cpp"],
    hdrs = ["//src:roo_rtc/ds3232.h"],
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = ["//src:i2c_clock"],
)

cc_library(
    name = "ds3234",
    srcs = ["//src:roo_rtc/ds3234.cpp"],
    hdrs = ["//src:roo_rtc/ds3234.h"],
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = [
        "//src:calendar",
        "@roo_io//:spi",
    ],
)

cc_library(
    name = "ds1307",
    srcs = ["//src:roo_rtc/ds1307.cpp"],
    hdrs = ["//src:roo_rtc/ds1307.h"],
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = ["//src:i2c_clock"],
)

cc_library(
    name = "pcf8523",
    srcs = ["//src:roo_rtc/pcf8523.cpp"],
    hdrs = ["//src:roo_rtc/pcf8523.h"],
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = ["//src:i2c_clock"],
)

cc_library(
    name = "rv3028",
    srcs = ["//src:roo_rtc/rv3028.cpp"],
    hdrs = ["//src:roo_rtc/rv3028.h"],
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = ["//src:i2c_clock"],
)

cc_library(
    name = "rv8803",
    srcs = ["//src:roo_rtc/rv8803.cpp"],
    hdrs = ["//src:roo_rtc/rv8803.h"],
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = ["//src:i2c_clock"],
)

cc_library(
    name = "roo_rtc",
    hdrs = ["//src:roo_rtc.h"],
    includes = ["src"],
    visibility = ["//visibility:public"],
    deps = [
        ":ds1307",
        ":ds3231",
        ":ds3232",
        ":ds3234",
        ":pcf8523",
        ":rv3028",
        ":rv8803",
    ],
)

# Retain the first release's test entry points.
alias(
    name = "rtc_test",
    actual = "//test:rtc_test",
)

alias(
    name = "ds3231_integration_test",
    actual = "//test:ds3231_test",
)
