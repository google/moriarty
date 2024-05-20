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

#include "src/internal/generation_bootstrap.h"

#include <cstdint>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/types/span.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::moriarty_testing::IsUnsatisfiedConstraint;
using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::ContainerEq;
using ::testing::Each;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::Le;
using ::testing::Property;
using ::testing::SizeIs;
using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;

TEST(GenerationBootstrapTest,
     GenerateAllValuesWithNoVariablesOrKnownValuesGeneratesNoValues) {
  RandomEngine rng({1, 2, 3}, "");

  EXPECT_THAT(GenerateAllValues(VariableSet(), ValueSet(), {rng, std::nullopt}),
              IsOkAndHolds(Property(&ValueSet::GetApproximateSize, 0)));
}

TEST(GenerationBootstrapTest,
     GenerateAllValuesWithNoVariablesButSomeKnownValuesSucceeds) {
  RandomEngine rng({1, 2, 3}, "");
  ValueSet known_values;
  known_values.Set<MInteger>("A", 5);
  known_values.Set<MInteger>("B", 10);

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      ValueSet values,
      GenerateAllValues(VariableSet(), known_values, {rng, std::nullopt}));

  EXPECT_THAT(values.Get<MInteger>("A"), IsOkAndHolds(5));
  EXPECT_THAT(values.Get<MInteger>("B"), IsOkAndHolds(10));
}

TEST(
    GenerationBootstrapTest,
    GenerateAllValuesWithKnownValueThatIsNotInVariablesIncludesThatKnownValue) {
  RandomEngine rng({1, 2, 3}, "");
  ValueSet known_values;
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MInteger().Between(123, 456)));
  known_values.Set<MInteger>("B", 10);

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      ValueSet values,
      GenerateAllValues(variables, known_values, {rng, std::nullopt}));

  EXPECT_THAT(values.Get<MInteger>("A"), IsOkAndHolds(AllOf(Ge(123), Le(456))));
  EXPECT_THAT(values.Get<MInteger>("B"), IsOkAndHolds(10));
}

TEST(GenerationBootstrapTest, GenerateAllValuesGivesValuesToVariables) {
  RandomEngine rng({1, 2, 3}, "");
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MInteger().Between(123, 456)));
  MORIARTY_ASSERT_OK(variables.AddVariable("B", MInteger().Between(777, 888)));

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      ValueSet values,
      GenerateAllValues(variables, ValueSet(), {rng, std::nullopt}));

  EXPECT_THAT(values.Get<MInteger>("A"), IsOkAndHolds(AllOf(Ge(123), Le(456))));
  EXPECT_THAT(values.Get<MInteger>("B"), IsOkAndHolds(AllOf(Ge(777), Le(888))));
}

TEST(GenerationBootstrapTest, GenerateAllValuesRespectsKnownValues) {
  RandomEngine rng({1, 2, 3}, "");
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MInteger().Between(123, 456)));
  ValueSet known_values;
  known_values.Set<MInteger>("A", 314);

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      ValueSet values,
      GenerateAllValues(variables, known_values, {rng, std::nullopt}));

  EXPECT_THAT(values.Get<MInteger>("A"), IsOkAndHolds(314));
}

TEST(GenerationBootstrapTest, GenerateAllValuesWithDependentVariablesSucceeds) {
  RandomEngine rng({1, 2, 3}, "");
  VariableSet variables;
  MORIARTY_ASSERT_OK(
      variables.AddVariable("A", MInteger().Between("N", "3 * N")));
  MORIARTY_ASSERT_OK(variables.AddVariable("N", MInteger().Between(50, 100)));

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      ValueSet values,
      GenerateAllValues(variables, ValueSet(), {rng, std::nullopt}));

  EXPECT_THAT(values.Get<MInteger>("N"), IsOkAndHolds(AllOf(Ge(50), Le(100))));
  MORIARTY_ASSERT_OK_AND_ASSIGN(int64_t N, values.Get<MInteger>("N"));
  EXPECT_THAT(values.Get<MInteger>("A"), IsOkAndHolds(AllOf(Ge(N), Le(3 * N))));
}

