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

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "src/internal/random_config.h"
#include "src/internal/random_engine.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {
namespace moriarty_internal {

namespace {

bool IsNonNegativeChar(char ch) { return (ch >= 0 && ch <= 127); }

bool IsSpecialCharacter(char ch) {
  static constexpr absl::string_view kSpecialCharacters = R"(\()[]{}^?*+-|)";
  return absl::StrContains(kSpecialCharacters, ch);
}

bool ValidCharSetRange(absl::string_view range) {
  if (range.size() != 3) return false;
  if (range[1] != '-') return false;
  char a = range[0];
  char b = range[2];
  return (a <= b) && ((std::islower(a) && std::islower(b)) ||
                      (std::isupper(a) && std::isupper(b)) ||
                      (std::isdigit(a) && std::isdigit(b)));
}

}  // namespace

absl::Status RepeatedCharSet::Add(char character) {
  if (!IsNonNegativeChar(character))
    return absl::InvalidArgumentError("Invalid character.");
  if (valid_chars_[character])
    return absl::AlreadyExistsError(
        absl::Substitute("Duplicate character: $0", character));
  valid_chars_[character] = true;
  return absl::OkStatus();
}

void RepeatedCharSet::FlipValidCharacters() { valid_chars_.flip(); }

absl::Status RepeatedCharSet::SetRange(int64_t min, int64_t max) {
  if (min > max) return absl::InvalidArgumentError("Invalid range.");
  if (max < 0) return absl::InvalidArgumentError("Invalid range.");
  min_ = std::max<int64_t>(0, min);
  max_ = max;
  return absl::OkStatus();
}

absl::Status RepeatedCharSet::IsValid(absl::string_view str) const {
  if (str.length() < min_)
    return absl::InvalidArgumentError("String's length is too small.");
  if (str.length() > max_)
    return absl::InvalidArgumentError("String's length is too large.");
  for (char ch : str) {
    if (!IsValidCharacter(ch))
      return absl::InvalidArgumentError("Invalid character.");
  }
  return absl::OkStatus();
}

bool RepeatedCharSet::IsValidCharacter(char character) const {
  return IsNonNegativeChar(character) && valid_chars_[character];
}

absl::StatusOr<int64_t> RepeatedCharSet::LongestValidPrefix(
    absl::string_view str) const {
  int64_t idx = 0;
  while (idx < str.size() && idx < max_) {
    if (!IsNonNegativeChar(str[idx]) || !valid_chars_[str[idx]]) break;
    idx++;
  }

  if (idx < min_)
    return absl::InvalidArgumentError("String's length is too small.");
  return idx;
}

int64_t RepeatedCharSet::MinLength() const { return min_; }

int64_t RepeatedCharSet::MaxLength() const { return max_; }

std::vector<char> RepeatedCharSet::ValidCharacters() const {
  std::vector<char> result;
  for (int c = 0; c < 128; c++)  // Use `int` over `char` to avoid overflow.
    if (valid_chars_[c]) result.push_back(c);
  return result;
}

absl::StatusOr<int> CharacterSetPrefixLength(absl::string_view pattern) {
  if (pattern.empty()) return absl::InvalidArgumentError("Empty pattern.");
  if (pattern[0] != '[') {
    if (IsSpecialCharacter(pattern[0])) {
      return absl::InvalidArgumentError(
          "Invalid character to start character set.");
    }
    return 1;  // Single character
  }

  // The end of the character set is either the first or the second ']' seen
  // (no character may be duplicated in a character set, so it cannot be the
  // 3rd or 4th, etc.). It is the second if '[' does not appear between the
  // first and the second.
  std::optional<int> close_index;
  for (int i = 1; i < pattern.size(); i++) {
    if (pattern[i] == ']') {
      if (close_index.has_value()) {
        close_index = i;
        break;
      }
      close_index = i;
    } else if (pattern[i] == '[') {
      if (close_index.has_value()) break;
    }
  }

  if (!close_index.has_value())
    return absl::InvalidArgumentError("No ']' found to end character set.");
  return *close_index + 1;
}

absl::StatusOr<RepeatedCharSet> ParseCharacterSetBody(absl::string_view chars) {
  if (chars.empty()) return absl::InvalidArgumentError("Empty character set.");

  RepeatedCharSet char_set;
  MORIARTY_RETURN_IF_ERROR(char_set.SetRange(1, 1));

  bool negation = false;
  if (chars[0] == '^') {
    chars.remove_prefix(1);
    if (chars.empty()) {
      MORIARTY_RETURN_IF_ERROR(char_set.Add('^'));
      return char_set;
    }
    negation = true;
  }

  if (chars.back() == '-') {
    MORIARTY_RETURN_IF_ERROR(char_set.Add('-'));
    chars.remove_suffix(1);
  }

  if (absl::StrContains(chars, '[') && absl::StrContains(chars, ']') &&
      chars.find('[') > chars.find(']')) {
    return absl::InvalidArgumentError(
        "The character ']' cannot come after '[' inside a character set.");
  }

  for (int i = 0; i < chars.size(); i++) {
    if (ValidCharSetRange(chars.substr(i, 3))) {
      for (char c = chars[i]; c <= chars[i + 2]; c++) {
        MORIARTY_RETURN_IF_ERROR(char_set.Add(c));
      }
      i += 2;  // Handled chars[i + 1] and chars[i + 2].
      continue;
    }

    if (chars[i] == '-')
      return absl::InvalidArgumentError("Invalid '-' in character set.");

    MORIARTY_RETURN_IF_ERROR(char_set.Add(chars[i]));
  }

  if (negation) char_set.FlipValidCharacters();

  return char_set;
}

absl::StatusOr<int> RepetitionPrefixLength(absl::string_view pattern) {
  if (pattern.empty()) return 0;
  if (pattern[0] == '?' || pattern[0] == '+' || pattern[0] == '*') return 1;
  if (pattern[0] != '{') return 0;

  int idx = pattern.find_first_of('}');
  if (idx == absl::string_view::npos)
    return absl::InvalidArgumentError("No '}' found to end repetition block.");

  return idx + 1;
}

absl::StatusOr<RepetitionRange> ParseRepetitionBody(
    absl::string_view repetition) {
  if (repetition.empty()) return RepetitionRange({1, 1});
  if (repetition.size() == 1) {
    if (repetition[0] == '?') return RepetitionRange({0, 1});
    if (repetition[0] == '+')
      return RepetitionRange({1, std::numeric_limits<int64_t>::max()});
    if (repetition[0] == '*')
      return RepetitionRange({0, std::numeric_limits<int64_t>::max()});

    return absl::InvalidArgumentError("Invalid repetition block.");
  }

  if (repetition.front() != '{' || repetition.back() != '}')
    return absl::InvalidArgumentError("Invalid repetition block.");
  repetition.remove_prefix(1);
  repetition.remove_suffix(1);

  absl::string_view min_str = repetition;
  absl::string_view max_str = repetition;
  if (absl::StrContains(repetition, ',')) {
    min_str = repetition.substr(0, repetition.find(','));
    max_str = repetition.substr(repetition.find(',') + 1);
  }

  RepetitionRange result = {0, std::numeric_limits<int64_t>::max()};
  if (!min_str.empty()) {
    if (!absl::SimpleAtoi(min_str, &result.min_length))
      return absl::InvalidArgumentError("Invalid min value in repetition.");
  }
  if (!max_str.empty()) {
    if (!absl::SimpleAtoi(max_str, &result.max_length))
      return absl::InvalidArgumentError("Invalid max value in repetition.");
  }

  return result;
}

absl::StatusOr<PatternNode> ParseRepeatedCharSetPrefix(
    absl::string_view pattern) {
  MORIARTY_ASSIGN_OR_RETURN(int char_set_len,
                            CharacterSetPrefixLength(pattern));
  absl::string_view chars = pattern.substr(0, char_set_len);
  if (chars.size() >= 2 && chars.front() == '[' && chars.back() == ']') {
    chars = chars.substr(1, chars.size() - 2);
  }
  MORIARTY_ASSIGN_OR_RETURN(RepeatedCharSet char_set,
                            ParseCharacterSetBody(chars));

  MORIARTY_ASSIGN_OR_RETURN(
      int repetition_len, RepetitionPrefixLength(pattern.substr(char_set_len)));
  MORIARTY_ASSIGN_OR_RETURN(
      RepetitionRange repetition,
      ParseRepetitionBody(pattern.substr(char_set_len, repetition_len)));

  MORIARTY_RETURN_IF_ERROR(
      char_set.SetRange(repetition.min_length, repetition.max_length));

  PatternNode result;
  result.repeated_character_set = std::move(char_set);
  result.pattern = pattern.substr(0, char_set_len + repetition_len);

  return result;
}

namespace {

absl::StatusOr<PatternNode> ParseAllOfNodeScopePrefix(
    absl::string_view pattern) {
  // The `allof_node` holds the concatenated elements. E.g., "a*(b|c)d" will
  // store 3 elements in `allof_node` ("a*", "(b|c)", "d").
  PatternNode allof_node;
  allof_node.subpattern_type = PatternNode::SubpatternType::kAllOf;

  std::size_t idx = 0;
  while (idx < pattern.size() && pattern[idx] != '|' && pattern[idx] != ')') {
    if (pattern[idx] != '(') {
      MORIARTY_ASSIGN_OR_RETURN(
          PatternNode char_set,
          ParseRepeatedCharSetPrefix(pattern.substr(idx)));
      allof_node.subpatterns.push_back(char_set);
      idx += char_set.pattern.size();
      continue;
    }

    MORIARTY_ASSIGN_OR_RETURN(
        PatternNode inner_scope,
        ParseScopePrefix(pattern.substr(idx + 1)));  // +1 for '('
    std::size_t inner_size = inner_scope.pattern.size();
    if (idx + 1 + inner_size >= pattern.size() ||
        pattern[idx + 1 + inner_size] != ')')
      return absl::InvalidArgumentError("Invalid end of scope. Expected ')'.");

    inner_scope.pattern = pattern.substr(idx, inner_size + 2);
    allof_node.subpatterns.push_back(std::move(inner_scope));
    idx += inner_size + 2;  // +2 for '(' and ')'
  }

  allof_node.pattern = pattern.substr(0, idx);
  return allof_node;
}

// Converts "\\" -> "\" and "\ " -> " ". And removes empty spaces (does not
// remove other whitespace characters).
absl::StatusOr<std::string> Sanitize(absl::string_view pattern) {
  std::string sanitized_pattern;
  for (int i = 0; i < pattern.size(); i++) {
    if (pattern[i] == '\\') {
      if (i + 1 == pattern.size()) {
        return absl::InvalidArgumentError(
            "Cannot have unescaped '\\' at the end of pattern.");
      }
      if (pattern[i + 1] != '\\' && pattern[i + 1] != ' ') {
        return absl::InvalidArgumentError(absl::Substitute(
            "Invalid escaped character in pattern: '\\$0'", pattern[i + 1]));
      }
      absl::StrAppend(&sanitized_pattern, pattern.substr(i + 1, 1));
      i++;
      continue;
    }
    if (pattern[i] != ' ')
      absl::StrAppend(&sanitized_pattern, pattern.substr(i, 1));
  }
  return sanitized_pattern;
}

}  // namespace

absl::StatusOr<PatternNode> ParseScopePrefix(absl::string_view pattern) {
  if (pattern.empty() || pattern[0] == ')')
    return absl::InvalidArgumentError("Empty scope.");

  // The `anyof_node` holds the top-level options. E.g., "ab|c(d|e)|f" will
  // store 3 elements in `anyof_node` ("ab", "c(d|e)", "f").
  PatternNode anyof_node;
  anyof_node.subpattern_type = PatternNode::SubpatternType::kAnyOf;

  std::size_t idx = 0;
  while (idx < pattern.size() && pattern[idx] != ')') {
    if (pattern[idx] == '|') {
      if (idx == 0 || idx + 1 >= pattern.size() || pattern[idx + 1] == '|')
        return absl::InvalidArgumentError("Empty or-block not allowed.");
      idx++;
    }
    MORIARTY_ASSIGN_OR_RETURN(PatternNode allof_node,
                              ParseAllOfNodeScopePrefix(pattern.substr(idx)));
    idx += allof_node.pattern.size();
    anyof_node.subpatterns.push_back(std::move(allof_node));
  }

  // If there is only one subpattern with `kAnyOf`, flatten it.
  if (anyof_node.subpatterns.size() == 1) return anyof_node.subpatterns[0];
  anyof_node.pattern = pattern.substr(0, idx);
  return anyof_node;
}

absl::StatusOr<int64_t> MatchesPrefixLength(const PatternNode& pattern_node,
                                            absl::string_view str) {
  MORIARTY_ASSIGN_OR_RETURN(
      int64_t prefix_length,
      pattern_node.repeated_character_set.LongestValidPrefix(str));
  str.remove_prefix(prefix_length);

  for (const PatternNode& subpattern : pattern_node.subpatterns) {
    absl::StatusOr<int64_t> subpattern_length =
        MatchesPrefixLength(subpattern, str);
    if (!subpattern_length.ok()) {
      if (pattern_node.subpattern_type == PatternNode::SubpatternType::kAllOf) {
        return absl::InvalidArgumentError("Invalid subpattern.");
      }
      continue;  // We are in a kAnyOf, so we don't *have* to match this.
    }

    prefix_length += *subpattern_length;

    if (pattern_node.subpattern_type == PatternNode::SubpatternType::kAnyOf) {
      return prefix_length;
    }

    str.remove_prefix(*subpattern_length);
  }

  if (pattern_node.subpattern_type == PatternNode::SubpatternType::kAnyOf) {
    // We are in a kAnyOf, but we didn't match anything...
    return absl::InvalidArgumentError("Invalid subpattern.");
  }
  return prefix_length;
}

absl::StatusOr<SimplePattern> SimplePattern::Create(absl::string_view pattern) {
  MORIARTY_ASSIGN_OR_RETURN(std::string sanitized_pattern, Sanitize(pattern));
  if (sanitized_pattern.empty())
    return absl::InvalidArgumentError("Empty pattern.");

  // Put the string into the SimplePattern immediately so that all
  // `string_view`s point to the correct memory.
  SimplePattern simple_pattern(std::move(sanitized_pattern));
  MORIARTY_ASSIGN_OR_RETURN(simple_pattern.pattern_node_,
                            ParseScopePrefix(simple_pattern.pattern_));

  if (simple_pattern.pattern_node_.pattern != simple_pattern.pattern_) {
    return absl::InvalidArgumentError(
        "Invalid pattern. Extra characters found.");
  }

  return simple_pattern;
}

SimplePattern::SimplePattern(std::string pattern)
    : pattern_(std::move(pattern)) {}

std::string SimplePattern::Pattern() const { return pattern_; }

bool SimplePattern::Matches(absl::string_view str) const {
  absl::StatusOr<int64_t> prefix_length =
      MatchesPrefixLength(pattern_node_, str);
  return prefix_length.ok() && *prefix_length == str.length();
}

namespace {

absl::StatusOr<std::string> GenerateRepeatedCharSet(
    const RepeatedCharSet& char_set,
    std::optional<absl::string_view> restricted_alphabet,
    RandomEngine& random_engine) {
  if (char_set.MaxLength() == std::numeric_limits<int64_t>::max()) {
    return absl::InvalidArgumentError(
        "Cannot generate with `*` or `+` or large lengths.");
  }
  MORIARTY_ASSIGN_OR_RETURN(
      int64_t len,
      random_engine.RandInt(char_set.MinLength(), char_set.MaxLength()));

  RepeatedCharSet restricted_char_set;
  if (restricted_alphabet.has_value()) {
    for (char c : *restricted_alphabet) {
      MORIARTY_RETURN_IF_ERROR(restricted_char_set.Add(c));
    }
  } else {
    restricted_char_set.FlipValidCharacters();  // Allow all characters.
  }
  std::vector<char> valid_chars;
  for (char c : char_set.ValidCharacters()) {
    if (restricted_char_set.IsValidCharacter(c)) valid_chars.push_back(c);
  }

  if (valid_chars.empty()) {
    // No valid characters, so the only valid string is the empty string.
    if (char_set.MinLength() <= 0) return "";
    return absl::InvalidArgumentError(
        "No valid characters for generation, but empty string is not "
        "allowed.");
  }

  MORIARTY_ASSIGN_OR_RETURN(
      std::vector<char> result,
      RandomElementsWithReplacement(random_engine, valid_chars, len));
  return std::string(result.begin(), result.end());
}

absl::StatusOr<std::string> GeneratePatternNode(
    const PatternNode& node,
    std::optional<absl::string_view> restricted_alphabet,
    RandomEngine& random_engine) {
  MORIARTY_ASSIGN_OR_RETURN(
      std::string result,
      GenerateRepeatedCharSet(node.repeated_character_set, restricted_alphabet,
                              random_engine));

  if (node.subpatterns.empty()) return result;

  if (node.subpattern_type == PatternNode::SubpatternType::kAnyOf) {
    MORIARTY_ASSIGN_OR_RETURN(int64_t idx,
                              random_engine.RandInt(node.subpatterns.size()));
    MORIARTY_ASSIGN_OR_RETURN(
        std::string subresult,
        GeneratePatternNode(node.subpatterns[idx], restricted_alphabet,
                            random_engine));
    return absl::StrCat(result, subresult);
  }

  for (const PatternNode& subpattern : node.subpatterns) {
    MORIARTY_ASSIGN_OR_RETURN(
        std::string subresult,
        GeneratePatternNode(subpattern, restricted_alphabet, random_engine));
    absl::StrAppend(&result, subresult);
  }
  return result;
}

}  // namespace

absl::StatusOr<std::string> SimplePattern::Generate(
    RandomEngine& random_engine) const {
  return GenerateWithRestrictions(/* restricted_alphabet = */ std::nullopt,
                                  random_engine);
}

absl::StatusOr<std::string> SimplePattern::GenerateWithRestrictions(
    std::optional<absl::string_view> restricted_alphabet,
    RandomEngine& random_engine) const {
  return GeneratePatternNode(pattern_node_, restricted_alphabet, random_engine);
}

}  // namespace moriarty_internal
}  // namespace moriarty
