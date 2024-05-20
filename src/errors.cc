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

#include "src/errors.h"

#include <optional>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"

namespace moriarty {

namespace {

// The error space name and payload strings are an implementation detail and may
// change at any point in time, so only including in `.cc`. Users should use
// `IsMoriartyError()` and equivalent functions to check if an `absl::Status` is
// from this error space, not check these strings directly.
//
// It is assumed that these contain no whitespace and are the first token in the
// payload string for the corresponding failure.
constexpr absl::string_view kMoriartyErrorSpace = "moriarty";

constexpr absl::string_view kMisconfiguredErrorPayload = "Misconfigured";
// constexpr absl::string_view kNonRetryableGenerationErrorPayload =
//     "NonRetryableGeneration";
// constexpr absl::string_view kRetryableGenerationErrorPayload =
//     "RetryableGeneration";
constexpr absl::string_view kUnsatisfiedConstraintErrorPayload =
    "UnsatisfiedConstraint";
constexpr absl::string_view kValueNotFoundErrorPayload = "ValueNotFound";
constexpr absl::string_view kVariableNotFoundErrorPayload = "VariableNotFound";

absl::Status MakeMoriartyError(absl::StatusCode code, absl::string_view message,
                               absl::string_view payload) {
  absl::Status status = absl::Status(code, message);
  status.SetPayload(kMoriartyErrorSpace, absl::Cord(payload));
  return status;
}

}  // namespace

bool IsMoriartyError(const absl::Status& status) {
  return status.GetPayload(kMoriartyErrorSpace) != std::nullopt;
}

std::optional<std::string> GetUnknownVariableName(const absl::Status& status) {
  std::optional<absl::Cord> payload = status.GetPayload(kMoriartyErrorSpace);
  if (!payload.has_value()) return std::nullopt;

  if (payload->StartsWith(kValueNotFoundErrorPayload)) {
    payload->RemovePrefix(kValueNotFoundErrorPayload.size() + 1);
  } else if (payload->StartsWith(kVariableNotFoundErrorPayload)) {
    payload->RemovePrefix(kVariableNotFoundErrorPayload.size() + 1);
  } else {
    return std::nullopt;
  }

  return std::string(*payload);
}

namespace {

constexpr absl::string_view ToString(InternalConfigurationType type) {
  switch (type) {
    case InternalConfigurationType::kUniverse:
      return "Universe";
    case InternalConfigurationType::kIOConfig:
      return "IOConfig";
    case InternalConfigurationType::kInputStream:
      return "InputStream";
    case InternalConfigurationType::kOutputStream:
      return "OutputStream";
    case InternalConfigurationType::kRandomEngine:
      return "RandomEngine";
    case InternalConfigurationType::kVariableName:
      return "VariableName";
    case InternalConfigurationType::kVariableSet:
      return "VariableSet";
    case InternalConfigurationType::kValueSet:
      return "ValueSet";
    case InternalConfigurationType::kTestCaseMetadata:
      return "TestCaseMetadata";
    case InternalConfigurationType::kGenerationConfig:
      return "GenerationConfig";
    default:
      ABSL_CHECK(false) << "Unknown InternalConfigurationType";
      return "?";
  }
}

}  // namespace

bool IsMisconfiguredError(const absl::Status& status,
                          InternalConfigurationType missing_item) {
  std::optional<absl::Cord> payload = status.GetPayload(kMoriartyErrorSpace);
  if (!payload.has_value()) return false;

  if (!payload->StartsWith(kMisconfiguredErrorPayload)) return false;
  payload->RemovePrefix(kMisconfiguredErrorPayload.size() + 1);

  return payload->StartsWith(ToString(missing_item));
}

absl::Status MisconfiguredError(absl::string_view class_name,
                                absl::string_view function_name,
                                InternalConfigurationType missing_item) {
  return MakeMoriartyError(
      absl::StatusCode::kFailedPrecondition,
      absl::Substitute(
          "Called $0::$1() without `$2` injected. This typically means you are "
          "trying to call some function without using the Moriarty ecosystem "
          "properly.\n\nFor example, if you have a `Generator`, you need to "
          "tell `Moriarty` about it: `M.AddGenerator( ... );`, then Moriarty "
          "is responsible for injecting information into your Generator, then "
          "calling `Generate()`.",
          class_name, function_name, ToString(missing_item)),
      absl::Substitute("$0 $1 $2 $3", kMisconfiguredErrorPayload,
                       ToString(missing_item), class_name, function_name));
}

bool IsUnsatisfiedConstraintError(const absl::Status& status) {
  std::optional<absl::Cord> payload = status.GetPayload(kMoriartyErrorSpace);
  if (!payload.has_value()) return false;

  return *payload == kUnsatisfiedConstraintErrorPayload;
}

absl::Status UnsatisfiedConstraintError(
    absl::string_view constraint_explanation) {
  return MakeMoriartyError(absl::StatusCode::kFailedPrecondition,
                           constraint_explanation,
                           kUnsatisfiedConstraintErrorPayload);
}

absl::Status CheckConstraint(const absl::Status& constraint,
                             absl::string_view constraint_explanation) {
  if (constraint.ok()) return absl::OkStatus();
  return UnsatisfiedConstraintError(
      absl::Substitute("$0; $1", constraint_explanation, constraint.message()));
}

absl::Status CheckConstraint(bool constraint,
                             absl::string_view constraint_explanation) {
  if (constraint) return absl::OkStatus();
  return UnsatisfiedConstraintError(constraint_explanation);
}

bool IsValueNotFoundError(const absl::Status& status) {
  std::optional<absl::Cord> payload = status.GetPayload(kMoriartyErrorSpace);
  if (!payload.has_value()) return false;

  return payload->StartsWith(kValueNotFoundErrorPayload);
}

absl::Status ValueNotFoundError(absl::string_view variable_name) {
  return MakeMoriartyError(
      absl::StatusCode::kNotFound,
      absl::Substitute("Value for `$0` not found", variable_name),
      absl::Substitute("$0 $1", kValueNotFoundErrorPayload, variable_name));
}

bool IsVariableNotFoundError(const absl::Status& status) {
  std::optional<absl::Cord> payload = status.GetPayload(kMoriartyErrorSpace);
  if (!payload.has_value()) return false;

  return payload->StartsWith(kVariableNotFoundErrorPayload);
}

absl::Status VariableNotFoundError(absl::string_view variable_name) {
  return MakeMoriartyError(
      absl::StatusCode::kNotFound,
      absl::Substitute("Unknown variable: `$0`", variable_name),
      absl::Substitute("$0 $1", kVariableNotFoundErrorPayload, variable_name));
}

}  // namespace moriarty
