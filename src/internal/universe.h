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

#ifndef MORIARTY_SRC_INTERNAL_UNIVERSE_H_
#define MORIARTY_SRC_INTERNAL_UNIVERSE_H_

#include <any>
#include <concepts>
#include <optional>
#include <string>
#include <utility>

#include "absl/base/nullability.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "src/errors.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/generation_config.h"
#include "src/internal/random_config.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_name_utils.h"
#include "src/internal/variable_set.h"
#include "src/librarian/io_config.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {
namespace moriarty_internal {

// Universe
//
// All MVariables in Moriarty live in a universe. The universe provides
// information about the global space they are involved in. For example, each
// invocation of AddTestCase() in a Generator has its own universe so that the
// variables do not conflict with one another between different test cases.
//
// Another example: if a variable may be dependent on another variable (e.g., a
// string's length is "N", where "N" is another variable, then the string can
// ask its universe what the value of "N" is).
class Universe {
 public:
  // ---------------------------------------------------------------------------
  //  VariableSet

  // SetMutableVariableSet()
  //
  // Sets `variables` to be the VariableSet for this universe. Ownership is not
  // transferred to this class. Only one of `SetMutableVariableSet` and
  // `SetConstVariableSet` may be called.
  Universe& SetMutableVariableSet(VariableSet* variable_set);

  // SetConstVariableSet()
  //
  // Sets `variables` to be the VariableSet for this universe. Ownership is not
  // transferred to this class. Only one of `SetMutableVariableSet` and
  // `SetConstVariableSet` may be called.
  Universe& SetConstVariableSet(const VariableSet* variable_set);

  // GetVariable<>()
  //
  // Returns the variable with the name `variable_name`.
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  absl::StatusOr<T> GetVariable(absl::string_view variable_name) const;

  // GetAbstractVariable()
  //
  // Returns a pointer to the `AbstractVariable` with the name `variable_name`.
  // Ownership is not transferred to the caller.
  absl::StatusOr<AbstractVariable*> GetAbstractVariable(
      absl::string_view variable_name);

  // GetAbstractVariable()
  //
  // Returns a pointer to the `AbstractVariable` with the name `variable_name`.
  // Ownership is not transferred to the caller.
  absl::StatusOr<const AbstractVariable*> GetAbstractVariable(
      absl::string_view variable_name) const;

  // ---------------------------------------------------------------------------
  //  ValueSet

  // SetMutableValueSet()
  //
  // Sets the internal `value_set`. Ownership is not transferred to this class.
  // Only one of `SetMutableValueSet` and `SetConstValueSet` may be called.
  Universe& SetMutableValueSet(ValueSet* value_set);

  // SetConstValueSet()
  //
  // Sets the internal `value_set`. Ownership is not transferred to this class.
  // Only one ValueSet may be set.
  Universe& SetConstValueSet(const ValueSet* value_set);

  // ValueIsKnown()
  //
  // Returns if the value for the variable `variable_name` has already been
  // computed.
  bool ValueIsKnown(absl::string_view variable_name) const;

  // GetValue<>()
  //
  // Returns the assigned value for `variable_name`. Fails if that variable has
  // not previously been assigned. See `GetOrGenerateAndSetValue` if you want to
  // generate it if unknown.
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  absl::StatusOr<typename T::value_type> GetValue(
      absl::string_view variable_name) const;

  // GetSubvalue<>()
  //
  // Returns the value for `subvalue_name` using the value assigned to
  // `variable_name`.
  //
  // Returns an error if the retrieved value is of the wrong type or any error
  // while retrieving the subvalue.
  template <typename T>
  absl::StatusOr<T> GetSubvalue(
      const VariableNameBreakdown& variable_name) const;

  // GetOrGenerateAndSetValue<>()
  //
  // Returns the assigned value for `variable_name`. If that variable has not
  // previously been assigned, this will generate a value and set that value
  // before returning. See `GetValue` if you don't want to possibly generate a
  // new value.
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  absl::StatusOr<typename T::value_type> GetOrGenerateAndSetValue(
      absl::string_view variable_name);

