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

#include "src/internal/universe.h"

#include <type_traits>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/types/span.h"
#include "src/errors.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/generation_config.h"
#include "src/internal/random_config.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_name_utils.h"
#include "src/internal/variable_set.h"
#include "src/librarian/io_config.h"
#include "src/testing/mtest_type.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::moriarty_testing::IsMisconfigured;
using ::moriarty_testing::IsValueNotFound;
using ::moriarty_testing::IsVariableNotFound;
using ::moriarty_testing::MTestType;
using ::moriarty_testing::TestType;
using ::testing::HasSubstr;
using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;

TEST(UniverseDeathTest, OnlyOneVariableSetAndOneValueSetCanBeSet) {
  VariableSet c1, c2;
  ValueSet v1, v2;

  EXPECT_DEATH(
      { Universe().SetMutableVariableSet(&c1).SetConstVariableSet(&c2); },
      "Only one of SetConstVariableSet and SetMutableVariableSet can be "
      "called");
  EXPECT_DEATH(
      { Universe().SetConstVariableSet(&c1).SetMutableVariableSet(&c2); },
      "Only one of SetConstVariableSet and SetMutableVariableSet can be "
      "called");

  EXPECT_DEATH(
      { Universe().SetMutableValueSet(&v1).SetConstValueSet(&v2); },
      "Only one of SetConstValueSet and SetMutableValueSet can be called.");
  EXPECT_DEATH(
      { Universe().SetConstValueSet(&v1).SetMutableValueSet(&v2); },
      "Only one of SetConstValueSet and SetMutableValueSet can be called.");
}

TEST(UniverseTest, MutableGetValueReturnsExpectedValues) {
  VariableSet variables;
  ValueSet values;
  RandomEngine rng({1, 2, 3}, "");
  GenerationConfig generation_config;

  Universe universe = Universe()
                          .SetRandomEngine(&rng)
                          .SetMutableVariableSet(&variables)
                          .SetMutableValueSet(&values)
                          .SetGenerationConfig(&generation_config);
  MORIARTY_EXPECT_OK(variables.AddVariable("A", MTestType()));
  MORIARTY_EXPECT_OK(variables.AddVariable("B", MTestType()));
  MORIARTY_EXPECT_OK(universe.SetValue<MTestType>("B", TestType(20)));

  EXPECT_THAT(universe.GetOrGenerateAndSetValue<MInteger>("A.multiplier"),
              IsOkAndHolds(1));
  EXPECT_THAT(universe.GetOrGenerateAndSetValue<MTestType>("A"),
              IsOkAndHolds(MTestType::kGeneratedValue));
  EXPECT_THAT(universe.GetOrGenerateAndSetValue<MTestType>("B"),
              IsOkAndHolds(20));

  // Wrong type...
  EXPECT_THAT(universe.GetOrGenerateAndSetValue<MInteger>("B"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Unable to cast")));
  EXPECT_THAT(universe.GetOrGenerateAndSetValue<MTestType>("A.multiplier"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("returned the wrong type")));
}

TEST(UniverseTest, EraseValueShouldWorkAppropriately) {
  ValueSet values;
  Universe universe = Universe().SetMutableValueSet(&values);
  MORIARTY_EXPECT_OK(universe.SetValue<MTestType>("A", TestType(20)));
  MORIARTY_EXPECT_OK(universe.EraseValue("A"));  // In the Universe
  MORIARTY_EXPECT_OK(universe.EraseValue("X"));  // Not in the Universe
}

TEST(UniverseTest, EraseValueShouldErase) {
  ValueSet values;
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MTestType()));
  Universe universe =
      Universe().SetMutableValueSet(&values).SetMutableVariableSet(&variables);
  MORIARTY_EXPECT_OK(universe.SetValue<MTestType>("A", TestType(20)));
  MORIARTY_EXPECT_OK(universe.EraseValue("A"));
  EXPECT_THAT(universe.GetValue<MTestType>("A"), IsValueNotFound("A"));
}

