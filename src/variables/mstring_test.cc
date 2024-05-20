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

#include "src/variables/mstring.h"

#include <algorithm>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/test_utils.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty_testing::AllGenerateSameValues;
using ::moriarty_testing::Generate;
using ::moriarty_testing::GenerateDifficultInstancesValues;
using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::GenerateSameValues;
using ::moriarty_testing::IsNotSatisfiedWith;
using ::moriarty_testing::IsSatisfiedWith;
using ::moriarty_testing::Print;
using ::moriarty_testing::Read;
using ::testing::AllOf;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::HasSubstr;
using ::testing::IsSupersetOf;
using ::testing::Le;
using ::testing::MatchesRegex;
using ::testing::Not;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAreArray;
using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;

TEST(MStringTest, TypenameIsCorrect) {
  EXPECT_EQ(MString().Typename(), "MString");
}

TEST(MStringTest, PrintShouldSucceed) {
  EXPECT_THAT(Print(MString(), "value!"), IsOkAndHolds("value!"));
  EXPECT_THAT(Print(MString(), ""), IsOkAndHolds(""));
  EXPECT_THAT(Print(MString(), "multiple tokens"),
              IsOkAndHolds("multiple tokens"));
}

TEST(MStringTest, SingleTokenReadShouldSucceed) {
  EXPECT_THAT(Read(MString(), "123"), IsOkAndHolds("123"));
  EXPECT_THAT(Read(MString(), "abc"), IsOkAndHolds("abc"));
}

TEST(MStringTest, InputWithTokenWithWhitespaceAfterShouldReadToken) {
  EXPECT_THAT(Read(MString(), "world "), IsOkAndHolds("world"));
  EXPECT_THAT(Read(MString(), "you should ignore some of this"),
              IsOkAndHolds("you"));
}