TEST(GenerationBootstrapTest, GenerateAllValuesWithDependentValuesSucceeds) {
  RandomEngine rng({1, 2, 3}, "");
  VariableSet variables;
  MORIARTY_ASSERT_OK(
      variables.AddVariable("A", MInteger().Between("N", "3 * N")));
  MORIARTY_ASSERT_OK(variables.AddVariable("C", MInteger().Is("N")));
  MORIARTY_ASSERT_OK(variables.AddVariable("B", MInteger().Is("2 * C")));
  ValueSet known_values;
  known_values.Set<MInteger>("N", 53);

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      ValueSet values,
      GenerateAllValues(variables, known_values, {rng, std::nullopt}));

  EXPECT_THAT(values.Get<MInteger>("A"),
              IsOkAndHolds(AllOf(Ge(53), Le(3 * 53))));
  EXPECT_THAT(values.Get<MInteger>("C"), IsOkAndHolds(53));
  EXPECT_THAT(values.Get<MInteger>("B"), IsOkAndHolds(2 * 53));
}

TEST(GenerationBootstrapTest,
     GenerateAllValuesWithMissingDependentVariableAndValueFails) {
  RandomEngine rng({1, 2, 3}, "");
  VariableSet variables;
  MORIARTY_ASSERT_OK(
      variables.AddVariable("A", MInteger().Between("N", "3 * N")));

  EXPECT_THAT(GenerateAllValues(variables, ValueSet(), {rng, std::nullopt}),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Unknown dependency")));
}

TEST(GenerationBootstrapTest, GenerateAllValuesShouldRespectIsAndIsOneOf) {
  RandomEngine rng({1, 2, 3}, "");
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MInteger().Is(15)));
  MORIARTY_ASSERT_OK(
      variables.AddVariable("B", MInteger().IsOneOf({111, 222, 333})));

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      ValueSet values,
      GenerateAllValues(variables, ValueSet(), {rng, std::nullopt}));

  EXPECT_THAT(values.Get<MInteger>("A"), IsOkAndHolds(15));
  EXPECT_THAT(values.Get<MInteger>("B"), IsOkAndHolds(AnyOf(111, 222, 333)));
}

TEST(GenerationBootstrapTest,
     GenerateAllValuesWithSoftGenerationLimitStopsEarly) {
  RandomEngine rng({1, 2, 3}, "");
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable(
      "S", MString().WithAlphabet("abc").OfLength(50, 10000)));

  // S could generate a string with a really long length. However, the soft
  // generation limit of 100 should stop a large string being generated.
  int generation_limit = 100;
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      ValueSet values,
      GenerateAllValues(variables, ValueSet(), {rng, generation_limit}));

  EXPECT_THAT(values.Get<MString>("S"),
              IsOkAndHolds(SizeIs(AllOf(Ge(50), Le(100)))));
}

TEST(GenerationBootstrapTest,
     GenerateAllValuesWithAKnownValueThatIsInvalidShouldFail) {
  RandomEngine rng({1, 2, 3}, "");
  VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MInteger().Between(123, 456)));
  ValueSet known_values;
  known_values.Set<MInteger>("A", 0);

  EXPECT_THAT(GenerateAllValues(variables, known_values, {rng, std::nullopt}),
              IsUnsatisfiedConstraint("range"));
}

