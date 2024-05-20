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

#include "src/librarian/mvariable.h"

#include <any>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "src/constraint_values.h"
#include "src/errors.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/generation_config.h"
#include "src/internal/random_engine.h"
#include "src/internal/universe.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/io_config.h"
#include "src/librarian/test_utils.h"
#include "src/property.h"
#include "src/testing/mtest_type.h"
#include "src/testing/random_test_util.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace librarian {
namespace {

using ::moriarty::moriarty_internal::MVariableManager;
using ::moriarty_testing::Context;
using ::moriarty_testing::Generate;
using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::GetUniqueValue;
using ::moriarty_testing::IsMisconfigured;
using ::moriarty_testing::IsNotSatisfiedWith;
using ::moriarty_testing::IsSatisfiedWith;
using ::moriarty_testing::MTestType;
using ::moriarty_testing::Print;
using ::moriarty_testing::Read;
using ::moriarty_testing::TestType;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::AnyWith;
using ::testing::Contains;
using ::testing::HasSubstr;
using ::testing::Optional;
using ::testing::SizeIs;
using ::testing::Truly;
using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;

TEST(MVariableTest, PrintShouldSucceed) {
  EXPECT_THAT(Print(MTestType(), -1), IsOkAndHolds("-1"));
  EXPECT_THAT(Print(MTestType(), 153), IsOkAndHolds("153"));
}

TEST(MVariableTest, GenerateShouldProduceAValue) {
  EXPECT_THAT(Generate(MTestType()), IsOkAndHolds(MTestType::kGeneratedValue));
}

TEST(MVariableTest, GenerateShouldObserveIs) {
  EXPECT_THAT(Generate(MTestType().Is(10)), IsOkAndHolds(10));
}

TEST(MVariableTest, GenerateShouldObserveIsOneOf) {
  EXPECT_THAT(GenerateN(MTestType().IsOneOf({50, 60, 70, 80, 90, 100}), 100),
              IsOkAndHolds(AllOf(Contains(50), Contains(60), Contains(70),
                                 Contains(80), Contains(90), Contains(100))));
}

TEST(MVariableTest, CannotGenerateAValueBeforeRngIsSet) {
  MTestType variable;
  EXPECT_THAT(moriarty_internal::MVariableManager(&variable).Generate(),
              IsMisconfigured(InternalConfigurationType::kUniverse));
}

TEST(MVariableTest, CannotCallSatisfiesConstraintsWithoutAUniverse) {
  MTestType variable;
  EXPECT_THAT(moriarty_internal::MVariableManager(&variable).IsSatisfiedWith(
                  MTestType::kGeneratedValue),
              IsMisconfigured(InternalConfigurationType::kUniverse));
}

TEST(MVariableTest, MergeFromShouldWork) {
  MTestType var1;
  MTestType var2;

  MVariableManager<MTestType, TestType> manager(&var1);

  EXPECT_NO_FATAL_FAILURE({ var1.MergeFrom(var2); });
  EXPECT_NO_FATAL_FAILURE({ var2.MergeFrom(var1); });
  EXPECT_NO_FATAL_FAILURE({ var1.MergeFrom(var1); });
  MORIARTY_EXPECT_OK(manager.MergeFrom(var1));
  EXPECT_TRUE(var1.WasMerged());
  MORIARTY_EXPECT_OK(manager.MergeFrom(var2));
  EXPECT_TRUE(var1.WasMerged());
}

TEST(MVariableTest, MergeFromShouldRespectOneOf) {
  MTestType var1 =
      MTestType().IsOneOf({TestType(11), TestType(22), TestType(33)});
  MTestType var2 =
      MTestType().IsOneOf({TestType(22), TestType(33), TestType(44)});

  ASSERT_THAT(var1, GeneratedValuesAre(
                        AnyOf(TestType(11), TestType(22), TestType(33))));

  var1.MergeFrom(var2);
  EXPECT_THAT(var1, GeneratedValuesAre(AnyOf(TestType(22), TestType(33))));
}

TEST(MVariableTest, MergeFromWithWrongTypeShouldFail) {
  MTestType var1;
  moriarty::MInteger var2;

  MVariableManager<MTestType, TestType> manager(&var1);

  EXPECT_THAT(manager.MergeFrom(var2),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("MergeFrom: Cannot convert")));
  EXPECT_FALSE(var1.WasMerged());
}

TEST(MVariableTest, MergeFromUsingAbstractVariablesShouldRespectIsAndIsOneOf) {
  auto merge_from = [](MTestType& a, const MTestType& b) {
    ABSL_CHECK_OK(
        static_cast<moriarty_internal::AbstractVariable*>(&a)->MergeFrom(b));
  };

  {
    MTestType var1 = MTestType().Is(10);
    MTestType var2;

    merge_from(var1, var2);
    EXPECT_THAT(Generate(var1), IsOkAndHolds(10));
    EXPECT_TRUE(var1.WasMerged());
  }

  {
    MTestType var1;
    MTestType var2 = MTestType().Is(20);

    merge_from(var1, var2);
    EXPECT_THAT(Generate(var1), IsOkAndHolds(20));
    EXPECT_TRUE(var1.WasMerged());
  }

  {
    MTestType var1 = MTestType().Is(10);
    MTestType var2 = MTestType().Is(20);

    merge_from(var1, var2);
    EXPECT_THAT(
        Generate(var1),
        StatusIs(absl::StatusCode::kFailedPrecondition,
                 HasSubstr("Is/IsOneOf used, but no viable value found")));
    EXPECT_TRUE(var1.WasMerged());  // Was merged, but bad options.
  }

  {
    MTestType var1 = MTestType().Is(10);
    MTestType var2 = MTestType().IsOneOf({40, 30, 20, 10});

    merge_from(var1, var2);
    EXPECT_THAT(Generate(var1), IsOkAndHolds(10));
    EXPECT_TRUE(var1.WasMerged());
  }

  {
    MTestType var1 = MTestType().IsOneOf({300, 17, 10, -1234});
    MTestType var2 = MTestType().IsOneOf({40, 30, 20, 10});

    merge_from(var1, var2);
    EXPECT_THAT(Generate(var1), IsOkAndHolds(10));
    EXPECT_TRUE(var1.WasMerged());
  }
}