TEST(MStringTest, ReadATokenWithLeadingWhitespaceShouldFail) {
  EXPECT_THAT(Read(MString(), " spacebefore"),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(MStringTest, ReadAtEofShouldFail) {
  EXPECT_THAT(Read(MString(), ""),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(MStringTest, GenerateShouldSuccessfullyComplete) {
  MString variable;
  MORIARTY_EXPECT_OK(Generate(
      variable.OfLength(MInteger().Between(4, 11)).WithAlphabet("abc")));
  MORIARTY_EXPECT_OK(Generate(variable.OfLength(4, 11).WithAlphabet("abc")));
  MORIARTY_EXPECT_OK(Generate(variable.OfLength(4).WithAlphabet("abc")));
}

TEST(MStringTest, RepeatedOfLengthCallsShouldBeIntersectedTogether) {
  auto gen_given_length1 = [](int lo, int hi) {
    return MString().OfLength(lo, hi).WithAlphabet("abcdef");
  };
  auto gen_given_length2 = [](int lo1, int hi1, int lo2, int hi2) {
    return MString().OfLength(lo1, hi1).OfLength(lo2, hi2).WithAlphabet(
        "abcdef");
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

TEST(MStringTest, InvalidLengthShouldFail) {
  EXPECT_THAT(Generate(MString().OfLength(-1).WithAlphabet("a")),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Valid range is empty")));
  EXPECT_THAT(
      Generate(MString().OfLength(MInteger().Between(0, -1)).WithAlphabet("a")),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               HasSubstr("Valid range is empty")));
}

TEST(MStringTest, LengthZeroProcudesTheEmptyString) {
  EXPECT_THAT(Generate(MString().OfLength(0).WithAlphabet("abc")),
              IsOkAndHolds(""));
}

TEST(MStringTest, AlphabetIsRequiredForGenerate) {
  EXPECT_THAT(
      Generate(MString().OfLength(10)),
      StatusIs(absl::StatusCode::kFailedPrecondition, HasSubstr("alphabet")));
  EXPECT_THAT(
      Generate(MString().OfLength(10).WithAlphabet("")),
      StatusIs(absl::StatusCode::kFailedPrecondition, HasSubstr("alphabet")));
}

TEST(MStringTest, MergeFromCorrectlyMergesOnLength) {
  // The alphabet is irrelevant for the tests
  auto get_str = []() { return MString().WithAlphabet("abc"); };

  // All possible valid intersection of length
  EXPECT_TRUE(GenerateSameValues(
      get_str().OfLength(0, 30).MergeFrom(get_str().OfLength(1, 10)),
      get_str().OfLength(1, 10)));  // First is a superset
  EXPECT_TRUE(GenerateSameValues(
      get_str().OfLength(1, 10).MergeFrom(get_str().OfLength(0, 30)),
      get_str().OfLength(1, 10)));  // Second is a superset
  EXPECT_TRUE(GenerateSameValues(
      get_str().OfLength(0, 10).MergeFrom(get_str().OfLength(1, 30)),
      get_str().OfLength(1, 10)));  // First on left
  EXPECT_TRUE(GenerateSameValues(
      get_str().OfLength(1, 30).MergeFrom(get_str().OfLength(0, 10)),
      get_str().OfLength(1, 10)));  // First on right
  EXPECT_TRUE(GenerateSameValues(
      get_str().OfLength(1, 8).MergeFrom(get_str().OfLength(8, 10)),
      get_str().OfLength(8)));  // Singleton range

  EXPECT_THAT(
      Generate(get_str().OfLength(1, 6).MergeFrom(get_str().OfLength(10, 20))),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               HasSubstr("range is empty")));
}

TEST(MStringTest, MergeFromCorrectlyMergesOnAlphabet) {
  auto string_with_alphabet = [](absl::string_view alphabet) {
    return MString().OfLength(20).WithAlphabet(alphabet);
  };

  // Intersections of alphabets
  EXPECT_TRUE(GenerateSameValues(
      string_with_alphabet("abcdef").MergeFrom(string_with_alphabet("abc")),
      string_with_alphabet("abc")));  // First is a superset
  EXPECT_TRUE(GenerateSameValues(
      string_with_alphabet("abc").MergeFrom(string_with_alphabet("abcdef")),
      string_with_alphabet("abc")));  // Second is a superset
  EXPECT_TRUE(GenerateSameValues(
      string_with_alphabet("ab").MergeFrom(string_with_alphabet("bc")),
      string_with_alphabet("b")));  // Non-empty intersection

  EXPECT_THAT(Generate(string_with_alphabet("ab").MergeFrom(
                  string_with_alphabet("cd"))),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("empty alphabet")));
}

TEST(MStringTest, LengthIsSatisfied) {
  // Includes small and large ranges
  for (int tries = 0; tries < 1000; tries++) {
    MORIARTY_ASSERT_OK_AND_ASSIGN(
        std::string s,
        Generate(MString().OfLength(tries / 2, tries).WithAlphabet("abc")));
    EXPECT_THAT(s, SizeIs(Ge(tries / 2)));
    EXPECT_THAT(s, SizeIs(Le(tries)));
  }
}

TEST(MStringTest, AlphabetIsSatisfied) {
  EXPECT_THAT(MString().OfLength(10, 20).WithAlphabet("a"),
              GeneratedValuesAre(MatchesRegex("a*")));
  EXPECT_THAT(MString().OfLength(10, 20).WithAlphabet("abc"),
              GeneratedValuesAre(MatchesRegex("[abc]*")));
}

TEST(MStringTest, AlphabetNotGivenInSortedOrderIsFine) {
  EXPECT_TRUE(
      GenerateSameValues(MString().OfLength(10, 20).WithAlphabet("abc"),
                         MString().OfLength(10, 20).WithAlphabet("cab")));
}

TEST(MStringTest, DuplicateLettersInAlphabetAreIgnored) {
  // a appears 90% of the alphabet. We should still see ~50% of the time in the
  // input. With 100,000 samples, I'm putting the cutoff at 60%. Anything over
  // 60% is extremely unlikely to happen.
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      std::string s,
      Generate(MString().OfLength(100000).WithAlphabet("aaaaaaaaab")));
  EXPECT_LE(std::count(s.begin(), s.end(), 'a'), 60000);
}

TEST(MStringTest, SatisfiesConstraintsShouldAcceptAllMStringsForDefault) {
  // Default string should accept all strings.
  EXPECT_THAT(MString(), IsSatisfiedWith(""));
  EXPECT_THAT(MString(), IsSatisfiedWith("hello"));
  EXPECT_THAT(MString(), IsSatisfiedWith("blah blah blah"));
}

TEST(MStringTest, SatisfiesConstraintsShouldAcceptAllMStringsOfCorrectLength) {
  EXPECT_THAT(MString().OfLength(5), IsSatisfiedWith("abcde"));
  EXPECT_THAT(MString().OfLength(4, 6), IsSatisfiedWith("abcde"));
  EXPECT_THAT(MString().OfLength(4, 5), IsSatisfiedWith("abcde"));
  EXPECT_THAT(MString().OfLength(5, 6), IsSatisfiedWith("abcde"));
  EXPECT_THAT(MString().OfLength(5, 5), IsSatisfiedWith("abcde"));

  EXPECT_THAT(MString().OfLength(3, 4), IsNotSatisfiedWith("abcde", "length"));
  EXPECT_THAT(MString().OfLength(1, 1000), IsNotSatisfiedWith("", "length"));
}

TEST(MStringTest, SatisfiesConstraintsShouldCheckTheAlphabetIfSet) {
  EXPECT_THAT(MString().WithAlphabet("abcdefghij"), IsSatisfiedWith("abcde"));
  EXPECT_THAT(MString().WithAlphabet("edbca"), IsSatisfiedWith("abcde"));

  EXPECT_THAT(MString().WithAlphabet("abcd"),
              IsNotSatisfiedWith("abcde", "alphabet"));
}

TEST(MStringTest, SatisfiesConstraintsWithInvalidLengthShouldFail) {
  EXPECT_THAT(MString().OfLength(MInteger().Between(0, -1)),
              IsNotSatisfiedWith("abcde", "length"));
}

TEST(MStringTest, SatisfiesConstraintsShouldCheckForDistinctCharacters) {
  EXPECT_THAT(MString().WithDistinctCharacters(), IsSatisfiedWith("abcdef"));
  EXPECT_THAT(MString().WithAlphabet("abcdef").WithDistinctCharacters(),
              IsSatisfiedWith("cbf"));

  EXPECT_THAT(MString().WithAlphabet("abcdef").WithDistinctCharacters(),
              IsNotSatisfiedWith("cc", "distinct"));
}

TEST(MStringTest, AllOverloadsForOfLengthProduceTheSameSequenceOfData) {
  // Exact size
  EXPECT_TRUE(AllGenerateSameValues<MString>(
      {MString().WithAlphabet("abc").OfLength(MInteger().Is(5)),
       MString().WithAlphabet("abc").OfLength(5),
       MString().WithAlphabet("abc").OfLength(5, 5),
       MString().WithAlphabet("abc").OfLength(5, "5"),
       MString().WithAlphabet("abc").OfLength("5", 5),
       MString().WithAlphabet("abc").OfLength("5", "5")}));

  // Range of sizes
  EXPECT_TRUE(AllGenerateSameValues<MString>(
      {MString().WithAlphabet("def").OfLength(MInteger().Between(1, 9)),
       MString().WithAlphabet("def").OfLength(1, 9),
       MString().WithAlphabet("def").OfLength(1, 9),
       MString().WithAlphabet("def").OfLength(1, "9"),
       MString().WithAlphabet("def").OfLength("1", 9),
       MString().WithAlphabet("def").OfLength("1", "9")}));
}

MATCHER(HasDuplicateLetter,
        negation ? "has no duplicate letters" : "has duplicate letters") {
  absl::flat_hash_set<char> seen;
  for (char c : arg) {
    auto [it, inserted] = seen.insert(c);
    if (!inserted) {
      *result_listener << "duplicate letter = " << c;
      return true;
    }
  }
  return false;
}

// These tests below are simply safety checks to ensure the alphabets are
// correctly typed and not accidentally modified later.
TEST(MStringTest, CommonAlphabetsDoNotHaveDuplicatedLetters) {
  EXPECT_THAT(MString::kAlphabet, Not(HasDuplicateLetter()));
  EXPECT_THAT(MString::kUpperCase, Not(HasDuplicateLetter()));
  EXPECT_THAT(MString::kLowerCase, Not(HasDuplicateLetter()));
  EXPECT_THAT(MString::kNumbers, Not(HasDuplicateLetter()));
  EXPECT_THAT(MString::kAlphaNumeric, Not(HasDuplicateLetter()));
  EXPECT_THAT(MString::kUpperAlphaNumeric, Not(HasDuplicateLetter()));
  EXPECT_THAT(MString::kLowerAlphaNumeric, Not(HasDuplicateLetter()));
}

TEST(MStringTest, CommonAlphabetsHaveTheAppropriateNumberOfElements) {
  EXPECT_THAT(MString::kAlphabet, SizeIs(26 + 26));
  EXPECT_THAT(MString::kUpperCase, SizeIs(26));
  EXPECT_THAT(MString::kLowerCase, SizeIs(26));
  EXPECT_THAT(MString::kNumbers, SizeIs(10));
  EXPECT_THAT(MString::kAlphaNumeric, SizeIs(26 + 26 + 10));
  EXPECT_THAT(MString::kUpperAlphaNumeric, SizeIs(26 + 10));
  EXPECT_THAT(MString::kLowerAlphaNumeric, SizeIs(26 + 10));
}

TEST(MStringTest, CommonAlphabetsHaveAppropriateSubsetsAndSuperSets) {
  EXPECT_THAT(MString::kAlphabet,
              UnorderedElementsAreArray(
                  absl::StrCat(MString::kUpperCase, MString::kLowerCase)));

  EXPECT_THAT(MString::kAlphaNumeric,
              UnorderedElementsAreArray(
                  absl::StrCat(MString::kAlphabet, MString::kNumbers)));

  EXPECT_THAT(MString::kUpperAlphaNumeric,
              UnorderedElementsAreArray(
                  absl::StrCat(MString::kUpperCase, MString::kNumbers)));

  EXPECT_THAT(MString::kLowerAlphaNumeric,
              UnorderedElementsAreArray(
                  absl::StrCat(MString::kLowerCase, MString::kNumbers)));
}

TEST(MStringTest, DistinctCharactersWorksInTheSimpleCase) {
  EXPECT_THAT(MString()
                  .OfLength(1, 26)
                  .WithAlphabet(MString::kAlphabet)
                  .WithDistinctCharacters(),
              GeneratedValuesAre(Not(HasDuplicateLetter())));
}

TEST(MStringTest, DistinctCharactersRequiresAShortLength) {
  EXPECT_THAT(Generate(MString()
                           .OfLength(5, 5)
                           .WithAlphabet("abc")
                           .WithDistinctCharacters()),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("length of the string")));

  // Most of the range is too large, the only way to succeed is for it to make a
  // string of length 10.
  EXPECT_THAT(Generate(MString()
                           .OfLength(10, 1000000)
                           .WithAlphabet(MString::kNumbers)
                           .WithDistinctCharacters()),
              IsOkAndHolds(Not(HasDuplicateLetter())));
}

TEST(MStringTest, MergingSimplePatternsIntoAnMStringWithoutShouldWork) {
  MString constraints = MString().WithSimplePattern("[abc]{10, 20}");
  MORIARTY_EXPECT_OK(MString().TryMergeFrom(constraints));
}

TEST(MStringTest, MergingTwoIdenticalSimplePatternsTogetherShouldWork) {
  MString constraints = MString().WithSimplePattern("[abc]{10, 20}");
  MORIARTY_EXPECT_OK(
      MString().WithSimplePattern("[abc]{10, 20}").TryMergeFrom(constraints));
}

TEST(MStringDeathTest, MergingTwoDifferentSimplePatternsTogetherShouldFail) {
  MString constraints = MString().WithSimplePattern("[abc]{10, 20}");
  EXPECT_DEATH(
      { MString().WithSimplePattern("xxxxx").MergeFrom(constraints); },
      "imple");  // checking for "[S|s]imple pattern"
}

TEST(MStringTest, GenerateWithoutSimplePatternOrLengthOrAlphabetShouldFail) {
  // No simple pattern and no alphabet
  EXPECT_THAT(
      Generate(MString()),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               AllOf(HasSubstr("simple pattern"), HasSubstr("alphabet"))));
  EXPECT_THAT(
      Generate(MString().WithAlphabet("")),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               AllOf(HasSubstr("simple pattern"), HasSubstr("alphabet"))));

  // Has Alphabet, but not simple pattern or length.
  EXPECT_THAT(
      Generate(MString().WithAlphabet("abc")),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               AllOf(HasSubstr("simple pattern"), HasSubstr("length"))));
}

