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

#ifndef MORIARTY_SRC_VARIABLES_CONSTRAINTS_STRING_CONSTRAINTS_H_
#define MORIARTY_SRC_VARIABLES_CONSTRAINTS_STRING_CONSTRAINTS_H_

#include <string>

#include "absl/strings/string_view.h"
#include "src/variables/constraints/base_constraints.h"

namespace moriarty {

// Constraint stating that the string must only contain characters from the
// given alphabet.
class Alphabet : public MConstraint {
 public:
  // The string must only contain characters from the given alphabet.
  explicit Alphabet(absl::string_view alphabet);

  // TODO(darcybest): Consider having allowing a container of chars as well.

  // The string must only contain English letters (A-Z, a-z).
  static Alphabet Letters();
  // The string must only contain uppercase English letters (A-Z).
  static Alphabet UpperCase();
  // The string must only contain lowercase English letters (a-z).
  static Alphabet LowerCase();
  // The string must only contain numbers (0-9).
  static Alphabet Numbers();
  // The string must only contain alpha-numeric digits (A-Z, a-z, 0-9).
  static Alphabet AlphaNumeric();
  // The string must only contain uppercase alpha-numeric digits (A-Z, 0-9).
  static Alphabet UpperAlphaNumeric();
  // The string must only contain lowercase alpha-numeric digits (a-z, 0-9).
  static Alphabet LowerAlphaNumeric();

  // Returns the alphabet that the string must only contain characters from.
  [[nodiscard]] std::string GetAlphabet() const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

 private:
  std::string alphabet_;
};

// Constraint stating that the characters in the string must all be distinct.
class DistinctCharacters : public MConstraint {
 public:
  // The characters in the string must all be distinct.
  explicit DistinctCharacters() = default;
};

// Constraint stating that the string must match this simple pattern.
//
// A "simple pattern" is a subset of normal regex, but acts in a greedy way
// and only allows very basic regex strings.
//
// See src/internal/simple_pattern.h for more details.
//
// TODO(darcybest): Add more specific/user-friendly documentation for simple
// pattern.
class SimplePattern : public MConstraint {
 public:
  // The string must match this simple pattern.
  explicit SimplePattern(absl::string_view pattern);

  // Returns the pattern that the string must match.
  [[nodiscard]] std::string GetPattern() const;

  // Returns a string representation of this constraint.
  [[nodiscard]] std::string ToString() const;

 private:
  std::string pattern_;
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_CONSTRAINTS_STRING_CONSTRAINTS_H_
