# Modified from:
# https://github.com/tensorflow/tensorflow/blob/master/third_party/ngraph/nlohmann_json.BUILD

licenses(["notice"])  # 3-Clause BSD

exports_files(["LICENSE.MIT"])

cc_library(
    name = "json",
    hdrs = glob([
        "include/nlohmann/**/*.hpp",
    ]),
    copts = [
        "-I external/nlohmann_json_lib",
    ],
    visibility = ["//visibility:public"],
    alwayslink = 1,
)

cc_library(
    name = "json_single_include",
    hdrs = ["single_include/nlohmann/json.hpp"],
    includes = ["single_include"],
    visibility = ["//visibility:public"],
)