TEST(MStringTest, SizePropertiesGiveDifferentSizes) {
  MString str = MString()
                    .OfLength(MInteger().Between(1, 1000))
                    .WithAlphabet(MString::kAlphabet);

  // The exact values here are fuzzy and may need to change with the
  // meaning of "small", "medium", etc.
  EXPECT_THAT(
      MString(str).WithKnownProperty({.category = "size", .descriptor = "min"}),
      GeneratedValuesAre(SizeIs(Eq(1))));
  EXPECT_THAT(MString(str).WithKnownProperty(
                  {.category = "size", .descriptor = "tiny"}),
              GeneratedValuesAre(SizeIs(Le(10))));
  EXPECT_THAT(MString(str).WithKnownProperty(
                  {.category = "size", .descriptor = "small"}),
              GeneratedValuesAre(SizeIs(Le(100))));
  EXPECT_THAT(MString(str).WithKnownProperty(
                  {.category = "size", .descriptor = "medium"}),
              GeneratedValuesAre(SizeIs(Le(500))));
  EXPECT_THAT(MString(str).WithKnownProperty(
                  {.category = "size", .descriptor = "large"}),
              GeneratedValuesAre(SizeIs(Ge(500))));
  EXPECT_THAT(MString(str).WithKnownProperty(
                  {.category = "size", .descriptor = "huge"}),
              GeneratedValuesAre(SizeIs(Ge(900))));
  EXPECT_THAT(
      MString(str).WithKnownProperty({.category = "size", .descriptor = "max"}),
      GeneratedValuesAre(SizeIs(Eq(1000))));
}

