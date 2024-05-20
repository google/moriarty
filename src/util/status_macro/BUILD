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

# Status Macros for Moriarty

package(
    default_visibility = [
        "//visibility:public"
    ],
)

licenses(["notice"])

cc_library(
    name = "examine_stack",
    srcs = ["examine_stack.cc"],
    hdrs = ["examine_stack.h"],
    deps = [
        "@absl//absl/base:core_headers",
        "@absl//absl/debugging:stacktrace",
        "@absl//absl/debugging:symbolize",
    ],
)

cc_library(
    name = "ostringstream",
    srcs = ["ostringstream.cc"],
    hdrs = ["ostringstream.h"],
)

cc_test(
    name = "ostringstream_test",
    srcs = ["ostringstream_test.cc"],
    deps = [
        ":ostringstream",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "status_macros",
    hdrs = ["status_macros.h"],
    deps = [
        ":source_location",
        ":status_builder",
        "@absl//absl/base:core_headers",
        "@absl//absl/status",
    ],
)

cc_library(
    name = "source_location",
    hdrs = ["source_location.h"],
)

cc_library(
    name = "status_builder",
    srcs = ["status_builder.cc"],
    hdrs = ["status_builder.h"],
    deps = [
        ":examine_stack",
        ":ostringstream",
        ":source_location",
        "@absl//absl/base:core_headers",
        "@absl//absl/base:log_severity",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/log",
        "@absl//absl/status",
        "@absl//absl/strings",
        "@absl//absl/strings:cord",
        "@absl//absl/strings:string_view",
        "@absl//absl/synchronization",
        "@absl//absl/time",
    ],
)

cc_test(
    name = "status_builder_test",
    size = "small",
    srcs = ["status_builder_test.cc"],
    deps = [
        ":source_location",
        ":status_builder",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/base:log_severity",
        "@absl//absl/log:scoped_mock_log",
        "@absl//absl/status",
    ],
)

cc_test(
    name = "status_macros_test",
    size = "small",
    srcs = ["status_macros_test.cc"],
    deps = [
        ":status_builder",
        ":status_macros",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "@absl//absl/strings:string_view",
    ],
)