TEST(MVariableTest, SubvariablesShouldBeSetableAndUseable) {
  MTestType var = MTestType().SetMultiplier(MInteger().Between(2, 2));

  EXPECT_THAT(Generate(var), IsOkAndHolds(2 * MTestType::kGeneratedValue));
}

// TODO(darcybest): Add test cases that cover multiple layers of subvalues. I.e.
// if A is an array of strings then we should be able to ask for A.2.length
TEST(MVariableTest, SubvaluesAreRetrievable) {
  MTestType M = MTestType();
  EXPECT_THAT(MVariableManager(&M).GetSubvalue(
                  TestType(2 * MTestType::kGeneratedValue), "multiplier"),
              IsOkAndHolds(AnyWith<int64_t>(2)));
}

TEST(MVariableTest, SubvaluesThatDontExistFail) {
  MTestType M = MTestType();
  EXPECT_THAT(MVariableManager(&M).GetSubvalue(
                  TestType(2 * MTestType::kGeneratedValue), "hat"),
              StatusIs(absl::StatusCode::kNotFound));
}

TEST(MVariableTest, GeneratingDependentVariableWithNoUniverseShouldFail) {
  moriarty_internal::VariableSet variables;
  MORIARTY_EXPECT_OK(
      variables.AddVariable("var1", MTestType().SetAdder("var2")));
  MORIARTY_EXPECT_OK(variables.AddVariable("var2", MTestType()));

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty_internal::AbstractVariable * var,
      variables.GetAbstractVariable("var1"));
  MVariableManager<MTestType, TestType> manager1(dynamic_cast<MTestType*>(var));

  EXPECT_THAT(var->AssignValue(),
              IsMisconfigured(InternalConfigurationType::kUniverse));
}

TEST(MVariableTest, BasingMyVariableOnAnotherSetValueShouldWorkBasicCase) {
  EXPECT_THAT(Generate(MTestType().SetAdder("x"),
                       Context().WithValue<MTestType>("x", TestType(20))),
              IsOkAndHolds(20 + MTestType::kGeneratedValue));
}

TEST(MVariableTest, BasingMyVariableOnAnotherUnSetVariableShouldWorkBasicCase) {
  EXPECT_THAT(Generate(MTestType().SetAdder("x"),
                       Context().WithVariable("x", MTestType())),
              IsOkAndHolds(2 * MTestType::kGeneratedValue));
}

TEST(MVariableTest, BasingMyVariableOnANonexistentOneShouldFail) {
  EXPECT_THAT(Generate(MTestType().SetAdder("x")),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Unknown variable: `x`")));
}

TEST(MVariableTest, DependentVariableOfTheWrongTypeShouldFail) {
  EXPECT_THAT(Generate(MTestType().SetAdder("x"),
                       Context().WithVariable("x", MInteger())),  // Wrong type
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Unable to cast x")));
}

TEST(MVariableTest, DependentVariablesInSubvariablesCanChain) {
  Context c = Context()
                  .WithVariable("x", MTestType().SetAdder("y"))
                  .WithVariable("y", MTestType().SetAdder("z"))
                  .WithValue<MTestType>("z", TestType(30));

  // Adds x (GeneratedValue), y (GeneratedValue), and z (30) to itself
  // (GeneratedValue).
  EXPECT_THAT(Generate(MTestType().SetAdder("x"), c),
              IsOkAndHolds(30 + 3 * MTestType::kGeneratedValue));
}

TEST(MVariableTest, SeparateCallsToGetShouldUseTheSameDependentVariableValue) {
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MInteger().Between("N", "N")));
  MORIARTY_ASSERT_OK(variables.AddVariable("B", MInteger().Between("N", "N")));
  MORIARTY_ASSERT_OK(
      variables.AddVariable("N", MInteger().Between(1, 1000000000)));

  moriarty_internal::ValueSet values;

  moriarty_internal::GenerationConfig generation_config;
  moriarty_internal::RandomEngine engine({1, 2, 3}, "v0.1");

  moriarty_internal::Universe universe =
      moriarty_internal::Universe()
          .SetMutableVariableSet(&variables)
          .SetMutableValueSet(&values)
          .SetGenerationConfig(&generation_config)
          .SetRandomEngine(&engine);
  variables.SetUniverse(&universe);

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty_internal::AbstractVariable * var_A,
      universe.GetAbstractVariable("A"));
  MORIARTY_ASSERT_OK(var_A->AssignValue());  // By assigning A, we assigned N.

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty_internal::AbstractVariable * var_B,
      universe.GetAbstractVariable("B"));
  MORIARTY_ASSERT_OK(
      var_B->AssignValue());  // Should use the already generated N.
  MORIARTY_ASSERT_OK_AND_ASSIGN(int N,
                                         universe.GetValue<MInteger>("N"));

  EXPECT_THAT(universe.GetValue<MInteger>("A"), IsOkAndHolds(N));
  EXPECT_THAT(universe.GetValue<MInteger>("B"), IsOkAndHolds(N));
}