TEST(GenerationBootstrapTest,
     GenerateAllValuesGivesStableResultsNoMatterTheInsertionOrder) {
  absl::flat_hash_map<std::string, MInteger> named_variables = {
      {"A", MInteger().Between(111, 222)},
      {"B", MInteger().Between(333, 444)},
      {"C", MInteger().Between(555, 666)},
  };
  std::vector<std::string> names = {"A", "B", "C"};

  // Do all 3! = 6 permutations, check they all generate the same values.
  std::vector<std::tuple<int64_t, int64_t, int64_t>> results;
  do {
    RandomEngine rng({1, 2, 3}, "");
    VariableSet variables;
    for (const std::string& name : names)
      MORIARTY_ASSERT_OK(variables.AddVariable(name, named_variables[name]));
    MORIARTY_ASSERT_OK_AND_ASSIGN(
        ValueSet values,
        GenerateAllValues(variables, ValueSet(), {rng, std::nullopt}));
    MORIARTY_ASSERT_OK_AND_ASSIGN(int64_t A,
                                           values.Get<MInteger>("A"));
    MORIARTY_ASSERT_OK_AND_ASSIGN(int64_t B,
                                           values.Get<MInteger>("B"));
    MORIARTY_ASSERT_OK_AND_ASSIGN(int64_t C,
                                           values.Get<MInteger>("C"));
    results.push_back({A, B, C});
  } while (absl::c_next_permutation(names));

  ASSERT_THAT(results, SizeIs(6));
  EXPECT_THAT(results, Each(Eq(results[0])));
}

TEST(GenerationBootstrapTest,
     GenerateAllValuesGivesStableResultsWithDependentVariables) {
  absl::flat_hash_map<std::string, MInteger> named_variables = {
      {"A", MInteger().Between(111, "B")},  // Forwards in alphabet. A -> B
      {"B", MInteger().Between(222, 333)},  //
      {"C", MInteger().Between(444, 555)},  // Backwards in alphabet. D -> C
      {"D", MInteger().Between("C", 666)},  //
      {"E", MInteger().Between(777, "G")},  // Two layers deep E -> G -> F
      {"F", MInteger().Between(888, 999)},  //
      {"G", MInteger().Between("F", 999)}};
  std::vector<std::string> names = {"A", "B", "C", "D", "E", "F", "G"};

  // Do all 7! = 5040 permutations, check they all generate the same values.
  std::vector<std::vector<int64_t>> results;
  do {
    RandomEngine rng({1, 2, 3}, "");
    VariableSet variables;
    for (const std::string& name : names)
      MORIARTY_ASSERT_OK(variables.AddVariable(name, named_variables[name]));
    MORIARTY_ASSERT_OK_AND_ASSIGN(
        ValueSet values,
        GenerateAllValues(variables, ValueSet(), {rng, std::nullopt}));
    MORIARTY_ASSERT_OK_AND_ASSIGN(int64_t A,
                                           values.Get<MInteger>("A"));
    MORIARTY_ASSERT_OK_AND_ASSIGN(int64_t B,
                                           values.Get<MInteger>("B"));
    MORIARTY_ASSERT_OK_AND_ASSIGN(int64_t C,
                                           values.Get<MInteger>("C"));
    MORIARTY_ASSERT_OK_AND_ASSIGN(int64_t D,
                                           values.Get<MInteger>("D"));
    MORIARTY_ASSERT_OK_AND_ASSIGN(int64_t E,
                                           values.Get<MInteger>("E"));
    MORIARTY_ASSERT_OK_AND_ASSIGN(int64_t F,
                                           values.Get<MInteger>("F"));
    MORIARTY_ASSERT_OK_AND_ASSIGN(int64_t G,
                                           values.Get<MInteger>("G"));
    results.push_back({A, B, C, D, E, F, G});
  } while (absl::c_next_permutation(names));

  ASSERT_THAT(results, SizeIs(5040));
  EXPECT_THAT(results, Each(Eq(results[0])));
}

TEST(GenerationBootstrapTest, GetGenerationOrderSingleRoot) {
  absl::flat_hash_map<std::string, std::vector<std::string>> deps_map = {
      {"A", {"C"}}, {"X", {"Y"}},      {"C", {"D"}},
      {"D", {}},    {"Y", {"A", "B"}}, {"B", {"C"}}};

  std::vector<std::string> expected_result = {"X", "Y", "A", "B", "C", "D"};

  EXPECT_THAT(GetGenerationOrder(deps_map, ValueSet()),
              IsOkAndHolds(ContainerEq(expected_result)));
}

