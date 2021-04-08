cc_binary(
    name = "gucilisp",
    srcs = ["main.cpp"],
    deps = [
        "//guci/eval:ast_eval_utils",
        "//guci/eval:eval",
        "//guci/eval:eval_error",
        "//guci/parse:ast",
        "//guci/parse:parse",
        "//guci/utils:outcome",
        "//guci/utils:overload",
        "//guci/utils:string_manipulation",
        "//third_party:fmt",
    ],
    linkopts = [
        "-lreadline"
    ]
)

cc_binary(
    name = "fmt_integration_test",
    srcs = ["fmt_integration_test.cpp"],
    deps = [
        "//third_party:fmt",
    ],
)
