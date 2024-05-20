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

#include <optional>
#include <string>
#include <tuple>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/scenario.h"
#include "src/testing/mtest_type.h"
#include "src/testing/mtest_type2.h"
#include "src/testing/status_test_util.h"
#include "src/util/status_macro/status_macros.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty::moriarty_internal::RandomEngine;
using ::moriarty::moriarty_internal::TestCaseManager;
using ::moriarty::moriarty_internal::ValueSet;
using ::moriarty::moriarty_internal::VariableSet;
using ::moriarty_testing::IsVariableNotFound;
using ::moriarty_testing::MTestType;
using ::moriarty_testing::MTestType2;
using ::moriarty_testing::TestType;
using ::testing::HasSubstr;
using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;

TEST(TestCaseTest, ConstrainVariableAndGetVariableWorkInGeneralCase) {
  TestCase T;
  T.ConstrainVariable("A", MTestType());
  MORIARTY_EXPECT_OK(TestCaseManager(&T).GetVariable<MTestType>("A"));
}

TEST(TestCaseTest, SetValueWithSpecificValueAndGetVariableWorkInGeneralCase) {
  TestCase T;
  T.SetValue<MTestType>("A", TestType());
  MORIARTY_EXPECT_OK(TestCaseManager(&T).GetVariable<MTestType>("A"));
}

