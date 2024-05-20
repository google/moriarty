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

#include "src/internal/simple_pattern.h"

#include <cstdint>
#include <limits>
#include <ostream>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "src/internal/random_engine.h"
#include "src/util/test_status_macro/status_testutil.h"

namespace moriarty {
namespace moriarty_internal {

// Some PrintTo's for nicer debugging experience.
void PrintTo(PatternNode::SubpatternType type, std::ostream* os) {
  *os << (type == PatternNode::SubpatternType::kAnyOf ? "AnyOf" : "AllOf");
}

void PrintTo(const PatternNode& node, std::ostream* os, int depth = 0) {
  *os << std::string(depth, ' ') << node.pattern << " ";
  PrintTo(node.subpattern_type, os);
  *os << "\n";
  for (const PatternNode& subpattern : node.subpatterns)
    PrintTo(subpattern, os, depth + 1);
}

namespace {

using ::testing::AllOf;
using ::testing::AnyOf;
using ::testing::Eq;
using ::testing::ExplainMatchResult;
using ::testing::Field;
using ::testing::FieldsAre;
using ::testing::Ge;
using ::testing::IsEmpty;
using ::testing::Le;
using ::testing::Matches;
using ::testing::Not;
using ::testing::SizeIs;
using ::moriarty::IsOk;
using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;

TEST(SimplePatternTest, CreateShouldAcceptGoodPatterns) {
  MORIARTY_EXPECT_OK(SimplePattern::Create("abc"));
  MORIARTY_EXPECT_OK(SimplePattern::Create("a*"));
  MORIARTY_EXPECT_OK(SimplePattern::Create("a*b+"));
  MORIARTY_EXPECT_OK(SimplePattern::Create("a|b|c"));
  MORIARTY_EXPECT_OK(SimplePattern::Create("a(bc)"));
}

TEST(SimplePatternTest, CreateShouldRejectBadPatterns) {
  EXPECT_THAT(SimplePattern::Create("ab)"), Not(IsOk()));
  EXPECT_THAT(SimplePattern::Create("(*)"), Not(IsOk()));
  EXPECT_THAT(SimplePattern::Create("*"), Not(IsOk()));
}

TEST(SimplePatternTest, CreateShouldRejectEmptyPattern) {
  EXPECT_THAT(SimplePattern::Create(""), Not(IsOk()));
}

TEST(SimplePatternTest, PatternShouldMatchSimpleCase) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("a"));
  EXPECT_TRUE(p.Matches("a"));
}

TEST(SimplePatternTest, PatternShouldMatchSimpleConcatCase) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("abc"));
  EXPECT_TRUE(p.Matches("abc"));
}

TEST(SimplePatternTest, PatternShouldMatchSimpleOrCase) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("a|b|c"));
  EXPECT_TRUE(p.Matches("c"));
  EXPECT_TRUE(p.Matches("a"));
  EXPECT_TRUE(p.Matches("b"));
}

TEST(SimplePatternTest, NestedPatternShouldMatch) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      SimplePattern p, SimplePattern::Create("(a|b|c)(d|e|f)"));
  EXPECT_TRUE(p.Matches("ce"));
  EXPECT_TRUE(p.Matches("af"));
  EXPECT_FALSE(p.Matches("bx"));
  EXPECT_FALSE(p.Matches("xe"));
}

TEST(SimplePatternTest, DotWildcardDoesNotExist) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("."));

  EXPECT_FALSE(p.Matches("a"));
  EXPECT_TRUE(p.Matches("."));
}

TEST(SimplePatternTest, StarWildcardsShouldMatch) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("a*"));

  EXPECT_TRUE(p.Matches(""));
  EXPECT_TRUE(p.Matches("a"));
  EXPECT_TRUE(p.Matches("aa"));
  EXPECT_TRUE(p.Matches("aaa"));

  EXPECT_FALSE(p.Matches("x"));
  EXPECT_FALSE(p.Matches("aax"));
}

TEST(SimplePatternTest, PlusWildcardsShouldMatch) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("a+"));

  EXPECT_FALSE(p.Matches(""));
  EXPECT_TRUE(p.Matches("a"));
  EXPECT_TRUE(p.Matches("aa"));
  EXPECT_TRUE(p.Matches("aaa"));

  EXPECT_FALSE(p.Matches("x"));
  EXPECT_FALSE(p.Matches("aax"));
}

TEST(SimplePatternTest, QuestionMarkWildcardsShouldMatch) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("a?"));

  EXPECT_TRUE(p.Matches(""));
  EXPECT_TRUE(p.Matches("a"));
  EXPECT_FALSE(p.Matches("aa"));

  EXPECT_FALSE(p.Matches("x"));
  EXPECT_FALSE(p.Matches("aax"));
}

TEST(SimplePatternTest, ExactLengthWildcardsShouldMatch) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("a{2}"));

  EXPECT_FALSE(p.Matches(""));
  EXPECT_FALSE(p.Matches("a"));
  EXPECT_TRUE(p.Matches("aa"));
  EXPECT_FALSE(p.Matches("aaa"));

  EXPECT_FALSE(p.Matches("x"));
  EXPECT_FALSE(p.Matches("aax"));
}

TEST(SimplePatternTest, RangeLengthWildcardsShouldMatch) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("a{1, 2}"));

  EXPECT_FALSE(p.Matches(""));
  EXPECT_TRUE(p.Matches("a"));
  EXPECT_TRUE(p.Matches("aa"));
  EXPECT_FALSE(p.Matches("aaa"));

  EXPECT_FALSE(p.Matches("x"));
  EXPECT_FALSE(p.Matches("aax"));
}

TEST(SimplePatternTest, UpperBoundWildcardsShouldMatch) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("a{,2}"));

  EXPECT_TRUE(p.Matches(""));
  EXPECT_TRUE(p.Matches("a"));
  EXPECT_TRUE(p.Matches("aa"));
  EXPECT_FALSE(p.Matches("aaa"));

  EXPECT_FALSE(p.Matches("x"));
  EXPECT_FALSE(p.Matches("aax"));
}

TEST(SimplePatternTest, LowerBoundWildcardsShouldMatch) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("a{2,}"));

  EXPECT_FALSE(p.Matches(""));
  EXPECT_FALSE(p.Matches("a"));
  EXPECT_TRUE(p.Matches("aa"));
  EXPECT_TRUE(p.Matches("aaa"));

  EXPECT_FALSE(p.Matches("x"));
  EXPECT_FALSE(p.Matches("aax"));
}

TEST(SimplePatternTest, CharacterSetsSimpleCaseShouldWork) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("[abc]"));

  EXPECT_TRUE(p.Matches("a"));
  EXPECT_TRUE(p.Matches("b"));
  EXPECT_TRUE(p.Matches("c"));
  EXPECT_FALSE(p.Matches("d"));
  EXPECT_FALSE(p.Matches("ab"));
}

