# Copyright 2024 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# The core parts of the Moriarty library.

package(
    default_visibility = [
        "//:internal",
    ],
)

licenses(["notice"])

cc_library(
    name = "abstract_variable",
    hdrs = [
        "abstract_variable.h",
    ],
    deps = [
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//src:property",
    ],
)

cc_library(
    name = "analysis_bootstrap",
    hdrs = ["analysis_bootstrap.h"],
    deps = [
        ":universe",
        ":value_set",
        ":variable_set",
        "@absl//absl/status",
        "@absl//absl/strings",
        "//src/librarian:mvariable",
    ],
)

cc_library(
    name = "combinatorial_coverage",
    srcs = ["combinatorial_coverage.cc"],
    hdrs = ["combinatorial_coverage.h"],
    deps = [
        "@absl//absl/algorithm:container",
        "@absl//absl/log:absl_check",
        "@absl//absl/types:span",
    ],
)

cc_library(
    name = "expressions",
    srcs = [
        "expressions.cc",
    ],
    hdrs = [
        "expressions.h",
    ],
    deps = [
        "@absl//absl/algorithm:container",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/log:absl_check",
        "@absl//absl/numeric:int128",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "@absl//absl/types:variant",
        "//src/util/status_macro:status_macros",
    ],
)

cc_library(
    name = "generation_config",
    srcs = ["generation_config.cc"],
    hdrs = ["generation_config.h"],
    deps = [
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/meta:type_traits",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
    ],
)

cc_library(
    name = "random_engine",
    srcs = [
        "random_engine.cc",
    ],
    hdrs = [
        "random_engine.h",
    ],
    deps = [
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "@absl//absl/types:span",
    ],
)

cc_library(
    name = "range",
    srcs = [
        "range.cc",
    ],
    hdrs = [
        "range.h",
    ],
    deps = [
        ":expressions",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "@absl//absl/types:span",
        "//src/util/status_macro:status_macros",
    ],
)

cc_library(
    name = "simple_pattern",
    srcs = ["simple_pattern.cc"],
    hdrs = ["simple_pattern.h"],
    deps = [
        ":random_config",
        ":random_engine",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "@absl//absl/strings:string_view",
        "//src/util/status_macro:status_macros",
    ],
)

cc_library(
    name = "status_utils",
    srcs = ["status_utils.cc"],
    hdrs = ["status_utils.h"],
    deps = [
        "@absl//absl/base:nullability",
        "@absl//absl/log:absl_check",
        "@absl//absl/log:absl_log",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
    ],
)

cc_library(
    name = "universe",
    srcs = [
        "universe.cc",
    ],
    hdrs = [
        "universe.h",
    ],
    deps = [
        ":abstract_variable",
        ":generation_config",
        ":random_config",
        ":random_engine",
        ":value_set",
        ":variable_name_utils",
        ":variable_set",
        "@absl//absl/base:nullability",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/log:absl_check",
        "@absl//absl/log:check",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//src:errors",
        "//src/librarian:io_config",
        "//src/util/status_macro:status_macros",
    ],
)

cc_library(
    name = "value_set",
    srcs = ["value_set.cc"],
    hdrs = ["value_set.h"],
    deps = [
        ":abstract_variable",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//src:errors",
    ],
)

cc_library(
    name = "variable_set",
    srcs = [
        "variable_set.cc",
    ],
    hdrs = [
        "variable_set.h",
    ],
    deps = [
        ":abstract_variable",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/meta:type_traits",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//src:errors",
        "//src:property",
        "//src:scenario",
        "//src/util/status_macro:status_macros",
    ],
)

cc_test(
    name = "analysis_bootstrap_test",
    srcs = ["analysis_bootstrap_test.cc"],
    deps = [
        ":analysis_bootstrap",
        ":value_set",
        ":variable_set",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
    ],
)

cc_library(
    name = "combinatorial_coverage_test_util",
    testonly = True,
    hdrs = ["combinatorial_coverage_test_util.h"],
    deps = [
        ":combinatorial_coverage",
        "@com_google_googletest//:gtest",
        "@absl//absl/log:absl_log",
    ],
)

