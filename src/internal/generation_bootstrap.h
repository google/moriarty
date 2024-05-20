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

#ifndef MORIARTY_SRC_INTERNAL_GENERATION_BOOTSTRAP_H_
#define MORIARTY_SRC_INTERNAL_GENERATION_BOOTSTRAP_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"

namespace moriarty {
namespace moriarty_internal {

struct GenerationOptions {
  RandomEngine& random_engine;
  std::optional<int64_t> soft_generation_limit;
};

// GenerateAllValues()
//
// Generates and returns a value for each variable in `variables`.
//
// All values in `known_values` will appear in the output (even if there is no
// corresponding variable in `variables`).
absl::StatusOr<ValueSet> GenerateAllValues(VariableSet variables,
                                           ValueSet known_values,
                                           const GenerationOptions& options);

// GetGenerationOrder()
//
// Returns a list of the variables names by order of generation.
//
// The order is from parent to child. E.g., 'A' depends on 'C' and 'C'
// depends on 'B'. The final order will be {A, B, C}.
absl::StatusOr<std::vector<std::string>> GetGenerationOrder(
    const absl::flat_hash_map<std::string, std::vector<std::string>>& deps_map,
    const ValueSet& known_values);

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_GENERATION_BOOTSTRAP_H_
