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

#include "src/generator.h"

#include <concepts>
#include <cstdint>
#include <optional>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "src/errors.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/size_property.h"
#include "src/librarian/test_utils.h"
#include "src/scenario.h"
#include "src/test_case.h"
#include "src/testing/generator_test_util.h"
#include "src/testing/mtest_type.h"
#include "src/testing/random_test_util.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty::moriarty_internal::GeneratorManager;
using ::moriarty_testing::GenerateSameValues;
using ::moriarty_testing::IsMisconfigured;
using ::moriarty_testing::IsUnsatisfiedConstraint;
using ::moriarty_testing::IsVariableNotFound;
using ::moriarty_testing::MTestType;
using ::moriarty_testing::RunGenerateForTest;
using ::moriarty_testing::SimpleTestTypeGenerator;
using ::moriarty_testing::SingleIntegerGenerator;
using ::moriarty_testing::TwoIntegerGeneratorWithRandomness;
using ::moriarty_testing::TwoTestTypeGenerator;
using ::testing::AllOf;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Le;
using ::testing::SizeIs;
using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;

// EmptyGenerator
//
// Optionally seeds itself with randomness and a set of empty variables.
// `GenerateTestCases()` is a no-op. Useful for tests that need to call
// `AddTestCase()` without having to call `GenerateTestCases()`.
class EmptyGenerator : public moriarty::Generator {
 public:
  EmptyGenerator() {
    moriarty_internal::GeneratorManager(this).SetSeed({1, 2, 3});
    moriarty_internal::GeneratorManager(this).SetGeneralConstraints({});
  }

  // If you do not want any seeding to happen, use this constructor instead.
  explicit EmptyGenerator(bool should_seed) {
    if (!should_seed) return;
    moriarty_internal::GeneratorManager(this).SetSeed({1, 2, 3});
    moriarty_internal::GeneratorManager(this).SetGeneralConstraints({});
  }

  void GenerateTestCases() override {}  // Do nothing.
};

TEST(GeneratorTest, NumberOfCasesShouldMatchNumberOfAddCasesCalled) {
  // 1 case generated
  EXPECT_THAT(RunGenerateForTest(SingleIntegerGenerator()),
              IsOkAndHolds(SizeIs(1)));

  // 3 cases generated
  EXPECT_THAT(RunGenerateForTest(TwoTestTypeGenerator()),
              IsOkAndHolds(SizeIs(3)));

  // 4 cases generated
  EXPECT_THAT(RunGenerateForTest(TwoIntegerGeneratorWithRandomness()),
              IsOkAndHolds(SizeIs(4)));
}

TEST(GeneratorDeathTest,
     GenerationWithoutSettingGeneralConstraintsShouldCrash) {
  TwoIntegerGeneratorWithRandomness gen3;  // 4 cases generated
  GeneratorManager(&gen3).SetSeed({1, 2, 3});

  EXPECT_DEATH({ gen3.GenerateTestCases(); }, "without `VariableSet` injected");
}

TEST(GeneratorManagerTest, ClearCasesDeletesAllTestCases) {
  EmptyGenerator gen;
  moriarty_internal::GeneratorManager generator_manager(&gen);
  gen.AddTestCase();
  gen.AddTestCase();
  ASSERT_THAT(generator_manager.GetTestCases(), SizeIs(2));

  generator_manager.ClearCases();

  EXPECT_THAT(generator_manager.GetTestCases(), IsEmpty());
}

TEST(GeneratorManagerTest, ClearCasesDeletesAllOptionalTestCases) {
  EmptyGenerator gen;
  moriarty_internal::GeneratorManager generator_manager(&gen);
  gen.AddOptionalTestCase();
  gen.AddOptionalTestCase();
  gen.AddOptionalTestCase();
  ASSERT_THAT(generator_manager.GetOptionalTestCases(), SizeIs(3));

  generator_manager.ClearCases();

  EXPECT_THAT(generator_manager.GetOptionalTestCases(), IsEmpty());
}

