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

#include "src/importer.h"

#include <optional>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/universe.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/io_config.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {

absl::Status Importer::ImportTestCases() {
  MORIARTY_RETURN_IF_ERROR(StartImport());

  if (num_test_cases_) {
    MORIARTY_RETURN_IF_ERROR(ImportTestCasesWithKnownNumberOfCases());
  } else {
    MORIARTY_RETURN_IF_ERROR(ImportTestCasesWithUnknownNumberOfCases());
  }

  MORIARTY_RETURN_IF_ERROR(EndImport());

  return absl::OkStatus();
}

absl::Status Importer::ImportTestCasesWithKnownNumberOfCases() {
  auto universe = moriarty_internal::Universe()
                      .SetConstVariableSet(&general_constraints_)
                      .SetIOConfig(io_config_);
  general_constraints_.SetUniverse(&universe);

  metadata_.num_test_cases = *num_test_cases_;
  for (int tc = 1; tc <= *num_test_cases_; tc++) {
    metadata_.test_case_number = tc;
    current_test_case_ = {};
    universe.SetMutableValueSet(&current_test_case_);

    MORIARTY_RETURN_IF_ERROR(ImportTestCase());
    test_cases_.push_back(std::move(current_test_case_));

    if (tc != num_test_cases_) {
      MORIARTY_RETURN_IF_ERROR(TestCaseDivider());
    }

    if (IsDone()) {
      return absl::FailedPreconditionError(
          "Cannot call both Done() and SetNumTestCases()");
    }
  }

  return absl::OkStatus();
}

absl::Status Importer::ImportTestCasesWithUnknownNumberOfCases() {
  auto universe = moriarty_internal::Universe()
                      .SetConstVariableSet(&general_constraints_)
                      .SetIOConfig(io_config_);
  general_constraints_.SetUniverse(&universe);

  metadata_.num_test_cases = 0;
  while (!IsDone()) {
    metadata_.test_case_number = test_cases_.size() + 1;
    current_test_case_ = {};
    universe.SetMutableValueSet(&current_test_case_);

    MORIARTY_RETURN_IF_ERROR(ImportTestCase());
    if (IsDone()) break;
    test_cases_.push_back(std::move(current_test_case_));

    MORIARTY_RETURN_IF_ERROR(TestCaseDivider());
  }

  return absl::OkStatus();
}

void Importer::Done() { done_ = true; }

bool Importer::IsDone() const { return done_; }

void Importer::SetNumTestCases(int num_test_cases) {
  num_test_cases_ = num_test_cases;
}

Importer::TestCaseMetadata Importer::GetTestCaseMetadata() const {
  return metadata_;
}

void Importer::SetIOConfig(librarian::IOConfig* io_config) {
  io_config_ = io_config;
}

void Importer::SetGeneralConstraints(
    moriarty_internal::VariableSet general_constraints) {
  general_constraints_ = std::move(general_constraints);
}

std::vector<moriarty_internal::ValueSet> Importer::GetTestCases() const {
  return test_cases_;
}

absl::StatusOr<moriarty_internal::AbstractVariable*>
Importer::GetAbstractVariable(absl::string_view variable_name) {
  return general_constraints_.GetAbstractVariable(variable_name);
}

const moriarty_internal::ValueSet& Importer::GetCurrentTestCase() const {
  return current_test_case_;
}

librarian::IOConfig* Importer::GetIOConfig() { return io_config_; }

namespace moriarty_internal {

ImporterManager::ImporterManager(moriarty::Importer* importer_to_manage)
    : managed_importer_(*importer_to_manage) {}

void ImporterManager::SetGeneralConstraints(VariableSet general_constraints) {
  managed_importer_.SetGeneralConstraints(std::move(general_constraints));
}

std::vector<moriarty_internal::ValueSet> ImporterManager::GetTestCases() const {
  return managed_importer_.GetTestCases();
}

absl::StatusOr<AbstractVariable*> ImporterManager::GetAbstractVariable(
    absl::string_view variable_name) {
  return managed_importer_.GetAbstractVariable(variable_name);
}

const ValueSet& ImporterManager::GetCurrentTestCase() const {
  return managed_importer_.GetCurrentTestCase();
}

librarian::IOConfig* ImporterManager::GetIOConfig() {
  return managed_importer_.GetIOConfig();
}

}  // namespace moriarty_internal

}  // namespace moriarty