TEST(SimplePatternTest, ConcatenatedCharacterSetsSimpleCaseShouldWork) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("[abc][def]"));

  EXPECT_TRUE(p.Matches("ad"));
  EXPECT_TRUE(p.Matches("bf"));
  EXPECT_TRUE(p.Matches("cd"));
  EXPECT_FALSE(p.Matches("ax"));
  EXPECT_FALSE(p.Matches("dd"));
}

TEST(SimplePatternTest, CharacterSetsWithRangesSimpleCaseShouldWork) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("[a-f]"));

  EXPECT_TRUE(p.Matches("a"));
  EXPECT_TRUE(p.Matches("b"));
  EXPECT_TRUE(p.Matches("f"));
  EXPECT_FALSE(p.Matches("g"));
  EXPECT_FALSE(p.Matches("x"));
}

TEST(SimplePatternTest,
     CharacterSetsWithRangesAndWildcardsSimpleCaseShouldWork) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("[a-f]*"));

  EXPECT_TRUE(p.Matches(""));
  EXPECT_TRUE(p.Matches("aaa"));
  EXPECT_TRUE(p.Matches("ba"));
  EXPECT_TRUE(p.Matches("deadbeef"));
  EXPECT_FALSE(p.Matches("xxx"));
  EXPECT_FALSE(p.Matches("0"));
}

TEST(SimplePatternTest, RecursiveGroupingsShouldWork) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      SimplePattern p, SimplePattern::Create("((hello|bye)world)"));

  EXPECT_TRUE(p.Matches("helloworld"));
  EXPECT_TRUE(p.Matches("byeworld"));
  EXPECT_FALSE(p.Matches("hello"));
  EXPECT_FALSE(p.Matches("bye"));
  EXPECT_FALSE(p.Matches("world"));
}

TEST(SimplePatternTest, GroupingsAndAnOrClauseShouldWork) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      SimplePattern p, SimplePattern::Create("((hello|bye)|other)"));

  EXPECT_FALSE(p.Matches("helloother"));
  EXPECT_FALSE(p.Matches("byeother"));
  EXPECT_TRUE(p.Matches("hello"));
  EXPECT_TRUE(p.Matches("bye"));
  EXPECT_TRUE(p.Matches("other"));
}

