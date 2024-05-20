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

#include "src/internal/status_utils.h"

#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

absl::StatusOr<int> StatusOrGood() { return 5; }
absl::StatusOr<int> StatusOrBad() {
  return absl::FailedPreconditionError("Intentional failure.");
}

absl::Status StatusGood() { return absl::OkStatus(); }
absl::Status StatusBad() {
  return absl::FailedPreconditionError("Intentional failure.");
}

TEST(StatusUtilsTest, TryFunctionOrCrashStatusOrVersionDoesNotCrashOnOkStatus) {
  EXPECT_EQ(TryFunctionOrCrash<int>(StatusOrGood, "StatusOrGood"), 5);
}

TEST(StatusUtilsTest, TryFunctionOrCrashStatusVersionDoesNotCrashOnOkStatus) {
  // The test is that this next line should not crash.
  TryFunctionOrCrash(StatusGood, "StatusGood");
}

TEST(StatusUtilsDeathTest,
     TryFunctionOrCrashStatusOrVersionCrashesOnNonOkStatus) {
  EXPECT_DEATH({ TryFunctionOrCrash<int>(StatusOrBad, "Foo"); }, "TryFoo");
}

TEST(StatusUtilsDeathTest,
     TryFunctionOrCrashStatusVersionCrashesOnNonOkStatus) {
  EXPECT_DEATH({ TryFunctionOrCrash(StatusBad, "Foo"); }, "TryFoo");
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
