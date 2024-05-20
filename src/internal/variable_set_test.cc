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

#include "src/internal/variable_set.h"

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/types/span.h"
#include "src/errors.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/generation_bootstrap.h"
#include "src/internal/random_engine.h"
#include "src/internal/universe.h"
#include "src/internal/value_set.h"
#include "src/librarian/test_utils.h"
#include "src/scenario.h"
#include "src/testing/mtest_type.h"
#include "src/testing/mtest_type2.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::moriarty_testing::Generate;
using ::moriarty_testing::IsMisconfigured;
using ::moriarty_testing::IsUnsatisfiedConstraint;
using ::moriarty_testing::IsValueNotFound;
using ::moriarty_testing::IsVariableNotFound;
using ::moriarty_testing::MTestType;
using ::moriarty_testing::MTestType2;
using ::moriarty_testing::TestType;
using ::testing::HasSubstr;
using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;

TEST(VariableSetTest, GetVariableCanRetrieveFromAddVariable) {
  VariableSet v;
  MORIARTY_ASSERT_OK(v.AddVariable("A", MTestType().Is(111111)));
  MORIARTY_ASSERT_OK(v.AddVariable("B", MTestType().Is(222222)));

  MORIARTY_ASSERT_OK_AND_ASSIGN(MTestType A,
                                         v.GetVariable<MTestType>("A"));
  EXPECT_THAT(Generate(A), IsOkAndHolds(111111));

  MORIARTY_ASSERT_OK_AND_ASSIGN(MTestType B,
                                         v.GetVariable<MTestType>("B"));
  EXPECT_THAT(Generate(B), IsOkAndHolds(222222));
}

TEST(VariableSetTest, AddOrMergeVariableSetsProperlyWhenNotMerging) {
  VariableSet v;
  MORIARTY_ASSERT_OK(v.AddOrMergeVariable("A", MTestType().Is(111111)));
  MORIARTY_ASSERT_OK(v.AddOrMergeVariable("B", MTestType().Is(222222)));

  MORIARTY_ASSERT_OK_AND_ASSIGN(MTestType A,
                                         v.GetVariable<MTestType>("A"));
  EXPECT_THAT(Generate(A), IsOkAndHolds(111111));

  MORIARTY_ASSERT_OK_AND_ASSIGN(MTestType B,
                                         v.GetVariable<MTestType>("B"));
  EXPECT_THAT(Generate(B), IsOkAndHolds(222222));
}

TEST(VariableSetTest, AddOrMergeVariableSetsProperlyWhenMerging) {
  VariableSet v;
  MORIARTY_ASSERT_OK(v.AddOrMergeVariable("A", MTestType().IsOneOf({11, 22})));
  MORIARTY_ASSERT_OK(v.AddOrMergeVariable("A", MTestType().IsOneOf({22, 33})));

  MORIARTY_ASSERT_OK_AND_ASSIGN(MTestType A,
                                         v.GetVariable<MTestType>("A"));
  EXPECT_THAT(Generate(A), IsOkAndHolds(22));
}

TEST(VariableSetTest, GetVariableOnNonExistentVariableFails) {
  VariableSet v;

  EXPECT_THAT(v.GetAbstractVariable("unknown"), IsVariableNotFound("unknown"));
  EXPECT_THAT(v.GetVariable<MTestType>("unknown"),
              IsVariableNotFound("unknown"));
}

