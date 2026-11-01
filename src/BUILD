load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = [
    "//:__pkg__",
    "//test:__pkg__",
])

exports_files(
    glob([
        "roo_rtc/*.h",
        "roo_rtc/*.cpp",
        "*.h",
    ]),
    visibility = ["//:__pkg__"],
)

cc_library(
    name = "calendar",
    srcs = [
        "roo_rtc/clock.cpp",
        "roo_rtc/internal/calendar.cpp",
    ],
    hdrs = [
        "roo_rtc/clock.h",
        "roo_rtc/internal/calendar.h",
    ],
    includes = ["."],
    deps = ["@roo_time"],
)

cc_library(
    name = "i2c_clock",
    srcs = ["roo_rtc/internal/i2c_clock.cpp"],
    hdrs = ["roo_rtc/internal/i2c_clock.h"],
    includes = ["."],
    deps = [
        ":calendar",
        "@roo_io//:i2c",
    ],
)

# The scripted boundary tests compile the same production sources with only
# the transport API substituted. These labels are test-only and private.
filegroup(
    name = "driver_sources",
    testonly = True,
    srcs = glob(["**/*.cpp"]),
)

cc_library(
    name = "driver_headers",
    testonly = True,
    hdrs = glob(["**/*.h"]),
    includes = ["."],
)