TEST(SimplePatternTest, GroupingWithWildcardsShouldFail) {
  EXPECT_THAT(SimplePattern::Create("(ac)*"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(SimplePatternTest, InvalidSyntaxShouldFail) {
  EXPECT_THAT(SimplePattern::Create("|"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(SimplePattern::Create("*"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(SimplePattern::Create("ab)"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(SimplePattern::Create("["),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(SimplePattern::Create("]"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(SimplePatternTest, FineToHaveLotsOfSquareBrackets) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      SimplePattern p, SimplePattern::Create("[a][b][X][Y][Z][@][!]"));

  EXPECT_TRUE(p.Matches("abXYZ@!"));
}

TEST(SimplePatternTest, SpecialCharactersCanBeUsedInsideSquareBrackets) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(
      SimplePattern p, SimplePattern::Create("[(][)][*][[][]][?][+]"));

  EXPECT_TRUE(p.Matches("()*[]?+"));
}

TEST(SimplePatternTest, SimpleGenerationWorks) {
  RandomEngine engine({1, 2, 3}, "v0.1");

  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("a"));
  for (int tries = 0; tries < 10; tries++) {
    EXPECT_THAT(p.Generate(engine), IsOkAndHolds("a"));
  }
}

TEST(SimplePatternTest, NonEscapedSpacesInCharSetShouldBeIgnored) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("a[b c]d"));

  EXPECT_TRUE(p.Matches("abd"));
  EXPECT_TRUE(p.Matches("acd"));
  EXPECT_FALSE(p.Matches("a d"));
}

TEST(SimplePatternTest, NonEscapedSpacesOutsideCharSetShouldBeIgnored) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("a bc"));

  EXPECT_TRUE(p.Matches("abc"));
  EXPECT_FALSE(p.Matches("a bc"));
}

TEST(SimplePatternTest, EscapedSpacesShouldWork) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create(R"(a[b\ c]d)"));

  EXPECT_TRUE(p.Matches("abd"));
  EXPECT_TRUE(p.Matches("acd"));
  EXPECT_TRUE(p.Matches("a d"));
}

TEST(SimplePatternTest, EscapedBackslashShouldWork) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create(R"(a[b\\c]d)"));

  EXPECT_TRUE(p.Matches("abd"));
  EXPECT_TRUE(p.Matches("acd"));
  EXPECT_TRUE(p.Matches(R"(a\d)"));
}

TEST(SimplePatternTest, EscapedCharacterAtEndOfStringShouldFail) {
  EXPECT_THAT(SimplePattern::Create(R"(abc\)"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(SimplePatternTest, InvalidEscapeCharactersShouldFail) {
  EXPECT_THAT(SimplePattern::Create(R"(\n)"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(SimplePattern::Create(R"(\r)"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(SimplePattern::Create(R"(\t)"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(SimplePattern::Create(R"(\x)"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(SimplePattern::Create(R"(\a)"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(SimplePattern::Create(R"(\b)"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

// SubpatternsAre() [for use with GoogleTest]
//
// Checks if the subpatterns of a PatternNode are as expected.
//
// Example usage:
//
// EXPECT_THAT(SimplePattern::Create("regex"),
//             GeneratedValuesAre(SizeIs(Le(10)));
MATCHER_P(GeneratedValuesAre, matcher, "") {
  if (!arg.ok()) {
    return false;
  }
  SimplePattern p = *arg;  // absl::StatusOr<SimplePattern> -> SimplePattern
  RandomEngine engine({1, 2, 3, 4}, "v0.1");

  for (int tries = 0; tries < 10; tries++) {
    absl::StatusOr<std::string> value = p.Generate(engine);
    if (!value.ok()) {
      *result_listener << "Failed to generate value: " << value.status();
      return false;
    }
    if (!Matches(matcher)(*value)) {
      return ExplainMatchResult(matcher, *value, result_listener);
    }
    if (!p.Matches(*value)) {
      *result_listener << "Generated value does not match pattern: " << *value;
      return false;
    }
  }
  return true;
}

MATCHER_P2(GeneratedValuesWithRestrictionsAre, matcher, restricted_alphabet,
           "") {
  if (!arg.ok()) {
    return false;
  }
  SimplePattern p = *arg;  // absl::StatusOr<SimplePattern> -> SimplePattern
  RandomEngine engine({1, 2, 3, 4}, "v0.1");

  for (int tries = 0; tries < 10; tries++) {
    absl::StatusOr<std::string> value =
        p.GenerateWithRestrictions(restricted_alphabet, engine);
    if (!value.ok()) {
      *result_listener << "Failed to generate value: " << value.status();
      return false;
    }
    if (!Matches(matcher)(*value)) {
      return ExplainMatchResult(matcher, *value, result_listener);
    }
    if (!p.Matches(*value)) {
      *result_listener << "Generated value does not match pattern: " << *value;
      return false;
    }
  }
  return true;
}

TEST(SimplePatternTest, GenerationWithOrClauseShouldWork) {
  EXPECT_THAT(SimplePattern::Create("a|b|c"),
              GeneratedValuesAre(AnyOf("a", "b", "c")));
}

TEST(SimplePatternTest, GenerationWithComplexOrClauseShouldWork) {
  EXPECT_THAT(SimplePattern::Create("a|bb|ccc"),
              GeneratedValuesAre(AnyOf("a", "bb", "ccc")));
}

TEST(SimplePatternTest, GenerationWithConcatenationShouldWork) {
  EXPECT_THAT(SimplePattern::Create("abcde"), GeneratedValuesAre("abcde"));
}

TEST(SimplePatternTest, GenerationWithLengthRangesGivesProperLengths) {
  EXPECT_THAT(SimplePattern::Create("a{4, 8}"),
              GeneratedValuesAre(SizeIs(AllOf(Ge(4), Le(8)))));
  EXPECT_THAT(SimplePattern::Create("a{,8}"),
              GeneratedValuesAre(SizeIs(Le(8))));
  EXPECT_THAT(SimplePattern::Create("a{8}"), GeneratedValuesAre(SizeIs(8)));
}

TEST(SimplePatternTest, GenerationWithNestedSubExpressionsShouldWork) {
  EXPECT_THAT(SimplePattern::Create("a(b|c)(d|e)"),
              GeneratedValuesAre(AnyOf("abd", "abe", "acd", "ace")));
}

TEST(SimplePatternTest, GenerationWithLargeWildcardShouldThrowError) {
  RandomEngine engine({1, 2, 3, 4}, "v0.1");

  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p1,
                                         SimplePattern::Create("a+"));
  EXPECT_THAT(p1.Generate(engine),
              StatusIs(absl::StatusCode::kInvalidArgument));

  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p2,
                                         SimplePattern::Create("a*"));
  EXPECT_THAT(p2.Generate(engine),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(SimplePatternTest, GenerationWithSmallWildcardShouldWork) {
  EXPECT_THAT(SimplePattern::Create("a?b"),
              GeneratedValuesAre(AnyOf("ab", "b")));
}

TEST(SimplePatternTest, GenerationWithAlphabetRestrictionsShouldWork) {
  // Restricting the alphabet to only 'f's should only be able to generate one
  // string.
  EXPECT_THAT(SimplePattern::Create("[a-z]{8}"),
              GeneratedValuesWithRestrictionsAre("ffffffff", "f"));

  EXPECT_THAT(
      SimplePattern::Create("[a-z]{2}"),
      GeneratedValuesWithRestrictionsAre(AnyOf("ff", "fx", "xf", "xx"), "fx"));
}

TEST(SimplePatternTest,
     GenerationWithVeryStrongRestrictionsShouldAttemptEmptyString) {
  // Restricting the alphabet to only 'x' means that only the empty string is
  // valid.
  EXPECT_THAT(SimplePattern::Create("[a-f]{0, 10}"),
              GeneratedValuesWithRestrictionsAre("", "x"));

  // If the pattern doesn't allow the empty string, ensure it fails.
  RandomEngine engine({1, 2, 3, 4}, "v0.1");
  MORIARTY_ASSERT_OK_AND_ASSIGN(SimplePattern p,
                                         SimplePattern::Create("[a-f]{1, 10}"));
  EXPECT_THAT(
      p.GenerateWithRestrictions(/*restricted_alphabet = */ "x", engine),
      StatusIs(absl::StatusCode::kInvalidArgument));
}

// -----------------------------------------------------------------------------
//  Tests below here are for more internal-facing functions. Only functions
//  above are for the external API.

// SubpatternsAre() [for use with GoogleTest]
//
// Checks if the subpatterns of a PatternNode are as expected.
//
// Example usage:
//
// EXPECT_THAT(your_code(), SubpatternsAre(PatternNode::SubpatternType::kAnyOf,
//                                         {"a", "b|c", "d"}));
MATCHER_P2(SubpatternsAre, expected_type, expected_subpatterns, "") {
  if (arg.subpattern_type != expected_type) {
    return ExplainMatchResult(Eq(expected_type), arg.subpattern_type,
                              result_listener);
  }

  if (arg.subpatterns.size() != expected_subpatterns.size()) {
    return ExplainMatchResult(SizeIs(expected_subpatterns.size()),
                              arg.subpatterns, result_listener);
  }

  for (int i = 0; i < arg.subpatterns.size(); i++) {
    if (arg.subpatterns[i].pattern != expected_subpatterns[i]) {
      return ExplainMatchResult(Eq(expected_subpatterns[i]),
                                arg.subpatterns[i].pattern, result_listener);
    }
  }

  return true;
}

TEST(RepeatedCharSetTest, DefaultSetShouldOnlyEmptyString) {
  RepeatedCharSet r;
  MORIARTY_EXPECT_OK(r.IsValid(""));
  EXPECT_THAT(r.IsValid("a"), Not(IsOk()));
  EXPECT_THAT(r.IsValid("b"), Not(IsOk()));
  EXPECT_THAT(r.IsValid("abc"), Not(IsOk()));
}

TEST(RepeatedCharSetTest, AddedCharactersShouldBeAccepted) {
  RepeatedCharSet r;
  MORIARTY_ASSERT_OK(r.SetRange(1, 1));
  MORIARTY_ASSERT_OK(r.Add('a'));
  MORIARTY_ASSERT_OK(r.Add('b'));
  MORIARTY_EXPECT_OK(r.IsValid("a"));
  MORIARTY_EXPECT_OK(r.IsValid("b"));
}

TEST(RepeatedCharSetTest, NonAddedCharactersShouldNotBeAccepted) {
  RepeatedCharSet r;
  MORIARTY_ASSERT_OK(r.SetRange(1, 1));
  MORIARTY_ASSERT_OK(r.Add('a'));
  MORIARTY_ASSERT_OK(r.Add('b'));

  EXPECT_THAT(r.IsValid("c"), StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(r.IsValid("d"), StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(RepeatedCharSetTest, DefaultLengthShouldBeZero) {
  RepeatedCharSet r;
  MORIARTY_ASSERT_OK(r.Add('a'));
  MORIARTY_EXPECT_OK(r.IsValid(""));
  EXPECT_THAT(r.IsValid("a"), StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(r.IsValid("aa"), StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(RepeatedCharSetTest, ZeroRangeShouldBeOk) {
  RepeatedCharSet r;
  MORIARTY_ASSERT_OK(r.SetRange(0, 0));
  MORIARTY_EXPECT_OK(r.IsValid(""));
}

TEST(RepeatedCharSetTest, IsValidShouldCheckRange) {
  RepeatedCharSet r;
  MORIARTY_ASSERT_OK(r.Add('a'));
  MORIARTY_ASSERT_OK(r.SetRange(2, 3));

  EXPECT_THAT(r.IsValid("a"), StatusIs(absl::StatusCode::kInvalidArgument));
  MORIARTY_EXPECT_OK(r.IsValid("aa"));
  MORIARTY_EXPECT_OK(r.IsValid("aaa"));
  EXPECT_THAT(r.IsValid("aaaa"), StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(RepeatedCharSetTest, FlippingShouldImpactIsValid) {
  RepeatedCharSet r;
  MORIARTY_ASSERT_OK(r.SetRange(1, 1));
  r.FlipValidCharacters();
  MORIARTY_EXPECT_OK(r.IsValid("a"));
  MORIARTY_EXPECT_OK(r.IsValid("b"));
  MORIARTY_EXPECT_OK(r.IsValid("c"));
}

TEST(RepeatedCharSetTest, FlippingShouldTurnOffAddedCharacters) {
  RepeatedCharSet r;
  MORIARTY_ASSERT_OK(r.SetRange(1, 1));
  MORIARTY_ASSERT_OK(r.Add('a'));
  r.FlipValidCharacters();

  EXPECT_THAT(r.IsValid("a"), StatusIs(absl::StatusCode::kInvalidArgument));
  MORIARTY_EXPECT_OK(r.IsValid("b"));
  MORIARTY_EXPECT_OK(r.IsValid("c"));
}

TEST(RepeatedCharSetTest, AddingAgainAfterFlipShouldBeOk) {
  RepeatedCharSet r;
  MORIARTY_ASSERT_OK(r.SetRange(1, 1));
  MORIARTY_ASSERT_OK(r.Add('a'));
  r.FlipValidCharacters();
  MORIARTY_ASSERT_OK(r.Add('a'));
  MORIARTY_EXPECT_OK(r.IsValid("a"));
}

TEST(RepeatedCharSetTest, LongestValidPrefixShouldSucceedForLengthZero) {
  EXPECT_THAT(RepeatedCharSet().LongestValidPrefix("a"), IsOkAndHolds(0));
}

TEST(RepeatedCharSetTest, LongestValidPrefixShouldSucceedForLengthOneGood) {
  RepeatedCharSet r;
  MORIARTY_ASSERT_OK(r.SetRange(1, 1));
  MORIARTY_ASSERT_OK(r.Add('a'));

  EXPECT_THAT(r.LongestValidPrefix("a"), IsOkAndHolds(1));
}

TEST(RepeatedCharSetTest, LongestValidPrefixShouldFailForLengthOneBad) {
  RepeatedCharSet r;
  MORIARTY_ASSERT_OK(r.SetRange(1, 1));
  MORIARTY_ASSERT_OK(r.Add('a'));

  EXPECT_THAT(r.LongestValidPrefix("b"), Not(IsOk()));
}

TEST(RepeatedCharSetTest, LongestValidPrefixShouldSucceedForLongerStrings) {
  RepeatedCharSet r;
  MORIARTY_ASSERT_OK(r.SetRange(1, 4));
  MORIARTY_ASSERT_OK(r.Add('a'));

  EXPECT_THAT(r.LongestValidPrefix("aaaaaaaaaaa"), IsOkAndHolds(4));
}

TEST(RepeatedCharSetTest, LongestValidPrefixShouldFailForShorterStrings) {
  RepeatedCharSet r;
  MORIARTY_ASSERT_OK(r.SetRange(3, 4));
  MORIARTY_ASSERT_OK(r.Add('a'));

  EXPECT_THAT(r.LongestValidPrefix("aa"), Not(IsOk()));
}

TEST(RepeatedCharSetTest, NegativeMinRangeShouldBeOk) {
  RepeatedCharSet r;
  MORIARTY_ASSERT_OK(r.Add('a'));
  MORIARTY_EXPECT_OK(r.SetRange(-5, 3));
  MORIARTY_EXPECT_OK(r.IsValid(""));
  MORIARTY_EXPECT_OK(r.IsValid("aaa"));
}

TEST(RepeatedCharSetTest, NegativeMaxRangeShouldFail) {
  RepeatedCharSet r;

  EXPECT_THAT(r.SetRange(-5, -3), StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(RepeatedCharSetTest, NegativeMinZeroMaxRangeShouldBeOk) {
  RepeatedCharSet r;
  MORIARTY_EXPECT_OK(r.SetRange(-5, 0));
  MORIARTY_EXPECT_OK(r.IsValid(""));
}

TEST(RepeatedCharSetTest, InvalidRangeShouldFail) {
  RepeatedCharSet r;

  EXPECT_THAT(r.SetRange(3, 2), StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(RepeatedCharSetTest, AddingANegativeCharShouldFail) {
  RepeatedCharSet r;
  EXPECT_THAT(r.Add(static_cast<char>(-1)),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(RepeatedCharSetTest, FlipShouldNotAcceptNegativeChars) {
  RepeatedCharSet r;
  r.FlipValidCharacters();

  EXPECT_THAT(r.IsValid(std::string(1, static_cast<char>(-1))),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(RepeatedCharSetTest, FlipShouldAcceptNullChar) {
  RepeatedCharSet r;
  MORIARTY_ASSERT_OK(r.SetRange(1, 1));
  r.FlipValidCharacters();
  MORIARTY_EXPECT_OK(r.IsValid(std::string(1, static_cast<char>(0))));
}

TEST(RepeatedCharSetTest, AddingCharactersMultipleTimesShouldFail) {
  RepeatedCharSet r;
  MORIARTY_ASSERT_OK(r.Add('a'));

  EXPECT_THAT(r.Add('a'), StatusIs(absl::StatusCode::kAlreadyExists));
}

TEST(ParseCharSetTest, EmptyCharacterSetsShouldFail) {
  EXPECT_THAT(CharacterSetPrefixLength(""),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseCharSetTest, SingleCharacterCharacterSetsShouldBeOk) {
  EXPECT_THAT(CharacterSetPrefixLength("a"), IsOkAndHolds(1));
  EXPECT_THAT(CharacterSetPrefixLength("ab"), IsOkAndHolds(1));
  EXPECT_THAT(CharacterSetPrefixLength("a["), IsOkAndHolds(1));
}

TEST(ParseCharSetTest, TypicalCasesShouldWork) {
  EXPECT_THAT(CharacterSetPrefixLength("[a]"), IsOkAndHolds(3));
  EXPECT_THAT(CharacterSetPrefixLength("[ab]"), IsOkAndHolds(4));
}

TEST(ParseCharSetTest, CharactersAfterCloseShouldBeIgnored) {
  EXPECT_THAT(CharacterSetPrefixLength("[a]xx"), IsOkAndHolds(3));
  EXPECT_THAT(CharacterSetPrefixLength("[ab]xx"), IsOkAndHolds(4));
}

TEST(ParseCharSetTest, OpenSquareBraceShouldBeAccepted) {
  EXPECT_THAT(CharacterSetPrefixLength("[[]"), IsOkAndHolds(3));
}

TEST(ParseCharSetTest, CloseSquareBraceShouldBeAccepted) {
  EXPECT_THAT(CharacterSetPrefixLength("[]]"), IsOkAndHolds(3));
}

TEST(ParseCharSetTest, OpenThenCloseSquareBraceShouldBeAccepted) {
  EXPECT_THAT(CharacterSetPrefixLength("[[]]"), IsOkAndHolds(4));
}

TEST(ParseCharSetTest, CloseThenOpenSquareBraceShouldTakeEarlierOne) {
  EXPECT_THAT(CharacterSetPrefixLength("[][]"), IsOkAndHolds(2));
}

TEST(ParseCharSetTest, AnyDuplicateCharacterShouldForceTheEarlierOne) {
  EXPECT_THAT(CharacterSetPrefixLength("[(][)][*][[][]][?][+]"),
              IsOkAndHolds(3));
  EXPECT_THAT(CharacterSetPrefixLength("[)][*][[][]][?][+]"), IsOkAndHolds(3));
  EXPECT_THAT(CharacterSetPrefixLength("[*][[][]][?][+]"), IsOkAndHolds(3));
  EXPECT_THAT(CharacterSetPrefixLength("[[][]][?][+]"), IsOkAndHolds(3));
  EXPECT_THAT(CharacterSetPrefixLength("[]][?][+]"), IsOkAndHolds(3));
  EXPECT_THAT(CharacterSetPrefixLength("[?][+]"), IsOkAndHolds(3));
  EXPECT_THAT(CharacterSetPrefixLength("[+]"), IsOkAndHolds(3));
}

TEST(ParseCharSetTest, MissingCloseBraceShouldFail) {
  EXPECT_THAT(CharacterSetPrefixLength("[hi"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseCharSetTest, SpecialCharacterAtStartShouldFail) {
  EXPECT_THAT(CharacterSetPrefixLength("]"), Not(IsOk()));
  EXPECT_THAT(CharacterSetPrefixLength("("), Not(IsOk()));
  EXPECT_THAT(CharacterSetPrefixLength(")"), Not(IsOk()));
  EXPECT_THAT(CharacterSetPrefixLength("{"), Not(IsOk()));
  EXPECT_THAT(CharacterSetPrefixLength("}"), Not(IsOk()));
  EXPECT_THAT(CharacterSetPrefixLength("^"), Not(IsOk()));
  EXPECT_THAT(CharacterSetPrefixLength("?"), Not(IsOk()));
  EXPECT_THAT(CharacterSetPrefixLength("*"), Not(IsOk()));
  EXPECT_THAT(CharacterSetPrefixLength("+"), Not(IsOk()));
  EXPECT_THAT(CharacterSetPrefixLength("-"), Not(IsOk()));
  EXPECT_THAT(CharacterSetPrefixLength("|"), Not(IsOk()));
}

TEST(ParseCharSetTest, EmptyCharSetBodyShouldFail) {
  EXPECT_THAT(ParseCharacterSetBody(""),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseCharSetTest, DuplicateCharactersInCharSetBodyShouldFail) {
  EXPECT_THAT(ParseCharacterSetBody("aa"), Not(IsOk()));
}

TEST(ParseCharSetTest, SquareBracesShouldBeOk) {
  MORIARTY_EXPECT_OK(ParseCharacterSetBody("]"));
  MORIARTY_EXPECT_OK(ParseCharacterSetBody("["));
}

TEST(ParseCharSetTest, SquareBracesShouldBeInTheRightOrder) {
  MORIARTY_EXPECT_OK(ParseCharacterSetBody("[]"));
  EXPECT_THAT(ParseCharacterSetBody("]["),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseCharSetTest, SpecialCharacterAsBodyShouldBeOk) {
  MORIARTY_EXPECT_OK(ParseCharacterSetBody("["));
  MORIARTY_EXPECT_OK(ParseCharacterSetBody("]"));
  MORIARTY_EXPECT_OK(ParseCharacterSetBody("("));
  MORIARTY_EXPECT_OK(ParseCharacterSetBody(")"));
  MORIARTY_EXPECT_OK(ParseCharacterSetBody("{"));
  MORIARTY_EXPECT_OK(ParseCharacterSetBody("}"));
  MORIARTY_EXPECT_OK(ParseCharacterSetBody("^"));
  MORIARTY_EXPECT_OK(ParseCharacterSetBody("?"));
  MORIARTY_EXPECT_OK(ParseCharacterSetBody("*"));
  MORIARTY_EXPECT_OK(ParseCharacterSetBody("+"));
  MORIARTY_EXPECT_OK(ParseCharacterSetBody("-"));
  MORIARTY_EXPECT_OK(ParseCharacterSetBody("|"));
}

TEST(ParseCharSetTest, SingleCaretShouldNotBeTreatedAsNegation) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(RepeatedCharSet r,
                                         ParseCharacterSetBody("^"));
  MORIARTY_EXPECT_OK(r.IsValid("^"));
  EXPECT_THAT(r.IsValid("a"), StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseCharSetTest, StartingCaretShouldNotConsiderLaterCaretDuplicate) {
  MORIARTY_EXPECT_OK(ParseCharacterSetBody("^^"));
}

TEST(ParseCharSetTest, StartingCaretShouldNotInterferWithLaterCaret) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(RepeatedCharSet r,
                                         ParseCharacterSetBody("^^"));
  MORIARTY_EXPECT_OK(r.IsValid("a"));
  EXPECT_THAT(r.IsValid("^"), StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseCharSetTest, StartingCaretShouldBeTreatedAsNegation) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(RepeatedCharSet r,
                                         ParseCharacterSetBody("^a"));
  MORIARTY_EXPECT_OK(r.IsValid("b"));
  MORIARTY_EXPECT_OK(r.IsValid("^"));
  EXPECT_THAT(r.IsValid("a"), StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseCharSetTest, TrailingNegativeSignShouldBeTreatedAsChar) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(RepeatedCharSet r,
                                         ParseCharacterSetBody("abc-"));
  MORIARTY_EXPECT_OK(r.IsValid("-"));
}

TEST(ParseCharSetTest, SingleNegativeSignShouldBeTreatedAsChar) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(RepeatedCharSet r,
                                         ParseCharacterSetBody("-"));
  MORIARTY_EXPECT_OK(r.IsValid("-"));
}

TEST(ParseCharSetTest, NegativeSignInMiddleShouldBeTreatedAsRange) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(RepeatedCharSet r,
                                         ParseCharacterSetBody("a-z"));
  MORIARTY_EXPECT_OK(r.IsValid("a"));
  MORIARTY_EXPECT_OK(r.IsValid("b"));
  EXPECT_THAT(r.IsValid("-"), StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseCharSetTest, UpperCaseRangeShouldWork) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(RepeatedCharSet r,
                                         ParseCharacterSetBody("B-Z"));
  MORIARTY_EXPECT_OK(r.IsValid("B"));
  MORIARTY_EXPECT_OK(r.IsValid("Z"));
  EXPECT_THAT(r.IsValid("A"), StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(r.IsValid("a"), StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseCharSetTest, LowerCaseRangeShouldWork) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(RepeatedCharSet r,
                                         ParseCharacterSetBody("a-q"));
  MORIARTY_EXPECT_OK(r.IsValid("a"));
  MORIARTY_EXPECT_OK(r.IsValid("q"));
  EXPECT_THAT(r.IsValid("r"), StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(r.IsValid("A"), StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseCharSetTest, DigitRangeShouldWork) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(RepeatedCharSet r,
                                         ParseCharacterSetBody("0-5"));
  MORIARTY_EXPECT_OK(r.IsValid("0"));
  MORIARTY_EXPECT_OK(r.IsValid("5"));
  EXPECT_THAT(r.IsValid("6"), StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(r.IsValid("a"), StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseCharSetTest, InvalidRangeShouldFail) {
  EXPECT_THAT(ParseCharacterSetBody("9-0"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseCharacterSetBody("Z-A"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseCharacterSetBody("z-a"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseCharacterSetBody("a-A"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseCharacterSetBody("A-a"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseCharacterSetBody("A-z"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseCharacterSetBody("A-?"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseCharSetTest, BackToBackRangesShouldWork) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(RepeatedCharSet r,
                                         ParseCharacterSetBody("A-Za-z"));
  MORIARTY_EXPECT_OK(r.IsValid("A"));
  MORIARTY_EXPECT_OK(r.IsValid("B"));
  MORIARTY_EXPECT_OK(r.IsValid("a"));
  MORIARTY_EXPECT_OK(r.IsValid("b"));
}

TEST(ParseCharSetTest, DotsShouldNotBeTreatedAsWildcards) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(RepeatedCharSet r,
                                         ParseCharacterSetBody("."));
  MORIARTY_EXPECT_OK(r.IsValid("."));
  EXPECT_THAT(r.IsValid("a"), StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseCharSetTest, NegativeAtTheEndPlusRangesShouldNotCountAsDuplicate) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(RepeatedCharSet r,
                                         ParseCharacterSetBody("a-z-"));
  MORIARTY_EXPECT_OK(r.IsValid("a"));
  MORIARTY_EXPECT_OK(r.IsValid("z"));
  MORIARTY_EXPECT_OK(r.IsValid("-"));
}

TEST(ParseCharSetTest, NegativeAtTheEndPlusShouldNotBeConsideredForRange) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(RepeatedCharSet r,
                                         ParseCharacterSetBody("a-"));
  MORIARTY_EXPECT_OK(r.IsValid("a"));
  MORIARTY_EXPECT_OK(r.IsValid("-"));
  EXPECT_THAT(r.IsValid("b"), StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseCharSetTest, NegativeCharsShouldBeInvalid) {
  EXPECT_THAT(ParseCharacterSetBody(std::string(1, static_cast<char>(-1))),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseRepetitionTest, EmptyRepetitionShouldPass) {
  MORIARTY_EXPECT_OK(RepetitionPrefixLength(""));
}

TEST(ParseRepetitionTest, SingleRepetitionCharacterShouldReturnOne) {
  EXPECT_THAT(RepetitionPrefixLength("*"), IsOkAndHolds(1));
  EXPECT_THAT(RepetitionPrefixLength("?"), IsOkAndHolds(1));
  EXPECT_THAT(RepetitionPrefixLength("+"), IsOkAndHolds(1));
}

TEST(ParseRepetitionTest, SingleNonSpecialCharacterShouldReturnZero) {
  EXPECT_THAT(RepetitionPrefixLength("a"), IsOkAndHolds(0));
  EXPECT_THAT(RepetitionPrefixLength("abc"), IsOkAndHolds(0));
  EXPECT_THAT(RepetitionPrefixLength("3"), IsOkAndHolds(0));
}

TEST(ParseRepetitionTest, TypicalCasesShouldWork) {
  EXPECT_THAT(RepetitionPrefixLength("{1}"), IsOkAndHolds(3));
  EXPECT_THAT(RepetitionPrefixLength("{1,2}"), IsOkAndHolds(5));
  EXPECT_THAT(RepetitionPrefixLength("{1,}"), IsOkAndHolds(4));
  EXPECT_THAT(RepetitionPrefixLength("{,2}"), IsOkAndHolds(4));
}

TEST(ParseRepetitionTest, CharactersAfterCloseShouldBeIgnored) {
  EXPECT_THAT(RepetitionPrefixLength("{1}xx"), IsOkAndHolds(3));
  EXPECT_THAT(RepetitionPrefixLength("{1,2}xx"), IsOkAndHolds(5));
}

TEST(ParseRepetitionTest, MissingCloseBraceShouldFail) {
  EXPECT_THAT(RepetitionPrefixLength("{1"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseRepetitionTest, SpecialCharacterAtStartShouldReturnZero) {
  EXPECT_THAT(RepetitionPrefixLength("}"), IsOkAndHolds(0));
  EXPECT_THAT(RepetitionPrefixLength("("), IsOkAndHolds(0));
  EXPECT_THAT(RepetitionPrefixLength(")"), IsOkAndHolds(0));
  EXPECT_THAT(RepetitionPrefixLength("["), IsOkAndHolds(0));
  EXPECT_THAT(RepetitionPrefixLength("]"), IsOkAndHolds(0));
  EXPECT_THAT(RepetitionPrefixLength("^"), IsOkAndHolds(0));
  EXPECT_THAT(RepetitionPrefixLength("-"), IsOkAndHolds(0));
  EXPECT_THAT(RepetitionPrefixLength("|"), IsOkAndHolds(0));
}

TEST(ParseRepetitionTest, EmptyRepetitionShouldGiveLengthOne) {
  EXPECT_THAT(ParseRepetitionBody(""), IsOkAndHolds(FieldsAre(1, 1)));
}

TEST(ParseRepetitionTest, QuestionMarkGiveLengthZeroOrOne) {
  EXPECT_THAT(ParseRepetitionBody("?"), IsOkAndHolds(FieldsAre(0, 1)));
}

TEST(ParseRepetitionTest, PlusShouldGiveLengthOneAndUp) {
  EXPECT_THAT(ParseRepetitionBody("+"),
              IsOkAndHolds(FieldsAre(1, std::numeric_limits<int64_t>::max())));
}

TEST(ParseRepetitionTest, AsteriskShouldGiveLengthZeroAndUp) {
  EXPECT_THAT(ParseRepetitionBody("*"),
              IsOkAndHolds(FieldsAre(0, std::numeric_limits<int64_t>::max())));
}

TEST(ParseRepetitionTest, OneNumberShouldReturnThatNumber) {
  EXPECT_THAT(ParseRepetitionBody("{3}"), IsOkAndHolds(FieldsAre(3, 3)));
}

TEST(ParseRepetitionTest, TwoNumbersShouldReturnThoseNumbers) {
  EXPECT_THAT(ParseRepetitionBody("{3,14}"), IsOkAndHolds(FieldsAre(3, 14)));
}

TEST(ParseRepetitionTest, LowerBoundShouldReturn) {
  EXPECT_THAT(ParseRepetitionBody("{3,}"),
              IsOkAndHolds(FieldsAre(3, std::numeric_limits<int64_t>::max())));
}

TEST(ParseRepetitionTest, UpperBoundShouldReturn) {
  EXPECT_THAT(ParseRepetitionBody("{,3}"), IsOkAndHolds(FieldsAre(0, 3)));
}

TEST(ParseRepetitionTest, EmptyEmptyShouldGiveZeroAndUp) {
  EXPECT_THAT(ParseRepetitionBody("{,}"),
              IsOkAndHolds(FieldsAre(0, std::numeric_limits<int64_t>::max())));
}

TEST(ParseRepetitionTest, MultipleCommasShouldFail) {
  EXPECT_THAT(ParseRepetitionBody("{3,4,5}"), Not(IsOk()));
}

TEST(ParseRepetitionTest, MissingBracesShouldFail) {
  EXPECT_THAT(ParseRepetitionBody("{1,2"), Not(IsOk()));
  EXPECT_THAT(ParseRepetitionBody("1,2}"), Not(IsOk()));
  EXPECT_THAT(ParseRepetitionBody(",1}"), Not(IsOk()));
  EXPECT_THAT(ParseRepetitionBody("{1,"), Not(IsOk()));
}

TEST(ParseRepetitionTest, NonIntegerShouldFail) {
  EXPECT_THAT(ParseRepetitionBody("{1.5,2}"), Not(IsOk()));
  EXPECT_THAT(ParseRepetitionBody("{a,b}"), Not(IsOk()));
  EXPECT_THAT(ParseRepetitionBody("{1,b}"), Not(IsOk()));
  EXPECT_THAT(ParseRepetitionBody("{a,2}"), Not(IsOk()));
  EXPECT_THAT(ParseRepetitionBody("{a,}"), Not(IsOk()));
  EXPECT_THAT(ParseRepetitionBody("{,a}"), Not(IsOk()));
}

TEST(ParseRepetitionTest, HexShouldFail) {
  EXPECT_THAT(ParseRepetitionBody("{0x00,0x10}"), Not(IsOk()));
}

TEST(ParseRepetitionTest, SpecialCharacterAtStartShouldFail) {
  EXPECT_THAT(ParseRepetitionBody("}"), Not(IsOk()));
  EXPECT_THAT(ParseRepetitionBody("^"), Not(IsOk()));
  EXPECT_THAT(ParseRepetitionBody("("), Not(IsOk()));
  EXPECT_THAT(ParseRepetitionBody("|"), Not(IsOk()));
  EXPECT_THAT(ParseRepetitionBody("["), Not(IsOk()));
  EXPECT_THAT(ParseRepetitionBody(")"), Not(IsOk()));
  EXPECT_THAT(ParseRepetitionBody("]"), Not(IsOk()));
  EXPECT_THAT(ParseRepetitionBody("-"), Not(IsOk()));
}

TEST(ParseRepetitionTest, OtherCharacterAtStartShouldFail) {
  EXPECT_THAT(ParseRepetitionBody("a"), Not(IsOk()));
  EXPECT_THAT(ParseRepetitionBody("abc"), Not(IsOk()));
}

TEST(ParseRepeatedCharSetTest, EmptyStringShouldFail) {
  EXPECT_THAT(ParseRepeatedCharSetPrefix(""), Not(IsOk()));
}

TEST(ParseRepeatedCharSetTest, SpecialCharacterAtStartShouldFail) {
  EXPECT_THAT(ParseRepeatedCharSetPrefix("{"), Not(IsOk()));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("}"), Not(IsOk()));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("^"), Not(IsOk()));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("("), Not(IsOk()));
  EXPECT_THAT(ParseRepeatedCharSetPrefix(")"), Not(IsOk()));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("|"), Not(IsOk()));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("["), Not(IsOk()));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("]"), Not(IsOk()));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("-"), Not(IsOk()));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("?"), Not(IsOk()));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("+"), Not(IsOk()));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("*"), Not(IsOk()));
}

TEST(ParseRepeatedCharSetTest, RepetitionPartFirstShouldFail) {
  EXPECT_THAT(ParseRepeatedCharSetPrefix("{3,1}"), Not(IsOk()));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("{,1}"), Not(IsOk()));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("*"), Not(IsOk()));
}

TEST(ParseRepeatedCharSetTest,
     FirstCharacterWithoutRepetitionShouldReturnSingleItem) {
  EXPECT_THAT(ParseRepeatedCharSetPrefix("a"),
              IsOkAndHolds(Field(&PatternNode::pattern, "a")));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("abc"),
              IsOkAndHolds(Field(&PatternNode::pattern, "a")));
}

TEST(ParseRepeatedCharSetTest,
     FirstCharacterWithRepetitionShouldReturnSingleItemWithRepetition) {
  EXPECT_THAT(ParseRepeatedCharSetPrefix("a*"),
              IsOkAndHolds(Field(&PatternNode::pattern, "a*")));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("a+bc"),
              IsOkAndHolds(Field(&PatternNode::pattern, "a+")));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("a{1,3}bc"),
              IsOkAndHolds(Field(&PatternNode::pattern, "a{1,3}")));
}

TEST(ParseRepeatedCharSetTest,
     FirstCharacterWithoutRepetitionShouldReturnJustCharSet) {
  EXPECT_THAT(ParseRepeatedCharSetPrefix("[a]"),
              IsOkAndHolds(Field(&PatternNode::pattern, "[a]")));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("[ab]c"),
              IsOkAndHolds(Field(&PatternNode::pattern, "[ab]")));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("[ab]c*"),
              IsOkAndHolds(Field(&PatternNode::pattern, "[ab]")));
}

TEST(ParseRepeatedCharSetTest,
     CharacterSetWithRepetitionShouldReturnWithRepetition) {
  EXPECT_THAT(ParseRepeatedCharSetPrefix("[a]*"),
              IsOkAndHolds(Field(&PatternNode::pattern, "[a]*")));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("[ab]?c"),
              IsOkAndHolds(Field(&PatternNode::pattern, "[ab]?")));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("[abc]{1,3}de"),
              IsOkAndHolds(Field(&PatternNode::pattern, "[abc]{1,3}")));
}

TEST(ParseRepeatedCharSetTest, CharacterSetParserShouldNotSetSubpattern) {
  EXPECT_THAT(ParseRepeatedCharSetPrefix("a"),
              IsOkAndHolds(Field(&PatternNode::subpatterns, IsEmpty())));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("[ab]"),
              IsOkAndHolds(Field(&PatternNode::subpatterns, IsEmpty())));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("a*"),
              IsOkAndHolds(Field(&PatternNode::subpatterns, IsEmpty())));
  EXPECT_THAT(ParseRepeatedCharSetPrefix("[ab]?c"),
              IsOkAndHolds(Field(&PatternNode::subpatterns, IsEmpty())));
}

TEST(ParseRepeatedCharSetTest,
     CharacterSetShouldNotIncludeSquareBracketsByDefault) {
  MORIARTY_ASSERT_OK_AND_ASSIGN(PatternNode p,
                                         ParseRepeatedCharSetPrefix("[a]"));
  RepeatedCharSet r = p.repeated_character_set;
  MORIARTY_EXPECT_OK(r.IsValid("a"));
  EXPECT_THAT(r.IsValid("["), StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(r.IsValid("]"), StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseScopePrefixTest, EmptyStringShouldFail) {
  EXPECT_THAT(ParseScopePrefix(""), Not(IsOk()));
}

TEST(ParseScopePrefixTest, SingleCloseBracketShouldFail) {
  EXPECT_THAT(ParseScopePrefix(")"), Not(IsOk()));
}

TEST(ParseScopePrefixTest, SpecialCharacterAtStartShouldFail) {
  EXPECT_THAT(ParseScopePrefix("{"), Not(IsOk()));
  EXPECT_THAT(ParseScopePrefix("}"), Not(IsOk()));
  EXPECT_THAT(ParseScopePrefix("^"), Not(IsOk()));
  EXPECT_THAT(ParseScopePrefix("|"), Not(IsOk()));
  EXPECT_THAT(ParseScopePrefix("["), Not(IsOk()));
  EXPECT_THAT(ParseScopePrefix("]"), Not(IsOk()));
  EXPECT_THAT(ParseScopePrefix("-"), Not(IsOk()));
  EXPECT_THAT(ParseScopePrefix("?"), Not(IsOk()));
  EXPECT_THAT(ParseScopePrefix("+"), Not(IsOk()));
  EXPECT_THAT(ParseScopePrefix("*"), Not(IsOk()));
}

TEST(ParseScopePrefixTest, UnmatchedOpenBraceShouldFail) {
  EXPECT_THAT(ParseScopePrefix("("), Not(IsOk()));
  EXPECT_THAT(ParseScopePrefix("(abc"), Not(IsOk()));
  EXPECT_THAT(ParseScopePrefix("(a(bc)"), Not(IsOk()));
}

TEST(ParseScopePrefixTest, NestedScopesShouldBeOk) {
  MORIARTY_EXPECT_OK(ParseScopePrefix("(ab(cd))"));
  MORIARTY_EXPECT_OK(ParseScopePrefix("a(b(cd))"));
}

TEST(ParseScopePrefixTest, SingleCharacterShouldWork) {
  EXPECT_THAT(
      ParseScopePrefix("a"),
      IsOkAndHolds(SubpatternsAre(PatternNode::SubpatternType::kAllOf,
                                  std::vector<absl::string_view>({"a"}))));
}

TEST(ParseScopePrefixTest, ParsingFlatScopeShouldGetPatternCorrect) {
  EXPECT_THAT(ParseScopePrefix("ab"),
              IsOkAndHolds(Field(&PatternNode::pattern, "ab")));

  EXPECT_THAT(ParseScopePrefix("a*b+"),
              IsOkAndHolds(Field(&PatternNode::pattern, "a*b+")));
}

TEST(ParseScopePrefixTest,
     ParsingFlatScopeWithCloseBracketShouldGetPatternCorrect) {
  EXPECT_THAT(ParseScopePrefix("ab)cd"),
              IsOkAndHolds(Field(&PatternNode::pattern, "ab")));

  EXPECT_THAT(ParseScopePrefix("a*b+)c?d*"),
              IsOkAndHolds(Field(&PatternNode::pattern, "a*b+")));
}

TEST(ParseScopePrefixTest, ParsingFlatScopeShouldGetSubpatternsCorrect) {
  EXPECT_THAT(
      ParseScopePrefix("ab"),
      IsOkAndHolds(SubpatternsAre(PatternNode::SubpatternType::kAllOf,
                                  std::vector<absl::string_view>({"a", "b"}))));

  EXPECT_THAT(ParseScopePrefix("a*b+"),
              IsOkAndHolds(SubpatternsAre(
                  PatternNode::SubpatternType::kAllOf,
                  std::vector<absl::string_view>({"a*", "b+"}))));
}

TEST(ParseScopePrefixTest,
     ParsingFlatScopeWithEndBraceShouldGetSubpatternsCorrect) {
  EXPECT_THAT(
      ParseScopePrefix("ab)cd"),
      IsOkAndHolds(SubpatternsAre(PatternNode::SubpatternType::kAllOf,
                                  std::vector<absl::string_view>({"a", "b"}))));

  EXPECT_THAT(ParseScopePrefix("a*b+)c?d*"),
              IsOkAndHolds(SubpatternsAre(
                  PatternNode::SubpatternType::kAllOf,
                  std::vector<absl::string_view>({"a*", "b+"}))));
}

TEST(ParseScopePrefixTest, ParsingScopeWithOrClauseShouldGetPatternCorrect) {
  EXPECT_THAT(ParseScopePrefix("ab|cd"),
              IsOkAndHolds(SubpatternsAre(
                  PatternNode::SubpatternType::kAnyOf,
                  std::vector<absl::string_view>({"ab", "cd"}))));

  EXPECT_THAT(ParseScopePrefix("a*b+|c?d*"),
              IsOkAndHolds(SubpatternsAre(
                  PatternNode::SubpatternType::kAnyOf,
                  std::vector<absl::string_view>({"a*b+", "c?d*"}))));
}

TEST(ParseScopePrefixTest, ParsingScopeWithEmptyOrClausesShouldFail) {
  EXPECT_THAT(ParseScopePrefix("|"), Not(IsOk()));
  EXPECT_THAT(ParseScopePrefix("||"), Not(IsOk()));
  EXPECT_THAT(ParseScopePrefix("|a"), Not(IsOk()));
  EXPECT_THAT(ParseScopePrefix("a|"), Not(IsOk()));
  EXPECT_THAT(ParseScopePrefix("a||b"), Not(IsOk()));
}

TEST(ParseScopePrefixTest,
     ParsingScopeWithOrClauseAndEndBraceShouldGetSubpatternsCorrect) {
  EXPECT_THAT(ParseScopePrefix("ab|cd)ef"),
              IsOkAndHolds(SubpatternsAre(
                  PatternNode::SubpatternType::kAnyOf,
                  std::vector<absl::string_view>({"ab", "cd"}))));

  EXPECT_THAT(ParseScopePrefix("a*b+|c?d*)ef"),
              IsOkAndHolds(SubpatternsAre(
                  PatternNode::SubpatternType::kAnyOf,
                  std::vector<absl::string_view>({"a*b+", "c?d*"}))));
}

TEST(ParseScopePrefixTest, SpecialCharactersCanBeUsedInsideSquareBrackets) {
  EXPECT_THAT(ParseScopePrefix("[(][)][*][[][]][?][+]"),
              IsOkAndHolds(SubpatternsAre(
                  PatternNode::SubpatternType::kAllOf,
                  std::vector<absl::string_view>(
                      {"[(]", "[)]", "[*]", "[[]", "[]]", "[?]", "[+]"}))));
}

TEST(ParseScopePrefixTest, NestingShouldGetSubpatternsCorrect) {
  EXPECT_THAT(ParseScopePrefix("a(bc(de|fg))"),
              IsOkAndHolds(SubpatternsAre(
                  PatternNode::SubpatternType::kAllOf,
                  std::vector<absl::string_view>({"a", "(bc(de|fg))"}))));

  EXPECT_THAT(ParseScopePrefix("(abc(de|fg))"),
              IsOkAndHolds(SubpatternsAre(
                  PatternNode::SubpatternType::kAllOf,
                  std::vector<absl::string_view>({"(abc(de|fg))"}))));
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