TEST(GenerationConfigTest, GetGenerationOrderMultiRoot) {
  absl::flat_hash_map<std::string, std::vector<std::string>> deps_map = {
      {"A", {"C"}}, {"F", {"X"}}, {"X", {"Y"}}, {"Y", {"A", "B"}}, {"T", {"C"}},
      {"R", {"A"}}, {"C", {"W"}}, {"A", {"W"}}, {"W", {}},         {"B", {}}};

  std::vector<std::string> expected_result = {"F", "R", "T", "X", "Y",
                                              "A", "B", "C", "W"};

  EXPECT_THAT(GetGenerationOrder(deps_map, ValueSet()),
              IsOkAndHolds(ContainerEq(expected_result)));
}

TEST(GenerationConfigTest, GetGenerationOrderSingleNode) {
  absl::flat_hash_map<std::string, std::vector<std::string>> deps_map = {
      {"X", {}}};

  std::vector<std::string> expected_result = {"X"};

  EXPECT_THAT(GetGenerationOrder(deps_map, ValueSet()),
              IsOkAndHolds(ContainerEq(expected_result)));
}

TEST(GenerationConfigTest, GetGenerationOrderMultiEdgesRemovesDuplicates) {
  absl::flat_hash_map<std::string, std::vector<std::string>> deps_map = {
      {"X", {"B", "B"}}, {"B", {}}};

  std::vector<std::string> expected_result = {"X", "B"};

  EXPECT_THAT(GetGenerationOrder(deps_map, ValueSet()),
              IsOkAndHolds(ContainerEq(expected_result)));
}

TEST(GenerationConfigTest, GetGenerationOrderRemainsStable) {
  std::vector<std::string> x_dependencies = {"A", "B", "C"};
  std::vector<std::string> expected_result = {"X", "A", "B", "C"};

  std::vector<std::vector<std::string>> results;
  do {
    absl::flat_hash_map<std::string, std::vector<std::string>> deps_map = {
        {"A", {}}, {"B", {}}, {"C", {}}, {"X", x_dependencies}};
    MORIARTY_ASSERT_OK_AND_ASSIGN(
        std::vector<std::string> ordered_vars,
        GetGenerationOrder(deps_map, ValueSet()));
    results.push_back(ordered_vars);
  } while (absl::c_next_permutation(x_dependencies));

  ASSERT_THAT(results, SizeIs(6));
  EXPECT_THAT(results, Each(Eq(results[0])));
}

TEST(GenerationConfigTest, GetGenerationOrderNoElementsReturnsEmpty) {
  absl::flat_hash_map<std::string, std::vector<std::string>> deps_map = {};

  std::vector<std::string> expected_result = {};

  EXPECT_THAT(GetGenerationOrder(deps_map, ValueSet()),
              IsOkAndHolds(ContainerEq(expected_result)));
}

TEST(GenerationConfigTest, GetGenerationOrderIgnoresElementInValueSet) {
  ValueSet vs;
  vs.Set<MInteger>("N", 53);

  absl::flat_hash_map<std::string, std::vector<std::string>> deps_map = {
      {"X", {"N"}}};

  std::vector<std::string> expected_result = {"X"};

  EXPECT_THAT(GetGenerationOrder(deps_map, vs),
              IsOkAndHolds(ContainerEq(expected_result)));
}

TEST(GenerationConfigTest, GetGenerationOrderFailsOnNonExistingDependency) {
  absl::flat_hash_map<std::string, std::vector<std::string>> deps_map = {
      {"X", {"N"}}};

  EXPECT_THAT(GetGenerationOrder(deps_map, ValueSet()),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(GenerationConfigTest, GetGenerationOrderFailsOnCycle) {
  absl::flat_hash_map<std::string, std::vector<std::string>> deps_map = {
      {"X", {"Y"}},
      {"Y", {"A", "B", "X"}},
      {"A", {"C"}},
      {"B", {"C"}},
      {"C", {"D"}}};

  EXPECT_THAT(GetGenerationOrder(deps_map, ValueSet()),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(GenerationConfigTest, GetGenerationOrderFailsOnItselfCycle) {
  absl::flat_hash_map<std::string, std::vector<std::string>> deps_map = {
      {"X", {"X"}}};

  EXPECT_THAT(GetGenerationOrder(deps_map, ValueSet()),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
