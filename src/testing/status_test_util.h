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

#ifndef MORIARTY_SRC_TESTING_STATUS_TEST_UTIL_H_
#define MORIARTY_SRC_TESTING_STATUS_TEST_UTIL_H_

// Moriarty Error Space googletest Matchers
//
// These test macros make testing Moriarty code much easier.
//
// For example:
//  EXPECT_THAT(Generate(MInteger().Between(5, 4)), IsGenerationFailure());

#include <optional>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/substitute.h"
#include "src/errors.h"

namespace moriarty_testing {
namespace moriarty_testing_internal {

// Convenience functions to extract status from either absl::Status or
// absl::StatusOr<>. Use this inside of MATCHER if you want to accept either
// `absl::Status` or `absl::StatusOr`.
inline absl::Status GetStatus(const absl::Status& status) { return status; }

template <typename T>
inline absl::Status GetStatus(const absl::StatusOr<T>& status_or) {
  return status_or.status();
}

}  // namespace moriarty_testing_internal

// googletest matcher checking for a Moriarty MisconfiguredError
MATCHER_P(
    IsMisconfigured, expected_missing_item,
    absl::Substitute(
        "$0 a Moriarty error saying that a Moriarty item is misconfigured.",
        (negation ? "is not" : "is"))) {
  absl::Status status = moriarty_testing_internal::GetStatus(arg);
  if (!moriarty::IsMisconfiguredError(status, expected_missing_item)) {
    *result_listener << "received status that is not an MisconfiguredError: "
                     << status;
    return false;
  }

  return true;
}

// googletest matcher checking for a Moriarty UnsatisfiedConstraintError
MATCHER_P(
    IsUnsatisfiedConstraint, substr_in_error_message,
    absl::Substitute("$0 a Moriarty error saying that a value does not satisfy "
                     "the variable's constraints with an error message "
                     "including the substring '$1'",
                     (negation ? "is not" : "is"), substr_in_error_message)) {
  absl::Status status = moriarty_testing_internal::GetStatus(arg);
  if (!moriarty::IsUnsatisfiedConstraintError(status)) {
    *result_listener
        << "received status that is not an UnsatisfiedConstraintError: "
        << status;
    return false;
  }

  return testing::ExplainMatchResult(
      testing::HasSubstr(substr_in_error_message), status.message(),
      result_listener);
}

// googletest matcher checking for a Moriarty ValueNotFoundError
MATCHER_P(IsValueNotFound, expected_variable_name,
          absl::Substitute("$0 a Moriarty error saying that a value for the "
                           "variable `$1` is unknown",
                           (negation ? "is not" : "is"),
                           expected_variable_name)) {
  absl::Status status = moriarty_testing_internal::GetStatus(arg);
  if (!moriarty::IsValueNotFoundError(status)) {
    *result_listener << "received status that is not a ValueNotFoundError: "
                     << status;
    return false;
  }

  std::optional<std::string> unknown_variable_name =
      moriarty::GetUnknownVariableName(status);
  if (!unknown_variable_name.has_value()) {
    *result_listener << "received ValueNotFoundError with no variable name";
    return false;
  }

  if (*unknown_variable_name != expected_variable_name) {
    *result_listener << "actual unknown variable name is `"
                     << *unknown_variable_name << "`";
    return false;
  }

  *result_listener << "is ValueNotFoundError for `" << expected_variable_name
                   << "`";
  return true;
}

// googletest matcher checking for a Moriarty VariableNotFoundError
MATCHER_P(IsVariableNotFound, expected_variable_name,
          absl::Substitute("$0 a Moriarty error saying that no variable with "
                           "the name `$1` is known",
                           (negation ? "is not" : "is"),
                           expected_variable_name)) {
  absl::Status status = moriarty_testing_internal::GetStatus(arg);
  if (!moriarty::IsVariableNotFoundError(status)) {
    *result_listener << "received status that is not a VariableNotFoundError: "
                     << status;
    return false;
  }

  std::optional<std::string> unknown_variable_name =
      moriarty::GetUnknownVariableName(status);
  if (!unknown_variable_name.has_value()) {
    *result_listener << "received VariableNotFoundError with no variable name";
    return false;
  }

  if (*unknown_variable_name != expected_variable_name) {
    *result_listener << "actual unknown variable name is `"
                     << *unknown_variable_name << "`";
    return false;
  }

  *result_listener << "is VariableNotFoundError for `" << expected_variable_name
                   << "`";
  return true;
}

}  // namespace moriarty_testing

#endif  // MORIARTY_SRC_TESTING_STATUS_TEST_UTIL_H_
