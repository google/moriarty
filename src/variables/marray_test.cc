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

#include "src/variables/marray.h"

#include <cstdint>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "src/librarian/io_config.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/test_utils.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/container_constraints.h"
#include "src/variables/constraints/io_constraints.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"
#include "src/variables/mtuple.h"

namespace moriarty {
namespace {

using ::moriarty_testing::AllGenerateSameValues;
using ::moriarty_testing::Context;
using ::moriarty_testing::Generate;
using ::moriarty_testing::GenerateDifficultInstancesValues;
using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::GenerateSameValues;
using ::moriarty_testing::IsNotSatisfiedWith;
using ::moriarty_testing::IsSatisfiedWith;
using ::moriarty_testing::Print;
using ::moriarty_testing::Read;
using ::testing::AllOf;
using ::testing::Each;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::IsSupersetOf;
using ::testing::Le;
using ::testing::Not;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;
using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;

TEST(MArrayTest, TypenameIsCorrect) {
  EXPECT_EQ(MArray(MInteger()).Typename(), "MArray<MInteger>");
  EXPECT_EQ(NestedMArray(MArray(MInteger())).Typename(),
            "MArray<MArray<MInteger>>");
  EXPECT_EQ(MArray(MTuple(MInteger(), MInteger(), MInteger())).Typename(),
            "MArray<MTuple<MInteger, MInteger, MInteger>>");
  EXPECT_EQ(
      MArray(MTuple(MInteger(), MTuple(MInteger(), MInteger()))).Typename(),
      "MArray<MTuple<MInteger, MTuple<MInteger, MInteger>>>");
}

TEST(MArrayTest, PrintShouldSucceed) {
  EXPECT_THAT(Print(MArray(MInteger()), {1, 2, 3}), IsOkAndHolds("1 2 3"));
}

TEST(MArrayTest, TypicalReadCaseWorks) {
  EXPECT_THAT(Read(MArray<MInteger>().OfLength(6), "1 2 3 4 5 6"),
              IsOkAndHolds(ElementsAre(1, 2, 3, 4, 5, 6)));
  EXPECT_THAT(
      Read(MArray<MString>().OfLength(5), "Hello, World! Welcome to Moriarty!"),
      IsOkAndHolds(
          ElementsAre("Hello,", "World!", "Welcome", "to", "Moriarty!")));
}

TEST(MArrayTest, ReadingTheWrongLengthOfMArrayShouldFail) {
  EXPECT_THAT(Read(MArray(MInteger()).OfLength(6), "1 2 3 4"),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(MArrayTest, SimpleGenerateCaseWorks) {
  EXPECT_THAT(Generate(MArray(MInteger().Between(1, 10)).OfLength(5, 50)),
              IsOkAndHolds(AllOf(SizeIs(AllOf(Ge(5), Le(50))),
                                 Each(AllOf(Ge(1), Le(10))))));
}

TEST(MArrayTest, NestedMArrayWorks) {
  EXPECT_THAT(
      Generate(NestedMArray(MArray(MInteger().Between(1, 10)).OfLength(3))
                   .OfLength(5)),
      IsOkAndHolds(AllOf(SizeIs(5),
                         Each(AllOf(SizeIs(3), Each(AllOf(Ge(1), Le(10))))))));
  {
    // Silly deeply nested case
    MArray A =
        NestedMArray(
            NestedMArray(
                NestedMArray(
                    NestedMArray(
                        MArray(MInteger().Between(1, 100)).OfLength(1, 3))
                        .OfLength(2, 4))
                    .OfLength(3, 5))
                .OfLength(4, 6))
            .OfLength(5, 7);

    MORIARTY_ASSERT_OK_AND_ASSIGN(auto a, Generate(A));

    EXPECT_THAT(a, SizeIs(AllOf(Ge(5), Le(7))));
    EXPECT_THAT(a, Each(SizeIs(AllOf(Ge(4), Le(6)))));
    EXPECT_THAT(a, Each(Each(SizeIs(AllOf(Ge(3), Le(5))))));
    EXPECT_THAT(a, Each(Each(Each(SizeIs(AllOf(Ge(2), Le(4)))))));
    EXPECT_THAT(a, Each(Each(Each(Each(SizeIs(AllOf(Ge(1), Le(3))))))));
    EXPECT_THAT(a, Each(Each(Each(Each(Each(AllOf(Ge(1), Le(100))))))));
  }
}

TEST(MArrayTest, GenerateShouldSuccessfullyComplete) {
  MORIARTY_EXPECT_OK(Generate(MArray<MInteger>(MInteger()).OfLength(4, 10)));
  MORIARTY_EXPECT_OK(Generate(MArray(MInteger()).OfLength(4, 10)));
  MORIARTY_EXPECT_OK(
      Generate(MArray(MInteger().Between(1, 10)).OfLength(4, 10)));
}

TEST(MArrayTest, RepeatedOfLengthCallsShouldBeIntersectedTogether) {
  auto gen_given_length1 = [](int lo, int hi) {
    return MArray(MInteger().Between(1, 10)).OfLength(lo, hi);
  };
  auto gen_given_length2 = [](int lo1, int hi1, int lo2, int hi2) {
    return MArray(MInteger().Between(1, 10))
        .OfLength(lo1, hi1)
        .OfLength(lo2, hi2);
  };

  // All possible valid intersections
  EXPECT_TRUE(
      GenerateSameValues(gen_given_length2(0, 30, 1, 10),
                         gen_given_length1(1, 10)));  // First is superset
  EXPECT_TRUE(
      GenerateSameValues(gen_given_length2(1, 10, 0, 30),
                         gen_given_length1(1, 10)));  // Second is superset
  EXPECT_TRUE(
      GenerateSameValues(gen_given_length2(0, 10, 1, 30),
                         gen_given_length1(1, 10)));  // First on the left
  EXPECT_TRUE(
      GenerateSameValues(gen_given_length2(1, 30, 0, 10),
                         gen_given_length1(1, 10)));  // First on the right
  EXPECT_TRUE(GenerateSameValues(gen_given_length2(1, 8, 8, 10),
                                 gen_given_length1(8, 8)));  // Singleton Range
}

TEST(MArrayTest, InvalidLengthShouldFail) {
  EXPECT_THAT(Generate(MArray(MInteger()).OfLength(-1)),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Valid range is empty")));
  EXPECT_THAT(Generate(MArray(MInteger()).OfLength(MInteger().Between(0, -1))),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Valid range is empty")));
}

TEST(MArrayTest, LengthZeroProcudesTheEmptyArray) {
  EXPECT_THAT(Generate(MArray(MInteger()).OfLength(0)),
              IsOkAndHolds(IsEmpty()));
}

TEST(MArrayTest, MergeFromCorrectlyMergesOnLength) {
  // The alphabet is irrelevant for the tests
  auto get_arr = []() { return MArray(MInteger().Between(1, 10)); };

  // All possible valid intersection of length
  EXPECT_TRUE(GenerateSameValues(
      get_arr().OfLength(0, 30).MergeFrom(get_arr().OfLength(1, 10)),
      get_arr().OfLength(1, 10)));  // First is a superset
  EXPECT_TRUE(GenerateSameValues(
      get_arr().OfLength(0, 30).MergeFrom(get_arr().OfLength(1, 10)),
      get_arr().OfLength(1, 10)));  // First is a superset
  EXPECT_TRUE(GenerateSameValues(
      get_arr().OfLength(1, 10).MergeFrom(get_arr().OfLength(0, 30)),
      get_arr().OfLength(1, 10)));  // Second is a superset
  EXPECT_TRUE(GenerateSameValues(
      get_arr().OfLength(0, 10).MergeFrom(get_arr().OfLength(1, 30)),
      get_arr().OfLength(1, 10)));  // First on left
  EXPECT_TRUE(GenerateSameValues(
      get_arr().OfLength(1, 30).MergeFrom(get_arr().OfLength(0, 10)),
      get_arr().OfLength(1, 10)));  // First on right
  EXPECT_TRUE(GenerateSameValues(
      get_arr().OfLength(1, 8).MergeFrom(get_arr().OfLength(8, 10)),
      get_arr().OfLength(8)));  // Singleton range

  EXPECT_THAT(
      Generate(get_arr().OfLength(1, 6).MergeFrom(get_arr().OfLength(10, 20))),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               HasSubstr("range is empty")));
}

TEST(MArrayTest, MergeFromCorrectlyMergesElementConstraints) {
  auto int_array = [](int lo, int hi) {
    return MArray(MInteger().Between(lo, hi)).OfLength(20);
  };

  EXPECT_TRUE(GenerateSameValues(int_array(1, 10).MergeFrom(int_array(6, 12)),
                                 int_array(6, 10)));

  EXPECT_THAT(Generate(int_array(1, 6).MergeFrom(int_array(10, 20))),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("range is empty")));
}

TEST(MArrayTest, LengthIsSatisfied) {
  // Includes small and large ranges
  for (int size = 0; size < 40; size++) {
    EXPECT_THAT(Generate(MArray(MInteger()).OfLength(size / 2, size)),
                IsOkAndHolds(SizeIs(AllOf(Ge(size / 2), Le(size)))));
  }
  for (int size = 900; size < 940; size++) {
    EXPECT_THAT(Generate(MArray(MInteger()).OfLength(size / 2, size)),
                IsOkAndHolds(SizeIs(AllOf(Ge(size / 2), Le(size)))));
  }
}

TEST(MArrayTest, SatisfiesConstraintsChecksMArrayContentsProperly) {
  EXPECT_THAT(MArray(MInteger().Between(1, 10)),
              IsSatisfiedWith(std::vector<int64_t>({5, 6, 1, 4, 3, 9, 10})));

  EXPECT_THAT(MArray(MInteger().Between(1, 10)),
              IsNotSatisfiedWith(std::vector<int64_t>({5, 6, 7, 8, 9, 10, 11}),
                                 "element 6"));
  EXPECT_THAT(MArray(MInteger().Between(1, 10)),
              IsNotSatisfiedWith(std::vector<int64_t>({5, 6, 0, 8, 9, 10}),
                                 "element 2"));
}

TEST(MArrayTest, SatisfiesConstraintsChecksMArraySizeWhenItIsVariable) {
  EXPECT_THAT(MArray(MInteger()).OfLength("N"),
              IsSatisfiedWith(std::vector<int64_t>({10, 20, 30}),
                              Context().WithValue<MInteger>("N", 3)));
}

TEST(MArrayTest,
     SatisfiesConstraintsFailsIfLengthDependsOnAVariableAndIsNotKnown) {
  // If N is definitely unique, it should pass.
  EXPECT_THAT(MArray(MInteger()).OfLength(3),
              IsSatisfiedWith(std::vector<int64_t>({10, 20, 30})));
  EXPECT_THAT(MArray(MInteger()).OfLength("N"),
              IsSatisfiedWith(std::vector<int64_t>({10, 20, 30}),
                              Context().WithVariable("N", MInteger().Is(3))));

  // If there is a unique value, then we might be able to determine the length.
  EXPECT_THAT(
      MArray(MInteger()).OfLength("N"),
      IsSatisfiedWith(std::vector<int64_t>({10, 20, 30}),
                      Context().WithVariable("N", MInteger().Between(3, 3))));
}

TEST(MArrayTest, SatisfiesConstraintsWithNoConstraintsShouldAcceptAnything) {
  EXPECT_THAT(MArray<MInteger>(), IsSatisfiedWith(std::vector<int64_t>({})));
  EXPECT_THAT(MArray<MInteger>(), IsSatisfiedWith(std::vector<int64_t>({1})));
  EXPECT_THAT(MArray<MInteger>(),
              IsSatisfiedWith(std::vector<int64_t>({1, 2, 3})));
}

TEST(MArrayTest, SatisfiesConstraintsChecksLengthConstraintsProperly) {
  auto constraints = []() { return MArray(MInteger().Between(1, 10)); };

  {  // Exact size
    EXPECT_THAT(constraints().OfLength(7),
                IsSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7})));
    EXPECT_THAT(constraints().OfLength(7, 7),
                IsSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7})));
    EXPECT_THAT(constraints().OfLength(MInteger().Between(7, 7)),
                IsSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7})));
    EXPECT_THAT(constraints().OfLength(0),
                IsSatisfiedWith(std::vector<int64_t>({})));

    EXPECT_THAT(constraints().OfLength(6),
                IsNotSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7}),
                                   "length"));
    EXPECT_THAT(constraints().OfLength(8),
                IsNotSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7}),
                                   "length"));

    EXPECT_THAT(constraints().OfLength(0),
                IsNotSatisfiedWith(std::vector<int64_t>({1}), "length"));
    EXPECT_THAT(constraints().OfLength(1),
                IsNotSatisfiedWith(std::vector<int64_t>(), "length"));
  }

  {  // Ranges for `OfLength`
     // Middle
    EXPECT_THAT(constraints().OfLength(6, 8),
                IsSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7})));
    // Low
    EXPECT_THAT(constraints().OfLength(7, 9),
                IsSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7})));
    // High
    EXPECT_THAT(constraints().OfLength(5, 7),
                IsSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7})));
  }
}

