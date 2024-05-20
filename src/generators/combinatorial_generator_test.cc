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

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "src/generator.h"
#include "src/internal/combinatorial_coverage.h"
#include "src/internal/combinatorial_coverage_test_util.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/testing/mtest_type.h"
#include "src/util/test_status_macro/status_testutil.h"

namespace moriarty {

using ::moriarty::CombinatorialCoverage;

namespace {

int MapValueToTestCaseNumber(int val) {
  // These numbers come from the difficult cases implementation in MTestType.
  // This only works since they share the same cases.
  switch (val) {
    case 2:
      return 0;
    case 3:
      return 1;
    default:
      return -1;
  }
}

std::vector<CoveringArrayTestCase> CasesToCoveringArray(
    std::vector<moriarty_internal::ValueSet> cases) {
  std::vector<CoveringArrayTestCase> cov_array;
  for (moriarty_internal::ValueSet& valset : cases) {
    CoveringArrayTestCase catc;
    std::vector<int> dims;

    dims.push_back(MapValueToTestCaseNumber(
        valset.Get<moriarty_testing::MTestType>("X").value().value));
    dims.push_back(MapValueToTestCaseNumber(
        valset.Get<moriarty_testing::MTestType>("Y").value().value));
    catc.test_case = dims;
    cov_array.push_back(catc);
  }
  return cov_array;
}

TEST(CombinatorialCoverage, GenerateShouldCreateCasesFromCoveringArray) {
  CombinatorialCoverage generator;
  moriarty_internal::GeneratorManager generator_manager(&generator);
  generator_manager.SetSeed({1, 2, 3, 4});
  moriarty_internal::VariableSet varset;
  MORIARTY_ASSERT_OK(varset.AddVariable("X", moriarty_testing::MTestType()));
  MORIARTY_ASSERT_OK(varset.AddVariable("Y", moriarty_testing::MTestType()));

  generator_manager.SetGeneralConstraints(varset);
  generator.GenerateTestCases();

  std::vector<moriarty_internal::ValueSet> mycases =
      generator_manager.AssignValuesInAllTestCases().value();
  std::vector<CoveringArrayTestCase> catc = CasesToCoveringArray(mycases);

  EXPECT_THAT(catc, IsStrength2CoveringArray(std::vector<int>({2, 2})));
}

}  // namespace
}  // namespace moriarty
