package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fmt",
    srcs = glob(["fmt/src/*.cc"]),
    hdrs = glob(["fmt/include/fmt/*.h"]),
    includes = ["fmt/include"],
)

cc_library(
    name = "catch",
    srcs =  [ "catch2/catch.cpp" ],
    hdrs = [ "catch2/catch.hpp" ],
    includes = [ "." ],
)

cc_library(
    name = "outcome",
    hdrs = ["outcome/outcome.hpp"],
)