TEST(MVariableTest, CyclicDependenciesShouldFail) {
  EXPECT_THAT(Generate(MTestType().SetAdder("y"),
                       Context()
                           .WithVariable("y", MTestType().SetAdder("z"))
                           .WithVariable("z", MTestType().SetAdder("y"))),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("cyclic dependency")));

  // Self-loop
  EXPECT_THAT(Generate(MTestType().SetAdder("y"),
                       Context().WithVariable("y", MTestType().SetAdder("y"))),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("cyclic dependency")));
}

TEST(MVariableTest, SatisfiesConstraintsWorksForValid) {
  // In the simplist case, everything will work since it is checking it is a
  // multiple of 1.
  EXPECT_THAT(MTestType(), IsSatisfiedWith(5));
  EXPECT_THAT(MTestType(), IsSatisfiedWith(MTestType::kGeneratedValue));

  // Now set the multiplier. The number must be a multiple of something in the
  // range.
  EXPECT_THAT(MTestType().SetMultiplier(MInteger().Between(5, 5)),
              IsSatisfiedWith(100));
  EXPECT_THAT(MTestType().SetMultiplier(MInteger().Between(3, 7)),
              IsSatisfiedWith(100));  // Multiple of both 4 and 5

  // 101 is prime, so not a multiple of 3,4,5,6,7.
  EXPECT_THAT(
      MTestType().SetMultiplier(MInteger().Between(3, 7)),
      IsNotSatisfiedWith(101, "not a multiple of any valid multiplier"));
}

TEST(MVariableTest, SatisfiesConstraintsNeedsDependentValues) {
  // No value known
  EXPECT_THAT(MTestType().SetAdder("t").SetMultiplier(MInteger().Between(3, 3)),
              IsNotSatisfiedWith(1, "Unknown variable"));
  // Variable, but no value known.
  EXPECT_THAT(MTestType().SetAdder("t").SetMultiplier(MInteger().Between(3, 3)),
              IsNotSatisfiedWith(1, "Value for `t` not found",
                                 Context().WithVariable("t", MTestType())));
}

TEST(MVariableTest, SatisfiesConstraintsWorksWithDependentValues) {
  // x = 107 + 3 * [something].
  //  So, 110 = 107 + 3 * 1 is valid, and 111 is not.
  EXPECT_THAT(
      MTestType().SetAdder("t").SetMultiplier(MInteger().Between(3, 3)),
      IsSatisfiedWith(110, Context().WithValue<MTestType>("t", TestType(107))));
  EXPECT_THAT(
      MTestType().SetAdder("t").SetMultiplier(MInteger().Between(3, 3)),
      IsNotSatisfiedWith(111, "not a multiple of any valid multiplier",
                         Context().WithValue<MTestType>("t", TestType(107))));
}

TEST(MVariableTest, SatisfiesConstraintsCanValidateSubvariablesIfNeeded) {
  // MInteger().Between(5, 0) is an invalid range. Should fail validation.
  EXPECT_THAT(MTestType().SetMultiplier(MInteger().Between(5, 0)),
              IsNotSatisfiedWith(1, "not a multiple of any valid multiplier"));
}

TEST(MVariableTest, SatisfiesConstraintsShouldAcknowledgeIsAndIsOneOf) {
  // In the simplist case, everything will work since it is checking it is a
  // multiple of 1.
  EXPECT_THAT(MTestType().IsOneOf({2, 3, 5, 7}), IsSatisfiedWith(5));
  EXPECT_THAT(MTestType().IsOneOf({2, 4, 8}), IsNotSatisfiedWith(5, "IsOneOf"));

  // 8 is in the list and a multiple of 4.
  EXPECT_THAT(
      MTestType().IsOneOf({2, 4, 8}).SetMultiplier(MInteger().Between(4, 4)),
      IsSatisfiedWith(8));
}

TEST(MVariableTest,
     SatisfiesConstraintsShouldFailIfIsOneOfSucceedsButLowerFails) {
  // 2 is in the one-of list, but not a multiple of 4.
  EXPECT_THAT(
      MTestType().IsOneOf({2, 4, 8}).SetMultiplier(MInteger().Between(4, 4)),
      IsNotSatisfiedWith(2, "not a multiple of any valid multiplier"));
}

TEST(MVariableTest, SatisfiesConstraintsShouldCheckCustomConstraints) {
  auto is_small_prime = [](const TestType& value) -> bool {
    if (value < 2) return false;
    for (int64_t p = 2; p * p <= value; p++)
      if (value % p == 0) return false;
    return true;
  };

  MTestType var = MTestType().AddCustomConstraint("Prime", is_small_prime);

  EXPECT_THAT(var, IsSatisfiedWith(2));
  EXPECT_THAT(var, IsSatisfiedWith(3));
  EXPECT_THAT(var, IsNotSatisfiedWith(4, "Prime"));
}

