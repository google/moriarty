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

#include "src/variables/mtuple.h"

#include <cstdint>
#include <tuple>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/test_utils.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty {
namespace {

using ::moriarty_testing::Generate;
using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::GenerateSameValues;
using ::moriarty_testing::IsNotSatisfiedWith;
using ::moriarty_testing::IsSatisfiedWith;
using ::moriarty_testing::Print;
using ::moriarty_testing::Read;
using ::testing::AllOf;
using ::testing::FieldsAre;
using ::testing::Ge;
using ::testing::Le;
using ::testing::SizeIs;
using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;

TEST(MTupleTest, TypenameIsCorrect) {
  EXPECT_EQ(MTuple(MInteger()).Typename(), "MTuple<MInteger>");
  EXPECT_EQ(MTuple(MInteger(), MInteger()).Typename(),
            "MTuple<MInteger, MInteger>");
  EXPECT_EQ(MTuple(MInteger(), MInteger(), MInteger()).Typename(),
            "MTuple<MInteger, MInteger, MInteger>");
  EXPECT_EQ(MTuple(MInteger(), MTuple(MInteger(), MInteger())).Typename(),
            "MTuple<MInteger, MTuple<MInteger, MInteger>>");
}

TEST(MTupleTest, PrintShouldSucceed) {
  EXPECT_THAT(Print(MTuple(MInteger(), MInteger()), {1, 2}),
              IsOkAndHolds("1 2"));
  EXPECT_THAT(
      Print(MTuple(MInteger(), MInteger(), MTuple(MInteger(), MInteger())),
            {1, 2, {5, 6}}),
      IsOkAndHolds("1 2 5 6"));
}

TEST(MTupleTest, TypicalReadCaseWorks) {
  EXPECT_THAT(Read(MTuple(MInteger(), MInteger(), MInteger()), "1 22 333"),
              IsOkAndHolds(FieldsAre(1, 22, 333)));
  EXPECT_THAT(Read(MTuple(MInteger(), MString(), MInteger()), "1 twotwo 333"),
              IsOkAndHolds(FieldsAre(1, "twotwo", 333)));
}

TEST(MTupleTest, ReadingTheWrongTypeShouldFail) {
  // MString where MInteger should be
  EXPECT_THAT(Read(MTuple(MInteger(), MInteger(), MInteger()), "1 two 3"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  // Not enough input
  EXPECT_THAT(Read(MTuple(MInteger(), MInteger(), MInteger()), "1 22"),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(MTupleTest, SimpleGenerateCaseWorks) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      (std::tuple<int64_t, int64_t> t),
      Generate(MTuple(MInteger().Between(1, 10), MInteger().Between(30, 40))));

  EXPECT_THAT(std::get<0>(t), AllOf(Ge(1), Le(10)));
  EXPECT_THAT(std::get<1>(t), AllOf(Ge(30), Le(40)));
}

TEST(MTupleTest, NestedMTupleWorksWithGenerate) {
  MTuple A =
      MTuple(MInteger().Between(0, 9),
             MTuple(MInteger().Between(10, 99), MInteger().Between(100, 999)));

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      (std::tuple<int64_t, std::tuple<int64_t, int64_t>> t), Generate(A));
  auto [x, yz] = t;
  auto [y, z] = yz;

  EXPECT_THAT(x, AllOf(Ge(0), Le(9)));
  EXPECT_THAT(y, AllOf(Ge(10), Le(99)));
  EXPECT_THAT(z, AllOf(Ge(100), Le(999)));
}

TEST(MTupleTest, GenerateShouldSuccessfullyComplete) {
  MORIARTY_EXPECT_OK(Generate(MTuple<MInteger>(MInteger())));
  MORIARTY_EXPECT_OK(
      Generate(MTuple<MInteger, MInteger, MTuple<MInteger, MInteger>,
                      MTuple<MInteger>>()));
}

TEST(MTupleTest, MergeFromCorrectlyMergesEachArgument) {
  {
    MTuple a = MTuple(MInteger().Between(1, 10), MInteger().Between(20, 30));
    MTuple b = MTuple(MInteger().Between(5, 15), MInteger().Between(22, 25));
    MTuple c = MTuple(MInteger().Between(5, 10), MInteger().Between(22, 25));

    EXPECT_TRUE(GenerateSameValues(a.MergeFrom(b), c));
  }

  {
    MTuple a =
        MTuple(MInteger().Between(1, 10),
               MTuple(MInteger().Between(20, 30), MInteger().Between(40, 50)));
    MTuple b =
        MTuple(MInteger().Between(5, 15),
               MTuple(MInteger().Between(22, 25), MInteger().Between(34, 45)));
    MTuple c =
        MTuple(MInteger().Between(5, 10),
               MTuple(MInteger().Between(22, 25), MInteger().Between(40, 45)));

    EXPECT_TRUE(GenerateSameValues(a.MergeFrom(b), c));
  }
}

TEST(MTupleTest, OfShouldMergeIndependentArguments) {
  {
    MTuple a = MTuple(MInteger().Between(1, 10), MInteger().Between(20, 30));
    MTuple b = MTuple(MInteger().Between(5, 10), MInteger().Between(20, 30));
    MInteger restrict_a = MInteger().Between(5, 15);

    EXPECT_TRUE(GenerateSameValues(a.Of<0>(restrict_a), b));
  }

  {
    MTuple a =
        MTuple(MInteger().Between(1, 10),
               MTuple(MInteger().Between(20, 30), MInteger().Between(40, 50)));
    MTuple b =
        MTuple(MInteger().Between(1, 10),
               MTuple(MInteger().Between(22, 30), MInteger().Between(40, 50)));
    MInteger restrict_a = MInteger().Between(22, 40);

    EXPECT_TRUE(GenerateSameValues(a.Of<1>(MTuple(restrict_a, MInteger())), b));
  }
}

TEST(MTupleTest, SatisfiesConstraintsWorksForValid) {
  {  // Simple
    MTuple constraints =
        MTuple(MInteger().Between(100, 111), MInteger().Between(200, 222));
    EXPECT_THAT(constraints,
                IsSatisfiedWith(std::tuple<int64_t, int64_t>({105, 205})));
  }

  {  // Nested [1, [2, 3]]
    MTuple constraints = MTuple(
        MInteger().Between(100, 111),
        MTuple(MInteger().Between(200, 222), MInteger().Between(300, 333)));
    using TupleType = std::tuple<int64_t, std::tuple<int64_t, int64_t>>;
    EXPECT_THAT(constraints, IsSatisfiedWith(TupleType({105, {205, 305}})));
  }

  {  // Very nested [1, [[2, 3], [4, 5, 6]]]
    MTuple constraints = MTuple(
        MInteger().Between(100, 111),
        MTuple(
            MTuple(MInteger().Between(200, 222), MInteger().Between(300, 333)),
            MTuple(MInteger().Between(400, 444), MInteger().Between(500, 555),
                   MInteger().Between(600, 666))));
    using TupleType =
        std::tuple<int64_t, std::tuple<std::tuple<int64_t, int64_t>,
                                       std::tuple<int64_t, int64_t, int64_t>>>;
    EXPECT_THAT(constraints,
                IsSatisfiedWith(TupleType{105, {{205, 305}, {405, 505, 605}}}));
  }
}

TEST(MTupleTest, SatisfiesConstraintsWorksForInvalid) {
  {  // Simple
    MTuple constraints =
        MTuple(MInteger().Between(100, 111), MInteger().Between(200, 222));
    using Type = std::tuple<int64_t, int64_t>;

    EXPECT_THAT(constraints, IsNotSatisfiedWith(Type{0, 205},
                                                "range"));  // first
    EXPECT_THAT(constraints,
                IsNotSatisfiedWith(Type{105, 0}, "range"));  // second
    EXPECT_THAT(constraints, IsNotSatisfiedWith(Type{0, 0}, "range"));  // both
  }

  {  // Nested [1, [2, 3]]
    MTuple constraints = MTuple(
        MInteger().Between(100, 111),
        MTuple(MInteger().Between(200, 222), MInteger().Between(300, 333)));
    using Type = std::tuple<int64_t, std::tuple<int64_t, int64_t>>;

    EXPECT_THAT(constraints,
                IsNotSatisfiedWith(Type{0, {205, 305}}, "range"));  // first
    EXPECT_THAT(constraints, IsNotSatisfiedWith(Type{105, {0, 305}},
                                                "range"));  // second.first
    EXPECT_THAT(constraints, IsNotSatisfiedWith(Type{105, {205, 0}},
                                                "range"));  // second.Second
    EXPECT_THAT(constraints,
                IsNotSatisfiedWith(Type{105, {0, 0}}, "range"));  // second.both
  }
}

TEST(MTupleTest, PropertiesArePassedToEachComponent) {
  // The exact value of 100 and 10^6 here are fuzzy and may need to change with
  // the meaning of "small".
  EXPECT_THAT(
      MTuple(MInteger().Between(1, 1000), MInteger().Between(1, 1000))
          .WithKnownProperty({.category = "size", .descriptor = "small"}),
      GeneratedValuesAre(FieldsAre(Le(100L), Le(100L))));
}

TEST(MTupleTest, PropertiesCanBePassedToMultipleDifferentTypes) {
  // The exact value of 100 here is fuzzy and may need to change with the
  // meaning of "small".
  EXPECT_THAT(
      MTuple(MInteger().Between(1, 1000),
             MString()
                 .WithAlphabet(MString::kNumbers)
                 .OfLength(MInteger().Between(1, 1000)))
          .WithKnownProperty({.category = "size", .descriptor = "small"}),
      GeneratedValuesAre(FieldsAre(Le(100L), SizeIs(Le(100L)))));
}

}  // namespace
}  // namespace moriarty
