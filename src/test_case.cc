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

#include "src/test_case.h"

#include <stdint.h>

#include <optional>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/generation_bootstrap.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/scenario.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {

TestCase& TestCase::WithScenario(Scenario scenario) {
  scenarios_.push_back(std::move(scenario));
  return *this;
}

void TestCase::SetVariables(moriarty_internal::VariableSet variables) {
  variables_ = std::move(variables);
}

absl::StatusOr<moriarty_internal::ValueSet> TestCase::AssignAllValues(
    moriarty_internal::RandomEngine& rng,
    std::optional<int64_t> approximate_generation_limit) {
  MORIARTY_RETURN_IF_ERROR(DistributeScenarios());

  return moriarty_internal::GenerateAllValues(
      variables_, /*known_values = */ {}, {rng, approximate_generation_limit});
}

absl::Status TestCase::DistributeScenarios() {
  for (const Scenario& scenario : scenarios_) {
    MORIARTY_RETURN_IF_ERROR(variables_.WithScenario(scenario));
  }
  return absl::OkStatus();
}

TestCase& TestCase::ConstrainVariable(
    absl::string_view variable_name,
    const moriarty_internal::AbstractVariable& var) {
  ABSL_CHECK_OK(variables_.AddOrMergeVariable(variable_name, var));
  return *this;
}

namespace moriarty_internal {

TestCaseManager::TestCaseManager(moriarty::TestCase* test_case_to_manage)
    : managed_test_case_(*test_case_to_manage) {}

void TestCaseManager::SetVariables(VariableSet variables) {
  managed_test_case_.SetVariables(std::move(variables));
}

absl::StatusOr<ValueSet> TestCaseManager::AssignAllValues(
    RandomEngine& rng, std::optional<int64_t> approximate_generation_limit) {
  return managed_test_case_.AssignAllValues(rng, approximate_generation_limit);
}

TestCase& TestCaseManager::ConstrainVariable(
    absl::string_view variable_name,
    const moriarty_internal::AbstractVariable& var) {
  return managed_test_case_.ConstrainVariable(variable_name, var);
}

}  // namespace moriarty_internal

}  // namespace moriarty
