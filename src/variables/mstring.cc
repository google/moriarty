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
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "src/errors.h"
#include "src/internal/random_engine.h"
#include "src/internal/simple_pattern.h"
#include "src/librarian/io_config.h"
#include "src/librarian/mvariable.h"
#include "src/property.h"
#include "src/util/status_macro/status_macros.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/container_constraints.h"
#include "src/variables/constraints/size_constraints.h"
#include "src/variables/constraints/string_constraints.h"
#include "src/variables/minteger.h"

namespace moriarty {

using moriarty::librarian::IOConfig;

MString& MString::AddConstraint(const Exactly<std::string>& constraint) {
  return Is(constraint.GetValue());
}

MString& MString::AddConstraint(const Length& constraint) {
  return OfLength(constraint.GetConstraints());
}

MString& MString::AddConstraint(const Alphabet& constraint) {
  return WithAlphabet(constraint.GetAlphabet());
}

MString& MString::AddConstraint(const DistinctCharacters& constraint) {
  return WithDistinctCharacters();
}

MString& MString::AddConstraint(const SimplePattern& constraint) {
  return WithSimplePattern(constraint.GetPattern());
}

MString& MString::AddConstraint(const SizeCategory& constraint) {
  return AddConstraint(Length(constraint));
}

MString& MString::OfLength(const MInteger& length) {
  if (length_)
    length_->MergeFrom(length);
  else
    length_ = length;
  return *this;
}

MString& MString::OfLength(int64_t length) { return OfLength(length, length); }

MString& MString::OfLength(absl::string_view length_expression) {
  return OfLength(length_expression, length_expression);
}

MString& MString::OfLength(int64_t min_length, int64_t max_length) {
  return OfLength(MInteger().Between(min_length, max_length));
}

MString& MString::OfLength(int64_t min_length,
                           absl::string_view max_length_expression) {
  return OfLength(MInteger().Between(min_length, max_length_expression));
}

MString& MString::OfLength(absl::string_view min_length_expression,
                           int64_t max_length) {
  return OfLength(MInteger().Between(min_length_expression, max_length));
}

MString& MString::OfLength(absl::string_view min_length_expression,
                           absl::string_view max_length_expression) {
  return OfLength(
      MInteger().Between(min_length_expression, max_length_expression));
}

MString& MString::WithAlphabet(absl::string_view valid_characters) {
  // If `valid_characters` isn't sorted or contains multiple occurrences of
  // the same letter, we need to create a new string in order to sort the
  // characters.
  std::string valid_character_str;
  if (!std::is_sorted(std::begin(valid_characters),
                      std::end(valid_characters)) ||
      std::adjacent_find(std::begin(valid_characters),
                         std::end(valid_characters)) !=
          std::end(valid_characters)) {
    valid_character_str = valid_characters;

    // Sort
    std::sort(valid_character_str.begin(), valid_character_str.end());
    // Remove duplicates
    valid_character_str.erase(
        std::unique(valid_character_str.begin(), valid_character_str.end()),
        valid_character_str.end());
    valid_characters = valid_character_str;
  }

  if (!alphabet_) {
    alphabet_ = valid_characters;
  } else {  // Do set intersection of the two alphabets
    alphabet_->erase(std::set_intersection(
                         std::begin(valid_characters),
                         std::end(valid_characters), std::begin(*alphabet_),
                         std::end(*alphabet_), std::begin(*alphabet_)),
                     std::end(*alphabet_));
  }

  return *this;
}

MString& MString::WithDistinctCharacters() {
  distinct_characters_ = true;
  return *this;
}

MString& MString::WithSimplePattern(absl::string_view simple_pattern) {
  // TODO(darcybest): Do not crash, but set global "invalid state flag".
  if (simple_pattern_) {
    ABSL_CHECK(simple_pattern_->Pattern() == simple_pattern)
        << "Merging two incompatible simple patterns";
    return *this;
  }
  absl::StatusOr<moriarty_internal::SimplePattern> pattern =
      moriarty_internal::SimplePattern::Create(simple_pattern);
  ABSL_CHECK_OK(pattern) << "Error creating simple pattern";
  simple_pattern_ = *pattern;
  return *this;
}

absl::Status MString::MergeFromImpl(const MString& other) {
  if (other.length_) OfLength(*other.length_);
  if (other.alphabet_) WithAlphabet(*other.alphabet_);
  distinct_characters_ = other.distinct_characters_;
  if (other.simple_pattern_)
    WithSimplePattern(other.simple_pattern_->Pattern());

  return absl::OkStatus();
}

absl::Status MString::IsSatisfiedWithImpl(const std::string& value) const {
  if (length_) {
    MORIARTY_RETURN_IF_ERROR(
        CheckConstraint(SatisfiesConstraints(*length_, value.length()),
                        "length of string is invalid"));
  }

  if (alphabet_) {
    for (char c : value) {
      MORIARTY_RETURN_IF_ERROR(CheckConstraint(
          absl::c_binary_search(*alphabet_, c),
          absl::Substitute("character '$0' not in alphabet, but in '$1'", c,
                           value)));
    }
  }

  if (distinct_characters_) {
    absl::flat_hash_set<char> seen;
    for (char c : value) {
      auto [it, inserted] = seen.insert(c);
      MORIARTY_RETURN_IF_ERROR(CheckConstraint(
          inserted,
          absl::Substitute(
              "Characters are not distinct. '$0' appears multiple times.", c)));
    }
  }

  if (simple_pattern_) {
    MORIARTY_RETURN_IF_ERROR(CheckConstraint(
        simple_pattern_->Matches(value),
        absl::Substitute("string '$0' does not match simple pattern '$1'",
                         value, simple_pattern_->Pattern())));
  }

  return absl::OkStatus();
}

absl::StatusOr<std::vector<MString>> MString::GetDifficultInstancesImpl()
    const {
  if (!length_) {
    return absl::FailedPreconditionError(
        "Attempting to get difficult instances of a string with no "
        "length parameter given.");
  }
  std::vector<MString> values = {};
  MORIARTY_ASSIGN_OR_RETURN(std::vector<MInteger> lengthCases,
                            length_->GetDifficultInstances());

  values.reserve(lengthCases.size());
  for (const auto& c : lengthCases) {
    values.push_back(MString().OfLength(c));
  }
  // TODO(hivini): Add alphabet cases, pattern cases and integrate
  // SimplePattern.
  // TODO(darcybest): Add Thue-Morse sequence.
  return values;
}

absl::StatusOr<std::string> MString::GenerateImpl() {
  if (!simple_pattern_ && (!alphabet_ || alphabet_->empty())) {
    return absl::FailedPreconditionError(
        "Attempting to generate a string with an empty alphabet and no simple "
        "pattern.");
  }
  if (!simple_pattern_ && !length_) {
    return absl::FailedPreconditionError(
        "Attempting to generate a string with no length parameter or simple "
        "pattern given.");
  }

  if (simple_pattern_) return GenerateSimplePattern();

  // Negative string length is impossible.
  length_->AtLeast(0);

  if (length_size_property_) {
    MORIARTY_RETURN_IF_ERROR(length_->OfSizeProperty(*length_size_property_));
  }

  std::optional<int64_t> generation_limit =
      this->GetApproximateGenerationLimit();

  if (generation_limit) {
    length_->AtMost(*generation_limit);
  }

  if (distinct_characters_) return GenerateImplWithDistinctCharacters();

  MORIARTY_ASSIGN_OR_RETURN(int length, Random("length", *length_),
                            _ << "Error determining the length of the string");

  std::vector<char> alphabet(alphabet_->begin(), alphabet_->end());
  MORIARTY_ASSIGN_OR_RETURN(std::vector<char> ret,
                            RandomElementsWithReplacement(alphabet, length));

  return std::string(ret.begin(), ret.end());
}

absl::StatusOr<std::string> MString::GenerateSimplePattern() {
  ABSL_CHECK(simple_pattern_);

  // moriarty::MString needs direct access its RandomEngine. Non
  // built-in types should not access the RandomEngine directly, use
  // Random(MInteger().Between(x, y)) instead.
  moriarty_internal::RandomEngine& rng =
      moriarty_internal::MVariableManager(this).GetRandomEngine();
  return simple_pattern_->GenerateWithRestrictions(alphabet_, rng);
}

absl::StatusOr<std::string> MString::GenerateImplWithDistinctCharacters() {
  // Creating a copy in case the alphabet changes in the future, we don't want
  // to limit the length forever.
  MInteger mlength = *length_;
  mlength.AtMost(alphabet_->size());  // Each char appears at most once.
  MORIARTY_ASSIGN_OR_RETURN(int length, Random("length", mlength),
                            _ << "Error determining the length of the string");

  std::vector<char> alphabet(alphabet_->begin(), alphabet_->end());
  MORIARTY_ASSIGN_OR_RETURN(std::vector<char> ret,
                            RandomElementsWithoutReplacement(alphabet, length));

  return std::string(ret.begin(), ret.end());
}

absl::Status MString::OfSizeProperty(Property property) {
  length_size_property_ = std::move(property);
  return absl::OkStatus();
}

std::vector<std::string> MString::GetDependenciesImpl() const {
  return length_ ? GetDependencies(*length_) : std::vector<std::string>();
}

absl::StatusOr<std::string> MString::ReadImpl() {
  MORIARTY_ASSIGN_OR_RETURN(IOConfig * io_config, GetIOConfig());
  return io_config->ReadToken();
}

absl::Status MString::PrintImpl(const std::string& value) {
  MORIARTY_ASSIGN_OR_RETURN(IOConfig * io_config, GetIOConfig());
  return io_config->PrintToken(value);
}

std::string MString::ToStringImpl() const {
  std::string result;
  if (length_) absl::StrAppend(&result, "length: ", length_->ToString(), "; ");
  if (alphabet_) absl::StrAppend(&result, "alphabet: ", *alphabet_, "; ");
  if (distinct_characters_)
    absl::StrAppend(&result, "Only distinct characters; ");
  if (simple_pattern_)
    absl::StrAppend(&result, "simple_pattern: ", simple_pattern_->Pattern(),
                    "; ");
  if (length_size_property_.has_value())
    absl::StrAppend(&result, "length: ", length_size_property_->ToString(),
                    "; ");
  return result;
}

absl::StatusOr<std::string> MString::ValueToStringImpl(
    const std::string& value) const {
  return value;
}

}  // namespace moriarty
