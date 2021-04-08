package(default_visibility = ["//guci:__subpackages__" ])

cc_library(
    name = "actions",
    hdrs = ["actions.hpp"],
    deps = [
        "//guci/parse:ast",
    ]
)

cc_library(
    name = "ast_eval_utils",
    hdrs = ["ast_eval_utils.hpp"],
    deps = [
        ":eval",
        ":eval_error",
        "//guci/parse:ast",
    ],
)

cc_test(
    name = "ast_eval_utils_test",
    srcs = [ "ast_eval_utils.test.cpp" ],
    deps = [
        ":eval_ast_utils",
        "//guci/parse:parse",
        "//third_party:catch",
    ],
)

cc_library(
    name = "eval",
    hdrs = [ "eval.hpp" ],
    deps = [
        ":eval_result",
        "//guci/parse:ast",
    ]
)

cc_library(
    name = "eval_result",
    hdrs = [ "eval_result.hpp" ],
    deps = [
        ":actions",
        ":eval_error",
    ],
)

cc_library(
    name = "eval_error",
    hdrs = [ "eval_error.hpp" ],
    deps = [
        "//guci/utils:outcome",
    ]
)