TEST(MArrayTest, SatisfiesConstraintsShouldCheckForDistinctElements) {
  EXPECT_THAT(
      MArray(MInteger().Between(1, 5)).WithDistinctElements().OfLength(3),
      IsSatisfiedWith(std::vector<int64_t>({1, 2, 5})));

  EXPECT_THAT(
      MArray(MInteger().Between(1, 5)).WithDistinctElements().OfLength(3),
      IsNotSatisfiedWith(std::vector<int64_t>({1, 2, 2}), "distinct"));
}

TEST(MArrayTest, SatisfiesConstraintsShouldCheckForDistinctElementsWithArg) {
  EXPECT_THAT(MArray(MInteger().Between(1, 5))
                  .WithDistinctElementsWithArg()
                  .OfLength(3),
              IsSatisfiedWith(std::vector<int64_t>({1, 2, 5})));

  EXPECT_THAT(MArray(MInteger().Between(1, 5))
                  .WithDistinctElementsWithArg()
                  .OfLength(3),
              IsNotSatisfiedWith(std::vector<int64_t>({1, 2, 2}), "distinct"));
}

TEST(MArrayTest, AllOverloadsForOfLengthProduceTheSameSequenceOfData) {
  EXPECT_TRUE(AllGenerateSameValues<MArray<MInteger>>(
      {MArray(MInteger()).OfLength(5),         //
       MArray(MInteger()).OfLength("5"),       //
       MArray(MInteger()).OfLength(5, 5),      //
       MArray(MInteger()).OfLength(5, "5"),    //
       MArray(MInteger()).OfLength("5", 5),    //
       MArray(MInteger()).OfLength("5", "5"),  //
       MArray(MInteger()).OfLength(MInteger().Is(5))}));

  EXPECT_TRUE(AllGenerateSameValues<MArray<MInteger>>(
      {MArray(MInteger()).OfLength(MInteger().Between(1, 9)),  //
       MArray(MInteger()).OfLength(1, 9),                      //
       MArray(MInteger()).OfLength(1, "9"),                    //
       MArray(MInteger()).OfLength("1", 9),                    //
       MArray(MInteger()).OfLength("1", "9")}));
}

