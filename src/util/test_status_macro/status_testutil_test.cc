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

#include "src/util/test_status_macro/status_testutil.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace {

// Returns the reason why x matches, or doesn't match, m.
template <typename MatcherType, typename Value>
std::string Explain(const MatcherType& m, const Value& x) {
  testing::StringMatchResultListener listener;
  ExplainMatchResult(m, x, &listener);
  return listener.str();
}

TEST(StatusTestutilTest, IsOk) {
  EXPECT_THAT(absl::OkStatus(), ::moriarty::IsOk());

  EXPECT_THAT(absl::StatusOr<int>(1), ::moriarty::IsOk());
  EXPECT_THAT(absl::StatusOr<int>(2), ::moriarty::IsOk());
  EXPECT_THAT(absl::InternalError(""), ::testing::Not(::moriarty::IsOk()));

  // IsOk() doesn't explain; EXPECT logs the value.
  EXPECT_THAT(Explain(::moriarty::IsOk(), absl::InternalError("")),
              testing::IsEmpty());
  EXPECT_THAT(Explain(::moriarty::IsOk(), absl::OkStatus()),
              testing::IsEmpty());

  // Our macros
  MORIARTY_EXPECT_OK(absl::OkStatus());
  MORIARTY_ASSERT_OK(absl::OkStatus());
}

TEST(StatusTestutilTest, IsOkAndHolds) {
  EXPECT_THAT(absl::StatusOr<int>(1), ::moriarty::IsOkAndHolds(1));
  EXPECT_THAT(absl::StatusOr<int>(2), ::moriarty::IsOkAndHolds(2));
  EXPECT_THAT(absl::StatusOr<int>(1), ::moriarty::IsOkAndHolds(::testing::_));

  // Negations
  EXPECT_THAT(absl::StatusOr<int>(2),
              ::moriarty::IsOkAndHolds(::testing::Not(1)));

  int result;
  MORIARTY_ASSERT_OK_AND_ASSIGN(result, absl::StatusOr<int>(2));
  EXPECT_EQ(2, result);

  EXPECT_THAT(Explain(::moriarty::IsOkAndHolds(1), absl::StatusOr<int>(2)),
              testing::HasSubstr("2 doesn't match"));
}

TEST(StatusTestutilTest, StatusIs) {
  EXPECT_THAT(absl::InternalError(""),
              moriarty::StatusIs(absl::StatusCode::kInternal));
  EXPECT_THAT(absl::InternalError(""),
              moriarty::StatusIs(absl::StatusCode::kInternal));
  EXPECT_THAT(absl::InternalError(""),
              moriarty::StatusIs(absl::StatusCode::kInternal));

  EXPECT_THAT(absl::OkStatus(),
              ::testing::Not(moriarty::StatusIs(absl::StatusCode::kInternal)));
  EXPECT_THAT(absl::OkStatus(), moriarty::StatusIs(absl::StatusCode::kOk));

  EXPECT_THAT(Explain(moriarty::StatusIs(absl::StatusCode::kOk),
                      absl::InternalError("")),
              testing::HasSubstr("whose status code INTERNAL doesn't match"));
}

TEST(StatusTestutilTest, StatusIs_WithMessage) {
  EXPECT_THAT(absl::InternalError("strongbad"),
              moriarty::StatusIs(::testing::_, ::testing::HasSubstr("bad")));
  EXPECT_THAT(absl::InternalError("strongbad"),
              moriarty::StatusIs(::testing::_, ::testing::HasSubstr("bad")));

  EXPECT_THAT(absl::InternalError("strongbad"),
              moriarty::StatusIs(::testing::_, ::testing::HasSubstr("bad")));
  EXPECT_THAT(absl::InternalError("strongbad"),
              moriarty::StatusIs(::testing::_,
                                 ::testing::Not(::testing::HasSubstr("good"))));

  EXPECT_THAT(absl::Status{absl::InternalError("strongbad")},
              moriarty::StatusIs(::testing::Not(absl::StatusCode::kAborted),
                                 ::testing::Not(::testing::HasSubstr("good"))));
}

TEST(StatusTestutilTest, MatchesStatus) {
  EXPECT_THAT(absl::InternalError(""),
              ::moriarty::MatchesStatus(absl::StatusCode::kInternal));
  EXPECT_THAT(absl::InternalError(""),
              ::moriarty::MatchesStatus(absl::StatusCode::kInternal));

  EXPECT_THAT(absl::InternalError(""),
              ::moriarty::MatchesStatus(absl::StatusCode::kInternal));
  EXPECT_THAT(absl::OkStatus(),
              ::moriarty::MatchesStatus(absl::StatusCode::kOk));
}

TEST(StatusTestutilTest, MatchesStatus_Pattern) {
  EXPECT_THAT(absl::InternalError("a"),
              ::moriarty::MatchesStatus(absl::StatusCode::kInternal, "a"));
  EXPECT_THAT(absl::InternalError("a"),
              ::moriarty::MatchesStatus(absl::StatusCode::kInternal, "a"));

  EXPECT_THAT(absl::InternalError("a"),
              ::moriarty::MatchesStatus(absl::StatusCode::kInternal, "a"));
  EXPECT_THAT(absl::InternalError("a"),
              ::testing::Not(
                  ::moriarty::MatchesStatus(absl::StatusCode::kInternal, "b")));
  EXPECT_THAT(absl::InternalError("a"),
              ::testing::Not(::moriarty::MatchesStatus(
                  absl::StatusCode::kCancelled, "a")));
}

}  // namespace
