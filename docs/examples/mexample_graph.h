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

#ifndef MORIARTY_DOCS_EXAMPLES_MEXAMPLE_GRAPH_H_
#define MORIARTY_DOCS_EXAMPLES_MEXAMPLE_GRAPH_H_

#include <optional>
#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "docs/examples/example_graph.h"
#include "src/librarian/mvariable.h"
#include "src/variables/minteger.h"

namespace moriarty_examples {

// MExampleGraph()
//
// An example MVariable to be used in codelabs. Not production quality.
//
// Specifies constraints for a graph.
class MExampleGraph
    : public moriarty::librarian::MVariable<MExampleGraph, ExampleGraph> {
 public:
  std::string Typename() const override { return "MGraph"; }

  // Adds a constraint on the number of nodes in the graph.
  MExampleGraph& WithNumNodes(moriarty::MInteger num_nodes);

  // Adds a constraint on the number of edges in the graph.
  MExampleGraph& WithNumEdges(moriarty::MInteger num_edges);

  // Adds a constraint stating that this graph must be connected.
  MExampleGraph& IsConnected();

 private:
  std::optional<moriarty::MInteger> num_nodes_;
  std::optional<moriarty::MInteger> num_edges_;
  bool is_connected_ = false;

  absl::StatusOr<ExampleGraph> GenerateImpl() override;
  absl::Status IsSatisfiedWithImpl(const ExampleGraph& g) const override;

  absl::Status MergeFromImpl(const MExampleGraph& other) override;
};

}  // namespace moriarty_examples

#endif  // MORIARTY_DOCS_EXAMPLES_MEXAMPLE_GRAPH_H_
