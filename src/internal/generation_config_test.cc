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

#include "src/internal/generation_config.h"

#include <optional>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "src/util/test_status_macro/status_testutil.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

using ::testing::HasSubstr;
using ::testing::Optional;
using ::testing::PrintToString;
using ::moriarty::StatusIs;

MATCHER(IsRetry, "recommends to retry generation") {
  if (!arg.ok()) {
    *result_listener << "Non-ok result: " << arg.status();
    return false;
  }

  if (arg.value().policy != GenerationConfig::RetryRecommendation::kRetry) {
    *result_listener << "Wrong policy: " << PrintToString(arg.value().policy);
    return false;
  }

  return true;
}

MATCHER(IsAbort, "recommends to abort generation") {
  if (!arg.ok()) {
    *result_listener << "Non-ok result: " << arg.status();
    return false;
  }

  if (arg.value().policy != GenerationConfig::RetryRecommendation::kAbort) {
    *result_listener << "Wrong policy: " << PrintToString(arg.value().policy);
    return false;
  }

  return true;
}

MATCHER_P(IsRetryWithDeletedVars, vars_to_delete, "") {
  if (!arg.ok()) {
    *result_listener << "Non-ok result: " << arg.status();
    return false;
  }

  if (arg.value().policy != GenerationConfig::RetryRecommendation::kRetry) {
    *result_listener << "Wrong policy: " << PrintToString(arg.value().policy);
    return false;
  }

  return testing::ExplainMatchResult(
      testing::UnorderedElementsAreArray(vars_to_delete),
      arg.value().variable_names_to_delete, result_listener);
}

MATCHER_P(IsAbortWithDeletedVars, vars_to_delete, "") {
  if (!arg.ok()) {
    *result_listener << "Non-ok result: " << arg.status();
    return false;
  }

  if (arg.value().policy != GenerationConfig::RetryRecommendation::kAbort) {
    *result_listener << "Wrong policy: " << PrintToString(arg.value().policy);
    return false;
  }

  return testing::ExplainMatchResult(
      testing::UnorderedElementsAreArray(vars_to_delete),
      arg.value().variable_names_to_delete, result_listener);
}

TEST(GenerationConfigTest,
     AddGenerationFailureRetriesRightNumberOfTimesForActiveRetries) {
  absl::Status fail = absl::FailedPreconditionError("test");
  GenerationConfig g;

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  for (int i = 0; i < GenerationConfig::kMaxActiveRetries; i++) {
    EXPECT_THAT(g.AddGenerationFailure("x", fail), IsRetry());
  }

  EXPECT_THAT(g.AddGenerationFailure("x", fail), IsAbort());
}

TEST(GenerationConfigTest, ActiveRetriesResetsWithMarkSuccessfulGeneration) {
  absl::Status fail = absl::FailedPreconditionError("test");
  GenerationConfig g;

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  for (int i = 0; i < GenerationConfig::kMaxActiveRetries; i++) {
    ASSERT_THAT(g.AddGenerationFailure("x", fail), IsRetry());
  }
  MORIARTY_ASSERT_OK(g.MarkSuccessfulGeneration("x"));

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  for (int i = 0; i < GenerationConfig::kMaxActiveRetries; i++) {
    ASSERT_THAT(g.AddGenerationFailure("x", fail), IsRetry());
  }
  EXPECT_THAT(g.AddGenerationFailure("x", fail), IsAbort());

  // Should continue recommending to abort.
  EXPECT_THAT(g.AddGenerationFailure("x", fail), IsAbort());
  EXPECT_THAT(g.AddGenerationFailure("x", fail), IsAbort());
}

TEST(GenerationConfigTest, ActiveRetriesResetsWithMarkAbandonedGeneration) {
  absl::Status fail = absl::FailedPreconditionError("test");
  GenerationConfig g;

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  for (int i = 0; i < GenerationConfig::kMaxActiveRetries; i++) {
    ASSERT_THAT(g.AddGenerationFailure("x", fail), IsRetry());
  }
  MORIARTY_ASSERT_OK(g.MarkAbandonedGeneration("x"));

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  for (int i = 0; i < GenerationConfig::kMaxActiveRetries; i++) {
    ASSERT_THAT(g.AddGenerationFailure("x", fail), IsRetry());
  }

  EXPECT_THAT(g.AddGenerationFailure("x", fail), IsAbort());

  // Should continue recommending to abort.
  EXPECT_THAT(g.AddGenerationFailure("x", fail), IsAbort());
  EXPECT_THAT(g.AddGenerationFailure("x", fail), IsAbort());
}

TEST(GenerationConfigTest, MarkStartGenerationCatchesCycles) {
  GenerationConfig g;

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("y"));
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("z"));

  EXPECT_THAT(
      g.MarkStartGeneration("x"),
      StatusIs(absl::StatusCode::kFailedPrecondition, HasSubstr("cycl")));
}