TEST(VariableSetTest, GeneralScenariosAreAppliedToAllVariables) {
  RandomEngine random({1, 2, 3}, "");
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MTestType()));
  MORIARTY_ASSERT_OK(variables.AddVariable("B", MTestType()));
  MORIARTY_ASSERT_OK(variables.AddVariable("C", MTestType2()));
  MORIARTY_ASSERT_OK(variables.AddVariable("D", MTestType2()));
  MORIARTY_EXPECT_OK(variables.WithScenario(Scenario().WithGeneralProperty(
      {.category = "size", .descriptor = "small"})));
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      ValueSet values,
      GenerateAllValues(variables, /* known_values = */ ValueSet(), {random}));

  EXPECT_THAT(values.Get<MTestType>("A"),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
  EXPECT_THAT(values.Get<MTestType>("B"),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
  EXPECT_THAT(values.Get<MTestType2>("C"),
              IsOkAndHolds(MTestType2::kGeneratedValueSmallSize));
  EXPECT_THAT(values.Get<MTestType2>("D"),
              IsOkAndHolds(MTestType2::kGeneratedValueSmallSize));
}

TEST(VariableSetTest,
     TypeSpecificScenariosAreAppliedToAllAppropriateVariables) {
  RandomEngine random({1, 2, 3}, "");
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MTestType()));
  MORIARTY_ASSERT_OK(variables.AddVariable("B", MTestType()));
  MORIARTY_ASSERT_OK(variables.AddVariable("C", MTestType2()));
  MORIARTY_ASSERT_OK(variables.AddVariable("D", MTestType2()));
  MORIARTY_EXPECT_OK(variables.WithScenario(Scenario().WithTypeSpecificProperty(
      "MTestType", {.category = "size", .descriptor = "small"})));
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      ValueSet values,
      GenerateAllValues(variables, /* known_values = */ ValueSet(), {random}));

  EXPECT_THAT(values.Get<MTestType>("A"),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
  EXPECT_THAT(values.Get<MTestType>("B"),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
  EXPECT_THAT(values.Get<MTestType2>("C"),
              IsOkAndHolds(MTestType2::kGeneratedValue));
  EXPECT_THAT(values.Get<MTestType2>("D"),
              IsOkAndHolds(MTestType2::kGeneratedValue));
}

TEST(VariableSetTest,
     DifferentTypeSpecificScenariosArePassedToTheAppropriateType) {
  RandomEngine random({1, 2, 3}, "");
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MTestType()));
  MORIARTY_ASSERT_OK(variables.AddVariable("B", MTestType()));
  MORIARTY_ASSERT_OK(variables.AddVariable("C", MTestType2()));
  MORIARTY_ASSERT_OK(variables.AddVariable("D", MTestType2()));
  MORIARTY_EXPECT_OK(variables.WithScenario(Scenario().WithTypeSpecificProperty(
      "MTestType", {.category = "size", .descriptor = "small"})));
  MORIARTY_EXPECT_OK(variables.WithScenario(Scenario().WithTypeSpecificProperty(
      "MTestType2", {.category = "size", .descriptor = "large"})));
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      ValueSet values,
      GenerateAllValues(variables, /* known_values = */ ValueSet(), {random}));

  EXPECT_THAT(values.Get<MTestType>("A"),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
  EXPECT_THAT(values.Get<MTestType>("B"),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
  EXPECT_THAT(values.Get<MTestType2>("C"),
              IsOkAndHolds(MTestType2::kGeneratedValueLargeSize));
  EXPECT_THAT(values.Get<MTestType2>("D"),
              IsOkAndHolds(MTestType2::kGeneratedValueLargeSize));
}

TEST(VariableSetTest, UnknownMandatoryPropertyInScenarioShouldFail) {
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MTestType()));
  MORIARTY_ASSERT_OK(variables.AddVariable("B", MTestType()));
  MORIARTY_ASSERT_OK(variables.AddVariable("C", MTestType2()));
  MORIARTY_ASSERT_OK(variables.AddVariable("D", MTestType2()));

  EXPECT_THAT(variables.WithScenario(Scenario().WithTypeSpecificProperty(
                  "MTestType", {.category = "unknown", .descriptor = "????"})),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Property with non-optional category")));
}

TEST(VariableSetTest, AllVariablesSatisfyConstraintsSucceedsWithNoVariables) {
  VariableSet variables;
  Universe universe = Universe().SetConstVariableSet(&variables);

  variables.SetUniverse(&universe);
  MORIARTY_EXPECT_OK(variables.AllVariablesSatisfyConstraints());
}

TEST(VariableSetTest, AllVariablesSatisfyConstraintsSucceedsInNormalCase) {
  VariableSet variables;
  ValueSet values;
  Universe universe =
      Universe().SetConstValueSet(&values).SetConstVariableSet(&variables);

  std::vector<TestType> options;
  for (int i = 0; i < 100; i++) options.push_back(TestType(i));
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MTestType().IsOneOf(options)));
  MORIARTY_ASSERT_OK(variables.AddVariable("B", MTestType().IsOneOf(options)));

  values.Set<MTestType>("A", options[4]);
  values.Set<MTestType>("B", options[53]);

  variables.SetUniverse(&universe);
  MORIARTY_EXPECT_OK(variables.AllVariablesSatisfyConstraints());
}

