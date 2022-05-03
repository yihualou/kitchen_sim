load("//:variables.bzl", "COPTS")

cc_library(
    name = "base",
    hdrs = ["base.h"],
    copts = COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "@boost//:asio",
        "@boost//:fiber",
    ],
)

cc_binary(
    name = "kitchen_sim",
    srcs = ["kitchen_sim.cc"],
    copts = COPTS,
    deps = [
        ":kitchen_sim_lib",
        "@gflags",
    ],
)

cc_library(
    name = "kitchen_sim_lib",
    srcs = ["kitchen_sim_lib.cc"],
    hdrs = [
        "kitchen_sim_lib.h",
    ],
    copts = COPTS,
    deps = [
        "//model:courier",
        "//model:kitchen",
        "@absl//absl/strings",
        "@boost//:log",
        "@gflags",
        "@nlohmann_json_lib//:json_single_include",
    ],
)