TEST(GeneratorManagerTest, AssignValuesInAllTestCasesReturnsOnDefaultCase) {
  EmptyGenerator gen;
  moriarty_internal::GeneratorManager generator_manager(&gen);
  gen.AddTestCase().ConstrainVariable("X", MTestType());
  gen.AddTestCase().ConstrainVariable("Y", MInteger().Is(5));

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      std::vector<moriarty_internal::ValueSet> values,
      generator_manager.AssignValuesInAllTestCases());

  ASSERT_THAT(values, SizeIs(2));
  EXPECT_THAT(values[0].Get<MTestType>("X"),
              IsOkAndHolds(MTestType::kGeneratedValue));
  EXPECT_THAT(values[1].Get<MInteger>("Y"), IsOkAndHolds(5));
}

TEST(GeneratorManagerTest,
     AssignValuesInAllTestCasesIncludesOptionalTestCases) {
  EmptyGenerator gen;
  moriarty_internal::GeneratorManager generator_manager(&gen);
  gen.AddOptionalTestCase().ConstrainVariable("X", MTestType());
  gen.AddOptionalTestCase().ConstrainVariable("Y", MInteger().Is(5));

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      std::vector<moriarty_internal::ValueSet> values,
      generator_manager.AssignValuesInAllTestCases());

  ASSERT_THAT(values, SizeIs(2));
  EXPECT_THAT(values[0].Get<MTestType>("X"),
              IsOkAndHolds(MTestType::kGeneratedValue));
  EXPECT_THAT(values[1].Get<MInteger>("Y"), IsOkAndHolds(5));
}

TEST(GeneratorManagerTest,
     AssignValuesInAllTestCasesIgnoresInvalidOptionalTestCases) {
  EmptyGenerator gen;
  moriarty_internal::GeneratorManager generator_manager(&gen);
  gen.AddOptionalTestCase().ConstrainVariable(
      "Y", MInteger().Between(10, 5));  // Bad
  gen.AddOptionalTestCase().ConstrainVariable("X", MTestType());

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      std::vector<moriarty_internal::ValueSet> values,
      generator_manager.AssignValuesInAllTestCases());

  ASSERT_THAT(values, SizeIs(1));  // First case is invalid.
  EXPECT_THAT(values[0].Get<MTestType>("X"),
              IsOkAndHolds(MTestType::kGeneratedValue));
}

TEST(GeneratorManagerTest, AssignValuesInAllTestCasesAllowsRandomness) {
  EmptyGenerator gen;
  moriarty_internal::GeneratorManager generator_manager(&gen);
  gen.AddOptionalTestCase().ConstrainVariable("Y", MInteger().Between(50, 100));

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      std::vector<moriarty_internal::ValueSet> values,
      generator_manager.AssignValuesInAllTestCases());

  ASSERT_THAT(values, SizeIs(1));
  EXPECT_THAT(values[0].Get<MInteger>("Y"),
              IsOkAndHolds(AllOf(Ge(50), Le(100))));
}

