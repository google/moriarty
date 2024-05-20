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

#include "src/generator.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "src/errors.h"
#include "src/internal/random_config.h"
#include "src/internal/random_engine.h"
#include "src/internal/status_utils.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/scenario.h"
#include "src/test_case.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {

TestCase& Generator::AddTestCase() {
  return moriarty_internal::TryFunctionOrCrash<TestCase>(
      [this]() { return this->TryAddTestCase(); }, "AddTestCase");
}

TestCase& Generator::AddOptionalTestCase() {
  return moriarty_internal::TryFunctionOrCrash<TestCase>(
      [this]() { return this->TryAddOptionalTestCase(); },
      "AddOptionalTestCase");
}

absl::StatusOr<absl::Nonnull<TestCase*>> Generator::TryAddTestCase() {
  if (!general_constraints_) {
    return MisconfiguredError("Generator", "TryAddTestCase",
                              InternalConfigurationType::kVariableSet);
  }

  test_cases_.push_back(std::make_shared<TestCase>());

  TestCase& result = *test_cases_.back();
  for (const Scenario& scenario : scenarios_) result.WithScenario(scenario);
  moriarty_internal::TestCaseManager(&result).SetVariables(
      *general_constraints_);
  return &result;
}

// TODO(darcybest): merge these two together since they're just updating
// different vectors.
absl::StatusOr<absl::Nonnull<TestCase*>> Generator::TryAddOptionalTestCase() {
  if (!general_constraints_) {
    return MisconfiguredError("Generator", "TryAddTestCase",
                              InternalConfigurationType::kVariableSet);
  }

  optional_test_cases_.push_back(std::make_shared<TestCase>());

  TestCase& result = *optional_test_cases_.back();
  for (const Scenario& scenario : scenarios_) result.WithScenario(scenario);
  moriarty_internal::TestCaseManager(&result).SetVariables(
      *general_constraints_);
  return &result;
}

void Generator::WithScenario(Scenario scenario) {
  moriarty_internal::TryFunctionOrCrash(
      [this, &scenario]() {
        return this->TryWithScenario(std::move(scenario));
      },
      "WithScenario");
}

absl::Status Generator::TryWithScenario(Scenario scenario) {
  scenarios_.push_back(std::move(scenario));
  return absl::OkStatus();
}

const std::vector<std::shared_ptr<TestCase>>& Generator::GetTestCases() {
  return test_cases_;
}

std::optional<const moriarty_internal::VariableSet>
Generator::GetGeneralConstraints() {
  return general_constraints_;
}

const std::vector<std::shared_ptr<TestCase>>&
Generator::GetOptionalTestCases() {
  return optional_test_cases_;
}

absl::StatusOr<std::vector<moriarty_internal::ValueSet>>
Generator::AssignValuesInAllTestCases() {
  if (!rng_) {
    return MisconfiguredError("Generator", "AssignValuesInAllTestCases",
                              InternalConfigurationType::kRandomEngine);
  }
  if (!general_constraints_) {
    return MisconfiguredError("Generator", "AssignValuesInAllTestCases",
                              InternalConfigurationType::kVariableSet);
  }

  std::vector<moriarty_internal::ValueSet> assigned_test_cases;
  assigned_test_cases.reserve(test_cases_.size());
  for (auto& case_ptr : test_cases_) {
    MORIARTY_ASSIGN_OR_RETURN(
        moriarty_internal::ValueSet values,
        moriarty_internal::TestCaseManager(case_ptr.get())
            .AssignAllValues(*rng_, approximate_generation_limit_));
    assigned_test_cases.push_back(values);
  }

  for (auto& case_ptr : optional_test_cases_) {
    absl::StatusOr<moriarty_internal::ValueSet> values =
        moriarty_internal::TestCaseManager(case_ptr.get())
            .AssignAllValues(*rng_, approximate_generation_limit_);
    if (values.ok()) assigned_test_cases.push_back(*values);
  }

  return assigned_test_cases;
}

void Generator::SetSeed(absl::Span<const int64_t> seed) {
  rng_.emplace(seed, "v0.1");
}

void Generator::SetGeneralConstraints(
    moriarty_internal::VariableSet general_constraints) {
  general_constraints_.emplace(std::move(general_constraints));
}

void Generator::SetApproximateGenerationLimit(int64_t limit) {
  approximate_generation_limit_ = limit;
}

void Generator::ClearCases() {
  test_cases_.clear();
  optional_test_cases_.clear();
}

absl::Nullable<moriarty_internal::RandomEngine*> Generator::GetRandomEngine() {
  if (!rng_.has_value()) return nullptr;
  return &(*rng_);
}

absl::StatusOr<std::vector<int>> Generator::TryRandomPermutation(int n) {
  if (!rng_) {
    return MisconfiguredError("Generator", "RandomPermutation",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::RandomPermutation(*rng_, n);
}

absl::StatusOr<int64_t> Generator::TryRandomInteger(int64_t min, int64_t max) {
  if (!rng_) {
    return MisconfiguredError("Generator", "RandomInteger",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::RandomInteger(*rng_, min, max);
}

absl::StatusOr<int64_t> Generator::TryRandomInteger(int64_t n) {
  if (!rng_) {
    return MisconfiguredError("Generator", "RandomInteger",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::RandomInteger(*rng_, n);
}

std::vector<int> Generator::RandomPermutation(int n) {
  return moriarty_internal::TryFunctionOrCrash<std::vector<int>>(
      [&, this]() { return this->TryRandomPermutation(n); },
      "RandomPermutation");
}

int64_t Generator::RandomInteger(int64_t min, int64_t max) {
  return moriarty_internal::TryFunctionOrCrash<int64_t>(
      [&, this]() { return this->TryRandomInteger(min, max); },
      "RandomInteger");
}

int64_t Generator::RandomInteger(int64_t n) {
  return moriarty_internal::TryFunctionOrCrash<int64_t>(
      [&, this]() { return this->TryRandomInteger(n); }, "RandomInteger");
}

namespace moriarty_internal {

GeneratorManager::GeneratorManager(Generator* generator_to_manage)
    : managed_generator_(*generator_to_manage) {}

const std::vector<std::shared_ptr<TestCase>>& GeneratorManager::GetTestCases() {
  return managed_generator_.GetTestCases();
}

std::optional<const moriarty_internal::VariableSet>
GeneratorManager::GetGeneralConstraints() {
  return managed_generator_.GetGeneralConstraints();
}

const std::vector<std::shared_ptr<TestCase>>&
GeneratorManager::GetOptionalTestCases() {
  return managed_generator_.GetOptionalTestCases();
}

absl::StatusOr<std::vector<ValueSet>>
GeneratorManager::AssignValuesInAllTestCases() {
  return managed_generator_.AssignValuesInAllTestCases();
}

void GeneratorManager::SetSeed(absl::Span<const int64_t> seed) {
  managed_generator_.SetSeed(seed);
}

void GeneratorManager::SetGeneralConstraints(VariableSet general_constraints) {
  managed_generator_.SetGeneralConstraints(std::move(general_constraints));
}

void GeneratorManager::SetApproximateGenerationLimit(int64_t limit) {
  managed_generator_.SetApproximateGenerationLimit(limit);
}

void GeneratorManager::ClearCases() { managed_generator_.ClearCases(); }

absl::Nullable<moriarty_internal::RandomEngine*>
GeneratorManager::GetRandomEngine() {
  return managed_generator_.GetRandomEngine();
}

}  // namespace moriarty_internal

}  // namespace moriarty
