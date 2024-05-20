// Copyright 2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "docs/examples/example_graph.h"

#include <numeric>
#include <tuple>
#include <vector>

namespace moriarty_examples {

// This file does not contain anything specific to Moriarty in it. It is just a
// simple implementation of a Graph class to be used only in codelabs.

namespace {

class UnionFind {
 public:
  explicit UnionFind(int n) : parent_(n, -1) {
    std::iota(parent_.begin(), parent_.end(), 0);
  }

  void Merge(int u, int v) { parent_[Find(u)] = Find(v); }

  int Find(int v) {
    return parent_[v] = (parent_[v] == v ? v : Find(parent_[v]));
  }

 private:
  std::vector<int> parent_;
};

}  // namespace

bool GraphIsConnected(const ExampleGraph& g) {
  UnionFind uf(g.num_nodes);

  for (const auto& [u, v] : g.edges) uf.Merge(u, v);

  for (int i = 1; i < g.num_nodes; i++)
    if (uf.Find(i) != uf.Find(0)) return false;
  return true;
}

bool operator<(const ExampleGraph& G1, const ExampleGraph& G2) {
  return std::tie(G1.num_nodes, G1.edges) < std::tie(G2.num_nodes, G2.edges);
}

bool operator==(const ExampleGraph& G1, const ExampleGraph& G2) {
  return std::tie(G1.num_nodes, G1.edges) == std::tie(G2.num_nodes, G2.edges);
}

}  // namespace moriarty_examples
