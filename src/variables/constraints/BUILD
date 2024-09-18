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

cc_test(
    name = "base_constraints_test",
    srcs = ["base_constraints_test.cc"],
    deps = [
        ":base_constraints",
        "@com_google_googletest//:gtest_main",
        "@absl//absl/strings:string_view",
    ],
)