TEST(VariableSetTest,
     AllVariablesSatisfyConstraintsFailIfAtLeastOneValueFails) {
  VariableSet variables;
  ValueSet values;
  Universe universe =
      Universe().SetConstValueSet(&values).SetConstVariableSet(&variables);

  std::vector<TestType> options;
  for (int i = 0; i < 100; i++) options.push_back(TestType(i));
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MTestType().IsOneOf(options)));
  MORIARTY_ASSERT_OK(variables.AddVariable("B", MTestType().IsOneOf(options)));

  values.Set<MTestType>("A", options[4]);
  values.Set<MTestType>("B", TestType(100000));  // Not in the list!

  variables.SetUniverse(&universe);
  EXPECT_THAT(variables.AllVariablesSatisfyConstraints(),
              IsUnsatisfiedConstraint("B"));
}

TEST(VariableSetTest, AllVariablesSatisfyConstraintsFailIfAnyValueIsMissing) {
  VariableSet variables;
  ValueSet values;
  Universe universe =
      Universe().SetConstValueSet(&values).SetConstVariableSet(&variables);

  std::vector<TestType> options;
  for (int i = 0; i < 100; i++) options.push_back(TestType(i));
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MTestType().IsOneOf(options)));
  MORIARTY_ASSERT_OK(variables.AddVariable("B", MTestType().IsOneOf(options)));

  values.Set<MTestType>("A", options[4]);

  variables.SetUniverse(&universe);
  EXPECT_THAT(variables.AllVariablesSatisfyConstraints(), IsValueNotFound("B"));
}

TEST(VariableSetTest,
     AllVariablesSatisfyConstraintsSucceedsIfThereAreExtraValues) {
  VariableSet variables;
  ValueSet values;
  Universe universe =
      Universe().SetConstValueSet(&values).SetConstVariableSet(&variables);

  std::vector<TestType> options;
  for (int i = 0; i < 100; i++) options.push_back(TestType(i));
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MTestType().IsOneOf(options)));
  MORIARTY_ASSERT_OK(variables.AddVariable("B", MTestType().IsOneOf(options)));

  values.Set<MTestType>("A", options[30]);
  values.Set<MTestType>("B", options[40]);
  values.Set<MTestType>("C", options[50]);

  variables.SetUniverse(&universe);
  MORIARTY_EXPECT_OK(variables.AllVariablesSatisfyConstraints());
}

TEST(VariableSetTest,
     AllVariablesSatisfyConstraintsWorksForDependentVariables) {
  VariableSet variables;
  ValueSet values;
  Universe universe =
      Universe().SetConstValueSet(&values).SetConstVariableSet(&variables);

  MORIARTY_ASSERT_OK(variables.AddVariable("A", MTestType()));
  MORIARTY_ASSERT_OK(variables.AddVariable("B", MTestType().SetAdder("A")));

  TestType valA = MTestType::kGeneratedValue;
  TestType valB = 2 * MTestType::kGeneratedValue;  // One by default, one for A

  values.Set<MTestType>("A", valA);
  values.Set<MTestType>("B", valB);

  variables.SetUniverse(&universe);
  MORIARTY_EXPECT_OK(variables.AllVariablesSatisfyConstraints());
}

TEST(VariableSetTest, AllVariablesSatisfyConstraintsFailsIfMissingValues) {
  VariableSet variables;
  Universe universe = Universe().SetConstVariableSet(&variables);

  MORIARTY_ASSERT_OK(variables.AddVariable("A", MTestType()));

  variables.SetUniverse(&universe);
  EXPECT_THAT(variables.AllVariablesSatisfyConstraints(),
              IsMisconfigured(InternalConfigurationType::kValueSet));
}

TEST(VariableSetTest, AllVariablesSatisfyConstraintsFailsIfMissingConstraints) {
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MTestType()));

  EXPECT_THAT(variables.AllVariablesSatisfyConstraints(),
              IsMisconfigured(InternalConfigurationType::kUniverse));
}

// TODO(darcybest): Consider adding tests for `SetUniverse()`. There
// is currently no public API to check that it has been set. It is implicitly
// checked via other methods (particularly, `AssignAllValues`).

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
