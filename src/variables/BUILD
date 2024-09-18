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

# The core variables that are a part of the Moriarty library.

package(
    default_visibility = [
        "//visibility:public"
    ],
)

licenses(["notice"])

cc_library(
    name = "marray",
    hdrs = ["marray.h"],
    deps = [
        ":minteger",
        "@absl//absl/algorithm:container",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/log:check",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//src:errors",
        "//src:property",
        "//src/librarian:io_config",
        "//src/librarian:mvariable",
        "//src/util/status_macro:status_macros",
        "//src/variables/constraints:base_constraints",
        "//src/variables/constraints:container_constraints",
        "//src/variables/constraints:io_constraints",
        "//src/variables/constraints:size_constraints",
    ],
)

cc_library(
    name = "minteger",
    srcs = [
        "minteger.cc",
    ],
    hdrs = [
        "minteger.h",
    ],
    deps = [
        "@absl//absl/algorithm:container",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "@absl//absl/types:span",
        "//src:errors",
        "//src:property",
        "//src/internal:random_engine",
        "//src/internal:range",
        "//src/librarian:io_config",
        "//src/librarian:mvariable",
        "//src/librarian:size_property",
        "//src/util/status_macro:status_macros",
        "//src/variables/constraints:base_constraints",
        "//src/variables/constraints:numeric_constraints",
        "//src/variables/constraints:size_constraints",
    ],
)

cc_library(
    name = "mstring",
    srcs = [
        "mstring.cc",
    ],
    hdrs = [
        "mstring.h",
    ],
    deps = [
        ":minteger",
        "@absl//absl/algorithm:container",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/log:absl_check",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//src:errors",
        "//src:property",
        "//src/internal:random_engine",
        "//src/internal:simple_pattern",
        "//src/librarian:io_config",
        "//src/librarian:mvariable",
        "//src/util/status_macro:status_macros",
        "//src/variables/constraints:base_constraints",
        "//src/variables/constraints:container_constraints",
        "//src/variables/constraints:size_constraints",
        "//src/variables/constraints:string_constraints",
    ],
)

cc_library(
    name = "mtuple",
    hdrs = ["mtuple.h"],
    deps = [
        "@absl//absl/algorithm:container",
        "@absl//absl/log:check",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings",
        "//src:property",
        "//src/librarian:io_config",
        "//src/librarian:mvariable",
        "//src/util/status_macro:status_macros",
    ],
)

cc_test(
    name = "marray_test",
    srcs = ["marray_test.cc"],
    deps = [
        ":marray",
        ":minteger",
        ":mstring",
        ":mtuple",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/status",
        "//src/librarian:io_config",
        "//src/librarian:mvariable",
        "//src/librarian:test_utils",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables/constraints:base_constraints",
        "//src/variables/constraints:container_constraints",
        "//src/variables/constraints:io_constraints",
        "//src/variables/constraints:numeric_constraints",
    ],
)

cc_test(
    name = "minteger_test",
    size = "small",
    srcs = [
        "minteger_test.cc",
    ],
    deps = [
        ":minteger",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "//src/librarian:size_property",
        "//src/librarian:test_utils",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables/constraints:base_constraints",
        "//src/variables/constraints:numeric_constraints",
        "//src/variables/constraints:size_constraints",
    ],
)

cc_test(
    name = "mstring_test",
    size = "small",
    timeout = "moderate",
    srcs = [
        "mstring_test.cc",
    ],
    deps = [
        ":minteger",
        ":mstring",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/container:flat_hash_set",
        "@absl//absl/status",
        "@absl//absl/strings",
        "//src/librarian:mvariable",
        "//src/librarian:test_utils",
        "//src/util/test_status_macro:status_testutil",
        "//src/variables/constraints:container_constraints",
        "//src/variables/constraints:numeric_constraints",
        "//src/variables/constraints:string_constraints",
    ],
)

cc_test(
    name = "mtuple_test",
    srcs = ["mtuple_test.cc"],
    deps = [
        ":minteger",
        ":mstring",
        ":mtuple",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "//src/librarian:mvariable",
        "//src/librarian:test_utils",
        "//src/util/test_status_macro:status_testutil",
    ],
)
