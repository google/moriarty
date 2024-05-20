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

#include "src/librarian/io_config.h"

#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "src/errors.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"

namespace moriarty {
namespace librarian {
namespace {

using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;
using ::moriarty_testing::IsMisconfigured;
using ::testing::StrEq;

TEST(IOConfigTest, DefaultWhitespacePolicyShouldBeExact) {
  IOConfig c;
  EXPECT_EQ(c.GetWhitespacePolicy(), IOConfig::WhitespacePolicy::kExact);
}

TEST(IOConfigTest, GetAndSetWhitespacePolicyShouldWork) {
  IOConfig c;
  c.SetWhitespacePolicy(IOConfig::WhitespacePolicy::kIgnoreWhitespace);
  EXPECT_EQ(c.GetWhitespacePolicy(),
            IOConfig::WhitespacePolicy::kIgnoreWhitespace);
}

TEST(IOConfigTest, AnyReadFunctionWithoutSetInputStreamShouldFail) {
  IOConfig c;
  EXPECT_THAT(c.ReadToken(), moriarty_testing::IsMisconfigured(
                                 InternalConfigurationType::kInputStream));
  EXPECT_THAT(c.ReadWhitespace(Whitespace::kNewline),
              IsMisconfigured(InternalConfigurationType::kInputStream));
}

TEST(IOConfigTest, AnyPrintFunctionWithoutSetOutputStreamShouldFail) {
  IOConfig c;
  EXPECT_THAT(c.PrintWhitespace(Whitespace::kSpace),
              IsMisconfigured(InternalConfigurationType::kOutputStream));
  EXPECT_THAT(c.PrintToken("hello!"),
              IsMisconfigured(InternalConfigurationType::kOutputStream));
}

TEST(IOConfigTest, ReadWhitespaceShouldRespectWhitespacePolicy) {
  std::stringstream ss;
  IOConfig c;
  c.SetInputStream(ss);

  {  // In kIgnoreWhitespace mode, ReadWhitespace is a no-op.
    ss.str("Hello!");
    c.SetWhitespacePolicy(IOConfig::WhitespacePolicy::kIgnoreWhitespace);
    MORIARTY_EXPECT_OK(c.ReadWhitespace(Whitespace::kSpace));
  }
  {  // In kExact mode, this should read the whitespace character.
    ss.str(" Hello!");
    c.SetWhitespacePolicy(IOConfig::WhitespacePolicy::kExact);
    MORIARTY_EXPECT_OK(c.ReadWhitespace(Whitespace::kSpace));
    EXPECT_THAT(c.ReadWhitespace(Whitespace::kSpace),
                StatusIs(absl::StatusCode::kInvalidArgument));
  }
}

TEST(IOConfigTest, ReadWhitespaceShouldReadTheCorrectWhitespace) {
  std::stringstream ss;
  IOConfig c;
  c.SetInputStream(ss);
  c.SetWhitespacePolicy(IOConfig::WhitespacePolicy::kExact);

  {
    ss.str(" \n\t");
    MORIARTY_EXPECT_OK(c.ReadWhitespace(Whitespace::kSpace));
    MORIARTY_EXPECT_OK(c.ReadWhitespace(Whitespace::kNewline));
    MORIARTY_EXPECT_OK(c.ReadWhitespace(Whitespace::kTab));
  }

  {
    ss.str(" \n\t");
    EXPECT_THAT(c.ReadWhitespace(Whitespace::kNewline),
                StatusIs(absl::StatusCode::kInvalidArgument));
    EXPECT_THAT(c.ReadWhitespace(Whitespace::kTab),
                StatusIs(absl::StatusCode::kInvalidArgument));
    MORIARTY_EXPECT_OK(c.ReadWhitespace(Whitespace::kSpace));

    EXPECT_THAT(c.ReadWhitespace(Whitespace::kSpace),
                StatusIs(absl::StatusCode::kInvalidArgument));
    EXPECT_THAT(c.ReadWhitespace(Whitespace::kTab),
                StatusIs(absl::StatusCode::kInvalidArgument));
    MORIARTY_EXPECT_OK(c.ReadWhitespace(Whitespace::kNewline));

    EXPECT_THAT(c.ReadWhitespace(Whitespace::kNewline),
                StatusIs(absl::StatusCode::kInvalidArgument));
    EXPECT_THAT(c.ReadWhitespace(Whitespace::kSpace),
                StatusIs(absl::StatusCode::kInvalidArgument));
    MORIARTY_EXPECT_OK(c.ReadWhitespace(Whitespace::kTab));
  }
}

TEST(IOConfigTest, PrintWhitespaceShouldPrintTheCorrectWhitespace) {
  std::stringstream ss;
  IOConfig c;
  c.SetOutputStream(ss);
  MORIARTY_EXPECT_OK(c.PrintWhitespace(Whitespace::kSpace));
  MORIARTY_EXPECT_OK(c.PrintWhitespace(Whitespace::kNewline));
  MORIARTY_EXPECT_OK(c.PrintWhitespace(Whitespace::kTab));
  EXPECT_THAT(ss.str(), StrEq(" \n\t"));
}

TEST(IOConfigTest, ReadTokenShouldReadExactlyOneToken) {
  std::stringstream ss;
  IOConfig c;
  c.SetInputStream(ss).SetWhitespacePolicy(
      IOConfig::WhitespacePolicy::kIgnoreWhitespace);

  {
    ss.str("Hello!");
    EXPECT_THAT(c.ReadToken(), IsOkAndHolds("Hello!"));
  }
  {
    ss = std::stringstream("One Two Three");
    EXPECT_THAT(c.ReadToken(), IsOkAndHolds("One"));
    EXPECT_THAT(c.ReadToken(), IsOkAndHolds("Two"));
    EXPECT_THAT(c.ReadToken(), IsOkAndHolds("Three"));
  }
}

TEST(IOConfigTest, ReadTokenShouldRespectWhitespacePolicy) {
  std::stringstream ss;
  IOConfig c;
  c.SetInputStream(ss);

  {
    c.SetWhitespacePolicy(IOConfig::WhitespacePolicy::kIgnoreWhitespace);
    ss.str("One Two Three");
    EXPECT_THAT(c.ReadToken(), IsOkAndHolds("One"));
    EXPECT_THAT(c.ReadToken(), IsOkAndHolds("Two"));
    EXPECT_THAT(c.ReadToken(), IsOkAndHolds("Three"));
  }

  {
    c.SetWhitespacePolicy(IOConfig::WhitespacePolicy::kExact);
    ss = std::stringstream("Four Five Six");

    EXPECT_THAT(c.ReadToken(), IsOkAndHolds("Four"));
    EXPECT_THAT(c.ReadToken(), StatusIs(absl::StatusCode::kFailedPrecondition));
    MORIARTY_EXPECT_OK(c.ReadWhitespace(Whitespace::kSpace));

    EXPECT_THAT(c.ReadToken(), IsOkAndHolds("Five"));
    EXPECT_THAT(c.ReadToken(), StatusIs(absl::StatusCode::kFailedPrecondition));
    MORIARTY_EXPECT_OK(c.ReadWhitespace(Whitespace::kSpace));

    EXPECT_THAT(c.ReadToken(), IsOkAndHolds("Six"));
    EXPECT_THAT(c.ReadToken(), StatusIs(absl::StatusCode::kFailedPrecondition));
  }
}

TEST(IOConfigTest, PrintTokenShouldPrintProperly) {
  std::stringstream ss;
  IOConfig c;
  c.SetOutputStream(ss);
  MORIARTY_EXPECT_OK(c.PrintToken("Hello!"));
  EXPECT_EQ(ss.str(), "Hello!");
}

}  // namespace
}  // namespace librarian
}  // namespace moriarty
