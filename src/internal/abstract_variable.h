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

#ifndef MORIARTY_SRC_INTERNAL_ABSTRACT_VARIABLE_H_
#define MORIARTY_SRC_INTERNAL_ABSTRACT_VARIABLE_H_

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "src/property.h"

namespace moriarty {
namespace moriarty_internal {

class Universe;  // Forward declaring Universe

// AbstractVariable
//
// This class should not be directly derived from. See `MVariable<>`.
// Most users should not need any context about this class's existence.
//
// This is the top of the variable hierarchy. Each instance of AbstractVariable
// is a single variable (it is the `x` in `auto x = 5;`). This variable will
// contain its own constraints as well as have knowledge of other variables.
// This is needed so the variables can interact. For example "I am
// a string S, and my length is 2*N, where N is another AbstractVariable."
class AbstractVariable {
 public:
  virtual ~AbstractVariable() = default;

 protected:
  // Only derived classes should make copies of me directly to avoid accidental
  // slicing. Call derived class's constructors instead.
  AbstractVariable() = default;
  AbstractVariable(const AbstractVariable&) = default;
  AbstractVariable(AbstractVariable&&) = default;
  AbstractVariable& operator=(const AbstractVariable&) = default;
  AbstractVariable& operator=(AbstractVariable&&) = default;

 public:
  // Typename() [pure virtual]
  //
  // Returns a string representing the name of this type (for example,
  // "MInteger"). This is mostly used for debugging/error messages.
  virtual std::string Typename() const = 0;

  // Clone() [pure virtual]
  //
  // Create a copy and return a pointer to the newly created object. Ownership
  // of the returned pointer is passed to the caller.
  virtual std::unique_ptr<AbstractVariable> Clone() const = 0;

  // AssignValue() [pure virtual]
  //
  // Given all current constraints, assigns a specific value to this variable
  // via the `Universe`. The value will be stored under the name
  // provided when calling `SetUniverse` (Let's say it's `X`).
  //
  // 1. Asks Universe (Uni) for a value for `X`.
  // 2. Uni checks its ValueSet. If it has it, we're done!
  // 3. If not, Uni tells the variable named `X` to generate a value.
  // 4. `X` generates itself.
  // 5. Stores that value into the ValueSet.
  //
  // Note that the variable named `X` in the universe may or may not be
  // identically `this` variable, but it should be assumed to be equivalent.
  virtual absl::Status AssignValue() = 0;

  // AssignUniqueValue() [pure virtual]
  //
  // Determines if there is exactly one value that this variable can be assigned
  // to. If so, assigns that value.
  //
  // If there is not a unique value (or it is too hard to determine that there
  // is a unique value), this does nothing.
  //
  // Example: MInteger().Between(7, 7) might be able to determine that its
  // unique value is 7.
  virtual absl::Status AssignUniqueValue() = 0;

  // GetUniqueValueUntyped() [pure virtual]
  //
  // Determines if there is exactly one value that this variable can be assigned
  // to. If so, return that value.
  //
  // If there is not a unique value (or it is too hard to determine that there
  // is a unique value), this returns `std::nullopt`.
  //
  // This function is `Untyped` since it returns an `std::any`. However, the
  // type of `std::any` must be exactly the same as the type assigned to the
  // universe via `AssignValue()`.
  //
  // Example: MInteger().Between(7, 7) might be able to determine that its
  // unique value is 7.
  virtual std::optional<std::any> GetUniqueValueUntyped() const = 0;

  // ReadValue() [pure virtual]
  //
  // Given all current I/O constraints on this variable, read a value from the
  // Universe's IOConfig and store it in its Universe under the
  // name provided via `SetUniverse`.
  virtual absl::Status ReadValue() = 0;

  // PrintValue() [pure virtual]
  //
  // Given all current I/O constraints on this variable, print the value
  // currently stored in Universe under the name provided by
  // SetUniverse into the output stream in its Universe's
  // IOConfig.
  virtual absl::Status PrintValue() = 0;