MATCHER(HasDuplicateIntegers,
        negation ? "has no duplicate values" : "has duplicate values") {
  absl::flat_hash_set<int64_t> seen;
  for (int64_t c : arg) {
    auto [it, inserted] = seen.insert(c);
    if (!inserted) {
      *result_listener << "duplicate integer = " << c;
      return true;
    }
  }
  return false;
}

TEST(MArrayTest, WithDistinctElementsReturnsOnlyDistinctValues) {
  EXPECT_THAT(
      MArray(MInteger().Between(1, 10)).OfLength(10).WithDistinctElements(),
      GeneratedValuesAre(UnorderedElementsAre(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)));

  EXPECT_THAT(
      MArray(MInteger().Between(1, 100)).OfLength(50).WithDistinctElements(),
      GeneratedValuesAre(Not(HasDuplicateIntegers())));
}

TEST(MArrayTest, WithDistinctElementsWithNotEnoughDistinctValuesDies) {
  EXPECT_THAT(
      Generate(
          MArray(MInteger().Between(1, 5)).OfLength(10).WithDistinctElements()),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               HasSubstr("distinct values")));
}

TEST(MArrayTest, WithDistinctElementsIsAbleToGenerateWithHugeValue) {
  // This is asking for all integers between 1 and 10^4. This should
  // succeed relatively quickly. Testing if there are enough retries.
  MORIARTY_EXPECT_OK(Generate(MArray(MInteger().Between(1, "10^4"))
                                  .OfLength("10^4")
                                  .WithDistinctElements()));
}

