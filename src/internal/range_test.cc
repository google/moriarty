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

#include "src/internal/range.h"

#include <cstdint>
#include <limits>
#include <optional>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "src/util/test_status_macro/status_testutil.h"

namespace moriarty {
namespace {

using ::moriarty::IsOk;
using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::Optional;
using ::testing::UnorderedElementsAre;

Range Intersect(Range r1, const Range& r2) {
  r1.Intersect(r2);
  return r1;
}

testing::AssertionResult EqualRanges(const Range& r1, const Range& r2) {
  auto extremes1 = r1.Extremes();
  auto extremes2 = r2.Extremes();
  if (!extremes1.ok())
    return testing::AssertionFailure()
           << "r1.Extremes() failed; " << extremes1.status();
  if (!extremes2.ok())
    return testing::AssertionFailure()
           << "r2.Extremes() failed; " << extremes2.status();

  if (!(*extremes1).has_value() && !(*extremes2).has_value())
    return testing::AssertionSuccess() << "both ranges are empty";

  if (!(*extremes1).has_value())
    return testing::AssertionFailure()
           << "first range is empty, second is "
           << "[" << (*extremes2)->min << ", " << (*extremes2)->max << "]";

  if (!(*extremes2).has_value())
    return testing::AssertionFailure()
           << "first range is "
           << "[" << (*extremes1)->min << ", " << (*extremes1)->max << "]"
           << ", second is empty";

  if ((*extremes1) == (*extremes2))
    return testing::AssertionSuccess()
           << "both ranges are [" << (*extremes1)->min << ", "
           << (*extremes1)->max;

  return testing::AssertionFailure()
         << "are not equal "
         << "[" << (*extremes1)->min << ", " << (*extremes1)->max << "]"
         << " vs "
         << "[" << (*extremes2)->min << ", " << (*extremes2)->max << "]";
}

testing::AssertionResult IsEmptyRange(const Range& r) {
  auto extremes = r.Extremes();
  if (!extremes.ok())
    return testing::AssertionFailure() << "Extremes() failed.";

  if (!(*extremes))
    return testing::AssertionSuccess() << "is an empty range";
  else
    return testing::AssertionFailure()
           << "[" << (*extremes)->min << ", " << (*extremes)->min
           << "] is a non-empty range";
}

TEST(RangeTest, IntersectNonEmptyIntersectionsWork) {
  // Partial overlap
  EXPECT_TRUE(EqualRanges(Intersect({1, 10}, {5, 12}), Range(5, 10)));
  // Subset
  EXPECT_TRUE(EqualRanges(Intersect({1, 10}, {5, 8}), Range(5, 8)));
  EXPECT_TRUE(EqualRanges(Intersect({1, 10}, {0, 18}), Range(1, 10)));
  // Equal
  EXPECT_TRUE(EqualRanges(Intersect({1, 10}, {1, 10}), Range(1, 10)));
  // Singleton overlap
  EXPECT_TRUE(EqualRanges(Intersect({1, 10}, {10, 100}), Range(10, 10)));
  EXPECT_TRUE(EqualRanges(Intersect({1, 10}, {-5, 1}), Range(1, 1)));
}

TEST(RangeTest, EmptyRangeWorks) {
  EXPECT_TRUE(IsEmptyRange({0, -1}));
  EXPECT_TRUE(IsEmptyRange({10, 2}));

  EXPECT_FALSE(IsEmptyRange({0, 0}));
  EXPECT_FALSE(IsEmptyRange({5, 10}));

  EXPECT_TRUE(IsEmptyRange(EmptyRange()));
}

TEST(RangeTest, EmptyIntersectionsWork) {
  // Normal cases
  EXPECT_TRUE(IsEmptyRange(Intersect({1, 10}, {11, 100})));
  EXPECT_TRUE(IsEmptyRange(Intersect({101, 1000}, {11, 100})));

  // Input was already empty
  EXPECT_TRUE(IsEmptyRange(Intersect({10, 1}, {5, 5})));
  EXPECT_TRUE(IsEmptyRange(Intersect({10, 1}, {10, 1})));

  EXPECT_FALSE(IsEmptyRange(Intersect({10, 10}, {10, 100})));
}

TEST(RangeTest, ExtremesWork) {
  EXPECT_EQ(Range::ExtremeValues({1, 2}), Range::ExtremeValues({1, 2}));

  // Normal case
  EXPECT_THAT(Range(1, 2).Extremes(),
              IsOkAndHolds(Optional(Range::ExtremeValues({1, 2}))));

  // Default constructor gives full 64-bit range
  EXPECT_THAT(Range().Extremes(), IsOkAndHolds(Optional(Range::ExtremeValues(
                                      {std::numeric_limits<int64_t>::min(),
                                       std::numeric_limits<int64_t>::max()}))));

  // Empty range returns nullopt
  EXPECT_THAT(EmptyRange().Extremes(), IsOkAndHolds(std::nullopt));
}

TEST(RangeTest,
     RepeatedCallsToAtMostAndAtLeastIntegerVersionsShouldConsiderAll) {
  Range R;
  R.AtLeast(5);
  R.AtLeast(6);
  R.AtLeast(4);

  EXPECT_THAT(R.Extremes(), IsOkAndHolds(Optional(Range::ExtremeValues(
                                {6, std::numeric_limits<int64_t>::max()}))));

  R.AtMost(30);
  R.AtMost(20);
  R.AtMost(10);
  R.AtMost(15);
  EXPECT_THAT(R.Extremes(),
              IsOkAndHolds(Optional(Range::ExtremeValues({6, 10}))));

  R.AtMost(5);
  EXPECT_TRUE(IsEmptyRange(R));
}

TEST(RangeTest, ExpressionsWorkInAtLeastAndAtMost) {
  Range R;
  MORIARTY_ASSERT_OK(R.AtLeast("N + 5"));
  MORIARTY_ASSERT_OK(R.AtMost("3 * N + 1"));

  EXPECT_THAT(R.Extremes({{"N", 4}}),
              IsOkAndHolds(Optional(Range::ExtremeValues({9, 13}))));
  EXPECT_THAT(R.Extremes({{"N", 2}}),
              IsOkAndHolds(Optional(Range::ExtremeValues({7, 7}))));
  EXPECT_THAT(R.Extremes({{"N", 0}}), IsOkAndHolds(std::nullopt));  // Empty
}

TEST(RangeTest,
     RepeatedCallsToAtMostAndAtLeastExpressionVersionsShouldConsiderAll) {
  Range R;  // {y>=3x+1, y>=-x+3, y>=x+5, y<=x+15, y<=-x+15}

  MORIARTY_ASSERT_OK(R.AtLeast("-N + 3"));     // Valid: (-infinity, -1]
  MORIARTY_ASSERT_OK(R.AtLeast("N + 5"));      // Valid: [-1, 1.5]
  MORIARTY_ASSERT_OK(R.AtLeast("3 * N + 1"));  // Valid: [1.5, infinity)

  MORIARTY_ASSERT_OK(R.AtMost("-N + 15"));  // Valid: [0, infinity)
  MORIARTY_ASSERT_OK(R.AtMost("N + 15"));   // Valid: (-infinity, 0]

  // Left of valid range (-infinity, -6)
  EXPECT_THAT(R.Extremes({{"N", -10}}), IsOkAndHolds(std::nullopt));
  // Between -N + 3 and N + 15 [-6, -1]
  EXPECT_THAT(R.Extremes({{"N", -6}}),
              IsOkAndHolds(Optional(Range::ExtremeValues({9, 9}))));
  // Between N + 5 and N + 15 [-1, 0]
  EXPECT_THAT(R.Extremes({{"N", 0}}),
              IsOkAndHolds(Optional(Range::ExtremeValues({5, 15}))));
  // Between N + 5 and -N + 15 [0, 2]
  EXPECT_THAT(R.Extremes({{"N", 1}}),
              IsOkAndHolds(Optional(Range::ExtremeValues({6, 14}))));
  // Between 3 * N + 1 and -N + 15 [2, 3.5]
  EXPECT_THAT(R.Extremes({{"N", 3}}),
              IsOkAndHolds(Optional(Range::ExtremeValues({10, 12}))));
  // Right of valid range (3.5, infinity)
  EXPECT_THAT(R.Extremes({{"N", 4}}), IsOkAndHolds(std::nullopt));
}

TEST(RangeTest, AtMostShouldConsiderBothIntegerAndExpression) {
  Range R;

  R.AtLeast(-100);

  R.AtMost(10);                      // Upper bound for [3, infinity)
  MORIARTY_ASSERT_OK(R.AtMost("3 * N + 1"));  // Upper bound for (-infinity, 3]

  EXPECT_THAT(R.Extremes({{"N", 0}}),
              IsOkAndHolds(Optional(Range::ExtremeValues({-100, 1}))));
  EXPECT_THAT(R.Extremes({{"N", 3}}),
              IsOkAndHolds(Optional(Range::ExtremeValues({-100, 10}))));
  EXPECT_THAT(R.Extremes({{"N", 5}}),
              IsOkAndHolds(Optional(Range::ExtremeValues({-100, 10}))));
}

TEST(RangeTest, AtLeastShouldConsiderBothIntegerAndExpression) {
  Range R;
  R.AtMost(100);

  R.AtLeast(10);                      // Lower bound for [3, infinity)
  MORIARTY_ASSERT_OK(R.AtLeast("3 * N + 1"));  // Lower bound for (-infinity, 3]

  EXPECT_THAT(R.Extremes({{"N", 0}}),
              IsOkAndHolds(Optional(Range::ExtremeValues({10, 100}))));
  EXPECT_THAT(R.Extremes({{"N", 3}}),
              IsOkAndHolds(Optional(Range::ExtremeValues({10, 100}))));
  EXPECT_THAT(R.Extremes({{"N", 5}}),
              IsOkAndHolds(Optional(Range::ExtremeValues({16, 100}))));
}

TEST(RangeTest, ValidExpressionParses) {
  MORIARTY_EXPECT_OK(Range().AtMost("3 * N + M * N + 150"));
  MORIARTY_EXPECT_OK(Range().AtLeast("3 * N + M * N + 150"));
  MORIARTY_EXPECT_OK(Range().AtMost("-(3 * N)^150 + M * N + 150"));
  MORIARTY_EXPECT_OK(Range().AtLeast("(3 * N)^150 + M * N + 150"));
}

TEST(RangeTest, InvalidExpressionShouldFail) {
  EXPECT_THAT(Range().AtMost("-3 ^ (N + ) * N"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(Range().AtLeast("-3 ^ (N + ) * N"),
              StatusIs(absl::StatusCode::kInvalidArgument));

  EXPECT_THAT(Range().AtMost(""), StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(Range().AtLeast(""),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(RangeTest, NeededVariablesIsEmptyByDefault) {
  EXPECT_THAT(Range().NeededVariables(), IsOkAndHolds(IsEmpty()));
}

TEST(RangeTest, NeededVariablesRespondsToAtLeast) {
  Range r;
  MORIARTY_ASSERT_OK(r.AtLeast("N"));
  MORIARTY_ASSERT_OK(r.AtLeast("3 * M"));
  EXPECT_THAT(r.NeededVariables(),
              IsOkAndHolds(UnorderedElementsAre("N", "M")));
}

TEST(RangeTest, NeededVariablesRespondsToAtMost) {
  Range r;
  MORIARTY_ASSERT_OK(r.AtMost("N"));
  MORIARTY_ASSERT_OK(r.AtMost("3 * M"));
  EXPECT_THAT(r.NeededVariables(),
              IsOkAndHolds(UnorderedElementsAre("N", "M")));
}

TEST(RangeTest, NeededVariablesReturnsFailureOnInvalidParse) {
  Range r;
  ASSERT_THAT(r.AtMost("N +"), Not(IsOk()));
  EXPECT_THAT(r.NeededVariables(),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(RangeTest, NeededVariablesWorksAfterIntersect) {
  Range r1;
  Range r2;
  MORIARTY_ASSERT_OK(r1.AtMost("N"));
  MORIARTY_ASSERT_OK(r2.AtMost("M"));

  r1.Intersect(r2);
  EXPECT_THAT(r1.NeededVariables(),
              IsOkAndHolds(UnorderedElementsAre("N", "M")));
}

TEST(RangeTest, ToStringWithEmptyOrDefaultRangeShouldWork) {
  EXPECT_EQ(Range().ToString(), "(-inf, inf)");
  EXPECT_EQ(EmptyRange().ToString(), "(Empty Range)");
}

TEST(RangeTest, TwoSidedInequalitiesToStringShouldWork) {
  Range r1;
  r1.AtLeast(1);
  r1.AtMost(5);
  EXPECT_EQ(r1.ToString(), "[1, 5]");

  Range r2;
  MORIARTY_ASSERT_OK(r2.AtLeast("N"));
  r2.AtMost(5);
  EXPECT_EQ(r2.ToString(), "[N, 5]");

  Range r3;
  MORIARTY_ASSERT_OK(r3.AtLeast("N"));
  MORIARTY_ASSERT_OK(r3.AtMost("M"));
  EXPECT_EQ(r3.ToString(), "[N, M]");
}

TEST(RangeTest, OneSidedInequalitiesToStringShouldWork) {
  Range r1;
  r1.AtLeast(1);
  EXPECT_EQ(r1.ToString(), "[1, inf)");

  Range r2;
  r2.AtMost(5);
  EXPECT_EQ(r2.ToString(), "(-inf, 5]");

  Range r3;
  MORIARTY_ASSERT_OK(r3.AtMost("M"));
  EXPECT_EQ(r3.ToString(), "(-inf, M]");

  Range r4;
  MORIARTY_ASSERT_OK(r4.AtLeast("M"));
  EXPECT_EQ(r4.ToString(), "[M, inf)");
}

TEST(RangeTest, InequalitiesWithMultipleItemsShouldWork) {
  Range r1;
  r1.AtLeast(1);
  MORIARTY_ASSERT_OK(r1.AtLeast("3 * N"));
  EXPECT_EQ(r1.ToString(), "[max(1, 3 * N), inf)");

  Range r2;
  r2.AtMost(5);
  MORIARTY_ASSERT_OK(r2.AtMost("3 * N"));
  EXPECT_EQ(r2.ToString(), "(-inf, min(5, 3 * N)]");

  Range r3;
  MORIARTY_ASSERT_OK(r3.AtLeast("a"));
  MORIARTY_ASSERT_OK(r3.AtLeast("b"));
  MORIARTY_ASSERT_OK(r3.AtMost("c"));
  MORIARTY_ASSERT_OK(r3.AtMost("d"));
  EXPECT_EQ(r3.ToString(), "[max(a, b), min(c, d)]");

  Range r4;
  MORIARTY_ASSERT_OK(r4.AtLeast("a"));
  MORIARTY_ASSERT_OK(r4.AtMost("c"));
  MORIARTY_ASSERT_OK(r4.AtMost("d"));
  EXPECT_EQ(r4.ToString(), "[a, min(c, d)]");
}

}  // namespace
}  // namespace moriarty
