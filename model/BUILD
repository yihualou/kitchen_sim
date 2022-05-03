package(default_visibility = ["//visibility:public"])

load("//:variables.bzl", "COPTS")

cc_library(
    name = "courier",
    srcs = ["courier.cc"],
    hdrs = ["courier.h"],
    copts = COPTS,
    deps = [
        ":kitchen",
        ":order",
        "//:base",
        "@absl//absl/strings",
    ],
)

cc_test(
    name = "courier_test",
    srcs = ["courier_test.cc"],
    copts = COPTS,
    deps = [
        ":courier",
        ":kitchen",
        "@gtest",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "kitchen",
    srcs = ["kitchen.cc"],
    hdrs = ["kitchen.h"],
    copts = COPTS,
    deps = [
        ":order",
        "//:base",
        "@absl//absl/strings",
        "@absl//absl/time",
        "@boost//:log",
    ],
)

cc_test(
    name = "kitchen_test",
    srcs = ["kitchen_test.cc"],
    copts = COPTS,
    deps = [
        ":kitchen",
        "@gtest",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "order",
    srcs = ["order.cc"],
    hdrs = ["order.h"],
    copts = COPTS,
    deps = [
        "//:base",
        "@absl//absl/strings",
        "@absl//absl/time",
    ],
)

cc_test(
    name = "order_test",
    srcs = ["order_test.cc"],
    copts = COPTS,
    deps = [
        ":order",
        "@gtest",
        "@gtest//:gtest_main",
    ],
)