TEST(UniverseTest, EraseValueWithoutAVariableSetFails) {
  Universe universe;
  EXPECT_THAT(universe.EraseValue("X"),
              IsMisconfigured(InternalConfigurationType::kValueSet));
}

TEST(UniverseTest, GetValueDoesNotAlwaysRequireMutableVariables) {
  VariableSet variables;
  ValueSet values;
  RandomEngine rng({1, 2, 3}, "");
  GenerationConfig generation_config;

  Universe universe = Universe()
                          .SetRandomEngine(&rng)
                          .SetConstVariableSet(&variables)
                          .SetMutableValueSet(&values)
                          .SetGenerationConfig(&generation_config);
  MORIARTY_EXPECT_OK(variables.AddVariable("A", MTestType()));
  MORIARTY_EXPECT_OK(variables.AddVariable("B", MTestType()));
  MORIARTY_EXPECT_OK(universe.SetValue<MTestType>("B", TestType(20)));

  // A needs generation, but B has a value!
  EXPECT_THAT(universe.GetOrGenerateAndSetValue<MTestType>("A"),
              IsMisconfigured(InternalConfigurationType::kVariableSet));
  EXPECT_THAT(universe.GetOrGenerateAndSetValue<MTestType>("B"),
              IsOkAndHolds(20));
}

TEST(UniverseTest, GetValueShouldWorkIfValuesAreProvided) {
  ValueSet values;
  values.Set<MTestType>("A", TestType(10));
  values.Set<MTestType>("B", TestType(20));

  Universe universe = Universe().SetConstValueSet(&values);

  EXPECT_THAT(universe.GetValue<MTestType>("A"), IsOkAndHolds(10));
  EXPECT_THAT(universe.GetValue<MTestType>("B"), IsOkAndHolds(20));
}