TEST(GeneratorTest, ScenarioShouldApplyToAllFutureAddTestCaseCalls) {
  SimpleTestTypeGenerator generator;
  MORIARTY_ASSERT_OK(generator.TryWithScenario(Scenario().WithGeneralProperty(
      {.category = "size", .descriptor = "small"})));

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      std::vector<moriarty_internal::ValueSet> values,
      RunGenerateForTest(generator));
  ASSERT_THAT(values, SizeIs(1));

  EXPECT_THAT(values[0].Get<MTestType>("X"),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
  EXPECT_THAT(values[0].Get<MTestType>("Y"),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
}

class TestGenerator : public Generator {
 public:
  void GenerateTestCases() override {
    AddTestCase()
        .ConstrainVariable("X", MTestType())
        .ConstrainVariable("Y", MTestType());

    ABSL_CHECK_OK(TryWithScenario(Scenario().WithGeneralProperty(
        {.category = "size", .descriptor = "small"})));

    AddTestCase()
        .ConstrainVariable("X", MTestType())
        .ConstrainVariable("Y", MTestType());
  }
};

TEST(GeneratorTest, ScenarioShouldNotApplyToCallsToAddTestCasePreviously) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      std::vector<moriarty_internal::ValueSet> values,
      RunGenerateForTest(TestGenerator()));
  ASSERT_THAT(values, SizeIs(2));

  EXPECT_THAT(values[0].Get<MTestType>("X"),
              IsOkAndHolds(MTestType::kGeneratedValue));
  EXPECT_THAT(values[0].Get<MTestType>("Y"),
              IsOkAndHolds(MTestType::kGeneratedValue));

  EXPECT_THAT(values[1].Get<MTestType>("X"),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
  EXPECT_THAT(values[1].Get<MTestType>("Y"),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
}

// Simply seeds itself with a random engine and gives you access to all
// protected functions.
class ProtectedGenerator : public moriarty::Generator {
 public:
  ProtectedGenerator() { Seed(); }

  // If `should_seed` = `false`, constructs an unseeded Generator. (No
  // randomness available)
  explicit ProtectedGenerator(bool should_seed) {
    if (should_seed) {
      Seed();
    }
  }

  void Seed() {
    moriarty_internal::GeneratorManager(this).SetSeed({1, 2, 3});
    moriarty_internal::GeneratorManager(this).SetGeneralConstraints(vars_);
  }

  void GenerateTestCases() override {}  // Do nothing.

  // Promote protected functions in `Generator` to public for testing purposes.
  using Generator::GetVariable;
  using Generator::HugeValue;
  using Generator::LargeValue;
  using Generator::MaxValue;
  using Generator::MediumValue;
  using Generator::MinValue;
  using Generator::Random;
  using Generator::RandomInteger;
  using Generator::SatisfiesConstraints;
  using Generator::SizedValue;
  using Generator::SmallValue;
  using Generator::TinyValue;
  using Generator::TryGetVariable;
  using Generator::TryHugeValue;
  using Generator::TryLargeValue;
  using Generator::TryMaxValue;
  using Generator::TryMediumValue;
  using Generator::TryMinValue;
  using Generator::TryRandom;
  using Generator::TrySmallValue;
  using Generator::TryTinyValue;

 private:
  moriarty_internal::VariableSet vars_;
};

TEST(GeneratorTest, SatisfiesConstraintsWorksBasicCase) {
  MORIARTY_EXPECT_OK(
      ProtectedGenerator().SatisfiesConstraints(MInteger().Between(1, 10), 5));
  EXPECT_THAT(
      ProtectedGenerator().SatisfiesConstraints(MInteger().Between(1, 10), 15),
      IsUnsatisfiedConstraint("range"));
}

TEST(GeneratorTest, SatisfiesConstraintsUsesVariablesFromGeneralConstraints) {
  ProtectedGenerator G(/* should_seed = */ false);
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("N", MInteger().Is(10)));
  moriarty_internal::GeneratorManager(&G).SetGeneralConstraints(variables);
  MORIARTY_EXPECT_OK(G.SatisfiesConstraints(MInteger().Between(1, "N"), 5));
  EXPECT_THAT(G.SatisfiesConstraints(MInteger().Between(1, "N"), 11),
              IsUnsatisfiedConstraint("range"));
}

TEST(GeneratorTest, UnseededSatisfiesConstraintsShouldFail) {
  ProtectedGenerator G(/* should_seed = */ false);
  EXPECT_THAT(G.SatisfiesConstraints(MInteger().Between(1, 10), 5),
              IsMisconfigured(InternalConfigurationType::kVariableSet));
}

TEST(GeneratorDeathTest, RandomIntegerShouldCrashIfNoRngSet) {
  // (void) to ignore the [[nodiscard]] error.
  EXPECT_DEATH(
      {
        (void)ProtectedGenerator(/* should_seed = */ false).RandomInteger(1, 2);
      },
      "AddGenerator");
  EXPECT_DEATH(
      {
        (void)ProtectedGenerator(/* should_seed = */ false).RandomInteger(10);
      },
      "AddGenerator");
}

