package(default_visibility = ["//guci:__subpackages__" ])

cc_library(
    name = "ast",
    hdrs = [ "ast.hpp" ],
)

cc_library(
    name = "parse",
    hdrs = ["parse.hpp", ] ,
    deps = [
        ":ast",
        ":parse_error",
         "//guci/utils:string_manipulation",
    ],
)

cc_test(
    name = "parse_test",
    srcs = [ "parse.test.cpp" ],
    deps = [
        ":parse"
        ,
        "//third_party:catch",
    ]
)

cc_library(
    name = "parse_error",
    hdrs = [ "parse_error.hpp" ],
    deps = [
        "//guci/utils:outcome",
    ]
)