TEST(MVariableTest, SatisfiesConstraintsShouldCheckMultipleCustomConstraints) {
  MTestType var =
      MTestType()
          .AddCustomConstraint("Prime",
                               [](const TestType& value) -> bool {
                                 if (value < 2) return false;
                                 for (int64_t p = 2; p * p <= value; p++)
                                   if (value % p == 0) return false;
                                 return true;
                               })
          .AddCustomConstraint("3 mod 4", [](const TestType& value) -> bool {
            return value % 4 == 3;
          });

  // Yes prime, yes 3 mod 4.
  EXPECT_THAT(var, IsSatisfiedWith(7));

  // Yes prime, not 3 mod 4.
  EXPECT_THAT(var, IsNotSatisfiedWith(2, "3 mod 4"));

  // Not prime, yes 3 mod 4.
  EXPECT_THAT(var, IsNotSatisfiedWith(15, "Prime"));

  // Not prime, not 3 mod 4.
  EXPECT_THAT(var, IsNotSatisfiedWith(6, "Prime"));
}

TEST(MVariableTest, CustomConstraintWithDependentVariables) {
  EXPECT_THAT(
      Generate(MTestType()
                   .SetMultiplier(MInteger().Is(2))
                   .AddCustomConstraint(
                       "Custom1", {"A"},
                       [](const TestType value,
                          const ConstraintValues& known_values) -> bool {
                         return known_values.GetValue<MTestType>("A").value ==
                                value;
                       }),
               Context().WithVariable(
                   "A", MTestType().SetMultiplier(MInteger().Is(2)))),
      IsOkAndHolds(MTestType::kGeneratedValue * 2));
}

TEST(MVariableTest, CustomConstraintInvalidFailsToGenerate) {
  EXPECT_THAT(
      Generate(MTestType()
                   .SetMultiplier(MInteger().Is(1))
                   .AddCustomConstraint(
                       "Custom1", {"A"},
                       [](const TestType value,
                          const ConstraintValues& known_values) -> bool {
                         return known_values.GetValue<MTestType>("A").value ==
                                value;
                       }),
               Context().WithVariable(
                   "A", MTestType().SetMultiplier(MInteger().Is(2)))),
      StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(MVariableDeathTest, CustomConstraintsWithInvalidDependentVariableCrashes) {
  EXPECT_DEATH(
      auto status = Generate(
          MTestType()
              .SetMultiplier(MInteger().Is(1))
              .AddCustomConstraint(
                  "Custom1", {},
                  [](const TestType value,
                     const ConstraintValues& known_values) -> bool {
                    return known_values.GetValue<MTestType>("A").value == value;
                  })),
      "NOT_FOUND: Unknown variable");
}

TEST(MVariableDeathTest,
     CustomConstraintsKnownVariableButNotAddedAsDependencyFailsGeneration) {
  // Note that "L"'s value is not known since the generated variable's name is
  // "Generate(MTestType)" -- which comes before "L" alphabetically. If the
  // dependent's name was "A" instead of "L", this would pass, but it is not
  // guaranteed to since the user didn't specify it as a constraint.
  EXPECT_DEATH(
      auto status = Generate(
          MTestType()
              .SetMultiplier(MInteger().Is(1))
              .AddCustomConstraint(
                  "Custom1", {},
                  [](const TestType value,
                     const ConstraintValues& known_values) -> bool {
                    return known_values.GetValue<MTestType>("L").value == value;
                  }),
          Context().WithVariable("L",
                                 MTestType().SetMultiplier(MInteger().Is(2)))),
      "NOT_FOUND: Value");
}

// TODO(hivini): Test that by default, nothing is returned from
// GetDifficultInstances.

TEST(MVariableTest, GetDifficultInstancesPassesTheInstancesThrough) {
  EXPECT_THAT(MTestType().GetDifficultInstances(), IsOkAndHolds(SizeIs(2)));
}

TEST(MVariableTest, GetDifficultInstanceReturnsMergedInstance) {
  MTestType original = MTestType();
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      std::vector<MTestType> difficult_instances,
      original.GetDifficultInstances());

  EXPECT_THAT(difficult_instances, SizeIs(2));
  EXPECT_TRUE(difficult_instances[0].WasMerged());
  EXPECT_TRUE(difficult_instances[1].WasMerged());
  EXPECT_THAT(Generate(difficult_instances[0]), IsOkAndHolds(2));
  EXPECT_THAT(Generate(difficult_instances[1]), IsOkAndHolds(3));
}

class EmptyClass {
 public:
  bool operator==(const EmptyClass& other) const { return true; }
  bool operator<(const EmptyClass& other) const { return false; }
};
class MEmptyClass : public MVariable<MEmptyClass, EmptyClass> {
 public:
  std::string Typename() const override { return "MEmptyClass"; }

 private:
  absl::StatusOr<EmptyClass> GenerateImpl() override {
    return absl::UnimplementedError("GenerateImpl");
  }
  absl::Status IsSatisfiedWithImpl(const EmptyClass& c) const override {
    return absl::UnimplementedError("IsSatisfiedWith");
  }
  absl::Status MergeFromImpl(const MEmptyClass& c) override {
    return absl::UnimplementedError("MergeFromImpl");
  }
};

TEST(MVariableTest, ToStringByDefaultPrintsTypename) {
  EXPECT_THAT(MEmptyClass().ToString(), HasSubstr("MEmptyClass"));
}

TEST(MVariableTest, ToStringShouldIncludeOneOfOptions) {
  EXPECT_THAT(MEmptyClass().Is(EmptyClass()).ToString(), HasSubstr("Is"));
  EXPECT_THAT(MEmptyClass().IsOneOf({EmptyClass(), EmptyClass()}).ToString(),
              HasSubstr("Is"));
}

TEST(MVariableTest, MVariableShouldByDefaultNotBeAbleToRead) {
  EXPECT_THAT(Read(MEmptyClass(), "1234"),
              StatusIs(absl::StatusCode::kUnimplemented));
}

TEST(MVariableTest, MVariableShouldByDefaultNotBeAbleToPrint) {
  EXPECT_THAT(Print(MEmptyClass(), EmptyClass()),
              StatusIs(absl::StatusCode::kUnimplemented));
}

TEST(MVariableTest, ReadValueShouldBeSuccessfulInNormalState) {
  EXPECT_THAT(Read(MTestType(), "1234"), IsOkAndHolds(1234));
}

TEST(MVariableTest, FailedReadShouldFail) {
  EXPECT_THAT(Read(MTestType(), "bad"),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("failed to read")));
}

