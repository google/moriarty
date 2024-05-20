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

# Several classes in Moriarty must be derived to work. These classes provide simple
# derived classes to be used for testing.

package(
    default_visibility = [
        "//visibility:public"
    ],
)

licenses(["notice"])

cc_library(
    name = "exporter_test_util",
    testonly = True,
    hdrs = [
        "exporter_test_util.h",
    ],
    deps = [
        ":mtest_type",
        "@absl//absl/status:statusor",
        "//src:exporter",
        "//src:test_case",
        "//src/internal:value_set",
        "//src/util/status_macro:status_macros",
        "//src/variables:minteger",
        "//src/variables:mstring",
    ],
)

cc_library(
    name = "generator_test_util",
    testonly = True,
    hdrs = [
        "generator_test_util.h",
    ],
    deps = [
        ":mtest_type",
        "@absl//absl/status:statusor",
        "@absl//absl/types:span",
        "//src:generator",
        "//src:test_case",
        "//src/internal:value_set",
        "//src/internal:variable_set",
        "//src/variables:minteger",
        "//src/variables:mstring",
    ],
)

cc_library(
    name = "importer_test_util",
    testonly = True,
    hdrs = [
        "importer_test_util.h",
    ],
    deps = [
        "@absl//absl/log:absl_check",
        "@absl//absl/status",
        "//src:importer",
        "//src/variables:minteger",
    ],
)

cc_library(
    name = "mtest_type",
    testonly = True,
    srcs = [
        "mtest_type.cc",
    ],
    hdrs = [
        "mtest_type.h",
    ],
    deps = [
        "@absl//absl/log:absl_check",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//src:errors",
        "//src:property",
        "//src/librarian:io_config",
        "//src/librarian:mvariable",
        "//src/librarian:subvalues",
        "//src/util/status_macro:status_macros",
        "//src/variables:minteger",
    ],
)

cc_library(
    name = "mtest_type2",
    testonly = True,
    srcs = [
        "mtest_type2.cc",
    ],
    hdrs = [
        "mtest_type2.h",
    ],
    deps = [
        "@absl//absl/log:absl_check",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//src:errors",
        "//src:property",
        "//src/librarian:io_config",
        "//src/librarian:mvariable",
        "//src/librarian:subvalues",
        "//src/util/status_macro:status_macros",
        "//src/variables:minteger",
    ],
)

cc_library(
    name = "random_test_util",
    testonly = True,
    hdrs = [
        "random_test_util.h",
    ],
    deps = [
        "@com_google_googletest//:gtest",
        "@absl//absl/algorithm:container",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "//src/util/test_status_macro:status_testutil",
    ],
)

cc_library(
    name = "status_test_util",
    testonly = True,
    hdrs = [
        "status_test_util.h",
    ],
    deps = [
        "@com_google_googletest//:gtest",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//src:errors",
    ],
)