TEST(MArrayTest, WhitespaceSeparatorShouldAffectPrint) {
  EXPECT_THAT(
      Print(MArray(MInteger()).WithSeparator(Whitespace::kNewline), {1, 2, 3}),
      IsOkAndHolds("1\n2\n3"));  // Note no newline after
}

TEST(MArrayTest, WhitespaceSeparatorShouldAffectRead) {
  EXPECT_THAT(
      Read(MArray(MInteger()).WithSeparator(Whitespace::kNewline).OfLength(6),
           "1\n2\n3\n4\n5\n6"),
      IsOkAndHolds(ElementsAre(1, 2, 3, 4, 5, 6)));

  EXPECT_THAT(
      Read(MArray(MInteger()).WithSeparator(Whitespace::kTab).OfLength(3),
           "3\t2\t1"),
      IsOkAndHolds(ElementsAre(3, 2, 1)));

  // Read wrong whitespace between characters.
  EXPECT_THAT(
      Read(MArray(MInteger()).WithSeparator(Whitespace::kNewline).OfLength(6),
           "1\n2\n3 4\n5\n6"),
      StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(MArrayTest, WhitespaceSeparatorWithMultipleSameTypesPasses) {
  EXPECT_THAT(Read(MArray(MInteger())
                       .WithSeparator(Whitespace::kNewline)
                       .WithSeparator(Whitespace::kNewline)
                       .OfLength(6),
                   "1\n2\n3\n4\n5\n6"),
              IsOkAndHolds(ElementsAre(1, 2, 3, 4, 5, 6)));
}

TEST(MArrayTest, WhitespaceSeparatorShouldFailWithTwoSeparators) {
  EXPECT_THAT(Read(MArray(MInteger())
                       .WithSeparator(Whitespace::kNewline)
                       .WithSeparator(Whitespace::kTab)
                       .OfLength(6),
                   "1\n2\n3\n4\n5\n6"),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(MArrayTest, ReadShouldBeAbleToDetermineLengthFromAnotherVariable) {
  EXPECT_THAT(Read(MArray(MInteger()).OfLength("N"), "1 2 3",
                   Context().WithValue<MInteger>("N", 3)),
              IsOkAndHolds(ElementsAre(1, 2, 3)));

  // TODO(darcybest): The following 2 cases do not currently work (and do not
  // technically *have* to work), but it would be nice if we could make them
  // work.
  //
  // EXPECT_THAT(Read(MArray(MInteger()).OfLength("N"), "1 2 3",
  //                  Context().WithVariable("N", MInteger().Between(3, 3))),
  //             IsOkAndHolds(ElementsAre(1, 2, 3)));

  // EXPECT_THAT(Read(MArray(MInteger()).OfLength("N"), "1 2 3",
  //                  Context().WithVariable("N", MInteger().Is(3))),
  //             IsOkAndHolds(ElementsAre(1, 2, 3)));

  EXPECT_THAT(Read(MArray(MInteger()).OfLength(3), "1 2 3"),
              IsOkAndHolds(ElementsAre(1, 2, 3)));

  EXPECT_THAT(
      Read(MArray(MInteger()).OfLength(MInteger().Between(3, 3)), "1 2 3"),
      IsOkAndHolds(ElementsAre(1, 2, 3)));
}

TEST(MArrayTest,
     ReadShouldFailIfLengthDependsOnAnUnknownVariableOrNonUniqueInteger) {
  EXPECT_THAT(Read(MArray(MInteger()).OfLength("N"), "1 2 3"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("length of array")));

  EXPECT_THAT(Read(MArray(MInteger()).OfLength(2, 3), "1 2 3"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("length of array")));

  EXPECT_THAT(
      Read(MArray(MInteger()).OfLength(MInteger().Between(3, 4)), "1 2 3"),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               HasSubstr("length of array")));
}

TEST(MArrayTest, LengthPropertyIsProcessedProperly) {
  MArray<MInteger> arr =
      MArray(MInteger()).OfLength(MInteger().Between(1, 200));

  // The exact values here are fuzzy and may need to change with the
  // meaning of "small", "medium", etc.
  EXPECT_THAT(MArray<MInteger>(arr).WithKnownProperty(
                  {.category = "size", .descriptor = "min"}),
              GeneratedValuesAre(SizeIs(Eq(1))));
  EXPECT_THAT(MArray<MInteger>(arr).WithKnownProperty(
                  {.category = "size", .descriptor = "tiny"}),
              GeneratedValuesAre(SizeIs(Le(10))));
  EXPECT_THAT(MArray<MInteger>(arr).WithKnownProperty(
                  {.category = "size", .descriptor = "small"}),
              GeneratedValuesAre(SizeIs(Le(50))));
  EXPECT_THAT(MArray<MInteger>(arr).WithKnownProperty(
                  {.category = "size", .descriptor = "medium"}),
              GeneratedValuesAre(SizeIs(Le(100))));
  EXPECT_THAT(MArray<MInteger>(arr).WithKnownProperty(
                  {.category = "size", .descriptor = "large"}),
              GeneratedValuesAre(SizeIs(Ge(100))));
  EXPECT_THAT(MArray<MInteger>(arr).WithKnownProperty(
                  {.category = "size", .descriptor = "huge"}),
              GeneratedValuesAre(SizeIs(Ge(150))));
  EXPECT_THAT(MArray<MInteger>(arr).WithKnownProperty(
                  {.category = "size", .descriptor = "max"}),
              GeneratedValuesAre(SizeIs(Eq(200))));

  // Different ways of constructing the size.
  EXPECT_THAT(
      MArray<MInteger>()
          .OfLength(MInteger().Between(1, 1000))
          .WithKnownProperty({.category = "size", .descriptor = "small"}),
      GeneratedValuesAre(SizeIs(Le(200))));

  EXPECT_THAT(
      MArray(MInteger())
          .OfLength(MInteger().Between(1, 1000))
          .WithKnownProperty({.category = "size", .descriptor = "small"}),
      GeneratedValuesAre(SizeIs(Le(200))));
}

TEST(MArrayTest, GetDifficultInstancesContainsLengthCases) {
  EXPECT_THAT(GenerateDifficultInstancesValues(
                  MArray(MInteger()).OfLength(MInteger().Between(0, 1000))),
              IsOkAndHolds(IsSupersetOf({SizeIs(0), SizeIs(1), SizeIs(1000)})));
}

TEST(MArrayTest, GetDifficultInstancesNoLengthFails) {
  EXPECT_THAT(GenerateDifficultInstancesValues(MArray(MInteger())),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("no length parameter given")));
}

// -----------------------------------------------------------------------------
//  Tests for the non-builder version of MArray's API
//  These are copies of the above functions, but with the builder version of
//  MArray replaced with the non-builder version.
// -----------------------------------------------------------------------------

TEST(MArrayNonBuilderTest, TypicalReadCaseWorks) {
  EXPECT_THAT(Read(MArray<MInteger>(Length(6)), "1 2 3 4 5 6"),
              IsOkAndHolds(ElementsAre(1, 2, 3, 4, 5, 6)));
  EXPECT_THAT(
      Read(MArray<MString>(Length(5)), "Hello, World! Welcome to Moriarty!"),
      IsOkAndHolds(
          ElementsAre("Hello,", "World!", "Welcome", "to", "Moriarty!")));
}

TEST(MArrayNonBuilderTest, ReadingTheWrongLengthOfMArrayShouldFail) {
  EXPECT_THAT(Read(MArray<MInteger>(Length(6)), "1 2 3 4"),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(MArrayNonBuilderTest, SimpleGenerateCaseWorks) {
  EXPECT_THAT(Generate(MArray<MInteger>(Elements<MInteger>(Between(1, 10)),
                                        Length(Between(5, 50)))),
              IsOkAndHolds(AllOf(SizeIs(AllOf(Ge(5), Le(50))),
                                 Each(AllOf(Ge(1), Le(10))))));
}

TEST(MArrayNonBuilderTest, NestedMArrayWorks) {
  EXPECT_THAT(
      Generate(NestedMArray(
          MArray<MInteger>(Elements<MInteger>(Between(1, 10)), Length(3)),
          Length(5))),
      IsOkAndHolds(AllOf(SizeIs(5),
                         Each(AllOf(SizeIs(3), Each(AllOf(Ge(1), Le(10))))))));
  {
    // Silly deeply nested case
    MArray A = NestedMArray(
        NestedMArray(
            NestedMArray(NestedMArray(MArray<MInteger>(
                                          Elements<MInteger>(Between(1, 100)),
                                          Length(Between(1, 3))),
                                      Length(Between(2, 4))),
                         Length(Between(3, 5))),
            Length(Between(4, 6))),
        Length(Between(5, 7)));

    MORIARTY_ASSERT_OK_AND_ASSIGN(auto a, Generate(A));

    EXPECT_THAT(a, SizeIs(AllOf(Ge(5), Le(7))));
    EXPECT_THAT(a, Each(SizeIs(AllOf(Ge(4), Le(6)))));
    EXPECT_THAT(a, Each(Each(SizeIs(AllOf(Ge(3), Le(5))))));
    EXPECT_THAT(a, Each(Each(Each(SizeIs(AllOf(Ge(2), Le(4)))))));
    EXPECT_THAT(a, Each(Each(Each(Each(SizeIs(AllOf(Ge(1), Le(3))))))));
    EXPECT_THAT(a, Each(Each(Each(Each(Each(AllOf(Ge(1), Le(100))))))));
  }
}

TEST(MArrayNonBuilderTest, GenerateShouldSuccessfullyComplete) {
  MORIARTY_EXPECT_OK(Generate(MArray<MInteger>(Length(Between(4, 10)))));
  MORIARTY_EXPECT_OK(Generate(MArray<MInteger>(
      Elements<MInteger>(Between(1, 10)), Length(Between(4, 10)))));
}

TEST(MArrayNonBuilderTest, RepeatedOfLengthCallsShouldBeIntersectedTogether) {
  auto gen_given_length1 = [](int lo, int hi) {
    return MArray<MInteger>(Elements<MInteger>(Between(1, 10)),
                            Length(Between(lo, hi)));
  };
  auto gen_given_length2 = [](int lo1, int hi1, int lo2, int hi2) {
    return MArray<MInteger>(Elements<MInteger>(Between(1, 10)),
                            Length(Between(lo1, hi1)),
                            Length(Between(lo2, hi2)));
  };

  // All possible valid intersections
  EXPECT_TRUE(
      GenerateSameValues(gen_given_length2(0, 30, 1, 10),
                         gen_given_length1(1, 10)));  // First is superset
  EXPECT_TRUE(
      GenerateSameValues(gen_given_length2(1, 10, 0, 30),
                         gen_given_length1(1, 10)));  // Second is superset
  EXPECT_TRUE(
      GenerateSameValues(gen_given_length2(0, 10, 1, 30),
                         gen_given_length1(1, 10)));  // First on the left
  EXPECT_TRUE(
      GenerateSameValues(gen_given_length2(1, 30, 0, 10),
                         gen_given_length1(1, 10)));  // First on the right
  EXPECT_TRUE(GenerateSameValues(gen_given_length2(1, 8, 8, 10),
                                 gen_given_length1(8, 8)));  // Singleton Range
}

TEST(MArrayNonBuilderTest, InvalidLengthShouldFail) {
  EXPECT_THAT(Generate(MArray<MInteger>(Length(-1))),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Valid range is empty")));
  EXPECT_THAT(Generate(MArray<MInteger>(Length(Between(0, -1)))),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Valid range is empty")));
}

TEST(MArrayNonBuilderTest, LengthZeroProducesTheEmptyArray) {
  EXPECT_THAT(Generate(MArray<MInteger>(Length(0))), IsOkAndHolds(IsEmpty()));
}

TEST(MArrayNonBuilderTest, MergeFromCorrectlyMergesOnLength) {
  // The alphabet is irrelevant for the tests
  auto get_arr = [](int lo, int hi) {
    return MArray<MInteger>(Elements<MInteger>(Between(1, 10)),
                            Length(Between(lo, hi)));
  };

  // All possible valid intersection of length
  EXPECT_TRUE(GenerateSameValues(get_arr(0, 30).MergeFrom(get_arr(1, 10)),
                                 get_arr(1, 10)));  // First is a superset
  EXPECT_TRUE(GenerateSameValues(get_arr(1, 10).MergeFrom(get_arr(0, 30)),
                                 get_arr(1, 10)));  // Second is a superset
  EXPECT_TRUE(GenerateSameValues(get_arr(0, 10).MergeFrom(get_arr(1, 30)),
                                 get_arr(1, 10)));  // First on left
  EXPECT_TRUE(GenerateSameValues(get_arr(1, 30).MergeFrom(get_arr(0, 10)),
                                 get_arr(1, 10)));  // First on right
  EXPECT_TRUE(GenerateSameValues(get_arr(1, 8).MergeFrom(get_arr(8, 10)),
                                 get_arr(8, 8)));  // Singleton range

  EXPECT_THAT(Generate(get_arr(1, 6).MergeFrom(get_arr(10, 20))),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("range is empty")));
}

TEST(MArrayNonBuilderTest, MergeFromCorrectlyMergesElementConstraints) {
  auto int_array = [](int lo, int hi) {
    return MArray<MInteger>(Elements<MInteger>(Between(lo, hi)), Length(20));
  };

  EXPECT_TRUE(GenerateSameValues(int_array(1, 10).MergeFrom(int_array(6, 12)),
                                 int_array(6, 10)));

  EXPECT_THAT(Generate(int_array(1, 6).MergeFrom(int_array(10, 20))),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("range is empty")));
}

TEST(MArrayNonBuilderTest, LengthIsSatisfied) {
  // Includes small and large ranges
  for (int size = 0; size < 40; size++) {
    EXPECT_THAT(Generate(MArray<MInteger>(Length(Between(size / 2, size)))),
                IsOkAndHolds(SizeIs(AllOf(Ge(size / 2), Le(size)))));
  }
  for (int size = 900; size < 940; size++) {
    EXPECT_THAT(Generate(MArray(MInteger()).OfLength(size / 2, size)),
                IsOkAndHolds(SizeIs(AllOf(Ge(size / 2), Le(size)))));
  }
}

TEST(MArrayNonBuilderTest, SatisfiesConstraintsChecksMArrayContentsProperly) {
  EXPECT_THAT(MArray<MInteger>(Elements<MInteger>(Between(1, 10))),
              IsSatisfiedWith(std::vector<int64_t>({5, 6, 1, 4, 3, 9, 10})));

  EXPECT_THAT(MArray<MInteger>(Elements<MInteger>(Between(1, 10))),
              IsNotSatisfiedWith(std::vector<int64_t>({5, 6, 7, 8, 9, 10, 11}),
                                 "element 6"));
  EXPECT_THAT(MArray<MInteger>(Elements<MInteger>(Between(1, 10))),
              IsNotSatisfiedWith(std::vector<int64_t>({5, 6, 0, 8, 9, 10}),
                                 "element 2"));
}

TEST(MArrayNonBuilderTest,
     SatisfiesConstraintsChecksMArraySizeWhenItIsVariable) {
  EXPECT_THAT(MArray<MInteger>(Length("N")),
              IsSatisfiedWith(std::vector<int64_t>({10, 20, 30}),
                              Context().WithValue<MInteger>("N", 3)));
}

TEST(MArrayNonBuilderTest,
     SatisfiesConstraintsFailsIfLengthDependsOnAVariableAndIsNotKnown) {
  // If N is definitely unique, it should pass.
  EXPECT_THAT(MArray<MInteger>(Length(3)),
              IsSatisfiedWith(std::vector<int64_t>({10, 20, 30})));
  EXPECT_THAT(
      MArray<MInteger>(Length("N")),
      IsSatisfiedWith(std::vector<int64_t>({10, 20, 30}),
                      Context().WithVariable("N", MInteger(Exactly(3)))));

  // If there is a unique value, then we might be able to determine the length.
  EXPECT_THAT(
      MArray<MInteger>(Length("N")),
      IsSatisfiedWith(std::vector<int64_t>({10, 20, 30}),
                      Context().WithVariable("N", MInteger(Between(3, 3)))));
}

TEST(MArrayNonBuilderTest,
     SatisfiesConstraintsWithNoConstraintsShouldAcceptAnything) {
  EXPECT_THAT(MArray<MInteger>(), IsSatisfiedWith(std::vector<int64_t>({})));
  EXPECT_THAT(MArray<MInteger>(), IsSatisfiedWith(std::vector<int64_t>({1})));
  EXPECT_THAT(MArray<MInteger>(),
              IsSatisfiedWith(std::vector<int64_t>({1, 2, 3})));
}

TEST(MArrayNonBuilderTest,
     SatisfiesConstraintsChecksLengthConstraintsProperly) {
  auto constraints = [](Length length_constraint) {
    return MArray<MInteger>(Elements<MInteger>(Between(1, 10)),
                            std::move(length_constraint));
  };

  {  // Exact length
    EXPECT_THAT(constraints(Length(7)),
                IsSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7})));
    EXPECT_THAT(constraints(Length(Between(7, 7))),
                IsSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7})));
    EXPECT_THAT(constraints(Length(0)),
                IsSatisfiedWith(std::vector<int64_t>({})));

    EXPECT_THAT(constraints(Length(6)),
                IsNotSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7}),
                                   "length"));
    EXPECT_THAT(constraints(Length(8)),
                IsNotSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7}),
                                   "length"));

    EXPECT_THAT(constraints(Length(0)),
                IsNotSatisfiedWith(std::vector<int64_t>({1}), "length"));
    EXPECT_THAT(constraints(Length(1)),
                IsNotSatisfiedWith(std::vector<int64_t>(), "length"));
  }

  {  // Ranges for `Length`
     // Middle
    EXPECT_THAT(constraints(Length(Between(6, 8))),
                IsSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7})));
    // Low
    EXPECT_THAT(constraints(Length(Between(7, 9))),
                IsSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7})));
    // High
    EXPECT_THAT(constraints(Length(Between(5, 7))),
                IsSatisfiedWith(std::vector<int64_t>({1, 2, 3, 4, 5, 6, 7})));
  }
}

