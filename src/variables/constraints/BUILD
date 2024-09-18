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
    name = "base_constraints",
    srcs = ["base_constraints.cc"],
    hdrs = ["base_constraints.h"],
    deps = [
        "@absl//absl/strings:string_view",
    ],
)

cc_library(
    name = "numeric_constraints",
    srcs = ["numeric_constraints.cc"],
    hdrs = ["numeric_constraints.h"],
    deps = [
        ":base_constraints",
        "@absl//absl/strings:string_view",
        "//src/internal:range",
    ],
)

cc_library(
    name = "container_constraints",
    srcs = ["container_constraints.cc"],
    hdrs = ["container_constraints.h"],
    deps = [
        ":base_constraints",
        "@absl//absl/strings:string_view",
        "//src/variables:minteger",
    ],
)

cc_library(
    name = "io_constraints",
    srcs = ["io_constraints.cc"],
    hdrs = ["io_constraints.h"],
    deps = [
        ":base_constraints",
        "//src/librarian:io_config",
    ],
)

cc_library(
    name = "size_constraints",
    srcs = ["size_constraints.cc"],
    hdrs = ["size_constraints.h"],
    deps = [
        ":base_constraints",
        "@absl//absl/strings:string_view",
        "//src/librarian:size_property",
    ],
)

cc_library(
    name = "string_constraints",
    srcs = ["string_constraints.cc"],
    hdrs = ["string_constraints.h"],
    deps = [
        ":base_constraints",
        "@absl//absl/strings:string_view",
    ],
)

cc_test(
    name = "base_constraints_test",
    srcs = ["base_constraints_test.cc"],
    deps = [
        ":base_constraints",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/strings:string_view",
    ],
)

cc_test(
    name = "container_constraints_test",
    srcs = ["container_constraints_test.cc"],
    deps = [
        ":container_constraints",
        ":numeric_constraints",
        "@com_google_googletest//:gtest_main",
        "//src/librarian:test_utils",
        "//src/variables:minteger",
    ],
)

cc_test(
    name = "io_constraints_test",
    srcs = ["io_constraints_test.cc"],
    deps = [
        ":io_constraints",
        "@com_google_googletest//:gtest_main",
        "//src/librarian:io_config",
    ],
)

cc_test(
    name = "numeric_constraints_test",
    srcs = ["numeric_constraints_test.cc"],
    deps = [
        ":numeric_constraints",
        "@com_google_googletest//:gtest_main",
        "//src/internal:range",
    ],
)

cc_test(
    name = "size_constraints_test",
    srcs = ["size_constraints_test.cc"],
    deps = [
        ":size_constraints",
        "@com_google_googletest//:gtest_main",
        "//src/librarian:size_property",
    ],
)

cc_test(
    name = "string_constraints_test",
    srcs = ["string_constraints_test.cc"],
    deps = [
        ":string_constraints",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/container:flat_hash_set",
    ],
)