TEST(TestCaseTest, GetVariableWithWrongTypeShouldFail) {
  TestCase T;
  T.ConstrainVariable("A", MTestType());
  EXPECT_THAT(TestCaseManager(&T).GetVariable<MInteger>("A"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(TestCaseTest, GetVariableWithSpecificValueWithWrongTypeShouldFail) {
  TestCase T;
  T.SetValue<MTestType>("A", TestType());
  EXPECT_THAT(TestCaseManager(&T).GetVariable<MInteger>("A"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(TestCaseTest, GetVariableWithWrongNameShouldNotFind) {
  TestCase T;
  T.ConstrainVariable("A", MTestType());
  EXPECT_THAT(TestCaseManager(&T).GetVariable<MTestType>("xxx"),
              IsVariableNotFound("xxx"));
}

TEST(TestCaseTest, SetVariablesAndGetVariableWorkInGeneralCase) {
  TestCase T;
  VariableSet variable_set;
  MORIARTY_ASSERT_OK(variable_set.AddVariable("A", MTestType()));

  TestCaseManager(&T).SetVariables(variable_set);
  MORIARTY_EXPECT_OK(TestCaseManager(&T).GetVariable<MTestType>("A"));
  EXPECT_THAT(TestCaseManager(&T).GetVariable<MTestType>("xxx"),
              IsVariableNotFound("xxx"));
}

TEST(TestCaseTest, SetVariablesOverwritesAllVariablesPreviouslyThere) {
  TestCase T;

  VariableSet variable_set_orig;
  MORIARTY_ASSERT_OK(variable_set_orig.AddVariable("A", MTestType()));
  TestCaseManager(&T).SetVariables(variable_set_orig);

  VariableSet variable_set_new;
  MORIARTY_ASSERT_OK(variable_set_new.AddVariable("B", MTestType()));
  TestCaseManager(&T).SetVariables(variable_set_new);

  EXPECT_THAT(TestCaseManager(&T).GetVariable<MTestType>("A"),
              IsVariableNotFound("A"));
  MORIARTY_EXPECT_OK(TestCaseManager(&T).GetVariable<MTestType>("B"));
}

TEST(TestCaseTest, AssignAllValuesGivesSomeValueForEachVariable) {
  TestCase T;
  T.ConstrainVariable("A", MTestType());
  T.ConstrainVariable("B", MTestType());

  RandomEngine rng({}, "v0.1");
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      ValueSet value_set,
      TestCaseManager(&T).AssignAllValues(rng, std::nullopt));

  EXPECT_TRUE(value_set.Contains("A"));
  EXPECT_TRUE(value_set.Contains("B"));
  EXPECT_FALSE(value_set.Contains("C"));
}

TEST(TestCaseTest, AssignAllValuesShouldGiveRepeatableResults) {
  using NameVariablePair = std::pair<std::string, MTestType>;

  // Generate the value for A, B, C in some order and return those values in the
  // order A, B, C.
  auto gen = [](NameVariablePair first, NameVariablePair second,
                NameVariablePair third)
      -> absl::StatusOr<std::tuple<TestType, TestType, TestType>> {
    TestCase T;
    T.ConstrainVariable(first.first, first.second);
    T.ConstrainVariable(second.first, second.second);
    T.ConstrainVariable(third.first, third.second);

    RandomEngine rng({1, 2, 3}, "v0.1");
    MORIARTY_ASSIGN_OR_RETURN(
        ValueSet value_set,
        TestCaseManager(&T).AssignAllValues(rng, std::nullopt));
    MORIARTY_ASSIGN_OR_RETURN(TestType a, value_set.Get<MTestType>("A"));
    MORIARTY_ASSIGN_OR_RETURN(TestType b, value_set.Get<MTestType>("B"));
    MORIARTY_ASSIGN_OR_RETURN(TestType c, value_set.Get<MTestType>("C"));
    return std::tuple<TestType, TestType, TestType>({a, b, c});
  };

  NameVariablePair a = {"A",
                        MTestType().SetMultiplier(MInteger().Between(1, 10))};
  NameVariablePair b = {"B",
                        MTestType().SetMultiplier(MInteger().Between(1, 3))};
  NameVariablePair c = {"C", MTestType().SetAdder("A")};

  MORIARTY_ASSERT_OK_AND_ASSIGN(auto abc, gen(a, b, c));
  EXPECT_THAT(gen(a, c, b), IsOkAndHolds(abc));
  EXPECT_THAT(gen(b, a, c), IsOkAndHolds(abc));
  EXPECT_THAT(gen(b, c, a), IsOkAndHolds(abc));
  EXPECT_THAT(gen(c, b, a), IsOkAndHolds(abc));
  EXPECT_THAT(gen(c, a, b), IsOkAndHolds(abc));
}

TEST(TestCaseTest, AssignAllValuesShouldRespectSpecificValuesSet) {
  TestCase T;
  T.SetValue<MTestType>("A", TestType(789));
  T.SetValue<MTestType>("B", TestType(654));

  RandomEngine rng({}, "v0.1");
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      ValueSet value_set,
      TestCaseManager(&T).AssignAllValues(rng, std::nullopt));

  EXPECT_THAT(value_set.Get<MTestType>("A"), IsOkAndHolds(TestType(789)));
  EXPECT_THAT(value_set.Get<MTestType>("B"), IsOkAndHolds(TestType(654)));
}

TEST(TestCaseTest, GeneralScenariosShouldBeRespected) {
  TestCase T = TestCase()
                   .ConstrainVariable("A", MTestType())
                   .ConstrainVariable("B", MTestType())
                   .WithScenario(Scenario().WithGeneralProperty(
                       {.category = "size", .descriptor = "small"}));
  RandomEngine rng({}, "v0.1");

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      ValueSet value_set,
      TestCaseManager(&T).AssignAllValues(rng, std::nullopt));

  EXPECT_THAT(value_set.Get<MTestType>("A"),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
  EXPECT_THAT(value_set.Get<MTestType>("B"),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
}

TEST(TestCaseTest, TypeSpecificScenariosAreAppliedToAllAppropriateVariables) {
  TestCase T =
      TestCase()
          .ConstrainVariable("A", MTestType())
          .ConstrainVariable("B", MTestType())
          .ConstrainVariable("C", MTestType2())
          .ConstrainVariable("D", MTestType2())
          .WithScenario(Scenario().WithTypeSpecificProperty(
              "MTestType", {.category = "size", .descriptor = "small"}));
  RandomEngine rng({}, "v0.1");

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      ValueSet value_set,
      TestCaseManager(&T).AssignAllValues(rng, std::nullopt));

  EXPECT_THAT(value_set.Get<MTestType>("A"),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
  EXPECT_THAT(value_set.Get<MTestType>("B"),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
  EXPECT_THAT(value_set.Get<MTestType2>("C"),
              IsOkAndHolds(MTestType2::kGeneratedValue));
  EXPECT_THAT(value_set.Get<MTestType2>("D"),
              IsOkAndHolds(MTestType2::kGeneratedValue));
}

TEST(TestCaseTest,
     DifferentTypeSpecificScenariosArePassedToTheAppropriateType) {
  TestCase T =
      TestCase()
          .ConstrainVariable("A", MTestType())
          .ConstrainVariable("B", MTestType())
          .ConstrainVariable("C", MTestType2())
          .ConstrainVariable("D", MTestType2())
          .WithScenario(Scenario().WithTypeSpecificProperty(
              "MTestType", {.category = "size", .descriptor = "small"}))
          .WithScenario(Scenario().WithTypeSpecificProperty(
              "MTestType2", {.category = "size", .descriptor = "large"}));
  RandomEngine rng({}, "v0.1");

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      ValueSet value_set,
      TestCaseManager(&T).AssignAllValues(rng, std::nullopt));

  EXPECT_THAT(value_set.Get<MTestType>("A"),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
  EXPECT_THAT(value_set.Get<MTestType>("B"),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
  EXPECT_THAT(value_set.Get<MTestType2>("C"),
              IsOkAndHolds(MTestType2::kGeneratedValueLargeSize));
  EXPECT_THAT(value_set.Get<MTestType2>("D"),
              IsOkAndHolds(MTestType2::kGeneratedValueLargeSize));
}

TEST(TestCaseTest, UnknownMandatoryPropertyInScenarioShouldFail) {
  TestCase T =
      TestCase()
          .ConstrainVariable("A", MTestType())
          .ConstrainVariable("B", MTestType())
          .ConstrainVariable("C", MTestType2())
          .ConstrainVariable("D", MTestType2())
          .WithScenario(Scenario().WithTypeSpecificProperty(
              "MTestType", {.category = "unknown", .descriptor = "???"}));
  RandomEngine rng({}, "v0.1");

  EXPECT_THAT(TestCaseManager(&T).AssignAllValues(rng, std::nullopt),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Property with non-optional category")));
}

}  // namespace
}  // namespace moriarty