TEST(GeneratorDeathTest, RandomIntegerShouldCrashForInvalidRanges) {
  // (void) to ignore the [[nodiscard]] error.

  // Invalid RandomInteger(x, y)
  EXPECT_DEATH({ (void)ProtectedGenerator().RandomInteger(3, 1); }, "x > y");

  // Invalid RandomInteger(z)
  EXPECT_DEATH({ (void)ProtectedGenerator().RandomInteger(-1); }, "x > y");
  EXPECT_DEATH({ (void)ProtectedGenerator().RandomInteger(0); }, "x > y");
}

TEST(GeneratorTest, RandomIntegerShouldReturnValuesInRange) {
  EXPECT_THAT(ProtectedGenerator().RandomInteger(100, 200),
              AllOf(Ge(100), Le(200)));
  EXPECT_THAT(ProtectedGenerator().RandomInteger(-200, -150),
              AllOf(Ge(-200), Le(-150)));

  EXPECT_THAT(ProtectedGenerator().RandomInteger(50), AllOf(Ge(0), Le(49)));

  // Generate several values to ensure the boundaries are satisfied.
  for (int i = 0; i < 40; i++) {
    EXPECT_THAT(ProtectedGenerator().RandomInteger(5), AllOf(Ge(0), Le(4)));
    EXPECT_THAT(ProtectedGenerator().RandomInteger(50, 55),
                AllOf(Ge(50), Le(55)));
  }
}

TEST(GeneratorTest, RandomReturnsSuccessfullyRandomValues) {
  EXPECT_THAT(ProtectedGenerator().TryRandom(MInteger().Between(11, 22)),
              IsOkAndHolds(AllOf(Ge(11), Le(22))));
  EXPECT_THAT(ProtectedGenerator().Random(MInteger().Between(111, 122)),
              AllOf(Ge(111), Le(122)));
}

