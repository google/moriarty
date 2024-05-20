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

#ifndef MORIARTY_SRC_UTIL_TEST_STATUS_MACRO_STATUS_TESTUTIL_H_
#define MORIARTY_SRC_UTIL_TEST_STATUS_MACRO_STATUS_TESTUTIL_H_

/// \file
/// Implements GMock matchers for absl::Status and absl::StatusOr.
/// For example, to test an Ok result, perhaps with a value, use:
///
///   EXPECT_THAT(DoSomething(), ::moriarty::IsOk());
///   EXPECT_THAT(DoSomething(), ::moriarty::IsOkAndHolds(7));
///
/// A shorthand for IsOk() is provided:
///
///   MORIARTY_EXPECT_OK(DoSomething());
///
/// To test an error expectation, use:
///
///   EXPECT_THAT(DoSomething(),
///               ::moriarty::StatusIs(absl::StatusCode::kInternal));
///   EXPECT_THAT(DoSomething(),
///               ::moriarty::StatusIs(absl::StatusCode::kInternal,
///                                       ::testing::HasSubstr("foo")));
///
///   EXPECT_THAT(DoSomething(),
///               ::moriarty::MatchesStatus(absl::StatusCode::kInternal,
///               "foo"));
///

#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "src/testing/status_test_util.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {
namespace moriarty_status {

// Monomorphic implementation of matcher IsOkAndHolds(m).
// StatusType is a const reference to Status or StatusOr<T>.
template <typename StatusType>
class IsOkAndHoldsMatcherImpl : public ::testing::MatcherInterface<StatusType> {
 public:
  typedef
      typename std::remove_reference<StatusType>::type::value_type value_type;

  template <typename InnerMatcher>
  explicit IsOkAndHoldsMatcherImpl(InnerMatcher&& inner_matcher)
      : inner_matcher_(::testing::SafeMatcherCast<const value_type&>(
            std::forward<InnerMatcher>(inner_matcher))) {}

  void DescribeTo(std::ostream* os) const override {
    *os << "is OK and has a value that ";
    inner_matcher_.DescribeTo(os);
  }
  void DescribeNegationTo(std::ostream* os) const override {
    *os << "isn't OK or has a value that ";
    inner_matcher_.DescribeNegationTo(os);
  }
  bool MatchAndExplain(
      StatusType actual_value,
      ::testing::MatchResultListener* result_listener) const override {
    auto status =
        ::moriarty_testing::moriarty_testing_internal::GetStatus(actual_value);
    if (!status.ok()) {
      *result_listener << "whose status code is "
                       << absl::StatusCodeToString(status.code());
      return false;
    }

    ::testing::StringMatchResultListener inner_listener;
    if (!inner_matcher_.MatchAndExplain(actual_value.value(),
                                        &inner_listener)) {
      *result_listener << "whose value "
                       << ::testing::PrintToString(actual_value)
                       << " doesn't match";
      if (!inner_listener.str().empty()) {
        *result_listener << ", " << inner_listener.str();
      }
      return false;
    }
    return true;
  }

 private:
  const ::testing::Matcher<const value_type&> inner_matcher_;
};

// Implements IsOkAndHolds(m) as a polymorphic matcher.
template <typename InnerMatcher>
class IsOkAndHoldsMatcher {
 public:
  explicit IsOkAndHoldsMatcher(InnerMatcher inner_matcher)
      : inner_matcher_(std::move(inner_matcher)) {}

  // Converts this polymorphic matcher to a monomorphic matcher of the
  // given type.
  template <typename StatusType>
  operator ::testing::Matcher<StatusType>() const {  // NOLINT
    return ::testing::Matcher<StatusType>(
        new IsOkAndHoldsMatcherImpl<const StatusType&>(inner_matcher_));
  }

 private:
  const InnerMatcher inner_matcher_;
};

// Monomorphic implementation of matcher IsOk() for a given type T.
template <typename StatusType>
class MonoIsOkMatcherImpl : public ::testing::MatcherInterface<StatusType> {
 public:
  void DescribeTo(std::ostream* os) const override { *os << "is OK"; }
  void DescribeNegationTo(std::ostream* os) const override {
    *os << "is not OK";
  }
  bool MatchAndExplain(
      StatusType actual_value,
      ::testing::MatchResultListener* result_listener) const override {
    return actual_value.ok();
  }
};

// Implements IsOk() as a polymorphic matcher.
class IsOkMatcher {
 public:
  template <typename StatusType>
  operator ::testing::Matcher<StatusType>() const {  // NOLINT
    return ::testing::Matcher<StatusType>(
        new MonoIsOkMatcherImpl<const StatusType&>());
  }
};

// Monomorphic implementation of matcher StatusIs().
// StatusType is a reference to Status or StatusOr<T>.
template <typename StatusType>
class StatusIsMatcherImpl : public ::testing::MatcherInterface<StatusType> {
 public:
  explicit StatusIsMatcherImpl(
      testing::Matcher<absl::StatusCode> code_matcher,
      testing::Matcher<const std::string&> message_matcher)
      : code_matcher_(std::move(code_matcher)),
        message_matcher_(std::move(message_matcher)) {}

  void DescribeTo(std::ostream* os) const override {
    *os << "has a status code that ";
    code_matcher_.DescribeTo(os);
    *os << ", and has an error message that ";
    message_matcher_.DescribeTo(os);
  }