  // SetValue<>()
  //
  // Sets the value of `variable_name` to be `value`.
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  absl::Status SetValue(absl::string_view variable_name, T::value_type value);

  // EraseValue()
  //
  // Erases the value for `variable_name`. This is a no-op, and returns
  // absl::OkStatus(), if the variable name is unknown.
  absl::Status EraseValue(absl::string_view variable_name);

  // ---------------------------------------------------------------------------
  //  RandomConfig

  // SetRandomEngine()
  //
  // Sets the internal `rng`. Ownership is not transferred to this class.
  Universe& SetRandomEngine(RandomEngine* rng);

  // GetRandomEngine()
  //
  // Returns a reference to the RandomEngine used for this generation. Returns
  // nullptr if non-existent.
  RandomEngine* GetRandomEngine();

  // GetRandomConfig()
  //
  // Returns a reference to the internally stored `RandomConfig`. The
  // RandomConfig's RandomEngine is the one set via `SetRandomEngine`.
  RandomConfig& GetRandomConfig();

  // ---------------------------------------------------------------------------
  //  IOConfig

  // SetIOConfig()
  //
  // Sets the internal IOConfig. Ownership is not transferred to this class.
  Universe& SetIOConfig(librarian::IOConfig* io_config);

  // GetIOConfig()
  //
  // Returns the IOConfig, or nullptr if it hasn't been set.
  absl::Nullable<const librarian::IOConfig*> GetIOConfig() const;

  // GetIOConfig()
  //
  // Returns the IOConfig, or nullptr if it hasn't been set.
  absl::Nullable<librarian::IOConfig*> GetIOConfig();

  // ---------------------------------------------------------------------------
  //  GenerationConfig

  // SetGenerationConfig()
  //
  // Sets the internal generation config. Ownership is not transferred to this
  // class.
  Universe& SetGenerationConfig(GenerationConfig* generation_config);

  // GetGenerationConfig()
  //
  // Returns a pointer to the Universe's GenerationConfig.
  //
  // Ownership is *not* transferred to the caller. The pointer's lifetime is
  // determined by the caller of `SetGenerationConfig()`.
  GenerationConfig* GetGenerationConfig();

 private:
  // None of these pointers are owned by this class

  // VariableSet. Do not use directly. Access via GetVariableSet().
  VariableSet* mutable_variable_set_ = nullptr;
  const VariableSet* const_variable_set_ = nullptr;

  // ValueSet. Do not use directly. Access via GetValues().
  ValueSet* mutable_value_set_ = nullptr;
  const ValueSet* const_value_set_ = nullptr;

  RandomConfig random_config_;

  // IOConfig. Do not use this directly. Access via GetIOConfig().
  librarian::IOConfig* io_config_ = nullptr;

  // Variables that are in the process of being assigned. Used to catch cycles.
  absl::flat_hash_set<std::string> currently_generating_variables_;

  // GenerationConfig
  GenerationConfig* generation_config_ = nullptr;

  // Returns a const-correct version of VariableSet. See above. Returns
  // nullptr if non-existent.
  VariableSet* GetVariableSet();
  const VariableSet* GetVariableSet() const;

  // Returns a const-correct version of the ValueSet. See above. Returns
  // nullptr if non-existent.
  ValueSet* GetValueSet();
  const ValueSet* GetValueSet() const;

