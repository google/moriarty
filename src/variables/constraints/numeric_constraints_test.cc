/*
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/variables/constraints/numeric_constraints.h"

#include "gtest/gtest.h"
#include "src/internal/range.h"

namespace moriarty {
namespace {

using ::moriarty::AtLeast;
using ::moriarty::AtMost;
using ::moriarty::Between;

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
           << "first range is empty, second is " << "[" << (*extremes2)->min
           << ", " << (*extremes2)->max << "]";

  if (!(*extremes2).has_value())
    return testing::AssertionFailure()
           << "first range is " << "[" << (*extremes1)->min << ", "
           << (*extremes1)->max << "]" << ", second is empty";

  if ((*extremes1) == (*extremes2))
    return testing::AssertionSuccess()
           << "both ranges are [" << (*extremes1)->min << ", "
           << (*extremes1)->max;

  return testing::AssertionFailure()
         << "are not equal " << "[" << (*extremes1)->min << ", "
         << (*extremes1)->max << "]" << " vs " << "[" << (*extremes2)->min
         << ", " << (*extremes2)->max << "]";
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

TEST(NumericConstraintsTest, BetweenWorks) {
  EXPECT_TRUE(EqualRanges(Between(10, 20).GetRange(), Range(10, 20)));
  EXPECT_TRUE(EqualRanges(Between("10", 20).GetRange(), Range(10, 20)));
  EXPECT_TRUE(EqualRanges(Between(10, "20").GetRange(), Range(10, 20)));
  EXPECT_TRUE(EqualRanges(Between("10", "20").GetRange(), Range(10, 20)));
  EXPECT_TRUE(IsEmptyRange(Between(0, -5).GetRange()));
}

TEST(NumericConstraintsTest, AtLeastWorks) {
  Range expected;
  expected.AtLeast(20);
  EXPECT_TRUE(EqualRanges(AtLeast(20).GetRange(), expected));
  EXPECT_TRUE(EqualRanges(AtLeast("20").GetRange(), expected));
}

TEST(NumericConstraintsTest, AtMostWorks) {
  Range expected;
  expected.AtMost(23);
  EXPECT_TRUE(EqualRanges(AtMost(23).GetRange(), expected));
  EXPECT_TRUE(EqualRanges(AtMost("23").GetRange(), expected));
}

}  // namespace
}  // namespace moriarty