TEST(MStringTest, SimplePatternWorksForGeneration) {
  EXPECT_THAT(MString().WithSimplePattern("[abc]{10, 20}"),
              GeneratedValuesAre(MatchesRegex("[abc]{10,20}")));
}

TEST(MStringTest, SatisfiesConstraintsShouldCheck) {
  EXPECT_THAT(MString().WithSimplePattern("[abc]{10, 20}"),
              IsSatisfiedWith("abcabcabca"));
  EXPECT_THAT(MString().WithSimplePattern("[abc]{10, 20}"),
              IsNotSatisfiedWith("ABCABCABCA", "simple pattern"));
}

TEST(MStringTest, SimplePatternWithWildcardsShouldFailGeneration) {
  EXPECT_THAT(Generate(MString().WithSimplePattern("a*")),
              StatusIs(absl::StatusCode::kFailedPrecondition, HasSubstr("*")));
  EXPECT_THAT(Generate(MString().WithSimplePattern("a+")),
              StatusIs(absl::StatusCode::kFailedPrecondition, HasSubstr("+")));
}

TEST(MStringTest, SimplePatternWithWildcardsShouldWorkForSatisfiesConstraints) {
  EXPECT_THAT(MString().WithSimplePattern("a*"), IsSatisfiedWith("aaaaaa"));
  EXPECT_THAT(MString().WithSimplePattern("a+"), IsSatisfiedWith("aaaaaa"));

  EXPECT_THAT(MString().WithSimplePattern("a*"), IsSatisfiedWith(""));
  EXPECT_THAT(MString().WithSimplePattern("a+"),
              IsNotSatisfiedWith("", "simple pattern"));
}

