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

#ifndef MORIARTY_SRC_VARIABLES_MSTRING_H_
#define MORIARTY_SRC_VARIABLES_MSTRING_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "src/internal/simple_pattern.h"
#include "src/librarian/mvariable.h"
#include "src/property.h"
#include "src/variables/minteger.h"

namespace moriarty {

// MString
//
// The native string component for Moriarty.
class MString : public moriarty::librarian::MVariable<MString, std::string> {
 public:
  // Common alphabets to use for `WithAlphabet()`.
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

  MString();

  [[nodiscard]] std::string Typename() const override { return "MString"; }

  // OfLength()
  //
  // Sets the constraints for the length of this string. If two parameters are
  // provided, the length is between the first parameter and the second.
  //
  // For example:
  // `OfLength(5, 10)` is equivalent to `OfLength(MInteger().Between(5, 10))`.
  // `OfLength("3 * N")` is equivalent to `OfLength(MInteger().Is("3 * N"))`.
  MString& OfLength(const MInteger& length);
  MString& OfLength(int64_t length);
  MString& OfLength(absl::string_view length_expression);
  MString& OfLength(int64_t min_length, int64_t max_length);
  MString& OfLength(int64_t min_length,
                    absl::string_view max_length_expression);
  MString& OfLength(absl::string_view min_length_expression,
                    int64_t max_length);
  MString& OfLength(absl::string_view min_length_expression,
                    absl::string_view max_length_expression);

  // WithAlphabet()
  //
  // Sets the valid set of characters that this string will be made of
  //
  // TODO(b/208296393): We may wish for this to be a set of chars in the future,
  // but for now, this is much easier for the user to write. We may also want
  // enums for common types (decimal, hex, lower, upper, alpha, alphanumeric,
  // etc).
  MString& WithAlphabet(absl::string_view valid_characters);

  // WithDistinctCharacters()
  //
  // States that this string must contain only distinct characters. By default,
  // this restriction is off, and does not guarantee duplicate entries will
  // occur.
  MString& WithDistinctCharacters();

  // OfSizeProperty()
  //
  // Tells this string to have a specific size/length. `property.category` must
  // be "size". The exact values here are not guaranteed and may change over
  // time. If exact values are required, specify them manually.
  absl::Status OfSizeProperty(Property property);

  // WithSimplePattern()
  //
  // Tells this string to have a simple pattern.
  //
  // A "simple pattern" is a subset of normal regex, but acts in a greedy way
  // and only allows very basic regex strings.
  // TODO(darcybest): Add more specific documentation for simple pattern.
  MString& WithSimplePattern(absl::string_view simple_pattern);

 private:
  // A value of length must be set prior to GenerateValue() being called.
  std::optional<MInteger> length_;

  // alphabet_ is a sorted list of characters that are valid to be in the
  // string.
  std::optional<std::string> alphabet_;

  bool distinct_characters_ = false;

  std::optional<Property> length_size_property_;

  std::optional<moriarty_internal::SimplePattern> simple_pattern_;

  absl::StatusOr<std::string> GenerateSimplePattern();

  // GenerateImplWithDistinctCharacters()
  //
  // Same as GenerateImpl(), but with distinct characters.
  absl::StatusOr<std::string> GenerateImplWithDistinctCharacters();

  // ---------------------------------------------------------------------------
  //  MVariable overrides
  absl::StatusOr<std::string> GenerateImpl() override;
  absl::Status IsSatisfiedWithImpl(const std::string& value) const override;
  absl::Status MergeFromImpl(const MString& other) override;
  absl::StatusOr<std::string> ReadImpl() override;
  absl::Status PrintImpl(const std::string& value) override;
  std::vector<std::string> GetDependenciesImpl() const override;
  absl::StatusOr<std::vector<MString>> GetDifficultInstancesImpl()
      const override;
  std::string ToStringImpl() const override;
  absl::StatusOr<std::string> ValueToStringImpl(
      const std::string& value) const override;
  // ---------------------------------------------------------------------------
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_MSTRING_H_
