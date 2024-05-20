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

#include "src/librarian/size_property.h"

#include <cstdint>
#include <limits>
#include <optional>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "src/internal/range.h"
#include "src/util/status_macro/status_macros.h"
#include "src/util/test_status_macro/status_testutil.h"

namespace moriarty {

// Needed for testing comparisons.
bool operator==(const moriarty::Range& lhs, const moriarty::Range& rhs) {
  return lhs.Extremes() == rhs.Extremes();
}

namespace librarian {
namespace {

using ::testing::Optional;
using ::testing::Range;
using ::testing::Values;
using ::moriarty::StatusIs;

class SizePropertyTest : public testing::TestWithParam<int64_t> {};

absl::StatusOr<moriarty::Range::ExtremeValues> ExtractExtremes(
    moriarty::Range r) {
  MORIARTY_ASSIGN_OR_RETURN(
      std::optional<moriarty::Range::ExtremeValues> extremes, r.Extremes());
  if (!extremes.has_value())
    return absl::FailedPreconditionError("Range is empty.");
  return *extremes;
}

TEST_P(SizePropertyTest, MinimumShouldBeExtremal) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty::Range::ExtremeValues extremes,
      ExtractExtremes(GetMinRange(GetParam())));

  EXPECT_EQ(extremes.min, 1);
  EXPECT_EQ(extremes.max, 1);
}

TEST_P(SizePropertyTest, MaximumShouldBeExtremal) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty::Range::ExtremeValues extremes,
      ExtractExtremes(GetMaxRange(GetParam())));

  EXPECT_EQ(extremes.min, GetParam());
  EXPECT_EQ(extremes.max, GetParam());
}

TEST_P(SizePropertyTest, AllSizesShouldBeOrdered) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty::Range::ExtremeValues min,
      ExtractExtremes(GetMinRange(GetParam())));
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty::Range::ExtremeValues tiny,
      ExtractExtremes(GetTinyRange(GetParam())));
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty::Range::ExtremeValues small,
      ExtractExtremes(GetSmallRange(GetParam())));
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty::Range::ExtremeValues medium,
      ExtractExtremes(GetMediumRange(GetParam())));
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty::Range::ExtremeValues large,
      ExtractExtremes(GetLargeRange(GetParam())));
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty::Range::ExtremeValues huge,
      ExtractExtremes(GetHugeRange(GetParam())));
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty::Range::ExtremeValues max,
      ExtractExtremes(GetMaxRange(GetParam())));

  // Left endpoints are ordered.
  EXPECT_LE(min.min, tiny.min);
  EXPECT_LE(tiny.min, small.min);
  EXPECT_LE(small.min, medium.min);
  EXPECT_LE(medium.min, large.min);
  EXPECT_LE(large.min, huge.min);
  EXPECT_LE(huge.min, max.min);

  // Right endpoints are ordered.
  EXPECT_LE(min.max, tiny.max);
  EXPECT_LE(tiny.max, small.max);
  EXPECT_LE(small.max, medium.max);
  EXPECT_LE(medium.max, large.max);
  EXPECT_LE(large.max, huge.max);
  EXPECT_LE(huge.max, max.max);
}

TEST_P(SizePropertyTest, AllSizesShouldBeBetweenOneAndN) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty::Range::ExtremeValues min,
      ExtractExtremes(GetMinRange(GetParam())));
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty::Range::ExtremeValues tiny,
      ExtractExtremes(GetTinyRange(GetParam())));
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty::Range::ExtremeValues small,
      ExtractExtremes(GetSmallRange(GetParam())));
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty::Range::ExtremeValues medium,
      ExtractExtremes(GetMediumRange(GetParam())));
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty::Range::ExtremeValues large,
      ExtractExtremes(GetLargeRange(GetParam())));
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty::Range::ExtremeValues huge,
      ExtractExtremes(GetHugeRange(GetParam())));
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      moriarty::Range::ExtremeValues max,
      ExtractExtremes(GetMaxRange(GetParam())));

  // Left endpoints are >= 1.
  EXPECT_GE(min.min, 1);
  EXPECT_GE(tiny.min, 1);
  EXPECT_GE(small.min, 1);
  EXPECT_GE(medium.min, 1);
  EXPECT_GE(large.min, 1);
  EXPECT_GE(huge.min, 1);
  EXPECT_GE(max.min, 1);

  // Right endpoints are <= N.
  EXPECT_LE(min.max, GetParam());
  EXPECT_LE(tiny.max, GetParam());
  EXPECT_LE(small.max, GetParam());
  EXPECT_LE(medium.max, GetParam());
  EXPECT_LE(large.max, GetParam());
  EXPECT_LE(huge.max, GetParam());
  EXPECT_LE(max.max, GetParam());
}

