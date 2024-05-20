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

#include <cassert>
#include <cctype>
#include <istream>
#include <ostream>
#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "src/errors.h"

namespace moriarty {
namespace librarian {

IOConfig& IOConfig::SetWhitespacePolicy(IOConfig::WhitespacePolicy policy) {
  whitespace_policy_ = policy;
  return *this;
}

IOConfig::WhitespacePolicy IOConfig::GetWhitespacePolicy() const {
  return whitespace_policy_;
}

namespace {

absl::Status ReadSingleChar(std::istream& is, char expected) {
  char c = is.peek();

  auto get_name_for_error_msg = [](char c) -> std::string {
    switch (c) {
      case '\n':
        return "\\n";
      case '\t':
        return "\\t";
    }
    return std::string(1, c);
  };

  if (is.eof())
    return absl::FailedPreconditionError(absl::Substitute(
        "Expected '$0', but got EOF.", get_name_for_error_msg(expected)));
  if (!is)
    return absl::FailedPreconditionError(
        absl::Substitute("Expected '$0', but got a non-EOF std::istream error.",
                         get_name_for_error_msg(expected)));

  if (c != expected)
    return absl::InvalidArgumentError(absl::Substitute(
        "Expected '$0', but got '$1'.", get_name_for_error_msg(expected),
        get_name_for_error_msg(c)));

  is.get();
  return absl::OkStatus();
}

char GetChar(Whitespace whitespace) {
  switch (whitespace) {
    case Whitespace::kNewline:
      return '\n';
    case Whitespace::kTab:
      return '\t';
    case Whitespace::kSpace:
      return ' ';
  }
  assert(false);
}

}  // namespace

absl::Status IOConfig::ReadWhitespace(Whitespace whitespace) {
  if (GetWhitespacePolicy() == WhitespacePolicy::kIgnoreWhitespace)
    return absl::OkStatus();

  if (!is_) {
    return MisconfiguredError("IOConfig", "ReadWhitespace",
                              InternalConfigurationType::kInputStream);
  }

  return ReadSingleChar(*is_, GetChar(whitespace));
}

absl::StatusOr<std::string> IOConfig::ReadToken() {
  if (!is_) {
    return MisconfiguredError("IOConfig", "ReadToken",
                              InternalConfigurationType::kInputStream);
  }

  if (GetWhitespacePolicy() == WhitespacePolicy::kExact) {
    char c = is_->peek();
    if (is_->eof())
      return absl::FailedPreconditionError(
          "Attempted to read a token, but got EOF.");
    if (std::isspace(c))
      return absl::FailedPreconditionError(
          "Attempted to read a token, but got whitespace instead.");
  }

  std::string s;
  if (!(*is_ >> s)) {
    if (is_->eof())
      return absl::FailedPreconditionError(
          "Attempted to read a token, but read EOF.");
    return absl::FailedPreconditionError(
        "Attempted to read a token, but got a non-EOF std::istream error.");
  }

  return s;
}

absl::Status IOConfig::PrintWhitespace(Whitespace whitespace) {
  if (!os_) {
    return MisconfiguredError("IOConfig", "PrintWhitespace",
                              InternalConfigurationType::kOutputStream);
  }

  *os_ << GetChar(whitespace);
  return absl::OkStatus();
}

absl::Status IOConfig::PrintToken(absl::string_view token) {
  if (!os_) {
    return MisconfiguredError("IOConfig", "PrintToken",
                              InternalConfigurationType::kOutputStream);
  }

  *os_ << token;
  return absl::OkStatus();
}

IOConfig& IOConfig::SetInputStream(std::istream& is) {
  is_ = &is;
  return *this;
}

IOConfig& IOConfig::SetOutputStream(std::ostream& os) {
  os_ = &os;
  return *this;
}

}  // namespace librarian
}  // namespace moriarty
