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

#ifndef MORIARTY_SRC_SIMPLE_IO_H_
#define MORIARTY_SRC_SIMPLE_IO_H_

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "src/exporter.h"
#include "src/importer.h"
#include "src/librarian/io_config.h"

namespace moriarty {

// StringLiteral
//
// A string literal. Stores an std::string and can cast to an std::string.
class StringLiteral {
 public:
  explicit StringLiteral(absl::string_view literal) : literal_(literal) {}
  explicit operator std::string() const { return literal_; }

  bool operator==(const StringLiteral& other) const {
    return literal_ == other.literal_;
  }

 private:
  std::string literal_;
};

// Token
//
// Each token in SimpleIO must be one of the following:
//  - `std::string`  : The name of a variable
//  - `StringLiteral`: An exact string to be printed/read.
using SimpleIOToken = std::variant<std::string, StringLiteral>;

class SimpleIOExporter;  // Forward declaration.
class SimpleIOImporter;  // Forward declaration.

// SimpleIO
//
// For many situations, we just simply need the test cases to be read/write from
// a stream in a predictable way. `SimpleIO` works on tokens and lines. Each
// line is a sequence of tokens. The corresponding `Importer()` and `Exporter()`
// will decide how the tokens are separated on each line.
class SimpleIO {
 public:
  // AddLine()
  //
  // For each test case, all tokens here will be with a single space
  // between them, followed by '\n'.
  //
  // For example:
  //   SimpleIO()
  //       .AddLine("N", "X")
  //       .AddLine(StringLiteral("Hello"), "P")
  //       .AddLine("A");
  //
  // states that each test case has 3 lines: (1) the variables `N` and `X`,
  // (2) the string "Hello" and the variable `P`, (3) the variable `A`.
  template <typename... Tokens>
  SimpleIO& AddLine(Tokens&&... token);

  // AddHeaderLine()
  //
  // These lines appear before all test cases. Similar format to `AddLine()`.
  template <typename... Tokens>
  SimpleIO& AddHeaderLine(Tokens&&... token);

  // AddFooterLine()
  //
  // These lines appear after all test cases. Similar format to `AddLine()`.
  template <typename... Tokens>
  SimpleIO& AddFooterLine(Tokens&&... token);

  // WithNumberOfTestCasesInHeader()
  //
  // The first line of the header (regardless of other calls to
  // `AddHeaderLine()`) will be a line containing a single integer, the number
  // of test cases.
  SimpleIO& WithNumberOfTestCasesInHeader();

  // Exporter()
  //
  // Creates a SimpleIOExporter from the configuration provided by this class.
  // The output will be printed to `os`.
  [[nodiscard]] SimpleIOExporter Exporter(std::ostream& os) const;

  // Importer()
  //
  // Creates a SimpleIOImporter from the configuration provided by this class.
  // The output will be read from `is`.
  [[nodiscard]] SimpleIOImporter Importer(std::istream& is) const;

  // Access the lines
  using Line = std::vector<SimpleIOToken>;
  const std::vector<Line>& LinesInHeader() const;
  const std::vector<Line>& LinesPerTestCase() const;
  const std::vector<Line>& LinesInFooter() const;
  bool HasNumberOfTestCasesInHeader() const;

 private:
  std::vector<Line> lines_in_header_;
  std::vector<Line> lines_per_test_case_;
  std::vector<Line> lines_in_footer_;

  bool has_number_of_test_cases_in_header_ = false;

  template <typename... Tokens>
  std::vector<SimpleIOToken> GetTokens(Tokens&&... token);
};

// SimpleIOImporter
//
// Imports test cases using `SimpleIO` to orchestrate how to read the data.
// By default, this importer will only read one test case. To alter this, use
// either `WithNumberOfTestCasesInHeader` or `SetNumTestCases`.
class SimpleIOImporter : public Importer {
 public:
  explicit SimpleIOImporter(SimpleIO simple_io, std::istream& is);

  // StartImport()
  //
  // Reads the header lines.
  absl::Status StartImport() override;

  // ImportTestCase()
  //
  // Reads lines from `AddLine()`.
  absl::Status ImportTestCase() override;

  // EndImport()
  //
  // Reads the footer lines.
  absl::Status EndImport() override;

  // SetNumTestCases()
  //
  // Sets the number of test cases to import. Default = 1.
  void SetNumTestCases(int num_test_cases);

 private:
  SimpleIO simple_io_;
  librarian::IOConfig io_config_;
  int num_test_cases_ = 1;

  absl::Status ReadLines(absl::Span<const SimpleIO::Line> lines);
  absl::Status ReadLine(const std::vector<SimpleIOToken>& line);
  absl::Status ReadToken(const SimpleIOToken& token);
  absl::Status ReadVariable(absl::string_view variable_name);
};

// SimpleIOExporter
//
// Exports test cases using `SimpleIO` to orchestrate how to print the data.
class SimpleIOExporter : public Exporter {
 public:
  explicit SimpleIOExporter(SimpleIO simple_io, std::ostream& os);

  // StartExport()
  //
  // Prints the header lines.
  void StartExport() override;

  // ExportTestCase()
  //
  // Prints lines from `AddLine()`.
  void ExportTestCase() override;

  // EndExport()
  //
  // Prints the footer lines.
  void EndExport() override;

 private:
  SimpleIO simple_io_;
  librarian::IOConfig io_config_;

  void PrintLines(absl::Span<const SimpleIO::Line> lines);
  void PrintLine(const std::vector<SimpleIOToken>& line);
  void PrintToken(const SimpleIOToken& token);
  void PrintVariable(absl::string_view variable_name);
};

template <typename... Tokens>
std::vector<SimpleIOToken> SimpleIO::GetTokens(Tokens&&... token) {
  std::vector<SimpleIOToken> line;
  line.reserve(sizeof...(Tokens));
  (line.push_back(std::move(token)), ...);
  return line;
}

template <typename... Tokens>
SimpleIO& SimpleIO::AddLine(Tokens&&... token) {
  lines_per_test_case_.push_back(
      GetTokens(std::forward<Tokens>(token)...));
  return *this;
}

template <typename... Tokens>
SimpleIO& SimpleIO::AddHeaderLine(Tokens&&... token) {
  lines_in_header_.push_back(GetTokens(std::forward<Tokens>(token)...));
  return *this;
}

template <typename... Tokens>
SimpleIO& SimpleIO::AddFooterLine(Tokens&&... token) {
  lines_in_footer_.push_back(GetTokens(std::forward<Tokens>(token)...));
  return *this;
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_SIMPLE_IO_H_
