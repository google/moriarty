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

#include <ostream>
#include <regex>  // NOLINT
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"

namespace moriarty {
namespace moriarty_status {
namespace {

template <typename StringType>
class RegexMatchImpl : public ::testing::MatcherInterface<StringType> {
 public:
  RegexMatchImpl(const std::string& message_pattern)
      : message_pattern_(message_pattern) {}

  void DescribeTo(std::ostream* os) const override {
    *os << "message matches pattern ";
    ::testing::internal::UniversalPrint(message_pattern_, os);
  }

  void DescribeNegationTo(std::ostream* os) const override {
    *os << "message doesn't match pattern ";
    ::testing::internal::UniversalPrint(message_pattern_, os);
  }

  bool MatchAndExplain(
      StringType message,
      ::testing::MatchResultListener* result_listener) const override {
    return std::regex_match(message, std::regex(message_pattern_));
  }

 private:
  const std::string message_pattern_;
};

}  // namespace
}  // namespace moriarty_status

moriarty_status::StatusIsMatcher MatchesStatus(
    absl::StatusCode status_code, const std::string& message_pattern) {
  return moriarty_status::StatusIsMatcher(
      status_code, ::testing::Matcher<const std::string&>(
                       new moriarty_status::RegexMatchImpl<const std::string&>(
                           message_pattern)));
}

}  // namespace moriarty
