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

#include "docs/examples/mexample_graph.h"

#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "docs/examples/example_graph.h"
#include "src/errors.h"
#include "src/util/status_macro/status_macros.h"
#include "src/variables/minteger.h"

namespace moriarty_examples {

MExampleGraph& MExampleGraph::WithNumNodes(moriarty::MInteger num_nodes) {
  if (num_nodes_.has_value()) {
    num_nodes_->MergeFrom(num_nodes);
  } else {
    num_nodes_ = num_nodes;
  }

  return *this;
}

MExampleGraph& MExampleGraph::WithNumEdges(moriarty::MInteger num_edges) {
  if (num_edges_.has_value()) {
    num_edges_->MergeFrom(num_edges);
  } else {
    num_edges_ = num_edges;
  }

  return *this;
}

MExampleGraph& MExampleGraph::IsConnected() {
  is_connected_ = true;
  return *this;
}

absl::StatusOr<ExampleGraph> MExampleGraph::GenerateImpl() {
  if (!num_nodes_.has_value()) {
    return absl::FailedPreconditionError("Number of nodes must be constrained");
  }
  if (!num_edges_.has_value()) {
    return absl::FailedPreconditionError("Number of edges must be constrained");
  }

  MORIARTY_ASSIGN_OR_RETURN(int num_nodes, Random("num_nodes", *num_nodes_));
  MORIARTY_ASSIGN_OR_RETURN(int num_edges, Random("num_edges", *num_edges_));

  if (is_connected_ && num_edges < num_nodes - 1) {
    moriarty::MInteger num_edges_copy = *num_edges_;
    num_edges_copy.AtLeast(num_nodes - 1);
    MORIARTY_ASSIGN_OR_RETURN(num_edges, Random("num_edges", num_edges_copy));
  }

  std::vector<std::pair<int, int>> edges;
  if (is_connected_) {
    // This part is not important to understanding Moriarty. We are simply
    // adding edges here to guarantee that we get a connected graph.
    for (int i = 1; i < num_nodes; i++) {
      MORIARTY_ASSIGN_OR_RETURN(int v, RandomInteger(i));
      edges.push_back({v, i});
    }
  }

  while (edges.size() < num_edges) {
    MORIARTY_ASSIGN_OR_RETURN(int u, RandomInteger(num_nodes));
    MORIARTY_ASSIGN_OR_RETURN(int v, RandomInteger(num_nodes));
    edges.push_back({u, v});
  }

  return ExampleGraph({.num_nodes = num_nodes, .edges = edges});
}

absl::Status MExampleGraph::IsSatisfiedWithImpl(const ExampleGraph& g) const {
  if (num_nodes_.has_value()) {
    MORIARTY_RETURN_IF_ERROR(SatisfiesConstraints(*num_nodes_, g.num_nodes));
  }

  if (num_edges_.has_value()) {
    MORIARTY_RETURN_IF_ERROR(SatisfiesConstraints(*num_edges_, g.edges.size()));
  }

  if (is_connected_) {
    if (!GraphIsConnected(g)) {
      return moriarty::UnsatisfiedConstraintError("G is not connected");
    }
  }

  return absl::OkStatus();
}

absl::Status MExampleGraph::MergeFromImpl(const MExampleGraph& other) {
  if (other.num_nodes_.has_value()) WithNumNodes(*other.num_nodes_);
  if (other.num_edges_.has_value()) WithNumEdges(*other.num_edges_);
  if (other.is_connected_) IsConnected();

  return absl::OkStatus();
}

}  // namespace moriarty_examples
