licenses(["notice"])

package(default_visibility = ["//visibility:public"])

load("//internal_tools:py_extension.bzl", "py_extension")
#load("@pybind11_bazel//:build_defs.bzl", "pybind_extension")

cc_library(
    name = "HAC_lib",
    srcs = ["HAC_lib.cc"],
    hdrs = ["HAC_lib.h"],
    deps = [
        "//benchmarks/Clustering/SeqHAC:HAC_api",
        "//gbbs",
    ],
)

py_extension(
    name = "gbbs_lib",
    srcs = ["gbbs_lib.cc"],
    deps = [
        ":HAC_lib",
        "//gbbs",
        "//gbbs:compressed_vertex",
        "//gbbs:graph",
        "//gbbs:graph_io",
        "//gbbs:vertex",
        "//gbbs:vertex_subset",
        "@local_config_python//:python_headers",
        "@pybind11",
    ],
)