TEST(UniverseTest, ConstGetValueDoesNotAttemptToGenerate) {
  VariableSet variables;
  ValueSet values;
  RandomEngine rng({1, 2, 3}, "");
  GenerationConfig generation_config;

  Universe universe = Universe()
                          .SetRandomEngine(&rng)
                          .SetMutableVariableSet(&variables)
                          .SetConstValueSet(&values)
                          .SetGenerationConfig(&generation_config);
  MORIARTY_EXPECT_OK(variables.AddVariable("A", MTestType()));
  MORIARTY_EXPECT_OK(variables.AddVariable("B", MTestType()));

  variables.SetUniverse(&universe);

  // Cannot set a constant ValueSet
  EXPECT_THAT(universe.SetValue<MTestType>("B", TestType(20)),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("called without a mutable ValueSet")));

  // Cannot generate a constant ValueSet
  EXPECT_THAT(universe.GetOrGenerateAndSetValue<MTestType>("A"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Cannot generate 'A' with a const ValueSet")));
  EXPECT_THAT(universe.GetOrGenerateAndSetValue<MTestType>("B"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Cannot generate 'B' with a const ValueSet")));
}

TEST(UniverseTest,
     ConstGetValueOnAValuelessVariableReturnsValueDependingOnUniqueness) {
  VariableSet variables;
  ValueSet values;
  RandomEngine rng({1, 2, 3}, "");
  GenerationConfig generation_config;

  Universe universe = Universe()
                          .SetRandomEngine(&rng)
                          .SetConstVariableSet(&variables)
                          .SetConstValueSet(&values)
                          .SetGenerationConfig(&generation_config);

  MORIARTY_ASSERT_OK(variables.AddVariable("A", MTestType().Is(TestType(123))));
  MORIARTY_ASSERT_OK(variables.AddVariable(
      "B", MTestType().IsOneOf({TestType(456), TestType(789)})));

  variables.SetUniverse(&universe);

  EXPECT_THAT(universe.GetValue<MTestType>("A"), IsOkAndHolds(TestType(123)));
  EXPECT_THAT(universe.GetValue<MTestType>("B"), IsValueNotFound("B"));
  EXPECT_THAT(universe.GetValue<MTestType>("C"), IsVariableNotFound("C"));
}

TEST(UniverseTest,
     RequestingANonExistentVariableInGetFrozenVariableShouldFail) {
  VariableSet variables;
  Universe universe = Universe().SetMutableVariableSet(&variables);

  EXPECT_THAT(universe.GetAbstractVariable("A"), IsVariableNotFound("A"));
  EXPECT_THAT(universe.GetVariable<MTestType>("A"), IsVariableNotFound("A"));
}

TEST(UniverseTest, RequestingSubvalueWithoutSubVariableNameFails) {
  VariableSet variables;
  Universe universe = Universe().SetMutableVariableSet(&variables);

  EXPECT_THAT(universe.GetSubvalue<int>(
                  VariableNameBreakdown({.base_variable_name = "A"})),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("has no subvariable name")));
}

TEST(UniverseTest, GetAbstractVariableWithNoVariableSetShouldFail) {
  EXPECT_THAT(Universe().GetAbstractVariable("A"),
              moriarty_testing::IsMisconfigured(
                  InternalConfigurationType::kVariableSet));

  // Test the const-version of GetAbstractVariable
  Universe universe;
  EXPECT_THAT(std::as_const(universe).GetAbstractVariable("A"),
              IsMisconfigured(InternalConfigurationType::kVariableSet));
}

TEST(UniverseTest, DependentVariablesAreEvaluatedAndPropagated) {
  VariableSet variables;
  ValueSet values;
  RandomEngine rng({1, 2, 3}, "");
  GenerationConfig generation_config;

  Universe universe = Universe()
                          .SetRandomEngine(&rng)
                          .SetMutableVariableSet(&variables)
                          .SetMutableValueSet(&values)
                          .SetGenerationConfig(&generation_config);
  MORIARTY_EXPECT_OK(variables.AddVariable("A", MTestType().SetAdder("B")));
  MORIARTY_EXPECT_OK(variables.AddVariable("B", MTestType()));
  MORIARTY_EXPECT_OK(universe.SetValue<MTestType>("B", TestType(20)));

  EXPECT_THAT(universe.GetOrGenerateAndSetValue<MTestType>("A"),
              IsOkAndHolds(MTestType::kGeneratedValue + 20));
  EXPECT_THAT(universe.GetOrGenerateAndSetValue<MTestType>("B"),
              IsOkAndHolds(20));
}

TEST(UniverseTest, GetAndSetIOConfigShouldBehave) {
  Universe universe;

  // Initially should be `nullptr`.
  EXPECT_EQ(universe.GetIOConfig(), nullptr);
  EXPECT_EQ(std::as_const(universe).GetIOConfig(), nullptr);

  librarian::IOConfig io_config;
  universe.SetIOConfig(&io_config);

  // Comparing pointers.
  EXPECT_EQ(universe.GetIOConfig(), &io_config);
  EXPECT_EQ(std::as_const(universe).GetIOConfig(), &io_config);
}

TEST(UniverseTest, GetAndSetRandomEngineShouldBehave) {
  Universe universe;

  // Initially should be `nullptr`.
  EXPECT_EQ(universe.GetRandomEngine(), nullptr);
  EXPECT_EQ(universe.GetRandomConfig().GetRandomEngine(), nullptr);

  RandomEngine rng({1, 2, 3}, "v0.1");
  universe.SetRandomEngine(&rng);

  // These should point exactly to `rng`.
  EXPECT_EQ(universe.GetRandomEngine(), &rng);
  EXPECT_EQ(universe.GetRandomConfig().GetRandomEngine(), &rng);
}

TEST(UniverseTest, GetAndSetGenerationConfigShouldBehave) {
  Universe universe;

  // Initially should be `nullptr`.
  EXPECT_EQ(universe.GetGenerationConfig(), nullptr);

  RandomEngine rng({1, 2, 3}, "v0.1");
  universe.SetRandomEngine(&rng);

  // These should point exactly to `rng`.
  EXPECT_EQ(universe.GetRandomEngine(), &rng);
  EXPECT_EQ(universe.GetRandomConfig().GetRandomEngine(), &rng);
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