TEST(MArrayNonBuilderTest, SatisfiesConstraintsShouldCheckForDistinctElements) {
  EXPECT_THAT(MArray<MInteger>(Elements<MInteger>(Between(1, 5)),
                               DistinctElements(), Length(3)),
              IsSatisfiedWith(std::vector<int64_t>({1, 2, 5})));

  EXPECT_THAT(MArray<MInteger>(Elements<MInteger>(Between(1, 5)),
                               DistinctElements(), Length(3)),
              IsNotSatisfiedWith(std::vector<int64_t>({1, 2, 2}), "distinct"));
}

TEST(MArrayNonBuilderTest,
     SatisfiesConstraintsShouldCheckForDistinctElementsWithArg) {
  EXPECT_THAT(MArray<MInteger>(Elements<MInteger>(Between(1, 5)), Length(3))
                  .WithDistinctElementsWithArg(),
              IsSatisfiedWith(std::vector<int64_t>({1, 2, 5})));

  EXPECT_THAT(MArray<MInteger>(Elements<MInteger>(Between(1, 5)), Length(3))
                  .WithDistinctElementsWithArg(),
              IsNotSatisfiedWith(std::vector<int64_t>({1, 2, 2}), "distinct"));
}

TEST(MArrayNonBuilderTest, WithDistinctElementsReturnsOnlyDistinctValues) {
  EXPECT_THAT(
      MArray<MInteger>(Elements<MInteger>(Between(1, 10)), Length(10),
                       DistinctElements()),
      GeneratedValuesAre(UnorderedElementsAre(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)));

  EXPECT_THAT(MArray<MInteger>(Elements<MInteger>(Between(1, 100)), Length(50),
                               DistinctElements()),
              GeneratedValuesAre(Not(HasDuplicateIntegers())));
}