TEST(MVariableTest, CannotReadValueWhenThereIsAConstValueSet) {
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("x", MTestType()));

  moriarty_internal::ValueSet values;

  std::stringstream ss("12345");
  librarian::IOConfig io_config = librarian::IOConfig().SetInputStream(ss);

  moriarty_internal::Universe universe = moriarty_internal::Universe()
                                             .SetMutableVariableSet(&variables)
                                             .SetConstValueSet(&values)
                                             .SetIOConfig(&io_config);
  variables.SetUniverse(&universe);

  EXPECT_THAT((*universe.GetAbstractVariable("x"))->ReadValue(),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("without a mutable ValueSet")));
}

TEST(MVariableTest, PrintValueShouldBeSuccessfulInNormalState) {
  EXPECT_THAT(Print(MTestType(), TestType(1234)), IsOkAndHolds("1234"));
}

TEST(MVariableTest, PrintValueAndReadValueWithoutAUniverseShouldFail) {
  moriarty_internal::VariableSet variables;
  MORIARTY_EXPECT_OK(variables.AddVariable("x", MTestType()));
  moriarty_internal::ValueSet values;
  moriarty_internal::Universe universe = moriarty_internal::Universe()
                                             .SetMutableVariableSet(&variables)
                                             .SetConstValueSet(&values);

  {  // No Universe...
    EXPECT_THAT((*variables.GetAbstractVariable("x"))->ReadValue(),
                IsMisconfigured(InternalConfigurationType::kUniverse));
    EXPECT_THAT((*variables.GetAbstractVariable("x"))->PrintValue(),
                IsMisconfigured(InternalConfigurationType::kUniverse));
  }
}

class ProtectedIOMethodsInMVariable : public MTestType {
 public:  // Promote protected methods to public for testing
  using MTestType::Print;
  using MTestType::Read;
};

TEST(MVariableTest, ReadWithNoUniverseShouldFail) {
  EXPECT_THAT(ProtectedIOMethodsInMVariable().Read("var_name", MInteger()),
              IsMisconfigured(InternalConfigurationType::kUniverse));
}

TEST(MVariableTest, ReadWithNoIOConfigShouldFail) {
  ProtectedIOMethodsInMVariable var;
  moriarty_internal::Universe universe;  // Has no I/O config
  moriarty_internal::MVariableManager(&var).SetUniverse(&universe, "var_name");
  EXPECT_THAT(var.Read("var_name", MInteger()),
              IsMisconfigured(InternalConfigurationType::kInputStream));
}

TEST(MVariableTest, ReadWithNoInputStreamShouldFail) {
  ProtectedIOMethodsInMVariable var;
  librarian::IOConfig io_config;  // No InputStream
  moriarty_internal::Universe universe =
      moriarty_internal::Universe().SetIOConfig(&io_config);
  moriarty_internal::MVariableManager(&var).SetUniverse(&universe, "var_name");
  EXPECT_THAT(var.Read("var_name", MInteger()),
              IsMisconfigured(InternalConfigurationType::kInputStream));
}

TEST(MVariableTest, ReadShouldUseTheBaseMVariablesInputStream) {
  ProtectedIOMethodsInMVariable var1;
  std::stringstream ss1("123");  // Should read from here
  librarian::IOConfig io_config1 = librarian::IOConfig().SetInputStream(ss1);
  moriarty_internal::Universe universe1 =
      moriarty_internal::Universe().SetIOConfig(&io_config1);
  moriarty_internal::MVariableManager(&var1).SetUniverse(&universe1,
                                                         "var_name");

  ProtectedIOMethodsInMVariable var2;
  std::stringstream ss2("789");
  librarian::IOConfig io_config2 = librarian::IOConfig().SetInputStream(ss2);
  moriarty_internal::Universe universe2 =
      moriarty_internal::Universe().SetIOConfig(&io_config2);
  moriarty_internal::MVariableManager(&var2).SetUniverse(&universe2,
                                                         "var_name");

  EXPECT_THAT(var1.Read<MTestType>("var_name", var2), IsOkAndHolds(123));
}

TEST(MVariableTest, PrintWithNoUniverseShouldFail) {
  EXPECT_THAT(
      ProtectedIOMethodsInMVariable().Print("var_name", MInteger(), 123),
      IsMisconfigured(InternalConfigurationType::kUniverse));
}