TEST(MStringTest, SimplePatternShouldRespectAlphabets) {
  // Odds are that random cases should generate at least some `a`s. But our
  // alphabet is only "b", so the only correct answer is the string "b".
  EXPECT_THAT(MString().WithSimplePattern("a{0, 123456}b").WithAlphabet("b"),
              GeneratedValuesAre("b"));
}

TEST(MStringTest, GetDifficultInstancesContainsLengthCases) {
  EXPECT_THAT(
      GenerateDifficultInstancesValues(
          MString().OfLength(MInteger().Between(0, 10)).WithAlphabet("a")),
      IsOkAndHolds(IsSupersetOf({"", "a", "aaaaaaaaaa"})));
}

TEST(MStringTest, GetDifficultInstancesNoLengthFails) {
  EXPECT_THAT(GenerateDifficultInstancesValues(MString()),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("no length parameter given")));
}

TEST(MStringTest, ToStringWorks) {
  EXPECT_THAT(MString().ToString(), HasSubstr("MString"));
  EXPECT_THAT(MString().WithAlphabet("abx").ToString(), HasSubstr("abx"));
  EXPECT_THAT(MString().OfLength(1, 10).ToString(), HasSubstr("[1, 10]"));
  EXPECT_THAT(MString().WithDistinctCharacters().ToString(),
              HasSubstr("istinct"));  // [D|d]istinct
  EXPECT_THAT(MString().WithSimplePattern("[abc]{10, 20}").ToString(),
              HasSubstr("[abc]{10,20}"));
  EXPECT_THAT(MString()
                  .WithKnownProperty({.category = "size", .descriptor = "tiny"})
                  .ToString(),
              HasSubstr("iny"));  // [t|T]iny
}

}  // namespace
}  // namespace moriarty