TEST(GenerationConfigTest,
     GenerationAttemptsMustOnlyBeForMostRecentStartedVariable) {
  GenerationConfig g;

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("y"));

  EXPECT_THAT(g.MarkAbandonedGeneration("x"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(g.MarkSuccessfulGeneration("x"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(g.AddGenerationFailure("x", absl::FailedPreconditionError("")),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(GenerationConfigTest, AbandonedAndSuccessfulGenerationPopFromStack) {
  GenerationConfig g;

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("y"));
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("z"));
  MORIARTY_EXPECT_OK(g.MarkAbandonedGeneration("z"));
  MORIARTY_EXPECT_OK(g.MarkSuccessfulGeneration("y"));
  MORIARTY_EXPECT_OK(g.MarkSuccessfulGeneration("x"));
}

TEST(GenerationConfigTest, AddGenerationFailureDoesNotAffectTheStack) {
  GenerationConfig g;

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("y"));
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("z"));
  MORIARTY_EXPECT_OK(g.MarkAbandonedGeneration("z"));
  MORIARTY_EXPECT_OK(
      g.AddGenerationFailure("y", absl::FailedPreconditionError("")));
  MORIARTY_EXPECT_OK(g.MarkSuccessfulGeneration("y"));
  MORIARTY_EXPECT_OK(g.MarkSuccessfulGeneration("x"));
}

TEST(GenerationConfigTest, VariablesCanBeReAddedToTheStackAfterRemoval) {
  GenerationConfig g;

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("y"));
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("z"));

  MORIARTY_ASSERT_OK(g.MarkAbandonedGeneration("z"));
  MORIARTY_EXPECT_OK(g.MarkStartGeneration("z"));

  MORIARTY_ASSERT_OK(g.MarkAbandonedGeneration("z"));
  MORIARTY_ASSERT_OK(g.MarkAbandonedGeneration("y"));
  MORIARTY_EXPECT_OK(g.MarkStartGeneration("z"));
}

TEST(GenerationConfigTest, TotalRetriesStopsGeneration) {
  absl::Status fail = absl::FailedPreconditionError("test");
  GenerationConfig g;

  for (int i = 0; i < GenerationConfig::kMaxTotalRetries; i++) {
    MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
    MORIARTY_ASSERT_OK(g.MarkStartGeneration("y"));

    MORIARTY_ASSERT_OK(g.AddGenerationFailure("y", fail));
    MORIARTY_ASSERT_OK(g.MarkAbandonedGeneration("y"));

    MORIARTY_ASSERT_OK(g.AddGenerationFailure("x", fail));
    MORIARTY_ASSERT_OK(g.MarkSuccessfulGeneration("x"));
  }

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  EXPECT_THAT(g.AddGenerationFailure("x", fail), IsAbort());

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("y"));
  EXPECT_THAT(g.AddGenerationFailure("y", fail), IsAbort());
}

