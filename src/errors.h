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

#ifndef MORIARTY_SRC_ERRORS_H_
#define MORIARTY_SRC_ERRORS_H_

// Moriarty Error Space
//
// Moriarty uses `absl::Status` as its canonical way to pass errors.
//
// In several locations throughout Moriarty, it is required that a Moriarty
// Error is used instead of a generic `absl::Status`. To create a Moriarty
// Error, you must call an `FooError()` function below. And you can check if a
// status is a specific  error using `IsFooError`. There are googletest matchers
// available in testing/status_test_util.h for unit tests (named `IsFoo`).
//
// To be clear: a Moriarty status is an `absl::Status` with information
// injected into it. You should not depend on any message or implementation
// details of how we are dealing with these. They may (and likely will) change
// in the future.
//
// Types of Errors (this list may grow in the future):
//
//  * MisconfiguredError
//  * NonRetryableGenerationError
//  * RetryableGenerationError
//  * UnsatisfiedConstraintError
//  * ValueNotFoundError
//  * VariableNotFoundError

#include <optional>
#include <string>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"

namespace moriarty {

// IsMoriartyError()
//
// Returns true if `status` is not `OkStatus()` and was generated via Moriarty.
bool IsMoriartyError(const absl::Status& status);

// GetUnknownVariableName()
//
// If `status` is a `ValueNotFoundError` or `VariableNotFoundError`, returns the
// name of the variable that wasn't found (if known). If the status is not an
// error listed above or the name is unknown, returns `std::nullopt`.
std::optional<std::string> GetUnknownVariableName(const absl::Status& status);

// -----------------------------------------------------------------------------
//   Misconfigured -- Some part of Moriarty was misconfigured

// Items that may be misconfigured by the user. In general, if you're seeing
// this, it means that you have not set up your ecosystem properly.
enum class InternalConfigurationType {
  kUniverse,
  kIOConfig,
  kInputStream,
  kOutputStream,
  kRandomEngine,
  kVariableName,
  kVariableSet,
  kValueSet,
  kTestCaseMetadata,
  kGenerationConfig,
  kThisIsNotAnExhaustiveListItMayGrowOverTime
};

// IsMisconfiguredError()
//
// Returns true if `status` signals that some part of Moriarty was
// misconfigured and missing `missing_item`.
bool IsMisconfiguredError(const absl::Status& status,
                          InternalConfigurationType missing_item);

// MisconfiguredError()
//
// Returns an error signaling that some `missing_item` is not in `class_name`.
//
// If you receive this error, it typically means you are trying to call some
// function without using the Moriarty ecosystem. For example, if you have a
// `Generator`, you need to tell `Moriarty` about it: `M.AddGenerator( ... );`,
// then Moriarty is responsible for injecting information into your Generator,
// then calling `Generate()`.
//
// In the example above, `AddGenerator` will seed your generator with
// appropriate information (random seed, variables, etc). Without these, your
// Generator does not behave as expected.
absl::Status MisconfiguredError(absl::string_view class_name,
                                absl::string_view function_name,
                                InternalConfigurationType missing_item);

// -----------------------------------------------------------------------------
//   UnsatisfiedConstraint -- Some constraint on a variable was not satisfied.

// IsUnsatisfiedConstraintError()
//
// Returns true if `status` signals that a constraint on some variable was not
// satisfied.
bool IsUnsatisfiedConstraintError(const absl::Status& status);

// UnsatisfiedConstraintError()
//
// Returns a status that states that this constraint is not satisfied. The
// `constraint_explanation` will be shown to the user if requested.
absl::Status UnsatisfiedConstraintError(
    absl::string_view constraint_explanation);

// CheckConstraint()
//
// Convenience function basically equivalent to:
//  if (constraint.ok()) return absl::OkStatus();
//  return UnsatisfiedConstraintError(
//                               constraint_explanation + constraint.message());
//
// Useful when mixed with MORIARTY_RETURN_IF_ERROR.
absl::Status CheckConstraint(const absl::Status& constraint,
                             absl::string_view constraint_explanation);

// CheckConstraint()
//
// Convenience function basically equivalent to:
//  if (constraint) return absl::OkStatus();
//  return UnsatisfiedConstraintError(constraint_explanation);
//
// Useful when mixed with MORIARTY_RETURN_IF_ERROR.
absl::Status CheckConstraint(bool constraint,
                             absl::string_view constraint_explanation);

// -----------------------------------------------------------------------------
//   ValueNotFound -- when a variable is known, but no value is known for it.

// IsValueNotFoundError()
//
// Returns true if `status` signals that the value of a variable is not known.
bool IsValueNotFoundError(const absl::Status& status);

// ValueNotFoundError()
//
// Returns a status specifying that a specific value for `variable_name` is not
// known. This normally implies that the MVariable with the same name is known,
// but its value is not.
//
// For example, this error indicates that we know an MInteger exists with the
// name "N", but we don't know an integer value for "N".
//
// It is not typical for users to need to use this function.
absl::Status ValueNotFoundError(absl::string_view variable_name);

// -----------------------------------------------------------------------------
//   VariableNotFound -- when a variable is unknown

// IsVariableNotFoundError()
//
// Returns true if `status` signals that a variable not known.
bool IsVariableNotFoundError(const absl::Status& status);

// VariableNotFoundError()
//
// Returns a status specifying that `variable_name` is not known. In contexts
// where variables and values may both be known, this normally means that no
// variable or value is known for this name.
//
// It is not typical for users to need to use this function.
absl::Status VariableNotFoundError(absl::string_view variable_name);

}  // namespace moriarty

#endif  // MORIARTY_SRC_ERRORS_H_