TEST(MVariableTest, PrintWithNoIOConfigShouldFail) {
  ProtectedIOMethodsInMVariable var;
  moriarty_internal::Universe universe;  // Has no I/O config
  moriarty_internal::MVariableManager(&var).SetUniverse(&universe, "var_name");
  EXPECT_THAT(var.Print("var_name", MInteger(), 123),
              IsMisconfigured(InternalConfigurationType::kOutputStream));
}

TEST(MVariableTest, PrintWithNoOutputStreamShouldFail) {
  ProtectedIOMethodsInMVariable var;
  librarian::IOConfig io_config;  // No OutputStream
  moriarty_internal::Universe universe =
      moriarty_internal::Universe().SetIOConfig(&io_config);
  moriarty_internal::MVariableManager(&var).SetUniverse(&universe, "var_name");
  EXPECT_THAT(var.Print("var_name", MInteger(), 123),
              IsMisconfigured(InternalConfigurationType::kOutputStream));
}

TEST(MVariableTest, PrintShouldUseTheBaseMVariablesOutputStream) {
  ProtectedIOMethodsInMVariable var1;
  std::stringstream ss1;  // Should print to here
  librarian::IOConfig io_config1 = librarian::IOConfig().SetOutputStream(ss1);
  moriarty_internal::Universe universe1 =
      moriarty_internal::Universe().SetIOConfig(&io_config1);
  moriarty_internal::MVariableManager(&var1).SetUniverse(&universe1,
                                                         "var1_name");

  ProtectedIOMethodsInMVariable var2;
  std::stringstream ss2;
  librarian::IOConfig io_config2 = librarian::IOConfig().SetOutputStream(ss2);
  moriarty_internal::Universe universe2 =
      moriarty_internal::Universe().SetIOConfig(&io_config2);
  moriarty_internal::MVariableManager(&var2).SetUniverse(&universe2,
                                                         "var2_name");
  MORIARTY_EXPECT_OK(var1.Print<MTestType>("var2_name", var2, 123));
  EXPECT_EQ(ss1.str(), "123");
  EXPECT_EQ(ss2.str(), "");
}

TEST(MVariableTest, PrintValueShouldPrintTheAssignedValue) {
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("x", MTestType()));

  moriarty_internal::ValueSet values;
  values.Set<MTestType>("x", TestType(12345));

  std::stringstream ss;
  librarian::IOConfig io_config = librarian::IOConfig().SetOutputStream(ss);

  moriarty_internal::Universe universe = moriarty_internal::Universe()
                                             .SetMutableVariableSet(&variables)
                                             .SetConstValueSet(&values)
                                             .SetIOConfig(&io_config);
  variables.SetUniverse(&universe);
  MORIARTY_EXPECT_OK((*universe.GetAbstractVariable("x"))->PrintValue());
  EXPECT_EQ(ss.str(), "12345");
}

TEST(MVariableTest, IOConfigShouldBeAvailable) {
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("x", MTestType()));

  moriarty_internal::ValueSet values;
  values.Set<MTestType>("x", TestType(12345));

  std::stringstream ss;
  librarian::IOConfig io_config = librarian::IOConfig().SetOutputStream(ss);

  moriarty_internal::Universe universe = moriarty_internal::Universe()
                                             .SetMutableVariableSet(&variables)
                                             .SetConstValueSet(&values)
                                             .SetIOConfig(&io_config);
  variables.SetUniverse(&universe);

  // Check 1
  io_config.SetWhitespacePolicy(librarian::IOConfig::WhitespacePolicy::kExact);
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty_internal::AbstractVariable * var,
      universe.GetAbstractVariable("x"));
  EXPECT_EQ(dynamic_cast<MTestType*>(var)->GetWhitespacePolicy(),
            IOConfig::WhitespacePolicy::kExact);

  // Check 2
  io_config.SetWhitespacePolicy(
      librarian::IOConfig::WhitespacePolicy::kIgnoreWhitespace);
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty_internal::AbstractVariable * var2,
      universe.GetAbstractVariable("x"));
  EXPECT_EQ(dynamic_cast<MTestType*>(var2)->GetWhitespacePolicy(),
            IOConfig::WhitespacePolicy::kIgnoreWhitespace);
}

TEST(MVariableTest, GetUniqueValueShouldReturnUniqueValueViaIsMethod) {
  EXPECT_THAT(GetUniqueValue(MInteger().Is(123)), Optional(123));
  EXPECT_THAT(GetUniqueValue(MTestType().Is(2 * MTestType::kGeneratedValue)),
              Optional(2 * MTestType::kGeneratedValue));
}

TEST(MVariableTest, GetUniqueValueShouldReturnNulloptByDefault) {
  EXPECT_EQ(GetUniqueValue(MInteger()), std::nullopt);
  EXPECT_EQ(GetUniqueValue(MTestType()), std::nullopt);
}

TEST(MVariableTest, GetUniqueValueWithMultipleOptionsShouldReturnNullopt) {
  EXPECT_EQ(GetUniqueValue(MInteger().IsOneOf({123, 456})), std::nullopt);
  EXPECT_EQ(
      GetUniqueValue(MTestType().IsOneOf(
          {MTestType::kGeneratedValue, MTestType::kGeneratedValueSmallSize})),
      std::nullopt);
}

