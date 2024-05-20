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

#ifndef MORIARTY_SRC_INTERNAL_SIMPLE_PATTERN_H_
#define MORIARTY_SRC_INTERNAL_SIMPLE_PATTERN_H_

#include <bitset>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "src/internal/random_engine.h"

namespace moriarty {
namespace moriarty_internal {

// A set of characters to be used as part of a SimplePattern. By default,
// accepts any string which has length == 0.
class RepeatedCharSet {
 public:
  // Adds `character` to the set of valid characters. Returns
  // `absl::kAlreadyExists` if `character` is already in the set and
  // `absl::kInvalidArgument` if `character` is negative.
  absl::Status Add(char character);

  // Flip the characters which are valid and invalid. Negative characters are
  // still invalid.
  void FlipValidCharacters();

  // Sets the number of repetitions to be between `min` and `max`.
  absl::Status SetRange(int64_t min, int64_t max);

  // Returns if `str` is valid.
  absl::Status IsValid(absl::string_view str) const;

  // Returns if `character` is a valid character.
  bool IsValidCharacter(char character) const;

  // Returns the longest prefix of `str` that is valid. If no prefix is valid,
  // returns `absl::kInvalidArgument`.
  absl::StatusOr<int64_t> LongestValidPrefix(absl::string_view str) const;

  // Minimum length of repetition.
  int64_t MinLength() const;

  // Maximum length of repetition.
  int64_t MaxLength() const;

  // Returns all valid characters.
  std::vector<char> ValidCharacters() const;

 private:
  std::bitset<128> valid_chars_;  // We only support non-negative signed char.
  int64_t min_ = 0;
  int64_t max_ = 0;
};

// A PatternNode matches in two stages:
//  * First: Matches character(s) in `repeated_character_set`.
//  * Second: Matches `subpatterns`.
struct PatternNode {
  RepeatedCharSet repeated_character_set;

  // `kAllOf` needs to match all of the `subpattern`s from left-to-right.
  // `kAnyOf` attempts to match each `subpattern` from left-to-right, and stops
  //          on the first success.
  enum class SubpatternType { kAllOf, kAnyOf };
  SubpatternType subpattern_type = SubpatternType::kAllOf;
  std::vector<PatternNode> subpatterns;

  // The pattern corresponding to this node.
  absl::string_view pattern;
};

// SimplePattern [class]
//
// Represents a very simplified expression checker. It is *NOT* equivalent to
// regex, even though it follows similar syntax. Patterns are matched greedily,
// attempting to match as much as possible without any backtracking.
//
// SimplePattern accepts the following conventions. Other than the greedy nature
// of the matching, the behavior of most the following is the same as regex, so
// exact behaviors are not listed here for brevity.
//
// Special characters:
//  * There are several special characters: \()[]{}^?*+-| (and the space char).
//  * Outside of character sets, these represent something special. To use them
//    in an expression, put them in a character set. For example, to accept
//    any number of 'a' inside of parentheses, use this:
//      GOOD: "[(]a*[)]"
//      BAD: "\(a*\)" or "(a*)"
//
// Character sets:
//  * "[abc]" matches one of 'a' or 'b' or 'c'.
//  * "[abc]*" matches "aaa", "accb"
//  * No character in the character set may be duplicated.
//  * The character set must not be empty.
//  * Anything inside "[]" is treated as-is. "[a[]]" accepts 'a' or '[' or ']'.
//  * Exceptions:
//    - Spaces must be escaped: Use '\ ' for a wanted space character.
//    - Backslashes must be escaped: Use '\\' for a wanted backslash character.
//    - '^' is treated as the negation operation if it is both the first
//      character and not the only character. Otherwise it is the '^' character.
//    - '-' is treated as a range EXCEPT if it is the last character in the
//      character set, then it is treated as the '-' character.
//       E.g., "[+-]?[0-9]" accepts "-5".
//    - If '[' and ']' are both in the set, '[' must come before ']'. For
//      example, "[][]" is treated as two empty character sets, not as the
//      character set of ']' and '['.
//  * Ranges of letters and numbers (e.g., A-Z, a-z, 0-9, A-F). These ranges
//    must be "upper-upper", "lower-lower", or "digit-digit". So "A-a" is
//    invalid.
//
// Wildcards:
//  * There are no wildcards. In particular, ".*" does not match all strings. It
//    only matches zero or more "."s.
//
// Or-expressions:
//  * "hello|world" will match either "hello" or "world".
//
// Repetition:
//  * "a*"     = Zero or more occurrences.
//  * "a+"     = One or more occurrences.
//  * "a?"     = Zero or one occurrences.
//  * "a{n}"   = Exactly n occurrences.
//  * "a{x,y}" = Between x and y occurrences, inclusive.
//  * "a{x,}"  = At least x occurrences.
//  * "a{,y}"  = At most y occurrences.
//
// Groupings:
//  * "(hello|world)" matches "hello" or "world". "(a|b)c(d|e)" matches 4
//    different strings.
//  * Groupings may recurse. "((hello|bye)world)"
//  * Groupings may NOT have repetitions. No: "(ab)*", "(ab|cd){3,4}", "(ab)?cd"
//
// Whitespace:
//  * Space characters are ignored, with exception to "\ " (backslash-space)
//    inside character sets, which represents the space character.
//
// Escaped Characters (these will be replaced by their non-escaped versions).
//  * "\\", "\ " (quotes for clarity)
class SimplePattern {
 public:
  static absl::StatusOr<SimplePattern> Create(absl::string_view pattern);