TEST(GeneratorTest, RandomFailsIfInvalidInputIsProvided) {
  EXPECT_THAT(ProtectedGenerator().TryRandom(MInteger().Between(-1, -2)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(GeneratorTest, GetVariableSuccessfullyReturnsKnownVariable) {
  ProtectedGenerator G(/* should_seed = */ false);
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("x", MInteger().Between(111, 122)));
  moriarty_internal::GeneratorManager(&G).SetGeneralConstraints(variables);

  MORIARTY_ASSERT_OK_AND_ASSIGN(MInteger x,
                                         G.TryGetVariable<MInteger>("x"));
  EXPECT_TRUE(GenerateSameValues(x, MInteger().Between(111, 122)));
}

TEST(GeneratorTest, GetVariableFailsIfNoVariablesKnown) {
  ProtectedGenerator G(/* should_seed = */ false);
  EXPECT_THAT(G.TryGetVariable<MInteger>("x"),
              IsMisconfigured(InternalConfigurationType::kVariableSet));
}

TEST(GeneratorTest, GetVariableFailsForUnknownVariableName) {
  ProtectedGenerator G(/* should_seed = */ false);
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("x", MInteger().Between(111, 122)));
  moriarty_internal::GeneratorManager(&G).SetGeneralConstraints(variables);

  EXPECT_THAT(G.TryGetVariable<MInteger>("u"),
              moriarty_testing::IsVariableNotFound("u"));
}

TEST(GeneratorTest, GetVariableFailsForWrongVariableType) {
  ProtectedGenerator G;
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("x", MInteger()));
  moriarty_internal::GeneratorManager(&G).SetGeneralConstraints(variables);

  EXPECT_THAT(G.TryGetVariable<MTestType>("x"),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Unable to convert")));
}

TEST(GeneratorTest, SizedValuesReturnsAppropriatelySizedValues) {
  // G1 and G2 share the same seed, so should generate the same sequence of
  // values. Only G1 is seeded with the variable.
  ProtectedGenerator G1;
  ProtectedGenerator G2;
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("x", MInteger().Between(123, 234)));
  moriarty_internal::GeneratorManager(&G1).SetGeneralConstraints(variables);

  EXPECT_THAT(G1.SizedValue<MInteger>("x", "min"),
              IsOkAndHolds(G2.Random(
                  MInteger().Between(123, 234).WithSize(CommonSize::kMin))));
  EXPECT_THAT(G1.SizedValue<MInteger>("x", "tiny"),
              IsOkAndHolds(G2.Random(
                  MInteger().Between(123, 234).WithSize(CommonSize::kTiny))));
  EXPECT_THAT(G1.SizedValue<MInteger>("x", "small"),
              IsOkAndHolds(G2.Random(
                  MInteger().Between(123, 234).WithSize(CommonSize::kSmall))));
  EXPECT_THAT(G1.SizedValue<MInteger>("x", "medium"),
              IsOkAndHolds(G2.Random(
                  MInteger().Between(123, 234).WithSize(CommonSize::kMedium))));
  EXPECT_THAT(G1.SizedValue<MInteger>("x", "large"),
              IsOkAndHolds(G2.Random(
                  MInteger().Between(123, 234).WithSize(CommonSize::kLarge))));
  EXPECT_THAT(G1.SizedValue<MInteger>("x", "huge"),
              IsOkAndHolds(G2.Random(
                  MInteger().Between(123, 234).WithSize(CommonSize::kHuge))));
  EXPECT_THAT(G1.SizedValue<MInteger>("x", "max"),
              IsOkAndHolds(G2.Random(
                  MInteger().Between(123, 234).WithSize(CommonSize::kMax))));

  EXPECT_THAT(G1.TryMinValue<MInteger>("x"),
              IsOkAndHolds(G2.Random(
                  MInteger().Between(123, 234).WithSize(CommonSize::kMin))));
  EXPECT_THAT(G1.TryTinyValue<MInteger>("x"),
              IsOkAndHolds(G2.Random(
                  MInteger().Between(123, 234).WithSize(CommonSize::kTiny))));
  EXPECT_THAT(G1.TrySmallValue<MInteger>("x"),
              IsOkAndHolds(G2.Random(
                  MInteger().Between(123, 234).WithSize(CommonSize::kSmall))));
  EXPECT_THAT(G1.TryMediumValue<MInteger>("x"),
              IsOkAndHolds(G2.Random(
                  MInteger().Between(123, 234).WithSize(CommonSize::kMedium))));
  EXPECT_THAT(G1.TryLargeValue<MInteger>("x"),
              IsOkAndHolds(G2.Random(
                  MInteger().Between(123, 234).WithSize(CommonSize::kLarge))));
  EXPECT_THAT(G1.TryHugeValue<MInteger>("x"),
              IsOkAndHolds(G2.Random(
                  MInteger().Between(123, 234).WithSize(CommonSize::kHuge))));
  EXPECT_THAT(G1.TryMaxValue<MInteger>("x"),
              IsOkAndHolds(G2.Random(
                  MInteger().Between(123, 234).WithSize(CommonSize::kMax))));

  EXPECT_THAT(
      G1.MinValue<MInteger>("x"),
      G2.Random(MInteger().Between(123, 234).WithSize(CommonSize::kMin)));
  EXPECT_THAT(
      G1.TinyValue<MInteger>("x"),
      G2.Random(MInteger().Between(123, 234).WithSize(CommonSize::kTiny)));
  EXPECT_THAT(
      G1.SmallValue<MInteger>("x"),
      G2.Random(MInteger().Between(123, 234).WithSize(CommonSize::kSmall)));
  EXPECT_THAT(
      G1.MediumValue<MInteger>("x"),
      G2.Random(MInteger().Between(123, 234).WithSize(CommonSize::kMedium)));
  EXPECT_THAT(
      G1.LargeValue<MInteger>("x"),
      G2.Random(MInteger().Between(123, 234).WithSize(CommonSize::kLarge)));
  EXPECT_THAT(
      G1.HugeValue<MInteger>("x"),
      G2.Random(MInteger().Between(123, 234).WithSize(CommonSize::kHuge)));
  EXPECT_THAT(
      G1.MaxValue<MInteger>("x"),
      G2.Random(MInteger().Between(123, 234).WithSize(CommonSize::kMax)));
}