  // AssignValueToVariable()
  //
  // Assigns `variable_name` to a specific value. If this variable has been
  // previously assigned, this will not overwrite that.
  //
  // This may involve assigning zero or more other dependent variables. This
  // will assign values to each of those.
  absl::Status AssignValueToVariable(absl::string_view variable_name);
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename T>
  requires std::derived_from<T, AbstractVariable>
absl::Status Universe::SetValue(absl::string_view variable_name,
                                T::value_type value) {
  if (GetValueSet() == nullptr)
    return absl::FailedPreconditionError(
        "SetValue() called without a mutable ValueSet");

  GetValueSet()->Set<T>(variable_name, std::move(value));
  return absl::OkStatus();
}

template <typename T>
  requires std::derived_from<T, AbstractVariable>
absl::StatusOr<T> Universe::GetVariable(absl::string_view variable_name) const {
  MORIARTY_ASSIGN_OR_RETURN(const AbstractVariable* var,
                            GetAbstractVariable(variable_name));
  return ConvertTo<T>(var, variable_name);
}

template <typename T>
  requires std::derived_from<T, AbstractVariable>
absl::StatusOr<typename T::value_type> Universe::GetValue(
    absl::string_view variable_name) const {
  if (!GetValueSet()) {
    return MisconfiguredError("Universe", "GetValue",
                              InternalConfigurationType::kValueSet);
  }

  if (HasSubvariable(variable_name)) {
    return GetSubvalue<typename T::value_type>(
        CreateVariableNameBreakdown(variable_name));
  }

  absl::StatusOr<typename T::value_type> val =
      GetValueSet()->Get<T>(variable_name);
  if (val.ok() || !IsValueNotFoundError(val.status())) return val;

  // If no value is found with that name, we can see if there is a uniquely
  // determined value for the corresponding variable.
  MORIARTY_ASSIGN_OR_RETURN(const AbstractVariable* var,
                            GetAbstractVariable(variable_name));

  std::optional<std::any> unique_value = var->GetUniqueValueUntyped();
  if (!unique_value.has_value()) {
    return ValueNotFoundError(variable_name);
  }

  using TV = typename T::value_type;
  const TV* typed_value = std::any_cast<TV>(&(*unique_value));
  if (typed_value == nullptr) {
    // This should not happen. It means the AbstractVariable is returning the
    // wrong type from GetUniqueValueUntyped(). It must be exactly the right
    // type.
    return absl::InternalError(
        "Type returned by GetUniqueValueUntyped is of an incompatible type.");
  }
  return *typed_value;
}

// TODO(darcybest): Add tests.
template <typename T>
absl::StatusOr<T> Universe::GetSubvalue(
    const VariableNameBreakdown& variable_name) const {
  if (!variable_name.subvariable_name.has_value()) {
    return absl::FailedPreconditionError(absl::StrCat(
        variable_name.base_variable_name, " has no subvariable name"));
  }
  MORIARTY_ASSIGN_OR_RETURN(
      const AbstractVariable* var,
      GetAbstractVariable(variable_name.base_variable_name));
  MORIARTY_ASSIGN_OR_RETURN(
      std::any base_value,
      GetValueSet()->UnsafeGet(variable_name.base_variable_name));
  MORIARTY_ASSIGN_OR_RETURN(
      std::any value,
      var->GetSubvalue(base_value, *variable_name.subvariable_name));
  const T* val = std::any_cast<const T>(&value);
  if (val == nullptr)
    return absl::FailedPreconditionError(
        absl::StrCat("Subvalue ", ConstructVariableName(variable_name),
                     " returned the wrong type"));
  return *val;
}

template <typename T>
  requires std::derived_from<T, AbstractVariable>
absl::StatusOr<typename T::value_type> Universe::GetOrGenerateAndSetValue(
    absl::string_view variable_name) {
  auto val = GetValue<T>(variable_name);

  if (val.ok() || !IsValueNotFoundError(val.status())) return val;

  if (const_value_set_ != nullptr) {
    return absl::FailedPreconditionError(absl::Substitute(
        "Cannot generate '$0' with a const ValueSet", variable_name));
  }

  MORIARTY_RETURN_IF_ERROR(
      AssignValueToVariable(BaseVariableName(variable_name)));
  return GetValue<T>(variable_name);
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_UNIVERSE_H_
