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

#include "src/internal/analysis_bootstrap.h"

#include <cstdint>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::moriarty::MInteger;
using ::testing::HasSubstr;
using ::moriarty::StatusIs;

TEST(AnalyzeBootstrapTest, SatisfiesConstraintsSimpleCaseWorks) {
  MORIARTY_EXPECT_OK(SatisfiesConstraints(MInteger().Between(1, 10), 3));
  EXPECT_THAT(SatisfiesConstraints(MInteger().Between(1, 10), 0),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(AnalyzeBootstrapTest, SatisfiesConstraintsUsesKnownValues) {
  ValueSet values;
  values.Set<MInteger>("N", int64_t{10});
  MORIARTY_EXPECT_OK(
      SatisfiesConstraints(MInteger().Between(1, "N"), 3, values));
  EXPECT_THAT(SatisfiesConstraints(MInteger().Between(1, "N"), 0, values),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(AnalyzeBootstrapTest, SatisfiesConstraintsFailsProperlyOnUnknownValue) {
  ValueSet values;
  values.Set<MInteger>("X", int64_t{10});
  EXPECT_THAT(
      SatisfiesConstraints(MInteger().Between(1, "N"), 0, values),
      StatusIs(absl::StatusCode::kFailedPrecondition, HasSubstr("Unknown")));
}

TEST(AnalyzeBootstrapTest, SatisfiesConstraintsFailsProperlyNonUniqueVariable) {
  ValueSet values;
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("N", MInteger().Between(1, 10)));
  EXPECT_THAT(
      SatisfiesConstraints(MInteger().Between(1, "N"), 0, values),
      StatusIs(absl::StatusCode::kFailedPrecondition, HasSubstr("Unknown")));
}

TEST(AnalyzeBootstrapTest, SatisfiesConstraintsUsesUniqueValuesFromVariables) {
  ValueSet values;
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("N", MInteger().Is(5)));
  MORIARTY_EXPECT_OK(
      SatisfiesConstraints(MInteger().Between(1, "N"), 3, values, variables));
  EXPECT_THAT(
      SatisfiesConstraints(MInteger().Between(1, "N"), 6, values, variables),
      StatusIs(absl::StatusCode::kFailedPrecondition));
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
