/*
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MORIARTY_DOCS_EXAMPLES_EXAMPLE_GRAPH_H_
#define MORIARTY_DOCS_EXAMPLES_EXAMPLE_GRAPH_H_

#include <utility>
#include <vector>

namespace moriarty_examples {

// This is a simplified class to represent an example Graph for Moriarty.
//
// It does not conform to Google's C++ style guide. Please do not use it beyond
// the codelab! =)
struct ExampleGraph {
  int num_nodes;
  std::vector<std::pair<int, int>> edges;  // 0-based vertex numbers.
};

// Checks if a graph is connected.
bool GraphIsConnected(const ExampleGraph& g);

// < and == operators
bool operator<(const ExampleGraph& G1, const ExampleGraph& G2);
bool operator==(const ExampleGraph& G1, const ExampleGraph& G2);

template <typename H>
H AbslHashValue(H h, const ExampleGraph& G) {
  return H::combine(std::move(h), G.num_nodes, G.edges);
}

}  // namespace moriarty_examples

#endif  // MORIARTY_DOCS_EXAMPLES_EXAMPLE_GRAPH_H_