TEST(MVariableTest, GenerateShouldRetryIfNeeded) {
  // 1/7 numbers should be 3 mod 7, and 1/2 number should have 3rd digit even.
  // A single random guess should work 1/14 times. We'll generate 100. If
  // retries aren't there, this will fail frequently.
  EXPECT_THAT(
      MInteger()
          .Between(0, 999)
          .AddCustomConstraint("3 mod 7", [](int x) { return x % 7 == 3; })
          .AddCustomConstraint("3rd digit is even.",
                               [](int x) { return (x / 100) % 2 == 0; }),
      GeneratedValuesAre(
          AllOf(Truly([](int x) { return x % 7 == 3; }),
                Truly([](int x) { return (x / 100) % 2 == 0; }))));
}

TEST(MVariableTest, AssignValueShouldNotOverwriteAlreadySetValue) {
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(
      variables.AddVariable("N", MInteger().Between(1, 1000000000)));
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MInteger().Between(1, "N")));

  moriarty_internal::ValueSet values;

  moriarty_internal::GenerationConfig generation_config;
  moriarty_internal::RandomEngine engine({1, 2, 3}, "v0.1");

  moriarty_internal::Universe universe =
      moriarty_internal::Universe()
          .SetMutableVariableSet(&variables)
          .SetMutableValueSet(&values)
          .SetGenerationConfig(&generation_config)
          .SetRandomEngine(&engine);
  variables.SetUniverse(&universe);

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty_internal::AbstractVariable * var_A,
      universe.GetAbstractVariable("A"));
  MORIARTY_ASSERT_OK(var_A->AssignValue());  // By assigning A, we assigned N.
  MORIARTY_ASSERT_OK_AND_ASSIGN(int N,
                                         universe.GetValue<MInteger>("N"));

  // Attempt to re-assign N.
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty_internal::AbstractVariable * var_N,
      universe.GetAbstractVariable("N"));
  MORIARTY_ASSERT_OK(var_N->AssignValue());

  // Should not have changed.
  EXPECT_THAT(universe.GetValue<MInteger>("N"), IsOkAndHolds(N));
}

TEST(MVariableTest, AssignValueWithoutAUniverseShouldFail) {
  moriarty_internal::VariableSet variables;
  MORIARTY_EXPECT_OK(variables.AddVariable("x", MTestType()));
  moriarty_internal::ValueSet values;
  moriarty_internal::Universe universe = moriarty_internal::Universe()
                                             .SetMutableVariableSet(&variables)
                                             .SetConstValueSet(&values);

  {  // No Universe...
    EXPECT_THAT((*variables.GetAbstractVariable("x"))->AssignValue(),
                IsMisconfigured(InternalConfigurationType::kUniverse));
  }
}

TEST(MVariableTest, KnownPropertyCallsCorrectCallbackOnCopiedVariable) {
  MTestType a;
  MTestType b = a;
  b.WithKnownProperty({.category = "size",
                       .descriptor = "small",
                       .enforcement = Property::kFailIfUnknown});

  EXPECT_THAT(Generate(a), IsOkAndHolds(MTestType::kGeneratedValue));
  EXPECT_THAT(Generate(b), IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
}

TEST(MVariableTest, KnownPropertyWorksInSimpleCase) {
  EXPECT_THAT(Generate(MTestType().WithKnownProperty(
                  {.category = "size",
                   .descriptor = "small",
                   .enforcement = Property::kFailIfUnknown})),
              IsOkAndHolds(MTestType::kGeneratedValueSmallSize));
  EXPECT_THAT(Generate(MTestType().WithKnownProperty(
                  {.category = "size",
                   .descriptor = "large",
                   .enforcement = Property::kFailIfUnknown})),
              IsOkAndHolds(MTestType::kGeneratedValueLargeSize));
}

TEST(MVariableTest, KnownPropertyWithAnUnknownOptionalPropertyIsIgnored) {
  EXPECT_THAT(Generate(MTestType().WithKnownProperty(
                  {.category = "unknown",
                   .descriptor = "small",
                   .enforcement = Property::kIgnoreIfUnknown})),
              IsOkAndHolds(MTestType::kGeneratedValue));
}

TEST(MVariableDeathTest,
     KnownPropertyWithAnUnknownMandatoryPropertyShouldCrash) {
  EXPECT_DEATH(
      {
        MTestType().WithKnownProperty(
            {.category = "mystery",
             .descriptor = "small",
             .enforcement = Property::kFailIfUnknown});
      },
      "mystery");
}

TEST(MVariableDeathTest,
     PropertyWithAnKnownCategoryButUnknownDescriptorShouldCrashIfRequested) {
  EXPECT_DEATH(
      {
        MTestType().WithKnownProperty(
            {.category = "size",
             .descriptor = "hahaha",
             .enforcement = Property::kFailIfUnknown});
      },
      "hahaha");
}

TEST(MVariableTest,
     PropertyWithAnKnownCategoryButUnknownDescriptorShouldIgnoreIfRequested) {
  EXPECT_THAT(Generate(MTestType().WithKnownProperty(
                  {.category = "size",
                   .descriptor = "what is this?",
                   .enforcement = Property::kIgnoreIfUnknown})),
              IsOkAndHolds(MTestType::kGeneratedValue));
}

// A silly class that simply calls the underlying protected functions. This is
// just used for testing to ensure that it fails properly when the universe is
// not set up properly.
class BadMTestType : public MTestType {
 public:
  absl::StatusOr<TestType> CallRandom(MTestType m) {
    return MTestType::Random<MTestType>("var_name", m);
  }

  template <typename MType>
  absl::StatusOr<typename MType::value_type> CallGenerateValue(
      absl::string_view name) {
    return GenerateValue<MType>(name);
  }
};

TEST(MVariableTest, RandomWithoutAUniverseShouldFail) {
  EXPECT_THAT(BadMTestType().CallRandom(MTestType()),
              IsMisconfigured(InternalConfigurationType::kUniverse));
}

TEST(MVariableTest, RandomWithoutGenerationConfigShouldFail) {
  moriarty_internal::Universe universe;
  BadMTestType m;
  moriarty_internal::MVariableManager(&m).SetUniverse(&universe, "var_name");
  EXPECT_THAT(m.CallRandom(MTestType()),
              IsMisconfigured(InternalConfigurationType::kGenerationConfig));
}

TEST(MVariableTest, GetDependentVariableValueWithoutAUniverseShouldFail) {
  EXPECT_THAT(BadMTestType().CallGenerateValue<MTestType>("x"),
              IsMisconfigured(InternalConfigurationType::kUniverse));
}

}  // namespace
}  // namespace librarian
}  // namespace moriarty

