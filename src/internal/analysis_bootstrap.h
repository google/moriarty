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

#ifndef MORIARTY_SRC_INTERNAL_ANALYSIS_BOOTSTRAP_H_
#define MORIARTY_SRC_INTERNAL_ANALYSIS_BOOTSTRAP_H_

#include <concepts>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "src/internal/universe.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/mvariable.h"

namespace moriarty {
namespace moriarty_internal {

// SatisfiesConstraints()
//
// Determines if `value` satisfies the constraints defined by `constraints`.
// `known_values` contains all values that are known in the universe.
// `variables` can sometimes provide uniquely determined values (e.g.,
// `MInteger().Is(5)`). Note that no generation will occur inside of
// `variables`. `name` is used for debugging.
template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::Status SatisfiesConstraints(T constraints,
                                  const typename T::value_type& value,
                                  const ValueSet& known_values = {},
                                  const VariableSet& variables = {},
                                  absl::string_view name = "constraints");

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::Status SatisfiesConstraints(T constraints,
                                  const typename T::value_type& value,
                                  const ValueSet& known_values,
                                  const VariableSet& variables,
                                  absl::string_view name) {
  Universe universe = Universe()
                          .SetConstValueSet(&known_values)
                          .SetConstVariableSet(&variables);
  MVariableManager(&constraints).SetUniverse(&universe, name);

  return MVariableManager(&constraints).IsSatisfiedWith(value);
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_ANALYSIS_BOOTSTRAP_H_
