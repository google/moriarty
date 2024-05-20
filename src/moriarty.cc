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

#include "src/moriarty.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/span.h"
#include "src/generator.h"
#include "src/internal/status_utils.h"
#include "src/internal/universe.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/test_case.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {

Moriarty& Moriarty::SetName(absl::string_view name) {
  name_ = name;
  return *this;
}

Moriarty& Moriarty::SetNumCases(int num_cases) {
  moriarty_internal::TryFunctionOrCrash(
      [&]() { return TrySetNumCases(num_cases); }, "SetNumCases");
  return *this;
}

absl::Status Moriarty::TrySetNumCases(int num_cases) {
  if (num_cases < 0)
    return absl::InvalidArgumentError("num_cases must be non-negative");

  num_cases_ = num_cases;
  return absl::OkStatus();
}

Moriarty& Moriarty::SetSeed(absl::string_view seed) {
  moriarty_internal::TryFunctionOrCrash([&]() { return TrySetSeed(seed); },
                                        "SetSeed");
  return *this;
}

absl::Status Moriarty::TrySetSeed(absl::string_view seed) {
  if (seed.size() < kMinimumSeedLength) {
    return absl::InvalidArgumentError(absl::Substitute(
        "The seed's length must be at least $0", kMinimumSeedLength));
  }

  // Each generator receives a random engine whose seed only differs in the
  // final index. This copies in the first seed.size() here, then
  // GetSeedForGenerator() deals with the final value that changes for each
  // generator.
  seed_.resize(seed.size() + 1);
  for (int i = 0; i < seed.size(); i++) seed_[i] = seed[i];
  return absl::OkStatus();
}

absl::StatusOr<absl::Span<const int64_t>> Moriarty::GetSeedForGenerator(
    int index) {
  if (seed_.empty())
    return absl::FailedPreconditionError(
        "GetSeedForGenerator should only be called after a valid seed has been "
        "set.");
  seed_.back() = index;
  return seed_;
}

void Moriarty::GenerateTestCases() {
  moriarty_internal::TryFunctionOrCrash(
      [this]() { return this->TryGenerateTestCases(); }, "GenerateTestCases");
}

absl::Status Moriarty::TryGenerateTestCases() {
  if (generators_.empty()) {
    return absl::FailedPreconditionError(
        "no generators were found, maybe you need to add them?");
  }

  // The approximate size of all data generated
  int total_approximate_size = 0;

  int generator_idx = 0;
  for (auto& [generator_name, generator_ptr, call_n_times] : generators_) {
    moriarty_internal::GeneratorManager generator_manager(generator_ptr.get());
    MORIARTY_ASSIGN_OR_RETURN(auto seed, GetSeedForGenerator(generator_idx),
                              _ << "error retrieving seed");
    generator_manager.SetSeed(seed);
    for (int call = 1; call <= call_n_times; call++) {
      generator_manager.ClearCases();
      generator_manager.SetGeneralConstraints(variables_);
      if (approximate_generation_limit_) {
        generator_manager.SetApproximateGenerationLimit(
            *approximate_generation_limit_);
      }
      generator_ptr->GenerateTestCases();

      MORIARTY_ASSIGN_OR_RETURN(
          std::vector<moriarty_internal::ValueSet> test_cases,
          generator_manager.AssignValuesInAllTestCases(),
          _ << "Assigning variables in GenerateTestCases() failed.");

      int case_number = 1;
      for (moriarty_internal::ValueSet values : test_cases) {
        total_approximate_size += values.GetApproximateSize();
        assigned_test_cases_.push_back(std::move(values));
        test_case_metadata_.push_back(
            TestCaseMetadata()
                .SetTestCaseNumber(assigned_test_cases_.size())
                .SetGeneratorMetadata(
                    {.generator_name = generator_name,
                     .generator_iteration = call,
                     .case_number_in_generator = case_number++}));
      }

      if (approximate_generation_limit_ &&
          total_approximate_size >= *approximate_generation_limit_) {
        return absl::OkStatus();
      }
    }
    generator_idx++;
  }
  return absl::OkStatus();
}

absl::Status Moriarty::TryValidateTestCases() {
  if (assigned_test_cases_.empty()) {
    return absl::FailedPreconditionError("No TestCases to validate.");
  }

  int case_num = 1;
  for (const moriarty_internal::ValueSet& test_case : assigned_test_cases_) {
    MORIARTY_RETURN_IF_ERROR(TryValidateSingleTestCase(test_case))
        << "Case " << case_num << " invalid";
    case_num++;
  }
  return absl::OkStatus();
}

absl::Status Moriarty::TryValidateSingleTestCase(
    const moriarty_internal::ValueSet& values) {
  moriarty_internal::Universe universe = moriarty_internal::Universe()
                                             .SetConstValueSet(&values)
                                             .SetConstVariableSet(&variables_);

  variables_.SetUniverse(&universe);
  return variables_.AllVariablesSatisfyConstraints();
}

}  // namespace moriarty
