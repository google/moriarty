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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"

namespace moriarty {
namespace {

using ::moriarty_testing::IsMisconfigured;
using ::moriarty_testing::IsUnsatisfiedConstraint;
using ::moriarty_testing::IsValueNotFound;
using ::moriarty_testing::IsVariableNotFound;
using ::testing::Not;

// -----------------------------------------------------------------------------
//  General Moriarty Error space checks

TEST(StatusTest, IsMoriartyErrorAcceptsAllErrorFunctions) {
  EXPECT_TRUE(IsMoriartyError(
      MisconfiguredError("", "", InternalConfigurationType::kUniverse)));
  // EXPECT_TRUE(IsMoriartyError(NonRetryableGenerationError("")));
  // EXPECT_TRUE(IsMoriartyError(RetryableGenerationError("")));
  EXPECT_TRUE(IsMoriartyError(UnsatisfiedConstraintError("")));
  EXPECT_TRUE(IsMoriartyError(ValueNotFoundError("")));
  EXPECT_TRUE(IsMoriartyError(VariableNotFoundError("")));
}

TEST(StatusTest, IsMoriartyErrorDoesNotAcceptOkStatus) {
  EXPECT_FALSE(IsMoriartyError(absl::OkStatus()));
}

TEST(StatusTest, IsMoriartyErrorDoesNotAcceptGenericStatuses) {
  EXPECT_FALSE(IsMoriartyError(absl::AlreadyExistsError("")));
  EXPECT_FALSE(IsMoriartyError(absl::FailedPreconditionError("")));
  EXPECT_FALSE(IsMoriartyError(absl::InvalidArgumentError("")));
  EXPECT_FALSE(IsMoriartyError(absl::NotFoundError("")));
  EXPECT_FALSE(IsMoriartyError(absl::UnknownError("")));
}

TEST(StatusTest, MoriartyErrorsRecognizeAppropriateError) {
  EXPECT_TRUE(IsMisconfiguredError(
      MisconfiguredError("a", "b", InternalConfigurationType::kUniverse),
      InternalConfigurationType::kUniverse));
  // EXPECT_TRUE(IsNonRetryableGenerationError(NonRetryableGenerationError("")));
  // EXPECT_TRUE(IsRetryableGenerationError(RetryableGenerationError("")));
  EXPECT_TRUE(IsUnsatisfiedConstraintError(UnsatisfiedConstraintError("")));
  EXPECT_TRUE(IsValueNotFoundError(ValueNotFoundError("")));
  EXPECT_TRUE(IsVariableNotFoundError(VariableNotFoundError("")));
}

TEST(StatusTest, MoriartyErrorsDoNotAcceptUnderlyingTypesAlone) {
  // All "" are irrelevant, just creating empty statuses with a specific code.
  EXPECT_FALSE(IsMisconfiguredError(
      absl::Status(
          MisconfiguredError("", "", InternalConfigurationType::kUniverse)
              .code(),
          ""),
      InternalConfigurationType::kUniverse));
  // EXPECT_FALSE(IsNonRetryableGenerationError(
  //     absl::Status(NonRetryableGenerationError("").code(), "")));
  // EXPECT_FALSE(IsRetryableGenerationError(
  //     absl::Status(RetryableGenerationError("").code(), "")));
  EXPECT_FALSE(IsUnsatisfiedConstraintError(
      absl::Status(UnsatisfiedConstraintError("").code(), "")));
  EXPECT_FALSE(
      IsValueNotFoundError(absl::Status(ValueNotFoundError("").code(), "")));
  EXPECT_FALSE(IsVariableNotFoundError(
      absl::Status(VariableNotFoundError("").code(), "")));
}

TEST(StatusTest, MoriartyErrorsDoNotAcceptOkStatus) {
  EXPECT_FALSE(IsMisconfiguredError(absl::OkStatus(),
                                    InternalConfigurationType::kUniverse));
  // EXPECT_FALSE(IsNonRetryableGenerationError(absl::OkStatus()));
  // EXPECT_FALSE(IsRetryableGenerationError(absl::OkStatus()));
  EXPECT_FALSE(IsUnsatisfiedConstraintError(absl::OkStatus()));
  EXPECT_FALSE(IsValueNotFoundError(absl::OkStatus()));
  EXPECT_FALSE(IsVariableNotFoundError(absl::OkStatus()));
}

// -----------------------------------------------------------------------------
//  GetUnknownVariableName

TEST(StatusTest, GetUnknownVariableNameFromValueNotFoundErrorReturnsName) {
  EXPECT_EQ(GetUnknownVariableName(ValueNotFoundError("var_name")), "var_name");
  EXPECT_EQ(GetUnknownVariableName(ValueNotFoundError("with spaces")),
            "with spaces");
}

TEST(StatusTest, GetUnknownVariableNameFromVariableNotFoundErrorReturnsName) {
  EXPECT_EQ(GetUnknownVariableName(VariableNotFoundError("var_name")),
            "var_name");
  EXPECT_EQ(GetUnknownVariableName(VariableNotFoundError("with spaces")),
            "with spaces");
}

TEST(StatusTest, GetUnknownVariableNameReturnsNulloptForDifferentErrors) {
  EXPECT_EQ(GetUnknownVariableName(absl::OkStatus()), std::nullopt);
  EXPECT_EQ(GetUnknownVariableName(absl::FailedPreconditionError("hello?")),
            std::nullopt);
  EXPECT_EQ(GetUnknownVariableName(UnsatisfiedConstraintError("hello?")),
            std::nullopt);
}

// -----------------------------------------------------------------------------
//  MisconfiguredError

TEST(StatusTest, IsMisconfiguredErrorsGoogleTestMatcherWorks) {
  EXPECT_THAT(
      MisconfiguredError("a", "b", InternalConfigurationType::kUniverse),
      IsMisconfigured(InternalConfigurationType::kUniverse));
  EXPECT_THAT(absl::StatusOr<int>(MisconfiguredError(
                  "a", "b", InternalConfigurationType::kInputStream)),
              IsMisconfigured(InternalConfigurationType::kInputStream));

  EXPECT_THAT(UnsatisfiedConstraintError("some reason"),
              Not(IsMisconfigured(InternalConfigurationType::kUniverse)));
  EXPECT_THAT(absl::StatusOr<int>(MisconfiguredError(
                  "a", "b", InternalConfigurationType::kRandomEngine)),
              Not(IsMisconfigured(InternalConfigurationType::kInputStream)));
}

// -----------------------------------------------------------------------------
//  UnsatisfiedConstraintError

TEST(StatusTest, IsUnsatisfiedConstraintsGoogleTestMatcherWorks) {
  EXPECT_THAT(UnsatisfiedConstraintError("reason"),
              IsUnsatisfiedConstraint("reason"));
  EXPECT_THAT(absl::StatusOr<int>(UnsatisfiedConstraintError("reason")),
              IsUnsatisfiedConstraint("reason"));
  EXPECT_THAT(UnsatisfiedConstraintError("long long reason"),
              IsUnsatisfiedConstraint("reason"));

  EXPECT_THAT(UnsatisfiedConstraintError("some reason"),
              Not(IsUnsatisfiedConstraint("another reason")));
  EXPECT_THAT(absl::FailedPreconditionError("reason"),
              Not(IsUnsatisfiedConstraint("reason")));
}

TEST(StatusTest, FailedCheckConstraintsReturnsAnUnsatisfiedConstrainError) {
  EXPECT_TRUE(IsUnsatisfiedConstraintError(
      CheckConstraint(absl::FailedPreconditionError("message"), "reason")));
  EXPECT_TRUE(IsUnsatisfiedConstraintError(CheckConstraint(false, "reason")));
}

TEST(StatusTest, SuccessfulCheckConstraintsReturnsOk) {
  MORIARTY_EXPECT_OK(CheckConstraint(absl::OkStatus(), "reason"));
  MORIARTY_EXPECT_OK(CheckConstraint(true, "reason"));
}

// -----------------------------------------------------------------------------
//  ValueNotFoundError

TEST(StatusTest, IsValueNotFoundGoogleTestMatcherWorks) {
  EXPECT_THAT(ValueNotFoundError("name"), IsValueNotFound("name"));
  EXPECT_THAT(absl::StatusOr<int>(ValueNotFoundError("name")),
              IsValueNotFound("name"));

  EXPECT_THAT(ValueNotFoundError("name"), Not(IsValueNotFound("other")));
  EXPECT_THAT(absl::NotFoundError("name"), Not(IsValueNotFound("name")));
  EXPECT_THAT(VariableNotFoundError("name"), Not(IsValueNotFound("name")));
}

// -----------------------------------------------------------------------------
//  VariableNotFoundError

TEST(StatusTest, IsVariableNotFoundGoogleTestMatcherWorks) {
  EXPECT_THAT(VariableNotFoundError("name"), IsVariableNotFound("name"));
  EXPECT_THAT(absl::StatusOr<int>(VariableNotFoundError("name")),
              IsVariableNotFound("name"));

  EXPECT_THAT(VariableNotFoundError("name"), Not(IsVariableNotFound("other")));
  EXPECT_THAT(absl::NotFoundError("name"), Not(IsVariableNotFound("name")));
  EXPECT_THAT(ValueNotFoundError("name"), Not(IsVariableNotFound("name")));
}

}  // namespace
}  // namespace moriarty
