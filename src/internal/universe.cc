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

#include "src/internal/universe.h"

#include <string>

#include "absl/base/nullability.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "src/errors.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/generation_config.h"
#include "src/internal/random_config.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/io_config.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {
namespace moriarty_internal {

// ---------------------------------------------------------------------------
//  VariableSet

Universe& Universe::SetMutableVariableSet(VariableSet* variable_set) {
  ABSL_CHECK_EQ(const_variable_set_, nullptr)
      << "Only one of SetConstVariableSet and SetMutableVariableSet can be "
         "called.";
  mutable_variable_set_ = variable_set;
  return *this;
}

Universe& Universe::SetConstVariableSet(const VariableSet* variable_set) {
  ABSL_CHECK_EQ(mutable_variable_set_, nullptr)
      << "Only one of SetConstVariableSet and SetMutableVariableSet can be "
         "called.";
  const_variable_set_ = variable_set;
  return *this;
}

absl::StatusOr<AbstractVariable*> Universe::GetAbstractVariable(
    absl::string_view variable_name) {
  if (!GetVariableSet()) {
    return MisconfiguredError("Universe", "GetAbstractVariable",
                              InternalConfigurationType::kVariableSet);
  }
  return GetVariableSet()->GetAbstractVariable(variable_name);
}

absl::StatusOr<const AbstractVariable*> Universe::GetAbstractVariable(
    absl::string_view variable_name) const {
  if (!GetVariableSet()) {
    return MisconfiguredError("Universe", "GetAbstractVariable",
                              InternalConfigurationType::kVariableSet);
  }
  return GetVariableSet()->GetAbstractVariable(variable_name);
}

VariableSet* Universe::GetVariableSet() { return mutable_variable_set_; }

const VariableSet* Universe::GetVariableSet() const {
  return const_variable_set_ ? const_variable_set_ : mutable_variable_set_;
}

// ---------------------------------------------------------------------------
//  ValueSet

Universe& Universe::SetMutableValueSet(ValueSet* value_set) {
  ABSL_CHECK_EQ(const_value_set_, nullptr)
      << "Only one of SetConstValueSet and SetMutableValueSet can be called.";
  mutable_value_set_ = value_set;
  return *this;
}

Universe& Universe::SetConstValueSet(const ValueSet* value_set) {
  ABSL_CHECK_EQ(mutable_value_set_, nullptr)
      << "Only one of SetConstValueSet and SetMutableValueSet can be called.";
  const_value_set_ = value_set;
  return *this;
}

bool Universe::ValueIsKnown(absl::string_view variable_name) const {
  if (GetValueSet() == nullptr) return false;
  return GetValueSet()->Contains(variable_name);
}

absl::Status Universe::AssignValueToVariable(absl::string_view variable_name) {
  if (!GetValueSet()) {
    return MisconfiguredError("Universe", "AssignValueToVariable",
                              InternalConfigurationType::kValueSet);
  }

  // Value already known?
  if (GetValueSet()->Contains(variable_name)) return absl::OkStatus();

  MORIARTY_ASSIGN_OR_RETURN(AbstractVariable * var,
                            GetAbstractVariable(variable_name));

  // Already attempting to generate?
  auto [it, inserted] =
      currently_generating_variables_.insert(std::string(variable_name));
  if (!inserted) {
    return absl::FailedPreconditionError(absl::Substitute(
        "Found cyclic dependency in variables involving '$0'", variable_name));
  }

  var->SetUniverse(this, variable_name);
  absl::Status status = var->AssignValue();

  currently_generating_variables_.erase(variable_name);

  return status;
}

ValueSet* Universe::GetValueSet() { return mutable_value_set_; }

const ValueSet* Universe::GetValueSet() const {
  return const_value_set_ ? const_value_set_ : mutable_value_set_;
}

absl::Status Universe::EraseValue(absl::string_view variable_name) {
  if (!GetValueSet()) {
    return MisconfiguredError("Universe", "EraseValue",
                              InternalConfigurationType::kValueSet);
  }

  GetValueSet()->Erase(variable_name);
  return absl::OkStatus();
}

// ---------------------------------------------------------------------------
//  Random

Universe& Universe::SetRandomEngine(RandomEngine* rng) {
  random_config_.SetRandomEngine(rng);
  return *this;
}

RandomEngine* Universe::GetRandomEngine() {
  return random_config_.GetRandomEngine();
}

RandomConfig& Universe::GetRandomConfig() { return random_config_; }

// ---------------------------------------------------------------------------
//  IOConfig

Universe& Universe::SetIOConfig(librarian::IOConfig* io_config) {
  io_config_ = io_config;
  return *this;
}

absl::Nullable<const librarian::IOConfig*> Universe::GetIOConfig() const {
  return io_config_;
}

absl::Nullable<librarian::IOConfig*> Universe::GetIOConfig() {
  return io_config_;
}

// ---------------------------------------------------------------------------
//  GenerationConfig

Universe& Universe::SetGenerationConfig(GenerationConfig* generation_config) {
  generation_config_ = generation_config;
  return *this;
}

GenerationConfig* Universe::GetGenerationConfig() { return generation_config_; }

}  // namespace moriarty_internal
}  // namespace moriarty
