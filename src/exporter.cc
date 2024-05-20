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

#include "src/exporter.h"

#include <optional>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "src/errors.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/status_utils.h"
#include "src/internal/universe.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/io_config.h"
#include "src/test_case.h"

namespace moriarty {

void Exporter::ExportTestCases() {
  StartExport();

  auto universe = moriarty_internal::Universe()
                      .SetConstVariableSet(&general_constraints_)
                      .SetIOConfig(io_config_);
  general_constraints_.SetUniverse(&universe);

  for (int index = 0; index < all_values_.size(); index++) {
    current_values_ = all_values_[index];
    current_metadata_ = all_metadata_[index];
    universe.SetConstValueSet(&(*current_values_));

    ExportTestCase();
    current_values_ = std::nullopt;  // Unset it so they do not access it later.

    if (index + 1 != all_values_.size()) TestCaseDivider();
  }

  EndExport();
}

TestCaseMetadata Exporter::GetTestCaseMetadata() const {
  return moriarty_internal::TryFunctionOrCrash<TestCaseMetadata>(
      [this]() { return this->TryGetTestCaseMetadata(); },
      "GetTestCaseMetadata");
}

absl::StatusOr<TestCaseMetadata> Exporter::TryGetTestCaseMetadata() const {
  if (!current_metadata_) {
    return MisconfiguredError("Exporter", "TryGetTestCaseMetadata",
                              InternalConfigurationType::kTestCaseMetadata);
  }
  return *current_metadata_;
}

int Exporter::NumTestCases() const { return all_values_.size(); }

void Exporter::SetIOConfig(librarian::IOConfig* io_config) {
  io_config_ = io_config;
}

librarian::IOConfig* Exporter::GetIOConfig() { return io_config_; }

void Exporter::SetAllValues(std::vector<moriarty_internal::ValueSet> values) {
  all_values_ = std::move(values);
}

void Exporter::SetTestCaseMetadata(std::vector<TestCaseMetadata> metadata) {
  ABSL_CHECK_EQ(metadata.size(), all_values_.size());
  all_metadata_ = std::move(metadata);
}

absl::StatusOr<moriarty_internal::AbstractVariable*>
Exporter::GetAbstractVariable(absl::string_view variable_name) {
  return general_constraints_.GetAbstractVariable(variable_name);
}

void Exporter::SetGeneralConstraints(
    moriarty_internal::VariableSet general_constraints) {
  general_constraints_ = std::move(general_constraints);
}

// -----------------------------------------------------------------------------
//   Internal Extended API

namespace moriarty_internal {

ExporterManager::ExporterManager(moriarty::Exporter* exporter_to_manage)
    : managed_exporter_(*exporter_to_manage) {}

void ExporterManager::SetAllValues(std::vector<ValueSet> values) {
  managed_exporter_.SetAllValues(std::move(values));
}

void ExporterManager::SetTestCaseMetadata(
    std::vector<TestCaseMetadata> metadata) {
  return managed_exporter_.SetTestCaseMetadata(std::move(metadata));
}

absl::StatusOr<AbstractVariable*> ExporterManager::GetAbstractVariable(
    absl::string_view variable_name) {
  return managed_exporter_.GetAbstractVariable(variable_name);
}

void ExporterManager::SetGeneralConstraints(VariableSet general_constraints) {
  managed_exporter_.SetGeneralConstraints(std::move(general_constraints));
}

librarian::IOConfig* ExporterManager::GetIOConfig() {
  return managed_exporter_.GetIOConfig();
}

}  // namespace moriarty_internal

}  // namespace moriarty
