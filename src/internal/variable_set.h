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

// The base atom Moriarty is a collection of variables, not just a single
// variable.

#ifndef MORIARTY_SRC_INTERNAL_VARIABLE_SET_H_
#define MORIARTY_SRC_INTERNAL_VARIABLE_SET_H_

#include <concepts>
#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "src/errors.h"
#include "src/internal/abstract_variable.h"
#include "src/scenario.h"

namespace moriarty {
namespace moriarty_internal {

class Universe;  // Forward declaring Universe

// VariableSet
//
// A collection of (possibly interacting) variables. Constraints that reference
// other variables must be in the same `VariableSet` instance.
class VariableSet {
 public:
  // Rule of 5, plus swap
  VariableSet() = default;
  ~VariableSet() = default;
  VariableSet(const VariableSet& other);      // Copy
  VariableSet& operator=(VariableSet other);  // Assignment (both copy and move)
  VariableSet(VariableSet&& other) noexcept = default;  // Move
  void Swap(VariableSet& other);

  // Adds a variable to the collection. Fails if the variable already exists.
  absl::Status AddVariable(absl::string_view name,
                           const AbstractVariable& variable);

  // Adds an AbstractVariable to the collection, if it doesn't exist, or merges
  // it into the existing variable if it does.
  absl::Status AddOrMergeVariable(absl::string_view name,
                                  const AbstractVariable& variable);

  // GetAbstractVariable()
  //
  // Returns a pointer to the variable. Ownership of this pointer is *not*
  // transferred to the caller.
  //
  // Errors:
  //  * VariableNotFoundError() if the variable does not exist.
  absl::StatusOr<const AbstractVariable*> GetAbstractVariable(
      absl::string_view name) const;
  absl::StatusOr<AbstractVariable*> GetAbstractVariable(absl::string_view name);

  // GetVariable<>()
  //
  // Returns the variable named `name`.
  //
  // Errors:
  //  * VariableNotFoundError() if the variable does not exist.
  //  * kInvalidArgument if it is not convertible to `T`
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  absl::StatusOr<T> GetVariable(absl::string_view name) const;

  // GetAllVariables()
  //
  // Returns the map of internal variables.
  const absl::flat_hash_map<std::string, std::unique_ptr<AbstractVariable>>&
  GetAllVariables() const;

  // SetUniverse()
  //
  // Calls SetUniverse() on all variables owned by this variable set.
  void SetUniverse(Universe* universe);

  // WithScenario()
  //
  // Adds a scenario to all variables in this object. Calls `WithProperty`
  // on all applicable types.
  absl::Status WithScenario(const Scenario& scenario);

  // AllVariablesSatisfyConstraints()
  //
  // Determines if all variable constraints specified here have a corresponding
  // values (accessed via its Universe) that satisfies the constraints.
  //
  // If a variable does not have a value, this will return not ok.
  // If a value does not have a variable, this will return ok.
  absl::Status AllVariablesSatisfyConstraints() const;

 private:
  absl::flat_hash_map<std::string, std::unique_ptr<AbstractVariable>>
      variables_;

  // Returns either a pointer to the AbstractVariable or `nullptr` if it doesn't
  // exist.
  const AbstractVariable* GetAbstractVariableOrNull(
      absl::string_view name) const;
  AbstractVariable* GetAbstractVariableOrNull(absl::string_view name);
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename T>
  requires std::derived_from<T, AbstractVariable>
absl::StatusOr<T> VariableSet::GetVariable(absl::string_view name) const {
  const AbstractVariable* var = GetAbstractVariableOrNull(name);
  if (var == nullptr) return VariableNotFoundError(name);

  return ConvertTo<T>(var, name);
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_VARIABLE_SET_H_