// Test Randomness below
namespace moriarty_testing {
namespace {

using ::moriarty::InternalConfigurationType;

// SeededProtectedMVariable simply exposes the functions for randomness so they
// may be tested.
class MVariableRandom : public MTestType {
 public:
  MVariableRandom() : engine_({1, 2, 3}, "v0.1") {
    universe_.SetRandomEngine(&engine_).SetGenerationConfig(
        &generation_config_);
    moriarty::moriarty_internal::MVariableManager(this).SetUniverse(&universe_,
                                                                    "var_name");
  }
  enum class SeedStyle {
    kNone,
    kUniverseButNoRandomEngine,
    kBothUniverseAndRandomEngine
  };
  explicit MVariableRandom(SeedStyle seed) : engine_({1, 2, 3}, "v0.1") {
    if (seed == SeedStyle::kBothUniverseAndRandomEngine) {
      universe_.SetRandomEngine(&engine_);
    }
    if (seed != SeedStyle::kNone) {
      moriarty::moriarty_internal::MVariableManager(this).SetUniverse(
          &universe_, "var_name");
    }
  }

  // Promote the random functions from protected to public for this test.
  using MTestType::DistinctIntegers;
  using MTestType::RandomComposition;
  using MTestType::RandomElement;
  using MTestType::RandomElementsWithoutReplacement;
  using MTestType::RandomElementsWithReplacement;
  using MTestType::RandomInteger;
  using MTestType::RandomPermutation;
  using MTestType::Shuffle;

 private:
  moriarty::moriarty_internal::Universe universe_;
  moriarty::moriarty_internal::RandomEngine engine_;
  moriarty::moriarty_internal::GenerationConfig generation_config_;
};

// Tests the basic functionality of the above functions.
INSTANTIATE_TYPED_TEST_SUITE_P(MVariableRandom, ValidInputRandomnessTests,
                               MVariableRandom);
INSTANTIATE_TYPED_TEST_SUITE_P(MVariableRandom, InvalidInputRandomnessTests,
                               MVariableRandom);

TEST(MVariableTest, RandomFunctionsWithoutUniverseShouldFail) {
  MVariableRandom unseeded(MVariableRandom::SeedStyle::kNone);
  std::vector<int> helper = {1, 2, 3};

  EXPECT_THAT(unseeded.DistinctIntegers(10, 2),
              IsMisconfigured(InternalConfigurationType::kUniverse));
  EXPECT_THAT(unseeded.RandomInteger(1, 10),
              IsMisconfigured(InternalConfigurationType::kUniverse));
  EXPECT_THAT(unseeded.RandomInteger(10),
              IsMisconfigured(InternalConfigurationType::kUniverse));
  EXPECT_THAT(unseeded.RandomComposition(10, 2),
              IsMisconfigured(InternalConfigurationType::kUniverse));
  EXPECT_THAT(unseeded.RandomElement(helper),
              IsMisconfigured(InternalConfigurationType::kUniverse));
  EXPECT_THAT(unseeded.RandomElementsWithoutReplacement(helper, 2),
              IsMisconfigured(InternalConfigurationType::kUniverse));
  EXPECT_THAT(unseeded.RandomElementsWithReplacement(helper, 2),
              IsMisconfigured(InternalConfigurationType::kUniverse));
  EXPECT_THAT(unseeded.RandomPermutation(2),
              IsMisconfigured(InternalConfigurationType::kUniverse));
  EXPECT_THAT(unseeded.RandomPermutation(2, 6),
              IsMisconfigured(InternalConfigurationType::kUniverse));
  EXPECT_THAT(unseeded.Shuffle(helper),
              IsMisconfigured(InternalConfigurationType::kUniverse));
}

TEST(MVariableTest, RandomFunctionsWithoutRandomConfigShouldFail) {
  MVariableRandom unseeded(
      MVariableRandom::SeedStyle::kUniverseButNoRandomEngine);
  std::vector<int> helper = {1, 2, 3};

  EXPECT_THAT(unseeded.DistinctIntegers(10, 2),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(unseeded.RandomInteger(1, 10),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(unseeded.RandomInteger(10),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(unseeded.RandomComposition(10, 2),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(unseeded.RandomElement(helper),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(unseeded.RandomElementsWithoutReplacement(helper, 2),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(unseeded.RandomElementsWithReplacement(helper, 2),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(unseeded.RandomPermutation(2),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(unseeded.RandomPermutation(2, 6),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(unseeded.Shuffle(helper),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
}

}  // namespace
}  // namespace moriarty_testing
