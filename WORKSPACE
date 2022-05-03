load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

git_repository(
    name = "gflags",
    # tag    = "v2.2.2",
    commit = "e171aa2d15ed9eb17054558e0b3a6a413bb01067",
    remote = "https://github.com/gflags/gflags.git",
)

git_repository(
    name = "gtest",
    branch = "v1.10.x",
    remote = "https://github.com/google/googletest.git",
)

git_repository(
    name = "absl",
    branch = "20200225.2",
    remote = "https://github.com/abseil/abseil-cpp.git",
)

http_archive(
    name = "nlohmann_json_lib",
    build_file = "@//:nlohmann_json.BUILD",
    sha256 = "249548f4867417d66ae46b338dfe0a2805f3323e81c9e9b83c89f3adbfde6f31",
    strip_prefix = "json-3.7.3",
    urls = ["https://github.com/nlohmann/json/archive/v3.7.3.tar.gz"],
)

_RULES_BOOST_COMMIT = "32164a62e2472077320f48f52b8077207cd0c9c8"

http_archive(
    name = "com_github_nelhage_rules_boost",
    sha256 = "ce95d0705592e51eda91b38870e847c303f65219871683f7c34233caad150b0b",
    strip_prefix = "rules_boost-%s" % _RULES_BOOST_COMMIT,
    urls = [
        "https://github.com/nelhage/rules_boost/archive/%s.tar.gz" % _RULES_BOOST_COMMIT,
    ],
)

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
boost_deps()
