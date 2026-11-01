load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "simple",
    srcs = ["main.cpp"],
    linkstatic = True,
    target_compatible_with = select({
        "@roo_testing//roo_testing/platforms:is_idf": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    deps = [
        "//:i2c",
        "@roo_testing//:esp_idf_main",
        "@roo_testing//roo_testing/devices/clock/ds3231",
        "@roo_testing//roo_testing/microcontrollers/esp32:core",
    ],
)