TEST(GeneratorTest, SizedValuesFailsForUnknownSizes) {
  ProtectedGenerator G;
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("x", MInteger()));
  moriarty_internal::GeneratorManager(&G).SetGeneralConstraints(variables);

  EXPECT_THAT(
      G.SizedValue<MInteger>("x", "happy"),
      StatusIs(absl::StatusCode::kFailedPrecondition, HasSubstr("size")));
}

TEST(GeneratorTest, SizedValuesFailsForUnknownVariable) {
  // G1 will be seeded with variables, G2 will not be.
  ProtectedGenerator G1;
  ProtectedGenerator G2(/* should_seed = */ false);
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("x", MInteger()));
  moriarty_internal::GeneratorManager(&G1).SetGeneralConstraints(variables);

  EXPECT_THAT(G1.SizedValue<MInteger>("y", "min"), IsVariableNotFound("y"));

  EXPECT_THAT(G2.SizedValue<MInteger>("y", "min"),
              IsMisconfigured(InternalConfigurationType::kVariableSet));

  EXPECT_THAT(G1.TryMinValue<MInteger>("y"), IsVariableNotFound("y"));
  EXPECT_THAT(G1.TryTinyValue<MInteger>("y"), IsVariableNotFound("y"));
  EXPECT_THAT(G1.TrySmallValue<MInteger>("y"), IsVariableNotFound("y"));
  EXPECT_THAT(G1.TryMediumValue<MInteger>("y"), IsVariableNotFound("y"));
  EXPECT_THAT(G1.TryLargeValue<MInteger>("y"), IsVariableNotFound("y"));
  EXPECT_THAT(G1.TryHugeValue<MInteger>("y"), IsVariableNotFound("y"));
  EXPECT_THAT(G1.TryMaxValue<MInteger>("y"), IsVariableNotFound("y"));
}

TEST(GeneratorTest, AssignValuesInAllTetCasesWithoutRngFails) {
  EmptyGenerator G(/* should_seed =*/false);
  moriarty_internal::GeneratorManager manager(&G);
  manager.SetGeneralConstraints({});

  EXPECT_THAT(manager.AssignValuesInAllTestCases(),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
}

TEST(GeneratorTest, AssignValuesInAllTetCasesWithoutGeneralConstraintsFails) {
  EmptyGenerator G(/* should_seed =*/false);
  moriarty_internal::GeneratorManager manager(&G);
  manager.SetSeed({1, 2, 3});

  EXPECT_THAT(manager.AssignValuesInAllTestCases(),
              IsMisconfigured(InternalConfigurationType::kVariableSet));
}

TEST(GeneratorTest, GetGeneralConstraintsWithSetVarsHasValue) {
  EmptyGenerator gen;
  moriarty_internal::GeneratorManager generator_manager(&gen);
  moriarty_internal::VariableSet varset;
  MORIARTY_ASSERT_OK(varset.AddVariable("X", MInteger().Is(4)));
  MORIARTY_ASSERT_OK(varset.AddVariable("Y", MInteger().Is(5)));
  generator_manager.SetGeneralConstraints(varset);

  std::optional<const moriarty_internal::VariableSet> innerVarset =
      generator_manager.GetGeneralConstraints();
  MORIARTY_EXPECT_OK(innerVarset->GetVariable<MInteger>("X"));
  MORIARTY_EXPECT_OK(innerVarset->GetVariable<MInteger>("Y"));
}

}  // namespace
}  // namespace moriarty

// Tests for randomness:
namespace moriarty_testing {
namespace {

class ProtectedNonStatusRandomFunctions : public moriarty::Generator {
 public:
  ProtectedNonStatusRandomFunctions() {
    moriarty::GeneratorManager(this).SetSeed({1, 2, 3});
  }

  void GenerateTestCases() override {}  // Do nothing