  // Returns the underlying pattern.
  std::string Pattern() const;

  // Matches()
  //
  // Determines if `str` matches the underlying pattern. Unlike normal regex,
  // `Matches` will greedily match characters and groups if possible, without
  // backtracking. If any or-expression is used (e.g., "a|b|c"), then
  // the matcher attempts to match left-to-right and accepts the first match.
  //
  // Some examples which will *NOT* match:
  //
  //  patternA = "a*a"
  //  strA     = "aaaa"            <-- All `a`s are consumed by the `a*`.
  //
  //  patternB = "a{3,4}a"
  //  strB     = "aaaa"            <-- All 4 `a`s are consumed by the `a{3, 4}`.
  //
  //  patternC = "(hello|helloworld)"
  //  strC     = "helloworld"      <-- "hello" matches the left or-expression.
  bool Matches(absl::string_view str) const;

  // Generate()
  //
  // Generates a string which matches the underlying pattern.
  //
  // `Generate` does not guarantee any specific distribution of returned
  // results. It does attempt to give a "fair" distribution of lengths to each
  // subcomponent, but no underlying distribution is guaranteed and may change
  // at any time.
  //
  // All randomness comes from `random_engine`.
  absl::StatusOr<std::string> Generate(RandomEngine& random_engine) const;

  // GenerateWithRestrictions()
  //
  // Same as Generate(), except we impose multiple restrictions on the generated
  // string.
  //
  // If `restricted_alphabet` is set, the generated string will only contain
  // characters from that string.
  absl::StatusOr<std::string> GenerateWithRestrictions(
      std::optional<absl::string_view> restricted_alphabet,
      RandomEngine& random_engine) const;

 private:
  explicit SimplePattern(std::string pattern);

  std::string pattern_;
  PatternNode pattern_node_;
};

// Returns the length of the prefix that corresponds to a character set. This
// does not check the validity of the characters inside the [].
//
// Assumes `pattern` has no escape-characters.
absl::StatusOr<int> CharacterSetPrefixLength(absl::string_view pattern);

// Parses the character set body. `chars` should not contain the surrounding
// square braces []. E.g., "[abc]" should just pass "abc". `SetRange(1, 1)` will
// be called on the returned `RepeatedCharSet`.
//
// Assumes `chars` has no escape-characters.
absl::StatusOr<RepeatedCharSet> ParseCharacterSetBody(absl::string_view chars);

// Returns the length of the prefix that corresponds to a repetition. This does
// not check the validity of the characters inside the {}.
//
// Assumes `pattern` has no escape-characters.
absl::StatusOr<int> RepetitionPrefixLength(absl::string_view pattern);

// The return type for `ParseRepetitionBody`.
struct RepetitionRange {
  int64_t min_length;
  int64_t max_length;
};

// Parses the repetition body. `repetition` should be one of: "", '?', '+', '*',
// or {_, _}, where each _ is either empty or an integer.
//
// Assumes `repetition` has no escape-characters.
absl::StatusOr<RepetitionRange> ParseRepetitionBody(
    absl::string_view repetition);

// Parses the prefix that corresponds to a repeated character set, ignoring any
// suffix beyond the first repeated character set. E.g.,
//      "a*b" handles "a*", "a{1,2}b" handles "a{1,2}", "ab" handles "a".
// while these are invalid:
//      "*", "", "(a)".
//
// Assumes `pattern` has no escape-characters.
absl::StatusOr<PatternNode> ParseRepeatedCharSetPrefix(
    absl::string_view pattern);

// Parses the prefix that corresponds to a scope. A scope ends at either
// end-of-string or at the first unmatched ')'. E.g.,
//      "ab" handles "ab", "ab)" handles "ab", "a(b|c)" handles "a(b|c)".
//
// If a whole Pattern is passed in, the entire pattern should appear in the
// returned PatternNode. (i.e., `returned_pattern_node.pattern` == `pattern`).
//
// Assumes `pattern` has no escape-characters.
absl::StatusOr<PatternNode> ParseScopePrefix(absl::string_view pattern);

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_SIMPLE_PATTERN_H_
