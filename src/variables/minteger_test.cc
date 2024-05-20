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

#include "src/variables/minteger.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "src/librarian/size_property.h"
#include "src/librarian/test_utils.h"
#include "src/util/test_status_macro/status_testutil.h"

namespace moriarty {
namespace {

using ::moriarty_testing::Context;
using ::moriarty_testing::Generate;
using ::moriarty_testing::GenerateDifficultInstancesValues;
using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::GenerateSameValues;
using ::moriarty_testing::GetUniqueValue;
using ::moriarty_testing::IsNotSatisfiedWith;
using ::moriarty_testing::IsSatisfiedWith;
using ::moriarty_testing::Print;
using ::moriarty_testing::Read;
using ::testing::AllOf;
using ::testing::Each;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::IsSupersetOf;
using ::testing::Le;
using ::testing::Optional;
using ::testing::UnorderedElementsAre;
using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;

TEST(MIntegerTest, TypenameIsCorrect) {
  EXPECT_EQ(MInteger().Typename(), "MInteger");
}

TEST(MIntegerTest, PrintShouldSucceed) {
  EXPECT_THAT(Print(MInteger(), -1), IsOkAndHolds("-1"));
  EXPECT_THAT(Print(MInteger(), 0), IsOkAndHolds("0"));
  EXPECT_THAT(Print(MInteger(), 1), IsOkAndHolds("1"));
}

TEST(MIntegerTest, ValidReadShouldSucceed) {
  EXPECT_THAT(Read(MInteger(), "123"), IsOkAndHolds(123));
  EXPECT_THAT(Read(MInteger(), "456 "), IsOkAndHolds(456));
  EXPECT_THAT(Read(MInteger(), "-789"), IsOkAndHolds(-789));

  // Extremes
  int64_t min = std::numeric_limits<int64_t>::min();
  int64_t max = std::numeric_limits<int64_t>::max();
  EXPECT_THAT(Read(MInteger(), std::to_string(min)), IsOkAndHolds(min));
  EXPECT_THAT(Read(MInteger(), std::to_string(max)), IsOkAndHolds(max));
}

TEST(MIntegerTest, ReadWithTokensAfterwardsIsFine) {
  EXPECT_THAT(Read(MInteger(), "-123 you should ignore this"),
              IsOkAndHolds(-123));
}

TEST(MIntegerTest, InvalidReadShouldFail) {
  // EOF
  EXPECT_THAT(Read(MInteger(), ""),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  // EOF with whitespace
  EXPECT_THAT(Read(MInteger(), " "),
              StatusIs(absl::StatusCode::kFailedPrecondition));

  // Double negative
  EXPECT_THAT(Read(MInteger(), "--123"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  // Invalid character start/middle/end
  EXPECT_THAT(Read(MInteger(), "c123"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(Read(MInteger(), "12c3"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(Read(MInteger(), "123c"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(MIntegerTest, GenerateShouldSuccessfullyComplete) {
  moriarty::MInteger variable;
  MORIARTY_EXPECT_OK(Generate(variable));
}

TEST(MIntegerTest, BetweenShouldRestrictTheRangeProperly) {
  EXPECT_THAT(MInteger().Between(5, 5), GeneratedValuesAre(5));
  EXPECT_THAT(MInteger().Between(5, 10),
              GeneratedValuesAre(AllOf(Ge(5), Le(10))));
  EXPECT_THAT(MInteger().Between(-1, 1),
              GeneratedValuesAre(AllOf(Ge(-1), Le(1))));
}

TEST(MIntegerTest, RepeatedBetweenCallsShouldBeIntersectedTogether) {
  // All possible valid intersections
  EXPECT_TRUE(
      GenerateSameValues(MInteger().Between(0, 30).Between(1, 10),
                         MInteger().Between(1, 10)));  // First is a superset
  EXPECT_TRUE(
      GenerateSameValues(MInteger().Between(1, 10).Between(0, 30),
                         MInteger().Between(1, 10)));  // Second is a superset
  EXPECT_TRUE(
      GenerateSameValues(MInteger().Between(0, 10).Between(1, 30),
                         MInteger().Between(1, 10)));  // First on the left
  EXPECT_TRUE(
      GenerateSameValues(MInteger().Between(1, 30).Between(0, 10),
                         MInteger().Between(1, 10)));  // First on the right
  EXPECT_TRUE(GenerateSameValues(MInteger().Between(1, 8).Between(8, 10),
                                 MInteger().Between(8, 8)));  // Singleton Range

  // Several chained calls to Between should work
  EXPECT_TRUE(
      GenerateSameValues(MInteger().Between(1, 20).Between(3, 21).Between(2, 5),
                         MInteger().Between(3, 5)));
}

TEST(MIntegerTest, InvalidBoundsShouldCrash) {
  // Min > Max
  EXPECT_THAT(Generate(MInteger().Between(0, -1)),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Valid range is empty")));

  // Empty intersection (First interval to the left)
  EXPECT_THAT(Generate(MInteger().Between(1, 10).Between(20, 30)),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Valid range is empty")));

  // Empty intersection (First interval to the right)
  EXPECT_THAT(Generate(MInteger().Between(20, 30).Between(1, 10)),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Valid range is empty")));
}

// TODO(darcybest): MInteger should have an equality operator instead of this.
TEST(MIntegerTest, MergeFromCorrectlyMerges) {
  // All possible valid intersections
  EXPECT_TRUE(GenerateSameValues(
      MInteger().Between(0, 30).MergeFrom(MInteger().Between(1, 10)),
      MInteger().Between(0, 30).Between(1, 10)));  // First superset
  EXPECT_TRUE(GenerateSameValues(
      MInteger().Between(1, 10).MergeFrom(MInteger().Between(0, 30)),
      MInteger().Between(1, 10).Between(0, 30)));  // Second superset
  EXPECT_TRUE(GenerateSameValues(
      MInteger().Between(0, 10).MergeFrom(MInteger().Between(1, 30)),
      MInteger().Between(0, 10).Between(1, 30)));  // First on left
  EXPECT_TRUE(GenerateSameValues(
      MInteger().Between(1, 30).MergeFrom(MInteger().Between(0, 10)),
      MInteger().Between(1, 30).Between(0, 10)));  // First on  right
  EXPECT_TRUE(GenerateSameValues(
      MInteger().Between(1, 8).MergeFrom(MInteger().Between(8, 10)),
      MInteger().Between(1, 8).Between(8, 10)));  // Singleton Range
}

TEST(MIntegerTest, SatisfiesConstraintsWorksForValid) {
  EXPECT_THAT(MInteger().Between(1, 10), IsSatisfiedWith(5));   // Middle
  EXPECT_THAT(MInteger().Between(1, 10), IsSatisfiedWith(1));   // Low
  EXPECT_THAT(MInteger().Between(1, 10), IsSatisfiedWith(10));  // High

  // Whole range
  EXPECT_THAT(MInteger(), IsSatisfiedWith(0));
  EXPECT_THAT(MInteger(), IsSatisfiedWith(std::numeric_limits<int64_t>::min()));
  EXPECT_THAT(MInteger(), IsSatisfiedWith(std::numeric_limits<int64_t>::max()));
}

TEST(MIntegerTest, SatisfiesConstraintsWorksForInvalid) {
  EXPECT_THAT(MInteger().Between(1, 10), IsNotSatisfiedWith(0, "range"));
  EXPECT_THAT(MInteger().Between(1, 10), IsNotSatisfiedWith(11, "range"));

  // Empty range
  EXPECT_THAT(MInteger().Between(1, 0), IsNotSatisfiedWith(0, "range"));
}

TEST(MIntegerTest, SatisfiesConstraintsWorksForValidExpressions) {
  EXPECT_THAT(
      MInteger().Between(1, "3 * N + 1"),
      IsSatisfiedWith(5, Context().WithValue<MInteger>("N", 10)));  // Mid
  EXPECT_THAT(
      MInteger().Between(1, "3 * N + 1"),
      IsSatisfiedWith(1, Context().WithValue<MInteger>("N", 10)));  // Lo
  EXPECT_THAT(
      MInteger().Between(1, "3 * N + 1"),
      IsSatisfiedWith(31, Context().WithValue<MInteger>("N", 10)));  // High
}

TEST(MIntegerTest, SatisfiesConstraintsWorksForInvalidExpressions) {
  EXPECT_THAT(
      MInteger().Between(1, "3 * N + 1"),
      IsNotSatisfiedWith(0, "range", Context().WithValue<MInteger>("N", 10)));
  EXPECT_THAT(MInteger().Between(1, "3 * N + 1"),
              IsNotSatisfiedWith(0, "range"));
}

TEST(MIntegerTest, AtMostAndAtLeastShouldLimitTheOutputRange) {
  EXPECT_THAT(MInteger().AtMost(10).AtLeast(-5),
              GeneratedValuesAre(AllOf(Le(10), Ge(-5))));
}

TEST(MIntegerTest, AtMostLargerThanAtLeastShouldFail) {
  EXPECT_THAT(Generate(MInteger().AtLeast(10).AtMost(0)),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Valid range is empty")));

  EXPECT_THAT(Generate(MInteger().AtLeast(0).AtMost("3 * N + 1"),
                       Context().WithValue<MInteger>("N", -3)),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Valid range is empty")));
}

TEST(MIntegerTest,
     AtMostAtLeastBetweenWithUnparsableExpressionsShouldFailLazily) {
  // The following should not crash
  MInteger x = MInteger().AtLeast("3 + ");
  MInteger y = MInteger().AtMost("12345678901234567890");  // Overflow
  MInteger z = MInteger().Between("N + 2", "* M + M");

  EXPECT_THAT(Generate(x), StatusIs(absl::StatusCode::kFailedPrecondition,
                                    HasSubstr("invalid expression")));
  EXPECT_THAT(Generate(y), StatusIs(absl::StatusCode::kFailedPrecondition,
                                    HasSubstr("invalid expression")));
  EXPECT_THAT(Generate(z), StatusIs(absl::StatusCode::kFailedPrecondition,
                                    HasSubstr("invalid expression")));
}

TEST(MIntegerTest, AtMostAndAtLeastWithExpressionsShouldLimitTheOutputRange) {
  // TODO(darcybest): Make GeneratedValuesAre compatible with Context instead of
  // using GenerateLots().
  EXPECT_THAT(GenerateLots(MInteger().AtLeast(0).AtMost("3 * N + 1"),
                           Context().WithValue<MInteger>("N", 10)),
              IsOkAndHolds(Each(AllOf(Ge(0), Le(31)))));
  EXPECT_THAT(GenerateLots(MInteger().AtLeast("3 * N + 1").AtMost(50),
                           Context().WithValue<MInteger>("N", 10)),
              IsOkAndHolds(Each(AllOf(Ge(31), Le(50)))));
}

TEST(
    MIntegerTest,
    MultipleExpressionsAndConstantsInAtLeastAtMostBetweenShouldRestrictOutput) {
  // TODO(darcybest): Make GeneratedValuesAre compatible with Context.
  EXPECT_THAT(
      GenerateLots(
          MInteger().AtLeast(0).AtMost("3 * N + 1").Between("N + M", 100),
          Context().WithValue<MInteger>("N", 10).WithValue<MInteger>("M", 15)),
      IsOkAndHolds(Each(AllOf(Ge(0), Le(31), Ge(25), Le(100)))));

  EXPECT_THAT(
      GenerateLots(
          MInteger()
              .AtLeast("3 * N + 1")
              .AtLeast("3 * M + 3")
              .AtMost(50)
              .AtMost("M ^ 2"),
          Context().WithValue<MInteger>("N", 6).WithValue<MInteger>("M", 5)),
      IsOkAndHolds(Each(AllOf(Ge(19), Ge(18), Le(50), Le(25)))));
}

TEST(MIntegerTest, AllOverloadsOfBetweenAreEffective) {
  EXPECT_THAT(MInteger().Between(1, 10),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(MInteger().Between(1, "10"),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(MInteger().Between("1", 10),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(MInteger().Between("1", "10"),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
}

TEST(MIntegerTest, IsMIntegerExpressionShouldRestrictInput) {
  EXPECT_THAT(Generate(MInteger().Is("3 * N + 1"),
                       Context().WithValue<MInteger>("N", 10)),
              IsOkAndHolds(31));
}

TEST(MIntegerTest, GetUniqueValueWorksWhenUniqueValueKnown) {
  EXPECT_THAT(GetUniqueValue(MInteger().Between("N", "N"),
                             Context().WithValue<MInteger>("N", 10)),
              Optional(10));

  EXPECT_THAT(GetUniqueValue(MInteger().Between(20, "2 * N"),
                             Context().WithValue<MInteger>("N", 10)),
              Optional(20));
}

TEST(MIntegerTest, GetUniqueValueWithNestedDependenciesShouldWork) {
  EXPECT_THAT(GetUniqueValue(
                  MInteger().Between("X", "Y"),
                  Context()
                      .WithVariable<MInteger>("X", MInteger().Is(5))
                      .WithVariable<MInteger>("Y", MInteger().Between(5, "N"))
                      .WithValue<MInteger>("N", 5)),
              Optional(5));
}

TEST(MIntegerTest, GetUniqueValueFailsWhenAVariableIsUnknown) {
  EXPECT_EQ(GetUniqueValue(MInteger().Between("N", "N")), std::nullopt);
}

TEST(MIntegerTest, GetUniqueValueShouldSucceedIfTheValueIsUnique) {
  EXPECT_THAT(GetUniqueValue(MInteger().Between(123, 123)), Optional(123));
  EXPECT_THAT(GetUniqueValue(MInteger().Is(456)), Optional(456));
}

TEST(MIntegerTest, GetUniqueValueFailsWhenTheValueIsNotUnique) {
  EXPECT_EQ(GetUniqueValue(MInteger().Between(8, 10)), std::nullopt);
}

TEST(MIntegerTest, OfSizePropertyOnlyAcceptsSizeAsCategory) {
  EXPECT_THAT(
      MInteger().OfSizeProperty({.category = "wrong"}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("category")));
}

TEST(MIntegerTest, OfSizePropertyOnlyAcceptsKnownSizes) {
  EXPECT_THAT(
      MInteger().OfSizeProperty(
          {.category = "size", .descriptor = "unknown_type"}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("Unknown size")));
}

TEST(MIntegerTest, GetDifficultInstancesIncludesManyInterestingValues) {
  EXPECT_THAT(GenerateDifficultInstancesValues(MInteger()),
              IsOkAndHolds(IsSupersetOf(
                  {0LL, 1LL, 2LL, -1LL, 1LL << 32, (1LL << 62) - 1})));
}

TEST(MIntegerTest, GetDifficultInstancesIncludesIntMinAndMaxByDefault) {
  EXPECT_THAT(
      GenerateDifficultInstancesValues(MInteger()),
      IsOkAndHolds(IsSupersetOf({std::numeric_limits<int64_t>::min(),
                                 std::numeric_limits<int64_t>::max()})));
}

TEST(MIntegerTest, GetDifficultInstancesIncludesMinAndMaxValues) {
  EXPECT_THAT(GenerateDifficultInstancesValues(MInteger().Between(123, 234)),
              IsOkAndHolds(IsSupersetOf({123, 234})));
}

TEST(MIntegerTest, GetDifficultInstancesValuesAreNotRepeated) {
  EXPECT_THAT(GenerateDifficultInstancesValues(MInteger().Between(-1, 1)),
              IsOkAndHolds(UnorderedElementsAre(-1, 0, 1)));
}

TEST(MIntegerTest,
     GetDifficultInstancesForFixedNonDifficultValueFailsGeneration) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      std::vector<MInteger> instances,
      MInteger().Is(1234).GetDifficultInstances());

  for (MInteger instance : instances) {
    EXPECT_THAT(Generate(instance),
                StatusIs(absl::StatusCode::kFailedPrecondition,
                         HasSubstr("no viable value found")));
  }
}

TEST(MIntegerTest, WithSizeGivesAppropriatelySizedValues) {
  // These values here are fuzzy and may need to be changed over time. "small"
  // might be changed over time. The bounds here are mostly just to check the
  // approximate sizes are considered.

  EXPECT_THAT(MInteger().Between(1, "10^9").WithSize(CommonSize::kMin),
              GeneratedValuesAre(Eq(1)));
  EXPECT_THAT(MInteger().Between(1, "10^9").WithSize(CommonSize::kTiny),
              GeneratedValuesAre(Le(30)));
  EXPECT_THAT(MInteger().Between(1, "10^9").WithSize(CommonSize::kSmall),
              GeneratedValuesAre(Le(2000)));
  EXPECT_THAT(MInteger().Between(1, "10^9").WithSize(CommonSize::kMedium),
              GeneratedValuesAre(Le(1000000)));
  EXPECT_THAT(MInteger().Between(1, "10^9").WithSize(CommonSize::kLarge),
              GeneratedValuesAre(Ge(1000000)));
  EXPECT_THAT(MInteger().Between(1, "10^9").WithSize(CommonSize::kHuge),
              GeneratedValuesAre(Ge(500000000)));
  EXPECT_THAT(MInteger().Between(1, "10^9").WithSize(CommonSize::kMax),
              GeneratedValuesAre(Eq(1000000000)));
}

TEST(MIntegerTest, WithSizeBehavesWithMergeFrom) {
  MInteger small = MInteger().Between(1, "10^9").WithSize(CommonSize::kSmall);
  MInteger tiny = MInteger().Between(1, "10^9").WithSize(CommonSize::kTiny);
  MInteger large = MInteger().Between(1, "10^9").WithSize(CommonSize::kLarge);
  MInteger any = MInteger().Between(1, "10^9").WithSize(CommonSize::kAny);

  {
    EXPECT_FALSE(GenerateSameValues(small, tiny));
    MORIARTY_EXPECT_OK(small.TryMergeFrom(tiny));
    EXPECT_TRUE(GenerateSameValues(small, tiny));
  }
  {
    EXPECT_FALSE(GenerateSameValues(any, large));
    MORIARTY_EXPECT_OK(any.TryMergeFrom(large));
    EXPECT_TRUE(GenerateSameValues(any, large));
  }

  EXPECT_THAT(tiny.TryMergeFrom(large),
              StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("size")));
}

TEST(MIntegerTest, ToStringWorks) {
  EXPECT_THAT(MInteger().ToString(), HasSubstr("MInteger"));
  EXPECT_THAT(MInteger().Between(1, 10).ToString(), HasSubstr("[1, 10]"));
  EXPECT_THAT(MInteger().WithSize(CommonSize::kSmall).ToString(),
              HasSubstr("mall"));  // [S|s]mall
  EXPECT_THAT(MInteger().Between(1, 5).WithSize(CommonSize::kSmall).ToString(),
              AllOf(HasSubstr("[1, 5]"), HasSubstr("mall")));  // [S|s]mall
}

}  // namespace
}  // namespace moriarty
