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
        "//visibility:public"
    ],
)

licenses(["notice"])

cc_library(
    name = "errors",
    srcs = ["errors.cc"],
    hdrs = ["errors.h"],
    deps = [
        "@absl//absl/log:absl_check",
        "@absl//absl/status",
        "@absl//absl/strings",
        "@absl//absl/strings:cord",
    ],
)

cc_library(
    name = "exporter",
    srcs = [
        "exporter.cc",
    ],
    hdrs = [
        "exporter.h",
    ],
    deps = [
        ":errors",
        ":test_case",
        "@absl//absl/log:absl_check",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//src/internal:abstract_variable",
        "//src/internal:status_utils",
        "//src/internal:universe",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:io_config",
        "//src/librarian:mvariable",
    ],
)

cc_library(
    name = "generator",
    srcs = [
        "generator.cc",
    ],
    hdrs = [
        "generator.h",
    ],
    deps = [
        ":errors",
        ":property",
        ":scenario",
        ":test_case",
        "@absl//absl/base:nullability",
        "@absl//absl/log:check",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "@absl//absl/types:span",
        "//src/internal:analysis_bootstrap",
        "//src/internal:generation_config",
        "//src/internal:random_config",
        "//src/internal:random_engine",
        "//src/internal:status_utils",
        "//src/internal:universe",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:mvariable",
        "//src/util/status_macro:status_macros",
    ],
)

cc_library(
    name = "importer",
    srcs = ["importer.cc"],
    hdrs = ["importer.h"],
    deps = [
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//src/internal:abstract_variable",
        "//src/internal:analysis_bootstrap",
        "//src/internal:universe",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:io_config",
        "//src/librarian:mvariable",
        "//src/util/status_macro:status_macros",
    ],
)

cc_library(
    name = "moriarty",
    srcs = [
        "moriarty.cc",
    ],
    hdrs = [
        "moriarty.h",
    ],
    deps = [
        ":exporter",
        ":generator",
        ":importer",
        ":test_case",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "@absl//absl/types:span",
        "//src/internal:status_utils",
        "//src/internal:universe",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:mvariable",
        "//src/util/status_macro:status_macros",
    ],
)

cc_library(
    name = "property",
    srcs = [
        "property.cc",
    ],
    hdrs = [
        "property.h",
    ],
    deps = ["@absl//absl/strings"],
)

cc_library(
    name = "scenario",
    srcs = ["scenario.cc"],
    hdrs = [
        "scenario.h",
    ],
    deps = [
        ":property",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/strings",
    ],
)

cc_library(
    name = "simple_io",
    srcs = ["simple_io.cc"],
    hdrs = ["simple_io.h"],
    deps = [
        ":exporter",
        ":importer",
        "@absl//absl/log:absl_check",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "@absl//absl/types:span",
        "//src/internal:abstract_variable",
        "//src/librarian:io_config",
        "//src/util/status_macro:status_macros",
    ],
)

cc_library(
    name = "test_case",
    srcs = [
        "test_case.cc",
    ],
    hdrs = [
        "test_case.h",
    ],
    deps = [
        ":scenario",
        "@absl//absl/log:absl_check",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//src/internal:abstract_variable",
        "//src/internal:generation_bootstrap",
        "//src/internal:random_engine",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:mvariable",
        "//src/util/status_macro:status_macros",
    ],
)

cc_test(
    name = "errors_test",
    srcs = ["errors_test.cc"],
    deps = [
        ":errors",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
    ],
)

cc_test(
    name = "exporter_test",
    size = "small",
    srcs = [
        "exporter_test.cc",
    ],
    deps = [
        ":errors",
        ":exporter",
        ":test_case",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/algorithm:container",
        "@absl//absl/log:absl_check",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "//src/internal:abstract_variable",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:io_config",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
    ],
)

cc_test(
    name = "generator_test",
    size = "small",
    srcs = [
        "generator_test.cc",
    ],
    deps = [
        ":errors",
        ":generator",
        ":scenario",
        ":test_case",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/log:absl_check",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/types:span",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:size_property",
        "//src/librarian:test_utils",
        "//src/testing:generator_test_util",
        "//src/testing:mtest_type",
        "//src/testing:random_test_util",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
    ],
)

cc_test(
    name = "importer_test",
    srcs = ["importer_test.cc"],
    deps = [
        ":importer",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/librarian:io_config",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
        "//src/variables:mstring",
    ],
)

cc_test(
    name = "moriarty_test",
    size = "small",
    srcs = [
        "moriarty_test.cc",
    ],
    deps = [
        ":moriarty",
        ":test_case",
        "@com_google_googletest//:gtest_main",
        "//src/testing:exporter_test_util",
        "//src/testing:generator_test_util",
        "//src/testing:importer_test_util",
        "//src/testing:mtest_type",
        "//src/testing:status_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
    ],
)

cc_test(
    name = "simple_io_test",
    srcs = ["simple_io_test.cc"],
    deps = [
        ":exporter",
        ":importer",
        ":simple_io",
        ":test_case",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/log:absl_check",
        "@absl//absl/status",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/testing:exporter_test_util",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
    ],
)

cc_test(
    name = "scenario_test",
    srcs = ["scenario_test.cc"],
    deps = [
        ":property",
        ":scenario",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_case_test",
    srcs = ["test_case_test.cc"],
    deps = [
        ":scenario",
        ":test_case",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/types:span",
        "//src/internal:random_engine",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/testing:mtest_type",
        "//src/testing:mtest_type2",
        "//src/testing:status_test_util",
        "//src/util/status_macro:status_macros",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables:minteger",
    ],
)

cc_test(
    name = "constraint_values_test",
    srcs = ["constraint_values_test.cc"],
    deps = [
        ":constraint_values",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/log:absl_check",
        "//src/internal:universe",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/testing:mtest_type",
        "//src/variables:minteger",
        "//src/variables:mstring",
    ],
)

cc_library(
    name = "constraint_values",
    hdrs = ["constraint_values.h"],
    deps = [
        "@absl//absl/log:absl_check",
        "@absl//absl/status:statusor",
        "//src/internal:universe",
    ],
)