  // MergeFrom() [pure virtual]
  //
  // Merges my current constraints with the constraints of the `other` variable.
  // The merge should act as an intersection of the two constraints. If one says
  // 1 <= x <= 10 and the other says 5 <= x <= 20, then then merged version
  // should have 5 <= x <= 10.
  virtual absl::Status MergeFrom(const AbstractVariable& other) = 0;

  // SetUniverse() [pure virtual]
  //
  // Sets the universe for this variable.
  //
  // `my_name_in_universe` is the name of this variable in the context
  // of `universe`.
  virtual void SetUniverse(Universe* universe,
                           absl::string_view my_name_in_universe) = 0;

  // WithProperty() [pure virtual]
  //
  // Tells this variable that it should satisfy `property`.
  virtual absl::Status WithProperty(Property property) = 0;

  // ValueSatisfiesConstraints() [pure virtual]
  //
  // Determines if all variable constraints specified here have a corresponding
  // values (accessed via its Universe) that satisfies the constraints.
  //
  // If a variable does not have a value, this will return not ok.
  // If a value does not have a variable, this will return ok.
  virtual absl::Status ValueSatisfiesConstraints() const = 0;

  // GetDifficultAbstractVariables() [pure virtual]
  //
  // Returns a list of smart points to the difficult instances of this abstract
  // variable.
  virtual absl::StatusOr<
      std::vector<std::unique_ptr<moriarty_internal::AbstractVariable>>>
  GetDifficultAbstractVariables() const = 0;

  // GetDependencies() [pure virtual]
  //
  // Returns a list of variable names that this variable depends on.
  virtual std::vector<std::string> GetDependencies() = 0;

  // GetSubvalue() [pure virtual]
  //
  // `my_value` will be of the type generated by this variable.
  //
  // Given the value of this variable and the name of a subvalue, this will
  // return the std::any value of the subvalue if it is found.
  //
  // Returns an error if there are any issues retrieving the subvalue.
  //
  // Example usage:
  //    vector<string> A = {...};
  //    StatusOr<std::any> x = MArray().GetSubvalue(A, "1.length");
  // Where the value of the std::any is an int64_t (the length of the 2nd
  // element).
  virtual absl::StatusOr<std::any> GetSubvalue(
      const std::any& my_value, absl::string_view subvalue_name) const = 0;
};

// A simple pair of name and variable.
struct NamedVariable {
  std::string name;
  moriarty_internal::AbstractVariable* variable;  // Does not own `variable`.
};

// A simple pair of a single value and its corresponding variable.
struct VariableValue {
  // TODO(darcybest): Switch to `unique_ptr` once we determine our copy
  // semantics. As of today, these are all husks of objects, so sharing is fine.
  std::shared_ptr<const AbstractVariable> variable;
  std::any value;
};

}  // namespace moriarty_internal

namespace moriarty_internal {

// ConvertTo<>
//
//  Converts an AbstractVariable to some derived variable type. Returns
//  kInvalidArgument if it is not convertible.
template <typename Type>
absl::StatusOr<Type> ConvertTo(AbstractVariable* var, absl::string_view name) {
  Type* typed_var = dynamic_cast<Type*>(var);
  if (typed_var == nullptr)
    return absl::InvalidArgumentError(
        absl::Substitute("Unable to convert `$0` from $1 to $2", name,
                         var->Typename(), Type().Typename()));
  return *typed_var;
}

// ConvertTo<>
//
//  Converts an AbstractVariable to some derived variable type. Returns
//  kInvalidArgument if it is not convertible.
template <typename Type>
absl::StatusOr<Type> ConvertTo(const AbstractVariable* var,
                               absl::string_view name) {
  const Type* typed_var = dynamic_cast<const Type*>(var);
  if (typed_var == nullptr)
    return absl::InvalidArgumentError(
        absl::Substitute("Unable to convert `$0` from $1 to $2", name,
                         var->Typename(), Type().Typename()));
  return *typed_var;
}

}  // namespace moriarty_internal

}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_ABSTRACT_VARIABLE_H_