cc_test(
    name = "combinatorial_coverage_test",
    srcs = ["combinatorial_coverage_test.cc"],
    deps = [
        ":combinatorial_coverage",
        ":combinatorial_coverage_test_util",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/strings",
    ],
)

cc_test(
    name = "expressions_test",
    size = "small",
    srcs = [
        "expressions_test.cc",
    ],
    deps = [
        ":expressions",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//src/util/status_macro:status_macros",
        "//src/util/test_status_macro:status_testutil",
    ],
)

cc_test(
    name = "generation_config_test",
    srcs = ["generation_config_test.cc"],
    deps = [
        ":generation_config",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "//src/util/test_status_macro:status_testutil",
    ],
)

cc_test(
    name = "random_engine_test",
    size = "small",
    srcs = [
        "random_engine_test.cc",
    ],
    deps = [
        ":random_engine",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/types:span",
        "//src/util/status_macro:status_macros",
        "//src/util/test_status_macro:status_testutil",
    ],
)

cc_test(
    name = "range_test",
    size = "small",
    srcs = [
        "range_test.cc",
    ],
    deps = [
        ":range",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "//src/util/test_status_macro:status_testutil",
    ],
)

cc_test(
    name = "simple_pattern_test",
    srcs = ["simple_pattern_test.cc"],
    deps = [
        ":random_engine",
        ":simple_pattern",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings:string_view",
        "//src/util/test_status_macro:status_testutil",
    ],
)

cc_test(
    name = "status_utils_test",
    srcs = ["status_utils_test.cc"],
    deps = [
        ":status_utils",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
    ],
)

cc_test(
    name = "universe_test",
    size = "small",
    srcs = [
        "universe_test.cc",
    ],
    deps = [
        ":abstract_variable",
        ":generation_config",
        ":random_config",
        ":random_engine",
        ":universe",
        ":value_set",
        ":variable_name_utils",
        ":variable_set",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "@absl//absl/types:span",
        "//src:errors",
        "//src/librarian:io_config",
        "//src/testing:mtest_type",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
    ],
)

cc_test(
    name = "value_set_test",
    srcs = ["value_set_test.cc"],
    deps = [
        ":value_set",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:marray",
        "//src/variables:minteger",
        "//src/variables:mstring",
    ],
)

cc_test(
    name = "variable_set_test",
    size = "small",
    srcs = [
        "variable_set_test.cc",
    ],
    deps = [
        ":abstract_variable",
        ":generation_bootstrap",
        ":random_engine",
        ":universe",
        ":value_set",
        ":variable_set",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "@absl//absl/types:span",
        "//src:errors",
        "//src:scenario",
        "//src/librarian:test_utils",
        "//src/testing:mtest_type",
        "//src/testing:mtest_type2",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
    ],
)

cc_library(
    name = "random_config",
    srcs = ["random_config.cc"],
    hdrs = ["random_config.h"],
    deps = [
        ":random_engine",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/log:check",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//src/util/status_macro:status_macros",
    ],
)

cc_test(
    name = "random_config_test",
    srcs = ["random_config_test.cc"],
    deps = [
        ":random_config",
        ":random_engine",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/types:span",
        "//src/testing:random_test_util",
    ],
)

cc_library(
    name = "generation_bootstrap",
    srcs = ["generation_bootstrap.cc"],
    hdrs = ["generation_bootstrap.h"],
    deps = [
        ":abstract_variable",
        ":generation_config",
        ":random_engine",
        ":universe",
        ":value_set",
        ":variable_set",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//src/util/status_macro:status_macros",
    ],
)

cc_test(
    name = "generation_bootstrap_test",
    srcs = ["generation_bootstrap_test.cc"],
    deps = [
        ":generation_bootstrap",
        ":random_engine",
        ":value_set",
        ":variable_set",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/algorithm:container",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/status",
        "@absl//absl/types:span",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
        "//src/variables:mstring",
    ],
)

cc_library(
    name = "variable_name_utils",
    srcs = ["variable_name_utils.cc"],
    hdrs = ["variable_name_utils.h"],
    deps = ["@absl//absl/strings"],
)

cc_test(
    name = "variable_name_utils_test",
    srcs = ["variable_name_utils_test.cc"],
    deps = [
        ":variable_name_utils",
        "@com_google_googletest//:gtest_main",
    ],
)
