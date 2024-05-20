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

#include "src/internal/generation_bootstrap.h"

#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/substitute.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/generation_config.h"
#include "src/internal/universe.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {
namespace moriarty_internal {

namespace {

void SetUniverse(VariableSet& variables, Universe* universe) {
  for (const auto& [var_name, var_ptr] : variables.GetAllVariables())
    var_ptr->SetUniverse(universe, var_name);
}

GenerationConfig CreateGenerationConfig(const GenerationOptions& options) {
  GenerationConfig generation_config;

  if (options.soft_generation_limit)
    generation_config.SetSoftGenerationLimit(*options.soft_generation_limit);

  return generation_config;
}

absl::StatusOr<absl::flat_hash_map<std::string, std::vector<std::string>>>
GetDependenciesMap(const VariableSet& variables) {
  absl::flat_hash_map<std::string, std::vector<std::string>> deps_map;
  const absl::flat_hash_map<std::string, std::unique_ptr<AbstractVariable>>&
      var_map = variables.GetAllVariables();

  for (const auto& [var_name, var_ptr] : var_map)
    deps_map[var_name] = var_ptr->GetDependencies();

  return deps_map;
}

absl::flat_hash_map<std::string, int> CountIncomingEdges(
    const absl::flat_hash_map<std::string, std::vector<std::string>>&
        deps_map) {
  absl::flat_hash_map<std::string, int> num_incoming_edges;
  for (const auto& [var, deps] : deps_map) {
    if (!num_incoming_edges.contains(var)) num_incoming_edges[var] = 0;

    for (const std::string& dep : deps) {
      num_incoming_edges[dep]++;
    }
  }
  return num_incoming_edges;
}

std::priority_queue<std::string, std::vector<std::string>,
                    std::greater<std::string>>
GetNodesWithNoIncomingEdges(
    const absl::flat_hash_map<std::string, int>& num_incoming_edges) {
  std::priority_queue<std::string, std::vector<std::string>,
                      std::greater<std::string>>
      queue;

  for (const auto& [v, e] : num_incoming_edges)
    if (e == 0) queue.push(v);
  return queue;
}
}  // namespace

absl::StatusOr<std::vector<std::string>> GetGenerationOrder(
    const absl::flat_hash_map<std::string, std::vector<std::string>>& deps_map,
    const ValueSet& known_values) {
  std::vector<std::string> ordered_variables;
  absl::flat_hash_map<std::string, int> num_incoming_edges =
      CountIncomingEdges(deps_map);
  std::priority_queue<std::string, std::vector<std::string>,
                      std::greater<std::string>>
      no_incoming_edges = GetNodesWithNoIncomingEdges(num_incoming_edges);
  while (!no_incoming_edges.empty()) {
    std::string current = no_incoming_edges.top();
    no_incoming_edges.pop();
    ordered_variables.push_back(current);

    for (const std::string& dep : deps_map.at(current)) {
      if (!deps_map.contains(dep)) {
        if (!known_values.Contains(dep)) {
          return absl::FailedPreconditionError(absl::Substitute(
              "Unknown dependency '$0' for var '$1!'", dep, current));
        }
        continue;
      }
      num_incoming_edges[dep]--;
      if (num_incoming_edges[dep] == 0) no_incoming_edges.push(dep);
    }
  }
  if (ordered_variables.size() != deps_map.size()) {
    return absl::InvalidArgumentError("Cycle in the dependency order graph.");
  }
  return ordered_variables;
}

absl::StatusOr<ValueSet> GenerateAllValues(VariableSet variables,
                                           ValueSet known_values,
                                           const GenerationOptions& options) {
  GenerationConfig generation_config = CreateGenerationConfig(options);
  Universe universe;
  universe.SetMutableValueSet(&known_values)
      .SetMutableVariableSet(&variables)
      .SetGenerationConfig(&generation_config)
      .SetRandomEngine(&options.random_engine);

  SetUniverse(variables, &universe);

  MORIARTY_ASSIGN_OR_RETURN(
      (absl::flat_hash_map<std::string, std::vector<std::string>> deps_map),
      GetDependenciesMap(variables));

  MORIARTY_ASSIGN_OR_RETURN(std::vector<std::string> variable_names,
                            GetGenerationOrder(deps_map, known_values));

  // First do a quick assignment of all known values.
  for (const std::string& name : variable_names) {
    MORIARTY_ASSIGN_OR_RETURN(AbstractVariable * var,
                              variables.GetAbstractVariable(name));
    MORIARTY_RETURN_IF_ERROR(var->AssignUniqueValue());
  }

  // Now do a deep generation.
  for (const std::string& name : variable_names) {
    MORIARTY_ASSIGN_OR_RETURN(AbstractVariable * var,
                              variables.GetAbstractVariable(name));
    MORIARTY_RETURN_IF_ERROR(var->AssignValue());
  }

  // We may have initially generated invalid values during the
  // AssignUniqueValues(). Let's check for those now...
  // TODO(darcybest): Determine if there's a better way of doing this...
  for (const std::string& name : variable_names) {
    MORIARTY_ASSIGN_OR_RETURN(AbstractVariable * var,
                              variables.GetAbstractVariable(name));
    MORIARTY_RETURN_IF_ERROR(var->ValueSatisfiesConstraints());
  }

  return known_values;
}

}  // namespace moriarty_internal
}  // namespace moriarty