TEST_P(SizePropertyTest, GetRangeShouldPassThroughToAppropriateFunc) {
  EXPECT_EQ(GetRange(CommonSize::kMin, GetParam()), GetMinRange(GetParam()));
  EXPECT_EQ(GetRange(CommonSize::kTiny, GetParam()), GetTinyRange(GetParam()));
  EXPECT_EQ(GetRange(CommonSize::kSmall, GetParam()),
            GetSmallRange(GetParam()));
  EXPECT_EQ(GetRange(CommonSize::kMedium, GetParam()),
            GetMediumRange(GetParam()));
  EXPECT_EQ(GetRange(CommonSize::kLarge, GetParam()),
            GetLargeRange(GetParam()));
  EXPECT_EQ(GetRange(CommonSize::kHuge, GetParam()), GetHugeRange(GetParam()));
  EXPECT_EQ(GetRange(CommonSize::kMax, GetParam()), GetMaxRange(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(SmallValues, SizePropertyTest, Range<int64_t>(1, 30));

INSTANTIATE_TEST_SUITE_P(LargerValues, SizePropertyTest,
                         Values(50, 100, 1000, 100000, 1000000,
                                std::numeric_limits<int>::max(),
                                std::numeric_limits<int64_t>::max()));

TEST(SizePropertyTest, ZeroNShouldGiveEmptyRange) {
  EXPECT_THAT(ExtractExtremes(GetMinRange(0)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(ExtractExtremes(GetTinyRange(0)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(ExtractExtremes(GetSmallRange(0)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(ExtractExtremes(GetMediumRange(0)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(ExtractExtremes(GetLargeRange(0)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(ExtractExtremes(GetHugeRange(0)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(ExtractExtremes(GetMaxRange(0)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(SizePropertyTest, NegativeNShouldGiveEmptyRange) {
  EXPECT_THAT(ExtractExtremes(GetMinRange(-5)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(ExtractExtremes(GetTinyRange(-5)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(ExtractExtremes(GetSmallRange(-5)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(ExtractExtremes(GetMediumRange(-5)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(ExtractExtremes(GetLargeRange(-5)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(ExtractExtremes(GetHugeRange(-5)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(ExtractExtremes(GetMaxRange(-5)),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(SizePropertyTest, MergeSizesWithAnyShouldTakeOtherOption) {
  EXPECT_THAT(MergeSizes(CommonSize::kAny, CommonSize::kAny),
              Optional(CommonSize::kAny));

  EXPECT_THAT(MergeSizes(CommonSize::kAny, CommonSize::kMin),
              Optional(CommonSize::kMin));
  EXPECT_THAT(MergeSizes(CommonSize::kAny, CommonSize::kTiny),
              Optional(CommonSize::kTiny));
  EXPECT_THAT(MergeSizes(CommonSize::kAny, CommonSize::kSmall),
              Optional(CommonSize::kSmall));
  EXPECT_THAT(MergeSizes(CommonSize::kAny, CommonSize::kMedium),
              Optional(CommonSize::kMedium));
  EXPECT_THAT(MergeSizes(CommonSize::kAny, CommonSize::kLarge),
              Optional(CommonSize::kLarge));
  EXPECT_THAT(MergeSizes(CommonSize::kAny, CommonSize::kHuge),
              Optional(CommonSize::kHuge));
  EXPECT_THAT(MergeSizes(CommonSize::kAny, CommonSize::kMax),
              Optional(CommonSize::kMax));

  EXPECT_THAT(MergeSizes(CommonSize::kMin, CommonSize::kAny),
              Optional(CommonSize::kMin));
  EXPECT_THAT(MergeSizes(CommonSize::kTiny, CommonSize::kAny),
              Optional(CommonSize::kTiny));
  EXPECT_THAT(MergeSizes(CommonSize::kSmall, CommonSize::kAny),
              Optional(CommonSize::kSmall));
  EXPECT_THAT(MergeSizes(CommonSize::kMedium, CommonSize::kAny),
              Optional(CommonSize::kMedium));
  EXPECT_THAT(MergeSizes(CommonSize::kLarge, CommonSize::kAny),
              Optional(CommonSize::kLarge));
  EXPECT_THAT(MergeSizes(CommonSize::kHuge, CommonSize::kAny),
              Optional(CommonSize::kHuge));
  EXPECT_THAT(MergeSizes(CommonSize::kMax, CommonSize::kAny),
              Optional(CommonSize::kMax));
}

TEST(SizePropertyTest, MergeSizesWithThemselvesShouldReturn) {
  EXPECT_THAT(MergeSizes(CommonSize::kAny, CommonSize::kAny),
              Optional(CommonSize::kAny));

  EXPECT_THAT(MergeSizes(CommonSize::kMin, CommonSize::kMin),
              Optional(CommonSize::kMin));
  EXPECT_THAT(MergeSizes(CommonSize::kTiny, CommonSize::kTiny),
              Optional(CommonSize::kTiny));
  EXPECT_THAT(MergeSizes(CommonSize::kSmall, CommonSize::kSmall),
              Optional(CommonSize::kSmall));
  EXPECT_THAT(MergeSizes(CommonSize::kMedium, CommonSize::kMedium),
              Optional(CommonSize::kMedium));
  EXPECT_THAT(MergeSizes(CommonSize::kLarge, CommonSize::kLarge),
              Optional(CommonSize::kLarge));
  EXPECT_THAT(MergeSizes(CommonSize::kHuge, CommonSize::kHuge),
              Optional(CommonSize::kHuge));
  EXPECT_THAT(MergeSizes(CommonSize::kMax, CommonSize::kMax),
              Optional(CommonSize::kMax));
}

TEST(SizePropertyTest, MergeSizesWithNonRelatedSizesReturnsNothing) {
  // (small-ish, _____) pairs
  EXPECT_EQ(MergeSizes(CommonSize::kSmall, CommonSize::kMedium), std::nullopt);
  EXPECT_EQ(MergeSizes(CommonSize::kTiny, CommonSize::kMax), std::nullopt);

  // (medium-ish, _____) pairs
  EXPECT_EQ(MergeSizes(CommonSize::kMedium, CommonSize::kMin), std::nullopt);
  EXPECT_EQ(MergeSizes(CommonSize::kMedium, CommonSize::kHuge), std::nullopt);

  // (large-ish, _____) pairs
  EXPECT_EQ(MergeSizes(CommonSize::kMax, CommonSize::kMin), std::nullopt);
  EXPECT_EQ(MergeSizes(CommonSize::kHuge, CommonSize::kMedium), std::nullopt);
}

TEST(SizePropertyTest, MergeSizesWithRelatedSizesReturnsSmallerVersion) {
  // small-ish pairs
  EXPECT_THAT(MergeSizes(CommonSize::kSmall, CommonSize::kMin),
              Optional(CommonSize::kMin));
  EXPECT_THAT(MergeSizes(CommonSize::kTiny, CommonSize::kSmall),
              Optional(CommonSize::kTiny));

  // medium-ish pairs
  EXPECT_THAT(MergeSizes(CommonSize::kMedium, CommonSize::kMedium),
              Optional(CommonSize::kMedium));

  // large-ish pairs
  EXPECT_THAT(MergeSizes(CommonSize::kHuge, CommonSize::kMax),
              Optional(CommonSize::kMax));
  EXPECT_THAT(MergeSizes(CommonSize::kHuge, CommonSize::kLarge),
              Optional(CommonSize::kHuge));
}

TEST(SizePropertyTest, ToStringShouldWork) {
  EXPECT_EQ(ToString(CommonSize::kMin), "Min");
  EXPECT_EQ(ToString(CommonSize::kTiny), "Tiny");
  EXPECT_EQ(ToString(CommonSize::kSmall), "Small");
  EXPECT_EQ(ToString(CommonSize::kMedium), "Medium");
  EXPECT_EQ(ToString(CommonSize::kLarge), "Large");
  EXPECT_EQ(ToString(CommonSize::kHuge), "Huge");
  EXPECT_EQ(ToString(CommonSize::kMax), "Max");
  EXPECT_EQ(ToString(CommonSize::kAny), "Any");
}

}  // namespace
}  // namespace librarian
}  // namespace moriarty