TEST(GenerationConfigTest,
     GetGenerationStatusReturnsInvalidArgumentOnUnknownVariable) {
  GenerationConfig g;
  EXPECT_THAT(g.GetGenerationStatus("x"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(GenerationConfigTest,
     GetGenerationStatusReturnsFailedPreconditionOnUnfinishedVariable) {
  GenerationConfig g;
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  EXPECT_THAT(g.GetGenerationStatus("x"),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(GenerationConfigTest, GetGenerationStatusCanReturnAFailedStatus) {
  absl::Status fail = absl::FailedPreconditionError("test");
  GenerationConfig g;

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  MORIARTY_ASSERT_OK(g.AddGenerationFailure("x", fail));
  EXPECT_EQ(g.GetGenerationStatus("x"), fail);
}

TEST(GenerationConfigTest, GetGenerationStatusCanReturnASuccessStatus) {
  GenerationConfig g;
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  MORIARTY_ASSERT_OK(g.MarkSuccessfulGeneration("x"));
  MORIARTY_EXPECT_OK(g.GetGenerationStatus("x"));
}

TEST(GenerationConfigTest,
     GetGenerationStatusReturnsSuccessAfterFailureThenSuccess) {
  absl::Status fail = absl::FailedPreconditionError("test");
  GenerationConfig g;

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  MORIARTY_ASSERT_OK(g.AddGenerationFailure("x", fail));
  MORIARTY_ASSERT_OK(g.MarkSuccessfulGeneration("x"));
  MORIARTY_EXPECT_OK(g.GetGenerationStatus("x"));
}

TEST(GenerationConfigTest,
     GetGenerationStatusReturnsFailureAfterSuccessThenFailure) {
  absl::Status fail = absl::FailedPreconditionError("test");
  GenerationConfig g;

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  MORIARTY_ASSERT_OK(g.MarkSuccessfulGeneration("x"));
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  MORIARTY_ASSERT_OK(g.AddGenerationFailure("x", fail));

  EXPECT_EQ(g.GetGenerationStatus("x"), fail);
}

TEST(GenerationConfigTest,
     GetGenerationStatusReturnsSameStatusAfterMarkAbandonGeneration) {
  absl::Status fail = absl::FailedPreconditionError("test");
  GenerationConfig g;

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  MORIARTY_ASSERT_OK(g.AddGenerationFailure("x", fail));
  MORIARTY_ASSERT_OK(g.MarkAbandonedGeneration("x"));

  EXPECT_EQ(g.GetGenerationStatus("x"), fail);
}

TEST(GenerationConfigTest,
     ReachingTotalGenerateCallsStopsGenerationOnNextFailure) {
  absl::Status fail = absl::FailedPreconditionError("test");
  GenerationConfig g;

  for (int i = 0; i < GenerationConfig::kMaxTotalGenerateCalls; i++) {
    MORIARTY_ASSERT_OK(g.MarkStartGeneration("y"));
    MORIARTY_ASSERT_OK(g.MarkSuccessfulGeneration("y"));
  }

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));
  EXPECT_THAT(g.AddGenerationFailure("x", fail), IsAbort());
}

TEST(GenerationConfigTest, MarkSuccessfulGenerationWithoutCallingStartFails) {
  EXPECT_THAT(GenerationConfig().MarkSuccessfulGeneration("x"),
              StatusIs(absl::StatusCode::kInvalidArgument));

  GenerationConfig g;
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("y"));
  EXPECT_THAT(g.MarkSuccessfulGeneration("x"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(GenerationConfigTest, MarkAbandonedGenerationWithoutCallingStartFails) {
  EXPECT_THAT(GenerationConfig().MarkAbandonedGeneration("x"),
              StatusIs(absl::StatusCode::kInvalidArgument));

  GenerationConfig g;
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("y"));
  EXPECT_THAT(g.MarkAbandonedGeneration("x"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(GenerationConfigTest, AddGenerationFailureWithOkStatusFails) {
  GenerationConfig g;
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));

  EXPECT_THAT(
      g.AddGenerationFailure("x", absl::OkStatus()),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("OkStatus")));
}

TEST(GenerationConfigTest, GenerationAttemptsWithoutCallingStartFails) {
  GenerationConfig g;

  EXPECT_THAT(g.AddGenerationFailure("x", absl::FailedPreconditionError("")),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(g.MarkSuccessfulGeneration("x"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(g.MarkSuccessfulGeneration("x"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(GenerationConfigTest, SoftGenerationLimitIsNulloptByDefault) {
  EXPECT_EQ(GenerationConfig().GetSoftGenerationLimit(), std::nullopt);
}

TEST(GenerationConfigTest, SoftGenerationLimitIsUpdatedAppropriately) {
  GenerationConfig g;

  g.SetSoftGenerationLimit(123456);

  EXPECT_THAT(g.GetSoftGenerationLimit(), Optional(123456));
}

TEST(GenerationConfigTest,
     VariablesToDeleteAreOnesGeneratedBetweenBeginAndEnd) {
  absl::Status fail = absl::FailedPreconditionError("test");
  GenerationConfig g;

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("y"));
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("z"));
  MORIARTY_ASSERT_OK(g.MarkSuccessfulGeneration("z"));
  MORIARTY_ASSERT_OK(g.MarkSuccessfulGeneration("y"));

  EXPECT_THAT(g.AddGenerationFailure("x", fail),
              IsRetryWithDeletedVars(std::vector<std::string>({"y", "z"})));
}

TEST(GenerationConfigTest, VariablesToDeleteShouldNotDeleteUnrelatedVariables) {
  absl::Status fail = absl::FailedPreconditionError("test");
  GenerationConfig g;

  // Generate a variable first, so we shouldn't delete it.
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("w"));
  MORIARTY_ASSERT_OK(g.MarkSuccessfulGeneration("w"));

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("x"));

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("y"));
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("z"));
  MORIARTY_ASSERT_OK(g.MarkSuccessfulGeneration("z"));
  MORIARTY_ASSERT_OK(g.MarkSuccessfulGeneration("y"));

  EXPECT_THAT(g.AddGenerationFailure("x", fail),
              IsRetryWithDeletedVars(std::vector<std::string>({"y", "z"})));

  MORIARTY_ASSERT_OK(g.MarkStartGeneration("p"));
  MORIARTY_ASSERT_OK(g.MarkStartGeneration("q"));
  MORIARTY_ASSERT_OK(g.MarkSuccessfulGeneration("q"));
  MORIARTY_ASSERT_OK(g.MarkSuccessfulGeneration("p"));

  EXPECT_THAT(g.AddGenerationFailure("x", fail),
              IsRetryWithDeletedVars(std::vector<std::string>({"p", "q"})));
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