TEST(MArrayNonBuilderTest,
     WithDistinctElementsWithNotEnoughDistinctValuesFails) {
  EXPECT_THAT(Generate(MArray<MInteger>(Elements<MInteger>(Between(1, 5)),
                                        Length(10), DistinctElements())),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("distinct values")));
}

TEST(MArrayNonBuilderTest, WithDistinctElementsIsAbleToGenerateWithHugeValue) {
  // This is asking for all integers between 1 and 10^4. This should
  // succeed relatively quickly. Testing if there are enough retries.
  MORIARTY_EXPECT_OK(
      Generate(MArray<MInteger>(Elements<MInteger>(Between(1, "10^4")),
                                Length("10^4"), DistinctElements())));
}

TEST(MArrayNonBuilderTest, WhitespaceSeparatorShouldAffectPrint) {
  EXPECT_THAT(
      Print(MArray<MInteger>(IOSeparator(Whitespace::kNewline)), {1, 2, 3}),
      IsOkAndHolds("1\n2\n3"));  // Note no newline after
}

TEST(MArrayNonBuilderTest, WhitespaceSeparatorShouldAffectRead) {
  EXPECT_THAT(
      Read(MArray<MInteger>(IOSeparator(Whitespace::kNewline), Length(6)),
           "1\n2\n3\n4\n5\n6"),
      IsOkAndHolds(ElementsAre(1, 2, 3, 4, 5, 6)));

  EXPECT_THAT(Read(MArray<MInteger>(IOSeparator(Whitespace::kTab), Length(3)),
                   "3\t2\t1"),
              IsOkAndHolds(ElementsAre(3, 2, 1)));

  // Read wrong whitespace between characters.
  EXPECT_THAT(
      Read(MArray<MInteger>().WithSeparator(Whitespace::kNewline).OfLength(6),
           "1\n2\n3 4\n5\n6"),
      StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(MArrayNonBuilderTest, WhitespaceSeparatorWithMultipleSameTypesPasses) {
  EXPECT_THAT(
      Read(MArray<MInteger>(IOSeparator(Whitespace::kNewline),
                            IOSeparator(Whitespace::kNewline), Length(6)),
           "1\n2\n3\n4\n5\n6"),
      IsOkAndHolds(ElementsAre(1, 2, 3, 4, 5, 6)));
}

TEST(MArrayNonBuilderTest, WhitespaceSeparatorShouldFailWithTwoSeparators) {
  EXPECT_THAT(Read(MArray<MInteger>(IOSeparator(Whitespace::kNewline),
                                    IOSeparator(Whitespace::kTab), Length(6)),
                   "1\n2\n3\n4\n5\n6"),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(MArrayNonBuilderTest,
     ReadShouldBeAbleToDetermineLengthFromAnotherVariable) {
  EXPECT_THAT(Read(MArray<MInteger>(Length("N")), "1 2 3",
                   Context().WithValue<MInteger>("N", 3)),
              IsOkAndHolds(ElementsAre(1, 2, 3)));

  // TODO(darcybest): The following 2 cases do not currently work (and do not
  // technically *have* to work), but it would be nice if we could make them
  // work.
  //
  // EXPECT_THAT(Read(MArray<MInteger>().OfLength("N"), "1 2 3",
  //                  Context().WithVariable("N", MInteger().Between(3, 3))),
  //             IsOkAndHolds(ElementsAre(1, 2, 3)));

  // EXPECT_THAT(Read(MArray<MInteger>().OfLength("N"), "1 2 3",
  //                  Context().WithVariable("N", MInteger().Is(3))),
  //             IsOkAndHolds(ElementsAre(1, 2, 3)));

  EXPECT_THAT(Read(MArray<MInteger>(Length(3)), "1 2 3"),
              IsOkAndHolds(ElementsAre(1, 2, 3)));

  EXPECT_THAT(Read(MArray<MInteger>(Length(Between(3, 3))), "1 2 3"),
              IsOkAndHolds(ElementsAre(1, 2, 3)));
}

TEST(MArrayNonBuilderTest,
     ReadShouldFailIfLengthDependsOnAnUnknownVariableOrNonUniqueInteger) {
  EXPECT_THAT(Read(MArray<MInteger>(Length("N")), "1 2 3"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("length of array")));

  EXPECT_THAT(Read(MArray<MInteger>(Length(Between(2, 3))), "1 2 3"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("length of array")));

  EXPECT_THAT(Read(MArray<MInteger>(Length(Between(3, 4))), "1 2 3"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("length of array")));
}

TEST(MArrayNonBuilderTest, LengthPropertyIsProcessedProperly) {
  MArray<MInteger> arr = MArray<MInteger>(Length(Between(1, 200)));

  // The exact values here are fuzzy and may need to change with the
  // meaning of "small", "medium", etc.
  EXPECT_THAT(MArray<MInteger>(arr).WithKnownProperty(
                  {.category = "size", .descriptor = "min"}),
              GeneratedValuesAre(SizeIs(Eq(1))));
  EXPECT_THAT(MArray<MInteger>(arr).WithKnownProperty(
                  {.category = "size", .descriptor = "tiny"}),
              GeneratedValuesAre(SizeIs(Le(10))));
  EXPECT_THAT(MArray<MInteger>(arr).WithKnownProperty(
                  {.category = "size", .descriptor = "small"}),
              GeneratedValuesAre(SizeIs(Le(50))));
  EXPECT_THAT(MArray<MInteger>(arr).WithKnownProperty(
                  {.category = "size", .descriptor = "medium"}),
              GeneratedValuesAre(SizeIs(Le(100))));
  EXPECT_THAT(MArray<MInteger>(arr).WithKnownProperty(
                  {.category = "size", .descriptor = "large"}),
              GeneratedValuesAre(SizeIs(Ge(100))));
  EXPECT_THAT(MArray<MInteger>(arr).WithKnownProperty(
                  {.category = "size", .descriptor = "huge"}),
              GeneratedValuesAre(SizeIs(Ge(150))));
  EXPECT_THAT(MArray<MInteger>(arr).WithKnownProperty(
                  {.category = "size", .descriptor = "max"}),
              GeneratedValuesAre(SizeIs(Eq(200))));
}

TEST(MArrayNonBuilderTest, GetDifficultInstancesContainsLengthCases) {
  EXPECT_THAT(GenerateDifficultInstancesValues(
                  MArray<MInteger>(Length(Between(0, 1000)))),
              IsOkAndHolds(IsSupersetOf({SizeIs(0), SizeIs(1), SizeIs(1000)})));
}

TEST(MArrayNonBuilderTest, GetDifficultInstancesNoLengthFails) {
  EXPECT_THAT(GenerateDifficultInstancesValues(MArray<MInteger>()),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("no length parameter given")));
}

TEST(MArrayNonBuilderTest, ExactlyConstraintWorks) {
  EXPECT_THAT(
      Generate(MArray<MInteger>(Exactly(std::vector<int64_t>{1, 2, 3}))),
      IsOkAndHolds(ElementsAre(1, 2, 3)));
}

}  // namespace
}  // namespace moriarty
