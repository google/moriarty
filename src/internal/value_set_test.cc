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

#include "src/internal/value_set.h"

#include <any>
#include <cstdint>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/marray.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;
using ::moriarty_testing::IsValueNotFound;
using ::testing::AnyWith;
using ::testing::StrEq;

TEST(ValueSetTest, SimpleGetAndSetWorks) {
  ValueSet value_set;
  value_set.Set<MInteger>("x", 5);
  EXPECT_THAT(value_set.Get<MInteger>("x"), IsOkAndHolds(5));
}

TEST(ValueSetTest, OverwritingTheSameVariableShouldReplaceTheValue) {
  ValueSet value_set;
  value_set.Set<MInteger>("x", 5);
  EXPECT_THAT(value_set.Get<MInteger>("x"), IsOkAndHolds(5));
  value_set.Set<MInteger>("x", 10);
  EXPECT_THAT(value_set.Get<MInteger>("x"), IsOkAndHolds(10));
}

TEST(ValueSetTest, MultipleVariablesShouldWork) {
  ValueSet value_set;
  value_set.Set<MInteger>("x", 5);
  value_set.Set<MInteger>("y", 10);
  EXPECT_THAT(value_set.Get<MInteger>("x"), IsOkAndHolds(5));
  EXPECT_THAT(value_set.Get<MInteger>("y"), IsOkAndHolds(10));
}

TEST(ValueSetTest, RequestingANonExistentVariableShouldThrow) {
  ValueSet value_set;
  EXPECT_THAT(value_set.Get<MInteger>("x"), IsValueNotFound("x"));

  value_set.Set<MInteger>("x", 5);
  EXPECT_THAT(value_set.Get<MInteger>("y"), IsValueNotFound("y"));
}

TEST(ValueSetTest, RequestingTheWrongTypeShouldThrow) {
  ValueSet value_set;
  value_set.Set<MInteger>("x", 10);
  EXPECT_THAT(value_set.Get<MString>("x"),
              StatusIs(absl::StatusCode::kFailedPrecondition));

  value_set.Set<MString>("x", "hello");
  EXPECT_THAT(value_set.Get<MInteger>("x"),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_THAT(value_set.Get<MString>("x"), IsOkAndHolds(StrEq("hello")));
}

TEST(ValueSetTest, SimpleUnsafeGetWorks) {
  ValueSet value_set;
  value_set.Set<MInteger>("x", 5);

  EXPECT_THAT(value_set.UnsafeGet("x"), IsOkAndHolds(AnyWith<int64_t>(5)));
}

TEST(ValueSetTest, UnsafeGetOverwritingTheSameVariableShouldReplaceTheValue) {
  ValueSet value_set;

  value_set.Set<MInteger>("x", 5);
  absl::StatusOr<std::any> value = value_set.UnsafeGet("x");
  MORIARTY_EXPECT_OK(value);
  EXPECT_EQ(std::any_cast<int64_t>(value.value()), 5);
  value_set.Set<MString>("x", "hi");
  value = value_set.UnsafeGet("x");
  MORIARTY_EXPECT_OK(value);
  EXPECT_EQ(std::any_cast<std::string>(value.value()), "hi");
}

TEST(ValueSetTest, UnsafeGetRequestingANonExistentVariableFails) {
  ValueSet value_set;
  EXPECT_THAT(value_set.UnsafeGet("x"), IsValueNotFound("x"));

  value_set.Set<MInteger>("x", 5);
  EXPECT_THAT(value_set.UnsafeGet("y"), IsValueNotFound("y"));
}

TEST(ValueSetTest, ContainsShouldWork) {
  ValueSet value_set;
  EXPECT_FALSE(value_set.Contains("x"));
  value_set.Set<MInteger>("x", 5);
  EXPECT_TRUE(value_set.Contains("x"));
  value_set.Set<MInteger>("x", 10);
  EXPECT_TRUE(value_set.Contains("x"));

  EXPECT_FALSE(value_set.Contains("y"));
}

TEST(ValueSetTest, GetApproximateSizeWorksForIntegers) {
  ValueSet value_set;
  value_set.Set<MInteger>("x", 5);
  EXPECT_EQ(value_set.GetApproximateSize(), 1);
  value_set.Set<MInteger>("y", 10);
  EXPECT_EQ(value_set.GetApproximateSize(), 2);
}

TEST(ValueSetTest, GetApproximateSizeWorksForStrings) {
  ValueSet value_set;
  value_set.Set<MString>("x", "hello");
  EXPECT_EQ(value_set.GetApproximateSize(), 5);
  value_set.Set<MString>("y", "world!");
  EXPECT_EQ(value_set.GetApproximateSize(), 11);
}

TEST(ValueSetTest, GetApproximateSizeWorksForVectors) {
  ValueSet value_set;
  value_set.Set<MArray<MInteger>>("x", {1, 2, 3});
  EXPECT_EQ(value_set.GetApproximateSize(), 3);
  value_set.Set<MArray<MInteger>>("y", {4, 5, 6, 7});
  EXPECT_EQ(value_set.GetApproximateSize(), 7);
}

TEST(ValueSetTest, GetApproximateSizeWorksForNestedVectorsAndMixedTypes) {
  ValueSet value_set;
  value_set.Set<MArray<MArray<MString>>>(
      "x", {{"hello", "world"}, {"how", "are", "you?"}});
  EXPECT_EQ(value_set.GetApproximateSize(), 5 + 5 + 3 + 3 + 4);
  value_set.Set<MInteger>("y", 10);
  EXPECT_EQ(value_set.GetApproximateSize(), 5 + 5 + 3 + 3 + 4 + 1);
}

TEST(ValueSetTest, EraseRemovesTheValueFromTheSet) {
  ValueSet value_set;
  value_set.Set<MInteger>("x", 5);
  value_set.Erase("x");
  EXPECT_FALSE(value_set.Contains("x"));
}

TEST(ValueSetTest, ErasingANonExistentVariableSucceeds) {
  ValueSet value_set;
  value_set.Erase("x");
  EXPECT_FALSE(value_set.Contains("x"));
}

TEST(ValueSetTest, ErasingMultipleTimesSucceeds) {
  ValueSet value_set;
  value_set.Set<MInteger>("x", 5);

  value_set.Erase("x");
  EXPECT_FALSE(value_set.Contains("x"));
  value_set.Erase("x");
  EXPECT_FALSE(value_set.Contains("x"));
}

TEST(ValueSetTest, ErasingVariableLeavesOthersAlone) {
  ValueSet value_set;
  value_set.Set<MInteger>("x", 5);
  value_set.Set<MInteger>("y", 5);

  value_set.Erase("x");
  EXPECT_TRUE(value_set.Contains("y"));
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