  void DescribeNegationTo(std::ostream* os) const override {
    *os << "has a status code that ";
    code_matcher_.DescribeNegationTo(os);
    *os << ", or has an error message that ";
    message_matcher_.DescribeNegationTo(os);
  }

  bool MatchAndExplain(
      StatusType status,
      ::testing::MatchResultListener* result_listener) const override {
    testing::StringMatchResultListener inner_listener;
    auto status_value =
        ::moriarty_testing::moriarty_testing_internal::GetStatus(status);
    if (!code_matcher_.MatchAndExplain(status_value.code(), &inner_listener)) {
      *result_listener << "whose status code "
                       << absl::StatusCodeToString(status_value.code())
                       << " doesn't match";
      const std::string inner_explanation = inner_listener.str();
      if (!inner_explanation.empty()) {
        *result_listener << ", " << inner_explanation;
      }
      return false;
    }

    if (!message_matcher_.Matches(std::string(status_value.message()))) {
      *result_listener << "whose error message is wrong";
      return false;
    }

    return true;
  }

 private:
  const testing::Matcher<absl::StatusCode> code_matcher_;
  const testing::Matcher<const std::string&> message_matcher_;
};

// Implements StatusIs() as a polymorphic matcher.
class StatusIsMatcher {
 public:
  StatusIsMatcher(testing::Matcher<absl::StatusCode> code_matcher,
                  testing::Matcher<const std::string&> message_matcher)
      : code_matcher_(std::move(code_matcher)),
        message_matcher_(std::move(message_matcher)) {}

  // Converts this polymorphic matcher to a monomorphic matcher of the
  // given type.
  template <typename StatusType>
  operator ::testing::Matcher<StatusType>() const {  // NOLINT
    return ::testing::Matcher<StatusType>(
        new StatusIsMatcherImpl<const StatusType&>(code_matcher_,
                                                   message_matcher_));
  }

 private:
  const testing::Matcher<absl::StatusCode> code_matcher_;
  const testing::Matcher<const std::string&> message_matcher_;
};

}  // namespace moriarty_status

// Returns a gMock matcher that matches an OK Status/StatusOr.
inline moriarty_status::IsOkMatcher IsOk() {
  return moriarty_status::IsOkMatcher();
}

// Returns a gMock matcher that matches an OK Status/StatusOr.
// and whose value matches the inner matcher.
template <typename InnerMatcher>
moriarty_status::IsOkAndHoldsMatcher<typename std::decay<InnerMatcher>::type>
IsOkAndHolds(InnerMatcher&& inner_matcher) {
  return moriarty_status::IsOkAndHoldsMatcher<
      typename std::decay<InnerMatcher>::type>(
      std::forward<InnerMatcher>(inner_matcher));
}

// Returns a matcher that matches a Status/StatusOr whose code
// matches code_matcher, and whose error message matches message_matcher.
template <typename CodeMatcher, typename MessageMatcher>
moriarty_status::StatusIsMatcher StatusIs(CodeMatcher code_matcher,
                                          MessageMatcher message_matcher) {
  return moriarty_status::StatusIsMatcher(std::move(code_matcher),
                                          std::move(message_matcher));
}

// Returns a matcher that matches a Status/StatusOr whose code
// matches code_matcher.
template <typename CodeMatcher>
moriarty_status::StatusIsMatcher StatusIs(CodeMatcher code_matcher) {
  return moriarty_status::StatusIsMatcher(std::move(code_matcher),
                                          ::testing::_);
}

// Returns a matcher that matches a Status/StatusOr whose code
// is code_matcher.
inline moriarty_status::StatusIsMatcher MatchesStatus(
    absl::StatusCode status_code) {
  return moriarty_status::StatusIsMatcher(status_code, ::testing::_);
}

// Returns a matcher that matches a Status/StatusOr whose code
// is code_matcher, and whose message matches the provided regex pattern.
moriarty_status::StatusIsMatcher MatchesStatus(
    absl::StatusCode status_code, const std::string& message_pattern);

}  // namespace moriarty

/// EXPECT assertion that the argument, when converted to an `absl::Status` via
/// `moriarty::GetStatus`, has a code of `absl::StatusCode::kOk`.
///
/// Example::
///
///     absl::Status DoSomething();
///     absl::StatusOr<int> GetResult();
///
///     MORIARTY_EXPECT_OK(DoSomething());
///     MORIARTY_EXPECT_OK(GetResult());
///
/// \relates absl::Status
/// \relates absl::StatusOr
/// \membergroup Test support
#define MORIARTY_EXPECT_OK(expr) EXPECT_THAT(expr, ::moriarty::IsOk())

/// Same as `MORIARTY_EXPECT_OK`, but returns in the case of an error.
///
/// \relates absl::Status
/// \relates absl::StatusOr
/// \membergroup Test support
#define MORIARTY_ASSERT_OK(expr) ASSERT_THAT(expr, ::moriarty::IsOk())

/// ASSERTs that `expr` is a `moriarty::Result` with a value, and assigns the
/// value to `decl`.
///
/// Example:
///
///     absl::StatusOr<int> GetResult();
///
///     MORIARTY_ASSERT_OK_AND_ASSIGN(int x, GetResult());
///
/// \relates absl::StatusOr
/// \membergroup Test support
#define MORIARTY_ASSERT_OK_AND_ASSIGN(decl, expr)                      \
  MORIARTY_ASSIGN_OR_RETURN(decl, expr,                                \
                            ([&] { FAIL() << #expr << ": " << _; })()) \
  /**/

#endif  // MORIARTY_UTIL_STATUS_TESTUTIL_H_
