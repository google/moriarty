# Test Macros used in Moriarty.

package(
    default_visibility = [
        "//visibility:public"
    ],
)

cc_library(
    name = "status_testutil",
    testonly = True,
    srcs = ["status_testutil.cc"],
    hdrs = ["status_testutil.h"],
    deps = [
        "@com_google_googletest//:gtest",
        "@absl//absl/status",
        "//src/testing:status_test_util",
        "//src/util/status_macro:status_macros",
    ],
)

licenses(["notice"])

cc_test(
    name = "status_testutil_test",
    srcs = ["status_testutil_test.cc"],
    deps = [
        ":status_testutil",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
    ],
)
