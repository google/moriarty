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

#ifndef MORIARTY_SRC_LIBRARIAN_IO_CONFIG_H_
#define MORIARTY_SRC_LIBRARIAN_IO_CONFIG_H_

#include <istream>
#include <ostream>
#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"

namespace moriarty {

enum class Whitespace { kSpace, kTab, kNewline };

namespace librarian {

class IOConfig {
 public:
  enum class WhitespacePolicy { kExact, kIgnoreWhitespace };

  // SetWhitespacePolicy()
  //
  // Sets the configuration around how strict to be with whitespace. See
  // `ReadToken()` for more information.
  IOConfig& SetWhitespacePolicy(WhitespacePolicy policy);

  // GetWhitespacePolicy()
  //
  // Returns the whitespace policy. Default = `kExact`.
  [[nodiscard]] WhitespacePolicy GetWhitespacePolicy() const;

  // ReadWhitespace()
  //
  // Attempts to reads the next whitespace character from the input stream. If
  // reading is not successful, future calls to this IOConfig without resetting
  // the input stream is undefined.
  //
  // If `WhitespaceMode() == kIgnoreWhitespace`, then this function is a no-op.
  absl::Status ReadWhitespace(Whitespace whitespace);

  // ReadToken()
  //
  // Reads the next token in the input stream.
  //
  // If there is whitespace before the next token:
  //  * If `GetWhitespacePolicy() == kIgnoreWhitespace`, then leading whitespace
  //    will be ignored.
  //  * If `GetWhitespacePolicy() == kExact`, then a non-OK status will be
  //    thrown.
  absl::StatusOr<std::string> ReadToken();

  // PrintWhitespace()
  //
  // Prints the whitespace character to the output stream.
  absl::Status PrintWhitespace(Whitespace whitespace);

  // PrintToken()
  //
  // Prints a single token to the output stream.
  absl::Status PrintToken(absl::string_view token);

  // SetInputStream()
  //
  // Sets the input stream to `is`.
  IOConfig& SetInputStream(std::istream& is);

  // SetOutputStream()
  //
  // Sets the output stream to `os`.
  IOConfig& SetOutputStream(std::ostream& os);

 private:
  WhitespacePolicy whitespace_policy_ = WhitespacePolicy::kExact;

  std::ostream* os_ = nullptr;
  std::istream* is_ = nullptr;
};

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_LIBRARIAN_IO_CONFIG_H_
