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

#include "src/generators/combinatorial_generator.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "src/generator.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/combinatorial_coverage.h"
#include "src/internal/random_engine.h"
#include "src/internal/variable_set.h"
#include "src/test_case.h"

namespace moriarty {

void CombinatorialCoverage::GenerateTestCases() {
  moriarty_internal::GeneratorManager generator_manager(this);
  // TODO(hivini): Handle the (two) crashes better to inform the user about the
  // error.
  moriarty_internal::VariableSet var_set =
      generator_manager.GetGeneralConstraints().value();

  absl::Nullable<moriarty_internal::RandomEngine*> random_engine =
      generator_manager.GetRandomEngine();
  ABSL_CHECK_NE(random_engine, nullptr);

  InitializeCasesInfo cases_info = InitializeCases(var_set.GetAllVariables());

  std::function<int(int)> rand_f = [random_engine](int n) -> int {
    int result = random_engine->RandInt(n).value() % n;
    return result;
  };

  std::vector<CoveringArrayTestCase> covering_array = GenerateCoveringArray(
      cases_info.dimension_sizes, cases_info.dimension_sizes.size(), rand_f);
  CreateTestCases(covering_array, cases_info.cases, cases_info.variable_names);
}

void CombinatorialCoverage::CreateTestCases(
    absl::Span<const CoveringArrayTestCase> covering_array,
    absl::Span<
        const std::vector<std::unique_ptr<moriarty_internal::AbstractVariable>>>
        cases,
    absl::Span<const std::string> variable_names) {
  for (const CoveringArrayTestCase& x : covering_array) {
    TestCase& test_case = AddTestCase();
    for (int i = 0; i < x.test_case.size(); i++) {
      moriarty_internal::TestCaseManager manager(&test_case);
      manager.ConstrainVariable(variable_names[i],
                                *(cases[i][x.test_case[i]].get()));
    }
  }
}

InitializeCasesInfo CombinatorialCoverage::InitializeCases(
    const absl::flat_hash_map<
        std::string, std::unique_ptr<moriarty_internal::AbstractVariable>>&
        vars) {
  InitializeCasesInfo info;
  for (const auto& [name, var_ptr] : vars) {
    std::vector<std::unique_ptr<moriarty_internal::AbstractVariable>>
        difficult_vars = var_ptr->GetDifficultAbstractVariables().value();
    info.dimension_sizes.push_back(difficult_vars.size());
    info.cases.push_back(std::move(difficult_vars));
    info.variable_names.push_back(name);
  }
  return info;
}

}  // namespace moriarty
