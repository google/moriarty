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

#include "src/variables/constraints/string_constraints.h"

#include <string>

#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"

namespace moriarty {

Alphabet::Alphabet(absl::string_view alphabet) : alphabet_(alphabet) {}

std::string Alphabet::GetAlphabet() const { return alphabet_; }

namespace {

constexpr static absl::string_view kAlphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
constexpr static absl::string_view kUpperCase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr static absl::string_view kLowerCase = "abcdefghijklmnopqrstuvwxyz";
constexpr static absl::string_view kNumbers = "0123456789";
constexpr static absl::string_view kAlphaNumeric =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
constexpr static absl::string_view kUpperAlphaNumeric =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
constexpr static absl::string_view kLowerAlphaNumeric =
    "abcdefghijklmnopqrstuvwxyz0123456789";

}  // namespace

Alphabet Alphabet::Letters() { return Alphabet(kAlphabet); }
Alphabet Alphabet::UpperCase() { return Alphabet(kUpperCase); }
Alphabet Alphabet::LowerCase() { return Alphabet(kLowerCase); }
Alphabet Alphabet::Numbers() { return Alphabet(kNumbers); }
Alphabet Alphabet::AlphaNumeric() { return Alphabet(kAlphaNumeric); }
Alphabet Alphabet::UpperAlphaNumeric() { return Alphabet(kUpperAlphaNumeric); }
Alphabet Alphabet::LowerAlphaNumeric() { return Alphabet(kLowerAlphaNumeric); }

std::string Alphabet::ToString() const {
  return absl::Substitute("Alphabet($0)", alphabet_);
}

SimplePattern::SimplePattern(absl::string_view pattern) : pattern_(pattern) {}

std::string SimplePattern::GetPattern() const { return pattern_; }

std::string SimplePattern::ToString() const {
  return absl::Substitute("SimplePattern($0)", pattern_);
}

}  // namespace moriarty