  using Generator::DistinctIntegers;
  using Generator::RandomComposition;
  using Generator::RandomElement;
  using Generator::RandomElementsWithoutReplacement;
  using Generator::RandomElementsWithReplacement;
  using Generator::RandomInteger;
  using Generator::RandomPermutation;
  using Generator::Shuffle;
};

class ProtectedStatusRandomFunctions : public moriarty::Generator {
 public:
  explicit ProtectedStatusRandomFunctions(bool should_seed = true) {
    if (should_seed) moriarty::GeneratorManager(this).SetSeed({1, 2, 3});
  }

  void GenerateTestCases() override {}  // Do nothing

  absl::StatusOr<int64_t> RandomInteger(int64_t min, int64_t max) {
    return TryRandomInteger(min, max);
  }

  absl::StatusOr<int64_t> RandomInteger(int64_t n) {
    return TryRandomInteger(n);
  }

  template <typename T>
  absl::Status Shuffle(std::vector<T>& container) {
    return TryShuffle(container);
  }

  template <typename T>
  absl::StatusOr<T> RandomElement(const std::vector<T>& container) {
    return TryRandomElement(container);
  }

  template <typename T>
  absl::StatusOr<std::vector<T>> RandomElementsWithReplacement(
      const std::vector<T>& container, int k) {
    return TryRandomElementsWithReplacement(container, k);
  }

  template <typename T>
  absl::StatusOr<std::vector<T>> RandomElementsWithoutReplacement(
      const std::vector<T>& container, int k) {
    return TryRandomElementsWithoutReplacement(container, k);
  }

  absl::StatusOr<std::vector<int>> RandomPermutation(int n) {
    return TryRandomPermutation(n);
  }

  template <typename T>
    requires std::integral<T>
  absl::StatusOr<std::vector<T>> RandomPermutation(int n, T min) {
    return TryRandomPermutation(n, min);
  }

  template <typename T>
    requires std::integral<T>
  absl::StatusOr<std::vector<T>> DistinctIntegers(T n, int k, T min = 0) {
    return TryDistinctIntegers(n, k, min);
  }

  template <typename T>
    requires std::integral<T>
  absl::StatusOr<std::vector<T>> RandomComposition(T n, int k,
                                                   T min_bucket_size = 1) {
    return TryRandomComposition(n, k, min_bucket_size);
  }
};

INSTANTIATE_TYPED_TEST_SUITE_P(ProtectedStatusRandomFunctions,
                               ValidInputRandomnessTests,
                               ProtectedStatusRandomFunctions);
INSTANTIATE_TYPED_TEST_SUITE_P(ProtectedStatusRandomFunctions,
                               InvalidInputRandomnessTests,
                               ProtectedStatusRandomFunctions);
INSTANTIATE_TYPED_TEST_SUITE_P(
    ProtectedNonStatusRandomFunctions, ValidInputRandomnessTests,
    WrapRandomFuncsInStatus<ProtectedNonStatusRandomFunctions>);

TEST(GeneratorTest, RandomFunctionsWithoutRandomSeedShouldFail) {
  ProtectedStatusRandomFunctions unseeded(/*should_seed = */ false);
  std::vector<int> helper = {1, 2, 3};

  EXPECT_THAT(
      unseeded.DistinctIntegers(10, 2),
      IsMisconfigured(moriarty::InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(
      unseeded.RandomInteger(1, 10),
      IsMisconfigured(moriarty::InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(
      unseeded.RandomInteger(10),
      IsMisconfigured(moriarty::InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(
      unseeded.RandomComposition(10, 2),
      IsMisconfigured(moriarty::InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(
      unseeded.RandomElement(helper),
      IsMisconfigured(moriarty::InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(
      unseeded.RandomElementsWithoutReplacement(helper, 2),
      IsMisconfigured(moriarty::InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(
      unseeded.RandomElementsWithReplacement(helper, 2),
      IsMisconfigured(moriarty::InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(
      unseeded.RandomPermutation(2),
      IsMisconfigured(moriarty::InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(
      unseeded.RandomPermutation(2, 6),
      IsMisconfigured(moriarty::InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(
      unseeded.Shuffle(helper),
      IsMisconfigured(moriarty::InternalConfigurationType::kRandomEngine));
}

}  // namespace
}  // namespace moriarty_testing
