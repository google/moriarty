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

# Example code from the Moriarty codelabs.

package(
    default_visibility = [
        "//visibility:public"
    ],
)

licenses(["notice"])

cc_binary(
    name = "example_graph_main",
    srcs = ["example_graph_main.cc"],
    deps = [
        ":example_graph",
        ":mexample_graph",
        "//src:exporter",
        "//src:generator",
        "//src:moriarty",
        "//src/variables:minteger",
    ],
)

cc_library(
    name = "example_graph",
    srcs = ["example_graph.cc"],
    hdrs = ["example_graph.h"],
)

cc_library(
    name = "mexample_graph",
    srcs = ["mexample_graph.cc"],
    hdrs = ["mexample_graph.h"],
    deps = [
        ":example_graph",
        "@absl//absl/status",
        "@absl//absl/status:statusor",
        "//src:errors",
        "//src/librarian:mvariable",
        "//src/util/status_macro:status_macros",
        "//src/variables:minteger",
    ],
)
